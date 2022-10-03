#ifndef SPOOK64_RENDER
#define SPOOK64_RENDER

#include <stdint.h>
#include "dragon.h"
#include "vector.h"

typedef struct {
	uint16_t positions_len;
	uint16_t texcoords_len;
	uint16_t norms_len;
	uint16_t tris_len;

	float *positions;
	float *texcoords;
	float *norms;

	uint16_t *tris;
} model_t;

typedef struct {
	vector3_t position;
	float rotation_z;
} object_transform_t;

#define MODEL(positions, texcoords, norms, tris) {\
	sizeof(positions)/sizeof(float),\
	sizeof(texcoords)/sizeof(float),\
	sizeof(norms)/sizeof(float),\
	sizeof(tris)/sizeof(uint16_t),\
	positions,\
	texcoords,\
	norms,\
	tris}

bool render();
void renderer_init();
void clear_z_buffer();
void render_object_transformed_shaded(const object_transform_t *transform, const model_t *model);
void set_camera_pitch(float camera_pitch);

extern surface_t zbuffer;
extern float camera_position[];

#endif
