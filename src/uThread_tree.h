#ifndef uThread_tree_h
#define uThread_tree_h

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

uThread_tree *empty_tree(void);

uThread_tree *insert(Cont *thread, uThread_tree *tree);

uThread_tree * recolor_on_insert(uThread_tree *tree);

uThread_tree * recolor_on_removal(uThread_tree *tree);

uThread_tree *rotate_right(uThread_tree *tree);

uThread_tree *rotate_left(uThread_tree *tree);

uThread_tree *remove_node(uThread_tree *node, uThread_tree *tree);

uThread_tree *get_root(uThread_tree *node);

enum color get_color(uThread_tree *node);

#endif /* uThread_tree_h */
