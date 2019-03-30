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

/* specpqs: "specific priority queues".
 *
 * This file is used to implement a "specific" priority queues.
 * That is, a (sometimes) type-checked priority queue. (Or at least a
 * priority queue with related functions with distinctly typed parameters.)
 *
 * Before including this file, you must define the following:
 *   SPECPQ_TAG - this tag will be used to form names for functions etc.
 *   SPECPQ_PRIORITY_TYPE - the type for the priority property of the cells.
 *   SPECPQ_DATA_TYPE - the type for the data property of the cells.
 *
 * Assuming SPECPQ_TAG were 'foo', SPECPQ_PRIORITY_TYPE were 'priority_t',
 * and SPECPQ_DATA_TYPE were 'data_t'.
 * including this file would provide a struct definition for:
 *    struct foo_pq;
 *
 * function typedefs:
 *    typedef void (*foo_pq_data_free_fn_t) (data_t);
 *
 * and prototypes for the following functions:
 *    struct foo_pq *foo_pq_new(int initial_size);
 *    void foo_pq_destroy(struct foo_pq *pq);
 *    void foo_pq_destroy_full(struct foo_pq *pq,
 *                             foo_pq_data_free_fn_t data_free);
 *    void foo_pq_insert(struct foo_pq *pq, data_t data,
 *                       priority_t priority);
 *    void foo_pq_replace(struct foo_pq *pq, data_t data,
 *                        priority_t priority);
 *    bool foo_pq_remove(struct foo_pq *pq, data_t *pdata);
 *    bool foo_pq_peek(const struct foo_pq *pq, data_t *pdata);
 *    bool foo_pq_priority(const struct foo_pq *pq, priority_t *ppriority);
 *
 * Note this is not protected against multiple inclusions; this is so that
 * you can have multiple different speclists. For each speclist, this file
 * should be included _once_, inside a .h file which _is_ itself protected
 * against multiple inclusions. */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* utility */
#include "mem.h"

#ifndef SPECPQ_TAG
#error Must define a SPECPQ_TAG to use this header
#endif
#ifndef SPECPQ_PRIORITY_TYPE
#error Must define a SPECPQ_PRIORITY_TYPE to use this header
#endif
#ifndef SPECPQ_DATA_TYPE
#error Must define a SPECPQ_DATA_TYPE to use this header
#endif

#define SPECPQ_PASTE_(x, y) x ## y
#define SPECPQ_PASTE(x, y) SPECPQ_PASTE_(x, y)

#define SPECPQ_PQ struct SPECPQ_PASTE(SPECPQ_TAG, _pq)
#define SPECPQ_PQ_ struct SPECPQ_PASTE(SPECPQ_TAG, _pq_private_)
#define SPECPQ_CELL_ struct SPECPQ_PASTE(SPECPQ_TAG, _cell_private_)
#define SPECPQ_FOO(suffix) SPECPQ_PASTE(SPECPQ_TAG, suffix)

/* Dummy type. Actually a SPECPQ_PQ_, and not defined anywhere. */
SPECPQ_PQ;

/* Function related typedefs. */
typedef void (*SPECPQ_FOO(_pq_data_free_fn_t)) (SPECPQ_DATA_TYPE);


/* Private. */
SPECPQ_CELL_ {
  SPECPQ_DATA_TYPE data;
  SPECPQ_PRIORITY_TYPE priority;
};

SPECPQ_PQ_ {
  int size;
  int avail;
  int step;
  SPECPQ_CELL_ *cells;
};

/****************************************************************************
  Build a new queue.
  'initial_size' is the numer of queue items for which memory should be
  preallocated, that is, the initial size of the item array the queue
  uses. If you insert more than n items to the queue, another n items
  will be allocated automatically.
****************************************************************************/
static inline SPECPQ_PQ *SPECPQ_FOO(_pq_new)(int initial_size)
{
  SPECPQ_PQ_ *pq = fc_malloc(sizeof(*pq));

  pq->cells = fc_malloc(sizeof(*pq->cells) * initial_size);
  pq->avail = initial_size;
  pq->step = initial_size;
  pq->size = 1;
  return (SPECPQ_PQ *) pq;
}

/****************************************************************************
  Destructor for queue structure.
****************************************************************************/
static inline void SPECPQ_FOO(_pq_destroy)(SPECPQ_PQ *_pq)
{
  SPECPQ_PQ_ *pq = (SPECPQ_PQ_ *) _pq;

  free(pq->cells);
  free(pq);
}

/****************************************************************************
  Alternative destructor for queue structure.
****************************************************************************/
static inline void
SPECPQ_FOO(_pq_destroy_full)(SPECPQ_PQ *_pq,
                             SPECPQ_FOO(_pq_data_free_fn_t) data_free)
{
  SPECPQ_PQ_ *pq = (SPECPQ_PQ_ *) _pq;
  int i;

  if (data_free != NULL) {
    for (i = 1; i < pq->size; i++) {
      data_free(pq->cells[i].data);
    }
  }
  free(pq->cells);
  free(pq);
}

