#include "verlet.h"

#include <vita2d.h>

void display_objects(uint16_t target) {
  uint32_t morton = _pool._morton[target] >> 10;
  
  for (int i = 0; i < VERLETS; i++) {

    uint32_t color = (i == target) ? 0xffff0000 :
      (_pool._morton[i] >> 10 == morton) ? 0xffffff00 :
      0xffffffff;
    
    vita2d_draw_rectangle(_pool._pos_now[0][i] * 960.0f / 1024.0f,
			  _pool._pos_now[1][i] * 540.0f / 1024.0f,
			  4, 4,
			  color);
  }
}
