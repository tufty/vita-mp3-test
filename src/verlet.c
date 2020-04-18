#include "verlet.h"

#include <math.h>
#include <stdlib.h>
#include <arm_neon.h>

verlet_pool_t _pool;
uint16_t _object_index[VERLETS];
uint16_t _morton_index[VERLETS];

void verlet_pool_init() {
  for (int i = 0; i < VERLETS; i++) {
    for (int d = 0; d < DIMENSIONS; d++) {
      _pool._pos_then[d][i] = 0;
      _pool._pos_now[d][i] = 0;
      _pool._forces[d][i] = 0;
      _pool._direction[d][i] = 0;
    }
    _pool._type[i] = 0;
    _pool._one_over_mass[i] = 0;
    _pool._morton[i] = 0;
    _pool._collide[i] = 0;
  }
}

static const uint8_t morton5[] = { 0x00, 0x01, 0x04, 0x05, 0x10, 0x11, 0x14, 0x15,
				   0x40, 0x41, 0x44, 0x45, 0x50, 0x51, 0x54, 0x55 };

static const uint16_t x21212121[] = { 2, 1, 2, 1, 2, 1, 2, 1 }; 

int morton_comp(const void * e1, const void* e2);

void verlet_pool_integrate (verlet_pool_t * pool, float dt_over_dt, float dt_squared) {
  /* Vector versions of the arguments */
  float32x4_t dtdt, dt2;
  
  /* Constants we'll be wanting */
  float32x4_t vzero, vone, v1023;

  union {
    uint8x8x2_t vmorton5x2;
    uint8x16_t vmorton5;
  } vmorton;
  
  uint16x8_t vbit4, v21212121, tmp16_0, tmp16_1;
  uint16x4_t tmp16_3, vzero16, vf000, morton;
  uint32x4_t tmp32_0, tmp32_1;
  uint16x8x2_t tmp16_2;
  union {
    uint8x16_t x16;
    uint8x8x2_t x8x2;
  } tmp8;

  /* Values we read and write from / to the pool */
  float32x4_t forces[2], pos_then[2], pos_now[2], direction[2];
  float32x4_t one_over_mass;
  uint16x4_t type;

  /* Local variables */
  float32x4_t acceleration[2], norm;
  uint32x4_t tmp;

  /* set up our loop invariants */
  dtdt             = vdupq_n_f32(dt_over_dt);
  dt2              = vdupq_n_f32(dt_squared);
  
  vzero            = vdupq_n_f32(0.0);
  vone             = vdupq_n_f32(1.0);
  v1023            = vdupq_n_f32(1023.0);
  vzero16          = vdup_n_u16(0);
  vf000            = vdup_n_u16(0xf000);

  vbit4            = vdupq_n_u16(0x10);
  vmorton.vmorton5 = vld1q_u8(morton5);
  v21212121        = vld1q_u16(x21212121);
 
  /* Loop through, 4-wise */
  for (int i = 0; i < (VERLETS / 4); i++) {
    /* Load values */
    pos_now[0]    = vld1q_f32(&(pool->_pos_now[0][i << 2]));
    pos_then[0]   = vld1q_f32(&(pool->_pos_then[0][i << 2]));
    forces[0]     = vld1q_f32(&(pool->_forces[0][i << 2]));
    pos_now[1]    = vld1q_f32(&(pool->_pos_now[1][i << 2]));
    pos_then[1]   = vld1q_f32(&(pool->_pos_then[1][i << 2]));
    forces[1]     = vld1q_f32(&(pool->_forces[1][i << 2]));
    
    type          = vld1_u16(&(pool->_type[i << 2]));
    
    one_over_mass = vld1q_f32(&(pool->_one_over_mass[i << 2]));

    /* Start calculating */
    /* acceleration */
    acceleration[0] = vmulq_f32(forces[0], one_over_mass);
    acceleration[1] = vmulq_f32(forces[1], one_over_mass);

    /* direction */
    direction[0] = vsubq_f32(pos_now[0], pos_then[0]);
    direction[1] = vsubq_f32(pos_now[1], pos_then[1]);

    /* size of direction */
    norm = vmulq_f32(direction[0], direction[0]);
    norm = vmlaq_f32(norm, direction[1], direction[1]);

    /* Deal with case where direction is zero */
    tmp = vceqq_f32(vzero, norm);
    norm = vbslq_f32(tmp, vone, norm);

    /* scale */
    norm = vrsqrteq_f32(norm);

    /* Store the new "then" (old "now") */
    vst1q_f32(&(pool->_pos_then[0][i << 2]), pos_now[0]);
    vst1q_f32(&(pool->_pos_then[1][i << 2]), pos_now[1]);

    /* Calculate the new now */
    pos_now[0] = vmlaq_f32(pos_now[0], direction[0], dtdt);
    pos_now[0] = vmlaq_f32(pos_now[0], acceleration[0], dt2);
    pos_now[1] = vmlaq_f32(pos_now[1], direction[1], dtdt);
    pos_now[1] = vmlaq_f32(pos_now[1], acceleration[1], dt2);

    /* clamp to 0 <= n <= 1023 */
    pos_now[0] = vminq_f32(pos_now[0], v1023);
    pos_now[0] = vmaxq_f32(pos_now[0], vzero);
    pos_now[1] = vminq_f32(pos_now[1], v1023);
    pos_now[1] = vmaxq_f32(pos_now[1], vzero);

    /* Store the new "now" */
    vst1q_f32(&(pool->_pos_now[0][i << 2]), pos_now[0]);
    vst1q_f32(&(pool->_pos_now[1][i << 2]), pos_now[1]);

    /* scale direction to unit vector */
    direction[0] = vmulq_f32(direction[0], norm);
    direction[1] = vmulq_f32(direction[1], norm);

    /* Store the new direction vector */
    vst1q_f32(&(pool->_direction[0][i << 2]), direction[0]);
    vst1q_f32(&(pool->_direction[1][i << 2]), direction[1]);


    /* calculate the morton index for the position */
    /* This involves going from 32 bit to 16, we can thus do quadword 16x8 */
    
    /* So, let's go.  Convert to integer.  Can't avoid that. */
    tmp32_0 = vcvtq_u32_f32(pos_now[0]);
    tmp32_1 = vcvtq_u32_f32(pos_now[1]);
    
    /* Reinterpret these as quadword 16 bit values, should be zero cost. */
    tmp16_0 = vreinterpretq_u16_u32(tmp32_0);
    tmp16_1 = vreinterpretq_u16_u32(tmp32_1);

    /* We can now do a vector transpose, which will interleave the values in the first val element of the result */
    tmp16_2 = vtrnq_u16(tmp16_0, tmp16_1);

    /* We can now do the math we need.  Firstly, shift the whole lot right by 5 */
    tmp16_0 = vshrq_n_u16(tmp16_2.val[0], 5);

    /* extract bit 4 and clear from source */
    tmp16_1 = vandq_u16(tmp16_0, vbit4);
    tmp16_0 = vbicq_u16(tmp16_0, vbit4);

    /* then shift bit 4 to bit 8, and reinject */
    tmp16_0 = vmlaq_n_u16(tmp16_0, tmp16_1, 16);

    /* now reinterpret as uint8x16 */
    tmp8.x16 = vreinterpretq_u8_u16(tmp16_0);

    /* Do the table lookups */
    tmp8.x8x2.val[0] = vtbl2_u8(vmorton.vmorton5x2, tmp8.x8x2.val[0]);
    tmp8.x8x2.val[1] = vtbl2_u8(vmorton.vmorton5x2, tmp8.x8x2.val[1]);

    /* reinterpret back as 16x8 */
    tmp16_0 = vreinterpretq_u16_u8(tmp8.x16);

    /* Shifts of every other element */
    tmp16_0 = vmulq_u16(tmp16_0, v21212121);

    /* Combine to final value */
    tmp32_0 = vpaddlq_u16(tmp16_0);

    /* go to u16 */
    morton = vqmovn_u32(tmp32_0);

    /* Set high byte for objects with zero type */
    tmp16_3 = vceq_u16(type, vzero16);
    tmp16_3 = vand_u16(tmp16_3, vf000);    
    morton = vadd_u16(morton, tmp16_3);

    /* Store result */
    vst1_u16(&(pool->_morton[i << 2]), morton);

    // Set up the index while we're at it.
    _object_index [(i << 2)] = (i << 2);
    _object_index [(i << 2) + 1] = (i << 2) + 1;
    _object_index [(i << 2) + 2] = (i << 2) + 2;
    _object_index [(i << 2) + 3] = (i << 2) + 3;
  }

  /* Sort the object index */
  qsort(_object_index, VERLETS, sizeof(uint16_t), morton_comp);

  /* And create an index of morton code to entry in object index */
  int last_m = -1;
  for (int i = 0; i < VERLETS; i++) { /* Index into _object_index */

    _pool._collide[i] = 0; /* TODO TESTING */
    
    uint16_t o = _object_index[i];    /* Index to the object in _pool */
    uint16_t m = _pool._morton[o];    /* Morton code for the object */

    if (last_m < m) {                 /* We have a new morton code */
      for (int j = last_m + 1; j <= m; j++) {
	_morton_index[j] = i; 
      }
      last_m = m;
    }
  }
  
}

int morton_comp(const void * e1, const void* e2) {
  uint16_t m1 = _pool._morton[*((const uint16_t *)e1)];
  uint16_t m2 = _pool._morton[*((const uint16_t *)e2)];
  return m1 - m2;
}
