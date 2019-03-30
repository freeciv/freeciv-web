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

#include <stdarg.h>

/* utility */
#include "bitvector.h"
#include "log.h"
#include "registry.h"

/* common */
#include "connection.h"
#include "events.h"
#include "featured_text.h"
#include "game.h"
#include "research.h"
#include "packets.h"
#include "player.h"
#include "tile.h"

/* server */
#include "maphand.h"
#include "srv_main.h"

#include "notify.h"


/**********************************************************************//**
  Fill a packet_chat_msg structure.

  packet: A pointer to the packet.
  ptile: A pointer to a tile the event is occuring.
  event: The event type.
  pconn: The sender of the event (e.g. when event is E_CHAT_MSG).
  color: The requested color or ftc_any if not requested.  Some colors are
    predefined in common/featured_text.h.  You can pass a custom one using
    ft_color().
  format: The format of the message.
  vargs: The extra arguments to build the message.
**************************************************************************/
static void package_event_full(struct packet_chat_msg *packet,
                               const struct tile *ptile,
                               enum event_type event,
                               const struct connection *pconn,
                               const struct ft_color color,
                               const char *format, va_list vargs)
{
  char buf[MAX_LEN_MSG];
  char *str;

  fc_assert_ret(NULL != packet);

  packet->tile = (NULL != ptile ? tile_index(ptile) : -1);
  packet->event = event;
  packet->conn_id = pconn ? pconn->id : -1;
  packet->turn = game.info.turn;
  packet->phase = game.info.phase;

  fc_vsnprintf(buf, sizeof(buf), format, vargs);
  if (is_capitalization_enabled()) {
    str = capitalized_string(buf);
  } else {
    str = buf;
  }

  if (ft_color_requested(color)) {
    featured_text_apply_tag(str, packet->message, sizeof(packet->message),
                            TTT_COLOR, 0, FT_OFFSET_UNSET, color);
  } else {
    /* Simple case */
    sz_strlcpy(packet->message, str);
  }

  if (is_capitalization_enabled()) {
    free_capitalized(str);
  }
}

/**********************************************************************//**
  Fill a packet_chat_msg structure for a chat message.

  packet: A pointer to the packet.
  sender: The sender of the message.
  color: The requested color or ftc_any if not requested.  Some colors are
    predefined in common/featured_text.h.  You can pass a custom one using
    ft_color().
  format: The format of the message.
  vargs: The extra arguments to build the message.
**************************************************************************/
void vpackage_chat_msg(struct packet_chat_msg *packet,
                       const struct connection *sender,
                       const struct ft_color color,
                       const char *format, va_list vargs)
{
  package_event_full(packet, NULL, E_CHAT_MSG, sender, color, format, vargs);
}

/**********************************************************************//**
  Fill a packet_chat_msg structure for a chat message.

  packet: A pointer to the packet.
  sender: The sender of the message.
  color: The requested color or ftc_any if not requested.  Some colors are
    predefined in common/featured_text.h.  You can pass a custom one using
    ft_color().
  format: The format of the message.
  ...: The extra arguments to build the message.
**************************************************************************/
void package_chat_msg(struct packet_chat_msg *packet,
                      const struct connection *sender,
                      const struct ft_color color,
                      const char *format, ...)
{
  va_list args;

  va_start(args, format);
  vpackage_chat_msg(packet, sender, color, format, args);
  va_end(args);
}

/**********************************************************************//**
  Fill a packet_chat_msg structure for common server event.

  packet: A pointer to the packet.
  ptile: A pointer to a tile the event is occuring.
  event: The event type.
  color: The requested color or ftc_any if not requested.  Some colors are
    predefined in common/featured_text.h.  You can pass a custom one using
    ft_color().
  format: The format of the message.
  vargs: The extra arguments to build the message.
**************************************************************************/
void vpackage_event(struct packet_chat_msg *packet,
                    const struct tile *ptile,
                    enum event_type event,
                    const struct ft_color color,
                    const char *format, va_list vargs)
{
  package_event_full(packet, ptile, event, NULL, color, format, vargs);
}

