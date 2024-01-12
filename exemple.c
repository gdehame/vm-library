#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "./src/vm.c"

void printCont1()
{
  while(1)
  {
    printf("Je suis le contexte 1\n");
    sleep(3);
  }
}

void printCont2()
{
  while(1)
  {
    printf("Je suis le contexte 2\n");
    sleep(3);
  }
}

void printCont3()
{
  while (1)
  {
    printf("Je suis le contexte 3\n");
    sleep(3);
  }
}

void printCont4()
{
  while(1)
  {
    printf("Je suis le contexte 4\n");
    sleep(3);
  }
}



void main()
{
  configure_hmm(1);
  config_scheduler(5, 0);
  create_vcpu(3);
  create_uthread(printCont1);
  create_uthread(printCont2);
  create_uthread(printCont3);
  create_uthread(printCont4);
  run();
}
