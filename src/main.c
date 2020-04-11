#include <psp2/display.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>

#include <stdlib.h>

#include <vita2d.h>

#include "bgm.h"
#include "debugScreen.h"
#include "verlet.h"
#include "display.h"

int main(int argc, char *argv[]) {

  int status;

  // Initialise our output
  psvDebugScreenInit();
  PsvDebugScreenFont * font8x8 = psvDebugScreenGetFont();
  PsvDebugScreenFont * font16x16 = psvDebugScreenScaleFont2x(font8x8);
  psvDebugScreenSetFont(font16x16);
  
  bgm_init();
  verlet_pool_init();
  vita2d_init();

  bgm_start();

  /* Initialise random data in verlet pool */
  for (int i = 0; i < VERLETS; i++) {
    float x = (float)(rand()) / RAND_MAX;
    float y = (float)(rand()) / RAND_MAX;
    float m = (float)(rand()) / RAND_MAX;
    _pool._pos_now[0][i] = x;
    _pool._pos_then[0][i] = x;
    _pool._pos_now[1][i] = y;
    _pool._pos_then[1][i] = y;
    _pool._one_over_mass[i] = m;
  }
  
  while(1) {
    verlet_pool_integrate(1, 1);

    vita2d_start_drawing();
    vita2d_clear_screen();	

    display_objects();

    vita2d_end_drawing();
    vita2d_swap_buffers();
  }
  
  sceKernelExitProcess(0);
  return status;
}
