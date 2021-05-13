#pragma once

#include <stdint.h>
#include <psp2/ctrl.h>

void controls_init();
void controls_sample(int port, SceCtrlData * ctrl_data, uint32_t * pressed, uint32_t * released);
