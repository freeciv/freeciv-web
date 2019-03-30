/***********************************************************************
 Freeciv - Copyright (C) 1996-2015 - Freeciv Development Team
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

/* common/scriptcore */
#include "luascript.h"

/* ai */
#include "aitraits.h" /* ai_trait_get_value() */

/* server/scripting */
#include "script_server.h"

#include "api_server_game_methods.h"

/*************************************************************************//**
  Return the current value of an AI trait in force (base+mod)
*****************************************************************************/
int api_methods_player_trait(lua_State *L, Player *pplayer,
                             const char *tname)
{
  enum trait tr;

  LUASCRIPT_CHECK_STATE(L, -1);
  LUASCRIPT_CHECK_SELF(L, pplayer, -1);
  LUASCRIPT_CHECK_ARG_NIL(L, tname, 3, string, 0);

  tr = trait_by_name(tname, fc_strcasecmp);

  LUASCRIPT_CHECK_ARG(L, trait_is_valid(tr), 3, "no such trait", 0);

  return ai_trait_get_value(tr, pplayer);
}

/*************************************************************************//**
  Return the current base value of an AI trait (not including Lua mod)
*****************************************************************************/
int api_methods_player_trait_base(lua_State *L, Player *pplayer,
                                  const char *tname)
{
  enum trait tr;

  LUASCRIPT_CHECK_STATE(L, -1);
  LUASCRIPT_CHECK_SELF(L, pplayer, -1);
  LUASCRIPT_CHECK_ARG_NIL(L, tname, 3, string, 0);

  tr = trait_by_name(tname, fc_strcasecmp);

  LUASCRIPT_CHECK_ARG(L, trait_is_valid(tr), 3, "no such trait", 0);

  return pplayer->ai_common.traits[tr].val;
}

/*************************************************************************//**
  Return the current Lua increment to an AI trait
  (can be changed with api_edit_trait_mod_set())
*****************************************************************************/
int api_methods_player_trait_current_mod(lua_State *L, Player *pplayer,
                                         const char *tname)
{
  enum trait tr;

  LUASCRIPT_CHECK_STATE(L, -1);
  LUASCRIPT_CHECK_SELF(L, pplayer, -1);
  LUASCRIPT_CHECK_ARG_NIL(L, tname, 3, string, 0);

  tr = trait_by_name(tname, fc_strcasecmp);

  LUASCRIPT_CHECK_ARG(L, trait_is_valid(tr), 3, "no such trait", 0);

  return pplayer->ai_common.traits[tr].mod;
}

/*************************************************************************//**
  Return the minimum random trait value that will be allocated for a nation
*****************************************************************************/
int api_methods_nation_trait_min(lua_State *L, Nation_Type *pnation,
                                 const char *tname)
{
  enum trait tr;

  LUASCRIPT_CHECK_STATE(L, -1);
  LUASCRIPT_CHECK_SELF(L, pnation, -1);
  LUASCRIPT_CHECK_ARG_NIL(L, tname, 3, string, 0);

  tr = trait_by_name(tname, fc_strcasecmp);

  LUASCRIPT_CHECK_ARG(L, trait_is_valid(tr), 3, "no such trait", 0);

  return pnation->server.traits[tr].min;
}

/*************************************************************************//**
  Return the maximum random trait value that will be allocated for a nation
*****************************************************************************/
int api_methods_nation_trait_max(lua_State *L, Nation_Type *pnation,
                                 const char *tname)
{
  enum trait tr;

  LUASCRIPT_CHECK_STATE(L, -1);
  LUASCRIPT_CHECK_SELF(L, pnation, -1);
  LUASCRIPT_CHECK_ARG_NIL(L, tname, 3, string, 0);

  tr = trait_by_name(tname, fc_strcasecmp);

  LUASCRIPT_CHECK_ARG(L, trait_is_valid(tr), 3, "no such trait", 0);

  return pnation->server.traits[tr].max;
}

/*************************************************************************//**
  Return the default trait value that will be allocated for a nation
*****************************************************************************/
int api_methods_nation_trait_default(lua_State *L, Nation_Type *pnation,
                                     const char *tname)
{
  enum trait tr;

  LUASCRIPT_CHECK_STATE(L, -1);
  LUASCRIPT_CHECK_SELF(L, pnation, -1);
  LUASCRIPT_CHECK_ARG_NIL(L, tname, 3, string, 0);

  tr = trait_by_name(tname, fc_strcasecmp);

  LUASCRIPT_CHECK_ARG(L, trait_is_valid(tr), 3, "no such trait", 0);

  return pnation->server.traits[tr].fixed;
}
