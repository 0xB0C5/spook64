#include "model_viewer.h"
#include "render.h"
#include <math.h>

static object_transform_t transform = {0.0f, 0.0f, 0.0f, 0.0f};
static sprite_t *sprite;

static float pitchiness = 0.0f;

void update_model_viewer() {
	controller_scan();
	struct controller_data ckeys = get_keys_held();

	if(ckeys.c[0].down)
	{
		pitchiness += 0.01f;
		if (pitchiness > 1.0f) {
			pitchiness = 1.0f;
		}
	}
	if(ckeys.c[0].up)
	{
		pitchiness -= 0.01f;
		if (pitchiness < 0.0f) {
			pitchiness = 0.0f;
		}
	}

	if (ckeys.c[0].left) {
		transform.rotation_z -= 0.05f;
	}
	if (ckeys.c[0].right) {
		transform.rotation_z += 0.05f;
	}
}

void render_model_viewer(model_t *model) {
	surface_t *disp;
	do {
		disp = display_lock();
	} while (disp == NULL);

	set_camera_pitch(pitchiness * 0.5f*(float)M_PI);
	camera_position[1] = pitchiness * -7.0f;
	camera_position[2] = 1.0f + 6.0f * (1.0f - pitchiness);

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
	rdpq_mode_blender(RDPQ_BLENDER((IN_RGB, IN_ALPHA, IN_RGB, INV_MUX_ALPHA)));
	rdp_load_texture(0, 0, MIRROR_DISABLED, sprite);
	render_object(&transform, model);

	rdp_detach_show(disp);
}

void show_model_viewer(model_t *model, char *sprite_path) {
	camera_position[0] = 0.0f;
	camera_position[1] = 0.0f;
	camera_position[2] = 5.0f;

	sprite = sprite_load(sprite_path);

    while (1)
    {
        render_model_viewer(model);
		update_model_viewer();
	}
}
