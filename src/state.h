#ifndef SPOOK64_STATE
#define SPOOK64_STATE

#include "render.h"
#include "level.h"
#include "path.h"

#define MAX_SNOOPER_COUNT 32
#define MAX_SPOOKER_COUNT 4
#define MAX_LEVEL_LIGHT_COUNT 32
#define SNOOPER_DIE_DURATION 20
#define GAME_START_DURATION 90
#define GAME_END_DURATION 20

#define SPOOKER_KNOCKBACK_THRESHOLD 30

typedef enum {
	SNOOPER_STATUS_ALIVE=0,
	SNOOPER_STATUS_SPOOKED=1,
	SNOOPER_STATUS_DEAD=2,
	SNOOPER_STATUS_DYING=3,
} snooper_status_t;

typedef enum {
	GAME_STATUS_START=0,
	GAME_STATUS_PLAYING=1,
	GAME_STATUS_WIN=2,
	GAME_STATUS_LOSE=3,
	GAME_STATUS_PAUSED=4,
	GAME_STATUS_BEAT=5,
} game_status_t;

typedef struct {
	vector2_t position;
	float feet_rotation_z;
	float head_rotation_z;
	float head_target_rotation_z;
	path_follower_t path_follower;
	uint16_t rotate_timer;
	uint16_t freeze_timer;
	uint16_t light_brightness;
	uint16_t spooked_timer;
	float animation_progress;
	snooper_status_t status;
} snooper_state_t;

typedef struct {
	object_transform_t transform;
	vector2_t velocity;
	uint16_t knockback_timer;
	uint16_t spook_timer;
} spooker_state_t;

typedef struct {
	uint16_t timer;
} blink_state_t;

typedef struct {
	float velocity_x;
} hmove_state_t;

typedef union {
	blink_state_t blink;
	hmove_state_t hmove;
} level_light_type_state_t;

typedef struct {
	uint16_t brightness;
	bool is_on;
	vector2_t position;
	level_light_type_state_t type_state;
} level_light_state_t;

typedef struct {
	uint16_t snooper_count;
	uint16_t spooker_count;

	snooper_state_t snoopers[MAX_SNOOPER_COUNT];
	spooker_state_t spookers[MAX_SPOOKER_COUNT];

	level_light_state_t light_states[MAX_LEVEL_LIGHT_COUNT];

	vector3_t camera_position;

	uint16_t snooper_timer;
	uint16_t game_status_timer;

	uint16_t score;
	uint16_t snooper_death_count;

	game_status_t status;

	uint16_t level_index;
	const level_t *level;
} game_state_t;


extern game_state_t game_state;
void state_init();
void state_update();

#endif
