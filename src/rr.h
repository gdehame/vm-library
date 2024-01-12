#ifndef RRH
#define RRH

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
//#include "hm_main.h"
#include <stdlib.h>

void resetContList();

void handleAlarm_RR();

extern listCont * contexts;

#endif
