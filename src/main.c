
#include "dragon.h"
#include <malloc.h>
#include "render.h"
#include "_generated_models.h"
#include "primitive_models.h"
#include "model_viewer.h"
#include "state.h"
#include "sfx.h"
#include "instructions.h"
#include "end_screen.h"

model_t *test_models[] = {
	&floor_model,
};

int main()
{
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 3, GAMMA_NONE, ANTIALIAS_RESAMPLE);
	debug_init(DEBUG_FEATURE_LOG_ISVIEWER);
	debugf("debug test!\n");

    controller_init();
    timer_init();

    dfs_init(DFS_DEFAULT_LOCATION);

    audio_init(44100, 4);
    mixer_init(17);

    rdp_init();
    rdpq_debug_start();

	// TODO : for some reason it's generating underflow exceptions despite C1_FCR31_FS being set?
	// Disable underflow exceptions.
	// (C1_FCR31_FS is supposed to flush denormals to 0?)
	C1_WRITE_FCR31(
		C1_ENABLE_OVERFLOW
		//| C1_ENABLE_UNDERFLOW
		| C1_ENABLE_DIV_BY_0
		| C1_ENABLE_INVALID_OP
		| C1_FCR31_FS);

	renderer_init();
	sfx_init();

	show_instructions();

	state_init();

	const uint32_t ticks_per_frame = 1562500LL;
	const uint32_t max_delay = 156250LL;

	uint32_t last_update = (uint32_t)timer_ticks();
	uint32_t extra_ticks = 0;

    while (game_state.status != GAME_STATUS_BEAT)
    {
		// Wait for next frame.
		uint32_t now;
		do {
			audio_update();
			now = (uint32_t)timer_ticks();
		} while (now - last_update < ticks_per_frame);

		last_update += ticks_per_frame;
		if (now - last_update > max_delay) {
			extra_ticks += now - last_update;
			// Slowdown!
			last_update = now;
		}

		render();
		audio_update();
		state_update();
		// Update the state at 30fps regardless of graphics framerate.
		while (extra_ticks >= ticks_per_frame) {
			extra_ticks -= ticks_per_frame;
			audio_update();
			state_update();
		}
    }

	show_end_screen();
}
