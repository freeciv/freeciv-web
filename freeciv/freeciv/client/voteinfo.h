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
#ifndef FC__VOTEINFO_H
#define FC__VOTEINFO_H

#include "fc_types.h"

enum client_vote_type {
  CVT_NONE = 0,
  CVT_YES,
  CVT_NO,
  CVT_ABSTAIN
};

struct voteinfo {
  /* Set by the server via packets. */
  int vote_no;
  char user[MAX_LEN_NAME];
  char desc[512];
  int percent_required;
  int flags;
  int yes;
  int no;
  int abstain;
  int num_voters;
  bool resolved;
  bool passed;

  /* Set/used by the client. */
  enum client_vote_type client_vote;
  time_t remove_time;
};

void voteinfo_queue_init(void);
void voteinfo_queue_free(void);
void voteinfo_queue_remove(int vote_no);
void voteinfo_queue_delayed_remove(int vote_no);
void voteinfo_queue_check_removed(void);
void voteinfo_queue_add(int vote_no, const char *user, const char *desc,
                        int percent_required, int flags);
struct voteinfo *voteinfo_queue_find(int vote_no);
void voteinfo_do_vote(int vote_no, enum client_vote_type vote);
struct voteinfo *voteinfo_queue_get_current(int *pindex);
void voteinfo_queue_next(void);

/* Define struct voteinfo_list type. */
#define SPECLIST_TAG voteinfo
#define SPECLIST_TYPE struct voteinfo
#include "speclist.h"
#define voteinfo_list_iterate(alist, pitem)\
  TYPED_LIST_ITERATE(struct voteinfo, alist, pitem)
#define voteinfo_list_iterate_end  LIST_ITERATE_END

extern struct voteinfo_list *voteinfo_queue;

#endif /* FC__VOTEINFO_H */

