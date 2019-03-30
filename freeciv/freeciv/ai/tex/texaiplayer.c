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
#include "map.h"
#include "unit.h"

/* server/advisors */
#include "advchoice.h"
#include "infracache.h"

/* ai/default */
#include "aiplayer.h"
#include "daimilitary.h"

/* ai/threxpr */
#include "texaicity.h"
#include "texaiworld.h"

#include "texaiplayer.h"

/* What level of operation we should abort because
 * of received messages. Lower is more critical;
 * TEXAI_ABORT_EXIT means that whole thread should exit,
 * TEXAI_ABORT_NONE means that we can continue what we were doing */
enum texai_abort_msg_class
{
  TEXAI_ABORT_EXIT,
  TEXAI_ABORT_PHASE_END,
  TEXAI_ABORT_NONE
};

static enum texai_abort_msg_class texai_check_messages(struct ai_type *ait);

struct texai_thr
{
  int num_players;
  struct texai_msgs msgs_to;
  struct texai_reqs reqs_from;
  bool thread_running;
  fc_thread ait;
} exthrai;

struct texai_build_choice_req
{
  int city_id;
  struct adv_choice choice;
};

/**********************************************************************//**
  Initialize ai thread.
**************************************************************************/
void texai_init_threading(void)
{
  exthrai.thread_running = FALSE;

  exthrai.num_players = 0;
}

/**********************************************************************//**
  This is main function of ai thread.
**************************************************************************/
static void texai_thread_start(void *arg)
{
  bool finished = FALSE;
  struct ai_type *texai = arg;

  log_debug("New AI thread launched");

  texai_world_init();
  if (!map_is_empty()) {
    texai_map_init();
  }

  /* Just wait until we are signaled to shutdown */
  fc_allocate_mutex(&exthrai.msgs_to.mutex);
  while (!finished) {
    fc_thread_cond_wait(&exthrai.msgs_to.thr_cond, &exthrai.msgs_to.mutex);

    if (texai_check_messages(texai) <= TEXAI_ABORT_EXIT) {
      finished = TRUE;
    }
  }
  fc_release_mutex(&exthrai.msgs_to.mutex);

  texai_world_close();

  log_debug("AI thread exiting");
}

/**********************************************************************//**
  Main map has been allocated
**************************************************************************/
void texai_map_alloc(void)
{
  texai_send_msg(TEXAI_MSG_MAP_ALLOC, NULL, NULL);
}

/**********************************************************************//**
  Send all tiles to tex thread
**************************************************************************/
void texai_whole_map_copy(void)
{
  whole_map_iterate(&(wld.map), ptile) {
    texai_tile_info(ptile);
  } whole_map_iterate_end;
}

/**********************************************************************//**
  Map allocation message received
**************************************************************************/
static void texai_map_alloc_recv(void)
{
  texai_map_init();
}

/**********************************************************************//**
  Main map has been freed
**************************************************************************/
void texai_map_free(void)
{
  texai_send_msg(TEXAI_MSG_MAP_FREE, NULL, NULL);
}

/**********************************************************************//**
  Map free message received
**************************************************************************/
static void texai_map_free_recv(void)
{
  texai_map_close();
}

/**********************************************************************//**
  Callback that returns unit list from player tex ai data.
**************************************************************************/
static struct unit_list *texai_player_units(struct player *pplayer)
{
  struct texai_plr *plr_data = player_ai_data(pplayer, texai_get_self());

  return plr_data->units;
}

/**********************************************************************//**
  Handle messages from message queue.
**************************************************************************/
static enum texai_abort_msg_class texai_check_messages(struct ai_type *ait)
{
  enum texai_abort_msg_class ret_abort= TEXAI_ABORT_NONE;

