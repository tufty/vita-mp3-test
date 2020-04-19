#include <psp2/display.h>
#include <psp2/ctrl.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>

#include <stdlib.h>
#include <time.h>

#include <vita2d.h>

#include "bgm.h"
#include "verlet.h"
#include "display.h"
#include "collider.h"

uint16_t target = 0;
vita2d_pgf * font ;

int main(int argc, char *argv[]) {

  int status;
  
  //  bgm_init();
  verlet_pool_init();
  vita2d_init();

  font = vita2d_load_default_pgf();
 
  bgm_start();

  /* Initialise random data in verlet pool */
  srand(time(NULL));
  for (int i = 0; i < VERLETS; i++) {
    float x = 1024 * (float)(rand()) / (float)RAND_MAX;
    float y = 1024 * (float)(rand()) / (float)RAND_MAX;
    float m = (float)(rand()) / (float)RAND_MAX;
    _pool._type[i] = 1;
    _pool._pos_now[0][i] = x;
    _pool._pos_then[0][i] = x;
    _pool._pos_now[1][i] = y;
    _pool._pos_then[1][i] = y;
    _pool._one_over_mass[i] = m;
    _pool._forces[0][i] = 0;
    _pool._forces[1][i] = 0; //.01;
    _pool._collide[i] = 0;
  }

  /* boost our thread priority */
  int thread = sceKernelGetThreadId();
  sceKernelChangeThreadPriority(thread, SCE_KERNEL_HIGHEST_PRIORITY_USER);

  /* Set sampling mode */
  sceCtrlSetSamplingMode(SCE_CTRL_MODE_DIGITAL);
  SceCtrlData ct;

  uint64_t t0, t1 = sceKernelGetProcessTimeWide();
  uint32_t dt0, dt1 = 1;

  uint32_t frame = 0;
  uint32_t rtot = 0, avg = 0;

  while(1) {    

    t0 = t1;
    dt0 = dt1;
    t1 = sceKernelGetProcessTimeWide();
    dt1 = t1 - t0;
    verlet_pool_integrate(&_pool, dt1 / dt0, (dt1 * dt1) / 1000000);
    t0 = sceKernelGetProcessTimeWide();
    rtot += t0 - t1;
    frame++;
      
    vita2d_start_drawing();
    vita2d_clear_screen();

    collide_objects();

    display_objects();

    if ((frame % 256) == 0) {
      avg = rtot >> 8;
      rtot = 0;
    }

    vita2d_pgf_draw_textf(font, 0, 16, 0xffffffff, 1.0, "object : %#04x, morton : %#04x", target, _pool._morton[target]);
    
    
    vita2d_end_drawing();
    vita2d_swap_buffers();
    sceCtrlReadBufferPositive(0, &ct, 1);

    if (ct.buttons & SCE_CTRL_UP) {
      target = (target + 1) % 1024;
    }

    if (ct.buttons & SCE_CTRL_DOWN) {
      target = (target - 1) % 1024;
    }

  }
  
  sceKernelExitProcess(0);
  return status;
}
