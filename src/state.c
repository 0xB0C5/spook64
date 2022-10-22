#include "state.h"
#include "dragon.h"
#include "math.h"
#include "rand.h"

#include "sfx.h"

#define SPOOK_DISTANCE 3.0f

#define SNOOPER_MAX_Y 11.f
#define SNOOPER_MIN_Y 0.f
#define SNOOPER_SPEED 0.08f
#define SNOOPER_RUN_SPEED 0.6f
#define SNOOPER_LIGHT_ANGLE (0.17f * M_PI)
#define SNOOPER_LIGHT_MAX_DIST 5.5f
#define SNOOPER_LIGHT_MIN_DIST 1.f
#define SNOOPER_LOOK_DIST 8.f

#define SPOOKER_SPEED 0.35f
#define SPOOKER_KNOCKBACK_DURATION 30

#define CAMERA_OFFSET_Y -12.f
#define CAMERA_OFFSET_Z 12.f

#define C_CAMERA_OFFSET 6.f

#define CAMERA_BOUNDS_X 4.f
#define CAMERA_BOUNDS_MAX_Y 6.f
#define CAMERA_BOUNDS_MIN_Y 4.f

game_state_t game_state;

void state_init(const level_t *level) {
	game_state.spooker_count = 1;
	game_state.spookers[0].transform.position.x = 0.f;
	game_state.spookers[0].transform.position.y = 0.f;
	game_state.spookers[0].transform.position.z = 0.f;
	game_state.spookers[0].velocity.x = 0.f;
	game_state.spookers[0].velocity.y = 0.f;
	game_state.spookers[0].knockback_timer = 0;

	game_state.snooper_count = 0;

	game_state.camera_position.x = 0.f;
	game_state.camera_position.y = CAMERA_OFFSET_Y;
	game_state.camera_position.z = CAMERA_OFFSET_Z;

	game_state.snooper_timer = 1;

	game_state.level = level;

	path_set_graph(level->path_graph);

	for (int i = 0; i < game_state.level->light_count; i++) {
		switch (game_state.level->lights[i].type) {
			case LIGHT_TYPE_STATIC:
				game_state.light_states[i].brightness = 90;
				game_state.light_states[i].is_on = 1;
				break;
			case LIGHT_TYPE_BLINK:
				game_state.light_states[i].brightness = 0;
				game_state.light_states[i].is_on = 0;
				game_state.light_states[i].type_state.blink.timer = 120 + RANDN(240);
				break;
		}
	}
}

static void spawn_snooper() {
	if (game_state.snooper_count < MAX_SNOOPER_COUNT) {
		snooper_state_t *new_snooper = &game_state.snoopers[game_state.snooper_count++];

		new_snooper->status = SNOOPER_STATUS_ALIVE;

		path_follower_init(&new_snooper->path_follower);
		new_snooper->position = new_snooper->path_follower.position;

		new_snooper->feet_rotation_z = M_PI;
		new_snooper->head_rotation_z = M_PI;

		new_snooper->rotate_timer = 1;
		new_snooper->freeze_timer = 0;

		new_snooper->light_brightness = 0;
	}
}

static size_t get_light_at(float x, float y, vector2_t *out) {
	for (size_t i = 0; i < game_state.snooper_count; i++) {
		snooper_state_t *snooper = game_state.snoopers + i;
		if (snooper->status != SNOOPER_STATUS_ALIVE) {
			continue;
		}
		float dx = x - snooper->position.x;
		float dy = y - snooper->position.y;

		float dist2 = dx*dx + dy*dy;
		if (dist2 > SNOOPER_LIGHT_MAX_DIST*SNOOPER_LIGHT_MAX_DIST) continue;
		if (dist2 < SNOOPER_LIGHT_MIN_DIST*SNOOPER_LIGHT_MIN_DIST) continue;

		float angle = atan2f(dx, dy);
		float angle_diff = angle - snooper->head_rotation_z;
		if (angle_diff < -M_PI) {
			angle_diff += 2.f*M_PI;
		} else if (angle_diff > M_PI) {
			angle_diff -= 2.f*M_PI;
		}
		if (angle_diff < 0.5f*SNOOPER_LIGHT_ANGLE && angle_diff > -0.5f*SNOOPER_LIGHT_ANGLE) {
			float dist = sqrtf(dist2);
			out->x = dx / dist;
			out->y = dy / dist;
			return i;
		}
	}

	for (size_t i = 0; i < game_state.level->light_count; i++) {
		if (!game_state.light_states[i].is_on) continue;

		const level_light_t *light = &game_state.level->lights[i];

		float dx = x - light->position.x;
		float dy = y - light->position.y;

		float dist2 = dx*dx + dy*dy;
		float hit_radius = 0.8f * light->radius;
		if (dist2 > hit_radius * hit_radius) continue;

		if (dist2 < 0.01f) {
			out->x = 0.f;
			out->y = -1.f;
		} else {
			float dist = sqrtf(dist2);
			out->x = dx / dist;
			out->y = dy / dist;
		}
		return MAX_SNOOPER_COUNT;
	}

	return MAX_SNOOPER_COUNT+1;
}

