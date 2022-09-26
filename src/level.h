#ifndef SPOOK64_LEVEL
#define SPOOK64_LEVEL
#include "path.h"
#include "libdragon.h"

typedef struct {
	const uint16_t width;
	const uint16_t height;

	const size_t path_count;
	const path_t **paths;

	const uint8_t *data;
} level_t;

#define MAKE_LEVEL(half_width, half_height, paths, data) {\
	half_width,\
	half_height,\
	sizeof(paths)/sizeof(paths[0]),\
	paths,\
	data,\
}

extern const level_t level0;

#endif
