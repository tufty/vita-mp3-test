#pragma once

#include <inttypes.h>

#define DIMENSIONS 2
#define VERLETS 1024
#define VERLET_ALIGN __attribute__ ((aligned (16)))

typedef float verlet_t;

typedef struct {
  /* Index of object numbers by morton code */
  uint16_t _object_index[VERLETS]          VERLET_ALIGN;
  /* Index into _object_index for a given morton code */
  uint16_t _morton_index[VERLETS]          VERLET_ALIGN;
  
  uint16_t _type[VERLETS]                  VERLET_ALIGN;
  uint16_t _m01[VERLETS][2]                VERLET_ALIGN;  
  uint16_t _m23[VERLETS][2]                VERLET_ALIGN;
  uint16_t _morton[VERLETS]                VERLET_ALIGN;
  verlet_t _one_over_mass[VERLETS]         VERLET_ALIGN;
  verlet_t _forces[DIMENSIONS][VERLETS]    VERLET_ALIGN;
  verlet_t _direction[DIMENSIONS][VERLETS] VERLET_ALIGN;
  verlet_t _pos_now[DIMENSIONS][VERLETS]   VERLET_ALIGN;
  verlet_t _pos_then[DIMENSIONS][VERLETS]  VERLET_ALIGN;
} verlet_pool_t;

extern verlet_pool_t _pool;


void verlet_pool_init(verlet_pool_t * pool);
void verlet_pool_integrate (verlet_pool_t * pool, float dt_over_dt, float dt_squared);
