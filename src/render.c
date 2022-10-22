#include "render.h"
#include <math.h>
#include "_generated_models.h"
#include "state.h"
#include "primitive_models.h"
#include "sprites.h"
#include "debug.h"
#include "libdragon_hax.h"

#include "path.h"

#define MAX_MODEL_VERTICES 256

#define LIGHT_SURFACE_WIDTH 64
#define LIGHT_SURFACE_HEIGHT 64

#define LIGHT_SPRITE_VSLICES 2

const float camera_z_factor = -0.04f;
const float camera_w_factor_base = 0.16f;
static float camera_wx_factor;
static float camera_wy_factor;

float work_positions[4*MAX_MODEL_VERTICES] = {};
float work_colors[3*MAX_MODEL_VERTICES] = {};

float tri_vector_a[9] = {};
float tri_vector_b[9] = {};
float tri_vector_c[9] = {};

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

static sprite_t *snooper_sprite;
static sprite_t *snooper_light_sprite;
static sprite_t *level_light_sprite;

static sprite_t light_surface_sprite;

static long long fps_start_tick = 0;
static int32_t fps_frame_count = 0;
static int32_t fps = 0;

const uint32_t SCREEN_WIDTH = 320;
const uint32_t SCREEN_HEIGHT = 240;

surface_t zbuffer;
surface_t light_surface;

static uint32_t tri_count;

static float half_framebuffer_width;
static float half_framebuffer_height;

void update_framebuffer_size(surface_t *surf) {
	half_framebuffer_width = surf->width / 2;
	half_framebuffer_height = surf->height / 2;

	camera_wx_factor = half_framebuffer_width/160.f * camera_w_factor_base;
	camera_wy_factor = half_framebuffer_height/120.f * camera_w_factor_base;
}

void set_camera_pitch(float camera_pitch) {
	float sp = sinf(camera_pitch);
	float cp = cosf(camera_pitch);

	camera_xx = 100.f;
	camera_yy = 100.f * cp;
	camera_yz = 100.f * sp;
	camera_zy = -camera_z_factor * sp;
	camera_zz = camera_z_factor * cp;
}


static float dynamic_quad_positions[12] = {};

static model_t line_model;
static model_t level_light_model;

static float level_light_texcoords[] = {
	0.f, 0.f,
	63.f, 0.f,
	0.f, 63.f,
	63.f, 63.f,
};

void renderer_init() {
    floor_sprite = sprite_load("rom:/ground.sprite");
	snooper_sprite = sprite_load("rom:/snooper.sprite");
	snooper_light_sprite = sprite_load("rom:/light.sprite");
	level_light_sprite = sprite_load("rom:/level_light.sprite");

	light_surface = surface_alloc(FMT_RGBA16, LIGHT_SURFACE_WIDTH, LIGHT_SURFACE_HEIGHT);

	light_surface_sprite.width = LIGHT_SURFACE_WIDTH;
	light_surface_sprite.height = LIGHT_SURFACE_HEIGHT;
	light_surface_sprite.flags = SPRITE_FLAGS_EXT | FMT_IA16;
	light_surface_sprite.hslices = 1;
	light_surface_sprite.vslices = LIGHT_SPRITE_VSLICES;

	zbuffer = surface_alloc(FMT_RGBA16, 320, 240);

	set_camera_pitch(0.8f);
	graphics_set_default_font();

	line_model.positions_len = ARRAY_LENGTH(dynamic_quad_positions);
	line_model.texcoords_len = floor_model.texcoords_len;
	line_model.norms_len = floor_model.norms_len;
	line_model.tris_len = floor_model.tris_len;

	line_model.positions = dynamic_quad_positions;
	line_model.texcoords = floor_model.texcoords;
	line_model.norms = floor_model.norms;
	line_model.tris = floor_model.tris;

	level_light_model.positions_len = ARRAY_LENGTH(dynamic_quad_positions);
	level_light_model.texcoords_len = ARRAY_LENGTH(level_light_texcoords);
	level_light_model.norms_len = floor_model.norms_len;
	level_light_model.tris_len = floor_model.tris_len;

	level_light_model.positions = dynamic_quad_positions;
	level_light_model.texcoords = level_light_texcoords;
	level_light_model.norms = floor_model.norms;
	level_light_model.tris = floor_model.tris;
}

