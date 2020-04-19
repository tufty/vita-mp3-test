#include "collider.h"
#include "verlet.h"

#include <vita2d.h>

extern uint16_t target;
extern vita2d_pgf * font;

// deltas for neighbourhood morton code
// additive    : 0 <= x <= 30 --> morton += delta[x]
// additive    : x = 31       --> invalid
// subtractive : x = 0        --> invalid
// subtractive : 1 <= x <= 31 --> morton -= delta[x - 1]
// additive    : 0 <= y <= 30 --> morton += delta[y] << 1
// additive    : y = 31       --> invalid
// subtractive : y = 0        --> invalid
// subtractive : 1 <= y <= 31 --> morton -= delta[y - 1] << 1
static const uint16_t morton_delta[] = {1, 3, 1, 11, 1, 3, 1, 43, 1, 3, 1, 11, 1, 3, 1, 85, 1, 3, 1, 11, 1, 3, 1, 43, 1, 3, 1, 11, 1, 3, 1};

void collide_objects () {
  for (int i = 0; i < VERLETS; i++) {
    /* iterate through the objects in morton order */
    uint16_t o = _object_index[i];

    /* We are done if the object's morton code has its high bit set  */
    if (_pool._morton[o] & 0xf000) {
      break;
    }

    /* Work out which morton codes we must check */
    uint16_t morton_components[4], morton;

    uint16_t m_x = _pool._morton[o] & 0x5555;
    uint16_t m_y = _pool._morton[o] & 0xaaaa;
    uint16_t m_xi = (uint16_t)(_pool._pos_now[1][o] / 32);
    uint16_t m_yi = (uint16_t)(_pool._pos_now[0][o] / 32);

    // Calculate X axis morton elements for this object
    if (((uint16_t)(_pool._pos_now[0][o]) % 32) > 15) {
      /* we want x, x+1 */
      morton_components[0] = m_x;
      if (m_xi == 0x1f) {
	morton_components[1] = 0xffff;
      } else {
	morton_components[1] = m_x + (morton_delta[m_xi] * 1);
      }
    } else {
      /* we want x-1, x */
      morton_components[1] = m_x;
      if (m_xi == 0x00) {
	morton_components[0] = 0xffff;
      } else {
	morton_components[0] = m_x - (morton_delta[m_xi - 1] * 1);
      }
    }

    // Calculate Y axis morton elements for this object
    if (((uint16_t)(_pool._pos_now[1][o]) % 32) > 15) {
      /* we want y, y+1 */
      morton_components[2] = m_y;
      if (m_yi == 0x1f) {
	morton_components[3] = 0xffff;
      } else {
	morton_components[3] = m_y + (morton_delta[m_yi] * 2);
      }
    } else {
      /* we want y-1, y */
      morton_components[3] = m_y;
      if (m_yi == 0x00) {
	morton_components[2] = 0xffff;
      } else {
	morton_components[2] = m_y - (morton_delta[m_yi - 1] * 2);
      }
    }

    if (o == target) {
      vita2d_pgf_draw_textf(font, 0, 33, 0xffffffff, 1.0, "%#04x %#04x %#04x %#04x", morton_components[0] | morton_components[2], morton_components[0] | morton_components[3], morton_components[1] | morton_components[2], morton_components[1] | morton_components[3]);
      vita2d_pgf_draw_textf(font, 0, 50, 0xffffffff, 1.0, "%#04x %#04x %#04x %#04x", morton_components[0], morton_components[1], morton_components[2], morton_components[3]);
      vita2d_pgf_draw_textf(font, 0, 67, 0xffffffff, 1.0, "%#04x %#04x %#04x %#04x", m_x, m_y, m_xi, m_yi);
    }



    // And do our collision detection for the 4 possible neighbourhoods
    morton = morton_components[0] | morton_components[2];
    if (morton < 0x0400) {
      collide_morton(o, morton);
    }
    morton = morton_components[0] | morton_components[3];
    if (morton < 0x0400) {
      collide_morton(o, morton);
    }
    morton = morton_components[1] | morton_components[2];
    if (morton < 0x0400) {
      collide_morton(o, morton);
    }
    morton = morton_components[1] | morton_components[3];
    if (morton < 0x0400) {
      collide_morton(o, morton);
    }
  }
}

void collide_morton (uint16_t object, uint16_t morton) {

  if (object != target) return;
  
  for (uint16_t i = _morton_index[morton]; i < VERLETS; i++) {
    uint16_t o = _object_index[i];

    if (_pool._morton[o] != morton) break;
    
    if (o != object) {
      _pool._collide[o] = -1;
    }
  }
}
