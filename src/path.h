#ifndef SPOOK64_PATH
#define SPOOK64_PATH

#include "vector.h"
#include "render.h"
#include "macros.h"

typedef struct {
	int16_t waypoint_ancestor;
	vector2_t position;
} path_node_t;

typedef struct {
	int16_t src;
	int16_t dest;
} path_edge_t;

typedef struct {
	const int16_t node_count;
	const int16_t edge_count;

	const path_node_t *nodes;
	const path_edge_t *edges;
} path_graph_t;

#define MAKE_PATH_GRAPH(nodes, edges) {ARRAY_LENGTH(nodes), ARRAY_LENGTH(edges), nodes, edges}

typedef struct {
	int16_t src_index;
	int16_t dest_index;
	vector2_t position;
} path_follower_t;

void path_set_graph(const path_graph_t *graph);
void path_follower_init(path_follower_t *follower);
bool path_follow(path_follower_t *follower, float speed);

#endif
