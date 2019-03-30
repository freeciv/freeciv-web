/***********************************************************************
 Freeciv - Copyright (C) 2017 - Freeciv Development Team
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

/* common */
#include "fc_interface.h"

/* server */
#include "srv_main.h"


#include "tools_fc_interface.h"


/***********************************************************************//**
  Unused but required by fc_interface_init()
***************************************************************************/
static bool tool_player_tile_vision_get(const struct tile *ptile,
                                        const struct player *pplayer,
                                        enum vision_layer vision)
{
  log_error("Assumed unused function %s called.",  __FUNCTION__);
  return FALSE;
}

/***********************************************************************//**
  Unused but required by fc_interface_init()
***************************************************************************/
static int tool_player_tile_city_id_get(const struct tile *ptile,
                                        const struct player *pplayer)
{
  log_error("Assumed unused function %s called.",  __FUNCTION__);
  return IDENTITY_NUMBER_ZERO;
}

/***********************************************************************//**
  Unused but required by fc_interface_init()
***************************************************************************/
static void tool_gui_color_free(struct color *pcolor)
{
  log_error("Assumed unused function %s called.",  __FUNCTION__);
}

/***********************************************************************//**
  Initialize tool specific functions.
***************************************************************************/
void fc_interface_init_tool(void)
{
  struct functions *funcs = fc_interface_funcs();

  /* May be used when generating help texts */
  funcs->server_setting_by_name = server_ss_by_name;
  funcs->server_setting_name_get = server_ss_name_get;
  funcs->server_setting_type_get = server_ss_type_get;
  funcs->server_setting_val_bool_get = server_ss_val_bool_get;

  /* Not used. Set to dummy functions. */
  funcs->player_tile_vision_get = tool_player_tile_vision_get;
  funcs->player_tile_city_id_get = tool_player_tile_city_id_get;
  funcs->gui_color_free = tool_gui_color_free;

  /* Keep this function call at the end. It checks if all required functions
     are defined. */
  fc_interface_init();
}
