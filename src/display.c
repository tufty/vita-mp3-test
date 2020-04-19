#include "display.h"
#include "verlet.h"

#include <vita2d.h>

extern uint16_t target;

void display_objects() {
  float xmin = 0, xmax = 960, ymin = 0, ymax = 540;
  uint32_t grid_color = 0x808080ff;
  
  for (int i = 0; i < 1024; i += 32) {
    float x = i * xmax / 1024, y = i * ymax / 1024; 
    vita2d_draw_line(x, ymin, x, ymax, grid_color);
    vita2d_draw_line(xmin, y, xmax, y, grid_color);
  }



  for (int i = 0; i < VERLETS; i++) {

    uint32_t color = (i == target) ? 0xffffffff :
      (_pool._collide[i] == 0) ? 0x80808080 :
      0xffff00ff;
    
    vita2d_draw_rectangle(_pool._pos_now[0][i] * 960.0f / 1024.0f,
			  _pool._pos_now[1][i] * 540.0f / 1024.0f,
			  4, 4,
			  color);
  }
}
