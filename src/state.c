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

#define C_CAMERA_OFFSET 7.f

#define CAMERA_BOUNDS_X 4.f
#define CAMERA_BOUNDS_MAX_Y 6.f
#define CAMERA_BOUNDS_MIN_Y 4.f

#define LIGHT_HMOVE_VELOCITY 0.2f
#define LIGHT_HMOVE_ACCEL 0.02f
#define LIGHT_HMOVE_MARGIN 4.f

game_state_t game_state;

void load_level(uint16_t level_index) {
	game_state.level_index = level_index;
	game_state.level = levels[level_index];
	
	if (game_state.level == NULL) {
		game_state.status = GAME_STATUS_BEAT;
		return;
	}

	game_state.spooker_count = 1;
	game_state.spookers[0].transform.position.x = 0.f;
	game_state.spookers[0].transform.position.y = 0.f;
	game_state.spookers[0].transform.position.z = 0.f;
	game_state.spookers[0].transform.rotation_z = 0.f;
	game_state.spookers[0].velocity.x = 0.f;
	game_state.spookers[0].velocity.y = 0.f;
	game_state.spookers[0].knockback_timer = 0;

	game_state.snooper_count = 0;

	game_state.camera_position.x = 0.f;
	game_state.camera_position.y = CAMERA_OFFSET_Y;
	game_state.camera_position.z = CAMERA_OFFSET_Z;

	game_state.snooper_timer = 1;

	game_state.score = 0;
	game_state.snooper_death_count = 0;

	game_state.status = GAME_STATUS_START;
	game_state.game_status_timer = 0;

	path_set_graph(game_state.level->path_graph);

	for (int i = 0; i < game_state.level->light_count; i++) {
		game_state.light_states[i].position = game_state.level->lights[i].position;

		switch (game_state.level->lights[i].type) {
			case LIGHT_TYPE_STATIC:
				game_state.light_states[i].brightness = 90;
				game_state.light_states[i].is_on = 1;
				break;
			case LIGHT_TYPE_BLINK:
				game_state.light_states[i].brightness = 0;
				game_state.light_states[i].is_on = 0;
				game_state.light_states[i].type_state.blink.timer = 240 + RANDN(240);
				break;
			case LIGHT_TYPE_HMOVE:
				game_state.light_states[i].brightness = 90;
				game_state.light_states[i].is_on = 1;
				game_state.light_states[i].type_state.hmove.velocity_x = 0.f;
		}
	}

	sfx_level_start();
}

void state_init() {
	load_level(0);
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
		new_snooper->animation_progress = 0.f;
		new_snooper->spooked_timer = 0;

		int min_duration = game_state.level->min_snooper_spawn_duration;
		int max_duration = game_state.level->max_snooper_spawn_duration;
		game_state.snooper_timer = min_duration + RANDN(max_duration - min_duration);
	}
}

static inline float world2grid_x(float x) {
	return 0.5f*(x + game_state.level->width);
}
static inline float world2grid_y(float y) {
	return 0.5f*(-y + game_state.level->height);
}

bool is_wall(float x, float y) {
	int grid_x = (int)world2grid_x(x);
	int grid_y = (int)world2grid_y(y);

	if (grid_x < 0 || grid_x >= game_state.level->width) return true;
	if (grid_y < 0 || grid_y >= game_state.level->height) return true;

	return game_state.level->data[game_state.level->width*grid_y + grid_x] != 1;
}

