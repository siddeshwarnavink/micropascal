/* clomy.h v1.0 - C library of my own - Siddeshwar

   A header-only universal C library.

   Features:
     1. Arena
     2. Dynamic array
     3. Hash table
     4. String builder

   To use this library:
     #define CLOMY_IMPLEMENTATION
     #include "clomy.h"

   To add "clomy_" namespace prefix:
     #define CLOMY_NO_SHORT_NAMES */

#ifndef CLOMY_H
#define CLOMY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef CLOMY_ARENA_CAPACITY
#define CLOMY_ARENA_CAPACITY (8 * 1024)
#endif /* not CLOMY_ARENA_CAPACITY */

#ifndef CLOMY_STRINGBUILDER_CAPACITY
#define CLOMY_STRINGBUILDER_CAPACITY 1024
#endif /* not CLOMY_STRINGBUILDER_CAPACITY */

#define CLOMY_ALIGN_UP(n, a) (((n) + ((a) - 1)) & ~((a) - 1))

#define CLOMY_ALLOC_MAGIC 0x00636E6B

struct clomy_arfree_block
{
  unsigned int size;
  struct clomy_arfree_block *next;
};
typedef struct clomy_arfree_block clomy_arfree_block;

struct clomy_archunk
{
  unsigned int size;
  unsigned int capacity;
  clomy_arfree_block *free_list;
  struct clomy_archunk *next;
  unsigned char data[];
};
typedef struct clomy_archunk clomy_archunk;

struct clomy_aralloc_hdr
{
  struct clomy_archunk *cnk;
  unsigned int size;
  unsigned int magic;
};
typedef struct clomy_aralloc_hdr clomy_aralloc_hdr;

struct clomy_arena
{
  clomy_archunk *head, *tail;
};
typedef struct clomy_arena clomy_arena;

clomy_archunk *_clomy_newarchunk (unsigned int size);

clomy_arfree_block *_clomy_find_free_block (clomy_archunk *cnk,
                                            unsigned int needed_size,
                                            clomy_arfree_block **prev_ptr);

void _clomy_remove_free_block (clomy_archunk *cnk, clomy_arfree_block *block,
                               clomy_arfree_block *prev);

void _clomy_add_free_block (clomy_archunk *cnk, clomy_arfree_block *new_block);

/* Allocate memory inside arena. */
void *clomy_aralloc (clomy_arena *ar, unsigned int size);

/* Free the memory chunk inside arena. */
void clomy_arfree (void *value);

/* Free the entire arena. */
void clomy_arfold (clomy_arena *ar);

/* Print arena debug info. */
void clomy_ardebug (clomy_arena *ar);

/*----------------------------------------------------------------------*/

struct clomy_da
{
  clomy_arena *ar;
  void *data;
  unsigned data_size;
  unsigned int size;
  unsigned int capacity;
};
typedef struct clomy_da clomy_da;

/* Initialize the dynamic array in arena. */
int clomy_dainit (clomy_da *da, clomy_arena *ar, unsigned int data_size,
                  unsigned int capacity);

/* Initialize the dynamic array in heap.*/
int clomy_dainit2 (clomy_da *da, unsigned int data_size,
                   unsigned int capacity);

/* Set the capacity of dynamic array. */
int clomy_dacap (clomy_da *da, unsigned int capacity);

/* Doubles the capacity of the array. */
int clomy_dagrow (clomy_da *da);

/* Get Ith element of dynamic array. */
void *clomy_dageti (clomy_da *da, unsigned int i);

/* Append data at the end of dynamic array. */
int clomy_daappend (clomy_da *da, void *data);

/* Append data at the start of dynamic array.*/
int clomy_dapush (clomy_da *da, void *data);

/* Insert data at Ith position of dynamic array. */
int clomy_dainsert (clomy_da *da, void *data, unsigned int i);

/* Delete data at Ith position of dynamic array. */
void clomy_dadel (clomy_da *da, unsigned int i);

/* Delete data at front of the dynamic array and returns the data. */
void *clomy_dapop (clomy_da *da);

