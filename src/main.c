#include "libdragon.h"
#include <malloc.h>
#include "render.h"

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

    while (1)
    {
        render();

        if (audio_can_write()) {    	
            short *buf = audio_write_begin();
            mixer_poll(buf, audio_get_buffer_length());
            audio_write_end();
        }
    }
}
