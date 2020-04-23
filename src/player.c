#include "player.h"
#include "objects.h"
#include "verlet.h"

#include <psp2/ctrl.h>


SceCtrlData controller;

/* Initialise the player object */
void init_player(verlet_pool_t * pool, uint16_t object, float x, float y) {
  object_init_generic(PLAYER, pool, object, x, y);
  pool->_one_over_mass[object] = 1;
}

/* For each step of the player object */
void step_player(verlet_pool_t * pool, uint16_t object, float dt_over_dt, float dt_squared) {

  /* get controller data */
  sceCtrlReadBufferPositive(0, &controller, 1);

  /* Fire button */
  if (controller.buttons & SCE_CTRL_CROSS) {
  }

  /* Smart bomb */
  if (controller.buttons & SCE_CTRL_RTRIGGER) {
  }

  pool->_forces[0][object] += (float)(128 - (int16_t)controller.lx) / 1280.0;
  pool->_forces[1][object] += (float)(128 - (int16_t)controller.ly) / 1280.0;
  
}

/* Check we've not been killed or picked up a powerup */
void collide_player (verlet_pool_t * pool, uint16_t player, uint16_t morton) {
}


/* deal with player death */
void die_player (verlet_pool_t * pool, uint16_t player) {
}