/****************************************************************************
  Insert an item into the queue.
****************************************************************************/
static inline void SPECPQ_FOO(_pq_insert)(SPECPQ_PQ *_pq,
                                          SPECPQ_DATA_TYPE data,
                                          SPECPQ_PRIORITY_TYPE priority)
{
  SPECPQ_PQ_ *pq = (SPECPQ_PQ_ *) _pq;
  int i, j;

  /* Allocate more memory if necessary. */
  if (pq->size >= pq->avail) {
    int newsize = pq->size + pq->step;

    pq->cells = fc_realloc(pq->cells, sizeof(*pq->cells) * newsize);
    pq->avail = newsize;
  }

  /* Insert item. */
  i = pq->size++;
  while (i > 1 && (j = i / 2) && pq->cells[j].priority < priority) {
    pq->cells[i] = pq->cells[j];
    i = j;
  }
  pq->cells[i].data = data;
  pq->cells[i].priority = priority;
}

/****************************************************************************
  Set a better priority for datum. Insert if 'data' is not present yet.
****************************************************************************/
static inline void SPECPQ_FOO(_pq_replace)(SPECPQ_PQ *_pq,
                                           SPECPQ_DATA_TYPE data,
                                           SPECPQ_PRIORITY_TYPE priority)
{
  SPECPQ_PQ_ *pq = (SPECPQ_PQ_ *) _pq;
  int i, j;

  /* Lookup for 'data'... */
  for (i = pq->size - 1; i >= 1; i--) {
    if (pq->cells[i].data == data) {
      break;
    }
  }

  if (i == 0) {
    /* Not found, insert. */
    SPECPQ_FOO(_pq_insert)(_pq, data, priority);
  } else if (pq->cells[i].priority < priority) {
    /* Found, percolate-up. */
    while ((j = i / 2) && pq->cells[j].priority < priority) {
      pq->cells[i] = pq->cells[j];
      i = j;
    }
    pq->cells[i].data = data;
    pq->cells[i].priority = priority;
  }
}

/****************************************************************************
  Remove the highest-ranking item from the queue and store it in 'pdata'.
  'pdata' may be NULL. Return FALSE iff no item could be removed, because
  the queue was empty.
****************************************************************************/
static inline bool SPECPQ_FOO(_pq_remove)(SPECPQ_PQ *_pq,
                                          SPECPQ_DATA_TYPE *pdata)
{
  SPECPQ_PQ_ *pq = (SPECPQ_PQ_ *) _pq;
  SPECPQ_CELL_ tmp;
  SPECPQ_CELL_ *pcelli, *pcellj;
  SPECPQ_DATA_TYPE top;
  int i, j, s;

  if (pq->size == 1) {
    return FALSE;
  }

  fc_assert_ret_val(pq->size <= pq->avail, FALSE);
  top = pq->cells[1].data;
  pq->size--;
  tmp = pq->cells[pq->size];
  s = pq->size / 2;
  i = 1;
  pcelli = pq->cells + 1;
  while (i <= s) {
    j = 2 * i;
    pcellj = pq->cells + j;
    if (j < pq->size && pcellj->priority < pq->cells[j + 1].priority) {
      j++;
      pcellj++;
    }
    if (pcellj->priority <= tmp.priority) {
      break;
    }
    *pcelli = *pcellj;
    i = j;
    pcelli = pcellj;
  }
  *pcelli = tmp;
  if (pdata) {
    *pdata = top;
  }
  return TRUE;
}

/****************************************************************************
  Store the highest-ranking item in dest without removing it. Return FALSE
  if the queue is empty, in case 'pdata' is not set.
****************************************************************************/
static inline bool SPECPQ_FOO(_pq_peek)(const SPECPQ_PQ *_pq,
                                        SPECPQ_DATA_TYPE *pdata)
{
  const SPECPQ_PQ_ *pq = (SPECPQ_PQ_ *) _pq;

  if (pq->size == 1) {
    return FALSE;
  }

  *pdata = pq->cells[1].data;
  return TRUE;
}

/****************************************************************************
  Set the highest priority of the queue in 'datum_priority'. Return FALSE
  iff the queue is empty.
****************************************************************************/
static inline bool SPECPQ_FOO(_pq_priority)(const SPECPQ_PQ *_pq,
                                            SPECPQ_PRIORITY_TYPE *ppriority)

{
  const SPECPQ_PQ_ *pq = (SPECPQ_PQ_ *) _pq;

  if (pq->size == 1) {
    return FALSE;
  }

  *ppriority = pq->cells[1].priority;
  return TRUE;
}

#undef SPECPQ_TAG
#undef SPECPQ_PRIORITY_TYPE
#undef SPECPQ_DATA_TYPE
#undef SPECPQ_PASTE_
#undef SPECPQ_PASTE
#undef SPECPQ_PQ
#undef SPECPQ_PQ_
#undef SPECPQ_CELL_
#undef SPECPQ_FOO

#ifdef __cplusplus
}
#endif /* __cplusplus */
