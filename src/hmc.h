#ifndef HMCH
#define HMCH

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

#ifndef GLOBALVARS
#define GLOBALVARS
#define INITIAL_HEAP_SIZE 1000
#define MMAP_THRESHOLD 10240
#define PAGE_SIZE 4096
#endif

#include "hm_global.h"

struct bloc
{
	int size_to_malloc;//Taille totale du bloc
	int available;//Place encore disponible dans le bloc
	int state;
	struct bloc *next;
	struct bloc *prev;
	void *donnee;//Pointeur vers la donnée du bloc
};

struct bloc_mmap
{
	int size_usable;//Taille totale - taille(struct bloc)
	int state;
	struct bloc_mmap *next;
	struct bloc_mmap *prev;
	void * donnee;//Pointeur vers la donnée du bloc
};

void * c_split (struct bloc *to_split, int size);

void c_try_to_merge_with_next (struct bloc *to_merge);

void c_init_heap(int size);

struct bloc_mmap * c_create_bloc_mmap (int size);

struct bloc * c_create_bloc (int size);

void * c_malloc(int size);

void * rmalloc(int size);

void c_free(void *ptr);

void *c_realloc(void *ptr, int size);

#endif
