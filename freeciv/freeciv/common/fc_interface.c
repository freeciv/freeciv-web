/****************************************************************************
 Freeciv - Copyright (C) 2004 - The Freeciv Team
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
****************************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

/* utility */
#include "shared.h"

/* common */
#include "player.h"
#include "tile.h"
#include "vision.h"

#include "fc_interface.h"

/* Struct with functions pointers; the functions are defined in
   ./client/client_main.c:init_client_functions() and
   ./server/srv_main.c:init_server_functions(). */
struct functions fc_functions;

/* The functions are accessed via this pointer. */
const struct functions *fc_funcs = NULL;
/* After this is set to TRUE (in interface_init()), the functions are
   available via fc_funcs. */
bool fc_funcs_defined = FALSE;

/************************************************************************//**
  Return the function pointer. Only possible before interface_init() was
  called (fc_funcs_defined == FALSE).
****************************************************************************/
struct functions *fc_interface_funcs(void)
{
  fc_assert_exit(fc_funcs_defined == FALSE);

  return &fc_functions;
}

/************************************************************************//**
  Test and initialize the functions. The existence of all functions should
  be checked!
****************************************************************************/
void fc_interface_init(void)
{
  fc_funcs = &fc_functions;

  /* Test the existence of each required function here! */
  fc_assert_exit(fc_funcs->server_setting_by_name);
  fc_assert_exit(fc_funcs->server_setting_name_get);
  fc_assert_exit(fc_funcs->server_setting_type_get);
  fc_assert_exit(fc_funcs->server_setting_val_bool_get);
  fc_assert_exit(fc_funcs->player_tile_vision_get);
  fc_assert_exit(fc_funcs->player_tile_city_id_get);
  fc_assert_exit(fc_funcs->gui_color_free);

  fc_funcs_defined = TRUE;

  fc_strAPI_init();

  setup_real_activities_array();
}

/************************************************************************//**
  Free misc resources allocated for libfreeciv.
****************************************************************************/
void free_libfreeciv(void)
{
  diplrel_mess_close();
  free_data_dir_names();
  free_multicast_group();
  free_freeciv_storage_dir();
  free_user_home_dir();
  free_fileinfo_data();
  fc_strAPI_free();
}
