#include "end_screen.h"
#include "render.h"
#include "sfx.h"

void show_end_screen() {
	sfx_start_win_music();

	load_screen("rom:/screen_beat.sprite");

	float alpha = 0.f;

	while (1) {
		alpha += 0.03f;
		if (alpha > 1.f) alpha = 1.f;

		do {
			audio_update();
		} while (!render_screen(alpha));
	}
}
