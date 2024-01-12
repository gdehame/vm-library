#include "cfs.h"

void update_times()
{
  nb_qtm_passe++;
  for (int i = 0; i < total_vCPU; i++)
  {
    if ((vCPU_tab + i)->usedLastQTM == 1)
    {
      //nextContext vaut encore le contexte qu'il faisait tourner au quantum précédent car on n'aura pas encore attribué les suivants
      (vCPU_tab + i)->nextContext->vTime += (vCPU_tab + i)->exec_time_last_qtm;
      (vCPU_tab + i)->exec_time_last_qtm = 0;
      if ((vCPU_tab + i)->in_or_out == 0)
      {
        (vCPU_tab + i)->nextContext->vTime += (unsigned long) nb_qtm_passe * (unsigned long) sched.quantum * 1000000000 - (vCPU_tab + i)->time_last_in;
      }
      (vCPU_tab + i)->time_last_in = sched.quantum * nb_qtm_passe * 1000000000; //Il faut faire cela même si c'est faux car sinon au prochain quantum on va considérer la fin du temps schedulé in du quantum précédent
    }
  }
  //Maintenant, on retire tous les noeuds de l'arbre et on les réinsère avec leur nouveau temps d'exécution
  uThread_tree * new_tree = empty_tree();
  while (global_tree != NULL)
  {
    uThread_tree * node = leftmost(global_tree);
    insert(node->thread, new_tree);
  }
  global_tree = new_tree;
}

uThread_tree * leftmost(uThread_tree* tree)
{
  uThread_tree* node = tree;
  if (node == NULL) return node;
  while (node->left != NULL) node = node->left;
  return node;
}

uThread_tree* vCPU_attribution( uThread_tree* tree)
{
  printf("vcpu_attrib\n");
  int* used = malloc (total_vCPU * sizeof(int));
  Cont** scheduled_nodes = malloc (total_vCPU * sizeof(Cont*));
//  int* v_times = malloc (total_vCPU * sizeof(int));
  for (int i = 0; i < total_vCPU; ++i)
  {
    used[i] = 0;
    scheduled_nodes[i] = NULL;
  }
  //scheduled_nodes = NULL;
/*  for (int i = 0; i < total_vCPU; ++i)
  {
    v_times[i] = 0;
  }
*/  /* if (tree == NULL) printf("tree was null\n"); */
  /* clean_tree(tree, tree); */
  int i = 0;
  /* printf("b\n"); */
  /* if (tree == NULL)  printf("tree is null\n"); */
  printf("a\n");
  while (tree != NULL && i < total_vCPU)
  {
    printf("d ézerfz\n");
    uThread_tree* node = leftmost(tree);
    Cont* node_context = (node != NULL)? node->thread : NULL;
    int node_time = (node!= NULL)? node->thread->vTime : 0;
    printf("c azera\n");
    if (node != NULL && tree != NULL)
    {
      tree = remove_node(node, tree);
    }
    printf("e\n");
    printf("d1\n");
    //      printf("e\n");
    //      printf("node : %d\n",node->cont.id);
    if (node_context != NULL && node_context->scheduledLastQTM == 1)
	  {
	    printf("d2\n");
	    used[node_context->lastvCPU] = 2;
	    scheduled_nodes[node_context->lastvCPU] = node_context;
	  }
    else if (node_context != NULL)
	  {
	    printf("d3\n");
	    vCPU_tab[i].prevContext = vCPU_tab[i].nextContext;
	    node_context->lastvCPU = i;
	    vCPU_tab[i].nextContext = node_context;
	    printf("d3_\n");
	    used[i] = 1;
	    scheduled_nodes[i] = node_context;
	//    v_times[i] = node_time;
	    printf("d4\n");
	   }
    while (i < total_vCPU && used[i] != 0)
    {
      ++i;
    }
  }
  printf("b\n");
  //send signal to swap contexts on vCPUs
  //  printf("c\n");
  for (int j = 0; j < total_vCPU; j++)
  {
    if (used[j] == 1)
	  {
	    pthread_kill((vCPU_tab + j)->vCPU, SIGUSR1);
	  }
    else if (used[j] == 0)
    {
      (vCPU_tab + j)->nextContext = idleContext;
      (vCPU_tab + j)->usedLastQTM = 0;
      pthread_kill((vCPU_tab + j)->vCPU, SIGUSR1);
    }
  }
  for (int i = 0; i < total_vCPU; ++i)
  {
    if (!vCPU_tab[i].lastContextEnded && scheduled_nodes[i] != NULL)
	  {
//	    v_times[i] += max_execution_time; // to change : naïve way of calculating the execution time
	    //tree = insert(tree, scheduled_nodes[i]);
	    tree = insert(scheduled_nodes[i], tree);
	  }
    //      else free(scheduled_nodes[i]);
  }
  free(scheduled_nodes);
//  free(v_times);
  free(used);
  return tree;
}

void handleAlarm_cfs()
{
  update_times();
  global_tree = vCPU_attribution(global_tree);
}
