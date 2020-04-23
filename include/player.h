#pragma once

#include <stdint.h>

#include "verlet.h"


void init_player(verlet_pool_t * pool, uint16_t player, float x, float y);
void step_player(verlet_pool_t * pool, uint16_t player, float dt_over_dt, float dt_squared);
void collide_player (verlet_pool_t * pool, uint16_t player, uint16_t morton);
void die_player (verlet_pool_t * pool, uint16_t player);
