#include <psp2/display.h>
#include <psp2/ctrl.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>

#include <stdlib.h>
#include <time.h>

#include <vita2d.h>

#include "bgm.h"
#include "verlet.h"
#include "objects.h"
#include "player.h"
#include "controls.h"

uint16_t target = 0;
vita2d_pgf * font ;

int main(int argc, char *argv[]) {

  int status;

  //  bgm_init();
  verlet_pool_init(&_pool);
  vita2d_init();
  objects_init();

  font = vita2d_load_default_pgf();

  bgm_start();

  /* boost our thread priority */
  int thread = sceKernelGetThreadId();
  sceKernelChangeThreadPriority(thread, SCE_KERNEL_HIGHEST_PRIORITY_USER);

  controls_init();

  uint64_t t0, t1 = sceKernelGetProcessTimeWide();
  uint32_t dt0, dt1 = 1;

  uint32_t frame = 0;
  uint32_t rtot = 0, avg = 0;

  uint16_t player = allocate_object();
  init_player(&_pool, player, 480.0, 270.0);

  while(1) {

    controls_sample(0, &controller, &pressed_buttons, &released_buttons);

    t0 = t1;
    dt0 = dt1;
    t1 = sceKernelGetProcessTimeWide();
    dt1 = t1 - t0;
    verlet_pool_integrate(&_pool, dt1 / dt0, (dt1 * dt1) / 1000000);

    step_objects(&_pool, dt1 / dt0, (dt1 * dt1) / 1000000);

    collide_objects(&_pool);

    t0 = sceKernelGetProcessTimeWide();
    rtot += t0 - t1;
    frame++;

    vita2d_start_drawing();
    vita2d_clear_screen();

    draw_objects(&_pool);

    if ((frame % 64) == 0) {
      avg = rtot >> 6;
      rtot = 0;
    }

    vita2d_pgf_draw_textf(font, 0, 16, 0xffffffff, 1.0, "# : %u", _object_count);
    vita2d_pgf_draw_textf(font, 0, 33, 0xffffffff, 1.0, "o : %#04x, t : %#04x, morton : %#04x", target, _pool._type[target], _pool._morton[target]);
    vita2d_pgf_draw_textf(font, 0, 50, 0xffffffff, 1.0, "[%.f, %.f] [%.f, %.f]", _pool._pos_then[0][target], _pool._pos_then[1][target], _pool._pos_now[0][target], _pool._pos_now[1][target]);
    vita2d_pgf_draw_textf(font, 0, 67, 0xffffffff, 1.0, "Frame : %u, time : %u", frame, avg);

    vita2d_end_drawing();
    vita2d_swap_buffers();

    if (pressed_buttons & SCE_CTRL_SELECT) {
      target = (target - 1) & 0x3ff;
    }

    if (pressed_buttons & SCE_CTRL_START) {
      target = (target + 1) & 0x3ff;
    }
  }

  sceKernelExitProcess(0);
  return status;
}
