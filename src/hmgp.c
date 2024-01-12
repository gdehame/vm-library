#ifndef HMGPC
#define HMGPC

#include "hmgp.h"

struct gp_bloc * gp_cache = NULL;
struct gp_bloc_mmap * gp_cache_mmap = NULL;

int gp_bloc_size(int size) //Calcul le plus petit multiple de la taille d'une page supérieru à la taille du gp_bloc souhaité
{
  int page_size = sysconf(_SC_PAGE_SIZE);
  int real_size = page_size;
  while (real_size < size)
  {
    real_size += page_size;
  }
  return (real_size);
}

//Split d'un gp_bloc. S'il a suffisamment de place pour créer un autre gp_bloc, le fait, sinon renseigne la place excédentaire comme utilisable par le gp_bloc.
//Retourne un pointeur vers le gp_bloc qu'on a divisé
void * gp_split (struct gp_bloc * to_split, int size)
{
  //On créé un nouveau bloc, si le bloc a split a une page de garde devant lui (safety=1) on le colle derrière lui, sinon on laisse une page de garde entre
  int real_size = gp_bloc_size(size + sizeof(struct gp_bloc));
  struct gp_bloc * new_gp_bloc = NULL;
  if (to_split->safety == 1)
  {
    new_gp_bloc = (struct gp_bloc *)((char*)to_split + to_split->size_to_malloc - to_split->available - size - sizeof(struct gp_bloc));
    to_split->size_to_malloc = to_split->size_to_malloc - real_size;
  }
  else
  {
    new_gp_bloc = (struct gp_bloc *)((char*)to_split + to_split->size_to_malloc - to_split->available - size - sizeof(struct gp_bloc) - sysconf(_SC_PAGE_SIZE));
    to_split->size_to_malloc = to_split->size_to_malloc - real_size - sysconf(_SC_PAGE_SIZE);
    mprotect((char*)new_gp_bloc + size + sizeof(struct gp_bloc), sysconf(_SC_PAGE_SIZE), PROT_NONE);
  }
  new_gp_bloc->size_to_malloc = real_size;
  new_gp_bloc->available = real_size - size - sizeof(struct gp_bloc);
  new_gp_bloc->safety = 1;
  new_gp_bloc->state = 1;
  new_gp_bloc->donnee = (void*)((char*)new_gp_bloc + sizeof(struct gp_bloc));
  to_split->safety = 0;
  new_gp_bloc->next = to_split->next;
  new_gp_bloc->prev = to_split;
  to_split->next = new_gp_bloc;
  if (new_gp_bloc->next != NULL)
  {
    new_gp_bloc->next->prev = new_gp_bloc;
  }
  return new_gp_bloc;
}

struct gp_bloc * replace(struct gp_bloc * blc, int size) //Replace les metas datas du gp_bloc blc dans son prorpre gp_bloc à size octets avant la fin de son gp_bloc
{
  struct gp_bloc * p = (struct gp_bloc*)((char*)blc + blc->size_to_malloc - blc->available - size);
  struct gp_bloc * blc_prev = blc->prev;
  struct gp_bloc * blc_next = blc->next;
  int blc_safety = blc->safety;
  int blc_size_to_malloc = blc->size_to_malloc;

  p->size_to_malloc = blc_size_to_malloc;
  p->available = blc_size_to_malloc - size;
  p->safety = blc_safety;
  p->next = blc_next;
  p->prev = blc_prev;
  p->donnee = (void*)((char*)p + sizeof(struct gp_bloc));
  if (blc == gp_cache)
  {
    gp_cache = p;
  }
  if (p->prev != NULL)
  {
    p->prev->next = p;
  }
  if (p->next != NULL)
  {
    p->next->prev = p;
  }
  return p;
}

//Essaye de fusionner deux gp_blocs consécutifs
void gp_try_to_merge_with_next (struct gp_bloc *to_merge)
{
  if (to_merge->next != NULL && to_merge + to_merge->size_to_malloc - to_merge->available == to_merge->next)
  {
    //Si le bloc suivant est contigu, on le merge
    to_merge->size_to_malloc += to_merge->next->size_to_malloc;
    to_merge->next = to_merge->next->next;
    if (to_merge->next != NULL)
    {
      to_merge->next->prev = to_merge;
    }
  }
}

/*
 * Pour vous donner une idée, vous avez ceci
 */
 //Initialisation du tas