void render_model_positioned(const vector3_t *position, const model_t *model) {
	float relative_x = position->x - game_state.camera_position.x;
	float relative_y = position->y - game_state.camera_position.y;
	float relative_z = position->z - game_state.camera_position.z;

	float *in_positions = model->positions;
	float *out_pos = work_positions;
	for (uint16_t i = 0; i < model->positions_len; i += 3) {
		float in_x = in_positions[i];
		float in_y = in_positions[i + 1];
		float in_z = in_positions[i + 2];

		// translate
		in_x += relative_x;
		in_y += relative_y;
		in_z += relative_z;

		float x2 = camera_xx*in_x;
		float y2 = camera_yy*in_y + camera_yz*in_z;
		float z2 = camera_zy*in_y + camera_zz*in_z;

		x2 *= camera_wx_factor / z2;
		y2 *= camera_wy_factor / z2;
		x2 += half_framebuffer_width;
		y2 = half_framebuffer_height - y2;

		*(out_pos++) = x2;
		*(out_pos++) = y2;
		*(out_pos++) = z2;
	}

	float *in_texcoords = model->texcoords;
	uint16_t *in_tris = model->tris;
	for (uint16_t i = 0; i < model->tris_len; i += 9) {
		memcpy(tri_vector_a, work_positions+in_tris[i], 3*sizeof(float));
		memcpy(tri_vector_b, work_positions+in_tris[i+3], 3*sizeof(float));
		memcpy(tri_vector_c, work_positions+in_tris[i+6], 3*sizeof(float));

		float area = (
			tri_vector_a[0] * tri_vector_b[1]
			+ tri_vector_b[0] * tri_vector_c[1]
			+ tri_vector_c[0] * tri_vector_a[1]
			- tri_vector_a[0] * tri_vector_c[1]
			- tri_vector_b[0] * tri_vector_a[1]
			- tri_vector_c[0] * tri_vector_b[1]
		);
		if (area <= 0) continue;

		memcpy(tri_vector_a+3, in_texcoords+in_tris[i+1], 2*sizeof(float));
		memcpy(tri_vector_b+3, in_texcoords+in_tris[i+4], 2*sizeof(float));
		memcpy(tri_vector_c+3, in_texcoords+in_tris[i+7], 2*sizeof(float));

		tri_vector_a[5] = 1.f / tri_vector_a[2];
		tri_vector_b[5] = 1.f / tri_vector_b[2];
		tri_vector_c[5] = 1.f / tri_vector_c[2];

		rdpq_triangle_cpu(
			TILE0, // tile
			0, // mipmaps
			0, // pos_offset
			-1, // shade_offset
			3, // tex_offset
			2, // depth_offset
			tri_vector_a,
			tri_vector_b,
			tri_vector_c
		);
		tri_count++;
	}
}

