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

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

/* ai/threaded */
#include "taiplayer.h"

#include "taimsg.h"

/**********************************************************************//**
  Construct and send message to player thread.
**************************************************************************/
void tai_send_msg(enum taimsgtype type, struct player *pplayer,
                  void *data)
{
  struct tai_msg *msg;

  if (!tai_thread_running()) {
    /* No player thread to send messages to */
    return;
  }

  msg = fc_malloc(sizeof(*msg));

  msg->type = type;
  msg->plr = pplayer;
  msg->data = data;

  tai_msg_to_thr(msg);
}

/**********************************************************************//**
  Construct and send request from player thread.
**************************************************************************/
void tai_send_req(enum taireqtype type, struct player *pplayer,
                  void *data)
{
  struct tai_req *req = fc_malloc(sizeof(*req));

  req->type = type;
  req->plr = pplayer;
  req->data = data;

  tai_req_from_thr(req);
}

/**********************************************************************//**
  Time for phase first activities
**************************************************************************/
void tai_first_activities(struct ai_type *ait, struct player *pplayer)
{
  tai_send_msg(TAI_MSG_FIRST_ACTIVITIES, pplayer, NULL);
}

/**********************************************************************//**
  Player phase has finished
**************************************************************************/
void tai_phase_finished(struct ai_type *ait, struct player *pplayer)
{
  tai_send_msg(TAI_MSG_PHASE_FINISHED, pplayer, NULL);
}
