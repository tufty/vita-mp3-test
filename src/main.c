#include <psp2/display.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>

#include <stdlib.h>
#include <time.h>

#include <vita2d.h>

#include "bgm.h"
#include "verlet.h"
#include "display.h"

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
    float x = (float)(rand()) / (float)RAND_MAX;
    float y = (float)(rand()) / (float)RAND_MAX;
    float m = (float)(rand()) / (float)RAND_MAX;
    _pool._pos_now[0][i] = x;
    _pool._pos_then[0][i] = x;
    _pool._pos_now[1][i] = y;
    _pool._pos_then[1][i] = y;
    _pool._one_over_mass[i] = m;
    _pool._forces[0][i] = 0;
    _pool._forces[1][i] = 0;
  }

  /* boost our thread priority */
  int thread = sceKernelGetThreadId();
  sceKernelChangeThreadPriority(thread, SCE_KERNEL_HIGHEST_PRIORITY_USER);
  
  while(1) {
    int y = 17;
    
    vita2d_start_drawing();
    vita2d_clear_screen();	

    display_objects();
    vita2d_pgf_draw_textf(font, 0, (y++ * 17), 0xffffff00, 1, "F    : <%f, %f>\n", _pool._forces[0][1], _pool._forces[1][1]);
    vita2d_pgf_draw_textf(font, 0, (y++ * 17), 0xffffff00, 1, "Mass : %f\n", _pool._one_over_mass[1]);

    y++;
    
    vita2d_pgf_draw_textf(font, 0, (y++ * 17), 0xffffff00, 1, "Then : <%f, %f>\n", _pool._pos_then[0][1], _pool._pos_then[1][1]);
    vita2d_pgf_draw_textf(font, 0, (y++ * 17), 0xffffff00, 1, "Now  : <%f, %f>\n", _pool._pos_now[0][1], _pool._pos_now[1][1]);
    vita2d_pgf_draw_textf(font, 0, (y++ * 17), 0xffffff00, 1, "Dir  : <%f, %f>\n", _pool._direction[0][1], _pool._direction[1][1]);

    y++;
    
    verlet_pool_integrate(1, 1);
    
    vita2d_pgf_draw_textf(font, 0, (y++ * 17), 0xffffff00, 1, "Then : <%f, %f>\n", _pool._pos_then[0][1], _pool._pos_then[1][1]);
    vita2d_pgf_draw_textf(font, 0, (y++ * 17), 0xffffff00, 1, "Now  : <%f, %f>\n", _pool._pos_now[0][1], _pool._pos_now[1][1]);
    vita2d_pgf_draw_textf(font, 0, (y++ * 17), 0xffffff00, 1, "Dir  : <%f, %f>\n", _pool._direction[0][1], _pool._direction[1][1]);
    
    vita2d_end_drawing();
    vita2d_swap_buffers();

  }
  
  sceKernelExitProcess(0);
  return status;
}
