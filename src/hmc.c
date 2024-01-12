#ifndef HMCC
#define HMCC

#include "hmc.h"

struct bloc * c_cache = NULL;
struct bloc_mmap * c_cache_mmap = NULL;


//Split d'un bloc. S'il a suffisamment de place pour créer un autre bloc, le fait, sinon renseigne la place excédentaire comme utilisable par le bloc.
//Retourne un pointeur vers le bloc qu'on a divisé
void * c_split (struct bloc *to_split, int size)
{
  if (to_split->state == 0) //Dans ce cas on ne sépare pas mais on remplace
  {
    to_split->available -= sizeof(struct bloc) + size;
    to_split->donnee = (void*)((char*) to_split + sizeof(struct bloc));
    to_split->state = 1;
    return to_split;
  }
  //Sinon, on créé un nouveau bloc et on le colle à la fin de to_split (il récupère alors ce qui reste de libre)
  struct bloc * new_bloc = (struct bloc *)((char*) to_split + to_split->size_to_malloc - to_split->available);
  new_bloc->prev = to_split;
  new_bloc->next = to_split->next;
  if (to_split->next != NULL)
  {
    to_split->next->prev = new_bloc;
  }
  new_bloc->available = to_split->available - size - sizeof(struct bloc);
  new_bloc->size_to_malloc = to_split->available;
  to_split->size_to_malloc -= to_split->available;
  to_split->available = 0;
  new_bloc->state = 1;
  new_bloc->donnee = (void *)((char*) new_bloc + sizeof(struct bloc));
  to_split->next = new_bloc;
	return new_bloc;
}

