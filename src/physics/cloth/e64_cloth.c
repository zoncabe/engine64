#include <malloc.h>
#include <math.h>
#include <string.h>

#include "physics/cloth/e64_cloth.h"


/*
	Edge extraction: one constraint per unique mesh edge. On a triangulated
	grid that yields the structural edges plus the diagonal of every quad, and
	the diagonal is what resists shearing, so it comes free with the
	triangulation.
*/

#define EDGE_EMPTY 0xFFFFFFFFu

static uint32_t cloth_edgeKey(uint16_t a, uint16_t b)
{
	return a < b ? ((uint32_t)a << 16) | b
	             : ((uint32_t)b << 16) | a;
}

/* Returns true when the edge had not been seen yet. */
static bool cloth_edgeIsNew(uint32_t *table, uint32_t mask, uint32_t key)
{
	uint32_t i = (key * 2654435761u) & mask;   /* Knuth multiplicative hash */

	while (table[i] != EDGE_EMPTY) {
		if (table[i] == key) return false;
		i = (i + 1) & mask;
	}

	table[i] = key;
	return true;
}


static void cloth_updateNormals(Cloth *cloth);


/* The edge held in place: everything at or left of the def's X threshold. */
static bool cloth_atPinnedEdge(Vector3 position, void *user)
{
	return position.x <= *(const float *)user;
}


bool cloth_create(Cloth *cloth, const CollisionMesh *mesh, const ClothDef *def)
{
	*cloth = (Cloth){ .iterations = 4 };   /* Jakobsen: 3-4 relaxation passes */

	if (mesh == NULL || mesh->vertex_count == 0 || mesh->triangle_count == 0) return false;

	cloth->particle_count = mesh->vertex_count;
	cloth->position = malloc(sizeof(Vector3) * cloth->particle_count);
	cloth->previous = malloc(sizeof(Vector3) * cloth->particle_count);
	cloth->force    = malloc(sizeof(Vector3) * cloth->particle_count);
	cloth->normal   = malloc(sizeof(Vector3) * cloth->particle_count);
	cloth->render_position = malloc(sizeof(Vector3) * cloth->particle_count);
	cloth->pinned   = calloc(cloth->particle_count, sizeof(uint8_t));

	cloth->triangle_count = mesh->triangle_count;
	cloth->triangle = malloc(sizeof(uint16_t) * 3 * cloth->triangle_count);
	if (cloth->triangle)
		memcpy(cloth->triangle, mesh->indices, sizeof(uint16_t) * 3 * cloth->triangle_count);

	/* At rest both Verlet slots hold the same position, so velocity is zero. */
	if (cloth->position && cloth->previous && cloth->render_position) {
		memcpy(cloth->position, mesh->vertices, sizeof(Vector3) * cloth->particle_count);
		memcpy(cloth->previous, mesh->vertices, sizeof(Vector3) * cloth->particle_count);
		memcpy(cloth->render_position, mesh->vertices, sizeof(Vector3) * cloth->particle_count);
	}

	uint32_t edge_max = (uint32_t)mesh->triangle_count * 3;

	uint32_t capacity = 16;
	while (capacity < edge_max * 2) capacity *= 2;

	uint32_t *table = malloc(sizeof(uint32_t) * capacity);
	cloth->constraint = malloc(sizeof(ClothConstraint) * edge_max);

	if (!cloth->position || !cloth->previous || !cloth->force || !cloth->pinned
	    || !cloth->render_position || !cloth->triangle || !table || !cloth->constraint) {
		free(table);
		cloth_delete(cloth);
		return false;
	}

	memset(table, 0xFF, sizeof(uint32_t) * capacity);

	for (uint16_t t = 0; t < mesh->triangle_count; t++)
	{
		const uint16_t *tri = &mesh->indices[t * 3];

		for (int e = 0; e < 3; e++)
		{
			uint16_t a = tri[e];
			uint16_t b = tri[(e + 1) % 3];
			if (a == b) continue;

			if (!cloth_edgeIsNew(table, capacity - 1, cloth_edgeKey(a, b))) continue;

			Vector3 delta = vector3_difference(&cloth->position[a], &cloth->position[b]);
			float   rest  = delta.x*delta.x + delta.y*delta.y + delta.z*delta.z;

			/* A zero-length edge would divide by zero in the projection. */
			if (rest <= 0.0f) continue;

			cloth->constraint[cloth->constraint_count++] = (ClothConstraint){
				.a = a, .b = b, .rest_length_sq = rest,
			};
		}
	}

	free(table);

	if (def) {
		cloth->damping = def->damping;
		if (def->iterations) cloth->iterations = def->iterations;

		float pin_max_x = def->pin_max_x;
		cloth_pinWhere(cloth, cloth_atPinnedEdge, &pin_max_x);
	}

	/* The rest normals have to exist before anything binds to the cloth: that
	   is what lets the binding tell whether the mesh's own normals agree with
	   the winding of this triangle list. */
	cloth_updateNormals(cloth);

	return true;
}


