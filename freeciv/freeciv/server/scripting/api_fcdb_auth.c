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

/* utility */
#include "log.h"

/* common */
#include "connection.h"

/* common/scriptcore */
#include "luascript.h"

/* server */
#include "auth.h"

/* server/scripting */
#include "script_fcdb.h"

#include "api_fcdb_auth.h"

/*************************************************************************//**
  Get the username.
*****************************************************************************/
const char *api_auth_get_username(lua_State *L, Connection *pconn)
{
  LUASCRIPT_CHECK_STATE(L, NULL);
  LUASCRIPT_CHECK_SELF(L, pconn, NULL);
  fc_assert_ret_val(conn_is_valid(pconn), NULL);

  return auth_get_username(pconn);
}

/*************************************************************************//**
  Get the ip address.
*****************************************************************************/
const char *api_auth_get_ipaddr(lua_State *L, Connection *pconn)
{
  LUASCRIPT_CHECK_STATE(L, NULL);
  LUASCRIPT_CHECK_SELF(L, pconn, NULL);
  fc_assert_ret_val(conn_is_valid(pconn), NULL);

  return auth_get_ipaddr(pconn);
}
