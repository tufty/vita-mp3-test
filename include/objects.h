#pragma once

#include <stdint.h>
#include <vita2d.h>

#include "verlet.h"

typedef void (*init_fn_t)    (verlet_pool_t * pool, uint16_t object, float x, float y);
typedef void (*step_fn_t)    (verlet_pool_t * pool, uint16_t object, float dt_over_dt, float dt_squared);
typedef void (*draw_fn_t)    (verlet_pool_t * pool, uint16_t object);
typedef void (*collide_fn_t) (verlet_pool_t * pool, uint16_t object, uint16_t morton);
typedef void (*die_fn_t)     (verlet_pool_t * pool, uint16_t object);

typedef struct {
  init_fn_t        _init;
  step_fn_t        _step;
  draw_fn_t        _draw;
  collide_fn_t     _collide;
  die_fn_t         _die;
  vita2d_texture * _bitmap;
} object_t;

enum {
      PLAYER = 0,
      PLAYER_BULLET, 
      MAX_OBJECT_TYPES
};

extern object_t _object_types[];
extern uint32_t _object_state[];

/* Overall object management stuff */
void objects_init ();

/* Managing object memory */
void objects_mm_init ();
uint16_t allocate_object ();
void free_object (uint16_t object);

/* Global lifetime functions */
void step_objects(verlet_pool_t * pool, float dt_over_dt, float dt_squared);
void collide_objects(verlet_pool_t * pool);
void draw_objects(verlet_pool_t * pool);
  
/* Generic lifetime functions */
void object_init_generic(uint16_t type, verlet_pool_t * pool, uint16_t object, float x, float y);
void object_step_generic(verlet_pool_t * pool, uint16_t object, float dt_over_dt, float dt_squared);
void object_draw_generic(verlet_pool_t * pool, uint16_t object);
void object_die_generic(verlet_pool_t * pool, uint16_t object);
