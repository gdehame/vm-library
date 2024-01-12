#ifndef STRUCTURESH
#define STRUCTURESH

struct scheduler
{
  int defined; //0 si non défini, 1 sinon;
  int running; //0 si run() non exécuté, 1 sinon
  int quantum; //Durée du quantum
  int mod; //0=CFS, 1=RR
};

typedef struct c
{
  ucontext_t * context;
  int scheduledLastQTM;
  int scheduled;
  int vTime;
  int yielding; //Vaut le nombre de quantum pendant lesquels il ne faut pas le scheduler
  int lastvCPU;
  int id;
} Cont;

typedef struct lc
{
  pthread_t vCPU;
  pid_t thread_id;
  unsigned long time_last_in;
  int in_or_out; //vaut 0 si il lors de la dernière notification il a été schedule in, et 1 si c'était out
  unsigned long exec_time_last_qtm;
  int usedLastQTM;
  int lastContextEnded;
  Cont * prevContext;
  Cont * nextContext;
} tabCPU;

typedef struct l
{
  struct l * next;
  Cont element;
  struct l * previous;
} listCont;

typedef struct ls
{
  struct ls * next;
  listCont * originalContext;
  struct ls * previous;
} listTBS;

enum color { RED, BLACK };

struct uThread_tree
{
  Cont *thread;
  enum color color; //red <-> 0 && black <-> 1
  struct uThread_tree *parent;
  struct uThread_tree *left;
  struct uThread_tree *right;
  struct uThread_tree *leftmost;
};
typedef struct uThread_tree uThread_tree;

extern Cont * idleContext;
extern Cont * termContext;

extern tabCPU * vCPU_tab;
extern int total_vCPU;

struct scheduler sched;

#endif
