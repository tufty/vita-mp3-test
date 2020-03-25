#include <psp2/display.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>

#include "bgm.h"
#include "debugScreen.h"

int main(int argc, char *argv[]) {

  int status;

  // Initialise our output
  psvDebugScreenInit();
  PsvDebugScreenFont * font8x8 = psvDebugScreenGetFont();
  PsvDebugScreenFont * font16x16 = psvDebugScreenScaleFont2x(font8x8);
  psvDebugScreenSetFont(font16x16);

  psvDebugScreenPuts("Initialising bgm decoder and creating threads\n");
  
  bgm_init();

  //  psvDebugScreenPrintf("bgm decoder startup done\n");

  bgm_start();
  
  while(1) {
    sceKernelDelayThread(100000);
  }
  
  sceKernelExitProcess(0);
  return status;
}
