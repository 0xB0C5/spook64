#ifndef SPOOK64_RENDER
#define SPOOK64_RENDER

#include <stdint.h>
#include "libdragon.h"

typedef struct {
	uint16_t verts_len;
	uint16_t tris_len;

	float *verts;
	uint16_t *tris;
} model_t;

typedef struct {
	float x;
	float y;
	float z;

	float rotation_z;
} object_transform_t;

#define MODEL(verts, tris) {sizeof(verts)/sizeof(float),sizeof(tris)/sizeof(uint16_t),verts,tris}

void render();
void renderer_init();
void clear_z_buffer();
void render_object(const object_transform_t *transform, const model_t *model);
void set_camera_pitch(float camera_pitch);

extern surface_t zbuffer;
extern float camera_position[];

#endif