/* Get Ith element of dynamic array. */
void *clomy_dageti (clomy_da *da, unsigned int i);

/* Free the dynamic array. */
void clomy_dafold (clomy_da *da);

/*----------------------------------------------------------------------*/

struct clomy_htdata
{
  int key;
  struct clomy_htdata *next;
  unsigned char data[];
};
typedef struct clomy_htdata clomy_htdata;

struct clomy_stdata
{
  char *key;
  struct clomy_stdata *next;
  unsigned char data[];
};
typedef struct clomy_stdata clomy_stdata;

struct clomy_ht
{
  clomy_arena *ar;
  void **data;
  unsigned int data_size;
  unsigned int size;
  unsigned int capacity;
  unsigned int a;
};
typedef struct clomy_ht clomy_ht;

unsigned int _clomy_hash_int (clomy_ht *ht, unsigned int x);

unsigned int _clomy_hash_str (clomy_ht *ht, char *x);

/* Initialize hash table with either unsigned integer or string key. */
int clomy_htinit (clomy_ht *ht, clomy_arena *ar, unsigned int capacity,
                  unsigned int dsize);

/* Initialize hash table with either unsigned integer or string key in heap. */
int clomy_htinit2 (clomy_ht *ht, unsigned int capacity, unsigned int dsize);

/* Put value in unsigned integer key in hash table. */
int clomy_htput (clomy_ht *ht, int key, void *value);

/* Put value in string key in hash table. */
int clomy_stput (clomy_ht *ht, char *key, void *value);

/* Get value for unsigned integer key hash table. */
void *clomy_htget (clomy_ht *ht, int key);

/* Get value for string key hash table. */
void *clomy_stget (clomy_ht *ht, char *key);

/* Delete unsigned integer key from hash table. */
void clomy_htdel (clomy_ht *ht, int key);

/* Delete string key from hash table. */
void clomy_stdel (clomy_ht *ht, char *key);

/* Free the hash table with unsigned integer key. */
void clomy_htfold (clomy_ht *ht);

/* Free the hash table with string key. */
void clomy_stfold (clomy_ht *ht);

/*----------------------------------------------------------------------*/

struct clomy_sbchunk
{
  unsigned int size;
  unsigned int capacity;
  struct clomy_sbchunk *next;
  char data[];
};
typedef struct clomy_sbchunk clomy_sbchunk;

struct clomy_stringbuilder
{
  clomy_arena *ar;
  unsigned int size;
  clomy_sbchunk *head, *tail;
};
typedef struct clomy_stringbuilder clomy_stringbuilder;

/* Initialize string builder. */
void clomy_sbinit (clomy_stringbuilder *sb, clomy_arena *ar);

/* Initialize string builder in heap. */
void clomy_sbinit2 (clomy_stringbuilder *sb);

/* Append string to the end of string builder. */
int clomy_sbappend (clomy_stringbuilder *sb, char *val);

/* Append character to the end of string builder. */
int clomy_sbappendch (clomy_stringbuilder *sb, char val);

/* Insert string at Ith position of string builder. */
int clomy_sbinsert (clomy_stringbuilder *sb, char *val, unsigned int i);

/* Push string to the beginning of string builder. */
int clomy_sbpush (clomy_stringbuilder *sb, char *val);

/* Push character to the beginning of string builder. */
int clomy_sbpushch (clomy_stringbuilder *sb, char val);

/* Reverse the string. */
void clomy_sbrev (clomy_stringbuilder *sb);

/* Returns the constructed string and flushes the string builder. */
char *clomy_sbflush (clomy_stringbuilder *sb);

/* Reset the string builder. */
void clomy_sbreset (clomy_stringbuilder *sb);

/* Free the string builder. */
void clomy_sbfold (clomy_stringbuilder *sb);

/*----------------------------------------------------------------------*/

#ifndef CLOMY_NO_SHORT_NAMES

#define arena clomy_arena
#define archunk clomy_archunk
#define aralloc clomy_aralloc
#define arfree clomy_arfree
#define arfold clomy_arfold
#define ardebug clomy_ardebug

