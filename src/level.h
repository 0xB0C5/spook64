#ifndef SPOOK64_LEVEL
#define SPOOK64_LEVEL
#include "path.h"
#include "dragon.h"

typedef enum {
	LIGHT_TYPE_STATIC=0,
	LIGHT_TYPE_BLINK=1,
	LIGHT_TYPE_HMOVE=2,
} light_type_t;

typedef struct {
	vector2_t position;
	float radius;
	light_type_t type;
} level_light_t;

typedef struct {
	const uint16_t width;
	const uint16_t height;

	const uint16_t score_target;
	const uint16_t snooper_death_cap;

	const uint16_t min_snooper_spawn_duration;
	const uint16_t max_snooper_spawn_duration;

	const path_graph_t *path_graph;

	const uint8_t *data;

	const uint8_t light_count;
	const level_light_t *lights;

	const char *name;
} level_t;

extern const level_t **levels;

#endif
