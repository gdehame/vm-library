#ifndef HMGPH
#define HMGPH

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

struct gp_bloc
{
	int size_to_malloc;//Taille totale du gp_bloc
	int available;//Place encore disponible dans le gp_bloc "à gauche" des métas données
	int state; //Marque le gp_bloc comme étant libre(0) ou non(1)
	int safety; //Pour les pages de gardes: vaut 1 si le gp_bloc est suivit d'une page de garde et 0 sinon
	struct gp_bloc *next;
	struct gp_bloc *prev;
	void *donnee;//Pointeur vers la donnée du gp_bloc
};

struct gp_bloc_mmap
{
	int total_size; //taille totale du gp_bloc (sizeof(struct gp_bloc_mmap) compris)
  int used_space; //taille de l'espace utilisé dans le gp_bloc
	int state; //Marque le gp_bloc comme étant libre(0) ou non(1)
	struct gp_bloc_mmap *next;
	struct gp_bloc_mmap *prev;
	void * donnee;//Pointeur vers la donnée du gp_bloc
};

int gp_bloc_size(int size);

void * gp_split (struct gp_bloc * to_split, int size);

struct gp_bloc * replace(struct gp_bloc * blc, int size);

void gp_try_to_merge_with_next (struct gp_bloc *to_merge);

struct gp_bloc_mmap * gp_create_bloc_mmap (int size);

struct gp_bloc * gp_create_bloc (int size);

void gp_init_heap(int size);

void * gp_malloc(int size);

void gp_free(void *ptr);

void * gp_realloc(void *ptr, int size);

#endif
