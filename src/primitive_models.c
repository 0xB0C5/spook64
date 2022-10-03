#include "render.h"
#include "primitive_models.h"

// x, y, z, u, v, 1/w
static float floor_positions[] = {
	-1.f, -1.f, 0.0f,
	1.f, -1.f, 0.0f,
	-1.f, 1.f, 0.0f,
	1.f, 1.f, 0.0f,
};

static float floor_texcoords[] = {
	0.f, 0.f,
	64.f, 0.f,
	0.f, 32.f,
	64.f, 32.f,
};

static float floor_norms[] = {
	0.0f, 0.0f, 0.0f,
};

static uint16_t floor_tris[] = {
	0, 0, 0, 3, 2, 0, 6, 4, 0,
	3, 2, 0, 6, 4, 0, 9, 6, 0,
};

model_t floor_model = MODEL(floor_positions, floor_texcoords, floor_norms, floor_tris);

static float light_positions[] = {
	-0.5f, 0.5f, 0.0f,
	0.5f, 0.5f, 0.0f,
	-0.5f, 4.0f, 0.0f,
	0.5f, 4.0f, 0.0f,
};
static float light_texcoords[] = {
	0.f, 0.f,
	32.f, 0.f,
	0.f, 16.f,
	32.f, 16.f,
};

static float light_norms[] = {
	0.0f, 0.0f, 0.0f,
};

static uint16_t light_tris[] = {
	0, 0, 0, 3, 2, 0, 6, 4, 0,
	3, 2, 0, 6, 4, 0, 9, 6, 0,
};

model_t light_model = MODEL(light_positions, light_texcoords, light_norms, light_tris);


static float small_square_positions[] = {
	-0.2f, -0.2f, 0.0f,
	0.2f, -0.2f, 0.0f,
	-0.2f, 0.2f, 0.0f,
	0.2f, 0.2f, 0.0f,
};

model_t small_square_model = MODEL(small_square_positions, floor_texcoords, floor_norms, floor_tris);