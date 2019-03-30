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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

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
int voteinfo_queue_size(void);

bool voteinfo_bar_can_be_shown(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FC__VOTEINFO_H */

