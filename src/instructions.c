#include "instructions.h"
#include "render.h"
#include "sfx.h"
#include "macros.h"

const char *screen_paths[] = {
	"rom:/screen0_snooper.sprite",
	"rom:/screen1_spooker.sprite",
	"rom:/screen2_light.sprite",
	"rom:/screen3_controls.sprite",
};

void show_screen(const char *path, bool is_last) {
	load_screen(path);

	float alpha = 0.f;

	while (1) {
		alpha += 0.03f;
		if (alpha > 1.f) alpha = 1.f;

		do {
			audio_update();
		} while (!render_screen(alpha));

		controller_scan();
		struct controller_data ckeys = get_keys_held();
		if (alpha == 1.f && ckeys.c[0].A) {
			break;
		}
	}

	while (1) {
		alpha -= 0.03f;
		if (alpha < 0.f) alpha = 0.f;

		if (is_last) {
			sfx_set_music_volume(alpha);
		}

		do {
			audio_update();
		} while (!render_screen(alpha));

		if (alpha == 0.f) break;
	}
}

void show_instructions() {
	sfx_start_menu_music();

	for (int i = 0; i < ARRAY_LENGTH(screen_paths); i++) {
		show_screen(screen_paths[i], i + 1 == ARRAY_LENGTH(screen_paths));
	}

	sfx_stop_music();
}