/**********************************************************************//**
  Fill a packet_chat_msg structure for common server event.

  packet: A pointer to the packet.
  ptile: A pointer to a tile the event is occuring.
  event: The event type.
  color: The requested color or ftc_any if not requested.  Some colors are
    predefined in common/featured_text.h.  You can pass a custom one using
    ft_color().
  format: The format of the message.
  ...: The extra arguments to build the message.
**************************************************************************/
void package_event(struct packet_chat_msg *packet,
                   const struct tile *ptile,
                   enum event_type event,
                   const struct ft_color color,
                   const char *format, ...)
{
  va_list args;

  va_start(args, format);
  vpackage_event(packet, ptile, event, color, format, args);
  va_end(args);
}


/**********************************************************************//**
  This is the basis for following notify_* functions. It uses the struct
  packet_chat_msg as defined by vpackage_event().

  Notify specified connections of an event of specified type (from events.h)
  and specified (x,y) coords associated with the event.  Coords will only
  apply if game has started and the conn's player knows that tile (or
  NULL == pconn->playing && pconn->observer).  If coords are not required,
  caller should specify (x,y) = (-1,-1); otherwise make sure that the
  coordinates have been normalized.
**************************************************************************/
static void notify_conn_packet(struct conn_list *dest,
                               const struct packet_chat_msg *packet,
                               bool early)
{
  struct packet_chat_msg real_packet = *packet;
  int tile = packet->tile;
  struct tile *ptile = index_to_tile(&(wld.map), tile);

  if (!dest) {
    dest = game.est_connections;
  }

  conn_list_iterate(dest, pconn) {
    /* Avoid sending messages that could potentially reveal
     * internal information about the server machine to
     * connections that do not already have hack access. */
    if ((packet->event == E_LOG_ERROR || packet->event == E_LOG_FATAL)
        && pconn->access_level != ALLOW_HACK) {
      continue;
    }

    if (S_S_RUNNING <= server_state()
        && ptile /* special case, see above */
        && ((NULL == pconn->playing && pconn->observer)
            || (NULL != pconn->playing
                && map_is_known(ptile, pconn->playing)))) {
      /* tile info is OK; see above. */
      /* FIXME: in the case this is a city event, we should check if the
       * city is really known. */
      real_packet.tile = tile;
    } else {
      /* No tile info. */
      real_packet.tile = -1;
    }

    if (early) {
      send_packet_early_chat_msg(pconn, (struct packet_early_chat_msg *)(&real_packet));
    } else {
      send_packet_chat_msg(pconn, &real_packet);
    }
  } conn_list_iterate_end;
}

/**********************************************************************//**
  See notify_conn_packet - this is just the "non-v" version, with varargs.
**************************************************************************/
void notify_conn(struct conn_list *dest,
                 const struct tile *ptile,
                 enum event_type event,
                 const struct ft_color color,
                 const char *format, ...)
{
  struct packet_chat_msg genmsg;
  va_list args;

  va_start(args, format);
  vpackage_event(&genmsg, ptile, event, color, format, args);
  va_end(args);

  notify_conn_packet(dest, &genmsg, FALSE);

  if (!dest || dest == game.est_connections) {
    /* Add to the cache */
    event_cache_add_for_all(&genmsg);
  }
}

/**********************************************************************//**
  See notify_conn_packet - this is just the "non-v" version, with varargs.
  Use for early connecting protocol messages.
**************************************************************************/
void notify_conn_early(struct conn_list *dest,
                       const struct tile *ptile,
                       enum event_type event,
                       const struct ft_color color,
                       const char *format, ...)
{
  struct packet_chat_msg genmsg;
  va_list args;

  va_start(args, format);
  vpackage_event(&genmsg, ptile, event, color, format, args);
  va_end(args);

  notify_conn_packet(dest, &genmsg, TRUE);

  if (!dest || dest == game.est_connections) {
    /* Add to the cache */
    event_cache_add_for_all(&genmsg);
  }
}

