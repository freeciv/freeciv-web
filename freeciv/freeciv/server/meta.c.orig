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

#include "fc_prehdrs.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

/* utility */
#include "fcintl.h"
#include "fcthread.h"
#include "log.h"
#include "mem.h"
#include "netintf.h"
#include "netfile.h"
#include "support.h"
#include "timing.h"

/* common */
#include "capstr.h"
#include "connection.h"
#include "dataio.h"
#include "game.h"
#include "map.h"
#include "version.h"

/* server */
#include "console.h"
#include "plrhand.h"
#include "settings.h"
#include "srv_main.h"

#include "meta.h"

static bool server_is_open = FALSE;
static bool persistent_meta_connection = FALSE;
static int meta_retry_wait = 0;

static char meta_patches[256] = "";
static char meta_message[256] = "";

static fc_thread *meta_srv_thread = NULL;

/*********************************************************************//**
  The default metaserver patches for this server
*************************************************************************/
const char *default_meta_patches_string(void)
{
  return "none";
}

/*********************************************************************//**
  Return static string with default info line to send to metaserver.
*************************************************************************/
const char *default_meta_message_string(void)
{
#if IS_BETA_VERSION
  return "unstable pre-" NEXT_STABLE_VERSION ": beware";
#else  /* IS_BETA_VERSION */
#if IS_DEVEL_VERSION
  return "development version: beware";
#else  /* IS_DEVEL_VERSION */
  return "-";
#endif /* IS_DEVEL_VERSION */
#endif /* IS_BETA_VERSION */
}

/*********************************************************************//**
  The metaserver patches
*************************************************************************/
const char *get_meta_patches_string(void)
{
  return meta_patches;
}

/*********************************************************************//**
  The metaserver message
*************************************************************************/
const char *get_meta_message_string(void)
{
  return meta_message;
}

/*********************************************************************//**
  The server metaserver type
*************************************************************************/
static const char *get_meta_type_string(void)
{
  if (game.server.meta_info.type[0] != '\0') {
    return game.server.meta_info.type;
  }

  return NULL;
}

/*********************************************************************//**
  The metaserver message set by user
*************************************************************************/
const char *get_user_meta_message_string(void)
{
  if (game.server.meta_info.user_message[0] != '\0') {
    return game.server.meta_info.user_message;
  }

  return NULL;
}

/*********************************************************************//**
  Update meta message. Set it to user meta message, if it is available.
  Otherwise use provided message.
  It is ok to call this with NULL message. Then it only replaces current
  meta message with user meta message if available.
*************************************************************************/
void maybe_automatic_meta_message(const char *automatic)
{
  const char *user_message;

  user_message = get_user_meta_message_string();

  if (user_message == NULL) {
    /* No user message */
    if (automatic != NULL) {
      set_meta_message_string(automatic);
    }
    return;
  }

  set_meta_message_string(user_message);
}

/*********************************************************************//**
  Set the metaserver patches string
*************************************************************************/
void set_meta_patches_string(const char *string)
{
  sz_strlcpy(meta_patches, string);
}

/*********************************************************************//**
  Set the metaserver message string
*************************************************************************/
void set_meta_message_string(const char *string)
{
  sz_strlcpy(meta_message, string);
}

/*********************************************************************//**
  Set user defined metaserver message string
*************************************************************************/
void set_user_meta_message_string(const char *string)
{
  if (string != NULL && string[0] != '\0') {
    sz_strlcpy(game.server.meta_info.user_message, string);
    set_meta_message_string(string);
  } else {
    /* Remove user meta message. We will use automatic messages instead */
    game.server.meta_info.user_message[0] = '\0';
    set_meta_message_string(default_meta_message_string());    
  }
}

/*********************************************************************//**
  Return string describing both metaserver name and port.
*************************************************************************/
char *meta_addr_port(void)
{
  return srvarg.metaserver_addr;
}

/*********************************************************************//**
  We couldn't find or connect to the metaserver.
*************************************************************************/
static void metaserver_failed(void)
{
  if (!persistent_meta_connection) {
    con_puts(C_METAERROR, _("Not reporting to the metaserver in this game."));
    con_flush();

    server_close_meta();
  } else {
    con_puts(C_METAERROR, _("Metaserver connection currently failing."));
    meta_retry_wait = 1;
  }
}

/*********************************************************************//**
  Insert a setting in the metaserver message. Return TRUE if it succeded.
*************************************************************************/
static inline bool meta_insert_setting(struct netfile_post *post,
                                       const char *set_name)
{
  const struct setting *pset = setting_by_name(set_name);
  char buf[256];

  fc_assert_ret_val_msg(NULL != pset, FALSE,
                        "Setting \"%s\" not found!", set_name);
  netfile_add_form_str(post, "vn[]", setting_name(pset));
  netfile_add_form_str(post, "vv[]",
                       setting_value_name(pset, FALSE, buf, sizeof(buf)));
  return TRUE;
}