void cloth_pinWhere(Cloth *cloth, bool (*predicate)(Vector3 position, void *user), void *user)
{
	if (cloth->pinned == NULL || predicate == NULL) return;

	for (uint16_t i = 0; i < cloth->particle_count; i++)
		cloth->pinned[i] = predicate(cloth->position[i], user) ? 1 : 0;
}


/*
	Integration: Jakobsen's step, with the previous position standing in for
	velocity:

		x += x - oldx + a * dt * dt;
		oldx = temp;

	Damping scales the implied velocity term instead of adding a force, which
	keeps the step to one multiply-add per axis.
*/
/*
	Wind acts on the surface, not on the points. For each triangle the raw
	cross product gives a normal whose length is twice the area, and the unit
	normal says how squarely the face meets the wind:

		force = normal * dot(normalize(normal), wind)

	That product is what makes a cloth turn into the wind on its own: a face
	edge-on has dot near zero and catches nothing, so the faces still exposed
	drag it around until the whole sheet lines up. A uniform push per particle
	cannot do that, because it applies no torque.

	Method from Mosegaard's cloth tutorial (addWindForcesForTriangle).
*/
static void cloth_applyWind(Cloth *cloth)
{
	memset(cloth->force, 0, sizeof(Vector3) * cloth->particle_count);

	if (cloth->wind.x == 0.0f && cloth->wind.y == 0.0f && cloth->wind.z == 0.0f) return;

	for (uint16_t t = 0; t < cloth->triangle_count; t++)
	{
		const uint16_t *tri = &cloth->triangle[t * 3];

		Vector3 a = cloth->position[tri[0]];
		Vector3 edge1 = { cloth->position[tri[1]].x - a.x,
		                  cloth->position[tri[1]].y - a.y,
		                  cloth->position[tri[1]].z - a.z };
		Vector3 edge2 = { cloth->position[tri[2]].x - a.x,
		                  cloth->position[tri[2]].y - a.y,
		                  cloth->position[tri[2]].z - a.z };

		Vector3 normal = {
			edge1.y*edge2.z - edge1.z*edge2.y,
			edge1.z*edge2.x - edge1.x*edge2.z,
			edge1.x*edge2.y - edge1.y*edge2.x,
		};

		float len_sq = normal.x*normal.x + normal.y*normal.y + normal.z*normal.z;
		if (len_sq <= 0.0f) continue;

		/* dot(normal, wind) / |normal| — the unit normal without building it. */
		float facing = (normal.x*cloth->wind.x + normal.y*cloth->wind.y + normal.z*cloth->wind.z)
		             / sqrtf(len_sq);

		Vector3 force = { normal.x * facing, normal.y * facing, normal.z * facing };

		for (int v = 0; v < 3; v++) {
			cloth->force[tri[v]].x += force.x;
			cloth->force[tri[v]].y += force.y;
			cloth->force[tri[v]].z += force.z;
		}
	}
}


/* Area-weighted vertex normals: the raw cross product is already twice the
   face area, so accumulating it unnormalised weights each face by its size,
   which is what keeps a stretched triangle from swinging the result. */
static void cloth_updateNormals(Cloth *cloth)
{
	if (cloth->normal == NULL) return;

	memset(cloth->normal, 0, sizeof(Vector3) * cloth->particle_count);

	for (uint16_t t = 0; t < cloth->triangle_count; t++)
	{
		const uint16_t *tri = &cloth->triangle[t * 3];

		Vector3 a = cloth->position[tri[0]];
		Vector3 edge1 = { cloth->position[tri[1]].x - a.x,
		                  cloth->position[tri[1]].y - a.y,
		                  cloth->position[tri[1]].z - a.z };
		Vector3 edge2 = { cloth->position[tri[2]].x - a.x,
		                  cloth->position[tri[2]].y - a.y,
		                  cloth->position[tri[2]].z - a.z };

		Vector3 face = {
			edge1.y*edge2.z - edge1.z*edge2.y,
			edge1.z*edge2.x - edge1.x*edge2.z,
			edge1.x*edge2.y - edge1.y*edge2.x,
		};

		for (int v = 0; v < 3; v++) {
			cloth->normal[tri[v]].x += face.x;
			cloth->normal[tri[v]].y += face.y;
			cloth->normal[tri[v]].z += face.z;
		}
	}

	for (uint16_t i = 0; i < cloth->particle_count; i++) {
		Vector3 *n = &cloth->normal[i];
		float len_sq = n->x*n->x + n->y*n->y + n->z*n->z;

		if (len_sq <= 0.0f) { *n = (Vector3){ 0.0f, 0.0f, 1.0f }; continue; }

		float inv = 1.0f / sqrtf(len_sq);
		n->x *= inv;  n->y *= inv;  n->z *= inv;
	}
}