/**********************************************************************//**
  Similar to notify_conn_packet (see also), but takes player as "destination".
  If player != NULL, sends to all connections for that player.
  If player == NULL, sends to all game connections, to support
  old code, but this feature may go away - should use notify_conn(NULL)
  instead.
**************************************************************************/
void notify_player(const struct player *pplayer,
                   const struct tile *ptile,
                   enum event_type event,
                   const struct ft_color color,
                   const char *format, ...) 
{
  struct conn_list *dest = pplayer ? pplayer->connections : NULL;
  struct packet_chat_msg genmsg;
  va_list args;

  va_start(args, format);
  vpackage_event(&genmsg, ptile, event, color, format, args);
  va_end(args);

  notify_conn_packet(dest, &genmsg, FALSE);

  /* Add to the cache */
  event_cache_add_for_player(&genmsg, pplayer);
}

/**********************************************************************//**
  Send message to all players who have an embassy with pplayer,
  but excluding pplayer and specified player.
**************************************************************************/
void notify_embassies(const struct player *pplayer,
                      const struct tile *ptile,
                      enum event_type event,
                      const struct ft_color color,
                      const char *format, ...) 
{
  struct packet_chat_msg genmsg;
  struct event_cache_players *players = NULL;
  va_list args;

  va_start(args, format);
  vpackage_event(&genmsg, ptile, event, color, format, args);
  va_end(args);

  players_iterate(other_player) {
    if (player_has_embassy(other_player, pplayer)
        && pplayer != other_player) {
      notify_conn_packet(other_player->connections, &genmsg, FALSE);
      players = event_cache_player_add(players, other_player);
    }
  } players_iterate_end;

  /* Add to the cache */
  event_cache_add_for_players(&genmsg, players);
}

/**********************************************************************//**
  Sends a message to all players on pplayer's team. If 'pplayer' is NULL,
  sends to all players.
**************************************************************************/
void notify_team(const struct player *pplayer,
                 const struct tile *ptile,
                 enum event_type event,
                 const struct ft_color color,
                 const char *format, ...)
{
  struct conn_list *dest = game.est_connections;
  struct packet_chat_msg genmsg;
  struct event_cache_players *players = NULL;
  va_list args;

  va_start(args, format);
  vpackage_event(&genmsg, ptile, event, color, format, args);
  va_end(args);

  if (pplayer) {
    dest = conn_list_new();
    players_iterate(other_player) {
      if (!players_on_same_team(pplayer, other_player)) {
        continue;
      }
      conn_list_iterate(other_player->connections, pconn) {
        conn_list_append(dest, pconn);
      } conn_list_iterate_end;
      players = event_cache_player_add(players, other_player);
    } players_iterate_end;

    /* Add to the cache */
    event_cache_add_for_players(&genmsg, players);

  } else {
    /* Add to the cache for all players. */
    event_cache_add_for_all(&genmsg);
  }

  notify_conn_packet(dest, &genmsg, FALSE);

  if (pplayer) {
    conn_list_destroy(dest);
  }
}

/**********************************************************************//**
  Sends a message to all players that share research.

  Unlike other notify functions this one does not take a tile argument. We
  assume no research message will have a tile associated.
**************************************************************************/
void notify_research(const struct research *presearch,
                     const struct player *exclude,
                     enum event_type event,
                     const struct ft_color color,
                     const char *format, ...)
{
  struct packet_chat_msg genmsg;
  struct event_cache_players *players = NULL;
  va_list args;

  va_start(args, format);
  vpackage_event(&genmsg, NULL, event, color, format, args);
  va_end(args);

  research_players_iterate(presearch, aplayer) {
    if (exclude != aplayer) {
      lsend_packet_chat_msg(aplayer->connections, &genmsg);
      players = event_cache_player_add(players, aplayer);
    }
  } research_players_iterate_end;

  /* Add to the cache */
  event_cache_add_for_players(&genmsg, players);
}

