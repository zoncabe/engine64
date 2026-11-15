#include <assert.h>
#include <malloc.h>
#include <math.h>
#include <string.h>

#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmodel.h>

#include "model/e64_mesh_deform.h"
#include "entity/e64_entity.h"

namespace e64 {
namespace mesh {
namespace deform {

/* Positions live in the vertex buffer as int16, so quantising the source the
   same way turns the match into an exact integer compare: no tolerance to
   pick, and no risk of welding two points that are merely close. */

typedef struct {
	int16_t pos[3];
	uint16_t source; /* source index + 1; 0 marks an empty bucket */
} Bucket;


static void quantize(Vector3 v, float scale, int16_t out[3])
{
	out[0] = (int16_t)lroundf(v.x * scale);
	out[1] = (int16_t)lroundf(v.y * scale);
	out[2] = (int16_t)lroundf(v.z * scale);
}

/* t3d keeps the normal as 5,6,5 bits with the axes scaled by 15.5, 31.5 and
   15.5 — the same packing the model importer writes at build time. */
static uint16_t packNormal(Vector3 n, float sign)
{
	static const float scale[3] = { 15.5f, 31.5f, 15.5f };
	static const float low[3] = { -16.0f, -32.0f, -16.0f };
	static const float high[3] = { 15.0f, 31.0f, 15.0f };

	float axis[3] = { n.x * sign, n.y * sign, n.z * sign };
	int packed[3];

	for (int i = 0; i < 3; i++) {
		float v = roundf(axis[i] * scale[i]);
		if (v < low[i]) v = low[i];
		if (v > high[i]) v = high[i];
		packed[i] = (int)v;
	}

	return (uint16_t)(((packed[0] & 0x1F) << 11)
	                | ((packed[1] & 0x3F) << 5)
	                | ((packed[2] & 0x1F) ));
}

static Vector3 unpackNormal(uint16_t packed)
{
	/* Sign-extend each field, then undo the per-axis scale. */
	int x = (int)((packed >> 11) & 0x1F); if (x & 0x10) x -= 0x20;
	int y = (int)((packed >> 5) & 0x3F); if (y & 0x20) y -= 0x40;
	int z = (int)( packed & 0x1F); if (z & 0x10) z -= 0x20;

	return (Vector3){ x / 15.5f, y / 31.5f, z / 15.5f };
}

static uint32_t hash(const int16_t pos[3])
{
	/* FNV-1a over the six bytes of the quantised position */
	uint32_t h = 2166136261u;

	for (int i = 0; i < 3; i++) {
		uint16_t bits = (uint16_t)pos[i];
		h ^= bits & 0xFF; h *= 16777619u;
		h ^= bits >> 8; h *= 16777619u;
	}
	return h;
}

static void insert(Bucket *bucket, uint32_t mask, const int16_t pos[3], uint16_t source)
{
	uint32_t i = hash(pos) & mask;

	while (bucket[i].source != 0) {
		/* Duplicated source points would map every slot to the first one
		   anyway, so keeping the earlier entry is the stable choice. */
		if (memcmp(bucket[i].pos, pos, sizeof(int16_t) * 3) == 0) return;
		i = (i + 1) & mask;
	}

	memcpy(bucket[i].pos, pos, sizeof(int16_t) * 3);
	bucket[i].source = source + 1;
}

static uint16_t find(const Bucket *bucket, uint32_t mask, const int16_t pos[3])
{
	uint32_t i = hash(pos) & mask;

	while (bucket[i].source != 0) {
		if (memcmp(bucket[i].pos, pos, sizeof(int16_t) * 3) == 0) return bucket[i].source - 1;
		i = (i + 1) & mask;
	}
	return Deform::UNBOUND;
}


/* The RSP's segment table is one, shared by every deform, and the update
   binds every frame's buffer before running any display list: with a common
   id the last binding would feed its vertices to all of them. Each binding
   holds its own id for as long as it lives. */
static uint8_t segment_used;

static uint8_t acquireSegment(void)
{
	for (uint8_t id = Deform::SEGMENT; id <= 7; id++) {
		if (!(segment_used & (1u << id))) {
			segment_used |= (uint8_t)(1u << id);
			return id;
		}
	}
	return 0;
}


/* ------------------------------------------------------------------------ */
/* The binding                                                               */
/* ------------------------------------------------------------------------ */

/* Matches the model's rest pose against the source points. Returns false
   only if the binding could not be allocated; a partial match counts as
   success and leaves the unmatched slots frozen. */
static bool bind(Deform *deform, T3DModel *model,
                 const Vector3 *source, const Vector3 *source_normal,
                 uint16_t source_count, float scale)
{
	*deform = (Deform){
		.scale = scale, .model = model, .source = source,
		.source_normal = source_normal, .normal_sign = 1.0f,
	};

	if (model == NULL || source == NULL || source_count == 0) return false;

	deform->slot_count = model->totalVertCount;
	deform->slot_source = (uint16_t *)malloc(sizeof(uint16_t) * deform->slot_count);
	if (deform->slot_source == NULL) return false;

	/* Load factor stays under 0.5, so the linear probes stay short. */
	uint32_t capacity = 16;
	while (capacity < (uint32_t)source_count * 2) capacity *= 2;

	Bucket *bucket = (Bucket *)calloc(capacity, sizeof(Bucket));
	if (bucket == NULL) {
		free(deform->slot_source);
		deform->slot_source = NULL;
		return false;
	}

	for (uint16_t i = 0; i < source_count; i++) {
		int16_t pos[3];
		quantize(source[i], scale, pos);
		insert(bucket, capacity - 1, pos, i);
	}

	T3DVertPacked *verts = t3d_model_get_vertices(model);

	/* Summed over every bound slot: positive if the source's normals point the
	   same way as the model's, negative if the winding is reversed. */
	float agreement = 0.0f;

	for (uint16_t slot = 0; slot < deform->slot_count; slot++) {
		const int16_t *pos = t3d_vertbuffer_get_pos(verts, slot);
		uint16_t match = find(bucket, capacity - 1, pos);

		deform->slot_source[slot] = match;
		if (match == Deform::UNBOUND) continue;

		deform->bound_count++;

		if (source_normal) {
			Vector3 own = unpackNormal(*t3d_vertbuffer_get_norm(verts, slot));
			Vector3 src = source_normal[match];

			agreement += own.x*src.x + own.y*src.y + own.z*src.z;
		}
	}

	if (source_normal && agreement < 0.0f) deform->normal_sign = -1.0f;

	free(bucket);

	/* Cached, written back by hand at the end of every write. Uncached would
	   save the writeback, but the packed normal is a lone 16-bit store and
	   partial uncached writes do not survive on hardware: the positions, three
	   halves in a row, went through while the normals silently vanished. */
	deform->vertex_bytes = sizeof(T3DVertPacked) * (((uint32_t)deform->slot_count + 1) / 2);
	deform->segment = acquireSegment();
	if (deform->segment == 0) return false;

	for (int i = 0; i < Deform::BUFFERS; i++) {
		deform->vertex_buffer[i] = (T3DVertPacked *)memalign(16, deform->vertex_bytes);
		if (deform->vertex_buffer[i] == NULL) return false;

		memcpy(deform->vertex_buffer[i], verts, deform->vertex_bytes);
	}

	/* From here the recorded display lists reach the vertices through the
	   segment, so select decides which copy they read. */
	T3DModelIter it = t3d_model_iter_create(model, T3D_CHUNK_TYPE_OBJECT);
	while (t3d_model_iter_next(&it))
		t3d_model_make_object_vert_placeholder(model, it.object, deform->segment);

	return true;
}


/* Points the segment at the copy just written. The segment is this binding's
   own, so it holds that copy until the next write: the draws of every frame
   in between, a pause included, read the same vertices. */
static void select(const Deform *deform, uint8_t fb_index)
{
	if (deform->vertex_buffer[0] == NULL) return;

	t3d_segment_set(deform->segment, deform->vertex_buffer[fb_index % Deform::BUFFERS]);
}


static void write(const Deform *deform, uint8_t fb_index)
{
	const Vector3 *source = deform->source;

	if (deform->slot_source == NULL || source == NULL) return;

	/* This frame's copy, not the model's buffer: the RSP is still reading the
	   one the previous frame was drawn from. */
	T3DVertPacked *verts = deform->vertex_buffer[fb_index % Deform::BUFFERS];
	if (verts == NULL) return;

	const Vector3 *source_normal = deform->source_normal;
	const uint8_t *source_rgba = deform->source_rgba;

	for (uint16_t slot = 0; slot < deform->slot_count; slot++) {
		uint16_t index = deform->slot_source[slot];
		if (index == Deform::UNBOUND) continue;

		quantize(source[index], deform->scale, t3d_vertbuffer_get_pos(verts, slot));

		if (source_normal) {
			*t3d_vertbuffer_get_norm(verts, slot) =
				packNormal(source_normal[index], deform->normal_sign);
		}

		if (source_rgba) {
			uint8_t *rgba = t3d_vertbuffer_get_rgba(verts, slot);
			const uint8_t *color = &source_rgba[index * 4];
			rgba[0] = color[0];
			rgba[1] = color[1];
			rgba[2] = color[2];
			rgba[3] = color[3];
		}
	}

	/* The buffer is 16-byte aligned and its length is a multiple of 32, so the
	   range covers whole cache lines and nothing is left dirty behind the DMA.
	   Safe to do while the RSP draws, because it is reading another copy. */
	data_cache_hit_writeback(verts, deform->vertex_bytes);

	select(deform, fb_index);
}


static void release(Deform *deform)
{
	if (deform->segment >= Deform::SEGMENT)
		segment_used &= (uint8_t)~(1u << deform->segment);

	free(deform->slot_source);

	for (int i = 0; i < Deform::BUFFERS; i++)
		free(deform->vertex_buffer[i]);

	*deform = (Deform){};
}


/* ------------------------------------------------------------------------ */
/* The mesh side                                                             */
/* ------------------------------------------------------------------------ */

bool set(Mesh *mesh, const Vector3 *source, const Vector3 *source_normal,
         const uint8_t *source_rgba, uint16_t source_count, float scale)
{
	Deform *deform = (Deform *)malloc(sizeof(Deform));
	assert(deform);

	if (!bind(deform, mesh->model, source, source_normal, source_count, scale)) {
		free(deform);
		return false;
	}
	deform->source_rgba = source_rgba;

	/* The binding rewrote the model's vertex addresses as segment references,
	   and the geometry blocks were recorded before that with the old absolute
	   ones. Record them again so the draw goes through the segment. The
	   materials are untouched: they hold no vertex address. */
	for (uint16_t i = 0; T3DObject *o = object(mesh, i); i++) {
		if (o->userBlock) rspq_block_free(o->userBlock);
		o->userBlock = NULL;
	}
	recordGeometry(mesh);

	mesh->deform = deform;
	return true;
}

void update(Entity *const *entity, uint8_t count, uint8_t fb_index)
{
	for (uint8_t i = 0; i < count; i++)
		for (uint8_t j = 0; j < entity[i]->mesh_count; j++)
			if (entity[i]->mesh[j]->deform)
				write(entity[i]->mesh[j]->deform, fb_index);
}

void destroy(Mesh *mesh)
{
	if (!mesh->deform) return;

	release(mesh->deform);
	free(mesh->deform);
	mesh->deform = NULL;
}

}
}
}