void render_object_transformed_shaded(const object_transform_t *transform, const model_t *model) {
	float sin_yaw = sinf(transform->rotation_z);
	float cos_yaw = cosf(transform->rotation_z);

	float relative_x = transform->position.x - game_state.camera_position.x;
	float relative_y = transform->position.y - game_state.camera_position.y;
	float relative_z = transform->position.z - game_state.camera_position.z;

	float *in_positions = model->positions;
	float *out_pos = work_positions;
	for (uint16_t i = 0; i < model->positions_len; i += 3) {
		float in_x = in_positions[i];
		float in_y = in_positions[i + 1];
		float in_z = in_positions[i + 2];

		// Step 1: rotate
		float x1 = cos_yaw*in_x + sin_yaw*in_y;
		float y1 = cos_yaw*in_y - sin_yaw*in_x;
		float z1 = in_z;

		// Step 2: translate
		x1 += relative_x;
		y1 += relative_y;
		z1 += relative_z;

		float x2 = camera_xx*x1;
		float y2 = camera_yy*y1 + camera_yz*z1;
		float z2 = camera_zy*y1 + camera_zz*z1;

		x2 *= camera_wx_factor / z2;
		y2 *= camera_wy_factor / z2;
		x2 += half_framebuffer_width;
		y2 = half_framebuffer_height - y2;

		// *(out_pos++) = w;
		*(out_pos++) = x2;
		*(out_pos++) = y2;
		*(out_pos++) = z2;
	}

	float light_vec_x = light_direction_x * cos_yaw - light_direction_y * sin_yaw;
	float light_vec_y = light_direction_x * sin_yaw + light_direction_y * cos_yaw;

	float *in_norms = model->norms;
	for (uint16_t i = 0; i < model->norms_len; i += 3) {
		float norm_x = in_norms[i];
		float norm_y = in_norms[i+1];
		float norm_z = in_norms[i+2];

		// TODO : a lot of this can be precalculated.
		float brightness = (
			  light_vec_x * norm_x
			+ light_vec_y * norm_y
			+ light_direction_z * norm_z
		);
		if (brightness < 0.0f) {
			brightness = 0.0f;
		}

		// Skip normal/color for now.
		work_colors[i] = ambient_light_r + brightness * directional_light_r;
		work_colors[i+1] = ambient_light_g + brightness * directional_light_g;
		work_colors[i+2] = ambient_light_b + brightness * directional_light_b;
	}

	float *in_texcoords = model->texcoords;
	uint16_t *in_tris = model->tris;
	for (uint16_t i = 0; i < model->tris_len; i += 9) {
		memcpy(tri_vector_a, work_positions+in_tris[i], 3*sizeof(float));
		memcpy(tri_vector_c, work_positions+in_tris[i+6], 3*sizeof(float));
		memcpy(tri_vector_b, work_positions+in_tris[i+3], 3*sizeof(float));

		float area = (
			tri_vector_a[0] * tri_vector_b[1]
			+ tri_vector_b[0] * tri_vector_c[1]
			+ tri_vector_c[0] * tri_vector_a[1]
			- tri_vector_a[0] * tri_vector_c[1]
			- tri_vector_b[0] * tri_vector_a[1]
			- tri_vector_c[0] * tri_vector_b[1]
		);
		if (area <= 0) continue;

		memcpy(tri_vector_a+3, in_texcoords+in_tris[i+1], 2*sizeof(float));
		memcpy(tri_vector_b+3, in_texcoords+in_tris[i+4], 2*sizeof(float));
		memcpy(tri_vector_c+3, in_texcoords+in_tris[i+7], 2*sizeof(float));

		memcpy(tri_vector_a+6, work_colors+in_tris[i+2], 3*sizeof(float));
		memcpy(tri_vector_b+6, work_colors+in_tris[i+5], 3*sizeof(float));
		memcpy(tri_vector_c+6, work_colors+in_tris[i+8], 3*sizeof(float));

		tri_vector_a[5] = 1.f / tri_vector_a[2];
		tri_vector_b[5] = 1.f / tri_vector_b[2];
		tri_vector_c[5] = 1.f / tri_vector_c[2];

		rdpq_triangle_cpu(
			TILE0, // tile
			0, // mipmaps
			0, // pos_offset
			6, // shade_offset
			3, // tex_offset
			2, // depth_offset
			tri_vector_a,
			tri_vector_b,
			tri_vector_c
		);
		tri_count++;
	}
}

void clear_z_buffer() {
	rdpq_set_color_image(&zbuffer);
	rdpq_set_mode_fill(RGBA32(0xff, 0xff, 0xff, 0xff));
	rdpq_fill_rectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
}

static bool should_render(float x, float y) {
	float relative_y = y - game_state.camera_position.y;
	if (relative_y <= 4.f || relative_y >= 24.f) return false;
	
	float screen_z = camera_zy*relative_y;
	float screen_x = camera_xx*(x-game_state.camera_position.x)*camera_w_factor_base/screen_z;

	return (screen_x > -600.f && screen_x < 600.f);
}



static void render_line(vector2_t src, vector2_t dest, float width) {
	if (!(should_render(src.x, src.y) || should_render(dest.x, dest.y))) {
		return;
	}

	float dx = dest.x - src.x;
	float dy = dest.y - src.y;
	float dist = sqrtf(dx*dx + dy*dy);
	float scale = width / dist;
	float offset_x = dy * scale;
	float offset_y = -dx * scale;

	line_model.positions[0] = src.x - offset_x;
	line_model.positions[1] = src.y - offset_y;

	line_model.positions[3] = src.x + offset_x;
	line_model.positions[4] = src.y + offset_y;

	line_model.positions[6] = dest.x - offset_x;
	line_model.positions[7] = dest.y - offset_y;

	line_model.positions[9] = dest.x + offset_x;
	line_model.positions[10] = dest.y + offset_y;

	vector3_t pos = {0.f, 0.f, 0.f};
	render_model_positioned(&pos, &line_model);
}