/**********************************************************************//**
  Sends a message to all players that have embassies with someone who
  shares research.

  Unlike other notify functions this one does not take a tile argument. We
  assume no research message will have a tile associated.

  Exclude parameter excludes everyone who has embassy (only) with that
  player.

  FIXME: Should not send multiple messages if one has embassy with multiple
         members of the research group, should really exclude ones having
         embassy with the exclude -one as the use-case for exclusion is that
         different message is being sent to those excluded here.
**************************************************************************/
void notify_research_embassies(const struct research *presearch,
                               const struct player *exclude,
                               enum event_type event,
                               const struct ft_color color,
                               const char *format, ...)
{
  struct packet_chat_msg genmsg;
  struct event_cache_players *players = NULL;
  va_list args;

  va_start(args, format);
  vpackage_event(&genmsg, NULL, event, color, format, args);
  va_end(args);

  players_iterate(aplayer) {
    if (exclude == aplayer || research_get(aplayer) == presearch) {
      continue;
    }

    research_players_iterate(presearch, rplayer) {
      if (player_has_embassy(aplayer, rplayer)) {
        lsend_packet_chat_msg(aplayer->connections, &genmsg);
        players = event_cache_player_add(players, aplayer);
        break;
      }
    } research_players_iterate_end;
  } players_iterate_end;

  /* Add to the cache */
  event_cache_add_for_players(&genmsg, players);
}


/**************************************************************************
  Event cache datas.
**************************************************************************/
enum event_cache_target {
  ECT_ALL,
  ECT_PLAYERS,
  ECT_GLOBAL_OBSERVERS
};

/* Events are saved in that structure. */
struct event_cache_data {
  struct packet_chat_msg packet;
  time_t timestamp;
  enum server_states server_state;
  enum event_cache_target target_type;
  bv_player target;     /* Used if target_type == ECT_PLAYERS. */
};

#define SPECLIST_TAG event_cache_data
#define SPECLIST_TYPE struct event_cache_data
#include "speclist.h"
#define event_cache_iterate(pdata) \
    TYPED_LIST_ITERATE(struct event_cache_data, event_cache, pdata)
#define event_cache_iterate_end  LIST_ITERATE_END

struct event_cache_players {
  bv_player vector;
};

/* The full list of the events. */
static struct event_cache_data_list *event_cache = NULL;

/* Event cache status: ON(TRUE) / OFF(FALSE); used for saving the
 * event cache */
static bool event_cache_status = FALSE;

/**********************************************************************//**
  Callback for freeing event cache data
**************************************************************************/
static void event_cache_data_free(struct event_cache_data *data)
{
  free(data);
}

/**********************************************************************//**
  Creates a new event_cache_data, appened to the list.  It mays remove an
  old entry if needed.
**************************************************************************/
static struct event_cache_data *
event_cache_data_new(const struct packet_chat_msg *packet,
                     time_t timestamp, enum server_states server_status,
                     enum event_cache_target target_type,
                     struct event_cache_players *players)
{
  struct event_cache_data *pdata;
  int max_events;

  if (NULL == event_cache) {
    /* Don't do log for this, because this could make an infinite
     * recursion. */
    return NULL;
  }
  fc_assert_ret_val(NULL != packet, NULL);

  if (packet->event == E_MESSAGE_WALL) {
    /* No popups at save game load. */
    return NULL;
  }

  if (!game.server.event_cache.chat && packet->event == E_CHAT_MSG) {
    /* chat messages should _not_ be saved */
    return NULL;
  }

  /* check if cache is active */
  if (!event_cache_status) {
    return NULL;
  }

  pdata = fc_malloc(sizeof(*pdata));
  pdata->packet = *packet;
  pdata->timestamp = timestamp;
  pdata->server_state = server_status;
  pdata->target_type = target_type;
  if (players) {
    pdata->target = players->vector;
  } else {
    BV_CLR_ALL(pdata->target);
  }
  event_cache_data_list_append(event_cache, pdata);

  max_events = game.server.event_cache.max_size
               ? game.server.event_cache.max_size
               : GAME_MAX_EVENT_CACHE_MAX_SIZE;
  while (event_cache_data_list_size(event_cache) > max_events) {
    event_cache_data_list_pop_front(event_cache);
  }

  return pdata;
}

/**********************************************************************//**
  Initializes the event cache.
**************************************************************************/
void event_cache_init(void)
{
  if (event_cache != NULL) {
    event_cache_free();
  }
  event_cache = event_cache_data_list_new_full(event_cache_data_free);
  event_cache_status = TRUE;
}

/**********************************************************************//**
  Frees the event cache.
**************************************************************************/
void event_cache_free(void)
{
  if (event_cache != NULL) {
    event_cache_data_list_destroy(event_cache);
    event_cache = NULL;
  }
  event_cache_status = FALSE;
}

