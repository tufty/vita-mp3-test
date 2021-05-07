#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>

#define FRAME_SIZE (1152 * 2) 

int main (int argc, char ** argv) {
  FILE * fh;
  int16_t frame[FRAME_SIZE];
  size_t loaded = 0;
  
  if (argc != 3) {
    printf ("usage : %s <file.mp3> <frame>\n", argv[0]);
    return -1;
  }

  fh = fopen(argv[1], "r");

  if (fh == NULL) {
    printf("Unable to open %s\n", argv[1]);
    return errno;
  }

  while (loaded < atoi(argv[2])) {
    
    fread (frame, sizeof(int16_t), FRAME_SIZE, fh);
    
    if ((((uint8_t*)frame)[0] == 'I') &&
	(((uint8_t*)frame)[0] == 'D') &&
	(((uint8_t*)frame)[0] == '3')) {
      int seek = ((((uint8_t*)frame)[6] & 0x7f) << 21) +
	((((uint8_t*)frame)[7] & 0x7f) << 14) +
	((((uint8_t*)frame)[8] & 0x7f) << 7) +
	(((uint8_t*)frame)[9] & 0x7f) +
	((((uint8_t*)frame)[5] & 0x10) ? 20 : 10);
      
      fseek (fh, seek, SEEK_SET);
    }
    loaded ++;
  }

  fclose (fh);
  
  puts ("left :\n");
  for (int i = 0; i < 1024; i++) {
    printf ("%i, ", frame[i >> 1]);
  }
  puts ("\n\n");
  puts ("right :\n");
  for (int i = 0; i < 1024; i++) {
    printf ("%i, ", frame[(i >> 1) + 1]);
  }
  puts ("\n\n");

  return 0;
}