static size_t get_light_at(float x, float y, vector2_t *out) {
	// If we're inside a wall, there's no light.
	if (is_wall(x, y)) return MAX_SNOOPER_COUNT+1;

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
		const level_light_state_t *light_state = &game_state.light_states[i];
		if (!light_state->is_on) continue;

		const level_light_t *light = &game_state.level->lights[i];

		float dx = x - light_state->position.x;
		float dy = y - light_state->position.y;

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
	if (game_state.status == GAME_STATUS_BEAT) {
		return;
	}

	controller_scan();
	struct controller_data ckeys = get_keys_held();

	// Update status timer.
	if (game_state.status == GAME_STATUS_LOSE || game_state.status == GAME_STATUS_WIN) {
		if (ckeys.c[0].start || game_state.game_status_timer > 0) {
			game_state.game_status_timer++;
			if (game_state.game_status_timer >= GAME_END_DURATION) {
				uint16_t level_index = game_state.level_index;
				if (game_state.status == GAME_STATUS_WIN) level_index++;
				load_level(level_index);
				return;
			}
		}
	} else {		
		if (game_state.game_status_timer < 0xffff) {
			game_state.game_status_timer++;
		}
	}

	if (game_state.status == GAME_STATUS_START && game_state.game_status_timer >= GAME_START_DURATION) {
		game_state.status = GAME_STATUS_PLAYING;
		game_state.game_status_timer = 0;
		sfx_start_music();
		return;
	}

	if (game_state.status != GAME_STATUS_PLAYING) {
		return;
	}

	if (game_state.score >= game_state.level->score_target) {
		game_state.status = GAME_STATUS_WIN;
		game_state.game_status_timer = 0;
		sfx_win();
		return;
	}
	if (game_state.snooper_death_count >= game_state.level->snooper_death_cap) {
		game_state.status = GAME_STATUS_LOSE;
		game_state.game_status_timer = 0;
		sfx_lose();
		return;
	}

	// Spawn snooper?
	if (game_state.snooper_timer > 0) game_state.snooper_timer--;
	if (game_state.snooper_count == 0 || game_state.snooper_timer == 0) {
		spawn_snooper();
	}

	// Move snoopers
	for (uint16_t i = 0; i < game_state.snooper_count; i++) {
		snooper_state_t *snooper = game_state.snoopers + i;

		if (snooper->status == SNOOPER_STATUS_DYING) {
			if (++snooper->freeze_timer == SNOOPER_DIE_DURATION) {
				snooper->status = SNOOPER_STATUS_DEAD;
				sfx_bad();
				game_state.snooper_death_count++;
			}
			snooper->position.y -= SNOOPER_SPEED;

			continue;
		}

		update_light_brightness(&snooper->light_brightness);

		float speed = snooper->status == SNOOPER_STATUS_ALIVE ? SNOOPER_SPEED : -SNOOPER_RUN_SPEED;
		float animation_speed = snooper->status == SNOOPER_STATUS_ALIVE ? 0.05f : 0.15f;
		if (snooper->freeze_timer != 0) {
			// TODO : animation?
			speed *= 0.5f;
			animation_speed *= 0.5f;
		}

		snooper->animation_progress += animation_speed;
		if (snooper->animation_progress >= 1.f) {
			snooper->animation_progress = 0.f;
		}

		bool end = path_follow(&snooper->path_follower, speed);

		if (end) {
			if (snooper->status == SNOOPER_STATUS_ALIVE) {
				sfx_snooper_die();
				snooper->status = SNOOPER_STATUS_DYING;
				snooper->freeze_timer = 0;
			} else if (snooper->status == SNOOPER_STATUS_SPOOKED) {
				sfx_point();
				game_state.score++;
				snooper->status = SNOOPER_STATUS_DEAD;
			}
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
			if (snooper->status == SNOOPER_STATUS_SPOOKED) {
				snooper->spooked_timer++;
			}
			if (snooper->status == SNOOPER_STATUS_ALIVE) {
				if (--snooper->rotate_timer == 0) {
					snooper->rotate_timer = 20 + RANDN(60);
					float target_rotation_z = snooper->feet_rotation_z + (M_PI/3.0f)*(randf()-0.5f);
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
				sfx_spooker_oof();
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
				dx = SPOOKER_SPEED * ckeys.c[0].x / 64.f;
				dy = SPOOKER_SPEED * ckeys.c[0].y / 64.f;

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

		float spook_progress = spooker->spook_timer / 15.f;
		float spook_height = 0.8f * spook_progress * spook_progress;
		spooker->transform.position.z = 0.5f * spooker->transform.position.z + 0.5f * spook_height;
	}

	if (ckeys.c[0].Z && game_state.spookers[0].spook_timer == 0 && game_state.spookers[0].knockback_timer == 0) {
		if (is_wall(game_state.spookers[0].transform.position.x, game_state.spookers[0].transform.position.y)) {
			sfx_spooker_spook_muffled();
		} else {
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
					game_state.snoopers[i].spooked_timer = 0;
				}
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
					if (light_state->is_on) {
						light_state->type_state.blink.timer = 90 + RANDN(120);
					} else {
						light_state->type_state.blink.timer = 360 + RANDN(360);
					}
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
			case LIGHT_TYPE_HMOVE:
				update_light_brightness(&light_state->brightness);
				light_state->position.x += light_state->type_state.hmove.velocity_x;

				bool right = light_state->type_state.hmove.velocity_x >= 0.f;
				float max_x = game_state.level->width - LIGHT_HMOVE_MARGIN;
				if (light_state->position.x >= max_x) right = false;
				if (light_state->position.x <= -max_x) right = true;

				if (right) {
					light_state->type_state.hmove.velocity_x += LIGHT_HMOVE_ACCEL;
					if (light_state->type_state.hmove.velocity_x > LIGHT_HMOVE_VELOCITY) {
						light_state->type_state.hmove.velocity_x = LIGHT_HMOVE_VELOCITY;
					}
				} else {
					light_state->type_state.hmove.velocity_x -= LIGHT_HMOVE_ACCEL;
					if (light_state->type_state.hmove.velocity_x < -LIGHT_HMOVE_VELOCITY) {
						light_state->type_state.hmove.velocity_x = -LIGHT_HMOVE_VELOCITY;
					}
				}

				break;
		}
	}
}