void gp_init_heap(int size)
{
  int initial_size = max(gp_bloc_size(INITIAL_HEAP_SIZE), gp_bloc_size(size));
  sbrk(initial_size + sysconf(_SC_PAGE_SIZE));
	/*initialise the gp_cache*/
  gp_cache = (struct gp_bloc *)((char*) sbrk(0) - size - sysconf(_SC_PAGE_SIZE));
	gp_cache->size_to_malloc = initial_size;
	gp_cache->available = initial_size - size;
  gp_cache->state = 1;
	gp_cache->next = NULL;
	gp_cache->prev = NULL;
  gp_cache->safety = 1;
  gp_cache->donnee = (void*)((char*)gp_cache + sizeof(struct gp_bloc));
  /* initialise the gp_cache_mmap */
}

//Appels systemes dans create_...
struct gp_bloc_mmap * gp_create_bloc_mmap (int size)
{
  struct gp_bloc_mmap * p = gp_cache_mmap;
  while (p!= NULL && p->prev != NULL)
  {
    p = p->prev;
  }
  int min = -1;
  struct gp_bloc_mmap * blc = NULL;
  while (p != NULL)
  {
    if (p->state == 0 && p->total_size >= gp_bloc_size(sizeof(struct gp_bloc_mmap) + size) && (min == -1 || min > p->used_space))
    {
      blc = p;
      min = p->used_space;
    }
    p = p->next;
  }
  p = blc;
  if (p != NULL) //On utilise un gp_bloc_mmap précédemment libéré pour y mettre notre malloc
  {
    int p_total_size = p->total_size;
    struct gp_bloc_mmap * p_prev = p->prev;
    struct gp_bloc_mmap * p_next = p->next;
    blc = (struct gp_bloc_mmap*) ((char*)p + p->used_space - size - sizeof(struct gp_bloc_mmap));
    blc->used_space = size + sizeof(struct gp_bloc_mmap);
    blc->next = p_next;
    blc->prev = p_prev;
    if (blc->next != NULL)
    {
      blc->next->prev = blc;
    }
    if (blc->prev != NULL)
    {
      blc->prev->next = blc;
    }
    blc->state = 1;
    blc->total_size = p_total_size;
    blc->donnee = (void*)((char*)blc + sizeof(struct gp_bloc_mmap));
    return blc;
  }

  int real_size = gp_bloc_size(size + sizeof(struct gp_bloc_mmap));
  blc = (struct gp_bloc_mmap *)((char*) mmap(NULL, real_size + sysconf(_SC_PAGE_SIZE), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0) + real_size - size - sizeof(struct gp_bloc_mmap));
  blc->used_space = size + sizeof(struct gp_bloc_mmap);
  blc->total_size = real_size;
  blc->donnee = (void*)((char*)blc + sizeof(struct gp_bloc_mmap));
  blc->state = 1;
  mprotect((char*) blc + size + sizeof(struct gp_bloc_mmap), sysconf(_SC_PAGE_SIZE), PROT_NONE);
  if (gp_cache_mmap == NULL)
  {
    gp_cache_mmap = blc;
    blc->next = NULL;
    blc->prev = NULL;
  }
  else
  {
    struct gp_bloc_mmap * p = gp_cache_mmap;
    while (p->next != NULL)
    {
      p = p->next;
    }
    p->next = blc;
    blc->prev = p;
  }
  return blc;
}

