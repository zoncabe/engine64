/*
	Extracts the collision mesh from a .gltf/.glb and writes the collision
	binary (big-endian, native N64 format).

	Layout: header, indices, packed normals, vertices, active-edge bytes,
	each block padded to 4 bytes.

	Originally based on collisionBuilder.cpp from pyrite64 by Max Bebök
	(HailToDodongo), https://github.com/HailToDodongo/pyrite64, MIT licensed.
	Active-edge baking ported from JoltPhysics by Jorrit Rouwe, MIT licensed,
	https://github.com/jrouwe/JoltPhysics.
*/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

typedef struct {
	float x, y, z;
} Vec3;

typedef struct {
	int16_t x, y, z;
} PackedNormal;

typedef struct {
	Vec3         *verticesFloat;
	PackedNormal *normals;
	uint16_t     *indices;
	uint8_t      *active_edges;   /* 1 per triangle: bit 0 = v0v1, bit 1 = v1v2, bit 2 = v2v0 */
	size_t        vertex_count;
	size_t        index_count;
	size_t        normal_count;
} CollisionMesh;


static void fail(const char *message)
{
	fprintf(stderr, "Error: %s\n", message);
	exit(1);
}

static size_t bytes_written = 0;

static void write_u8(FILE *f, uint8_t v)
{
	fputc(v, f);
	bytes_written++;
}

static void write_u16(FILE *f, uint16_t v)
{
	write_u8(f, (uint8_t)(v >> 8));
	write_u8(f, (uint8_t)(v & 0xFF));
}

static void write_i16(FILE *f, int16_t v)
{
	write_u16(f, (uint16_t)v);
}

static void write_u32(FILE *f, uint32_t v)
{
	write_u16(f, (uint16_t)(v >> 16));
	write_u16(f, (uint16_t)(v & 0xFFFF));
}

static void write_f32(FILE *f, float v)
{
	uint32_t bits;
	memcpy(&bits, &v, sizeof(bits));
	write_u32(f, bits);
}

static void align4(FILE *f)
{
	while (bytes_written % 4 != 0) write_u8(f, 0);
}

/*
	Vertex welding. glTF splits a vertex whenever any attribute differs across
	it: UV seams, shading splits, material borders. Position is copied
	verbatim in every split, so duplicates are bit-identical and an exact hash
	welds all of them without the tolerance a distance test would need.
	Collision only reads positions, so the splits carry no information worth
	keeping.
*/

typedef struct {
	uint32_t *slot;      /* vertex index + 1; 0 marks an empty slot */
	size_t    capacity;  /* always a power of two */
	size_t    count;
} VertexHash;

/* -0.0f and 0.0f compare equal but carry different bits: fold them together. */
static float canonical(float v)
{
	return v == 0.0f ? 0.0f : v;
}

static uint32_t hashVertex(Vec3 v)
{
	uint32_t bits[3];
	memcpy(&bits[0], &v.x, 4);
	memcpy(&bits[1], &v.y, 4);
	memcpy(&bits[2], &v.z, 4);

	/* FNV-1a over the 12 raw bytes */
	uint32_t h = 2166136261u;
	for (int i = 0; i < 3; i++) {
		for (int b = 0; b < 4; b++) {
			h ^= (bits[i] >> (b * 8)) & 0xFF;
			h *= 16777619u;
		}
	}
	return h;
}

static int sameVertex(Vec3 a, Vec3 b)
{
	return memcmp(&a, &b, sizeof(Vec3)) == 0;
}

static void hashGrow(VertexHash *hash, const Vec3 *vertices)
{
	size_t old_capacity = hash->capacity;
	uint32_t *old_slot  = hash->slot;

	hash->capacity = old_capacity ? old_capacity * 2 : 256;
	hash->slot     = calloc(hash->capacity, sizeof(uint32_t));
	if (!hash->slot) fail("Out of memory growing the vertex hash!");

	for (size_t i = 0; i < old_capacity; i++)
	{
		if (old_slot[i] == 0) continue;

		size_t pos = hashVertex(vertices[old_slot[i] - 1]) & (hash->capacity - 1);
		while (hash->slot[pos] != 0) pos = (pos + 1) & (hash->capacity - 1);
		hash->slot[pos] = old_slot[i];
	}

	free(old_slot);
}

