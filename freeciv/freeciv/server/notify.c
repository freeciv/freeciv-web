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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdarg.h>

/* utility */
#include "log.h"

/* common */
#include "events.h"
#include "featured_text.h"
#include "game.h"
#include "packets.h"
#include "player.h"
#include "tile.h"

/* server */
#include "maphand.h"
#include "srv_main.h"

#include "notify.h"

/**************************************************************************
  Fill a packet_chat_msg structure.

  packet: A pointer to the packet.
  ptile: A pointer to a tile the event is occuring.
  event: The event type.
  pconn: The sender of the event (e.g. when event is E_CHAT_MSG).
  fg_color: The requested foreground color or NULL if not requested.
  bg_color: The requested background color or NULL if not requested.
  format: The format of the message.
  vargs: The extra arguments to build the message.
**************************************************************************/
static void package_event_full(struct packet_chat_msg *packet,
                               const struct tile *ptile,
                               enum event_type event,
                               const struct connection *pconn,
                               const char *fg_color, const char *bg_color,
                               const char *format, va_list vargs)
{
  RETURN_IF_FAIL(NULL != packet);

  if (ptile) {
    packet->x = ptile->x;
    packet->y = ptile->y;
  } else {
    packet->x = -1;
    packet->y = -1;
  }
  packet->event = event;
  packet->conn_id = pconn ? pconn->id : -1;

  if ((fg_color && fg_color[0] != '\0')
      || (bg_color && bg_color[0] != '\0')) {
    /* A color is requested. */
    char buf[MAX_LEN_MSG];

    my_vsnprintf(buf, sizeof(buf), format, vargs);
    featured_text_apply_tag(buf, packet->message, sizeof(packet->message),
                            TTT_COLOR, 0, OFFSET_UNSET, fg_color, bg_color);
  } else {
    /* Simple case */
    my_vsnprintf(packet->message, sizeof(packet->message), format, vargs);
  }
}

/**************************************************************************
  Fill a packet_chat_msg structure for a chat message.

  packet: A pointer to the packet.
  sender: The sender of the message.
  fg_color: The requested foreground color or NULL if not requested.
  bg_color: The requested background color or NULL if not requested.
  format: The format of the message.
  vargs: The extra arguments to build the message.
**************************************************************************/
void vpackage_chat_msg(struct packet_chat_msg *packet,
                       const struct connection *sender,
                       const char *fg_color, const char *bg_color,
                       const char *format, va_list vargs)
{
  package_event_full(packet, NULL, E_CHAT_MSG, sender,
                     fg_color, bg_color, format, vargs);
}

/**************************************************************************
  Fill a packet_chat_msg structure for a chat message.

  packet: A pointer to the packet.
  sender: The sender of the message.
  fg_color: The requested foreground color or NULL if not requested.
  bg_color: The requested background color or NULL if not requested.
  format: The format of the message.
  ...: The extra arguments to build the message.
**************************************************************************/
void package_chat_msg(struct packet_chat_msg *packet,
                      const struct connection *sender,
                      const char *fg_color, const char *bg_color,
                      const char *format, ...)
{
  va_list args;

  va_start(args, format);
  vpackage_chat_msg(packet, sender, fg_color, bg_color, format, args);
  va_end(args);
}

/**************************************************************************
  Fill a packet_chat_msg structure for common server event.

  packet: A pointer to the packet.
  ptile: A pointer to a tile the event is occuring.
  event: The event type.
  fg_color: The requested foreground color or NULL if not requested.
  bg_color: The requested background color or NULL if not requested.
  format: The format of the message.
  vargs: The extra arguments to build the message.
**************************************************************************/
void vpackage_event(struct packet_chat_msg *packet,
                    const struct tile *ptile, enum event_type event,
                    const char *fg_color, const char *bg_color,
                    const char *format, va_list vargs)
{
  package_event_full(packet, ptile, event, NULL,
                     fg_color, bg_color, format, vargs);
}

