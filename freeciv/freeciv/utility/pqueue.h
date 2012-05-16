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
#ifndef FC__PQUEUE_H
#define FC__PQUEUE_H

#include "shared.h"

typedef short pq_data_t;
struct pqueue;

struct pqueue *pq_create(int initial_size);
void pq_destroy(struct pqueue *q);
void pq_insert(struct pqueue *q, const pq_data_t datum, int datum_priority);
bool pq_remove(struct pqueue * q, pq_data_t *dest);
bool pq_peek(struct pqueue * q, pq_data_t*dest);

#endif
