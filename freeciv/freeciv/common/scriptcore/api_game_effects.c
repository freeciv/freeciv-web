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

/* common */
#include "effects.h"

/* common/scriptcore */
#include "luascript.h"

#include "api_game_effects.h"

/*************************************************************************//**
  Returns the effect bonus in the world
*****************************************************************************/
int api_effects_world_bonus(lua_State *L, const char *effect_type)
{
  enum effect_type etype = EFT_COUNT;

  LUASCRIPT_CHECK_STATE(L, 0);
  LUASCRIPT_CHECK_ARG_NIL(L, effect_type, 2, string, 0);

  etype = effect_type_by_name(effect_type, fc_strcasecmp);
  if (!effect_type_is_valid(etype)) {
    return 0;
  }
  return get_world_bonus(etype);
}

/*************************************************************************//**
  Returns the effect bonus for a player
*****************************************************************************/
int api_effects_player_bonus(lua_State *L, Player *pplayer,
                             const char *effect_type)
{
  enum effect_type etype = EFT_COUNT;

  LUASCRIPT_CHECK_STATE(L, 0);
  LUASCRIPT_CHECK_ARG_NIL(L, pplayer, 2, Player, 0);
  LUASCRIPT_CHECK_ARG_NIL(L, effect_type, 3, string, 0);

  etype = effect_type_by_name(effect_type, fc_strcasecmp);
  if (!effect_type_is_valid(etype)) {
    return 0;
  }
  return get_player_bonus(pplayer, etype);
}

/*************************************************************************//**
  Returns the effect bonus at a city.
*****************************************************************************/
int api_effects_city_bonus(lua_State *L, City *pcity, const char *effect_type)
{
  enum effect_type etype = EFT_COUNT;

  LUASCRIPT_CHECK_STATE(L, 0);
  LUASCRIPT_CHECK_ARG_NIL(L, pcity, 2, City, 0);
  LUASCRIPT_CHECK_ARG_NIL(L, effect_type, 3, string, 0);

  etype = effect_type_by_name(effect_type, fc_strcasecmp);
  if (!effect_type_is_valid(etype)) {
    return 0;
  }
  return get_city_bonus(pcity, etype);
}
