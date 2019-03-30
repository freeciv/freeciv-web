/*****************************************************************************
 Freeciv - Copyright (C) 2005 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*****************************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

/* common/scriptcore */
#include "luascript.h"

/* server */
#include "savegame.h"
#include "score.h"
#include "settings.h"
#include "srv_main.h"

/* server/scripting */
#include "script_server.h"

#include "api_server_base.h"

/*************************************************************************//**
  Return the civilization score (total) for player
*****************************************************************************/
int api_server_player_civilization_score(lua_State *L, Player *pplayer)
{
  LUASCRIPT_CHECK_STATE(L, 0);
  LUASCRIPT_CHECK_SELF(L, pplayer, 0);

  return get_civ_score(pplayer);
}

/*************************************************************************//**
  Returns TRUE if the game was started.
*****************************************************************************/
bool api_server_was_started(lua_State *L)
{
  LUASCRIPT_CHECK_STATE(L, FALSE);

  return game_was_started();
}

/*************************************************************************//**
  Save the game (a manual save is triggered).
*****************************************************************************/
bool api_server_save(lua_State *L, const char *filename)
{
  LUASCRIPT_CHECK_STATE(L, FALSE);

  /* Limit the allowed characters in the filename. */
  if (filename != NULL && !is_safe_filename(filename)) {
    return FALSE;
  }

  save_game(filename, "User request (Lua)", FALSE);

  return TRUE;
}

/*************************************************************************//**
  Play music track for player
*****************************************************************************/
bool api_play_music(lua_State *L, Player *pplayer, const char *tag)
{
  struct packet_play_music p;

  LUASCRIPT_CHECK_STATE(L, FALSE);
  LUASCRIPT_CHECK_SELF(L, pplayer, FALSE);
  LUASCRIPT_CHECK_ARG_NIL(L, tag, 3, API_TYPE_STRING, FALSE);

  sz_strlcpy(p.tag, tag);

  lsend_packet_play_music(pplayer->connections, &p);

  return TRUE;
}

/*************************************************************************//**
  Show image with music for player
*****************************************************************************/
bool api_showimg_playsnd(lua_State *L, Player *pplayer,
                         const char *img_filename, const char *snd_filename,
                         const char *desc, bool fullsize)
{
  struct packet_show_img_play_sound p;

  LUASCRIPT_CHECK_STATE(L, FALSE);
  LUASCRIPT_CHECK_SELF(L, pplayer, FALSE);
  LUASCRIPT_CHECK_ARG_NIL(L, img_filename, 2, API_TYPE_STRING, FALSE);
  LUASCRIPT_CHECK_ARG_NIL(L, img_filename, 3, API_TYPE_STRING, FALSE);

  if (img_filename != NULL) {
    sz_strlcpy(p.img_path, img_filename);
  } else {
    sz_strlcpy(p.img_path, "");
  }
  if (snd_filename != NULL) {
    sz_strlcpy(p.snd_path, snd_filename);
  } else {
    sz_strlcpy(p.snd_path, "");
  }
  if (snd_filename != NULL) {
    sz_strlcpy(p.desc, desc);
  } else {
    sz_strlcpy(p.desc, "");
  }
  p.fullsize = fullsize;

  lsend_packet_show_img_play_sound(pplayer->connections, &p);

  return TRUE;
}

/*************************************************************************//**
  Return the formated value of the setting or NULL if no such setting exists,
*****************************************************************************/
const char *api_server_setting_get(lua_State *L, const char *sett_name)
{
  struct setting *pset;
  static char buf[512];

  LUASCRIPT_CHECK_STATE(L, NULL);
  LUASCRIPT_CHECK_ARG_NIL(L, sett_name, 2, API_TYPE_STRING, NULL);

  pset = setting_by_name(sett_name);

  if (!pset) {
    return NULL;
  }

  return setting_value_name(pset, FALSE, buf, sizeof(buf));
}