#define da clomy_da
#define dainit clomy_dainit
#define dainit2 clomy_dainit2
#define dageti clomy_dageti
#define dapush clomy_dapush
#define daappend clomy_daappend
#define dainsert clomy_dainsert
#define dadel clomy_dadel
#define dapop clomy_dapop
#define dafold clomy_dafold

#define ht clomy_ht
#define htdata clomy_htdata
#define htinit clomy_htinit
#define htinit2 clomy_htinit2
#define htput clomy_htput
#define stput clomy_stput
#define htget clomy_htget
#define stget clomy_stget
#define htdel clomy_htdel
#define stdel clomy_stdel
#define htfold clomy_htfold
#define stfold clomy_stfold

#define stringbuilder clomy_stringbuilder
#define sbinit clomy_sbinit
#define sbinit2 clomy_sbinit2
#define sbappend clomy_sbappend
#define sbappendch clomy_sbappendch
#define sbinsert clomy_sbinsert
#define sbpush clomy_sbpush
#define sbpushch clomy_sbpushch
#define sbrev clomy_sbrev
#define sbflush clomy_sbflush
#define sbfold clomy_sbfold
#define sbreset clomy_sbreset

#endif /* not CLOMY_NO_SHORT_NAMES */

#ifdef CLOMY_IMPLEMENTATION

clomy_archunk *
_clomy_newarchunk (unsigned int size)
{
  unsigned int cnksize = sizeof (clomy_archunk) + size;
  clomy_archunk *cnk = (clomy_archunk *)malloc (cnksize);
  if (!cnk)
    return (clomy_archunk *)0;

  cnk->size = 0;
  cnk->capacity = size;
  cnk->next = (void *)0;
  cnk->free_list = (void *)0;

  return cnk;
}

clomy_arfree_block *
_clomy_find_free_block (clomy_archunk *cnk, unsigned int needed_size,
                        clomy_arfree_block **prev_ptr)
{
  clomy_arfree_block *prev = (void *)0;
  clomy_arfree_block *current = cnk->free_list;

  while (current)
    {
      if (current->size >= needed_size)
        {
          if (prev_ptr)
            *prev_ptr = prev;
          return current;
        }
      prev = current;
      current = current->next;
    }

  if (prev_ptr)
    *prev_ptr = (void *)0;
  return (void *)0;
}

void
_clomy_remove_free_block (clomy_archunk *cnk, clomy_arfree_block *block,
                          clomy_arfree_block *prev)
{
  if (prev)
    prev->next = block->next;
  else
    cnk->free_list = block->next;
}

void
_clomy_add_free_block (clomy_archunk *cnk, clomy_arfree_block *new_block)
{
  clomy_arfree_block *prev = (void *)0;
  clomy_arfree_block *current = cnk->free_list;

  while (current && current < new_block)
    {
      prev = current;
      current = current->next;
    }

  new_block->next = current;
  if (prev)
    prev->next = new_block;
  else
    cnk->free_list = new_block;

  if (current && ((char *)new_block + new_block->size) == (char *)current)
    {
      new_block->size += current->size;
      new_block->next = current->next;
    }

  if (prev && ((char *)prev + prev->size) == (char *)new_block)
    {
      prev->size += new_block->size;
      prev->next = new_block->next;
    }
}

