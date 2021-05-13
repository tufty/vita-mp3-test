#include "objects.h"

#include "verlet.h"
#include "player.h"
#include "player_bullet.h"

object_t _object_types[MAX_OBJECT_TYPES];
uint32_t _object_state_bitmap[VERLETS / 4];
uint32_t _object_state[VERLETS];

uint32_t _object_count;

/* Initialise object types */
void objects_init () {
  _object_types[PLAYER]._init = init_player;
  _object_types[PLAYER]._step = step_player;
  _object_types[PLAYER]._draw = object_draw_generic;
  _object_types[PLAYER]._collide = collide_player;
  _object_types[PLAYER]._die = die_player;
  _object_types[PLAYER]._bitmap = vita2d_load_PNG_file("app0:/assets/player.png");

  _object_types[PLAYER_BULLET]._init = init_player_bullet;
  _object_types[PLAYER_BULLET]._step = step_player_bullet;
  _object_types[PLAYER_BULLET]._draw = object_draw_generic;
  _object_types[PLAYER_BULLET]._collide = collide_player_bullet;
  _object_types[PLAYER_BULLET]._die = object_die_generic;
  _object_types[PLAYER_BULLET]._bitmap = vita2d_load_PNG_file("app0:/assets/player_bullet.png");

  objects_mm_init();

  _object_count = 0;
}

/* Initialise the memory management bitmap */
void objects_mm_init () {
  for (int i = 0; i < (VERLETS / 4); i++) {
    _object_state_bitmap[i] = 0;
  }
}

/* Pull an object from the pool */
uint16_t allocate_object () {
  for (int i = 0; i < (VERLETS / 4); i++) {
    int first_clear_bit = __builtin_ffs(~_object_state_bitmap[i]);

    if (first_clear_bit) {
      _object_state_bitmap[i] |= (1 << (first_clear_bit - 1));
      _object_count ++;
      return (i * 4) + (first_clear_bit - 1);
    }

  }
  return 0xffff;
}

void free_object (uint16_t object) {
  int bit = object & 0x1f;
  int word = object >> 5;

  _object_count--;

  _object_state_bitmap[word] &= ~(1 << bit);
}


void step_objects(verlet_pool_t * pool, float dt_over_dt, float dt_squared) {
  for (int i = 0; i < VERLETS; i++) {
    uint16_t o = pool->_object_index[i];
    uint16_t t = pool->_type[o];
    if (pool->_morton[o] == 0xf000) {
      return;
    }
    _object_types[t]._step(pool, o, dt_over_dt, dt_squared);
  }

}

void collide_objects(verlet_pool_t * pool) {
  for (int i = 0; i < VERLETS; i++) {
    /* iterate through the objects in morton order */
    uint16_t o = pool->_object_index[i];
    uint16_t t = pool->_type[o];

    /* We are done if the object's morton code has its high bit set  */
    if (pool->_morton[o] & 0xf000) {
      return;
    }

    /* Work out which morton codes we must check */
    uint16_t morton;
    uint16_t morton_components[4] =
      {
       pool->_m01[o][0] << 1,
       pool->_m23[o][0] << 1,
       pool->_m01[o][1],
       pool->_m23[o][1]
      };

    // And do our collision detection for the 4 possible neighbourhoods
    morton = morton_components[0] | morton_components[2];
    if (morton < 0x0400) {
      _object_types[t]._collide(pool, o, morton);
    }
    morton = morton_components[0] | morton_components[3];
    if (morton < 0x0400) {
      _object_types[t]._collide(pool, o, morton);
    }
    morton = morton_components[1] | morton_components[2];
    if (morton < 0x0400) {
      _object_types[t]._collide(pool, o, morton);
    }
    morton = morton_components[1] | morton_components[3];
    if (morton < 0x0400) {
      _object_types[t]._collide(pool, o, morton);
    }
  }
}

void draw_objects(verlet_pool_t * pool) {
  SceGxmContext * context = vita2d_get_context();

  sceGxmSetRegionClip(context, SCE_GXM_REGION_CLIP_OUTSIDE, 0, 0, 960, 540);

  // sceGxmSetRegionClip(context, SCE_GXM_REGION_CLIP_OUTSIDE, 32, 32, 960 + 32, 540 + 32);
  // sceGxmSetViewport(context, 32, -1, 32, -1, 0.5, 0.5);

    /* Draw a grid, test usage only */
    uint32_t grid_color = 0x808080ff;

    for (int i = 0; i < 1024; i += 32) {
      vita2d_draw_line(i, 0, i, 1024, grid_color);
      vita2d_draw_line(0, i, 1024, i, grid_color);
    }


  for (int i = 0; i < VERLETS; i++) {
    /* iterate through the objects in morton order */
    uint16_t o = pool->_object_index[i];
    uint16_t t = pool->_type[o];

    /* We are done if the object's morton code has its high bit set  */
    if (pool->_morton[o] == 0xf000) {
      return;
    }
    _object_types[t]._draw(pool, o);
  }

}

void object_init_generic(uint16_t type, verlet_pool_t * pool, uint16_t object, float x, float y) {
  pool->_type[object] = type;
  pool->_pos_now[0][object] = x;
  pool->_pos_now[1][object] = y;
  pool->_pos_now[0][object] = x;
  pool->_pos_now[1][object] = y;
  pool->_direction[0][object] = 0;
  pool->_direction[1][object] = 1;
  pool->_forces[0][object] = 0;
  pool->_forces[1][object] = 0;
  pool->_morton[object] = 0x0000;
}

/* Generic object step function, do nothing */
void object_step_generic(verlet_pool_t * pool, uint16_t object, float dt_over_dt, float dt_squared) {
}

/* Generic object drawing, probably need to make this centre the bitmap */
void object_draw_generic(verlet_pool_t * pool, uint16_t object) {
  uint16_t type = pool->_type[object];
  vita2d_texture * t = _object_types[type]._bitmap;

  vita2d_draw_texture(t, pool->_pos_now[0][object] - (vita2d_texture_get_width(t) / 2), pool->_pos_now[1][object] - (vita2d_texture_get_height(t) / 2));
}

void object_die_generic(verlet_pool_t * pool, uint16_t object) {
  pool->_type[object] = 0xffff;
  pool->_morton[object] = 0xf000;
  pool->_pos_now[0][object] = 0;
  pool->_pos_now[1][object] = 0;
  pool->_pos_then[0][object] = 0;
  pool->_pos_then[1][object] = 0;
  free_object(object);
}
