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
#ifndef FC__TAIMSG_H
#define FC__TAIMSG_H

#define SPECENUM_NAME taimsgtype
#define SPECENUM_VALUE0 TAI_MSG_THR_EXIT
#define SPECENUM_VALUE0NAME "Exit"
#define SPECENUM_VALUE1 TAI_MSG_FIRST_ACTIVITIES
#define SPECENUM_VALUE1NAME "FirstActivities"
#define SPECENUM_VALUE2 TAI_MSG_PHASE_FINISHED
#define SPECENUM_VALUE2NAME "PhaseFinished"
#include "specenum_gen.h"

#define SPECENUM_NAME taireqtype
#define SPECENUM_VALUE0 TAI_REQ_WORKER_TASK
#define SPECENUM_VALUE0NAME "WorkerTask"
#define SPECENUM_VALUE1 TAI_REQ_TURN_DONE
#define SPECENUM_VALUE1NAME "TurnDone"
#include "specenum_gen.h"

struct tai_msg
{
  enum taimsgtype type;
  struct player *plr;
  void *data;
};

struct tai_req
{
  enum taireqtype type;
  struct player *plr;
  void *data;
};

#define SPECLIST_TAG taimsg
#define SPECLIST_TYPE struct tai_msg
#include "speclist.h"

#define SPECLIST_TAG taireq
#define SPECLIST_TYPE struct tai_req
#include "speclist.h"

void tai_send_msg(enum taimsgtype type, struct player *pplayer,
                  void *data);
void tai_send_req(enum taireqtype type, struct player *pplayer,
                  void *data);

void tai_first_activities(struct ai_type *ait, struct player *pplayer);
void tai_phase_finished(struct ai_type *ait, struct player *pplayer);

#endif /* FC__TAIMSG_H */
