#pragma once

#include <inttypes.h>

#define DIMENSIONS 2
#define VERLETS 1024
#define VERLET_ALIGN __attribute__ ((aligned (4)))

typedef float verlet_t;

typedef struct {
  verlet_t _type[VERLETS]                  VERLET_ALIGN;
  uint32_t _morton[VERLETS]                VERLET_ALIGN;
  verlet_t _one_over_mass[VERLETS]         VERLET_ALIGN;
  verlet_t _forces[DIMENSIONS][VERLETS]    VERLET_ALIGN;
  verlet_t _direction[DIMENSIONS][VERLETS] VERLET_ALIGN;
  verlet_t _pos_now[DIMENSIONS][VERLETS]   VERLET_ALIGN;
  verlet_t _pos_then[DIMENSIONS][VERLETS]  VERLET_ALIGN;
} verlet_pool_t;

void init_verlet_pool();
void integrate (float dt_over_dt, float dt_squared);