/*********************************************************************//**
  Send POST to metaserver. This runs in its own thread.
*************************************************************************/
static void send_metaserver_post(void *arg)
{
  struct netfile_post *post = (struct netfile_post *) arg;
  char *addr;

  if (srvarg.bind_meta_addr != NULL) {
    addr = srvarg.bind_meta_addr;
  } else {
    addr = srvarg.bind_addr;
  }

  if (!netfile_send_post(srvarg.metaserver_addr, post, NULL, NULL, addr)) {
    con_puts(C_METAERROR, _("Error connecting to metaserver"));
    metaserver_failed();
  }

  netfile_close_post(post);
}

/*********************************************************************//**
  Construct the POST message and send info to metaserver.
*************************************************************************/
static bool send_to_metaserver(enum meta_flag flag)
{
  int players = 0;
  int humans = 0;
  char host[512];
  char state[20];
  char rs[256];
  struct netfile_post *post;

  switch(server_state()) {
  case S_S_INITIAL:
    sz_strlcpy(state, "Pregame");
    break;
  case S_S_RUNNING:
    sz_strlcpy(state, "Running");
    break;
  case S_S_OVER:
    sz_strlcpy(state, "Game Ended");
    break;
  }

  /* get hostname */
  if (srvarg.identity_name[0] != '\0') {
    sz_strlcpy(host, srvarg.identity_name);
  } else if (fc_gethostname(host, sizeof(host)) != 0) {
    sz_strlcpy(host, "unknown");
  }

  if (game.control.version[0] != '\0') {
    fc_snprintf(rs, sizeof(rs), "%s %s", game.control.name, game.control.version);
  } else {
    sz_strlcpy(rs, game.control.name);
  }

  /* Freed in metaserver thread function send_metaserver_post() */
  post = netfile_start_post();

  netfile_add_form_str(post, "host", host);
  netfile_add_form_int(post, "port", srvarg.port);
  netfile_add_form_str(post, "state", state);
  netfile_add_form_str(post, "ruleset", rs);

  if (flag == META_GOODBYE) {
    netfile_add_form_int(post, "bye", 1);
  } else {
    const char *srvtype = get_meta_type_string();

    if (srvtype != NULL) {
      netfile_add_form_str(post, "type", srvtype);
    }
    netfile_add_form_str(post, "version", VERSION_STRING);
    netfile_add_form_str(post, "patches",
                         get_meta_patches_string());
    netfile_add_form_str(post, "capability", our_capability);

    netfile_add_form_str(post, "serverid", srvarg.serverid);
    netfile_add_form_str(post, "message",
                         get_meta_message_string());

    /* NOTE: send info for ALL players or none at all. */
    if (normal_player_count() == 0) {
      netfile_add_form_int(post, "dropplrs", 1);
    } else {
      players = 0; /* a counter for players_available */
      humans = 0;

      players_iterate(plr) {
        bool is_player_available = TRUE;
        char type[15];
        struct connection *pconn = conn_by_user(plr->username);

        if (!plr->is_alive) {
          sz_strlcpy(type, "Dead");
        } else if (is_barbarian(plr)) {
          sz_strlcpy(type, "Barbarian");
        } else if (is_ai(plr)) {
          sz_strlcpy(type, "A.I.");
        } else if (is_human(plr)) {
          sz_strlcpy(type, "Human");
        } else {
          sz_strlcpy(type, "-");
        }

        netfile_add_form_str(post, "plu[]", plr->username);
        netfile_add_form_str(post, "plt[]", type);
        netfile_add_form_str(post, "pll[]", player_name(plr));
        netfile_add_form_str(post, "pln[]",
                             plr->nation != NO_NATION_SELECTED 
                             ? nation_plural_for_player(plr)
                             : "none");
        netfile_add_form_str(post, "plf[]",
                             plr->nation != NO_NATION_SELECTED 
                             ? nation_of_player(plr)->flag_graphic_str
                             : "none");
        netfile_add_form_str(post, "plh[]",
                             pconn ? pconn->addr : "");

        /* is this player available to take?
         * TODO: there's some duplication here with 
         * stdinhand.c:is_allowed_to_take() */
        if (is_barbarian(plr) && !strchr(game.server.allow_take, 'b')) {
          is_player_available = FALSE;
        } else if (!plr->is_alive && !strchr(game.server.allow_take, 'd')) {
          is_player_available = FALSE;
        } else if (is_ai(plr)
                   && !strchr(game.server.allow_take,
                              (game.info.is_new_game ? 'A' : 'a'))) {
          is_player_available = FALSE;
        } else if (is_human(plr)
                   && !strchr(game.server.allow_take,
                              (game.info.is_new_game ? 'H' : 'h'))) {
          is_player_available = FALSE;
        }

        if (pconn) {
          is_player_available = FALSE;
        }

        if (is_player_available) {
          players++;
        }

        if (is_human(plr) && plr->is_alive) {
          humans++;
        }
      } players_iterate_end;

      /* send the number of available players. */
      netfile_add_form_int(post, "available", players);
      netfile_add_form_int(post, "humans", humans);
    }

    /* Send some variables: should be listed in inverted order? */
    {
      static const char *settings[] = {
        "timeout", "endturn", "minplayers", "maxplayers",
        "aifill", "allowtake", "generator"
      };
      int i;

      for (i = 0; i < ARRAY_SIZE(settings); i++) {
        meta_insert_setting(post, settings[i]);
      }

      /* HACK: send the most determinant setting for the map size. */
      switch (wld.map.server.mapsize) {
      case MAPSIZE_FULLSIZE:
        meta_insert_setting(post, "size");
        break;
      case MAPSIZE_PLAYER:
        meta_insert_setting(post, "tilesperplayer");
        break;
      case MAPSIZE_XYSIZE:
        meta_insert_setting(post, "xsize");
        meta_insert_setting(post, "ysize");
        break;
      }
    }

    /* Turn and year. */
    netfile_add_form_str(post, "vn[]", "turn");
    netfile_add_form_int(post, "vv[]", game.info.turn);
    netfile_add_form_str(post, "vn[]", "year");

    if (server_state() != S_S_INITIAL) {
      netfile_add_form_int(post, "vv[]", game.info.year);
    } else {
      netfile_add_form_str(post, "vv[]", "Calendar not set up");
    }
  }

  if (meta_srv_thread != NULL) {
    /* Previously started thread */
    fc_thread_wait(meta_srv_thread);
  } else {
    meta_srv_thread = fc_malloc(sizeof(meta_srv_thread));
  }

  /* Send POST in new thread */
  fc_thread_start(meta_srv_thread, &send_metaserver_post, post);

  return TRUE;
}

