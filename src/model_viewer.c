#include "model_viewer.h"
#include "render.h"
#include <math.h>
#include "state.h"

typedef struct {
	object_transform_t transform;
	sprite_t *sprite;
	float pitchiness;
	model_t *model;
} model_viewer_t;

void update_model_viewer(model_viewer_t *viewer) {
	controller_scan();
	struct controller_data ckeys = get_keys_held();

	if(ckeys.c[0].down)
	{
		viewer->pitchiness += 0.01f;
		if (viewer->pitchiness > 1.0f) {
			viewer->pitchiness = 1.0f;
		}
	}
	if(ckeys.c[0].up)
	{
		viewer->pitchiness -= 0.01f;
		if (viewer->pitchiness < 0.0f) {
			viewer->pitchiness = 0.0f;
		}
	}

	if (ckeys.c[0].left) {
		viewer->transform.rotation_z -= 0.05f;
	}
	if (ckeys.c[0].right) {
		viewer->transform.rotation_z += 0.05f;
	}
}

void render_model_viewer(model_viewer_t *viewer) {
	surface_t *disp;
	do {
		disp = display_lock();
	} while (disp == NULL);

	set_camera_pitch(viewer->pitchiness * 0.5f*(float)M_PI);
	game_state.camera_position.y = viewer->pitchiness * -7.0f;
	game_state.camera_position.z = 1.0f + 6.0f * (1.0f - viewer->pitchiness);

	// Clear the z buffer.
	clear_z_buffer();

    rdp_attach(disp);
	rdpq_set_z_image(&zbuffer);

	// Clear the framebuffer.
	rdpq_set_mode_fill(RGBA32(0, 0, 0, 0));
	rdpq_fill_rectangle(0, 0, 320, 240);

	// Render the model.
	rdpq_set_mode_standard();
	rdpq_mode_combiner(RDPQ_COMBINER_TEX_SHADE);
	rdpq_set_other_modes_raw(SOM_TEXTURE_PERSP);
	rdpq_change_other_modes_raw(SOM_SAMPLE_MASK, SOM_SAMPLE_BILINEAR);
	rdpq_change_other_modes_raw(SOM_Z_WRITE, SOM_Z_WRITE);
	rdpq_change_other_modes_raw(SOM_Z_COMPARE, SOM_Z_COMPARE);
	rdpq_change_other_modes_raw(SOM_TF_MASK, SOM_TF0_RGB);
	rdpq_mode_blender(RDPQ_BLENDER((IN_RGB, IN_ALPHA, IN_RGB, INV_MUX_ALPHA)));
	rdpq_mode_mipmap(MIPMAP_NONE, 0);
	rdp_load_texture(0, 0, MIRROR_DISABLED, viewer->sprite);
	render_object_transformed_shaded(&viewer->transform, viewer->model);

	rdp_detach_show(disp);
}

void show_model_viewer(int model_count, model_t **models, char *sprite_path) {
	game_state.camera_position.x = 0.0f;
	game_state.camera_position.y = 0.0f;
	game_state.camera_position.z = 5.0f;

	model_viewer_t viewer;
	viewer.transform.position.x = 0.0f;
	viewer.transform.position.y = 0.0f;
	viewer.transform.position.z = 0.0f;
	viewer.transform.rotation_z = 0.0f;
	viewer.sprite = sprite_load(sprite_path);
	viewer.pitchiness = 0.0f;

	int model_index = 0;
    while (1)
    {
		viewer.model = models[model_index];
        render_model_viewer(&viewer);
        render_model_viewer(&viewer);
		update_model_viewer(&viewer);

		model_index++;
		if (model_index == model_count) {
			model_index = 0;
		}
	}
}
