#include "sfx.h"
#include "rand.h"

static uint16_t sfx_channel_index;

#define SNOOPER_SCREAM_COUNT 4
#define SNOOPER_SPEAK_COUNT 3
#define SPOOKER_SPOOK_COUNT 1

static uint16_t prev_snooper_scream = 0;
static uint16_t prev_snooper_speak = 0;

static wav64_t snooper_screams[SNOOPER_SCREAM_COUNT];
static wav64_t snooper_speaks[SNOOPER_SPEAK_COUNT];

static wav64_t spooker_spooks[SPOOKER_SPOOK_COUNT];

static wav64_t bass;

void sfx_init() {
    wav64_open(snooper_screams+0, "scream0.wav64");
    wav64_open(snooper_screams+1, "scream1.wav64");
    wav64_open(snooper_screams+2, "scream2.wav64");
    wav64_open(snooper_screams+3, "scream3.wav64");

    wav64_open(snooper_speaks+0, "speak0.wav64");
    wav64_open(snooper_speaks+1, "speak1.wav64");
    wav64_open(snooper_speaks+2, "speak2.wav64");

	wav64_open(spooker_spooks, "ah.wav64");

	wav64_open(&bass, "bass.wav64");

	sfx_channel_index = 0;

	// This is how you loops stuff.
//	bass.wave.loop_len = 295;
//	mixer_ch_play(16, &bass.wave);
}

static void sfx_play_randfreq(wav64_t *w) {
	mixer_ch_play(sfx_channel_index, &w->wave);
	mixer_ch_set_freq(sfx_channel_index, (1.f + 0.2f * randf())*16000.f);
	mixer_ch_set_vol(sfx_channel_index, 0.25f, 0.25f);

	sfx_channel_index = (sfx_channel_index + 1) % 16;
}

static void sfx_play_choice(uint16_t *prev_idx, wav64_t *table, const int n) {
	uint16_t idx = RANDN(n-1);
	if (idx >= *prev_idx) {
		idx++;
	}
	sfx_play_randfreq(&table[idx]);

	*prev_idx = idx;
}

void sfx_snooper_scream() {
	sfx_play_choice(&prev_snooper_scream, snooper_screams, SNOOPER_SCREAM_COUNT);
}

void sfx_snooper_speak() {
	sfx_play_choice(&prev_snooper_speak, snooper_speaks, SNOOPER_SPEAK_COUNT);
}

void sfx_spooker_spook() {
	sfx_play_randfreq(spooker_spooks);
}