/*********************************************************************//**
  Stop sending updates to metaserver
*************************************************************************/
void server_close_meta(void)
{
  server_is_open = FALSE;
  persistent_meta_connection = FALSE;
}

/*********************************************************************//**
  Lookup the correct address for the metaserver.
*************************************************************************/
bool server_open_meta(bool persistent)
{
  if (meta_patches[0] == '\0') {
    set_meta_patches_string(default_meta_patches_string());
  }
  if (meta_message[0] == '\0') {
    set_meta_message_string(default_meta_message_string());
  }

  server_is_open = TRUE;
  persistent_meta_connection = persistent;
  meta_retry_wait = 0;

  return TRUE;
}

/*********************************************************************//**
  Are we sending info to the metaserver?
*************************************************************************/
bool is_metaserver_open(void)
{
  return server_is_open;
}

/*********************************************************************//**
  Control when we send info to the metaserver.
*************************************************************************/
bool send_server_info_to_metaserver(enum meta_flag flag)
{
  static struct timer *last_send_timer = NULL;
  static bool want_update;

  if (!server_is_open) {
    return FALSE;
  }

  /* Persistent connection temporary failures handling */
  if (meta_retry_wait > 0) {
    if (meta_retry_wait++ > 5) {
      meta_retry_wait = 0;
    } else {
      return FALSE;
    }
  }

  /* if we're bidding farewell, ignore all timers */
  if (flag == META_GOODBYE) { 
    if (last_send_timer) {
      timer_destroy(last_send_timer);
      last_send_timer = NULL;
    }
    send_to_metaserver(flag);

    /* Wait metaserver thread to finish */
    fc_thread_wait(meta_srv_thread);
    free(meta_srv_thread);
    meta_srv_thread = NULL;

    return TRUE;
  }

  /* don't allow the user to spam the metaserver with updates */
  if (last_send_timer && (timer_read_seconds(last_send_timer)
                                          < METASERVER_MIN_UPDATE_INTERVAL)) {
    if (flag == META_INFO) {
      want_update = TRUE; /* we couldn't update now, but update a.s.a.p. */
    }
    return FALSE;
  }

  /* if we're asking for a refresh, only do so if 
   * we've exceeded the refresh interval */
  if ((flag == META_REFRESH) && !want_update && last_send_timer 
      && (timer_read_seconds(last_send_timer) < METASERVER_REFRESH_INTERVAL)) {
    return FALSE;
  }

  /* start a new timer if we haven't already */
  if (!last_send_timer) {
    last_send_timer = timer_new(TIMER_USER, TIMER_ACTIVE);
  }

  timer_clear(last_send_timer);
  timer_start(last_send_timer);
  want_update = FALSE;

  return send_to_metaserver(flag);
}