//Attribution d'un nouveau gp_bloc pour y stocker une donnée de taille size, et renvoie un pointeur vers le début du gp_bloc (méta données).
//Au besoin, agrandit le tas et crée un nouveau gp_bloc
struct gp_bloc * gp_create_bloc (int size)
{
	if(gp_cache == NULL)
  {
		gp_init_heap(size + sizeof(struct gp_bloc));
    mprotect((char*)gp_cache + size + sizeof(struct gp_bloc), sysconf(_SC_PAGE_SIZE), PROT_NONE);
    return gp_cache;
  }
  int real_size = gp_bloc_size(size + sizeof(struct gp_bloc));
  struct gp_bloc* blc = gp_cache;
  while (blc->prev != NULL)
  {
    blc = blc->prev;
  }
  int min = -1;
  struct gp_bloc * p = blc;
  //On recherche le bloc libre d'espace libre le plus faible possible mais plus grand que celui nécessaire pour notre malloc
  while (p != NULL)
  {
    if (p->state == 0 && ((p->size_to_malloc >= real_size && p->safety == 1) || (p->safety == 0 && p->size_to_malloc >= real_size + sysconf(_SC_PAGE_SIZE))) && (min == -1 || min > p->size_to_malloc))
    {
      min = p->size_to_malloc;
      blc = p;
    }
    p = p->next;
  }
  if (min != -1)
  {
    if (blc->size_to_malloc - real_size <= 2*sysconf(_SC_PAGE_SIZE) && blc->safety == 1) //Dans ce cas, il est inutile de garder le gp_bloc restant séparé de celui qu'on va rajouter car on n'aura pas assez de place pour y insérer un nouveau gp_bloc
    {//Da,s ce cas on peut garder le meme bloc car le split va créer un bloc libre de moins de 2 pages, il n'aura donc pas assez de place pour acceuillir un malloc
      blc = replace(blc, size + sizeof(struct gp_bloc));
      blc->state = 1;
    }
    else if (blc->size_to_malloc - real_size <= 3*sysconf(_SC_PAGE_SIZE) && blc->safety == 0)
    {//Meme cas que précédent mais en prenant en compte qu'il faut une nouvelle page de garde
      blc = replace(blc, size + sizeof(struct gp_bloc) + sysconf(_SC_PAGE_SIZE));
      blc->state = 1;
      mprotect((char*)blc + size + sizeof(struct gp_bloc), sysconf(_SC_PAGE_SIZE), PROT_NONE);
    }
    else
    {//On split
      blc = gp_split(blc, size);
    }
  }
  else if (heap_reached_stack == 1) //Si le tas et la pile sont mêlés, il faut faire un bloc_mmap et non un bloc normal
  {
    heap_reached_stack = 0; //On remet à 0 car on peut possiblement avoir la place de mettre un plus petit bloc entre le plus bas bloc_mmap et le plus haut bloc
    return NULL;
  }
  else
  {
    while (blc->next != NULL)
    {
      blc = blc->next;
    }
    //On ajoute un nouveau bloc en bout de liste
    char * p = sbrk(0);
    sbrk(real_size + sysconf(_SC_PAGE_SIZE));
    blc->next = (struct gp_bloc *)(p + real_size - size - sizeof(struct gp_bloc));
    blc->next->prev = blc;
    blc = blc->next;
    blc->safety = 1;
    blc->size_to_malloc = real_size;
    blc->state = 1;
    blc->available = real_size - size - sizeof(struct gp_bloc);
    blc->donnee = (void*)((char*) blc + sizeof(struct gp_bloc));
    mprotect(p + real_size, sysconf(_SC_PAGE_SIZE), PROT_NONE);
 }
  return blc;
}

//Fonction malloc d'interface, pour éviter de renvoyer l'adresse vers les méta données
void * gp_malloc (int size)
{
  if (heap_reached_stack == 0 && plus_bas_mmap != NULL && (char*) sbrk(0) + gp_bloc_size(size + sizeof(struct gp_bloc)) + sysconf(_SC_PAGE_SIZE) - plus_bas_mmap > 0)
  {
    heap_reached_stack = 1;
  }
  if (size < MMAP_THRESHOLD && heap_reached_stack == 0)
  {
    struct gp_bloc * blc = gp_create_bloc(size);
    if (blc != NULL)
    {
      return blc->donnee;
    }
  }
  struct gp_bloc_mmap * blc = gp_create_bloc_mmap(size);
  char * data = (char*)blc->donnee;
  for (int i = 0; i < size; i++)
  {
    data[i] = 0;
  }
  if (plus_bas_mmap - (char*) blc + size < 0)
  {
    plus_bas_mmap = (char*) blc + size;
  }
  return blc->donnee;
}

//Libère le gp_bloc stockant la donnée pointée, et nettoie la mémoire en fusionnant les gp_blocs vides consécutifs et en désallouant l'extrêmité du tas si elle est inutilisée
void gp_free(void * ptr)
{
  if (ptr == NULL)
  {
    return;
  }
	struct gp_bloc * blc = gp_cache;
  while (blc->prev != NULL)
  {
    blc = blc->prev;
  }
  while (blc != NULL && blc->donnee != ptr)
  {
    blc = blc->next;
  }
  if (blc != NULL)
  {//On ramene les meta datas au debut du gp_bloc pour simplifier le split
    blc->state = 0;
    blc = replace(blc, blc->size_to_malloc);
    blc->state = 0;
    if (blc->safety == 0 && blc->next == (struct gp_bloc *)((char*) blc + blc->size_to_malloc - blc->available))
    {
      gp_try_to_merge_with_next(blc);
    }
    if (blc->prev != NULL && blc->prev->safety == 0 && blc == (struct gp_bloc *)((char*) blc->prev + blc->prev->size_to_malloc - blc->prev->available))
    {
      gp_try_to_merge_with_next(blc->prev);
    }
  }
  else
  {
    struct gp_bloc_mmap * mblc = gp_cache_mmap;
    while (mblc->prev != NULL)
    {
      mblc = mblc->prev;
    }
    while (mblc != NULL && mblc->donnee != ptr)
    {
      mblc = mblc->next;
    }
    if (mblc != NULL)
    {
      mblc->state = 0;
    }
  }
}

