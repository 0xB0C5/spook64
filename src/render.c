#include "render.h"
#include "libdragon.h"
#include <math.h>

#define MAX_MODEL_VERTICES 256
#define VERT_LEN 6
float work_vertices[MAX_MODEL_VERTICES*VERT_LEN] = {};


float cube_vertices[] = {
	-0.5f, -0.5f, -0.5f, 0.f, 0.f, 1.f,
	0.5f, -0.5f, -0.5f, 0.f, 32.f,  1.f,
	-0.5f, 0.5f, -0.5f, 0.f, 0.f,  1.f,
	0.5f, 0.5f, -0.5f, 0.f, 32.f,  1.f,
	-0.5f, -0.5f, 0.5f, 0.f, 0.f,  1.f,
	0.5f, -0.5f, 0.5f, 0.f, 32.f, 1.f,
	-0.5f, 0.5f, 0.5f, 0.f, 0.f, 1.f,
	0.5f, 0.5f, 0.5f, 0.f, 32.f, 1.f,
};

uint16_t cube_tris[] = {
	0, 1, 2,
	1, 2, 3,
	0, 1, 4,
	1, 4, 5,
	0, 2, 4,
	0, 4, 6,

	1, 3, 5,
	3, 5, 7,
	2, 3, 6,
	3, 6, 7,
	4, 5, 6,
	5, 6, 7,
};


// x, y, z, u, v, 1/w
float ground_vertices[] = {
	0.f, 0.f, 1.0f, 0.f, 0.f, 0.05f,
	320.f, 0.f, 1.0f, 128.f, 0.f, 0.05f,
	480.f, 240.f, 0.5f, 128.f, 128.f, 0.1f,
	-160.f, 240.f, 0.5f, 0.f, 128.f, 0.1f,
};

uint16_t ground_triangles[] = {
	0, 1, 3,
	1, 2, 3,
};

float quad_verts[] = {
	40.f, 0.f, 0.7f, 0.f, 0.f, 0.75f,
	280.f, 0.f, 0.7f, 32.f, 0.f, 0.75f,
	280.f, 240.f, 0.7f, 32.f, 32.f, 0.75f,
	40.f, 240.f, 0.7f, 0.f, 32.f, 0.75f,
};
uint16_t quad_tris[] = {
	0, 1, 3,
	1, 2, 3,
};

float render_matrix[] = {
	100.f, 0.f, 0.f,
	0.f, 100.f, 0.f,
	0.f, 0.f, 100.f,
};
float camera_position[] = {0.f, 0.5f, -5.f};
float camera_yaw = 0.f;
float camera_pitch = 0.f;

static sprite_t *ground_sprite;
static sprite_t *wall_sprite;

static surface_t zbuffer;

float wall_direction = 1.f;

void renderer_init() {
    ground_sprite = sprite_load("rom:/ground.sprite");
	wall_sprite = sprite_load("rom:/wall.sprite");
	
	zbuffer = surface_alloc(FMT_RGBA16, 320, 240);
}

void render_perspective_model(uint16_t verts_size, uint16_t tris_size, float *verts, uint16_t *tris) {
	float *verts_end = verts + verts_size;

	float *in_v = verts;
	float *out_v = work_vertices;

	while (in_v < verts_end) {
		float in_x = *(in_v++);// - camera_position[0];
		float in_y = *(in_v++);// - camera_position[1];
		float in_z = *(in_v++);// - camera_position[2];

		float in_s = *(in_v++);
		float in_t = *(in_v++);
		in_v++;

		in_x -= camera_position[0];
		in_y -= camera_position[1];
		in_z -= camera_position[2];

		float out_x = (
			render_matrix[0]*in_x
			+ render_matrix[1]*in_y
			+ render_matrix[2]*in_z
		);
		float out_y = (
			render_matrix[3]*in_x
			+ render_matrix[4]*in_y
			+ render_matrix[5]*in_z
		);
		float out_z = (
			render_matrix[6]*in_x
			+ render_matrix[7]*in_y
			+ render_matrix[8]*in_z
		);
		float out_s = in_s;
		float out_t = in_t;
		float out_w = 400.f / out_z;
		out_x *= out_w;
		out_y *= out_w;
		out_x += 160.f;
		out_y = 120.f - out_y;

		*(out_v++) = out_x;
		*(out_v++) = out_y;
		*(out_v++) = out_z;
		*(out_v++) = out_s;
		*(out_v++) = out_t;
		*(out_v++) = out_w;
		/*

		float out_x = 
		float out_y = 
		float out_z = 

		float inv_w = 1/out_z;
		out_x *= inv_w;
		out_y *= inv_w;

		*(out_v++) = out_x;
		*(out_v++) = out_y;
		*(out_v++) = out_z;
		*(out_v++) = *(in_v++);
		*(out_v++) = *(in_v++);
		*(out_v++) = inv_w;
		in_v++;
		*/
	}

	uint16_t *tris_end = tris + tris_size;
	for (uint16_t *t = tris; t < tris_end;) {
		uint16_t a = *(t++);
		uint16_t b = *(t++);
		uint16_t c = *(t++);

		rdpq_triangle(
			TILE0, // tile
			0, // mipmaps
			0, // pos_offset
			-1, // shade_offset
			3, // tex_offset
			2, // depth_offset
			work_vertices+VERT_LEN*a,
			work_vertices+VERT_LEN*b,
			work_vertices+VERT_LEN*c
		);
	}
}