static void cloth_integrate(Cloth *cloth, float dt)
{
	float dt2    = dt * dt;
	float retain = 1.0f - cloth->damping;

	for (uint16_t i = 0; i < cloth->particle_count; i++)
	{
		if (cloth->pinned[i]) continue;

		Vector3 current = cloth->position[i];

		Vector3 accel = { cloth->gravity.x + cloth->force[i].x,
		                  cloth->gravity.y + cloth->force[i].y,
		                  cloth->gravity.z + cloth->force[i].z };

		cloth->position[i].x += (current.x - cloth->previous[i].x) * retain + accel.x * dt2;
		cloth->position[i].y += (current.y - cloth->previous[i].y) * retain + accel.y * dt2;
		cloth->position[i].z += (current.z - cloth->previous[i].z) * retain + accel.z * dt2;

		cloth->previous[i] = current;
	}
}


/*
	Constraint relaxation. The exact projection needs the current distance, and
	therefore a root:

		deltalength = sqrt(delta * delta);
		diff        = (deltalength - restlength) / deltalength;
		x1 -= delta * 0.5 * diff;
		x2 += delta * 0.5 * diff;

	Jakobsen's approximation replaces it with the first-order Taylor expansion
	around the rest length, which is exact where it matters most because a
	satisfied constraint sits precisely there:

		delta *= restlength * restlength / (delta * delta + restlength * restlength) - 0.5;
		x1 -= delta;
		x2 += delta;

	One division per constraint, no roots. The factor already carries the half
	share, so a particle facing a pinned neighbour takes twice as much.
*/
static void cloth_satisfy(Cloth *cloth)
{
	for (uint8_t pass = 0; pass < cloth->iterations; pass++)
	{
		for (uint16_t i = 0; i < cloth->constraint_count; i++)
		{
			const ClothConstraint *c = &cloth->constraint[i];

			bool pin_a = cloth->pinned[c->a];
			bool pin_b = cloth->pinned[c->b];
			if (pin_a && pin_b) continue;

			Vector3 *x1 = &cloth->position[c->a];
			Vector3 *x2 = &cloth->position[c->b];

			Vector3 delta = { x2->x - x1->x, x2->y - x1->y, x2->z - x1->z };

			float dist_sq = delta.x*delta.x + delta.y*delta.y + delta.z*delta.z;
			float scale   = c->rest_length_sq / (dist_sq + c->rest_length_sq) - 0.5f;

			if (pin_a) {
				scale *= 2.0f;
				x2->x += delta.x * scale;
				x2->y += delta.y * scale;
				x2->z += delta.z * scale;
			}
			else if (pin_b) {
				scale *= 2.0f;
				x1->x -= delta.x * scale;
				x1->y -= delta.y * scale;
				x1->z -= delta.z * scale;
			}
			else {
				x1->x -= delta.x * scale;
				x1->y -= delta.y * scale;
				x1->z -= delta.z * scale;
				x2->x += delta.x * scale;
				x2->y += delta.y * scale;
				x2->z += delta.z * scale;
			}
		}
	}
}


bool cloth_isCulled(const Cloth *cloth)
{
	return cloth->culled != NULL && *cloth->culled;
}


void cloth_step(Cloth *cloth, float dt)
{
	if (cloth->position == NULL) return;

	cloth_applyWind(cloth);
	cloth_integrate(cloth, dt);
	cloth_satisfy(cloth);

	/* After the solver, so the normals describe the pose the mesh will show.
	   Reusing the cross products from cloth_applyWind would save the pass but
	   would describe the pose of the previous frame. */
	cloth_updateNormals(cloth);
}


void cloth_blendRenderState(Cloth *cloth, float t)
{
	if (cloth->render_position == NULL) return;

	for (uint16_t i = 0; i < cloth->particle_count; i++) {
		cloth->render_position[i].x = cloth->previous[i].x + (cloth->position[i].x - cloth->previous[i].x) * t;
		cloth->render_position[i].y = cloth->previous[i].y + (cloth->position[i].y - cloth->previous[i].y) * t;
		cloth->render_position[i].z = cloth->previous[i].z + (cloth->position[i].z - cloth->previous[i].z) * t;
	}
}


void cloth_delete(Cloth *cloth)
{
	free(cloth->position);
	free(cloth->previous);
	free(cloth->force);
	free(cloth->normal);
	free(cloth->render_position);
	free(cloth->pinned);
	free(cloth->triangle);
	free(cloth->constraint);

	*cloth = (Cloth){0};
}