//Ajuste la taille du gp_bloc. Si suffisamment d'espace a été libéré dans le gp_bloc, crée un nouveau gp_bloc vide dans cet espace.
//Si le gp_bloc actuel s'avère insuffisant, déplace les données dans un autre gp_bloc, qu'il peut créer au besoin.
//Renvoie un pointeur vers les données après éventuel déplacement
void * gp_realloc(void * ptr, int size)
{
  struct gp_bloc * blc = gp_cache;
  while (blc->prev != NULL)
  {
    blc = blc->prev;
  }
  while (blc != NULL && blc->donnee != ptr)
  {
    blc = blc->next;
  }
  if (blc != NULL)
  {
    if (blc->size_to_malloc >= gp_bloc_size(size + sizeof(struct gp_bloc)))
    {
      //On déplace le pointeur dans le bloc
      char * data = (char*) blc->donnee;
      struct gp_bloc * blc_prev = blc->prev;
      struct gp_bloc * blc_next = blc->next;
      int blc_size_to_malloc = blc->size_to_malloc;
      char * new_data = (char*)blc + blc->size_to_malloc - blc->available - size;
      if (blc->size_to_malloc - blc->available - sizeof(struct gp_bloc) < size)
      {
        int max = blc->size_to_malloc - blc->available - sizeof(struct gp_bloc);
        for (int i = 0; i < max; i++)
        {
          new_data[i] = data[i];
        }
      }
      else
      {
        for (int i = size-1; i >= 0; i--)
        {
          new_data[i] = data[i];
        }
      }
      struct gp_bloc * new_blc = (struct gp_bloc *)(new_data - sizeof(struct gp_bloc));
      new_blc->prev = blc_prev;
      new_blc->next = blc_next;
      if (new_blc->prev != NULL)
      {
        new_blc->prev->next = new_blc;
      }
      if (new_blc->next != NULL)
      {
        new_blc->next->prev = new_blc;
      }
      new_blc->size_to_malloc = blc_size_to_malloc;
      new_blc->available = blc_size_to_malloc - size - sizeof(struct gp_bloc);
      new_blc->state = 1;
      new_blc->safety = 1;
      new_blc->donnee = (void*) new_data;
      return new_blc->donnee;
    }
    else
    {
      char * old_data = (char*) blc->donnee;
      int old_size = blc->size_to_malloc - blc->available - sizeof(struct gp_bloc);
      char * new_data = (char*) gp_malloc(size);
      for (int i = 0; i < old_size; i++)
      {
        new_data[i] = old_data[i];
      }
      gp_free(old_data);
      return (void*) new_data;
    }
  }
  else
  {
    struct gp_bloc_mmap * mblc = gp_cache_mmap;
    while (mblc->prev != NULL)
    {
      mblc = mblc->prev;
    }
    while (mblc != NULL && mblc->donnee != ptr)
    {
      mblc = mblc->next;
    }
    if (mblc != NULL)
    {
      if (mblc->total_size >= size)
      {
        char * data = (char*) mblc->donnee;
        struct gp_bloc_mmap * mblc_prev = mblc->prev;
        struct gp_bloc_mmap * mblc_next = mblc->next;
        int mblc_total_size = mblc->total_size;
        char * new_data = (char*)mblc + mblc->used_space - size;
        if (mblc->used_space - sizeof(struct gp_bloc_mmap) < size)
        {
          int max = mblc->used_space - sizeof(struct gp_bloc_mmap);
          for (int i = 0; i < max; i++)
          {
            new_data[i] = data[i];
          }
        }
        else
        {
          for (int i = size-1; i >= 0; i--)
          {
            new_data[i] = data[i];
          }
        }
        struct gp_bloc_mmap * new_mblc = (struct gp_bloc_mmap *)(new_data - sizeof(struct gp_bloc_mmap));
        new_mblc->prev = mblc_prev;
        new_mblc->next = mblc_next;
        if (new_mblc->prev != NULL)
        {
          new_mblc->prev->next = new_mblc;
        }
        if (new_mblc->next != NULL)
        {
          new_mblc->next->prev = new_mblc;
        }
        new_mblc->total_size = mblc_total_size;
        new_mblc->used_space = size + sizeof(struct gp_bloc_mmap);
        new_mblc->state = 1;
        new_mblc->donnee = (void*) new_data;
        return new_mblc->donnee;
      }
      //Sinon, on malloc et on recopie les données
      char * data = (char*) mblc->donnee;
      int old_size = mblc->total_size - sizeof(struct gp_bloc_mmap);
      char * new_data = (char*) gp_malloc (size);
      int m = size;
      if (old_size < size)
      {
        m = old_size;
      }
      for (int i = 0; i < m; i++)
      {
        new_data[i] = data[i];
      }
      gp_free(ptr);
      return (void*) new_data;
    }
    else
    {
      return NULL;
    }
  }
}

#endif