//Essaye de fusionner deux blocs consécutifs
void c_try_to_merge_with_next (struct bloc *to_merge)
{
  if (to_merge->next != NULL && (char*)to_merge + to_merge->size_to_malloc == (char*) to_merge->next)
  {//Aprèes avoir vérifié que le voisin existe et est collé au bloc à merge, on merge
    to_merge->size_to_malloc += to_merge->next->size_to_malloc + sizeof(struct bloc);
    to_merge->available += to_merge->next->size_to_malloc + sizeof(struct bloc);
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
void c_init_heap(int size)
{
	c_cache = sbrk(0); //move to the current location of the program break
  int initial_size = max(size, INITIAL_HEAP_SIZE);
  sbrk(initial_size);
	/*initialise the cache*/
	c_cache->size_to_malloc = initial_size;
	c_cache->available = initial_size - size;
  c_cache->state = 1;
	c_cache->next = NULL;
	c_cache->prev = NULL;
  c_cache->donnee = NULL;
  /* initialise the cache_mmap */
}

//Appels systemes dans create_...
struct bloc_mmap * c_create_bloc_mmap (int size)
{ //params: mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)
  struct bloc_mmap * p = c_cache_mmap;
  while (p!= NULL && p->prev != NULL)
  {
    p = p->prev;
  }
  while (p != NULL)
  {
    if (p->state == 0 && p->size_usable >= size)
    {
      break;
    }
    p = p->next;
  }
  if (p != NULL)
  {
    p->state = 1;
    return p;
  }

  struct bloc_mmap * blc = mmap(NULL, size + sizeof(struct bloc_mmap), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  blc->size_usable = size;
  blc->donnee = (void*) ((char*)blc + sizeof(struct bloc_mmap));
  blc->state = 1;
  blc->next = NULL;
  if (c_cache_mmap == NULL)
  {
    c_cache_mmap = blc;
  }
  else
  {
    struct bloc_mmap * p = c_cache_mmap;
    while (p->next != NULL)
    {
      p = p->next;
    }
    p->next = blc;
    blc->prev = p;
  }
  return blc;
}

//Attribution d'un nouveau bloc pour y stocker une donnée de taille size, et renvoie un pointeur vers le début du bloc (méta données).
//Au besoin, agrandit le tas et crée un nouveau bloc
struct bloc * c_create_bloc (int size)
{
	if(c_cache == NULL)
  {
		c_init_heap(size + sizeof(struct bloc));
    c_cache->donnee = (void*)((char*)c_cache + sizeof(struct bloc));
    return c_cache;
  }
  struct bloc* blc = c_cache;
  while (blc->prev != NULL)
  {
    blc = blc->prev;
  }
  int min = -1;
  struct bloc * p = blc;
  while (p != NULL) //On recherche si il existe un bloc ayant suffisamment de place libre pour acceullit le nouveau et on prend le min
  {
    if (p->available >= size && (min == -1 || min >= p->available))
    {
      min = p->available;
      blc = p;
    }
    p = p->next;
  }
  if (min != -1) //Si on en a trouvé un, on le split
  {
    blc = c_split(blc, size);
  }
  else //Sinon, on créé un nouveau bloc
  {
    while (blc->next != NULL)
    {
      blc = blc->next;
    }
    blc->next = sbrk(0);
    sbrk(size + sizeof(struct bloc));
    blc->next->prev = blc;
    blc = blc->next;
    blc->size_to_malloc = size + sizeof(struct bloc);
    blc->state = 1;
    blc->available = 0;
    blc->donnee = (void*)((char*) blc + sizeof(struct bloc));
  }
  return blc;
}

//Fonction malloc d'interface, pour éviter de renvoyer l'adresse vers les méta données
void * rmalloc (int size)
{
  if (heap_reached_stack == 0 && plus_bas_mmap != NULL && (char*) sbrk(0) + size + sizeof(struct bloc) - plus_bas_mmap > 0) //En vérité il faudrait garder la possibilité de combler des blocs libérés, mais a voir après avoir débuggé le reste
  {
    heap_reached_stack = 1;
  }
  if (size < MMAP_THRESHOLD)
  {
    struct bloc * blc = c_create_bloc(size);
    return blc->donnee;
  }
  else
  {
    struct bloc_mmap * blc = c_create_bloc_mmap(size);
    if (plus_bas_mmap - (char*) blc + size < 0)
    {
      plus_bas_mmap = (char*) blc + size;
    }
    return blc->donnee;
  }
}

void * c_malloc (int size) //Malloc interface des canaries
{
  void * res = rmalloc(size + 1);
  *((char*) res + size) = 111;
  return res;
}

//Libère le bloc stockant la donnée pointée, et nettoie la mémoire en fusionnant les blocs vides consécutifs et en désallouant l'extrêmité du tas si elle est inutilisée
void c_free(void * ptr)
{
  if (ptr == NULL)
  {
    return;
  }
  //On cherche le pointeur à libérer dans les blocs
	struct bloc * blc = c_cache;
  while (blc->prev != NULL)
  {
    blc = blc->prev;
  }
  while (blc != NULL && blc->donnee != ptr) //On recherche le pointeur dans les blocs normaux
  {
    blc = blc->next;
  }
  if (blc != NULL)
  {
    if (*((char*)blc + blc->size_to_malloc - blc->available - 1) != 111) //Dans ce cas on a dépassé le buffer alloué
    {//On vérifie que la valeur des canary n'a pas été modifiée
      raise(SIGSEGV);
      return;
    }
    //On le marque comme libéré puis si ses voisins sont libres on essaie de les merge
    blc->state = 0;
    blc->available = blc->size_to_malloc;
    if (blc->next != NULL && blc->next->state == 0)
    {
      c_try_to_merge_with_next(blc);
    }
    if (blc->prev != NULL && blc->prev->state == 0)
    {
      blc = blc->prev;
      c_try_to_merge_with_next(blc);
    }
  }
  else
  {//On cherche dans les blocs mmap puis on marque comme libéré
    struct bloc_mmap * mblc = c_cache_mmap;
    while (mblc->prev != NULL)
    {
      mblc = mblc->prev;
    }
    while (mblc != NULL && mblc->donnee != ptr) //On recherche dans les blocs mmap
    {
      mblc = mblc->next;
    }
    if (mblc != NULL)
    {
      mblc->state = 0;
      if (*(((char*)mblc->donnee + mblc->size_usable - 1)) != 111)
      {
        raise(SIGSEGV);
        return;
      }
    }
  }
}

//Ajuste la taille du bloc. Si suffisamment d'espace a été libéré dans le bloc, crée un nouveau bloc vide dans cet espace.
//Si le bloc actuel s'avère insuffisant, déplace les données dans un autre bloc, qu'il peut créer au besoin.
//Renvoie un pointeur vers les données après éventuel déplacement
void * c_realloc(void * ptr, int size)
{
  //On cherche le pointeur dans les blocs
  struct bloc * blc = c_cache;
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
    if (blc->size_to_malloc - sizeof(struct bloc) >= size) //Dans ce cas on a de la place dans notre bloc, inutile de changer de place le pointeur
    {
      blc->available = blc->size_to_malloc - sizeof(struct bloc) - size;
      return blc->donnee;
    }
    else
    {
      //On commence par rechercher la place réellement libre en regardant les blocs contigus suivants
      struct bloc * p = blc;
      int total_free_space = blc->size_to_malloc - sizeof(struct bloc);
      p = p->next;
      while (p != NULL && p->state == 0 && (struct bloc *)((char*)p->prev + p->prev->size_to_malloc) == p)
      {
        total_free_space += p->size_to_malloc;
        p = p->next;
      }
      if (total_free_space >= size) //Dans ce cas on peut garder notre pointeur en court-circuitant les blocs contigus suivants
      {
        blc->state = 1;
        blc->size_to_malloc = total_free_space + sizeof(struct bloc);
        blc->available = total_free_space - size;
        blc->next = p;
        if (p != NULL)
        {
          p->prev = blc;
        }
        return blc->donnee;
      }
      else //Dans ce cas on doit déplacer le pointeur en recréant un bloc
      {
        char * data = (char*) blc->donnee;
        int old_size = blc->size_to_malloc - blc->available - sizeof(struct bloc);
        char * new_data = (char*) c_malloc(size);
        int m = size;
        if (old_size < size)
        {
          m = old_size;
        }
        for (int i = 0; i < m; i++) //On copie les données
        {
          new_data[i] = data[i];
        }
        c_free(ptr);
        return (void*) new_data;
      }
    }
  }
  else //De meme pour les mmap
  {
    struct bloc_mmap * mblc = c_cache_mmap;
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
      if (mblc->size_usable >= size)
      {
        return mblc->donnee;
      }
      char * data = (char*) mblc->donnee;
      int old_size = mblc->size_usable;
      char * new_data = (char*) c_malloc(size);
      int m = size;
      if (old_size < size)
      {
        m = old_size;
      }
      for (int i = 0; i < m; i++)
      {
        new_data[i] = data[i];
      }
      c_free(ptr);
      return (void*) new_data;
    }
    else
    {
      return NULL;
    }
  }
}

#endif
