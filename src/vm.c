#include "vm.h"

int idNextCont = 1;

uThread_tree * global_tree = NULL;
listCont * contexts = NULL;
int schedulerDefined = 0;
Cont * idleContext = NULL;
Cont * termContext = NULL;
int hmm = -1;
tabCPU * vCPU_tab = NULL;
int total_vCPU = 0;
int nb_qtm_passe = 0;
int max_execution_time = 0;

void * idleP(){while(1){}}
void idle(){while(1){printf("%d est idle\n", getid(pthread_self())); sleep(3);}}

int getid(pthread_t vCPU) //Récupère l'id du vCPU en paramètre, à savoir la place dans le tableau vCPU_tab
{
  for (int i = 0; i < total_vCPU; i++)
  {
    if (pthread_equal((vCPU_tab + i)->vCPU, vCPU))
    {
      return i;
    }
  }
  return -1;
}

void handleUSR() //Se contente de changer de contexte en sauvegardant le précédent si il y en a un à sauvegarder
{
  int CPU_id = getid(pthread_self());
  if (CPU_id != -1)
  {
    if ((vCPU_tab + CPU_id)->prevContext == NULL || (vCPU_tab + CPU_id)->prevContext->context == NULL)
    {
      if (setcontext((vCPU_tab + CPU_id)->nextContext->context) == -1)
      {
        printf("Erreur lors du setcontext\n");
        return;
      }
    }
    else if (swapcontext((vCPU_tab + CPU_id)->prevContext->context, (vCPU_tab + CPU_id)->nextContext->context) == -1)
    {
      printf("Erreur lors du swapcontext\n");
      return;
    }
  }
  else
  {
    printf("Signal envoyé à un vCPU non enregistré (%ld)\n", pthread_self());
  }
}

void handleUSR2()
{
  int CPU_id = getid(pthread_self());
  (vCPU_tab + CPU_id)->thread_id = gettid();
}

void context_terminaison() //Signal avoir terminé son contexte courant et passe sur le contexte d'attente
{
  int CPU_id = getid(pthread_self());
  (vCPU_tab + CPU_id)->nextContext = NULL;
  (vCPU_tab + CPU_id)->lastContextEnded = 1;
  setcontext(idleContext->context);
}

void yield(int nb_quantum)
{
  int CPU_id = getid(pthread_self());
  listCont * p = contexts;
  //On cherche le context dans la liste contexts qui s'execute sur le vCPU;
  while (p->previous != NULL)
  {
    p = p->previous;
  }
  while (p != NULL && p->element.lastvCPU != CPU_id)
  {
    p = p->next;
  }
  if (p == NULL)
  {
    printf("Tentative de yield sur un vCPU dont le context n'est pas dans les contexts à scheduler\n");
    return;
  }
  //A ce moment là, le vCPU a dans sa variable nextContext le contexte qu'il exécute actuellement
  ucontext_t * actual_context = (vCPU_tab + CPU_id)->nextContext->context;
  pthread_mutex_lock(&lock);
  p->element.yielding = nb_quantum;
  (vCPU_tab + CPU_id)->nextContext->vTime += (vCPU_tab + CPU_id)->exec_time_last_qtm;
  (vCPU_tab + CPU_id)->nextContext = idleContext;
  (vCPU_tab + CPU_id)->usedLastQTM = 0;
  pthread_mutex_unlock(&lock);
  swapcontext(actual_context, idleContext->context);
}

void create_vcpu(int nb) //Créé le tableau des vCPUs et les dit vCPUs
{
  if (hmm == -1)
  {
    printf("Veuillez configurer le hmm avant de créer des vCPUs\n");
    return;
  }
  if (sched.defined == 1)
  {
    vCPU_tab = (tabCPU *) malloc (nb*sizeof(tabCPU));
    total_vCPU = nb;
    for (int i = 0; i < nb; i++)
    {
      (vCPU_tab + i)->usedLastQTM = 0;
      (vCPU_tab + i)->thread_id = -1;
      (vCPU_tab + i)->time_last_in = -1;
      (vCPU_tab + i)->in_or_out = -1;
      (vCPU_tab + i)->exec_time_last_qtm = 0;
      (vCPU_tab + i)->lastContextEnded = 0;
      (vCPU_tab + i)->prevContext = NULL;
      (vCPU_tab + i)->nextContext = NULL;
      pthread_create(&((vCPU_tab + i)->vCPU), NULL, idleP, NULL);
    }
  }
  else
  {
    printf("Veuillez configurer un scheduler avant de créer des vCPUs\n");
  }
}