void *
clomy_aralloc (clomy_arena *ar, unsigned int size)
{
  clomy_archunk *cnk;
  clomy_aralloc_hdr *hdr;
  clomy_arfree_block *free_blk, *prev_free, *rem;
  unsigned int cnk_size;
  const unsigned int hdr_size = CLOMY_ALIGN_UP (sizeof (clomy_aralloc_hdr), 8);

  size = CLOMY_ALIGN_UP (size, 8);

  /* Initialize arena */
  if (!ar->head)
    {
      ar->head = _clomy_newarchunk (CLOMY_ARENA_CAPACITY);
      if (!ar->head)
        return (void *)0;

      ar->tail = ar->head;
    }

  /* Trying first-fit exising chunk. */
  cnk = ar->head;
  cnk_size = size + hdr_size;
  cnk_size = CLOMY_ALIGN_UP (cnk_size, 8);

  while (cnk)
    {
      free_blk = _clomy_find_free_block (cnk, cnk_size, &prev_free);
      if (free_blk)
        {
          _clomy_remove_free_block (cnk, free_blk, prev_free);

          if (free_blk->size >= cnk_size + sizeof (clomy_arfree_block))
            {
              rem = (clomy_arfree_block *)((char *)free_blk + cnk_size);
              rem->size = free_blk->size - cnk_size;
              _clomy_add_free_block (cnk, rem);
            }

          hdr = (clomy_aralloc_hdr *)free_blk;
          hdr->cnk = cnk;
          hdr->size = size;
          hdr->magic = CLOMY_ALLOC_MAGIC;

          return (void *)((char *)hdr + hdr_size);
        }

      if (cnk->capacity - cnk->size >= cnk_size)
        {
          hdr = (clomy_aralloc_hdr *)(cnk->data + cnk->size);
          hdr->cnk = cnk;
          hdr->size = size;
          hdr->magic = CLOMY_ALLOC_MAGIC;

          cnk->size += cnk_size;
          return (void *)((char *)hdr + hdr_size);
        }

      cnk = cnk->next;
    }

  /* Allocate new memory. */
  cnk_size = cnk_size > CLOMY_ARENA_CAPACITY ? cnk_size : CLOMY_ARENA_CAPACITY;
  cnk = _clomy_newarchunk (cnk_size);
  if (!cnk)
    return (void *)0;

  cnk->size = cnk_size;

  ar->tail->next = cnk;
  ar->tail = cnk;

  hdr = (clomy_aralloc_hdr *)cnk->data;
  hdr->cnk = cnk;
  hdr->size = size;
  hdr->magic = CLOMY_ALLOC_MAGIC;

  return (void *)((char *)hdr + hdr_size);
}

void
clomy_arfree (void *value)
{
  clomy_archunk *cnk;
  clomy_aralloc_hdr *hdr;
  clomy_arfree_block *blk;
  const unsigned int hdr_size = CLOMY_ALIGN_UP (sizeof (clomy_aralloc_hdr), 8);

  if (!value)
    return;

  hdr = (clomy_aralloc_hdr *)((char *)value - hdr_size);
  if (hdr->magic != CLOMY_ALLOC_MAGIC)
    return;

  cnk = hdr->cnk;
  /* cnk->size -= hdr->size + hdr_size; */

  hdr->magic = 0;

  blk = (clomy_arfree_block *)hdr;
  blk->size = hdr->size + hdr_size;
  blk->next = (void *)0;

  _clomy_add_free_block (cnk, blk);
}

void
clomy_arfold (clomy_arena *ar)
{
  clomy_archunk *cnk = ar->head, *next;
  while (cnk)
    {
      next = cnk->next;
      free (cnk);
      cnk = next;
    }

  ar->head = (void *)0;
  ar->tail = (void *)0;
}

void
clomy_ardebug (clomy_arena *ar)
{
  clomy_archunk *cnk = ar->head;
  unsigned int chunk_num = 0;

  printf ("----------------------------------------\n");
  printf ("Arena Debug Information:\n");
  while (cnk)
    {
      printf ("Chunk %u: size=%u, capacity=%u\n", chunk_num, cnk->size,
              cnk->capacity);

      clomy_arfree_block *free_block = cnk->free_list;
      unsigned int free_num = 0;
      while (free_block)
        {
          printf ("  Free block %u: size=%u, addr=%p\n", free_num,
                  free_block->size, (void *)free_block);
          free_block = free_block->next;
          free_num++;
        }

      cnk = cnk->next;
      chunk_num++;
    }
  printf ("----------------------------------------\n");
}

/*----------------------------------------------------------------------*/

