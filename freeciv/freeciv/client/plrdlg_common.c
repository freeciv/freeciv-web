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

#include <string.h>

/* utility */
#include "fcintl.h"
#include "log.h"
#include "support.h"

/* common */
#include "connection.h"
#include "game.h"

/* client */
#include "client_main.h"
#include "climisc.h"
#include "options.h"
#include "text.h"

/* client/include */
#include "plrdlg_g.h"

#include "plrdlg_common.h"


/************************************************************************//**
  The player-name (aka nation leader) column of the plrdlg.
****************************************************************************/
static const char *col_name(const struct player *player)
{
  return player_name(player);
}

/************************************************************************//**
  Compares the names of two players in players dialog.
****************************************************************************/
static int cmp_name(const struct player *pplayer1,
                     const struct player *pplayer2)
{
  return fc_stricoll(player_name(pplayer1), player_name(pplayer2));
}

/************************************************************************//**
  The username (connection name) column of the plrdlg.
****************************************************************************/
static const char *col_username(const struct player *player)
{
  return player->username;
}

/************************************************************************//**
  The name of the player's nation for the plrdlg.
****************************************************************************/
static const char *col_nation(const struct player *player)
{
  return nation_adjective_for_player(player);
}

/************************************************************************//**
  The name of the player's team (or empty) for the plrdlg.
****************************************************************************/
static const char *col_team(const struct player *player)
{
  return team_name_translation(player->team);
}

/************************************************************************//**
  TRUE if the player is AI-controlled.
****************************************************************************/
static bool col_ai(const struct player *plr)
{
  /* TODO: Currently is_ai() is a macro so we can't have it
   *       directly as this callback, but once it's a function,
   *       do that. */
  return is_ai(plr);
}

/************************************************************************//**
  Returns a translated string giving the embassy status
  (none/with us/with them/both).
****************************************************************************/
static const char *col_embassy(const struct player *player)
{
  return get_embassy_status(client.conn.playing, player);
}

/************************************************************************//**
  Returns a translated string giving the diplomatic status
  ("war" or "ceasefire (5)").
****************************************************************************/
static const char *col_diplstate(const struct player *player)
{
  static char buf[100];
  const struct player_diplstate *pds;

  if (NULL == client.conn.playing || player == client.conn.playing) {
    return "-";
  } else {
    pds = player_diplstate_get(client.conn.playing, player);
    if (pds->type == DS_CEASEFIRE || pds->type == DS_ARMISTICE) {
      fc_snprintf(buf, sizeof(buf), "%s (%d)",
                  diplstate_type_translated_name(pds->type),
                  pds->turns_left);
      return buf;
    } else {
      return diplstate_type_translated_name(pds->type);
    }
  }
}

/************************************************************************//**
  Return a numerical value suitable for ordering players by
  their diplomatic status in the players dialog

  A lower value stands for more friendly diplomatic status.
****************************************************************************/
static int diplstate_value(const struct player *plr)
{
  /* these values are scaled so that adding/subtracting
     the number of turns left makes sense */
  static const int diplstate_cmp_lookup[DS_LAST] = {
    [DS_TEAM] = 1 << 16,
    [DS_ALLIANCE] = 2 << 16,
    [DS_PEACE] = 3 << 16,
    [DS_ARMISTICE] = 4 << 16,
    [DS_CEASEFIRE] = 5 << 16,
    [DS_WAR] = 6 << 16,
    [DS_NO_CONTACT] = 7 << 16
  };

  const struct player_diplstate *pds;
  int ds_value;

  if (NULL == client.conn.playing || plr == client.conn.playing) {
    /* current global player is as close as players get
       -> return value smaller than for DS_TEAM */
    return 0;
  }

  pds = player_diplstate_get(client.conn.playing, plr);
  ds_value = diplstate_cmp_lookup[pds->type];

  if (pds->type == DS_ARMISTICE || pds->type == DS_CEASEFIRE) {
    ds_value += pds->turns_left;
  }

  return ds_value;
}

/************************************************************************//**
  Compares diplomatic status of two players in players dialog
****************************************************************************/
static int cmp_diplstate(const struct player *player1,
                         const struct player *player2)
{
  return diplstate_value(player1) - diplstate_value(player2);
}

/************************************************************************//**
  Return a string displaying the AI's love (or not) for you...
****************************************************************************/
static const char *col_love(const struct player *player)
{
  if (NULL == client.conn.playing || player == client.conn.playing
      || is_human(player)) {
    return "-";
  } else {
    return love_text(player->ai_common.love[player_index(client.conn.playing)]);
  }
}

/************************************************************************//**
  Compares ai's attitude toward the player
****************************************************************************/
static int cmp_love(const struct player *player1,
                    const struct player *player2)
{
  int love1, love2;

  if (NULL == client.conn.playing) {
    return player_number(player1) - player_number(player2);
  }

  if (player1 == client.conn.playing || is_human(player1)) {
    love1 = MAX_AI_LOVE + 999;
  } else {
    love1 = player1->ai_common.love[player_index(client.conn.playing)];
  }

  if (player2 == client.conn.playing || is_human(player2)) {
    love2 = MAX_AI_LOVE + 999;
  } else {
    love2 = player2->ai_common.love[player_index(client.conn.playing)];
  }
  
  return love1 - love2;
}

