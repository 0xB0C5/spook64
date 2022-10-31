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

#define SCORE_X 80
#define SCORE_Y 4

#define DEATH_X 170

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
static sprite_t *spooker_sprite;
static sprite_t *snooper_light_sprite;
static sprite_t *level_light_sprite;
static sprite_t *roof_sprite;
static sprite_t *numbers_sprite;
static sprite_t *win_sprite;
static sprite_t *lose_sprite;

static sprite_t *cur_screen_sprite;

static sprite_t light_surface_sprite;

/*
static long long fps_start_tick = 0;
static int32_t fps_frame_count = 0;
static int32_t fps = 0;
*/

const uint32_t SCREEN_WIDTH = 320;
const uint32_t SCREEN_HEIGHT = 240;

surface_t zbuffer;
surface_t light_surface;

static uint32_t tri_count;

static float half_framebuffer_width;
static float half_framebuffer_height;

static model_t *snooper_models[] = {
	&snooper_000001_model,
	&snooper_000002_model,
	&snooper_000003_model,
	&snooper_000004_model,
	&snooper_000005_model,
	&snooper_000006_model,
	&snooper_000007_model,
	&snooper_000008_model,
	&snooper_000009_model,
	&snooper_000010_model,
	&snooper_000011_model,
	&snooper_000012_model,
	&snooper_000013_model,
	&snooper_000014_model,
	&snooper_000015_model,
	&snooper_000016_model,
	&snooper_000017_model,
	&snooper_000018_model,
	&snooper_000019_model,
	&snooper_000020_model,
};

static model_t *snooper_feet_models[] = {
	&snooper_000001_feet_model,
	&snooper_000002_feet_model,
	&snooper_000003_feet_model,
	&snooper_000004_feet_model,
	&snooper_000005_feet_model,
	&snooper_000006_feet_model,
	&snooper_000007_feet_model,
	&snooper_000008_feet_model,
	&snooper_000009_feet_model,
	&snooper_000010_feet_model,
	&snooper_000011_feet_model,
	&snooper_000012_feet_model,
	&snooper_000013_feet_model,
	&snooper_000014_feet_model,
	&snooper_000015_feet_model,
	&snooper_000016_feet_model,
	&snooper_000017_feet_model,
	&snooper_000018_feet_model,
	&snooper_000019_feet_model,
	&snooper_000020_feet_model,
};


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

typedef struct {
	surface_t *surface;
	float alpha;
} surface_alpha_t;

surface_alpha_t screen_surface_alphas[4];

