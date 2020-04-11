#include "verlet.h"

#include <math.h>
#include <arm_neon.h>

static verlet_pool_t _pool;

void init_verlet_pool() {
  for (int i = 0; i < VERLETS; i++) {
    for (int d = 0; d < DIMENSIONS; d++) {
      _pool._pos_then[d][i] = 0;
      _pool._pos_now[d][i] = 0;
      _pool._forces[d][i] = 0;
      _pool._direction[d][i] = 0;
    }
    vzero            = vdupq_n_f32(0.0);
    vone             = vdupq_n_f32(1.0);
    v00ff00ff        = vdupq_n_u32(0x00ff00ff);
    v0f0f0f0f        = vdupq_n_u32(0x0f0f0f0f);
    v33333333        = vdupq_n_u32(0x33333333);
    v55555555        = vdupq_n_u32(0x55555555);
    
    /* Start calculating */
    /* acceleration */
    acceleration[0] = vmulq_f32(forces[0], one_over_mass);
    acceleration[1] = vmulq_f32(forces[1], one_over_mass);

    /* direction */
    direction[0] = vsubq_f32(pos_then[0], pos_now[0]);
    direction[1] = vsubq_f32(pos_then[1], pos_now[1]);

    /* size of direction */
    norm = vmulq_f32(direction[0], direction[0]);
    norm = vmlaq_f32(direction[1], direction[1], norm);
    norm = vrsqrteq_f32(norm);

    /* scale direction to unit vector */
    direction[0] = vmulq_f32(direction[0], norm);
    direction[1] = vmulq_f32(direction[1], norm);

    /* Store the new "then" (old "now") */
    vst1q_f32(&(_pool._direction[0][i << 2]), direction[0]);
    vst1q_f32(&(_pool._direction[1][i << 2]), direction[1]);

    /* Store the new "then" (old "now") */
    vst1q_f32(&(_pool._pos_then[0][i << 2]), pos_now[0]);
    vst1q_f32(&(_pool._pos_then[1][i << 2]), pos_now[1]);

    /* Calculate the new now */
    pos_now[0] = vmlaq_f32(direction[0], dtdt, pos_then[0]);
    pos_now[0] = vmlaq_f32(acceleration[0], dt2, pos_now[0]);
    pos_now[1] = vmlaq_f32(direction[1], dtdt, pos_then[1]);
    pos_now[1] = vmlaq_f32(acceleration[1], dt2, pos_now[1]);

    /* clamp to 0 <= n <= 1 */
    pos_now[0] = vminq_f32(pos_now[0], vone);
    pos_now[0] = vmaxq_f32(pos_now[0], vzero);
    pos_now[1] = vminq_f32(pos_now[1], vone);
    pos_now[1] = vmaxq_f32(pos_now[1], vzero);

    /* Store the new "now" */
    vst1q_f32(&(_pool._pos_now[0][i << 2]), pos_now[0]);
    vst1q_f32(&(_pool._pos_now[1][i << 2]), pos_now[1]);

    /* Scale up for morton calculation */
    pos_now[0] = vmulq_n_f32(pos_now[0], 1024);

    /* Convert to integer */
    morton[0] = vcvtq_u32_f32(pos_now[0]);

    /* bit twiddle */
    tmp = vshlq_n_u32(morton[0], 8);
    morton[0] = vorrq_u32(morton[0], tmp);
    morton[0] = vandq_u32(morton[0], v00ff00ff);
    tmp = vshlq_n_u32(morton[0], 4);
    morton[0] = vorrq_u32(morton[0], tmp);
    morton[0] = vandq_u32(morton[0], v0f0f0f0f);
    tmp = vshlq_n_u32(morton[0], 2);
    morton[0] = vorrq_u32(morton[0], tmp);
    morton[0] = vandq_u32(morton[0], v33333333);
    tmp = vshlq_n_u32(morton[0], 1);
    morton[0] = vorrq_u32(morton[0], tmp);
    morton[0] = vandq_u32(morton[0], v55555555);

    /* Same for y element */
    /* Scale up for morton calculation */
    pos_now[1] = vmulq_n_f32(pos_now[1], 1024);

    /* Convert to integer */
    morton[1] = vcvtq_u32_f32(pos_now[1]);

    /* bit twiddle */
    tmp = vshlq_n_u32(morton[1], 8);
    morton[1] = vorrq_u32(morton[1], tmp);
    morton[1] = vandq_u32(morton[1], v00ff00ff);
    tmp = vshlq_n_u32(morton[1], 4);
    morton[1] = vorrq_u32(morton[1], tmp);
    morton[1] = vandq_u32(morton[1], v0f0f0f0f);
    tmp = vshlq_n_u32(morton[1], 2);
    morton[1] = vorrq_u32(morton[1], tmp);
    morton[1] = vandq_u32(morton[1], v33333333);
    tmp = vshlq_n_u32(morton[1], 1);
    morton[1] = vorrq_u32(morton[1], tmp);
    morton[1] = vandq_u32(morton[1], v55555555);

    /* Combine and save */
    morton[1] = vshlq_n_u32(morton[1], 1);
    tmp = vorrq_u32(morton[0], morton[1]);
    vst1q_u32(&(_pool._morton[i << 2]), tmp);
      
  }
}



#if 0


void integrate (float dt_over_dt, float dt_squared) {

  // With luck, this will get vectorised and I won't have to bother doing it manually
  for (int i = 0; i < VERLETS; i++) {
    verlet_t distance = 0;
    _pool._morton[i] = 0;
    
    for (int d = 0; d < DIMENSIONS; d++) {
      verlet_t acceleration, new_pos;
      // The actual verlet integration
      acceleration = _pool._forces[d][i] * _pool._one_over_mass[i];
      _pool._direction[d][i] = _pool._pos_now[d][i] - _pool._pos_then[d][i];
      _pool._pos_then[d][i] = _pool._pos_now[d][i];
      
      /* new_pos = fmaf(_pool._direction[d][i], dt_over_dt, _pool._pos_now[d][i]); */
      /* new_pos = fmaf (acceleration, dt_squared, new_pos); */
      new_pos = (_pool._direction[d][i] * dt_over_dt) + _pool._pos_now[d][i] + (acceleration * dt_squared);

      // clamp to 0 <= x <= 1.  This should be being vectorised to vsat, but it's not.
      _pool._pos_now[d][i] = fminf(fmaxf(new_pos, 0), 1);

      distance += _pool._direction[d][i] * _pool._direction[d][i];
    }

    distance = 1 / sqrtf(distance);

    /* Scale direction to unit size */
    _pool._direction[0][i] *= distance;
    _pool._direction[1][i] *= distance;

    /* scale position to 0 <= x <= 1024 */
    int32_t m[2] = { ldexpf(_pool._pos_now[0][i], 10), ldexpf(_pool._pos_now[1][i], 10)};

    /* Calculate morton order by bit twiddlery */
    m[0] = (m[0] | (m[0] << 8)) & 0x00ff00ff;
    m[0] = (m[0] | (m[0] << 4)) & 0x0f0f0f0f;
    m[0] = (m[0] | (m[0] << 2)) & 0x33333333;
    m[0] = (m[0] | (m[0] << 1)) & 0x55555555;
    m[1] = (m[1] | (m[1] << 8)) & 0x00ff00ff;
    m[1] = (m[1] | (m[1] << 4)) & 0x0f0f0f0f;
    m[1] = (m[1] | (m[1] << 2)) & 0x33333333;
    m[1] = (m[1] | (m[1] << 1)) & 0x55555555;
    
    _pool._morton[i] = m[0] + (m[1] << 1);
      
  }
}

#endif
