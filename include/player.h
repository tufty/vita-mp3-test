#pragma once

#include <stdint.h>

#include <psp2/ctrl.h>

#include "verlet.h"

extern SceCtrlData controller;
extern uint32_t pressed_buttons;
extern uint32_t released_buttons;
extern uint64_t controller_ts;

void init_player(verlet_pool_t * pool, uint16_t player, float x, float y);
void step_player(verlet_pool_t * pool, uint16_t player, float dt_over_dt, float dt_squared);
void collide_player (verlet_pool_t * pool, uint16_t player, uint16_t morton);
void die_player (verlet_pool_t * pool, uint16_t player);
