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
#include "registry_ini.h"

/* common */
#include "game.h"

/* common/scriptcore */
#include "luascript.h"

/* server/scripting */
#include "script_server.h"

#include "api_server_luadata.h"

/*************************************************************************//**
  Get string value from luadata.
*****************************************************************************/
const char *api_luadata_get_str(lua_State *L, const char *field)
{
  LUASCRIPT_CHECK_STATE(L, NULL);

  if (game.server.luadata == NULL) {
    return NULL;
  }

  return secfile_lookup_str_default(game.server.luadata, NULL, "%s", field);
}
