#include "verlet.h"

#include <math.h>
#include <stdlib.h>
#include <arm_neon.h>

verlet_pool_t _pool;

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
  }
}

uint8_t morton5[] = { 0x00, 0x01, 0x04, 0x05, 0x10, 0x11, 0x14, 0x15,
		      0x40, 0x41, 0x44, 0x45, 0x50, 0x51, 0x54, 0x55 };

void verlet_pool_integrate (verlet_pool_t * pool, float dt_over_dt, float dt_squared) {
  /* Vector versions of the arguments */
  float32x4_t dtdt, dt2;
  
  /* Constants we'll be wanting */
  float32x4_t vzero, vone, v1023;

  union {
    uint8x8x2_t vmorton5x2;
    uint8x16_t vmorton5;
  } vmorton;
  
  uint16x4_t vbit4;

  /* Values we read and write from / to the pool */
  float32x4_t forces[2], pos_then[2], pos_now[2], direction[2];
  float32x4_t one_over_mass;

  /* Local variables */
  float32x4_t acceleration[2], norm;
  uint32x4_t tmp;
  uint16x4_t morton[2];

  /* set up our loop invariants */
  dtdt             = vdupq_n_f32(dt_over_dt);
  dt2              = vdupq_n_f32(dt_squared);
  
  vzero            = vdupq_n_f32(0.0);
  vone             = vdupq_n_f32(1.0);
  v1023            = vdupq_n_f32(1023.0);

  vbit4            = vdup_n_u16(0x10);
  vmorton.vmorton5 = vld1q_u8(morton5);
    
  /* Loop through, 4-wise */
  for (int i = 0; i < (VERLETS / 4); i++) {
    /* Load values */
    pos_now[0]    = vld1q_f32(&(pool->_pos_now[0][i << 2]));
    pos_then[0]   = vld1q_f32(&(pool->_pos_then[0][i << 2]));
    forces[0]     = vld1q_f32(&(pool->_forces[0][i << 2]));
    pos_now[1]    = vld1q_f32(&(pool->_pos_now[1][i << 2]));
    pos_then[1]   = vld1q_f32(&(pool->_pos_then[1][i << 2]));
    forces[1]     = vld1q_f32(&(pool->_forces[1][i << 2]));
    
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

    /* Scale up for morton calculation */
    //   pos_now[0] = vmulq_n_f32(pos_now[0], 1023);

    /* Convert to integer */
    uint32x4_t tmp32 = vcvtq_u32_f32(pos_now[0]);
    /* Shift right by 5 */
    tmp32 = vshrq_n_u32(tmp32, 5);
    /* go to 16 bit */
    uint16x4_t tmp16 = vqmovn_u32(tmp32);

    /* Extract high bit (can't use it) */
    uint16x4_t hibit = vand_u16(tmp16, vbit4);

    /* Clear high bit */
    tmp16 = veor_u16(tmp16, hibit);

    /* multiply by scalar and accumulate */
    vmla_n_u16(tmp16, hibit, 16);
    
    /* reinterpret 16x4 as 8x8 */
    uint8x8_t tmp8 = vreinterpret_u8_u16(tmp16);
    /* Table lookup */
    tmp8 = vtbl2_u8(vmorton.vmorton5x2, tmp8);

    morton[0] = vreinterpret_u16_u8(tmp8);

    // And the y axis
    tmp32 = vcvtq_u32_f32(pos_now[1]);
    /* Shift right by 5 */
    tmp32 = vshrq_n_u32(tmp32, 5);
    /* go to 16 bit */
    tmp16 = vqmovn_u32(tmp32);

    /* Extract high bit (can't use it) */
    hibit = vand_u16(tmp16, vbit4);

    /* Clear high bit */
    tmp16 = veor_u16(tmp16, hibit);

    /* multiply by scalar and accumulate */
    tmp16 = vmla_n_u16(tmp16, hibit, 16);
    
    /* reinterpret 16x4 as 8x8 */
    tmp8 = vreinterpret_u8_u16(tmp16);
    /* Table lookup */
    tmp8 = vtbl2_u8(vmorton.vmorton5x2, tmp8);

    morton[1] = vreinterpret_u16_u8(tmp8);

    morton[0] = vmla_n_u16(morton[0], morton[1], 2);

    vst1_u16(&(pool->_morton[i << 2]), morton[0]);
      
  }
}



int morton_comp(const void * e1, const void* e2) {
  uint32_t m1 = _pool._morton[*((const uint16_t *)e1)];
  uint32_t m2 = _pool._morton[*((const uint16_t *)e2)];
  return m1 - m2;
}

void sort_by_morton(uint16_t * array) {
  qsort(array, VERLETS, sizeof(uint16_t), morton_comp);
}
