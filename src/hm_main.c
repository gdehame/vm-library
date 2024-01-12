#ifndef HMMAINC
#define HMMAINC

#include "hm_main.h"

#ifndef COMMON
#define COMMON
char * plus_bas_mmap = NULL;
int heap_reached_stack = 0;
//Fonction max... Parce que comme d'hab elle est pas dans les fonctions de base
int max(int a, int b)
{
  if (a > b)
  {
    return a;
  }
  return b;
}
#endif

void * malloc(int size) //fonction d'interface qui choisit entre canary et page de garde
{
  if (hmm == 0)
  {
    return c_malloc(size);
  }
  else if (hmm == 1)
  {
    return gp_malloc(size);
  }
}

void free(void * ptr) //fonction d'interface qui choisit entre canary et page de garde
{
  if (hmm == 0)
  {
    c_free(ptr);
  }
  else if (hmm == 1)
  {
    gp_free(ptr);
  }
}

void * realloc(void * ptr, int size) //fonction d'interface qui choisit entre canary et page de garde
{
  if (hmm == 0)
  {
    return c_realloc(ptr, size);
  }
  else if (hmm == 1)
  {
    return gp_realloc(ptr, size);
  }
}

#endif