/**********************************************************************//**
  Remove all events from the cache.
**************************************************************************/
void event_cache_clear(void)
{
  event_cache_data_list_clear(event_cache);
}

/**********************************************************************//**
  Remove the old events from the cache.
**************************************************************************/
void event_cache_remove_old(void)
{
  struct event_cache_data *current;

  /* This assumes that entries are in order, the ones to be removed first. */
  current = event_cache_data_list_get(event_cache, 0);

    while (current != NULL
           && current->packet.turn + game.server.event_cache.turns <= game.info.turn) {
    event_cache_data_list_pop_front(event_cache);
    current = event_cache_data_list_get(event_cache, 0);
  }
}

/**********************************************************************//**
  Add an event to the cache for all connections.
**************************************************************************/
void event_cache_add_for_all(const struct packet_chat_msg *packet)
{
  if (0 < game.server.event_cache.turns) {
    (void) event_cache_data_new(packet, time(NULL),
                                server_state(), ECT_ALL, NULL);
  }
}

/**********************************************************************//**
  Add an event to the cache for all global observers.
**************************************************************************/
void event_cache_add_for_global_observers(const struct packet_chat_msg *packet)
{
  if (0 < game.server.event_cache.turns) {
    (void) event_cache_data_new(packet, time(NULL),
                                server_state(), ECT_GLOBAL_OBSERVERS, NULL);
  }
}

/**********************************************************************//**
  Add an event to the cache for one player.

  N.B.: event_cache_add_for_player(&packet, NULL) will have the same effect
        as event_cache_add_for_all(&packet).
  N.B.: in pregame, this will never success, because players are not fixed.
**************************************************************************/
void event_cache_add_for_player(const struct packet_chat_msg *packet,
                                const struct player *pplayer)
{
  if (NULL == pplayer) {
    event_cache_add_for_all(packet);
    return;
  }

  if (0 < game.server.event_cache.turns
      && (server_state() > S_S_INITIAL || !game.info.is_new_game)) {
    struct event_cache_data *pdata;

    pdata = event_cache_data_new(packet, time(NULL),
                                 server_state(), ECT_PLAYERS, NULL);
    fc_assert_ret(NULL != pdata);
    BV_SET(pdata->target, player_index(pplayer));
  }
}

/**********************************************************************//**
  Add an event to the cache for selected players.  See
  event_cache_player_add() to see how to select players.  This also
  free the players pointer argument.

  N.B.: in pregame, this will never success, because players are not fixed.
**************************************************************************/
void event_cache_add_for_players(const struct packet_chat_msg *packet,
                                 struct event_cache_players *players)
{
  if (0 < game.server.event_cache.turns
      && NULL != players
      && BV_ISSET_ANY(players->vector)
      && (server_state() > S_S_INITIAL || !game.info.is_new_game)) {
    (void) event_cache_data_new(packet, time(NULL),
                                server_state(), ECT_PLAYERS, players);
  }

  if (NULL != players) {
    free(players);
  }
}

/**********************************************************************//**
  Select players for event_cache_add_for_players().  Pass NULL as players
  argument to create a new selection.  Usually the usage of this function
  would look to:

  struct event_cache_players *players = NULL;

  players_iterate(pplayer) {
    if (some_condition) {
      players = event_cache_player_add(players, pplayer);
    }
  } players_iterate_end;
  // Now add to the cache.
  event_cache_add_for_players(&packet, players); // Free players.
**************************************************************************/
struct event_cache_players *
event_cache_player_add(struct event_cache_players *players,
                       const struct player *pplayer)
{
  if (NULL == players) {
    players = fc_malloc(sizeof(*players));
    BV_CLR_ALL(players->vector);
  }

  if (NULL != pplayer) {
    BV_SET(players->vector, player_index(pplayer));
  }

  return players;
}

/**********************************************************************//**
  Returns whether the event may be displayed for the connection.
**************************************************************************/
static bool event_cache_match(const struct event_cache_data *pdata,
                              const struct player *pplayer,
                              bool is_global_observer,
                              bool include_public)
{
  if (server_state() != pdata->server_state) {
    return FALSE;
  }

  if (server_state() == S_S_RUNNING
      && game.info.turn < pdata->packet.turn
      && game.info.turn > pdata->packet.turn - game.server.event_cache.turns) {
    return FALSE;
  }

  switch (pdata->target_type) {
  case ECT_ALL:
    return include_public;
  case ECT_PLAYERS:
    return (NULL != pplayer
            && BV_ISSET(pdata->target, player_index(pplayer)));
  case ECT_GLOBAL_OBSERVERS:
    return is_global_observer;
  }

  return FALSE;
}

