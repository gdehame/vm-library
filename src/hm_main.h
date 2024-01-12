#ifndef HMMH
#define HMMH

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

#include "hmgp.h"
#include "hmc.h"

#ifndef GLOBALVARS
#define GLOBALVARS
#define INITIAL_HEAP_SIZE 1000
#define MMAP_THRESHOLD 10240
#define PAGE_SIZE 4096
#endif

int hmm;

void * malloc(int size);
void free(void *ptr);
void * realloc(void *ptr, int size);

#endif