int
clomy_dainit (clomy_da *da, clomy_arena *ar, unsigned int data_size,
              unsigned int capacity)
{
  da->ar = ar;

  if (ar)
    da->data = clomy_aralloc (ar, data_size * capacity);
  else
    da->data = malloc (data_size * capacity);

  if (!da->data)
    return 1;

  da->data_size = data_size;
  da->size = 0;
  da->capacity = CLOMY_ALIGN_UP (capacity, 8);

  return 0;
}

int
clomy_dainit2 (clomy_da *da, unsigned int data_size, unsigned int capacity)
{
  return clomy_dainit (da, (void *)0, data_size, capacity);
}

int
clomy_dacap (clomy_da *da, unsigned int capacity)
{
  void *newarr;
  if (da->ar)
    {
      newarr = aralloc (da->ar, capacity * da->data_size);
      if (newarr)
        {
          /* TODO: Implement realloc in arena. */
          memcpy (newarr, da->data, da->capacity * da->data_size);
          arfree (da->data);
        }
    }
  else
    {
      newarr = realloc (da->data, capacity * da->data_size);
    }

  if (!newarr)
    return 1;

  da->data = newarr;
  da->capacity = capacity;

  return 0;
}

void *
clomy_dageti (clomy_da *da, unsigned int i)
{
  return da->data + i * da->data_size;
}

int
clomy_dagrow (clomy_da *da)
{
  if (da->size + 1 > da->capacity)
    return clomy_dacap (da, da->capacity * 2);
  return 0;
}

int
clomy_daappend (clomy_da *da, void *data)
{
  if (clomy_dagrow (da))
    return 1;

  memcpy ((char *)da->data + da->size * da->data_size, data, da->data_size);
  ++da->size;

  return 0;
}

int
clomy_dapush (clomy_da *da, void *data)
{
  if (clomy_dagrow (da))
    return 1;

  memmove ((char *)da->data + da->data_size, da->data,
           da->size * da->data_size);
  memcpy ((char *)da->data, data, da->data_size);
  ++da->size;

  return 0;
}

int
clomy_dainsert (clomy_da *da, void *data, unsigned int i)
{
  void *pos;

  if (i > da->size)
    return 1;

  if (clomy_dagrow (da))
    return 1;

  pos = da->data + i * da->data_size;
  memmove ((char *)pos + da->data_size, pos, (da->size - i) * da->data_size);
  memcpy ((char *)pos, data, da->data_size);
  ++da->size;

  return 0;
}

void
clomy_dadel (clomy_da *da, unsigned int i)
{
  void *pos = da->data + i * da->data_size;
  memmove (pos, pos + da->data_size,
           da->size * da->data_size - i * da->data_size);
  --da->size;
}

void *
clomy_dapop (clomy_da *da)
{
  void *data;
  if (da->ar)
    data = aralloc (da->ar, da->data_size);
  else
    data = malloc (da->data_size);

  memcpy (data, clomy_dageti (da, 0), da->data_size);
  clomy_dadel (da, 0);

  return data;
}

void
clomy_dafold (clomy_da *da)
{
  if (da->data)
    {
      if (da->ar)
        clomy_arfree (da->data);
      else
        free (da->data);
      da->data = (void *)0;
    }
}

/*----------------------------------------------------------------------*/

unsigned int
_clomy_hash_int (clomy_ht *ht, unsigned int x)
{
  unsigned int p = ht->a ^ x;
  return p % ht->capacity;
}

unsigned int
_clomy_hash_str (clomy_ht *ht, char *x)
{
  unsigned int hash = ht->a;
  int c;

  while ((c = *x++))
    hash = ((hash << 5) + hash) + c;

  return hash % ht->capacity;
}

int
clomy_htinit (clomy_ht *ht, clomy_arena *ar, unsigned int capacity,
              unsigned int dsize)
{
  unsigned int size;

  capacity = CLOMY_ALIGN_UP (capacity, 8);
  size = capacity * sizeof (clomy_htdata *);

  srand ((unsigned)time (NULL));
  ht->a = ((unsigned int)rand () << 1) | 1;

  ht->ar = ar;
  ht->data_size = dsize;
  ht->size = 0;
  ht->capacity = capacity;

  if (ar)
    ht->data = clomy_aralloc (ar, size);
  else
    ht->data = malloc (size);

  if (!ht->data)
    return 1;

  memset (ht->data, 0, size);

  return 0;
}