void config_scheduler(int qtm, int schedule) //Configue le scheduler en crééant les contextes de base (idle, terminaison) et le handler de SIGALRM (qui sera le scheduler)
{
  if (sched.running == 1)
  {
    return;
  }
  if (hmm == -1)
  {
    printf("Veuillez configurer le hmm avant le scheduler\n");
    return;
  }

  idleContext = (Cont*) malloc (sizeof(Cont));
  idleContext->scheduledLastQTM = 0;
  idleContext->scheduled = 0;
  idleContext->vTime = 0;
  idleContext->yielding = 0;
  idleContext->lastvCPU = -1;
  ucontext_t* idleC = (ucontext_t*) malloc (sizeof(ucontext_t));
  getcontext(idleC);
  idleC->uc_stack.ss_sp = (char *) malloc (10000);
  idleC->uc_stack.ss_size = 10000;
  makecontext(idleC, idle, 0);
  idleContext->context = idleC;

  termContext = (Cont*) malloc (sizeof(Cont));
  termContext->scheduledLastQTM = 0;
  termContext->scheduled = 0;
  termContext->vTime = 0;
  termContext->yielding = 0;
  termContext->lastvCPU = -1;
  ucontext_t* termC = (ucontext_t*) malloc (sizeof(ucontext_t));
  getcontext(termC);
  termC->uc_stack.ss_sp = (char *) malloc (10000);
  termC->uc_stack.ss_size = 10000;
  makecontext(termC, context_terminaison, 0);
  termContext->context = termC;

  contexts = NULL;
  sched.defined = 1;
  sched.quantum = qtm;

  struct sigaction usr1Handler;
  usr1Handler.sa_handler = &handleUSR;
  usr1Handler.sa_flags = SA_RESTART;
  sigemptyset(&usr1Handler.sa_mask);
  sigaction(SIGUSR1, &usr1Handler, NULL);

  struct sigaction usr2Handler;
  usr2Handler.sa_handler = &handleUSR2;
  usr2Handler.sa_flags = SA_RESTART;
  sigemptyset(&usr2Handler.sa_mask);
  sigaction(SIGUSR2, &usr2Handler, NULL);
  if (schedule == CFS)
  {
    max_execution_time = qtm;
    sched.mod = CFS;
    struct sigaction alarmHandler;
    alarmHandler.sa_handler = &handleAlarm_cfs;
    alarmHandler.sa_flags = SA_RESTART;
    sigemptyset(&alarmHandler.sa_mask);
    if (sigaction(SIGALRM, &alarmHandler, NULL) == -1)
    {
      printf("Erreur lors de la création du handler de l'alarme\n");
      return;
    }
  }
  else if (schedule == RR)
  {
    sched.mod = RR;
    struct sigaction alarmHandler;
    alarmHandler.sa_handler = &handleAlarm_RR;
    alarmHandler.sa_flags = SA_RESTART;
    sigemptyset(&alarmHandler.sa_mask);
    if (sigaction(SIGALRM, &alarmHandler, NULL) == -1)
    {
      printf("Erreur lors de la création du handler de l'alarme\n");
      return;
    }
  }
  else
  {
    printf("Mauvais argument pour le scheduler (0: CFS, 1: RR)\n");
  }
}

void create_uthread(void * afunction) //Créé un contexte pour exécuter afunction et l'ajoute à la liste des contextes (pour RR), à l'abre rouge-noir (pour CFS)
{
  if (sched.defined == 0)
  {
    printf("Veuillez configurer le scheduler avant de créer un uThread\n");
    return;
  }
  if (hmm == -1)
  {
    printf("Veuillez configurer le hmm avant de créer un uThread\n");
    return;
  }
  ucontext_t * c = (ucontext_t *) malloc (sizeof(ucontext_t));
  if (getcontext(c) == -1)
  {
    printf("Erreur lors de la création du contexte\n");
    return;
  }
  c->uc_stack.ss_sp = (char *) malloc (10000);
  c->uc_stack.ss_size = 10000;
  c->uc_link = termContext->context;
  makecontext(c, afunction, 0);

  if (sched.mod == RR)
  {
    //On ajoute le contexte créé à la liste des contextes pour RR
    listCont * p = contexts;
    listCont * new = (listCont *) malloc (sizeof(listCont));
    new->element.context = c;
    new->element.yielding = 0;
    new->element.id = idNextCont;
    idNextCont++;
    new->element.scheduledLastQTM = 0;
    new->element.lastvCPU = -1;
    new->element.vTime = 0;
    new->next = NULL;
    new->element.scheduled = 0;
    if (p == NULL)
    {
      new->previous = NULL;
      contexts = new;
      return;
    }
    while (p->next != NULL)
    {
      p = p->next;
    }
    new->previous = p;
    p->next = new;
  }
  else if (sched.mod == CFS)
  {
    Cont * new_cont = malloc(sizeof(Cont));
    new_cont->context = c;
    new_cont->vTime = 0;
    new_cont->scheduledLastQTM = 0;
    new_cont->scheduled = 0;
    global_tree = insert(new_cont, global_tree);
  }
}