/* Returns the index of the canonical vertex, appending it if it is new. */
static uint16_t weldVertex(CollisionMesh *mesh, VertexHash *hash, Vec3 v)
{
	v = (Vec3){ canonical(v.x), canonical(v.y), canonical(v.z) };

	if ((hash->count + 1) * 10 >= hash->capacity * 7) hashGrow(hash, mesh->verticesFloat);

	size_t pos = hashVertex(v) & (hash->capacity - 1);
	while (hash->slot[pos] != 0)
	{
		uint32_t index = hash->slot[pos] - 1;
		if (sameVertex(mesh->verticesFloat[index], v)) return (uint16_t)index;
		pos = (pos + 1) & (hash->capacity - 1);
	}

	if (mesh->vertex_count >= 0x10000) fail("Too many vertices (>65535)!");

	mesh->verticesFloat = realloc(mesh->verticesFloat, (mesh->vertex_count + 1) * sizeof(Vec3));
	if (!mesh->verticesFloat) fail("Out of memory storing vertices!");

	mesh->verticesFloat[mesh->vertex_count] = v;
	hash->slot[pos] = (uint32_t)(mesh->vertex_count + 1);
	hash->count++;

	return (uint16_t)mesh->vertex_count++;
}

/*
	Active edges, ported from Jolt's MeshShape::sFindActiveEdges and
	ActiveEdges::IsEdgeActive (JoltPhysics, MIT licensed). An edge is active
	when a contact normal found on it is real: a border with no neighbour, a
	convex fold sharper than the threshold, or a non-manifold edge. Inactive
	edges are the internal seams of flat floors and continuous ramps; at
	runtime a contact normal landing on one is replaced by the face normal.
*/

/* Folds between two walkable faces must never produce a blocking normal, so
   the threshold is the character's walkable limit (CHARACTER_FLOOR_MAX_SLOPE),
   not Jolt's 5-degree default, which is tuned for rigid-body fidelity. */
#define ACTIVE_EDGE_COS_THRESHOLD 0.6428f   /* cos(50 degrees) */

static Vec3 vecSub(Vec3 a, Vec3 b)
{
	return (Vec3){ a.x - b.x, a.y - b.y, a.z - b.z };
}

static Vec3 vecCross(Vec3 a, Vec3 b)
{
	return (Vec3){
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x,
	};
}

