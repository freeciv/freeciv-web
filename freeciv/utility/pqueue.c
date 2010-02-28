/**********************************************************************
 Freeciv - Copyright (C) 2002 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

/***********************************************************************
  Implementation of a priority queue aka heap.

  Currently only one value-type is supported.
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include "mem.h"

#include "pqueue.h"

struct pqueue {
  int size;			/* number of occupied cells */
  int avail;			/* total number of cells */
  int step;			/* additional memory allocation step */
  pq_data_t *cells;		/* array containing data */
  int *priorities;		/* backup priorities (in case data is changed) */
};

/**********************************************************************
  Initialize the queue.
 
  initial_size is the numer of queue items for which memory should be
  preallocated, that is, the initial size of the item array the queue
  uses. If you insert more than n items to the queue, another n items
  will be allocated automatically.
***********************************************************************/
struct pqueue *pq_create(int initial_size)
{
  struct pqueue *q = fc_malloc(sizeof(struct pqueue));

  q->cells = fc_malloc(sizeof(pq_data_t) * initial_size);
  q->priorities = fc_malloc(sizeof(int) * initial_size);
  q->avail = initial_size;
  q->step = initial_size;
  q->size = 1;
  return q;
}

/********************************************************************
  Destructor for queue structure.
********************************************************************/
void pq_destroy(struct pqueue *q)
{
  assert(q != NULL);
  free(q->cells);
  free(q->priorities);
  free(q);
}

/********************************************************************
  Insert an item into the queue.
*********************************************************************/
void pq_insert(struct pqueue *q, pq_data_t datum, int datum_priority)
{
  int i;

  assert(q != NULL);

  /* allocate more memory if necessary */
  if (q->size >= q->avail) {
    int newsize = q->size + q->step;

    q->cells = fc_realloc(q->cells, sizeof(pq_data_t) * newsize);
    q->priorities = fc_realloc(q->priorities, sizeof(int) * newsize);
    q->avail = newsize;
  }

  /* insert item */
  i = q->size++;
  while (i > 1 && q->priorities[i / 2] < datum_priority) {
    q->cells[i] = q->cells[i / 2];
    q->priorities[i] = q->priorities[i / 2];
    i /= 2;
  }
  q->cells[i] = datum;
  q->priorities[i] = datum_priority;
}

/*******************************************************************
  Remove the highest-ranking item from the queue and store it in
  dest. dest maybe NULL.
 
  Return value:
     TRUE   The value of the item that has been removed.
     FALSE  No item could be removed, because the queue was empty.
*******************************************************************/
bool pq_remove(struct pqueue * q, pq_data_t *dest)
{
  pq_data_t tmp;
  int tmp_priority;
  pq_data_t top;
  int i = 1;

  assert(q != NULL);

  if (q->size == 1) {
    return FALSE;
  }

  assert(q->size <= q->avail);
  top = q->cells[1];
  q->size--;
  tmp = q->cells[q->size];
  tmp_priority = q->priorities[q->size];
  while (i <= q->size / 2) {
    int j = 2 * i;
    if (j < q->size && q->priorities[j] < q->priorities[j + 1]) {
      j++;
    }
    if (q->priorities[j] <= tmp_priority) {
      break;
    }
    q->cells[i] = q->cells[j];
    q->priorities[i] = q->priorities[j];
    i = j;
  }
  q->cells[i] = tmp;
  q->priorities[i] = tmp_priority;
  if(dest) {
      *dest = top;
  }
  return TRUE;
}

/*********************************************************************
  Store the highest-ranking item in dest without removing it
 
  Return values:
     TRUE   dest was set.
     FALSE  The queue is empty.
**********************************************************************/
bool pq_peek(struct pqueue *q, pq_data_t * dest)
{
  assert(q != NULL);
  if (q->size == 1) {
    return FALSE;
  }

  *dest = q->cells[1];
  return TRUE;
}