void render_graph(const path_graph_t *graph, int16_t closest_node) {
	rdpq_mode_blender(RDPQ_BLENDER((BLEND_RGB, IN_ALPHA, MEMORY_RGB, INV_MUX_ALPHA)));
	for (int16_t node_idx = 0; node_idx < graph->node_count; node_idx++) {
		const vector2_t pos = graph->nodes[node_idx].position;

		vector3_t render_pos;
		render_pos.x = pos.x;
		render_pos.y = pos.y;
		render_pos.z = 0.f;

		if (node_idx == closest_node) {
			rdpq_set_blend_color(RGBA32(0x80, 0x80, 0x00, 0xff));
		} else {
			rdpq_set_blend_color(RGBA32(0x60, 0x60, 0x60, 0xff));
		}
		render_model_positioned(&render_pos, &small_square_model);

		if (node_idx != closest_node) continue;
		int16_t waypoint = graph->nodes[node_idx].waypoint_ancestor;
		if (waypoint >= 0) {
			rdpq_set_blend_color(RGBA32(0x80, 0x00, 0x00, 0xff));
			render_line(pos, graph->nodes[waypoint].position, 0.1f);
		}
	}

	rdpq_set_blend_color(RGBA32(0x00, 0x40, 0x80, 0xff));
	for (int16_t edge_idx = 0; edge_idx < graph->edge_count; edge_idx++) {
		const path_edge_t *edge = &graph->edges[edge_idx];
		if (edge->src != closest_node) continue;
		render_line(graph->nodes[edge->src].position, graph->nodes[edge->dest].position, 0.05f);
	}
}

