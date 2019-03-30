/********************************************************************** 
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
#ifndef FC__WORKERTASK_H
#define FC__WORKERTASK_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct worker_task
{
  struct tile *ptile;
  enum unit_activity act;
  struct extra_type *tgt;
  int want;
};

/* get 'struct worker_task_list' and related functions: */
#define SPECLIST_TAG worker_task
#define SPECLIST_TYPE struct worker_task
#include "speclist.h"

#define worker_task_list_iterate(tasklist, ptask) \
  TYPED_LIST_ITERATE(struct worker_task, tasklist, ptask)
#define worker_task_list_iterate_end LIST_ITERATE_END

void worker_task_init(struct worker_task *ptask);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__WORKERTASK_H */