int
clomy_htinit2 (clomy_ht *ht, unsigned int capacity, unsigned int dsize)
{
  return clomy_htinit (ht, (void *)0, capacity, dsize);
}

int
clomy_htput (clomy_ht *ht, int key, void *value)
{
  clomy_htdata *data;
  unsigned int i = _clomy_hash_int (ht, key),
               size = sizeof (clomy_htdata) + ht->data_size;

  if (ht->ar)
    data = clomy_aralloc (ht->ar, size);
  else
    data = malloc (size);

  if (!data)
    return 1;

  data->key = key;
  data->next = (void *)0;
  memcpy (data->data, value, ht->data_size);

  if (!ht->data[i])
    {
      ht->data[i] = data;
    }
  else
    {
      data->next = ht->data[i];
      ht->data[i] = data;
    }
  ++ht->size;

  return 0;
}

int
clomy_stput (clomy_ht *ht, char *key, void *value)
{
  clomy_stdata *data;
  unsigned int i = _clomy_hash_str (ht, key),
               size = sizeof (clomy_stdata) + ht->data_size,
               keylen = strlen (key);

  if (ht->ar)
    data = clomy_aralloc (ht->ar, size);
  else
    data = malloc (size);

  if (!data)
    return 1;

  if (ht->ar)
    data->key = clomy_aralloc (ht->ar, keylen + 1);
  else
    data->key = malloc (keylen + 1);

  if (!data->key)
    return 1;

  strcpy (data->key, key);
  data->next = (void *)0;
  memcpy (data->data, value, ht->data_size);

  if (!ht->data[i])
    {
      ht->data[i] = data;
    }
  else
    {
      data->next = ht->data[i];
      ht->data[i] = data;
    }
  ++ht->size;

  return 0;
}

void *
clomy_htget (clomy_ht *ht, int key)
{
  clomy_htdata *ptr;
  unsigned int i = _clomy_hash_int (ht, key);

  ptr = ht->data[i];
  if (!ptr)
    return (void *)0;

  while (ptr)
    {
      if (ptr->key == key)
        return ptr->data;
      ptr = ptr->next;
    }

  return (void *)0;
}

void *
clomy_stget (clomy_ht *ht, char *key)
{
  clomy_stdata *ptr;

  unsigned int i = _clomy_hash_str (ht, key);

  ptr = ht->data[i];
  if (!ptr)
    return (void *)0;

  while (ptr)
    {
      if (strcmp (ptr->key, key) == 0)
        return ptr->data;
      ptr = ptr->next;
    }

  return (void *)0;
}

void
clomy_htdel (clomy_ht *ht, int key)
{
  clomy_htdata *ptr, *prev = (void *)0;
  unsigned int i = _clomy_hash_int (ht, key);

  ptr = ht->data[i];

  while (ptr)
    {
      if (ptr->key == key)
        {
          if (prev)
            prev->next = ptr->next;

          if (ht->ar)
            arfree (ptr);
          else
            free (ptr);

          if (!prev)
            ht->data[i] = ptr->next;

          --ht->size;
          break;
        }

      prev = ptr;
      ptr = ptr->next;
    }
}

void
clomy_stdel (clomy_ht *ht, char *key)
{
  clomy_stdata *ptr, *prev = (void *)0;
  unsigned int i = _clomy_hash_str (ht, key);

  ptr = ht->data[i];

  while (ptr)
    {
      if (strcmp (ptr->key, key) == 0)
        {
          if (prev)
            prev->next = ptr->next;

          if (ht->ar)
            {
              arfree (ptr->key);
              arfree (ptr);
            }
          else
            {
              free (ptr->key);
              free (ptr);
            }

          if (!prev)
            ht->data[i] = (void *)0;

          --ht->size;
          break;
        }

      prev = ptr;
      ptr = ptr->next;
    }
}

