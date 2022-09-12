#include "render.h"
#include "libdragon.h"
#include <math.h>
#include "_generated_models.h"

#define MAX_MODEL_VERTICES 256
#define VERT_LEN 9

const float camera_z_factor = -0.04f;
const float camera_w_factor = 0.16f;

float work_vertices[MAX_MODEL_VERTICES*VERT_LEN] = {};

float cube_verts[] = {
	-0.5f, -0.5f, -0.5f, 0.f, 32.f,
	0.5f, -0.5f, -0.5f, 32.f, 32.f,
	-0.5f, 0.5f, -0.5f, 0.f, 0.f,
	0.5f, 0.5f, -0.5f, 32.f, 0.f,

	-0.5f, -0.5f, 0.5f, 0.f, 0.f,
	0.5f, -0.5f, 0.5f, 32.f, 0.f,
	-0.5f, 0.5f, 0.5f, 0.f, 32.f,
	0.5f, 0.5f, 0.5f, 32.f, 32.f,
	
	-0.5f, -0.5f, -0.5f, 32.f, 0.f,
	0.5f, -0.5f, -0.5f, 0.f, 0.f,
	-0.5f, 0.5f, -0.5f, 32.f, 32.f,
	0.5f, 0.5f, -0.5f, 0.f, 32.f,
};

uint16_t cube_tris[] = {
	0, 1, 4,
	1, 4, 5,
	8, 10, 4,
	10, 4, 6,

	9, 11, 5,
	11, 5, 7,
	2, 3, 6,
	3, 6, 7,
	4, 5, 6,
	5, 6, 7,
};

model_t cube_model = MODEL(cube_verts, cube_tris);

static object_transform_t test_transform = {
	0.f,
	0.f,
	0.5f,
	0.f
};

// x, y, z, u, v, 1/w
float floor_verts[] = {
	-0.5f, -0.5f, 0.0f, 0.f, 0.f,
	0.5f, -0.5f, 0.0f, 32.f, 0.f,
	-0.5f, 0.5f, 0.0f, 0.f, 32.f,
	0.5f, 0.5f, 0.0f, 32.f, 32.f,
	
};

uint16_t floor_tris[] = {
	0, 1, 2,
	1, 2, 3,
};

model_t floor_model = MODEL(floor_verts, floor_tris);

float light_verts[] = {
	-1.5f, -1.5f, 0.0f, 0.f, 0.f,
	1.5f, -1.5f, 0.0f, 32.f, 0.f,
	-1.5f, 0.0f, 0.0f, 0.f, 15.f,
	1.5f, 0.0f, 0.0f, 32.f, 15.f,
	-1.5f, 1.5f, 0.0f, 0.f, 0.f,
	1.5f, 1.5f, 0.0f, 32.f, 0.f,
};

uint16_t light_tris[] = {
	0, 1, 2,
	1, 2, 3,
	2, 3, 4,
	3, 4, 5,
};

model_t light_model = MODEL(light_verts, light_tris);

float camera_position[] = {0.f, -4.f, 8.f};

static float camera_xx;
static float camera_yy;
static float camera_yz;
static float camera_zy;
static float camera_zz;

static float light_direction_x = -0.57735f;
static float light_direction_y = -0.57735f;
static float light_direction_z = 0.57735f;

static float directional_light_r = 0.85f;
static float directional_light_g = 0.65f;
static float directional_light_b = 0.45f;

static float ambient_light_r = 0.15f;
static float ambient_light_g = 0.25f;
static float ambient_light_b = 0.35f;

static sprite_t *ground_sprite;
static sprite_t *snooper_sprite;
static sprite_t *light_sprite;

surface_t zbuffer;

void set_camera_pitch(float camera_pitch) {
	float sp = sinf(camera_pitch);
	float cp = cosf(camera_pitch);

	camera_xx = 100.f;
	camera_yy = 100.f * cp;
	camera_yz = 100.f * sp;
	camera_zy = -camera_z_factor * sp;
	camera_zz = camera_z_factor * cp;
}

