
#include <stdio.h>

#include "util.h"


/** Clears the screen
*/

void clear_screen() {
  printf("\033[2J");
  printf("\033[0;0f");
}
