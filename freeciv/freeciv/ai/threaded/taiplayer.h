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
#ifndef FC__TAIPLAYER_H
#define FC__TAIPLAYER_H

/* utility */
#include "fcthread.h"

/* common */
#include "player.h"

/* ai/default */
#include "aidata.h"

/* ai/threaded */
#include "taimsg.h"

struct player;

struct tai_msgs
{
  fc_thread_cond thr_cond;
  fc_mutex mutex;
  struct taimsg_list *msglist;
};

struct tai_reqs
{
  struct taireq_list *reqlist;
};

struct tai_plr
{
  struct ai_plr defai; /* Keep this first so default ai finds it */
};

void tai_init_threading(void);

bool tai_thread_running(void);

void tai_player_alloc(struct ai_type *ait, struct player *pplayer);
void tai_player_free(struct ai_type *ait, struct player *pplayer);
void tai_control_gained(struct ai_type *ait,struct player *pplayer);
void tai_control_lost(struct ai_type *ait, struct player *pplayer);
void tai_refresh(struct ai_type *ait, struct player *pplayer);

void tai_msg_to_thr(struct tai_msg *msg);

void tai_req_from_thr(struct tai_req *req);

static inline struct tai_plr *tai_player_data(struct ai_type *ait,
                                              const struct player *pplayer)
{
  return (struct tai_plr *)player_ai_data(pplayer, ait);
}

#endif /* FC__TAIPLAYER_H */