/**********************************************************************//**
  Send all available events.  If include_public is TRUE, also fully global
  message will be sent.
**************************************************************************/
void send_pending_events(struct connection *pconn, bool include_public)
{
  const struct player *pplayer = conn_get_player(pconn);
  bool is_global_observer = conn_is_global_observer(pconn);
  char timestr[64];
  struct packet_chat_msg pcm;

  event_cache_iterate(pdata) {
    if (event_cache_match(pdata, pplayer,
                          is_global_observer, include_public)) {
      if (game.server.event_cache.info) {
        /* add turn and time to the message */
        strftime(timestr, sizeof(timestr), "%H:%M:%S",
                 localtime(&pdata->timestamp));
        pcm = pdata->packet;
        fc_snprintf(pcm.message, sizeof(pcm.message), "(T%d - %s) %s",
                    pdata->packet.turn, timestr, pdata->packet.message);
        notify_conn_packet(pconn->self, &pcm, FALSE);
      } else {
        notify_conn_packet(pconn->self, &pdata->packet, FALSE);
      }
    }
  } event_cache_iterate_end;
}

/**********************************************************************//**
  Load the event cache from a savefile.
**************************************************************************/
void event_cache_load(struct section_file *file, const char *section)
{
  struct packet_chat_msg packet;
  enum event_cache_target target_type;
  enum server_states server_status;
  struct event_cache_players *players = NULL;
  int i, x, y, event_count;
  time_t timestamp, now;
  const char *p, *q;

  event_count = secfile_lookup_int_default(file, 0, "%s.count", section);
  log_verbose("Saved events: %d.", event_count);

  if (0 >= event_count) {
    return;
  }

  now = time(NULL);
  for (i = 0; i < event_count; i++) {
    int turn;
    int phase;

    /* restore packet */
    x = secfile_lookup_int_default(file, -1, "%s.events%d.x", section, i);
    y = secfile_lookup_int_default(file, -1, "%s.events%d.y", section, i);
    packet.tile = (is_normal_map_pos(x, y)
                   ? map_pos_to_index(&(wld.map), x, y) : -1);
    packet.conn_id = -1;

    p = secfile_lookup_str(file, "%s.events%d.event", section, i);
    if (NULL == p) {
      log_verbose("[Event cache %4d] Missing event type.", i);
      continue;
    }
    packet.event = event_type_by_name(p, fc_strcasecmp);
    if (!event_type_is_valid(packet.event)) {
      log_verbose("[Event cache %4d] Not supported event type: %s", i, p);
      continue;
    }

    p = secfile_lookup_str(file, "%s.events%d.message", section, i);
    if (NULL == p) {
      log_verbose("[Event cache %4d] Missing message.", i);
      continue;
    }
    sz_strlcpy(packet.message, p);

    /* restore event cache data */
    turn = secfile_lookup_int_default(file, 0, "%s.events%d.turn",
                                      section, i);
    packet.turn = turn;

    phase = secfile_lookup_int_default(file, PHASE_UNKNOWN, "%s.events%d.phase",
                                       section, i);
    packet.phase = phase;

    timestamp = secfile_lookup_int_default(file, now,
                                           "%s.events%d.timestamp",
                                           section, i);

    p = secfile_lookup_str(file, "%s.events%d.server_state", section, i);
    if (NULL == p) {
      log_verbose("[Event cache %4d] Missing server state info.", i);
      continue;
    }
    server_status = server_states_by_name(p, fc_strcasecmp);
    if (!server_states_is_valid(server_status)) {
      log_verbose("[Event cache %4d] Server state no supported: %s", i, p);
      continue;
    }

    p = secfile_lookup_str(file, "%s.events%d.target", section, i);
    if (NULL == p) {
      log_verbose("[Event cache %4d] Missing target info.", i);
      continue;
    } else if (0 == fc_strcasecmp(p, "All")) {
      target_type = ECT_ALL;
    } else if (0 == fc_strcasecmp(p, "Global Observers")) {
      target_type = ECT_GLOBAL_OBSERVERS;
    } else {
      bool valid = TRUE;

      target_type = ECT_PLAYERS;
      q = p;
      players_iterate(pplayer) {
        if ('1' == *q) {
          players = event_cache_player_add(players, pplayer);
        } else if ('0' != *q) {
          /* a value not '0' or '1' means a corruption of the savegame */
          valid = FALSE;
          break;
        }

        q++;
      } players_iterate_end;

      if (!valid && NULL == players) {
        log_verbose("[Event cache %4d] invalid target bitmap: %s", i, p);
        if (NULL != players) {
          FC_FREE(players);
        }
      }
    }

    /* insert event into the cache */
    (void) event_cache_data_new(&packet, timestamp, server_status,
                                target_type, players);

    if (NULL != players) {
      /* free the event cache player selection */
      FC_FREE(players);
    }

    log_verbose("Event %4d loaded.", i);
  }
}

