#ifndef VMH
#define VMH

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

extern int hmm;
//#include "hm_main.c"
#include <stdlib.h>
#include "structures.h"
#include "rr.h"
#include "cfs.h"

#define gettid() syscall(SYS_gettid)

#define CFS 0
#define RR 1

extern int schedulerDefined;

pthread_mutex_t lock;

int getid(pthread_t vCPU);

void handleUSR();

void context_terminaison();

void yield(int nb_quantum);

void create_vcpu(int nb);

void config_scheduler(int qtm, int schedule);

void create_uthread(void * afunction);

void configure_hmm(int n);

void run();

#endif
