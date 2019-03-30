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

/* utility */
#include "log.h"

/* common */
#include "ai.h"
#include "city.h"
#include "game.h"
#include "unit.h"

/* server/advisors */
#include "infracache.h"

/* ai/default */
#include "aiplayer.h"

/* ai/threaded */
#include "taicity.h"

#include "taiplayer.h"

/* What level of operation we should abort because
 * of received messages. Lower is more critical;
 * TAI_ABORT_EXIT means that whole thread should exit,
 * TAI_ABORT_NONE means that we can continue what we were doing */
enum tai_abort_msg_class
{
  TAI_ABORT_EXIT,
  TAI_ABORT_PHASE_END,
  TAI_ABORT_NONE
};

static enum tai_abort_msg_class tai_check_messages(struct ai_type *ait);

struct tai_thr
{
  int num_players;
  struct tai_msgs msgs_to;
  struct tai_reqs reqs_from;
  bool thread_running;
  fc_thread ait;
} thrai;

/**********************************************************************//**
  Initialize ai thread.
**************************************************************************/
void tai_init_threading(void)
{
  thrai.thread_running = FALSE;

  thrai.num_players = 0;
}

/**********************************************************************//**
  This is main function of ai thread.
**************************************************************************/
static void tai_thread_start(void *arg)
{
  bool finished = FALSE;
  struct ai_type *ait = arg;

  log_debug("New AI thread launched");

  /* Just wait until we are signaled to shutdown */
  fc_allocate_mutex(&thrai.msgs_to.mutex);
  while (!finished) {
    fc_thread_cond_wait(&thrai.msgs_to.thr_cond, &thrai.msgs_to.mutex);

    if (tai_check_messages(ait) <= TAI_ABORT_EXIT) {
      finished = TRUE;
    }
  }
  fc_release_mutex(&thrai.msgs_to.mutex);

  log_debug("AI thread exiting");
}

/**********************************************************************//**
  Handle messages from message queue.
**************************************************************************/
static enum tai_abort_msg_class tai_check_messages(struct ai_type *ait)
{
  enum tai_abort_msg_class ret_abort= TAI_ABORT_NONE;

  taimsg_list_allocate_mutex(thrai.msgs_to.msglist);
  while (taimsg_list_size(thrai.msgs_to.msglist) > 0) {
    struct tai_msg *msg;
    enum tai_abort_msg_class new_abort = TAI_ABORT_NONE;

    msg = taimsg_list_get(thrai.msgs_to.msglist, 0);
    taimsg_list_remove(thrai.msgs_to.msglist, msg);
    taimsg_list_release_mutex(thrai.msgs_to.msglist);

    log_debug("Plr thr got %s", taimsgtype_name(msg->type));

    switch(msg->type) {
    case TAI_MSG_FIRST_ACTIVITIES:
      fc_allocate_mutex(&game.server.mutexes.city_list);

      initialize_infrastructure_cache(msg->plr);

      /* Use _safe iterate in case the main thread
       * destroyes cities while we are iterating through these. */
      city_list_iterate_safe(msg->plr->cities, pcity) {
        tai_city_worker_requests_create(ait, msg->plr, pcity);

        /* Release mutex for a second in case main thread
         * wants to do something to city list. */
        fc_release_mutex(&game.server.mutexes.city_list);

        /* Recursive message check in case phase is finished. */
        new_abort = tai_check_messages(ait);
        fc_allocate_mutex(&game.server.mutexes.city_list);
        if (new_abort < TAI_ABORT_NONE) {
          break;
        }
      } city_list_iterate_safe_end;
      fc_release_mutex(&game.server.mutexes.city_list);

      tai_send_req(TAI_REQ_TURN_DONE, msg->plr, NULL);

      break;
    case TAI_MSG_PHASE_FINISHED:
      new_abort = TAI_ABORT_PHASE_END;
      break;
    case TAI_MSG_THR_EXIT:
      new_abort = TAI_ABORT_EXIT;
      break;
    default:
      log_error("Illegal message type %s (%d) for threaded ai!",
                taimsgtype_name(msg->type), msg->type);
      break;
    }

    if (new_abort < ret_abort) {
      ret_abort = new_abort;
    }

    FC_FREE(msg);

    taimsg_list_allocate_mutex(thrai.msgs_to.msglist);
  }
  taimsg_list_release_mutex(thrai.msgs_to.msglist);

  return ret_abort;
}

