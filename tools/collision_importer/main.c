/**
* collision_importer — extracts the collision mesh from a .gltf/.glb
* and writes the collision binary (big-endian, native N64 format).
*
* Based on collisionBuilder.cpp from pyrite64 by Max Bebök (HailToDodongo)
* https://github.com/HailToDodongo/pyrite64
* Original code licensed under the MIT license.
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

/* ---- conversion (port of convert() from pyrite64's collisionBuilder) ---- */

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
			size_t baseIndex = out->vertex_count;
			if (baseIndex >= 0x10000) fail("Too many vertices (>65535)!");

			cgltf_primitive *prim = &mesh->primitives[j];

			if (prim->indices != NULL)
			{
				cgltf_accessor *acc = prim->indices;
				out->indices = realloc(out->indices, (out->index_count + acc->count) * sizeof(uint16_t));
				for (cgltf_size k = 0; k < acc->count; k++) {
					out->indices[out->index_count++] = (uint16_t)(baseIndex + cgltf_accessor_read_index(acc, k));
				}
			}

			for (cgltf_size k = 0; k < prim->attributes_count; k++)
			{
				cgltf_attribute *attr = &prim->attributes[k];
				if (attr->type != cgltf_attribute_type_position) continue;

				cgltf_accessor *acc = attr->data;
				out->verticesFloat = realloc(out->verticesFloat, (out->vertex_count + acc->count) * sizeof(Vec3));
				for (cgltf_size l = 0; l < acc->count; l++) {
					cgltf_float raw[3];
					cgltf_accessor_read_float(acc, l, raw, 3);
					Vec3 vert = transformPoint(nodeMat, (Vec3){raw[0], raw[1], raw[2]});
					out->verticesFloat[out->vertex_count++] = (Vec3){
						vert.x * baseScale,
						vert.y * baseScale,
						vert.z * baseScale,
					};
				}
			}
		} /* primitives */
	} /* nodes */

	if (out->index_count % 3 != 0) fail("Index count not divisible by 3!");

	/* generate normals */
	out->normals = realloc(out->normals, (out->index_count / 3) * sizeof(PackedNormal));
	for (size_t v = 0; v < out->index_count; v += 3)
	{
		Vec3 a = out->verticesFloat[out->indices[v]];
		Vec3 b = out->verticesFloat[out->indices[v+1]];
		Vec3 c = out->verticesFloat[out->indices[v+2]];

		Vec3 edge1 = { b.x-a.x, b.y-a.y, b.z-a.z };
		Vec3 edge2 = { c.x-a.x, c.y-a.y, c.z-a.z };

		Vec3 normal = {
			edge1.y*edge2.z - edge1.z*edge2.y,
			edge1.z*edge2.x - edge1.x*edge2.z,
			edge1.x*edge2.y - edge1.y*edge2.x,
		};
		float len = sqrtf(normal.x*normal.x + normal.y*normal.y + normal.z*normal.z);

		if (len < 0.0001f) {
			fprintf(stderr, "Degenerate triangle:\nA: %.4f %.4f %.4f\nB: %.4f %.4f %.4f\nC: %.4f %.4f %.4f\n",
				a.x, a.y, a.z, b.x, b.y, b.z, c.x, c.y, c.z);
			fprintf(stderr, "Indices: %u %u %u\n", out->indices[v], out->indices[v+1], out->indices[v+2]);
			fail("Degenerate triangle!");
		}

		out->normals[out->normal_count++] = (PackedNormal){
			(int16_t)(normal.x / len * 32767.0f),
			(int16_t)(normal.y / len * 32767.0f),
			(int16_t)(normal.z / len * 32767.0f),
		};
	}

	cgltf_free(data);
}

/* ---- binary output ---- */

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

	fprintf(stderr, "%s: %zu vertices, %zu triangulos -> %s (%zu bytes)\n",
		argv[1], mesh.vertex_count, mesh.index_count / 3, argv[2], bytes_written);
	return 0;
}
