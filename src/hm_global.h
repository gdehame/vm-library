#ifndef COMMON
#define COMMON

#include <signal.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <ucontext.h>
#include <string.h>
#include <stdio.h>

extern char * plus_bas_mmap;
extern int heap_reached_stack;

//Fonction max... Parce que comme d'hab elle est pas dans les fonctions de base
int max(int a, int b);

#endif
