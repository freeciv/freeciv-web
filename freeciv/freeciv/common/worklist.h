/***********************************************************************
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#ifndef FC__WORKLIST_H
#define FC__WORKLIST_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "registry.h"

#include "fc_types.h"

#define MAX_LEN_WORKLIST 64
#define MAX_NUM_WORKLISTS 16

/* a worklist */
struct worklist {
  int length;
  struct universal entries[MAX_LEN_WORKLIST];
};

void worklist_init(struct worklist *pwl);

int worklist_length(const struct worklist *pwl);
bool worklist_is_empty(const struct worklist *pwl);
bool worklist_peek(const struct worklist *pwl, struct universal *prod);
bool worklist_peek_ith(const struct worklist *pwl,
                       struct universal *prod, int idx);
void worklist_advance(struct worklist *pwl);

void worklist_copy(struct worklist *dst, const struct worklist *src);
void worklist_remove(struct worklist *pwl, int idx);
bool worklist_append(struct worklist *pwl, const struct universal *prod);
bool worklist_insert(struct worklist *pwl, const struct universal *prod,
                     int idx);
bool are_worklists_equal(const struct worklist *wlist1,
                         const struct worklist *wlist2);

/* Iterate over all entries in the worklist. */
#define worklist_iterate(_list, _p)					\
{									\
  struct universal _p;						\
  int _p##_index = 0;							\
									\
  while (_p##_index < worklist_length(_list)) {				\
    worklist_peek_ith(_list, &_p, _p##_index++);

#define worklist_iterate_end						\
  }									\
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FC__WORKLIST_H */
