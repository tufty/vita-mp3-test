#include "verlet.h"

#include <vita2d.h>

void display_objects() {
  for (int i = 0; i < VERLETS; i++) {
    vita2d_draw_pixel(_pool._pos_now[0][i] * 960,
		      _pool._pos_now[1][i] * 540,
		      0xff00ff00);
  }
}
