#ifndef CFSH
#define CFSH

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

#include "structures.h"
#include "uThread_tree.h"
//#include "hm_main.h"
#include <stdlib.h>

void update_times();

uThread_tree * leftmost(uThread_tree* tree);

uThread_tree* vCPU_attribution( uThread_tree* tree);

void handleAlarm_cfs();

extern int nb_qtm_passe;
extern int max_execution_time;

extern uThread_tree * global_tree;

#endif