float step_to_angle(float cur, float target) {
	float diff = target - cur;
	// Normalize diff to [-pi, pi]
	if (diff < -M_PI) {
		diff += 2.f*M_PI;
	} else if (diff > M_PI) {
		diff -= 2.f*M_PI;
	}

	float res = cur + diff / 4.f;
	if (res < -M_PI) {
		res += 2.f*M_PI;
	} else if (res > M_PI) {
		res -= 2.f*M_PI;
	}
	return res;
}


void update_light_brightness(uint16_t *brightness) {
	if (*brightness < 85) {
		*brightness += RANDN(20);
	} else {
		*brightness += RANDN(5) - 2;
		if (*brightness > 100) *brightness = 100;
	}
}

void state_update() {
	// Spawn snooper?
	game_state.snooper_timer--;
	if (game_state.snooper_count == 0 || game_state.snooper_timer == 0) {
		game_state.snooper_timer = 120;

		spawn_snooper();
	}

	// Move snoopers
	for (uint16_t i = 0; i < game_state.snooper_count; i++) {
		snooper_state_t *snooper = game_state.snoopers + i;

		update_light_brightness(&snooper->light_brightness);

		float speed = snooper->status == SNOOPER_STATUS_ALIVE ? SNOOPER_SPEED : -SNOOPER_RUN_SPEED;
		if (snooper->freeze_timer != 0) {
			speed *= 0.5f;
		}

		bool end = path_follow(&snooper->path_follower, speed);

		if (end) {
			snooper->status = SNOOPER_STATUS_DEAD;
			continue;
		}
		float dx = snooper->path_follower.position.x - snooper->position.x;
		float dy = snooper->path_follower.position.y - snooper->position.y;
		if (dx != 0.f || dy != 0.f) {
			snooper->feet_rotation_z = atan2f(dx, dy);
		}
		float smoothness = snooper->status == SNOOPER_STATUS_ALIVE ? 0.9f : 0.8f;
		float roughness = 1.f - smoothness;
		snooper->position.x = smoothness * snooper->position.x + roughness * snooper->path_follower.position.x;
		snooper->position.y = smoothness * snooper->position.y + roughness * snooper->path_follower.position.y;

		if (snooper->freeze_timer == 0) {
			if (snooper->status == SNOOPER_STATUS_ALIVE) {
				if (--snooper->rotate_timer == 0) {
					snooper->rotate_timer = 30 + RANDN(90);
					float target_rotation_z = snooper->feet_rotation_z + (M_PI/2.0f)*(randf()-0.5f);
					if (target_rotation_z < -M_PI) {
						target_rotation_z += 2.f*M_PI;
					} else if (target_rotation_z > M_PI) {
						target_rotation_z -= 2.f*M_PI;
					}
					snooper->head_target_rotation_z = target_rotation_z;
				}
			} else {
				snooper->head_target_rotation_z = snooper->feet_rotation_z;
			}
		} else {
			snooper->freeze_timer--;

			if (snooper->status == SNOOPER_STATUS_ALIVE) {
				// Look at spooker if nearby and being knocked back.
				if (game_state.spookers[0].knockback_timer >= SPOOKER_KNOCKBACK_THRESHOLD) {
					vector3_t *spooker_pos = &game_state.spookers[0].transform.position;
					vector2_t *snooper_pos = &snooper->position;
					float dx = spooker_pos->x - snooper_pos->x;
					float dy = spooker_pos->y - snooper_pos->y;

					if (dx*dx + dy*dy < SNOOPER_LOOK_DIST*SNOOPER_LOOK_DIST) {
						snooper->head_target_rotation_z = atan2f(dx, dy);
					}
				}
			} else if (snooper->status == SNOOPER_STATUS_SPOOKED) {
				if (snooper->freeze_timer == 0) {
					sfx_snooper_scream();
				}
			}
		}

		snooper->head_rotation_z = step_to_angle(snooper->head_rotation_z, snooper->head_target_rotation_z);
	}

	// Remove killed snoopers
	{
		uint16_t i = 0;
		uint16_t j = 0;
		while (j < game_state.snooper_count) {
			if (game_state.snoopers[j].status == SNOOPER_STATUS_DEAD) {
				j++;
			} else {
				if (i != j) {
					game_state.snoopers[i] = game_state.snoopers[j];
				}
				i++;
				j++;
			}
		}
		game_state.snooper_count = i;
	}

	controller_scan();
	struct controller_data ckeys = get_keys_held();

	// Move spooker
	{
		spooker_state_t *spooker = game_state.spookers;

		if (spooker->knockback_timer == 0) {
			vector2_t light_direction;
			size_t snooper_light_index = get_light_at(
					spooker->transform.position.x,
					spooker->transform.position.y,
					&light_direction);

			if (snooper_light_index < MAX_SNOOPER_COUNT+1) {
				if (snooper_light_index < MAX_SNOOPER_COUNT) sfx_snooper_speak();
				spooker->velocity.x = 0.5f * light_direction.x;
				spooker->velocity.y = 0.5f * light_direction.y;
				spooker->knockback_timer = SPOOKER_KNOCKBACK_THRESHOLD + SPOOKER_KNOCKBACK_DURATION;
				spooker->spook_timer = 0;
				game_state.snoopers[snooper_light_index].freeze_timer = 120;
				game_state.snoopers[snooper_light_index].rotate_timer = 1;
			}
		}

		if (spooker->knockback_timer < SPOOKER_KNOCKBACK_THRESHOLD) {
			// TODO : scale down based on bounds of controller x/y?
			float dx;
			float dy;
			if (spooker->spook_timer == 0) {
				dx =  SPOOKER_SPEED * ckeys.c[0].x;
				dy =  SPOOKER_SPEED * ckeys.c[0].y;

				float speed2 = dx*dx + dy*dy;
				if (speed2 > SPOOKER_SPEED*SPOOKER_SPEED) {
					float speed = sqrtf(speed2);
					float scale = SPOOKER_SPEED / speed;
					dx *= scale;
					dy *= scale;
				}
			} else {
				spooker->spook_timer--;
				dx = 0.f;
				dy = 0.f;
			}

			spooker->velocity.x = (spooker->velocity.x + dx) / 2.f;
			spooker->velocity.y = (spooker->velocity.y + dy) / 2.f;

			if (spooker->velocity.x != 0 || spooker->velocity.y != 0) {
				spooker->transform.position.x += spooker->velocity.x;
				spooker->transform.position.y += spooker->velocity.y;

				float target_angle = atan2f(spooker->velocity.x, spooker->velocity.y);
				spooker->transform.rotation_z = step_to_angle(spooker->transform.rotation_z, target_angle);
			}
		} else {
			// knockback
			spooker->transform.position.x += spooker->velocity.x;
			spooker->transform.position.y += spooker->velocity.y;
			spooker->transform.rotation_z += 0.02f * (spooker->knockback_timer - SPOOKER_KNOCKBACK_THRESHOLD);

			spooker->velocity.x *= 0.92f;
			spooker->velocity.y *= 0.92f;

			if (spooker->transform.rotation_z > M_PI) {
				spooker->transform.rotation_z -= 2.f*M_PI;
			}
		}

		if (spooker->knockback_timer != 0) spooker->knockback_timer--;

		float spooker_max_x = game_state.level->width-2.f;
		if (spooker->transform.position.x > spooker_max_x) {
			spooker->transform.position.x = spooker_max_x;
		}
		if (spooker->transform.position.x < -spooker_max_x) {
			spooker->transform.position.x = -spooker_max_x;
		}
		float spooker_max_y = game_state.level->height-2.f;
		if (spooker->transform.position.y > spooker_max_y) {
			spooker->transform.position.y = spooker_max_y;
		}
		if (spooker->transform.position.y < -spooker_max_y) {
			spooker->transform.position.y = -spooker_max_y;
		}
	}

	if (ckeys.c[0].A && game_state.spookers[0].spook_timer == 0 && game_state.spookers[0].knockback_timer == 0) {
		sfx_spooker_spook();
		for (uint16_t i = 0; i < game_state.snooper_count; i++) {
			if (game_state.snoopers[i].status != SNOOPER_STATUS_ALIVE) {
				continue;
			}
			float dx = game_state.snoopers[i].position.x - game_state.spookers[0].transform.position.x;
			float dy = game_state.snoopers[i].position.y - game_state.spookers[0].transform.position.y;
			float dist2 = dx*dx + dy*dy;
			if (dist2 < SPOOK_DISTANCE*SPOOK_DISTANCE) {
				game_state.snoopers[i].status = SNOOPER_STATUS_SPOOKED;
				game_state.snoopers[i].freeze_timer = 5;
			}
		}
		game_state.spookers[0].spook_timer = 15;
	}

	// Move camera towards spooker
	{
		float target_x = game_state.spookers[0].transform.position.x;
		float target_y = game_state.spookers[0].transform.position.y;

		float max_x = game_state.level->width - CAMERA_BOUNDS_X;
		if (target_x > max_x) {
			target_x = max_x;
		}
		float min_x = -max_x;
		if (target_x < min_x) {
			target_x = min_x;
		}
		float max_y = game_state.level->height - CAMERA_BOUNDS_MAX_Y;
		if (target_y > max_y) {
			target_y = max_y;
		}
		float min_y = -game_state.level->height + CAMERA_BOUNDS_MIN_Y;
		if (target_y < min_y) {
			target_y = min_y;
		}

		if (ckeys.c[0].C_down) target_y -= C_CAMERA_OFFSET;
		if (ckeys.c[0].C_up) target_y += C_CAMERA_OFFSET;
		if (ckeys.c[0].C_left) target_x -= C_CAMERA_OFFSET;
		if (ckeys.c[0].C_right) target_x += C_CAMERA_OFFSET;

		target_y += CAMERA_OFFSET_Y;

		game_state.camera_position.x = 0.75f * game_state.camera_position.x + 0.25f*target_x;
		game_state.camera_position.y = 0.75f * game_state.camera_position.y + 0.25f*target_y;
	}

	// Update level lights
	for (int i = 0; i < game_state.level->light_count; i++) {
		level_light_state_t *light_state = &game_state.light_states[i];
		switch (game_state.level->lights[i].type) {
			case LIGHT_TYPE_STATIC:
				update_light_brightness(&light_state->brightness);
				break;
			case LIGHT_TYPE_BLINK:
				if (light_state->type_state.blink.timer == 0) {
					light_state->is_on = !light_state->is_on;
					light_state->type_state.blink.timer = 90 + RANDN(240);
				}
				light_state->type_state.blink.timer--;

				if (!light_state->is_on && light_state->type_state.blink.timer < 32) {
					light_state->brightness = RANDN(50);
				} else if (light_state->is_on) {
					if (light_state->brightness < 50) light_state->brightness = 50;
					update_light_brightness(&light_state->brightness);
				} else if (light_state->brightness) {
					if (light_state->type_state.blink.timer & 1) {
						light_state->brightness -= 30;
						if (light_state->brightness > 100) {
							light_state->brightness = 0;
						}
					} else {
						light_state->brightness += 15;
						if (light_state->brightness > 75) {
							light_state->brightness = 75;
						}
					}
				}
				break;
		}
	}
}