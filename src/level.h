#ifndef SPOOK64_LEVEL
#define SPOOK64_LEVEL
#include "path.h"
#include "libdragon.h"

typedef struct {
	const uint16_t width;
	const uint16_t height;

	const path_graph_t *path_graph;

	const uint8_t *data;
} level_t;

extern const level_t level0;

#endif