bool render() {
    surface_t *disp = display_lock();
    if (!disp)
    {
        return false;
    }

	tri_count = 0;
	int16_t closest_node = -1;
	{
		const vector3_t *spooker_position = &game_state.spookers[0].transform.position;
		float closest_dist2 = 9999.f;
		for (int16_t node_idx = 0; node_idx < game_state.level->path_graph->node_count; node_idx++) {
			vector2_t pos = game_state.level->path_graph->nodes[node_idx].position;
			float dx = pos.x - spooker_position->x;
			float dy = pos.y - spooker_position->y;
			float dist2 = dx*dx + dy*dy;
			if (dist2 < closest_dist2) {
				closest_dist2 = dist2;
				closest_node = node_idx;
			}
		}
	}

	// Clear the z buffer.
	clear_z_buffer();

	rdpq_set_color_image(&light_surface);

	// update_framebuffer_size assumes the surface exactly covers the screen
	// but it doesn't!
	// update_framebuffer_size(&light_surface);
	half_framebuffer_width = 32;
	half_framebuffer_height = 31;

	camera_wx_factor = 32/160.f * 0.16f;
	camera_wy_factor = 30/120.f * 0.16f;

	rdpq_set_mode_fill(RGBA32(0x00, 0x00, 0x20, 0xff));
	rdpq_fill_rectangle(0, 0, LIGHT_SURFACE_WIDTH, LIGHT_SURFACE_HEIGHT);

	// Render lights
	rdpq_set_mode_standard();

	// rdpq_set_other_modes_raw(SOM_TF0_RGB);
	rdpq_change_other_modes_raw(SOM_SAMPLE_MASK, SOM_SAMPLE_BILINEAR);
	rdpq_change_other_modes_raw(SOM_Z_WRITE, 0);
	rdpq_change_other_modes_raw(SOM_Z_COMPARE, 0);

	rdpq_mode_combiner(RDPQ_COMBINER_TEX_FLAT);
	object_transform_t work_transform = {{0.f, 0.f, 0.f}, 0.f};
	rdpq_set_blend_color(RGBA32(0, 0, 0xff, 0xff));
	rdpq_mode_blender(RDPQ_BLENDER((BLEND_RGB, IN_ALPHA, MEMORY_RGB, INV_MUX_ALPHA)));
	rdpq_mode_persp(true);
	rdpq_mode_mipmap(MIPMAP_NONE, 0);
	rdpq_sync_load();
	// rdp_load_texture(0, 0, MIRROR_DISABLED, snooper_light_sprite);
	rdp_load_texture_stride_hax(0, 0, MIRROR_DISABLED, snooper_light_sprite, snooper_light_sprite->data, 0);
	for (int i = 0; i < game_state.snooper_count; i++) {
		snooper_state_t *snooper = &game_state.snoopers[i];
		if (snooper->status != SNOOPER_STATUS_ALIVE) continue;

		float s = sinf(snooper->head_rotation_z);
		float c = cosf(snooper->head_rotation_z);

		float end_x = snooper->position.x + 6.f * s;
		float end_y = snooper->position.y + 6.f * c;
		if (!(
			should_render(snooper->position.x, snooper->position.y)
			|| should_render(end_x, end_y)
		)) {
			continue;
		}

		work_transform.position.x = snooper->position.x;
		work_transform.position.y = snooper->position.y;
		work_transform.rotation_z = snooper->head_rotation_z;

		rdpq_set_prim_color(RGBA32(0xff, 0xff, 0xff, snooper->light_brightness * 255 / 100));

		// render_model_positioned(&work_transform.position, &light_model);
		// TODO : no shade?
		render_object_transformed_shaded(&work_transform, &light_model);
	}

	rdpq_sync_load();
	rdp_load_texture_stride_hax(0, 0, MIRROR_DISABLED, level_light_sprite, level_light_sprite->data, 0);
	for (int i = 0; i < game_state.level->light_count; i++) {
		const level_light_t *light = &game_state.level->lights[i];
		work_transform.position.x = light->position.x;
		work_transform.position.y = light->position.y;

		if (!(
			should_render(work_transform.position.x - light->radius, work_transform.position.y - light->radius)
			|| should_render(work_transform.position.x + light->radius, work_transform.position.y - light->radius)
			|| should_render(work_transform.position.x - light->radius, work_transform.position.y + light->radius)
			|| should_render(work_transform.position.x + light->radius, work_transform.position.y + light->radius)
		)) {
			continue;
		}

		level_light_model.positions[0] = -light->radius;
		level_light_model.positions[1] = -light->radius;

		level_light_model.positions[3] = light->radius;
		level_light_model.positions[4] = -light->radius;

		level_light_model.positions[6] = -light->radius;
		level_light_model.positions[7] = light->radius;

		level_light_model.positions[9] = light->radius;
		level_light_model.positions[10] = light->radius;

		rdpq_set_prim_color(RGBA32(0xff, 0xff, 0xff, game_state.light_states[i].brightness*255/100));
		render_model_positioned(&work_transform.position, &level_light_model);
	}

    rdp_attach(disp);
	update_framebuffer_size(disp);

	// TODO : set color image to light buffer?
	rdpq_set_z_image(&zbuffer);

	// Clear the framebuffer.
	rdpq_set_mode_fill(RGBA32(0, 0, 0, 0));
	rdpq_fill_rectangle(0, 0, 320, 240);

	rdpq_set_mode_standard();
	rdpq_mode_persp(true);
	rdpq_change_other_modes_raw(SOM_SAMPLE_MASK, SOM_SAMPLE_BILINEAR);
	// rdpq_change_other_modes_raw(SOM_AA_ENABLE, SOM_AA_ENABLE);
	rdpq_mode_mipmap(MIPMAP_NONE, 0);

	rdpq_mode_combiner(RDPQ_COMBINER_TEX);

	// Render level
	vector3_t level_position;
	level_position.z = 0.f;
	level_position.y = game_state.level->height - 1;
	sprite_t *loaded_sprite = NULL;
	const uint8_t *data = game_state.level->data;
	for (uint16_t y = 0; y < game_state.level->height; y++) {
		level_position.x = -game_state.level->width + 1;
		for (uint16_t x = 0; x < game_state.level->width; x++) {
			uint8_t d = *(data++);
			if (d && should_render(level_position.x, level_position.y)) {
				sprite_t *obj_sprite = floor_sprite;
				if (obj_sprite != loaded_sprite) {
					loaded_sprite = obj_sprite;
					rdpq_sync_load();
					rdp_load_texture(0, 0, MIRROR_DISABLED, loaded_sprite);
				}

				render_model_positioned(&level_position, &floor_model);
			}
			level_position.x += 2.f;
		}
		level_position.y -= 2.f;
	}

	// Render paths
	// render_graph(game_state.level->path_graph, closest_node);

	// Apply lights
	rdpq_sync_pipe();
	rdpq_set_mode_standard();
	rdpq_set_prim_color(RGBA32(0, 0, 0, 0xff));
	rdpq_change_other_modes_raw(SOM_SAMPLE_MASK, SOM_SAMPLE_BILINEAR);
	rdpq_mode_combiner(RDPQ_COMBINER1((TEX0, 0, PRIM, TEX0), (TEX0, 0, PRIM, TEX0)));
	rdpq_set_blend_color(RGBA32(0, 0x00, 0x08, 0xff));
	rdpq_mode_blender(RDPQ_BLENDER((MEMORY_RGB, IN_ALPHA, BLEND_RGB, ONE)));

	// int y = 0;
	rdpq_sync_load();
	rdp_load_texture_stride_hax(0, 0, MIRROR_DISABLED, &light_surface_sprite, light_surface.buffer, 0);
	rdpq_texture_rectangle_fx(
		0, // tile
		0, // x0
		0, // y0
		320*4, // x1
		120*4, // y1
		0, //s
		32, //t
		1024.f * LIGHT_SURFACE_WIDTH / 320.f, // dsdx
		1024.f * (60) / 240.f); // dtdy

	rdpq_sync_load();
	rdp_load_texture_stride_hax(
		0, 0, MIRROR_DISABLED, &light_surface_sprite,
		light_surface.buffer + (2 * LIGHT_SURFACE_WIDTH * (30)),
		0);
	rdpq_texture_rectangle_fx(
		0, // tile
		0, // x0
		120*4, // y0
		320*4, // x1
		240*4, // y1
		0, //s
		32, //t
		1024.f * LIGHT_SURFACE_WIDTH / 320.f, // dsdx
		1024.f * (60) / 240.f); // dtdy

	// Render snoopers
	rdpq_set_mode_standard();
	rdpq_mode_combiner(RDPQ_COMBINER_TEX_SHADE);
	rdpq_set_other_modes_raw(SOM_TEXTURE_PERSP);
	rdpq_change_other_modes_raw(SOM_SAMPLE_MASK, SOM_SAMPLE_BILINEAR);
	rdpq_change_other_modes_raw(SOM_Z_WRITE, SOM_Z_WRITE);
	rdpq_change_other_modes_raw(SOM_Z_COMPARE, SOM_Z_COMPARE);
	rdpq_change_other_modes_raw(SOM_TF_MASK, SOM_TF0_RGB);
	rdpq_mode_blender(RDPQ_BLENDER((IN_RGB, IN_ALPHA, IN_RGB, INV_MUX_ALPHA)));
	rdpq_mode_mipmap(MIPMAP_NONE, 0);

	rdpq_sync_load();
	rdp_load_texture(0, 0, MIRROR_DISABLED, snooper_sprite);
	for (int i = 0; i < game_state.snooper_count; i++) {
		if (!should_render(game_state.snoopers[i].position.x, game_state.snoopers[i].position.y)) continue;

		work_transform.position.x = game_state.snoopers[i].position.x;
		work_transform.position.y = game_state.snoopers[i].position.y;
		work_transform.rotation_z = game_state.snoopers[i].head_rotation_z;
		// render_model_positioned(&work_transform.position, &snooper_model);
		render_object_transformed_shaded(&work_transform, &snooper_model);
	}

	// Render spookers
	// rdpq_sync_load();
	// rdp_load_texture(0, 0, MIRROR_DISABLED, spooker_sprite);
	for (int i = 0; i < game_state.spooker_count; i++) {
		// render_model_positioned(&game_state.spookers[i].transform.position, &snooper_model);
		spooker_state_t *spooker = &game_state.spookers[i];
		if (spooker->knockback_timer < SPOOKER_KNOCKBACK_THRESHOLD && spooker->knockback_timer % 4 >= 2) continue;
		render_object_transformed_shaded(&spooker->transform, &snooper_model);
	}

	char info_str[64];
	sprintf(info_str, "%ldFPS", fps);

	rspq_wait();
	graphics_set_color(0xFFFFFFFF, 0x00000000);
	graphics_draw_text(disp, 6, 2, info_str);

	{
		const vector3_t *spooker_position = &game_state.spookers[0].transform.position;
		sprintf(info_str, "%d %.1f %.1f %ld", closest_node, spooker_position->x, spooker_position->y, tri_count);
	}

	graphics_draw_text(disp, 60, 2, info_str);
	graphics_draw_text(disp, 6, 10, debug_message);

	rdp_detach_show(disp);

	fps_frame_count++;
	if (fps_frame_count >= 10) {
		long long now = timer_ticks();
		long long fps_ticks = now - fps_start_tick;
		fps_start_tick = now;
		fps_frame_count = 0;

		float frame_seconds = ((float)fps_ticks) / 468750000.0f;

		fps = (int)(1.0f / frame_seconds + 0.5f);
		if (fps < 0) fps = 0;
		if (fps > 99) fps = 99;
	}

	return true;
}
