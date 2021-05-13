#include <psp2/ctrl.h>
#include <stdlib.h>
#include <string.h>
#include "controls.h"

SceCtrlData ctrl_samples[2];

void controls_init() {
  sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);

  // Read in a "start" sample
  sceCtrlReadBufferPositive(0, ctrl_samples, 2);
}

void controls_sample(int port, SceCtrlData * ctrl_data, uint32_t * pressed, uint32_t * released) {
  // peek controller state into our current buffer
  sceCtrlPeekBufferPositive(port, ctrl_samples, 2);
  memcpy(ctrl_data, &(ctrl_samples[1]), sizeof(SceCtrlData));
  uint32_t changed_buttons = ctrl_samples[0].buttons ^ ctrl_samples[1].buttons;
  if (pressed) *pressed = changed_buttons & ctrl_samples[1].buttons;
  if (released) *released = changed_buttons & ctrl_samples[0].buttons;
}