/************************************************************************//**
  Returns a translated string giving our shared-vision status.
****************************************************************************/
static const char *col_vision(const struct player *player)
{
  return get_vision_status(client.conn.playing, player);
}

/************************************************************************//**
  Returns a translated string giving the player's "state".

  FIXME: These terms aren't very intuitive for new players.
****************************************************************************/
const char *plrdlg_col_state(const struct player *plr)
{
  if (!plr->is_alive) {
    /* TRANS: Dead -- Rest In Peace -- Reqia In Pace */
    return _("R.I.P.");
  } else if (!plr->is_connected) {
    struct option *opt;
    bool consider_tb = FALSE;

    if (is_ai(plr)) {
      return "";
    }

    opt = optset_option_by_name(server_optset, "turnblock");
    if (opt != NULL) {
      consider_tb = option_bool_get(opt);
    }

    if (!consider_tb) {
      /* TRANS: No connection */
      return _("noconn");
    }

    if (!is_player_phase(plr, game.info.phase)) {
      return _("waiting");
    } else if (plr->phase_done) {
      return _("done");
    } else {
      /* TRANS: Turnblocking & player not connected */
      return _("blocking");
    }
  } else {
    if (!is_player_phase(plr, game.info.phase)) {
      return _("waiting");
    } else if (plr->phase_done) {
      return _("done");
    } else {
      return _("moving");
    }
  }
}

/************************************************************************//**
  Returns a string telling the player's client's hostname (the
  machine from which he is connecting).
****************************************************************************/
static const char *col_host(const struct player *player)
{
  return player_addr_hack(player);
}

/************************************************************************//**
  Returns a string telling how many turns the player has been idle.
****************************************************************************/
static const char *col_idle(const struct player *plr)
{
  int idle;
  static char buf[100];

  if (plr->nturns_idle > 3) {
    idle = plr->nturns_idle - 1;
  } else {
    idle = 0;
  }
  fc_snprintf(buf, sizeof(buf), "%d", idle);
  return buf;
}

/************************************************************************//**
 Compares score of two players in players dialog
****************************************************************************/
static int cmp_score(const struct player* player1,
                     const struct player* player2)
{
  return player1->score.game - player2->score.game;
}

/****************************************************************************
  ...
****************************************************************************/
struct player_dlg_column player_dlg_columns[] = {
  {TRUE, COL_TEXT, N_("?Player:Name"), col_name, NULL, cmp_name, "name"},
  {FALSE, COL_TEXT, N_("Username"), col_username, NULL, NULL, "username"},
  {TRUE, COL_FLAG, N_("Flag"), NULL, NULL, NULL,  "flag"},
  {TRUE, COL_TEXT, N_("Nation"), col_nation, NULL, NULL,  "nation"},
  {TRUE, COL_COLOR, N_("Border"), NULL, NULL, NULL,  "border"},
  {TRUE, COL_RIGHT_TEXT, N_("Score"), get_score_text, NULL, cmp_score, "score"},
  {TRUE, COL_TEXT, N_("Team"), col_team, NULL, NULL,  "team"},
  {TRUE, COL_BOOLEAN, N_("AI"), NULL, col_ai, NULL,  "ai"},
  {TRUE, COL_TEXT, N_("Attitude"), col_love, NULL, cmp_love,  "attitude"},
  {TRUE, COL_TEXT, N_("Embassy"), col_embassy, NULL, NULL,  "embassy"},
  {TRUE, COL_TEXT, N_("Dipl.State"), col_diplstate, NULL, cmp_diplstate,
    "diplstate"},
  {TRUE, COL_TEXT, N_("Vision"), col_vision, NULL, NULL,  "vision"},
  {TRUE, COL_TEXT, N_("State"), plrdlg_col_state, NULL, NULL,  "state"},
  {FALSE, COL_TEXT, N_("?Player_dlg:Host"), col_host, NULL, NULL,  "host"},
  {FALSE, COL_RIGHT_TEXT, N_("?Player_dlg:Idle"), col_idle, NULL, NULL,  "idle"},
  {FALSE, COL_RIGHT_TEXT, N_("Ping"), get_ping_time_text, NULL, NULL,  "ping"}
};

const int num_player_dlg_columns = ARRAY_SIZE(player_dlg_columns);

/************************************************************************//**
  Return default player dlg sorting column.
****************************************************************************/
int player_dlg_default_sort_column(void)
{
  return 3;
}

/************************************************************************//**
  Translate all titles
****************************************************************************/
void init_player_dlg_common()
{
  int i;

  for (i = 0; i < num_player_dlg_columns; i++) {
    player_dlg_columns[i].title = Q_(player_dlg_columns[i].title);
  }
}

/************************************************************************//**
  The only place where this is used is the player dialog.
  Eventually this should go the way of the dodo with everything here
  moved into col_host above, but some of the older clients (+win32) still
  use this function directly.

  This code in this function is only really needed so that the host is
  kept as a blank address if no one is controlling a player, but there are
  observers.
****************************************************************************/
const char *player_addr_hack(const struct player *pplayer)
{ 
  conn_list_iterate(pplayer->connections, pconn) {
    if (!pconn->observer) {
      return pconn->addr;
    }
  } conn_list_iterate_end;

  return blank_addr_str;
}   
