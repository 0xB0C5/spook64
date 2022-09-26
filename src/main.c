#include "libdragon.h"
#include <malloc.h>
#include "render.h"
#include "_generated_models.h"
#include "model_viewer.h"
#include "state.h"
#include "sfx.h"

/*
model_t *test_models[] = {
	&snooper_animated_000001_model,
	&snooper_animated_000002_model,
	&snooper_animated_000003_model,
	&snooper_animated_000004_model,
	&snooper_animated_000005_model,
	&snooper_animated_000006_model,
	&snooper_animated_000007_model,
	&snooper_animated_000008_model,
	&snooper_animated_000009_model,
	&snooper_animated_000010_model,
	&snooper_animated_000011_model,
	&snooper_animated_000012_model,
	&snooper_animated_000013_model,
	&snooper_animated_000014_model,
	&snooper_animated_000015_model,
	&snooper_animated_000016_model,
	&snooper_animated_000017_model,
	&snooper_animated_000018_model,
	&snooper_animated_000019_model,
	&snooper_animated_000020_model,
};
*/

int main()
{
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 3, GAMMA_NONE, ANTIALIAS_RESAMPLE);

    debug_init_isviewer();
    debug_init_usblog();

    controller_init();
    timer_init();

    dfs_init(DFS_DEFAULT_LOCATION);

    audio_init(44100, 4);
    mixer_init(32);

    rdp_init();
    rdpq_debug_start();

	renderer_init();
	state_init();

	sfx_init();

	// show_model_viewer(sizeof(test_models) / sizeof(model_t*), test_models, "rom:/snooper.sprite");

    while (1)
    {
        if (render()) {
			state_update();
		}

        if (audio_can_write()) {    	
            short *buf = audio_write_begin();
            mixer_poll(buf, audio_get_buffer_length());
            audio_write_end();
        }
    }
}