static float vecDot(Vec3 a, Vec3 b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

/* Port of ActiveEdges::IsEdgeActive: normal1 belongs to the triangle whose
   winding gives edge_direction, normal2 to its neighbour. */
static int isEdgeActive(Vec3 normal1, Vec3 normal2, Vec3 edge_direction)
{
	/* If normals are opposite the edges are active (the triangles are back to back). */
	float cos_angle_normals = vecDot(normal1, normal2);
	if (cos_angle_normals < -0.999848f) return 1;   /* cos(179 degrees) */

	/* Concave edge: not active. */
	if (vecDot(vecCross(normal1, normal2), edge_direction) < 0.0f) return 0;

	/* Convex edge: active when the angle is bigger than the threshold. */
	return cos_angle_normals < ACTIVE_EDGE_COS_THRESHOLD;
}

typedef struct {
	uint32_t key;       /* (min_index << 16) | max_index; EDGE_SLOT_EMPTY marks a free slot */
	uint8_t  count;     /* triangles seen on this edge, saturating at 3 */
	uint32_t tri[2];
	uint8_t  edge[2];   /* which edge of that triangle: 0 = v0v1, 1 = v1v2, 2 = v2v0 */
} EdgeSlot;

#define EDGE_SLOT_EMPTY 0xFFFFFFFFu

/* Port of MeshShape::sFindActiveEdges over the compacted triangle list.
   face_normals holds the unit float normal of each triangle. */
static void findActiveEdges(CollisionMesh *mesh, const Vec3 *face_normals)
{
	size_t tri_count = mesh->normal_count;

	mesh->active_edges = calloc(tri_count ? tri_count : 1, 1);
	if (!mesh->active_edges) fail("Out of memory storing active edges!");

	size_t capacity = 256;
	while (capacity < tri_count * 6) capacity *= 2;

	EdgeSlot *slots = malloc(capacity * sizeof(EdgeSlot));
	if (!slots) fail("Out of memory building the edge map!");
	for (size_t i = 0; i < capacity; i++) slots[i].key = EDGE_SLOT_EMPTY;

	/* Pass 1: map every edge to the triangles that share it. From the third
	   triangle on an edge is non-manifold: active for that triangle on the
	   spot, and for the first two in pass 2. */
	for (size_t t = 0; t < tri_count; t++)
	{
		for (int e = 0; e < 3; e++)
		{
			uint16_t a = mesh->indices[t*3 + e];
			uint16_t b = mesh->indices[t*3 + (e + 1) % 3];
			uint32_t key = a < b ? ((uint32_t)a << 16) | b : ((uint32_t)b << 16) | a;

			size_t pos = (key * 2654435761u) & (capacity - 1);
			while (slots[pos].key != EDGE_SLOT_EMPTY && slots[pos].key != key)
				pos = (pos + 1) & (capacity - 1);

			EdgeSlot *slot = &slots[pos];
			if (slot->key == EDGE_SLOT_EMPTY) {
				slot->key   = key;
				slot->count = 0;
			}

			if (slot->count < 2) {
				slot->tri[slot->count]  = (uint32_t)t;
				slot->edge[slot->count] = (uint8_t)e;
				slot->count++;
			} else {
				mesh->active_edges[t] |= 1 << e;
				slot->count = 3;
			}
		}
	}

	/* Pass 2: border edges are active; edges shared by two triangles follow
	   the fold test, with the edge direction as wound by the first one. */
	for (size_t i = 0; i < capacity; i++)
	{
		const EdgeSlot *slot = &slots[i];
		if (slot->key == EDGE_SLOT_EMPTY) continue;

		int num_active;
		if (slot->count == 1) {
			num_active = 1;
		}
		else if (slot->count == 2) {
			uint32_t t1 = slot->tri[0];
			Vec3 e1 = mesh->verticesFloat[mesh->indices[t1*3 + slot->edge[0]]];
			Vec3 e2 = mesh->verticesFloat[mesh->indices[t1*3 + (slot->edge[0] + 1) % 3]];
			num_active = isEdgeActive(face_normals[t1], face_normals[slot->tri[1]], vecSub(e2, e1)) ? 2 : 0;
		}
		else {
			num_active = 2;
		}

		for (int n = 0; n < num_active; n++)
			mesh->active_edges[slot->tri[n]] |= 1 << slot->edge[n];
	}

	free(slots);
}

static Vec3 transformPoint(const cgltf_float m[16], Vec3 v)
{
	return (Vec3){
		m[0]*v.x + m[4]*v.y + m[8]*v.z  + m[12],
		m[1]*v.x + m[5]*v.y + m[9]*v.z  + m[13],
		m[2]*v.x + m[6]*v.y + m[10]*v.z + m[14],
	};
}

static int nodeSelected(const cgltf_node *node, char **meshes, int mesh_count)
{
	if (mesh_count == 0) return 1;
	if (node->name == NULL) return 0;

	for (int i = 0; i < mesh_count; i++) {
		if (strcmp(node->name, meshes[i]) == 0) return 1;
	}
	return 0;
}

static void convert(const char *gltfPath, CollisionMesh *out, float baseScale, char **meshes, int mesh_count)
{
	cgltf_options options = {0};
	cgltf_data *data = NULL;
	cgltf_result result = cgltf_parse_file(&options, gltfPath, &data);

	if (result == cgltf_result_file_not_found) fail("File not found!");
	if (result != cgltf_result_success)        fail("Failed to parse glTF file!");
	if (cgltf_validate(data) != cgltf_result_success) fail("Invalid glTF data!");

	cgltf_load_buffers(&options, data, gltfPath);

	VertexHash hash = {0};

	for (cgltf_size i = 0; i < data->nodes_count; ++i)
	{
		cgltf_node *node = &data->nodes[i];
		if (!node->mesh) continue;
		if (node->name && strncmp(node->name, "fast64_f3d_material_library", 27) == 0) continue;
		if (!nodeSelected(node, meshes, mesh_count)) continue;

		cgltf_float nodeMat[16];
		cgltf_node_transform_world(node, nodeMat);
		cgltf_mesh *mesh = node->mesh;

		for (cgltf_size j = 0; j < mesh->primitives_count; j++)
		{
			cgltf_primitive *prim = &mesh->primitives[j];
			cgltf_accessor  *pos_acc = NULL;

			for (cgltf_size k = 0; k < prim->attributes_count; k++) {
				if (prim->attributes[k].type == cgltf_attribute_type_position) {
					pos_acc = prim->attributes[k].data;
					break;
				}
			}
			if (pos_acc == NULL) continue;

			/* Weld first: the primitive's own indices need remapping afterwards. */
			uint16_t *remap = malloc(pos_acc->count * sizeof(uint16_t));
			if (!remap) fail("Out of memory building the remap table!");

			for (cgltf_size l = 0; l < pos_acc->count; l++)
			{
				cgltf_float raw[3];
				cgltf_accessor_read_float(pos_acc, l, raw, 3);
				Vec3 vert = transformPoint(nodeMat, (Vec3){raw[0], raw[1], raw[2]});

				/* The t3dm importer writes this same point rounded to int16
				   (roundf, tiny3d's Vec3::round), and the mesh deform binding
				   matches the two by quantized position. Snap to that grid
				   before scaling so both sides always land on the same value,
				   even for coordinates sitting exactly on .5. */
				vert = (Vec3){ roundf(vert.x), roundf(vert.y), roundf(vert.z) };

				remap[l] = weldVertex(out, &hash, (Vec3){
					vert.x * baseScale,
					vert.y * baseScale,
					vert.z * baseScale,
				});
			}

			if (prim->indices != NULL)
			{
				cgltf_accessor *acc = prim->indices;
				out->indices = realloc(out->indices, (out->index_count + acc->count) * sizeof(uint16_t));
				if (!out->indices) fail("Out of memory storing indices!");

				for (cgltf_size k = 0; k < acc->count; k++)
				{
					cgltf_size local = cgltf_accessor_read_index(acc, k);
					if (local >= pos_acc->count) fail("Index out of range for its primitive!");
					out->indices[out->index_count++] = remap[local];
				}
			}

			free(remap);
		} /* primitives */
	} /* nodes */

	free(hash.slot);

	if (out->index_count % 3 != 0) fail("Index count not divisible by 3!");

	/* Generate per-triangle normals, compacting away the ones without area.
	   Welding can expose degenerate triangles that were hidden behind split
	   vertices; they contribute nothing to collision, so they are dropped
	   rather than aborting the build. */
	out->normals = realloc(out->normals, (out->index_count / 3) * sizeof(PackedNormal));
	if (out->index_count && !out->normals) fail("Out of memory storing normals!");

	/* Unit float normals, kept aside for the active-edge fold test: the packed
	   ones lose precision and the test compares angles of 5 degrees. */
	Vec3 *face_normals = malloc((out->index_count / 3 ? out->index_count / 3 : 1) * sizeof(Vec3));
	if (!face_normals) fail("Out of memory storing face normals!");

	size_t write = 0;

	for (size_t v = 0; v < out->index_count; v += 3)
	{
		uint16_t ia = out->indices[v], ib = out->indices[v+1], ic = out->indices[v+2];

		if (ia == ib || ib == ic || ia == ic) continue;

		Vec3 a = out->verticesFloat[ia];
		Vec3 b = out->verticesFloat[ib];
		Vec3 c = out->verticesFloat[ic];

		Vec3 edge1 = { b.x-a.x, b.y-a.y, b.z-a.z };
		Vec3 edge2 = { c.x-a.x, c.y-a.y, c.z-a.z };

		Vec3 normal = {
			edge1.y*edge2.z - edge1.z*edge2.y,
			edge1.z*edge2.x - edge1.x*edge2.z,
			edge1.x*edge2.y - edge1.y*edge2.x,
		};
		float len = sqrtf(normal.x*normal.x + normal.y*normal.y + normal.z*normal.z);

		if (len < 0.0001f) continue;   /* no area: contributes nothing to collision */

		out->indices[write*3 + 0] = ia;
		out->indices[write*3 + 1] = ib;
		out->indices[write*3 + 2] = ic;

		face_normals[write] = (Vec3){ normal.x / len, normal.y / len, normal.z / len };

		out->normals[write] = (PackedNormal){
			(int16_t)(normal.x / len * 32767.0f),
			(int16_t)(normal.y / len * 32767.0f),
			(int16_t)(normal.z / len * 32767.0f),
		};
		write++;
	}

	out->index_count  = write * 3;
	out->normal_count = write;

	findActiveEdges(out, face_normals);
	free(face_normals);

	cgltf_free(data);
}

static void writeFile(const CollisionMesh *mesh, const char *path)
{
	FILE *f = fopen(path, "wb");
	if (!f) fail("Cannot open output file!");

	write_u32(f, (uint32_t)(mesh->index_count / 3));
	write_u32(f, (uint32_t)mesh->vertex_count);
	write_f32(f, 1.0f);
	write_u32(f, 0); /* vertex pointer */
	write_u32(f, 0); /* normals pointer */
	write_u32(f, 0); /* BVH pointer */

	for (size_t i = 0; i < mesh->index_count; i++)
		write_u16(f, mesh->indices[i]);
	align4(f);

	for (size_t i = 0; i < mesh->normal_count; i++) {
		write_i16(f, mesh->normals[i].x);
		write_i16(f, mesh->normals[i].y);
		write_i16(f, mesh->normals[i].z);
	}
	align4(f);

	for (size_t i = 0; i < mesh->vertex_count; i++) {
		write_f32(f, mesh->verticesFloat[i].x);
		write_f32(f, mesh->verticesFloat[i].y);
		write_f32(f, mesh->verticesFloat[i].z);
	}
	align4(f);

	for (size_t i = 0; i < mesh->index_count / 3; i++)
		write_u8(f, mesh->active_edges[i]);
	align4(f);

	fclose(f);
}

int main(int argc, char **argv)
{
	if (argc < 3) {
		fprintf(stderr, "Uso: %s entrada.glb salida.col [mesh ...]\n", argv[0]);
		return 1;
	}

	/* glb files are authored in render units; physics runs in metres. */
	const float base_scale = 0.01f;

	CollisionMesh mesh = {0};
	convert(argv[1], &mesh, base_scale, &argv[3], argc - 3);
	writeFile(&mesh, argv[2]);

	return 0;
}
