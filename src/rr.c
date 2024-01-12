#ifndef RRC
#define RRC

#include "rr.h"

void resetContList() //Revient au début de la liste
{
  while(contexts != NULL && contexts->previous != NULL)
  {
    contexts = contexts->previous;
  }
}

void handleAlarm_RR()
{
  listCont * p = contexts;
  for (int j = 0; j < total_vCPU; j++) //On recherche les contextes qui auraient terminés lors du dernier quantum
  {
    if ((vCPU_tab + j)->lastContextEnded == 1)
    {
      (vCPU_tab + j)->lastContextEnded = 0;
      p = contexts;
      while (p->previous != NULL)
      {
        p = p->previous;
      }
      while (p != NULL) //On parcourt les contextes à la recherche du contexte qui était sur le vCPU (vCPU_tab + j)
      {
        if (p->element.lastvCPU == j)
        {//On retire le contexte qui a terminé de la liste des contextes
          listCont * pt = p;
          p = p->next;
          if (pt->next != NULL)
          {
            pt->next->previous = pt->previous;
          }
          if (pt->previous != NULL)
          {
            pt->previous->next = pt->next;
          }
          free(pt->element.context);
          free(pt);
          break;
        }
        p = p->next;
      }
    }
  }
  int * usableVCPU = (int *) malloc (sizeof(int) * total_vCPU); //Ce tableau servira à déterminer quels vCPU vont garder leur contexte précédent (0) ou vont changer de contexte (1)
  for (int j = 0; j < total_vCPU; j++)
  {
    *(usableVCPU + j) = 1;
  }
  listTBS * tobeSched = NULL;
  listCont * lastScheduled = contexts;
  p = contexts;
  while (p->previous != NULL) //On reset l'état de schedulé ou non de chaque contexte qui ne sont pas terminés (on a retiré les terminés)
  {
    p = p->previous;
  }
  while (p != NULL)
  {
    p->element.scheduled = 0;
    p = p->next;
  }
  for (int i = 0; i < total_vCPU; i++)
  {
    int tousSchedule = 0; //Determine si on a encore un uThread a scheduler, pour cela on le met à 1 la première fois qu'on arrive en fin de liste et si on atteint de nouveau la fin de la liste des contexts c'est qu'ils sont tous schedulés
    while(contexts->element.scheduled != 0) //On cycle à travers les contextes en partant du premier après le dernier schedulé (on set après avoir déterminé les contextes a scheduler la variable contextes au contexte qui suit le dernier schedulé pour les prochain réveil du scheduler ce qui assure le bon scheduling RR)
    {
      if (contexts->next != NULL)
      {
        contexts = contexts->next;
      }
      else if (tousSchedule == 1)
      {//Dans ce cas, on a parcouru tous les contextes et donc on n'en a plus aucun à scheduler
        tousSchedule++;
        break;
      }
      else
      {
        tousSchedule = 1;
        resetContList();
      }
    }
    if (tousSchedule < 2 && contexts->element.yielding == 0)
    {
      lastScheduled = contexts;
      contexts->element.scheduled = 1;
      if (contexts->element.scheduledLastQTM == 1)
      {//Dans ce cas on garde le même vCPU
        *(usableVCPU + contexts->element.lastvCPU) = 0;
      }
      else
      {//On l'ajoute à la liste des contextes à scheduler qui imposent un changement de contexte sur un vCPU
        listTBS * new = (listTBS *) malloc (sizeof(listTBS));
        new->originalContext = contexts;
        new->previous = tobeSched;
        new->next = NULL;
        if (tobeSched != NULL)
        {
          tobeSched->next = new;
        }
        tobeSched = new;
      }
    }
    if (contexts->element.yielding != 0)
    {
      contexts->element.scheduled = -1; //Il faut que ce soit différent d'un contexte schedulé et différent d'un contexte non schedulé quelconque car on ne veut pas le scheduler et on ne veut pas  qu'au prochain quantum on le considère comme ayant été schedulé
      i--;
    }
  }
  if (lastScheduled != NULL && lastScheduled->next != NULL)
  {
    contexts = lastScheduled->next;
  }
  else
  {
    resetContList();
  }

  p = contexts;
  while (p->previous != NULL)
  {
    p = p->previous;
  }
  while (p != NULL) //On met à jour les variables scheduledLastQTM pour le prochain passage du scheduler, à ce moment du code, les variables scheduled valent 0 si on n'a pas schedulé pour le prochain quantum et 1 sinon
  {
    p->element.scheduledLastQTM = p->element.scheduled;
    if (p->element.yielding > 0)
    {
      p->element.yielding -= 1;
    }
    p = p->next;
  }

  if (tobeSched != NULL) //On vérifie qu'il y a des contextes à changer
  {
    while (tobeSched->previous != NULL)
    {
      tobeSched = tobeSched->previous;
    }
    int i = 0;
    while(i < total_vCPU && *(usableVCPU + i) == 0) //On passe tous les vCPUs qui gardent leur contexte jusqu'au premier qui doit changer
    {
      i++;
    }
    listTBS * pSched = tobeSched;
    while (pSched != NULL)
    {
      if (i == total_vCPU)
      {
        printf("Erreur lors du scheduling: plus d'uthread schedulé que de vCPU\n");
        break;
      }
      //On met à jour les contextes précédents/suivants du vCPU (qui change de contexte)
      (vCPU_tab + i)->prevContext = (vCPU_tab + i)->nextContext;
      (vCPU_tab + i)->nextContext = &(pSched->originalContext->element);
      (vCPU_tab + i)->usedLastQTM = 1;
      pSched->originalContext->element.lastvCPU = i;
      pSched = pSched->next;
      i++;
      while(i < total_vCPU && *(usableVCPU + i) == 0)//On passe tous les vCPUs qui gardent leur contexte jusqu'au premier qui doit changer
      {
        i++;
      }
    }
    while (tobeSched->next != NULL) //On libère la liste temporaire
    {
      tobeSched = tobeSched->next;
      free(tobeSched->previous);
    }
    free(tobeSched);
    while (i < total_vCPU) //On met les vCPUs inutilisé au prochain quantum sur le contexte d'attente
    {
      if ((vCPU_tab + i)->nextContext != idleContext)
      {

        (vCPU_tab + i)->nextContext = idleContext;
        (vCPU_tab + i)->usedLastQTM = 0;
      }
      i++;
      while(i < total_vCPU && *(usableVCPU + i) == 0)
      {
        i++;
      }
    }
    for (int j = 0; j < total_vCPU; j++) //On demande aux vCPUs qui en on besoin de changer de contextes
    {
      if (*(usableVCPU + j) == 1)
      {
        pthread_kill((vCPU_tab + j)->vCPU, SIGUSR1);
      }
    }
  }
  free(usableVCPU);
}

#endif