void renderer_init() {
    ground_sprite = sprite_load("rom:/ground.sprite");
	snooper_sprite = sprite_load("rom:/snooper.sprite");
	light_sprite = sprite_load("rom:/light.sprite");

	zbuffer = surface_alloc(FMT_RGBA16, 320, 240);

	set_camera_pitch(0.5f);
}

void render_object(const object_transform_t *transform, const model_t *model) {
	float *verts_end = model->verts + model->verts_len;

	float *in_v = model->verts;
	float *out_v = work_vertices;

	float sin_yaw = sinf(transform->rotation_z);
	float cos_yaw = cosf(transform->rotation_z);

	float relative_x = transform->x - camera_position[0];
	float relative_y = transform->y - camera_position[1];
	float relative_z = transform->z - camera_position[2];

	while (in_v < verts_end) {
		float in_x = *(in_v++);
		float in_y = *(in_v++);
		float in_z = *(in_v++);

		// Step 1: rotate
		float x1 = cos_yaw*in_x + sin_yaw*in_y;
		float y1 = cos_yaw*in_y - sin_yaw*in_x;
		float z1 = in_z;

		// Step 2: translate
		x1 += relative_x;
		y1 += relative_y;
		z1 += relative_z;

		float out_x = camera_xx*x1;
		float out_y = camera_yy*y1 + camera_yz*z1;
		float out_z = camera_zy*y1 + camera_zz*z1;

		float out_w = camera_w_factor / out_z;
		out_x *= out_w;
		out_y *= out_w;
		out_x += 160.f;
		out_y = 120.f - out_y;

		*(out_v++) = out_x;
		*(out_v++) = out_y;
		*(out_v++) = out_z;
		*(out_v++) = *(in_v++);
		*(out_v++) = *(in_v++);
		*(out_v++) = out_w;

		float norm_x = *(in_v++);
		float norm_y = *(in_v++);
		float norm_z = *(in_v++);

		float brightness = (
			  (light_direction_x * cos_yaw - light_direction_y * sin_yaw) * norm_x
			+ (light_direction_x * sin_yaw + light_direction_y * cos_yaw) * norm_y
			+ light_direction_z * norm_z
		);
		if (brightness < 0.0f) {
			brightness = 0.0f;
		}

		// Skip normal/color for now.
		*(out_v++) = ambient_light_r + brightness * directional_light_r;
		*(out_v++) = ambient_light_g + brightness * directional_light_g;
		*(out_v++) = ambient_light_b + brightness * directional_light_b;
	}

	uint16_t *tris_end = model->tris + model->tris_len;
	for (uint16_t *t = model->tris; t < tris_end;) {
		uint16_t a = *(t++);
		uint16_t b = *(t++);
		uint16_t c = *(t++);

		rdpq_triangle(
			TILE0, // tile
			0, // mipmaps
			0, // pos_offset
			6, // shade_offset
			3, // tex_offset
			2, // depth_offset
			work_vertices+VERT_LEN*a,
			work_vertices+VERT_LEN*b,
			work_vertices+VERT_LEN*c
		);
	}
}

void clear_z_buffer() {
    rdp_attach(&zbuffer);
	rdpq_set_mode_fill(RGBA32(0xff, 0xff, 0xff, 0xff));
	rdpq_fill_rectangle(0, 0, 320, 240);
	rdp_detach();
}