  texaimsg_list_allocate_mutex(exthrai.msgs_to.msglist);
  while (texaimsg_list_size(exthrai.msgs_to.msglist) > 0) {
    struct texai_msg *msg;
    enum texai_abort_msg_class new_abort = TEXAI_ABORT_NONE;

    msg = texaimsg_list_get(exthrai.msgs_to.msglist, 0);
    texaimsg_list_remove(exthrai.msgs_to.msglist, msg);
    texaimsg_list_release_mutex(exthrai.msgs_to.msglist);

    log_debug("Plr thr got %s", texaimsgtype_name(msg->type));

    switch(msg->type) {
    case TEXAI_MSG_FIRST_ACTIVITIES:
      fc_allocate_mutex(&game.server.mutexes.city_list);

      initialize_infrastructure_cache(msg->plr);

      /* Use _safe iterate in case the main thread
       * destroyes cities while we are iterating through these. */
      city_list_iterate_safe(msg->plr->cities, pcity) {
        struct adv_choice *choice;
        struct texai_build_choice_req *choice_req
          = fc_malloc(sizeof(struct texai_build_choice_req));
        struct city *tex_city = texai_map_city(pcity->id);

        texai_city_worker_requests_create(ait, msg->plr, pcity);
        texai_city_worker_wants(ait, msg->plr, pcity);

        if (tex_city != NULL) {
          choice = military_advisor_choose_build(ait, msg->plr, tex_city,
                                                 texai_map_get(), texai_player_units);
          choice_req->city_id = tex_city->id;
          adv_choice_copy(&(choice_req->choice), choice);
          adv_free_choice(choice);
          texai_send_req(TEXAI_BUILD_CHOICE, msg->plr, choice_req);
        }

        /* Release mutex for a second in case main thread
         * wants to do something to city list. */
        fc_release_mutex(&game.server.mutexes.city_list);

        /* Recursive message check in case phase is finished. */
        new_abort = texai_check_messages(ait);
        fc_allocate_mutex(&game.server.mutexes.city_list);
        if (new_abort < TEXAI_ABORT_NONE) {
          break;
        }
      } city_list_iterate_safe_end;
      fc_release_mutex(&game.server.mutexes.city_list);

      texai_send_req(TEXAI_REQ_TURN_DONE, msg->plr, NULL);

      break;
    case TEXAI_MSG_TILE_INFO:
      texai_tile_info_recv(msg->data);
      break;
    case TEXAI_MSG_UNIT_MOVED:
      texai_unit_moved_recv(msg->data);
      break;
    case TEXAI_MSG_UNIT_CREATED:
      texai_unit_info_recv(msg->data, msg->type);
      break;
    case TEXAI_MSG_UNIT_DESTROYED:
      texai_unit_destruction_recv(msg->data);
      break;
    case TEXAI_MSG_CITY_CREATED:
      texai_city_info_recv(msg->data, msg->type);
      break;
    case TEXAI_MSG_CITY_DESTROYED:
      texai_city_destruction_recv(msg->data);
      break;
    case TEXAI_MSG_PHASE_FINISHED:
      new_abort = TEXAI_ABORT_PHASE_END;
      break;
    case TEXAI_MSG_THR_EXIT:
      new_abort = TEXAI_ABORT_EXIT;
      break;
    case TEXAI_MSG_MAP_ALLOC:
      texai_map_alloc_recv();
      break;
    case TEXAI_MSG_MAP_FREE:
      texai_map_free_recv();
      break;
    default:
      log_error("Illegal message type %s (%d) for threaded ai!",
                texaimsgtype_name(msg->type), msg->type);
      break;
    }

    if (new_abort < ret_abort) {
      ret_abort = new_abort;
    }

    FC_FREE(msg);

    texaimsg_list_allocate_mutex(exthrai.msgs_to.msglist);
  }
  texaimsg_list_release_mutex(exthrai.msgs_to.msglist);

  return ret_abort;
}

/**********************************************************************//**
  Initialize player for use with tex AI.
**************************************************************************/
void texai_player_alloc(struct ai_type *ait, struct player *pplayer)
{
  struct texai_plr *player_data = fc_calloc(1, sizeof(struct texai_plr));

  player_set_ai_data(pplayer, ait, player_data);

  /* Default AI */
  dai_data_init(ait, pplayer);

  player_data->units = unit_list_new();
}

/**********************************************************************//**
  Free player from use with tex AI.
**************************************************************************/
void texai_player_free(struct ai_type *ait, struct player *pplayer)
{
  struct texai_plr *player_data = player_ai_data(pplayer, ait);

  /* Default AI */
  dai_data_close(ait, pplayer);

  if (player_data != NULL) {
    player_set_ai_data(pplayer, ait, NULL);
    unit_list_destroy(player_data->units);
    FC_FREE(player_data);
  }
}

