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
#include "luascript_signal.h"
#include "luascript.h"

#include "api_signal_base.h"

/*************************************************************************//**
  Connects a callback function to a certain signal.
*****************************************************************************/
void api_signal_connect(lua_State *L, const char *signal_name,
                               const char *callback_name)
{
  struct fc_lua *fcl;

  LUASCRIPT_CHECK_STATE(L);
  LUASCRIPT_CHECK_ARG_NIL(L, signal_name, 2, string);
  LUASCRIPT_CHECK_ARG_NIL(L, callback_name, 3, string);

  fcl = luascript_get_fcl(L);

  LUASCRIPT_CHECK(L, fcl != NULL, "Undefined Freeciv lua state!");

  luascript_signal_callback(fcl, signal_name, callback_name, TRUE);
}

/*************************************************************************//**
  Removes a callback function to a certain signal.
*****************************************************************************/
void api_signal_remove(lua_State *L, const char *signal_name,
                       const char *callback_name)
{
  struct fc_lua *fcl;

  LUASCRIPT_CHECK_STATE(L);
  LUASCRIPT_CHECK_ARG_NIL(L, signal_name, 2, string);
  LUASCRIPT_CHECK_ARG_NIL(L, callback_name, 3, string);

  fcl = luascript_get_fcl(L);

  LUASCRIPT_CHECK(L, fcl != NULL, "Undefined Freeciv lua state!");

  luascript_signal_callback(fcl, signal_name, callback_name, FALSE);
}

/*************************************************************************//**
  Returns if a callback function to a certain signal is defined.
*****************************************************************************/
bool api_signal_defined(lua_State *L, const char *signal_name,
                        const char *callback_name)
{
  struct fc_lua *fcl;

  LUASCRIPT_CHECK_STATE(L, FALSE);
  LUASCRIPT_CHECK_ARG_NIL(L, signal_name, 2, string, FALSE);
  LUASCRIPT_CHECK_ARG_NIL(L, callback_name, 3, string, FALSE);

  fcl = luascript_get_fcl(L);

  LUASCRIPT_CHECK(L, fcl != NULL, "Undefined Freeciv lua state!", FALSE);

  return luascript_signal_callback_defined(fcl, signal_name, callback_name);
}

/*************************************************************************//**
  Return the name of the signal with the given index.
*****************************************************************************/
const char *api_signal_callback_by_index(lua_State *L,
                                         const char *signal_name, int sindex)
{
  struct fc_lua *fcl;

  LUASCRIPT_CHECK_STATE(L, NULL);
  LUASCRIPT_CHECK_ARG_NIL(L, signal_name, 2, string, NULL);

  fcl = luascript_get_fcl(L);

  LUASCRIPT_CHECK(L, fcl != NULL, "Undefined Freeciv lua state!", NULL);

  return luascript_signal_callback_by_index(fcl, signal_name, sindex);
}

/*************************************************************************//**
  Return the name of the 'index' callback function of the signal with the
  name 'signal_name'.
*****************************************************************************/
const char *api_signal_by_index(lua_State *L, int sindex)
{
  struct fc_lua *fcl;

  LUASCRIPT_CHECK_STATE(L, NULL);

  fcl = luascript_get_fcl(L);

  LUASCRIPT_CHECK(L, fcl != NULL, "Undefined Freeciv lua state!", NULL);

  return luascript_signal_by_index(fcl, sindex);
}
