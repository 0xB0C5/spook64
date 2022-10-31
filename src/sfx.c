#include "sfx.h"
#include "rand.h"
#include <math.h>

static uint16_t sfx_channel_index;

#define SNOOPER_SCREAM_COUNT 2
#define SNOOPER_DEATH_COUNT 2
#define SNOOPER_SPEAK_COUNT 3
#define SPOOKER_SPOOK_COUNT 1

static uint16_t prev_snooper_scream = 0;
static uint16_t prev_snooper_speak = 0;
static uint16_t prev_snooper_death = 0;

static float point_freq;
static float point_lo_vol;

static wav64_t snooper_screams[SNOOPER_SCREAM_COUNT];
static wav64_t snooper_deaths[SNOOPER_DEATH_COUNT];
static wav64_t snooper_speaks[SNOOPER_SPEAK_COUNT];

static wav64_t spooker_spook;
static wav64_t spooker_spook_muffled;

static wav64_t spooker_oof;

static wav64_t point;
static wav64_t bad;

static wav64_t music;
static wav64_t menu_music;
static wav64_t win;
static wav64_t lose;
static wav64_t level_start;

void sfx_init() {
    wav64_open(snooper_screams+0, "scream2.wav64");
    wav64_open(snooper_screams+1, "scream3.wav64");

    wav64_open(snooper_deaths+0, "scream0.wav64");
    wav64_open(snooper_deaths+1, "scream1.wav64");

    wav64_open(snooper_speaks+0, "speak0.wav64");
    wav64_open(snooper_speaks+1, "speak1.wav64");
    wav64_open(snooper_speaks+2, "speak2.wav64");

	wav64_open(&spooker_spook, "ah.wav64");
	wav64_open(&spooker_spook_muffled, "ah_muffled.wav64");
	wav64_open(&spooker_oof, "oof.wav64");

	wav64_open(&point, "point.wav64");

	wav64_open(&music, "spooky_swing.wav64");

	wav64_open(&bad, "bad.wav64");

	wav64_open(&win, "win.wav64");
	wav64_open(&lose, "lose.wav64");
	wav64_open(&level_start, "level_start.wav64");

	wav64_open(&menu_music, "spooky_menu.wav64");

	sfx_channel_index = 0;

	point_freq = 16000.f;
	point_lo_vol = 0.f;

	music.wave.loop_len = 1148411L;
	menu_music.wave.loop_len = 512000L;
}

static void sfx_play_randfreq(wav64_t *w) {
	mixer_ch_play(sfx_channel_index, &w->wave);
	mixer_ch_set_freq(sfx_channel_index, (1.f + 0.2f * randf())*16000.f);
	mixer_ch_set_vol(sfx_channel_index, 0.2f, 0.2f);

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

void sfx_snooper_die() {
	sfx_play_choice(&prev_snooper_death, snooper_deaths, SNOOPER_DEATH_COUNT);
}

void sfx_spooker_spook() {
	sfx_play_randfreq(&spooker_spook);
}

void sfx_spooker_spook_muffled() {
	sfx_play_randfreq(&spooker_spook_muffled);
}

void sfx_spooker_oof() {
	sfx_play_randfreq(&spooker_oof);
}

void sfx_bad() {
	mixer_ch_play(sfx_channel_index, &bad.wave);
	mixer_ch_set_vol(sfx_channel_index, 0.3f, 0.3f);

	sfx_channel_index = (sfx_channel_index + 1) % 16;
}

void sfx_point() {
	float point_hi_vol = 1.f - point_lo_vol;

	float hi_vol = sqrtf(point_hi_vol);
	float lo_vol = sqrtf(point_lo_vol);

	mixer_ch_play(sfx_channel_index, &point.wave);
	mixer_ch_set_freq(sfx_channel_index, point_freq);
	mixer_ch_set_vol(sfx_channel_index, 0.5f*hi_vol, 0.5f*hi_vol);

	sfx_channel_index = (sfx_channel_index + 1) % 16;

	mixer_ch_play(sfx_channel_index, &point.wave);
	mixer_ch_set_freq(sfx_channel_index, 0.5f*point_freq);
	mixer_ch_set_vol(sfx_channel_index, 0.5f*lo_vol, 0.5f*lo_vol);

	sfx_channel_index = (sfx_channel_index + 1) % 16;

	point_freq *= 1.05946309f;
	point_lo_vol += 1.f/12.f;

	if (point_lo_vol > 1.f) {
		point_freq /= 2;
		point_lo_vol -= 1.f;
	}
}

void sfx_start_music() {
	mixer_ch_play(16, &music.wave);
	mixer_ch_set_vol(16, 0.25f, 0.25f);
}

void sfx_start_menu_music() {
	mixer_ch_play(16, &menu_music.wave);
	mixer_ch_set_vol(16, 0.25f, 0.25f);
}

void sfx_stop_music() {
	mixer_ch_stop(16);
}

void sfx_set_music_volume(float volume) {
	volume *= 0.25f;
	mixer_ch_set_vol(16, volume, volume);
}

void sfx_win() {
	mixer_ch_play(16, &win.wave);
	mixer_ch_set_vol(16, 0.25f, 0.25f);
}

void sfx_lose() {
	mixer_ch_play(16, &lose.wave);
	mixer_ch_set_vol(16, 0.25f, 0.25f);
}

void sfx_level_start() {
	mixer_ch_play(16, &level_start.wave);
	mixer_ch_set_vol(16, 0.25f, 0.25f);
}

void audio_update() {
	if (audio_can_write()) {    	
		short *buf = audio_write_begin();
		mixer_poll(buf, audio_get_buffer_length());
		audio_write_end();
	}
}

