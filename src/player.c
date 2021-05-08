#include "player.h"
#include "objects.h"
#include "verlet.h"
#include "player_bullet.h"

#include <psp2/ctrl.h>

SceCtrlData controller;
uint64_t controller_ts = 0;

/* Initialise the player object */
void init_player(verlet_pool_t * pool, uint16_t object, float x, float y) {
  object_init_generic(PLAYER, pool, object, x, y);
  pool->_one_over_mass[object] = 1;
}

/* For each step of the player object */
void step_player(verlet_pool_t * pool, uint16_t object, float dt_over_dt, float dt_squared) {

  float xforce = 0;
  float yforce = 0;

  if (controller_ts != controller.timeStamp) {
    /* Fire button */
    if (controller.buttons & SCE_CTRL_CROSS) {
      uint16_t bullet = allocate_object();
      init_player_bullet(pool, bullet, pool->_pos_now[0][object], pool->_pos_now[1][object]);
    }

    /* Smart bomb */
    if (controller.buttons & SCE_CTRL_RTRIGGER) {
    }

    xforce += ((controller.buttons & SCE_CTRL_LEFT) ? -0.01 : 0) +
              ((controller.buttons & SCE_CTRL_RIGHT) ? 0.01 : 0);
              yforce += ((controller.buttons & SCE_CTRL_UP) ? -0.01 : 0) +
              ((controller.buttons & SCE_CTRL_DOWN) ? 0.01 : 0);
  }

  pool->_forces[0][object] += xforce;
  pool->_forces[1][object] += yforce;
}

/* Check we've not been killed or picked up a powerup */
void collide_player (verlet_pool_t * pool, uint16_t player, uint16_t morton) {
  if (pool->_pos_now[0][player] < 0)        pool->_pos_now[0][player] = 0;
  if (pool->_pos_now[0][player] > 960)  pool->_pos_now[0][player] = 960;
  if (pool->_pos_now[1][player] < 0)        pool->_pos_now[1][player] = 0;
  if (pool->_pos_now[1][player] > 540)  pool->_pos_now[1][player] = 540;
}

/* deal with player death */
void die_player (verlet_pool_t * pool, uint16_t player) {
}
