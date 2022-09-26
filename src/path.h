#ifndef SPOOK64_PATH
#define SPOOK64_PATH

#include "vector.h"
#include "render.h"

typedef struct {
	const size_t length;
	const vector2_t *positions;
} path_t;

#define MAKE_PATH(positions) {\
	sizeof(positions)/sizeof(vector2_t),\
	positions,\
}

typedef struct {
	const path_t *path;
	int index;
	float progress;
} path_follower_t;

bool path_follow(path_follower_t *follower, vector2_t *out, float speed);

#endif
