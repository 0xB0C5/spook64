#include <math.h>
#include <stdbool.h>
#include "path.h"
#include "rand.h"
#include "debug.h"

#define MAX_NODE_COUNT 64
#define MAX_EDGE_COUNT 128
#define MAX_START_NODE_COUNT 16

static const path_graph_t *path_graph = NULL;

static int16_t child_counts[MAX_NODE_COUNT];
static int16_t children_starts[MAX_NODE_COUNT];
static int16_t children[MAX_EDGE_COUNT];
static int16_t start_nodes[MAX_START_NODE_COUNT];
static int16_t start_node_count;

void path_set_graph(const path_graph_t *graph) {
	assertf(graph->node_count <= MAX_NODE_COUNT, "Too many nodes.");
	assertf(graph->edge_count <= MAX_EDGE_COUNT, "Too many edges.");

	path_graph = graph;

	// Compute child counts.
	for (int16_t i = 0; i < graph->node_count; i++) {
		child_counts[i] = 0;
	}
	for (int16_t edge_index = 0; edge_index < graph->edge_count; edge_index++) {
		child_counts[graph->edges[edge_index].src]++;
	}

	// Compute children_starts
	{
		int16_t children_index = 0;
		for (int16_t i = 0; i < graph->node_count; i++) {
			children_starts[i] = children_index;
			children_index += child_counts[i];
		}
	}

	// Compute children (re-computing child_counts)
	for (int16_t i = 0; i < graph->node_count; i++) {
		child_counts[i] = 0;
	}
	for (int16_t edge_index = 0; edge_index < graph->edge_count; edge_index++) {
		const path_edge_t *edge = graph->edges+edge_index;
		children[children_starts[edge->src] + child_counts[edge->src]++] = edge->dest;
	}

	// Compute start nodes.
	start_node_count = 0;
	for (int16_t i = 0; i < graph->node_count; i++) {
		if (graph->nodes[i].waypoint_ancestor < 0) {
			assertf(start_node_count < MAX_START_NODE_COUNT, "Too many start nodes.");
			start_nodes[start_node_count++] = i;
		}
	}
}

void path_follower_init(path_follower_t *follower) {
	follower->dest_index = start_nodes[RANDN(start_node_count)];
	follower->src_index = -1;
	follower->position = path_graph->nodes[follower->dest_index].position;
}

bool path_follow(path_follower_t *follower, float speed) {
	int16_t target_index;
	if (speed >= 0.f) {
		target_index = follower->dest_index;
	} else {
		target_index = follower->src_index;
	}

	if (target_index == -1) {
		return true;
	}

	const vector2_t *target = &path_graph->nodes[target_index].position;

	float delta_x = target->x - follower->position.x;
	float delta_y = target->y - follower->position.y;
	float dist = sqrtf(delta_x*delta_x + delta_y*delta_y);

	float ratio = dist == 0.f ? 2.f : fabs(speed) / dist;
	if (ratio < 1.f) {
		// Normal movement - no target node change.
		follower->position.x += ratio * delta_x;
		follower->position.y += ratio * delta_y;

		return false;
	}

	// Advance node.
	follower->position = *target;
	if (speed >= 0.f) {
		// Move to next node.
		int16_t child_count = child_counts[target_index];
		follower->src_index = target_index;
		if (child_count == 0) {
			follower->dest_index = -1;
		} else {
			follower->dest_index = children[children_starts[target_index] + RANDN(child_count)];
		}

		return false;
	}

	// Speed is negative - move to prev node.
	follower->dest_index = target_index;
	follower->src_index = path_graph->nodes[target_index].waypoint_ancestor;

	return false;
}