/**********************************************************************//**
  Save the event cache into the savegame.
**************************************************************************/
void event_cache_save(struct section_file *file, const char *section)
{
  int event_count = 0;

  /* stop event logging; this way events from log_*() will not be added
   * to the event list while saving the event list */
  event_cache_status = FALSE;

  event_cache_iterate(pdata) {
    struct tile *ptile = index_to_tile(&(wld.map), pdata->packet.tile);
    char target[MAX_NUM_PLAYER_SLOTS + 1];
    char *p;
    int tile_x = -1, tile_y = -1;

    if (ptile != NULL) {
      index_to_map_pos(&tile_x, &tile_y, tile_index(ptile));
    }

    secfile_insert_int(file, pdata->packet.turn, "%s.events%d.turn",
                       section, event_count);
    if (pdata->packet.phase != PHASE_UNKNOWN) {
      /* Do not save current value of PHASE_UNKNOWN to savegame.
       * It practically means that "savegame had no phase stored".
       * Note that the only case where phase might be PHASE_UNKNOWN
       * may be present is that the event was loaded from previous
       * savegame created by a freeciv version that did not store event
       * phases. */
      secfile_insert_int(file, pdata->packet.phase, "%s.events%d.phase",
                         section, event_count);
    }
    secfile_insert_int(file, pdata->timestamp, "%s.events%d.timestamp",
                       section, event_count);
    secfile_insert_int(file, tile_x, "%s.events%d.x", section, event_count);
    secfile_insert_int(file, tile_y, "%s.events%d.y", section, event_count);
    secfile_insert_str(file, server_states_name(pdata->server_state),
                       "%s.events%d.server_state", section, event_count);
    secfile_insert_str(file, event_type_name(pdata->packet.event),
                       "%s.events%d.event", section, event_count);
    switch (pdata->target_type) {
    case ECT_ALL:
      fc_snprintf(target, sizeof(target), "All");
      break;
    case ECT_PLAYERS:
      p = target;
      players_iterate(pplayer) {
        *p++ = (BV_ISSET(pdata->target, player_index(pplayer)) ? '1' : '0');
      } players_iterate_end;
      *p = '\0';
    break;
    case ECT_GLOBAL_OBSERVERS:
      fc_snprintf(target, sizeof(target), "Global Observers");
      break;
    }
    secfile_insert_str(file, target, "%s.events%d.target",
                       section, event_count);
    secfile_insert_str(file, pdata->packet.message, "%s.events%d.message",
                       section, event_count);

    log_verbose("Event %4d saved.", event_count);

    event_count++;
  } event_cache_iterate_end;

  /* save the number of events in the event cache */
  secfile_insert_int(file, event_count, "%s.count", section);

  log_verbose("Events saved: %d.", event_count);

  event_cache_status = TRUE;
}

/**********************************************************************//**
  Mark all existing phase values in event cache invalid.
**************************************************************************/
void event_cache_phases_invalidate(void)
{
  event_cache_iterate(pdata) {
    if (pdata->packet.phase >= 0) {
      pdata->packet.phase = PHASE_INVALIDATED;
    }
  } event_cache_iterate_end;
}