void render() {
    surface_t *disp = display_lock();
    if (!disp)
    {
        return;
    }

	// Clear the z buffer.
	clear_z_buffer();

    rdp_attach(disp);
	rdpq_set_z_image(&zbuffer);

	// Clear the framebuffer.
	rdpq_set_mode_fill(RGBA32(0, 0, 0, 0));
	rdpq_fill_rectangle(0, 0, 320, 240);

	rdpq_set_mode_standard();
	rdpq_set_other_modes_raw(SOM_TEXTURE_PERSP);
	rdpq_change_other_modes_raw(SOM_SAMPLE_MASK, SOM_SAMPLE_BILINEAR);
	// rdpq_change_other_modes_raw(SOM_AA_ENABLE, SOM_AA_ENABLE);

	rdpq_mode_combiner(RDPQ_COMBINER_TEX);

	// Render ground
	const uint32_t floor_size_x = 9;
	const uint32_t floor_size_y = 7;
	const float floor_start_x = -0.5f*(floor_size_x-1);
	const float floor_start_y = -2.0f;

	object_transform_t floor_transform = {0.f, 0.f, 0.f, 0.f};
	for (uint32_t slice_y = 0; slice_y < ground_sprite->vslices; slice_y++) {
		for (uint32_t slice_x = 0; slice_x < ground_sprite->hslices; slice_x++) {
			rdp_load_texture_stride(0, 0, MIRROR_DISABLED, ground_sprite, slice_y*ground_sprite->hslices + slice_x);

			for (uint32_t x = slice_x; x < floor_size_x; x += ground_sprite->hslices) {
				for (uint32_t y = slice_y; y < floor_size_y; y += ground_sprite->vslices) {
					floor_transform.x = floor_start_x + (float)x;
					floor_transform.y = floor_start_y + (float)y;
					render_object(&floor_transform, &floor_model);
				}
			}
		}
	}

	// Render light
	rdpq_mode_blender(RDPQ_BLENDER((MEMORY_RGB, IN_ALPHA, MEMORY_RGB, ONE)));

	// You can't tell me what to do!
	/*
#define _RDPQ_SOM_BLEND2A_A_MEMORY_RGB cast64(1)
#define _RDPQ_SOM_BLEND2A_B2_ONE cast64(2)
    rdpq_mode_blender(RDPQ_BLENDER2(
        (MEMORY_RGB, IN_ALPHA, MEMORY_RGB, ONE),
        (CYCLE1_RGB, IN_ALPHA, CYCLE1_RGB, ONE)
    ));
	*/

	rdp_load_texture(0, 0, MIRROR_DISABLED, light_sprite);
	floor_transform.x = 0.0f;
	floor_transform.y = 0.0f;
	floor_transform.z = 0.0f;
	render_object(&test_transform, &light_model);

	// Render snooper
	rdpq_set_mode_standard();
	rdpq_change_other_modes_raw(SOM_Z_WRITE, SOM_Z_WRITE);
	rdpq_change_other_modes_raw(SOM_Z_COMPARE, SOM_Z_COMPARE);
	rdpq_mode_blender(RDPQ_BLENDER((IN_RGB, IN_ALPHA, MEMORY_RGB, ZERO)));
	rdp_load_texture(0, 0, MIRROR_DISABLED, snooper_sprite);
	render_object(&test_transform, &snooper_model);

	rdp_detach_show(disp);

	// update???
	controller_scan();
	struct controller_data ckeys = get_keys_held();

	if(ckeys.c[0].down)
	{
		test_transform.y -= 0.07f;
	}
	if(ckeys.c[0].up)
	{
		test_transform.y += 0.07f;
	}

	if (ckeys.c[0].left) {
		test_transform.x -= 0.07f;
	}
	if (ckeys.c[0].right) {
		test_transform.x += 0.07f;
	}
	
	if (ckeys.c[0].A) {
		test_transform.rotation_z += 0.1f;
		if (test_transform.rotation_z >= 2.f*M_PI) {
			test_transform.rotation_z -= 2.f*M_PI;
		}
	}
	if (ckeys.c[0].B) {
		test_transform.rotation_z -= 0.1f;
		if (test_transform.rotation_z < 0) {
			test_transform.rotation_z += 2.f*M_PI;
		}
	}
}
