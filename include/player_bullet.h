#pragma once

#include <stdint.h>

#include "verlet.h"
#include "objects.h"

#define PLAYER_BULLET_V_SPEED 4

void init_player_bullet(verlet_pool_t * pool, uint16_t object, float x, float y);
void step_player_bullet(verlet_pool_t * pool, uint16_t object, float dt_over_dt, float dt_squared);
void collide_player_bullet (verlet_pool_t * pool, uint16_t object, uint16_t morton);
void die_player_bullet (verlet_pool_t * pool, uint16_t object);
