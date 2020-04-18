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

void collide_morton (uint16_t object, uint16_t morton) {
  uint16_t i = _morton_index[morton];

  while ((i < VERLETS) && (_pool._morton[_object_index[i]] == morton)) {
    if (_object_index[i] != object) {
      _pool._collide[_object_index[i]] = -1;
    }
    i++;
  }
}

int main(int argc, char *argv[]) {

  int status;
  
  //  bgm_init();
  verlet_pool_init();
  vita2d_init();

  vita2d_pgf * font = vita2d_load_default_pgf();
 
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
    _pool._forces[1][i] = 0.001; //.01;
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
  uint16_t target = 0;

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

    /* We can now do broad phase object collision testing */
    /* iterate through the objects using the morton index. */
    uint16_t m = _pool._morton[target];
      
      /* if (m & 0xf000) { /\* Exit if high bit of morton code is set *\/ */
      /* 	break; */
      /* } */
      
      /* /\* Generate list of morton codes to scan for neighbours. *\/ */
      /* int x_dir = ((uint16_t)(_pool._pos_now[0][o]) % 32) > 16 ? 1 : -1; */
      /* int y_dir = ((uint16_t)(_pool._pos_now[1][o]) % 32) > 16 ? 1 : -1; */
      
    collide_morton(target, m);	

    
    vita2d_start_drawing();
    vita2d_clear_screen();	

    display_objects(target);

    if ((frame % 256) == 0) {
      avg = rtot >> 8;
      rtot = 0;
    }

    //    vita2d_pgf_draw_textf(font, 0, 32, 0xffffffff, 1.0, "%u us", avg);
    // vita2d_pgf_draw_textf(font, 0, 49, 0xffffffff, 1.0, "%#04x", _pool._morton[target]);


    uint16_t x = target;
    vita2d_pgf_draw_textf(font, 0, 16, 0xffffffff, 1.0, "object : %#04x, morton : %#04x", x, _pool._morton[x]);
    uint16_t y = _object_index[_morton_index[_pool._morton[x]]];
    vita2d_pgf_draw_textf(font, 0, 33, 0xffffffff, 1.0, "object : %#04x, morton : %#04x", y, _pool._morton[y]);
    y =  _object_index[_morton_index[_pool._morton[x]] + 1];
    vita2d_pgf_draw_textf(font, 0, 50, 0xffffffff, 1.0, "object : %#04x, morton : %#04x", y, _pool._morton[y]);
    
    
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