void
clomy_htfold (clomy_ht *ht)
{
  clomy_htdata *ptr, *next;
  unsigned int i;

  for (i = 0; i < ht->capacity; ++i)
    {
      ptr = ht->data[i];

      while (ptr)
        {
          next = ptr->next;

          if (ht->ar)
            arfree (ptr);
          else
            free (ptr);

          --ht->size;
          ptr = next;
        }
    }

  if (ht->ar)
    arfree (ht->data);
  else
    free (ht->data);
}

void
clomy_stfold (clomy_ht *ht)
{
  clomy_stdata *ptr, *next;
  unsigned int i;

  for (i = 0; i < ht->capacity; ++i)
    {
      ptr = ht->data[i];

      while (ptr)
        {
          next = ptr->next;

          if (ht->ar)
            {
              arfree (ptr->key);
              arfree (ptr);
            }
          else
            {
              free (ptr->key);
              free (ptr);
            }

          --ht->size;
          ptr = next;
        }
    }

  if (ht->ar)
    arfree (ht->data);
  else
    free (ht->data);
}

/*----------------------------------------------------------------------*/

clomy_sbchunk *
_clomy_newsbchunk (clomy_stringbuilder *sb, unsigned int capacity)
{
  clomy_sbchunk *cnk;
  unsigned int cnksize = sizeof (clomy_sbchunk) + capacity;

  if (sb->ar)
    cnk = (clomy_sbchunk *)clomy_aralloc (sb->ar, cnksize);
  else
    cnk = (clomy_sbchunk *)malloc (cnksize);

  if (!cnk)
    return (void *)0;

  cnk->size = 0;
  cnk->capacity = capacity;
  cnk->next = (void *)0;
  return cnk;
}

void
clomy_sbinit (clomy_stringbuilder *sb, clomy_arena *ar)
{
  sb->ar = ar;
  sb->size = 0;
}

void
clomy_sbinit2 (clomy_stringbuilder *sb)
{
  clomy_sbinit (sb, (void *)0);
}

int
clomy_sbappend (clomy_stringbuilder *sb, char *val)
{
  clomy_sbchunk *ptr;
  unsigned int i = 0, j = 0;

  if (!sb->head)
    {
      sb->head = _clomy_newsbchunk (sb, CLOMY_STRINGBUILDER_CAPACITY);
      if (!sb->head)
        return 1;

      sb->tail = sb->head;
    }

  ptr = sb->tail;

  do
    {
      if (j >= ptr->capacity)
        {
          if (!ptr->next)
            {
              ptr->next = _clomy_newsbchunk (sb, CLOMY_STRINGBUILDER_CAPACITY);
              if (!ptr->next)
                return 1;

              ptr = ptr->next;
            }
          else
            {
              ptr = ptr->next;
            }

          sb->tail = ptr;
        }

      j = ptr->size;

      while (val[i] != '\0' && j < ptr->capacity)
        {
          ptr->data[j++] = val[i++];
          ++ptr->size;
          ++sb->size;
        }
    }
  while (val[i] != '\0');

  return 0;
}

int
clomy_sbappendch (clomy_stringbuilder *sb, char val)
{
  clomy_sbchunk *ptr, *cnk;

  if (!sb->head)
    {
      sb->head = _clomy_newsbchunk (sb, CLOMY_STRINGBUILDER_CAPACITY);
      if (!sb->head)
        return 1;

      sb->tail = sb->head;
    }

  ptr = sb->tail;
  if (ptr->size + 1 > ptr->capacity)
    {
      cnk = _clomy_newsbchunk (sb, CLOMY_STRINGBUILDER_CAPACITY);
      if (!cnk)
        return 1;

      cnk->data[0] = val;
      cnk->size = 1;
      cnk->next = ptr->next;
      ptr->next = cnk;
      sb->tail = cnk;
    }
  else
    {
      ptr->data[ptr->size++] = val;
    }
  ++sb->size;

  return 0;
}