void render_model(uint16_t vertsSize, uint16_t trisSize, float *verts, uint16_t *tris) {
	uint16_t triIdx = 0;
	while (triIdx < trisSize) {
		uint16_t a = tris[triIdx++];
		uint16_t b = tris[triIdx++];
		uint16_t c = tris[triIdx++];

		rdpq_triangle(
			TILE0, // tile
			0, // mipmaps
			0, // pos_offset
			-1, // shade_offset
			3, // tex_offset
			2, // depth_offset
			verts+VERT_LEN*a,
			verts+VERT_LEN*b,
			verts+VERT_LEN*c
		);
	}
}

void render() {
    surface_t *disp = display_lock();
    if (!disp)
    {
        return;
    }

	quad_verts[1] += wall_direction;
	quad_verts[7] += wall_direction;
	quad_verts[13] += wall_direction;
	quad_verts[19] += wall_direction;
	if (quad_verts[1] >= 80.0f) {
		wall_direction = -1.0f;
	}
	if (quad_verts[1] <= -80.0f) {
		wall_direction = 1.0f;
	}

	// Clear the z buffer.
    rdp_attach(&zbuffer);
	rdpq_set_mode_fill(RGBA32(0xff, 0xff, 0xff, 0xff));
	rdpq_fill_rectangle(0, 0, 320, 240);
	rdp_detach();

	// Clear the framebuffer.
	// TODO : use a skybox or something instead?
    rdp_attach(disp);
	rdpq_set_mode_fill(RGBA32(10, 10, 20, 0));
	rdpq_fill_rectangle(0, 0, 320, 240);

	rdpq_set_mode_standard();
	rdpq_set_other_modes_raw(SOM_TEXTURE_PERSP);
	rdpq_change_other_modes_raw(SOM_SAMPLE_MASK, SOM_SAMPLE_BILINEAR);
	rdpq_change_other_modes_raw(SOM_AA_ENABLE, SOM_AA_ENABLE);
	//rdpq_change_other_modes_raw(SOM_ZMODE_MASK, SOM_ZMODE_OPAQUE);
	rdpq_change_other_modes_raw(SOM_Z_WRITE, SOM_Z_WRITE);
	rdpq_change_other_modes_raw(SOM_Z_COMPARE, SOM_Z_COMPARE);

	rdpq_set_z_image(&zbuffer);

	rdpq_mode_combiner(RDPQ_COMBINER_TEX);

	// Render ground
	rdp_load_texture(0, 0, MIRROR_DISABLED, ground_sprite);
	//render_perspective_model(sizeof(cube_vertices)/sizeof(float), sizeof(cube_tris)/sizeof(uint16_t), cube_vertices, cube_tris);
	render_model(sizeof(ground_vertices)/sizeof(float), sizeof(ground_triangles)/sizeof(uint16_t), ground_vertices, ground_triangles);

	rdp_load_texture(0, 0, MIRROR_DISABLED, wall_sprite);
	render_model(sizeof(quad_verts)/sizeof(float), sizeof(quad_tris)/sizeof(uint16_t), quad_verts, quad_tris);

	rdp_detach_show(disp);
}
