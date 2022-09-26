#ifndef SPOOK64_STATE
#define SPOOK64_STATE

#include "render.h"
#include "level.h"

#define MAX_SNOOPER_COUNT 32
#define MAX_SPOOKER_COUNT 4

typedef enum {
	SNOOPER_STATUS_ALIVE=0,
	SNOOPER_STATUS_SPOOKED=1,
	SNOOPER_STATUS_DEAD=2,
} snooper_status_t;

typedef struct {
	vector2_t position;
	float feet_rotation_z;
	float head_rotation_z;
	float head_target_rotation_z;
	path_follower_t path_follower;
	uint32_t rotate_timer;
	uint32_t freeze_timer;
	snooper_status_t status;
} snooper_state_t;

typedef struct {
	object_transform_t transform;
	vector2_t velocity;
	uint16_t knockback_timer;
	uint16_t spook_timer;
} spooker_state_t;

typedef struct {
	uint16_t snooper_count;
	uint16_t spooker_count;

	snooper_state_t snoopers[MAX_SNOOPER_COUNT];
	spooker_state_t spookers[MAX_SPOOKER_COUNT];

	vector3_t camera_position;

	uint32_t snooper_timer;
	
	const level_t *level;
} game_state_t;

extern game_state_t game_state;
void state_init();
void state_update();

#endif