/**********************************************************************//**
  We actually control the player
**************************************************************************/
void texai_control_gained(struct ai_type *ait, struct player *pplayer)
{
  exthrai.num_players++;

  log_debug("%s now under threxp AI (%d)", pplayer->name,
            exthrai.num_players);

  if (!exthrai.thread_running) {
    exthrai.msgs_to.msglist = texaimsg_list_new();
    exthrai.reqs_from.reqlist = texaireq_list_new();

    exthrai.thread_running = TRUE;
 
    fc_thread_cond_init(&exthrai.msgs_to.thr_cond);
    fc_init_mutex(&exthrai.msgs_to.mutex);
    fc_thread_start(&exthrai.ait, texai_thread_start, ait);
  }
}

/**********************************************************************//**
  We no longer control the player
**************************************************************************/
void texai_control_lost(struct ai_type *ait, struct player *pplayer)
{
  exthrai.num_players--;

  log_debug("%s no longer under threaded AI (%d)", pplayer->name,
            exthrai.num_players);

  if (exthrai.num_players <= 0) {
    texai_send_msg(TEXAI_MSG_THR_EXIT, pplayer, NULL);

    fc_thread_wait(&exthrai.ait);
    exthrai.thread_running = FALSE;

    fc_thread_cond_destroy(&exthrai.msgs_to.thr_cond);
    fc_destroy_mutex(&exthrai.msgs_to.mutex);
    texaimsg_list_destroy(exthrai.msgs_to.msglist);
    texaireq_list_destroy(exthrai.reqs_from.reqlist);
  }
}

/**********************************************************************//**
  Check for messages sent by player thread
**************************************************************************/
void texai_refresh(struct ai_type *ait, struct player *pplayer)
{
  if (exthrai.thread_running) {
    texaireq_list_allocate_mutex(exthrai.reqs_from.reqlist);
    while (texaireq_list_size(exthrai.reqs_from.reqlist) > 0) {
       struct texai_req *req;

       req = texaireq_list_get(exthrai.reqs_from.reqlist, 0);
       texaireq_list_remove(exthrai.reqs_from.reqlist, req);

       texaireq_list_release_mutex(exthrai.reqs_from.reqlist);

       log_debug("Plr thr sent %s", texaireqtype_name(req->type));

       switch(req->type) {
       case TEXAI_REQ_WORKER_TASK:
         texai_req_worker_task_rcv(req);
         break;
       case TEXAI_BUILD_CHOICE:
         {
           struct texai_build_choice_req *choice_req
             = (struct texai_build_choice_req *)(req->data);
           struct city *pcity = game_city_by_number(choice_req->city_id);

           if (pcity != NULL && city_owner(pcity) == req->plr) {
             adv_choice_copy(&(def_ai_city_data(pcity, ait)->choice),
                             &(choice_req->choice));
             FC_FREE(choice_req);
           }
         }
         break;
       case TEXAI_REQ_TURN_DONE:
         req->plr->ai_phase_done = TRUE;
         break;
       }

       FC_FREE(req);

       texaireq_list_allocate_mutex(exthrai.reqs_from.reqlist);
     }
    texaireq_list_release_mutex(exthrai.reqs_from.reqlist);
  }
}

/**********************************************************************//**
  Send message to thread. Be sure that thread is running so that messages
  are not just piling up to the list without anybody reading them.
**************************************************************************/
void texai_msg_to_thr(struct texai_msg *msg)
{
  fc_allocate_mutex(&exthrai.msgs_to.mutex);
  texaimsg_list_append(exthrai.msgs_to.msglist, msg);
  fc_thread_cond_signal(&exthrai.msgs_to.thr_cond);
  fc_release_mutex(&exthrai.msgs_to.mutex);
}

/**********************************************************************//**
  Thread sends message.
**************************************************************************/
void texai_req_from_thr(struct texai_req *req)
{
  texaireq_list_allocate_mutex(exthrai.reqs_from.reqlist);
  texaireq_list_append(exthrai.reqs_from.reqlist, req);
  texaireq_list_release_mutex(exthrai.reqs_from.reqlist);
}

/**********************************************************************//**
  Return whether player thread is running
**************************************************************************/
bool texai_thread_running(void)
{
  return exthrai.thread_running;
}