int
clomy_sbinsert (clomy_stringbuilder *sb, char *val, unsigned int i)
{
  clomy_sbchunk *ptr = sb->head, *prev, *cnk;
  int index = i;
  unsigned int len = strlen (val), offset;

  do
    {
      if (index - (int)ptr->size < 0)
        break;

      index -= ptr->size;
      ptr = ptr->next;
    }
  while (ptr);

  if (index == 0 || !ptr->next)
    {
      cnk = _clomy_newsbchunk (sb, len);
      if (!cnk)
        return 1;

      strncpy (cnk->data, val, len);
      cnk->size = len;
      cnk->next = ptr->next;

      ptr->next = cnk;
      sb->size += len;
    }
  else
    {
      prev = ptr;
      offset = ptr->size - index;

      cnk = _clomy_newsbchunk (sb, len - offset);
      if (!cnk)
        return 1;

      strncpy (cnk->data, &val[offset], len - offset);
      cnk->size = len - offset;
      cnk->next = ptr->next;
      ptr->next = cnk;
      ptr = cnk;

      cnk = _clomy_newsbchunk (sb, offset);
      if (!cnk)
        return 1;

      strncpy (cnk->data, &((char *)prev->data)[index], offset);
      cnk->size = offset;
      cnk->next = ptr->next;
      ptr->next = cnk;

      strncpy (&((char *)prev->data)[index], val, offset);
      sb->size += len;
    }

  return 0;
}

int
clomy_sbpush (clomy_stringbuilder *sb, char *val)
{
  clomy_sbchunk *cnk;
  unsigned int len = strlen (val);

  cnk = _clomy_newsbchunk (sb, len);
  if (!cnk)
    return 1;

  strncpy (cnk->data, val, len);

  cnk->size = len;
  cnk->next = sb->head;

  sb->head = cnk;
  sb->size += len;

  return 0;
}

int
clomy_sbpushch (clomy_stringbuilder *sb, char val)
{
  clomy_sbchunk *cnk;

  cnk = _clomy_newsbchunk (sb, 1);
  if (!cnk)
    return 1;

  cnk->data[0] = val;
  cnk->size = 1;
  cnk->next = sb->head;

  sb->head = cnk;
  ++sb->size;

  return 0;
}

void
clomy_sbrev (clomy_stringbuilder *sb)
{
  clomy_sbchunk *ptr = sb->head, *prev = (void *)0, *next;
  unsigned int a, b;
  char tmp;

  if (!ptr)
    return;

  sb->tail = sb->head;

  while (ptr)
    {
      a = 0;
      b = ptr->size - 1;
      while (b > a)
        {
          tmp = ptr->data[a];
          ptr->data[a] = ptr->data[b];
          ptr->data[b] = tmp;
          ++a;
          --b;
        }

      next = ptr->next;
      ptr->next = prev;
      prev = ptr;
      ptr = next;
    }

  sb->head = prev;
}

char *
clomy_sbflush (clomy_stringbuilder *sb)
{
  clomy_sbchunk *ptr = sb->head;
  unsigned int size = sb->size + 1, i, j = 0;
  char *str;

  if (!ptr)
    return (char *)0;

  if (sb->ar)
    str = clomy_aralloc (sb->ar, size);
  else
    str = malloc (size);

  if (!str)
    return (char *)0;

  while (ptr)
    {
      for (i = 0; i < ptr->size; ++i)
        {
          str[j++] = ptr->data[i];
          ptr->data[i] = 0;
        }

      ptr->size = 0;
      ptr = ptr->next;
    }

  clomy_sbreset (sb);

  str[j] = '\0';

  return str;
}

void
clomy_sbreset (clomy_stringbuilder *sb)
{
  sb->size = 0;
  sb->tail = sb->head;
}

void
clomy_sbfold (clomy_stringbuilder *sb)
{
  clomy_sbchunk *ptr = sb->head, *next;

  while (ptr)
    {
      next = ptr->next;

      if (sb->ar)
        clomy_arfree (ptr);
      else
        free (ptr);

      ptr = next;
    }

  sb->size = 0;
  sb->head = (void *)0;
  sb->tail = (void *)0;
}

#endif /* CLOMY_IMPLEMENTATION */

#endif /* not CLOMY_H */