/**************************************************************************
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
                               const struct packet_chat_msg *packet)
{
  struct packet_chat_msg real_packet = *packet;
  struct tile *ptile;

  if (!dest) {
    dest = game.est_connections;
  }

  if (is_normal_map_pos(packet->x, packet->y)) {
    ptile = map_pos_to_tile(packet->x, packet->y);
  } else {
    ptile = NULL;
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
      /* coordinates are OK; see above */
      real_packet.x = ptile->x;
      real_packet.y = ptile->y;
    } else {
      /* no coordinates */
      assert(S_S_RUNNING > server_state() || !is_normal_map_pos(-1, -1));
      real_packet.x = -1;
      real_packet.y = -1;
    }

    send_packet_chat_msg(pconn, &real_packet);
  } conn_list_iterate_end;
}

/**************************************************************************
  See notify_conn_packet - this is just the "non-v" version, with varargs.
**************************************************************************/
void notify_conn(struct conn_list *dest, const struct tile *ptile,
                 enum event_type event, const char *fg_color,
                 const char *bg_color, const char *format, ...)
{
  struct packet_chat_msg genmsg;
  va_list args;

  va_start(args, format);
  vpackage_event(&genmsg, ptile, event, fg_color, bg_color, format, args);

  va_end(args);

  notify_conn_packet(dest, &genmsg);
}

/**************************************************************************
  Similar to notify_conn_packet (see also), but takes player as "destination".
  If player != NULL, sends to all connections for that player.
  If player == NULL, sends to all game connections, to support
  old code, but this feature may go away - should use notify_conn(NULL)
  instead.
**************************************************************************/
void notify_player(const struct player *pplayer, const struct tile *ptile,
                   enum event_type event, const char *fg_color,
                   const char *bg_color, const char *format, ...) 
{
  struct conn_list *dest = pplayer ? pplayer->connections : NULL;
  struct packet_chat_msg genmsg;
  va_list args;

  va_start(args, format);
  vpackage_event(&genmsg, ptile, event, fg_color, bg_color, format, args);
  va_end(args);

  notify_conn_packet(dest, &genmsg);
}

/**************************************************************************
  Send message to all players who have an embassy with pplayer,
  but excluding pplayer and specified player.
**************************************************************************/
void notify_embassies(const struct player *pplayer,
                      const struct player *exclude,
                      const struct tile *ptile, enum event_type event,
                      const char *fg_color, const char *bg_color,
                      const char *format, ...) 
{
  struct packet_chat_msg genmsg;
  va_list args;

  va_start(args, format);
  vpackage_event(&genmsg, ptile, event, fg_color, bg_color, format, args);
  va_end(args);

  players_iterate(other_player) {
    if (player_has_embassy(other_player, pplayer)
        && exclude != other_player
        && pplayer != other_player) {
      notify_conn_packet(other_player->connections, &genmsg);
    }
  } players_iterate_end;
}

/**************************************************************************
  Sends a message to all players on pplayer's team. If 'pplayer' is NULL,
  sends to all players.
**************************************************************************/
void notify_team(const struct player *pplayer, const struct tile *ptile,
                 enum event_type event, const char *fg_color,
                 const char *bg_color, const char *format, ...)
{
  struct conn_list *dest = game.est_connections;
  struct packet_chat_msg genmsg;
  va_list args;

  va_start(args, format);
  vpackage_event(&genmsg, ptile, event, fg_color, bg_color, format, args);
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
    } players_iterate_end;
  }

  notify_conn_packet(dest, &genmsg);

  if (pplayer) {
    conn_list_free(dest);
  }
}

/****************************************************************************
  Sends a message to all players that share research with pplayer.  Currently
  this is all players on the same team but it may not always be that way.

  Unlike other notify functions this one does not take a tile argument.  We
  assume no research message will have a tile associated.
****************************************************************************/
void notify_research(const struct player *pplayer,
                     enum event_type event, const char *fg_color,
                     const char *bg_color, const char *format, ...)
{
  struct packet_chat_msg genmsg;
  va_list args;
  struct player_research *research = get_player_research(pplayer);

  va_start(args, format);
  vpackage_event(&genmsg, NULL, event, fg_color, bg_color, format, args);
  va_end(args);

  players_iterate(other_player) {
    if (get_player_research(other_player) == research) {
      lsend_packet_chat_msg(other_player->connections, &genmsg);
    }
  } players_iterate_end;
}
