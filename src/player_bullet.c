#include "player_bullet.h"

void init_player_bullet(verlet_pool_t * pool, uint16_t object, float x, float y) {
  object_init_generic(PLAYER_BULLET, pool, object, x, y);
  pool->_pos_then[1][object] += PLAYER_BULLET_V_SPEED;
  pool->_one_over_mass[object] = 0;
}

void step_player_bullet(verlet_pool_t * pool, uint16_t object, float dt_over_dt, float dt_squared) {
  if (pool->_pos_now[1][object] < 32) {
    die_player_bullet(pool, object);
  }
}

void collide_player_bullet (verlet_pool_t * pool, uint16_t object, uint16_t morton) {
}

void die_player_bullet (verlet_pool_t * pool, uint16_t object) {
  object_die_generic(pool, object);
}