void run() // Lance la VM: créé le signal régulier pour l'appel au scheduler et attend la fin des vCPUs
{
  nb_qtm_passe = 0;
  if (sched.defined == 0)
  {
    printf("run() called before configuring scheduler\n");
    return;
  }

  pthread_mutex_init(&lock, NULL);
  sched.running = 1;
  struct itimerval it;
  it.it_interval.tv_sec = sched.quantum;
  it.it_interval.tv_usec = 0;
  it.it_value = it.it_interval;
  if (setitimer(ITIMER_REAL, &it, NULL) )
  {
    printf("Problème de setitimer\n");
  }
  if (sched.mod == CFS) //Si on est en CFS, on demande aux vCPUs de stocker leur TID via SIGUSR2
  {
    int ended = 0;
    for (int i = 0; i < total_vCPU; i++)
    {
      pthread_kill((vCPU_tab + i)->vCPU, SIGUSR2);
    }
    while (ended != 1)
    {
      ended = 1;
      for (int i = 0; i < total_vCPU; i++)
      {
        if ((vCPU_tab + i)->thread_id == -1)
        {
          ended = 0;
          break;
        }
      }
    }
  }
  if (sched.mod == RR)
  {//En RR, on fait un join sur les vCPUs (pthread) et on free la liste des contexts lorsque tout est terminé
    for (int i = 0; i < total_vCPU; i++)
    {
      pthread_join((vCPU_tab + i)->vCPU, NULL);
    }
    listCont * p = contexts;
    while ( p!= NULL && p->previous != NULL)
    {
      p = p->previous;
    }
    while (p != NULL && p->next != NULL)
    {
      free(p->element.context);
      p = p->next;
      free(p->previous);
    }
    free(p->element.context);
    free(p);
  }
  if (sched.mod == CFS)
  {//En CFS, on installe le module dans le noyau puis on boucle infiniement pour lire les différents scheduling in/out notifiés dans la mémoire partagée
    system("make clean");
    system("make");
    system("modprobe uio");
    //On écrit puis exécute la commande `insmod uio_device2.ko pidarray=pid1,pid2,...` pour tracker les pthread
    char * cmd = (char*) malloc (31);
    int size = 31;
    char * s = "insmod uio_device2.ko pidarray=";
    for (int i = 0; i < 31; i++)
    {
      cmd[i] = s[i];
    }
    //Ici, on traduit sous forme de char* la liste des TID à tracker
    for (int i = 0; i < total_vCPU; i++)
    {
      int tid = (vCPU_tab + i)->thread_id;
      char * s = (char*) malloc(10);
      int j = 0;
      while (tid != 0)
      {
        s[j] = '0' + tid % 10;
        j++;
        tid /= 10;
      }
      size += j;
      cmd = realloc(cmd, size+1);
      int ind = size - j;
      while (j > 0)
      {
        cmd[ind] = s[j-1];
        j--;
        ind++;
      }
      size++;
      if (i != total_vCPU - 1)
      {
        cmd[ind] = ',';
      }
      else
      {
        cmd[ind] = '\0';
      }
    }
    printf("\n%s\n\n", cmd);
    system(cmd);
    int uioFd = open("/dev/uio0", O_RDWR);
    char * start = mmap(0, 0x1000, PROT_READ, MAP_SHARED, uioFd, 0);
    unsigned long * buffer = (unsigned long *) start;
    while (1)
    {//Boucle infinie de lecture de la mémoire partagée
      u_int32_t intInfo;
      ssize_t readSize;

      // Acknowldege interrupt
      intInfo = 1;
      readSize = read(uioFd, &intInfo, sizeof(intInfo));
      unsigned long time = buffer[0];
      unsigned long pid = buffer[1];
      unsigned long in_out = buffer[2];
      for (int i = 0; i < total_vCPU; i++)
      {
        if ((vCPU_tab + i)->thread_id != -1 && pid == (vCPU_tab + i)->thread_id)
        {
          (vCPU_tab + i)->in_or_out = in_out;
          if (in_out == 0)
          {
            (vCPU_tab + i)->time_last_in = time;
          }
          else if (in_out == 1 && (vCPU_tab + i)->time_last_in!= -1)
          {
            (vCPU_tab + i)->exec_time_last_qtm += time - (vCPU_tab + i)->time_last_in;
          }
          break;
        }
      }
      if ((char*)buffer - (char*)start > 4093 || (char*)buffer - (char*)start < -4093)
      {
        buffer = (unsigned long *) start;
      }
      else
      {
        buffer += 3;
      }
    }
    munmap((void*)start, 0x1000);
    close(uioFd);
    system("rmmod uio_device2");
    system("make clean");
  }
  sched.defined = 0;
  free(vCPU_tab);
  free(idleContext);
  free(termContext);
  hmm = -1;
  sched.running = 0;
}

void configure_hmm(int n) //Set le hmm à 0 (canary) ou 1 (page de garde)
{
  if (n != 0 && n != 1)
  {
    printf("Erreur de configuration du hmm: veuillez donner 0 pour Canary et 1 pour Page de garde\n");
    return;
  }
  hmm = n;
}
