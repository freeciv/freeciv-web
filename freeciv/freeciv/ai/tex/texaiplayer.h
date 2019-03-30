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
#ifndef FC__TEXAIPLAYER_H
#define FC__TEXAIPLAYER_H

/* utility */
#include "fcthread.h"

/* common */
#include "player.h"

/* ai/default */
#include "aidata.h"

/* ai/threxpt */
#include "texaimsg.h"

struct player;

struct texai_msgs
{
  fc_thread_cond thr_cond;
  fc_mutex mutex;
  struct texaimsg_list *msglist;
};

struct texai_reqs
{
  struct texaireq_list *reqlist;
};

struct texai_plr
{
  struct ai_plr defai; /* Keep this first so default ai finds it */
  struct unit_list *units;
};

struct ai_type *texai_get_self(void); /* Actually in texai.c */

void texai_init_threading(void);

bool texai_thread_running(void);

void texai_map_alloc(void);
void texai_whole_map_copy(void);
void texai_map_free(void);
void texai_player_alloc(struct ai_type *ait, struct player *pplayer);
void texai_player_free(struct ai_type *ait, struct player *pplayer);
void texai_control_gained(struct ai_type *ait,struct player *pplayer);
void texai_control_lost(struct ai_type *ait, struct player *pplayer);
void texai_refresh(struct ai_type *ait, struct player *pplayer);

void texai_msg_to_thr(struct texai_msg *msg);

void texai_req_from_thr(struct texai_req *req);

static inline struct texai_plr *texai_player_data(struct ai_type *ait,
                                                  const struct player *pplayer)
{
  return (struct texai_plr *)player_ai_data(pplayer, ait);
}

#endif /* FC__TEXAIPLAYER_H */