/**********************************************************************//**
  Initialize player for use with threaded AI.
**************************************************************************/
void tai_player_alloc(struct ai_type *ait, struct player *pplayer)
{
  struct tai_plr *player_data = fc_calloc(1, sizeof(struct tai_plr));

  player_set_ai_data(pplayer, ait, player_data);

  /* Default AI */
  dai_data_init(ait, pplayer);
}

/**********************************************************************//**
  Free player from use with threaded AI.
**************************************************************************/
void tai_player_free(struct ai_type *ait, struct player *pplayer)
{
  struct tai_plr *player_data = player_ai_data(pplayer, ait);

  /* Default AI */
  dai_data_close(ait, pplayer);

  if (player_data != NULL) {
    player_set_ai_data(pplayer, ait, NULL);
    FC_FREE(player_data);
  }
}

/**********************************************************************//**
  We actually control the player
**************************************************************************/
void tai_control_gained(struct ai_type *ait, struct player *pplayer)
{
  thrai.num_players++;

  log_debug("%s now under threaded AI (%d)", pplayer->name, thrai.num_players);

  if (!thrai.thread_running) {
    thrai.msgs_to.msglist = taimsg_list_new();
    thrai.reqs_from.reqlist = taireq_list_new();

    thrai.thread_running = TRUE;
 
    fc_thread_cond_init(&thrai.msgs_to.thr_cond);
    fc_init_mutex(&thrai.msgs_to.mutex);
    fc_thread_start(&thrai.ait, tai_thread_start, ait);
  }
}

/**********************************************************************//**
  We no longer control the player
**************************************************************************/
void tai_control_lost(struct ai_type *ait, struct player *pplayer)
{
  thrai.num_players--;

  log_debug("%s no longer under threaded AI (%d)", pplayer->name, thrai.num_players);

  if (thrai.num_players <= 0) {
    tai_send_msg(TAI_MSG_THR_EXIT, pplayer, NULL);

    fc_thread_wait(&thrai.ait);
    thrai.thread_running = FALSE;

    fc_thread_cond_destroy(&thrai.msgs_to.thr_cond);
    fc_destroy_mutex(&thrai.msgs_to.mutex);
    taimsg_list_destroy(thrai.msgs_to.msglist);
    taireq_list_destroy(thrai.reqs_from.reqlist);
  }
}

/**********************************************************************//**
  Check for messages sent by player thread
**************************************************************************/
void tai_refresh(struct ai_type *ait, struct player *pplayer)
{
  if (thrai.thread_running) {
    taireq_list_allocate_mutex(thrai.reqs_from.reqlist);
    while (taireq_list_size(thrai.reqs_from.reqlist) > 0) {
       struct tai_req *req;

       req = taireq_list_get(thrai.reqs_from.reqlist, 0);
       taireq_list_remove(thrai.reqs_from.reqlist, req);

       taireq_list_release_mutex(thrai.reqs_from.reqlist);

       log_debug("Plr thr sent %s", taireqtype_name(req->type));

       switch(req->type) {
       case TAI_REQ_WORKER_TASK:
         tai_req_worker_task_rcv(req);
         break;
       case TAI_REQ_TURN_DONE:
         req->plr->ai_phase_done = TRUE;
         break;
       }

       FC_FREE(req);

       taireq_list_allocate_mutex(thrai.reqs_from.reqlist);
     }
    taireq_list_release_mutex(thrai.reqs_from.reqlist);
  }
}

/**********************************************************************//**
  Send message to thread. Be sure that thread is running so that messages
  are not just piling up to the list without anybody reading them.
**************************************************************************/
void tai_msg_to_thr(struct tai_msg *msg)
{
  fc_allocate_mutex(&thrai.msgs_to.mutex);
  taimsg_list_append(thrai.msgs_to.msglist, msg);
  fc_thread_cond_signal(&thrai.msgs_to.thr_cond);
  fc_release_mutex(&thrai.msgs_to.mutex);
}

/**********************************************************************//**
  Thread sends message.
**************************************************************************/
void tai_req_from_thr(struct tai_req *req)
{
  taireq_list_allocate_mutex(thrai.reqs_from.reqlist);
  taireq_list_append(thrai.reqs_from.reqlist, req);
  taireq_list_release_mutex(thrai.reqs_from.reqlist);
}

/**********************************************************************//**
  Return whether player thread is running
**************************************************************************/
bool tai_thread_running(void)
{
  return thrai.thread_running;
}