void renderer_init() {
    floor_sprite = sprite_load("rom:/ground.sprite");
    wall_sprite = sprite_load("rom:/wall.sprite");
    roof_sprite = sprite_load("rom:/roof.sprite");
	snooper_sprite = sprite_load("rom:/snooper.sprite");
	spooker_sprite = sprite_load("rom:/spooker1.sprite");
	numbers_sprite = sprite_load("rom:/numbers.sprite");
	snooper_light_sprite = sprite_load("rom:/light.sprite");
	level_light_sprite = sprite_load("rom:/level_light.sprite");
	win_sprite = sprite_load("rom:/win.sprite");
	lose_sprite = sprite_load("rom:/lose.sprite");
	cur_screen_sprite = NULL;

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
	
	for (int i = 0; i < ARRAY_LENGTH(screen_surface_alphas); i++) {
		screen_surface_alphas[i].surface = NULL;
		screen_surface_alphas[i].alpha = 0.f;
	}
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

		rdpq_triangle(
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

		rdpq_triangle(
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

void render_wall(uint8_t d, vector3_t *position) {
	if (d & 2) {
		render_model_positioned(position, &wall_model);
	}
	if (d & 4) {
		render_model_positioned(position, &wall_left_model);
	}
	if (d & 8) {
		render_model_positioned(position, &wall_right_model);
	}
	if (d & 0x20) {
		render_model_positioned(position, &fall_model);
	}
}

void render_roof(uint8_t d, vector3_t *position) {
	if (d & 16) {
		render_model_positioned(position, &roof_model);
	}
}

void render_floor(uint8_t d, vector3_t *position) {
	if (d & 1) {
		render_model_positioned(position, &floor_model);
	}
}

void foreach_level_element(void (func)(uint8_t, vector3_t*)) {
	vector3_t level_position;
	level_position.z = 0.f;
	level_position.y = game_state.level->height - 1;
	const uint8_t *data = game_state.level->data;
	for (uint16_t y = 0; y < game_state.level->height; y++) {
		level_position.x = -game_state.level->width + 1;
		for (uint16_t x = 0; x < game_state.level->width; x++) {
			uint8_t d = *(data++);
			if (should_render(level_position.x, level_position.y)) {
				func(d, &level_position);
			}
			level_position.x += 2.f;
		}
		level_position.y -= 2.f;
	}
}

void render_digit(int x, int y, int digit) {
	float s = (digit % 8) * 8.f;
	float t = (digit / 8) * 16.f;
	rdpq_texture_rectangle(0, x, y, x + 8, y + 16, s, t, 1.f, 1.f);
}

void load_screen(const char *path) {
	for (int i = 0; i < ARRAY_LENGTH(screen_surface_alphas); i++) {
		screen_surface_alphas[i].surface = NULL;
		screen_surface_alphas[i].alpha = 0.f;
	}

	if (cur_screen_sprite != NULL) {
		sprite_free(cur_screen_sprite);
	}

	if (path != NULL) {
		cur_screen_sprite = sprite_load(path);
	}
}

bool render_screen(float alpha) {
    surface_t *disp = display_lock();
    if (!disp)
    {
        return false;
    }

    rdp_attach(disp);

	// find surface
	int surface_index = 0;
	for (int i = 0; i < ARRAY_LENGTH(screen_surface_alphas); i++) {
		if (screen_surface_alphas[i].surface == disp) {
			surface_index = i;
			break;
		}
		if (screen_surface_alphas[i].surface == NULL) {
			surface_index = i;
		}
	}

	if (screen_surface_alphas[surface_index].surface != disp) {
		screen_surface_alphas[surface_index].surface = disp;
		screen_surface_alphas[surface_index].alpha = 0.f;
	}

	float last_screen_alpha = screen_surface_alphas[surface_index].alpha;
	screen_surface_alphas[surface_index].alpha = alpha;

	int last_rect_height = (int)(last_screen_alpha * 240.f);
	int cur_rect_height = (int)(alpha * 240.f);

	int big_rect_height;
	int small_rect_height;
	if (last_rect_height > cur_rect_height) {
		big_rect_height = last_rect_height;
		small_rect_height = cur_rect_height;
	} else {
		big_rect_height = cur_rect_height;
		small_rect_height = last_rect_height;
	}

	int update_top_min_y = 120 - big_rect_height/2;
	int update_top_max_y = 120 - small_rect_height/2;
	int update_bottom_min_y = 120 + small_rect_height/2;
	int update_bottom_max_y = 120 + big_rect_height/2;

	rdpq_set_mode_standard();
	rdpq_mode_combiner(RDPQ_COMBINER_TEX);
	int slice_height = cur_screen_sprite->height / cur_screen_sprite->vslices;
	int slice_width = cur_screen_sprite->width / cur_screen_sprite->hslices;
	if (cur_rect_height > last_rect_height) {
		for (uint32_t y = 0; y < cur_screen_sprite->vslices; y++)
		{
			int screen_y = y * slice_height;
			bool overlap_top = (
				screen_y + slice_height >= update_top_min_y
				&& screen_y <= update_top_max_y
			);
			bool overlap_bottom = (
				screen_y + slice_height >= update_bottom_min_y
				&& screen_y <= update_bottom_max_y
			);

			if (overlap_top || overlap_bottom) {
				for (uint32_t x = 0; x < cur_screen_sprite->hslices; x++)
				{
					int screen_x = x * slice_width;

					rdp_load_texture_stride(0, 0, MIRROR_DISABLED, cur_screen_sprite, y*cur_screen_sprite->hslices + x);
					rdp_draw_sprite(0, screen_x, screen_y, MIRROR_DISABLED);
				}
			}
		}
	}

	rdpq_set_mode_fill(RGBA32(0, 0, 0, 0xff));

	int show_top = 120 - cur_rect_height/2;
	int show_bottom = 120 + cur_rect_height/2;

	if (show_top > 0) rdpq_fill_rectangle(0, 0, 320, show_top);
	if (show_bottom < 240) rdpq_fill_rectangle(0, show_bottom, 320, 240);

	rdp_detach_show(disp);

	return true;
}

bool render() {
    surface_t *disp = display_lock();
    if (!disp)
    {
        return false;
    }

	tri_count = 0;
	
	/*
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
	*/

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
	rdpq_mode_zbuf(false, false);

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
		const level_light_state_t *light_state = &game_state.light_states[i];

		work_transform.position.x = light_state->position.x;
		work_transform.position.y = light_state->position.y;

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

	if (game_state.status == GAME_STATUS_START) {
		rspq_wait();

		float progress = game_state.game_status_timer / (float)GAME_START_DURATION;
		float alpha = 1.f;
		if (progress < 0.2f) {
			alpha = progress / 0.2f;
		} else if (progress > 0.8f) {
			alpha = (1.f - progress) / 0.2f;
		}
		uint8_t v = (uint8_t)(255.f * alpha);

		graphics_set_color(graphics_make_color(v, v, v, 0xff), 0);
		graphics_draw_text(disp, 124, 100, game_state.level->name);

		alpha = 1.f;
		if (progress < 0.2f) {
			alpha = 0.f;
		} else if (progress < 0.4f) {
			alpha = (progress - 0.2f) / 0.2f;
		} else if (progress > 0.8f) {
			alpha = (1.f - progress) / 0.2f;
		}

		char snooper_count_str[32];
		sprintf(snooper_count_str, "Spook %d Snoopers", game_state.level->score_target);
		v = (uint8_t)(255.f * alpha);
		graphics_set_color(graphics_make_color(v, v, v, 0xff), 0);
		graphics_draw_text(disp, 84, 115, snooper_count_str);
	} else {
		float visibility;
		if (game_state.status == GAME_STATUS_WIN || game_state.status == GAME_STATUS_LOSE) {
			visibility = (GAME_END_DURATION - game_state.game_status_timer) / (float)GAME_END_DURATION;
		} else {
			visibility = game_state.game_status_timer / (float)GAME_END_DURATION;
		}

		if (visibility < 0.f) visibility = 0.f;
		if (visibility > 1.f) visibility = 1.f;
		int scissor_half_height = (int)(120.f * visibility);
		if (scissor_half_height <= 0) scissor_half_height = 1;
		if (scissor_half_height < 120) {
			rdpq_set_scissor(0, 120 - scissor_half_height, 320, 120 + scissor_half_height);
		}

		// Render floor
		rdpq_set_mode_standard();
		rdpq_mode_persp(true);
		rdpq_change_other_modes_raw(SOM_SAMPLE_MASK, SOM_SAMPLE_BILINEAR);
		// rdpq_change_other_modes_raw(SOM_AA_ENABLE, SOM_AA_ENABLE);
		rdpq_mode_mipmap(MIPMAP_NONE, 0);

		rdpq_mode_combiner(RDPQ_COMBINER_TEX);

		rdpq_sync_load();
		rdp_load_texture(0, 0, MIRROR_DISABLED, floor_sprite);
		foreach_level_element(render_floor);

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

		// Render walls
		rdpq_set_mode_standard();
		rdpq_mode_persp(true);
		rdpq_change_other_modes_raw(SOM_SAMPLE_MASK, SOM_SAMPLE_BILINEAR);
		rdpq_mode_zbuf(true, true);
		// rdpq_change_other_modes_raw(SOM_AA_ENABLE, SOM_AA_ENABLE);
		rdpq_mode_mipmap(MIPMAP_NONE, 0);

		rdpq_mode_combiner(RDPQ_COMBINER_TEX_FLAT);

		rdpq_set_prim_color(RGBA32(0x20, 0x20, 0x20, 0xff));

		rdpq_sync_load();
		rdp_load_texture(0, 0, MIRROR_DISABLED, wall_sprite);
		foreach_level_element(render_wall);

		rdpq_sync_load();
		rdp_load_texture(0, 0, MIRROR_DISABLED, roof_sprite);
		foreach_level_element(render_roof);

		// Render paths
		// render_graph(game_state.level->path_graph, closest_node);

		// Render spooker outlines
		rdpq_set_mode_standard();
		rdpq_set_other_modes_raw(SOM_TEXTURE_PERSP);
		rdpq_change_other_modes_raw(SOM_SAMPLE_MASK, SOM_SAMPLE_BILINEAR);
		rdpq_change_other_modes_raw(SOM_TF_MASK, SOM_TF0_RGB);
		rdpq_mode_mipmap(MIPMAP_NONE, 0);
		rdpq_mode_zbuf(false, false);
		rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
		rdpq_mode_blender(RDPQ_BLENDER((IN_RGB, IN_ALPHA, MEMORY_RGB, INV_MUX_ALPHA)));
		rdpq_set_prim_color(RGBA32(0, 0x0, 0x0, 0xc0));
		for (int i = 0; i < game_state.spooker_count; i++) {
			spooker_state_t *spooker = &game_state.spookers[i];
			if (spooker->knockback_timer < SPOOKER_KNOCKBACK_THRESHOLD && spooker->knockback_timer % 4 >= 2) continue;
			render_object_transformed_shaded(&spooker->transform, &spooker_model);
		}

		// Render spookers
		rdpq_mode_zbuf(true, true);
		rdpq_mode_combiner(RDPQ_COMBINER_TEX_SHADE);
		rdpq_mode_blender(RDPQ_BLENDER((IN_RGB, IN_ALPHA, IN_RGB, INV_MUX_ALPHA)));
		rdpq_sync_load();
		rdp_load_texture(0, 0, MIRROR_DISABLED, spooker_sprite);
		for (int i = 0; i < game_state.spooker_count; i++) {
			spooker_state_t *spooker = &game_state.spookers[i];
			if (spooker->knockback_timer < SPOOKER_KNOCKBACK_THRESHOLD && spooker->knockback_timer % 4 >= 2) continue;
			render_object_transformed_shaded(&spooker->transform, &spooker_model);
		}

		// Render snoopers
		rdpq_sync_load();
		rdp_load_texture(0, 0, MIRROR_DISABLED, snooper_sprite);
		for (int i = 0; i < game_state.snooper_count; i++) {
			snooper_state_t *snooper = &game_state.snoopers[i];
			if (!should_render(snooper->position.x, snooper->position.y)) continue;

			work_transform.position.x = snooper->position.x;
			work_transform.position.y = snooper->position.y;
			work_transform.rotation_z = snooper->head_rotation_z;

			if (snooper->status == SNOOPER_STATUS_DYING) {
				float progress = snooper->freeze_timer / (float)SNOOPER_DIE_DURATION;
				work_transform.position.z = -10.f * progress * progress;
			} else if (snooper->status == SNOOPER_STATUS_SPOOKED) {
				float t = snooper->spooked_timer / 8.f;
				if (t > 1.0f) t = 1.0f;
				work_transform.position.z = 4.f * t * (1.f - t);
			} else {
				work_transform.position.z = 0.f;
			}

			int animation_index = ARRAY_LENGTH(snooper_models) * snooper->animation_progress;
			render_object_transformed_shaded(&work_transform, snooper_models[animation_index]);
			work_transform.rotation_z = snooper->feet_rotation_z;
			render_object_transformed_shaded(&work_transform, snooper_feet_models[animation_index]);
		}

		// Render score
		rdpq_set_mode_standard();
		rdpq_mode_combiner(RDPQ_COMBINER_TEX);
		rdpq_mode_blender(RDPQ_BLENDER((IN_RGB, IN_ALPHA, MEMORY_RGB, INV_MUX_ALPHA)));
		rdpq_sync_load();
		rdp_load_texture(0, 0, MIRROR_DISABLED, numbers_sprite);

		if (game_state.score >= 10) {
			render_digit(SCORE_X, SCORE_Y, game_state.score / 10);
		}

		render_digit(SCORE_X+10, SCORE_Y, game_state.score % 10);

		render_digit(SCORE_X+20, SCORE_Y, 10);

		uint16_t score_target = game_state.level->score_target;
		render_digit(SCORE_X+30, SCORE_Y, score_target / 10);
		render_digit(SCORE_X+40, SCORE_Y, score_target % 10);

		render_digit(SCORE_X+50, SCORE_Y, 14);
		render_digit(SCORE_X+58, SCORE_Y, 15);

		render_digit(DEATH_X, SCORE_Y, game_state.snooper_death_count);
		render_digit(DEATH_X+10, SCORE_Y, 10);
		render_digit(DEATH_X+20, SCORE_Y, game_state.level->snooper_death_cap);
		render_digit(DEATH_X+30, SCORE_Y, 12);
		render_digit(DEATH_X+38, SCORE_Y, 13);

		if (game_state.status == GAME_STATUS_WIN || game_state.status == GAME_STATUS_LOSE) {
			rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
			rdpq_set_prim_color(RGBA32(0xc0, 0xc0, 0xc0, 0x40));
			rdpq_texture_rectangle(0, 35, 40, 30+250, 40+128, 0.f, 0.f, 1.f, 1.f);

			rdpq_mode_combiner(RDPQ_COMBINER_TEX);
			sprite_t *sprite = game_state.status == GAME_STATUS_WIN ? win_sprite : lose_sprite;

			for (uint32_t y = 0; y < sprite->vslices; y++)
			{
				for (uint32_t x = 0; x < sprite->hslices; x++)
				{
					rdp_load_texture_stride(0, 0, MIRROR_DISABLED, sprite, y*sprite->hslices + x);
					rdp_draw_sprite(0, 40 + x * (sprite->width / sprite->hslices), 40 + y * (sprite->height / sprite->vslices), MIRROR_DISABLED);
				}
			}
		}
	}

	/*
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
	*/

	rdp_detach_show(disp);

	/*
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
	*/

	return true;
}
