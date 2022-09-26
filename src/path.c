#include <math.h>
#include <stdbool.h>
#include "path.h"

bool path_follow(path_follower_t *follower, vector2_t *out, float speed) {
	const vector2_t *src = follower->path->positions + follower->index;
	const vector2_t *dest = src + 1;
	float delta_x = dest->x - src->x;
	float delta_y = dest->y - src->y;
	float dist = sqrtf(delta_x*delta_x + delta_y*delta_y);
	follower->progress += speed / dist;
	if (follower->progress > 1.f) {
		if (follower->index == follower->path->length - 2) {
			follower->progress = 1.f;
			out->x = dest->x;
			out->y = dest->y;
			return true;
		}
		follower->progress = 0.f;
		follower->index++;
		*out = follower->path->positions[follower->index];
		return false;
	}
	if (follower->progress < 0.f) {
		if (follower->index == 0) {
			follower->progress = 0.f;
			out->x = src->x;
			out->y = src->y;
			return true;
		}
		follower->progress = 1.f;
		*out = follower->path->positions[follower->index];
		follower->index--;
		return false;
	}
	out->x = src->x + follower->progress * delta_x;
	out->y = src->y + follower->progress * delta_y;
	return false;
}
