#pragma once

#include <inttypes.h>

#define DIMENSIONS 2
#define VERLETS 1024
#define VERLET_ALIGN __attribute__ ((aligned (16)))

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

extern verlet_pool_t _pool;

void verlet_pool_init();
void verlet_pool_integrate (float dt_over_dt, float dt_squared);

/* Sort a block of 1024 16 bit indexes into the pool by their morton order */
void sort_by_morton(uint16_t * array);
