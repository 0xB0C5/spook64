#include "dragon.h"
#include <malloc.h>
#include "render.h"
#include "_generated_models.h"
#include "primitive_models.h"
#include "model_viewer.h"
#include "state.h"
#include "sfx.h"

model_t *test_models[] = {
	&floor_model,
};

void audio_update() {
	if (audio_can_write()) {    	
		short *buf = audio_write_begin();
		mixer_poll(buf, audio_get_buffer_length());
		audio_write_end();
	}
}

int main()
{
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 3, GAMMA_NONE, ANTIALIAS_RESAMPLE);
	debug_init(DEBUG_FEATURE_LOG_ISVIEWER);
	debugf("debug test!\n");

    controller_init();
    timer_init();

    dfs_init(DFS_DEFAULT_LOCATION);

    audio_init(44100, 4);
    mixer_init(32);

    rdp_init();
	// TODO : rdpq debug is slowing things to a crawl.
	// investigate wtf I'm doing wrong!
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
	state_init(&level0);

	sfx_init();
	// show_model_viewer(ARRAY_LENGTH(test_models), test_models, "rom:/test.sprite");

	
	const uint32_t ticks_per_frame = 1562500LL;
	const uint32_t max_delay = 156250LL;

	uint32_t last_update = (uint32_t)timer_ticks();

    while (1)
    {
		// Wait for next frame.
		uint32_t now;
		do {
			audio_update();
			now = (uint32_t)timer_ticks();
		} while (now - last_update < ticks_per_frame);

		last_update += ticks_per_frame;
		if (now - last_update > max_delay) {
			// Slowdown!
			last_update = now;
		}

        if (render()) {
			state_update();
		} else {
			debugf("skipped render!\n");
		}
    }
}
