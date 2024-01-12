#include "hm_global.h"

char * plus_bas_mmap = NULL;
int heap_reached_stack = 0;

int max (int a, int b)
{
  if (a > b)
  {
    return a;
  }
  return b;
}
