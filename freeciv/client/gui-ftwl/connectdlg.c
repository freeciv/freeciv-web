/********************************************************************** 
 Freeciv - Copyright (C) 2004 - The Freeciv Project
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
#include <config.h>
#endif

#include <errno.h>
#include <stdio.h>		/* sscanf */

/* utility */
#include "fcintl.h"
#include "log.h"
#include "support.h"

/* client */
#include "chatline_common.h"	/* for output_window_append */
#include "client_main.h"
#include "clinet.h"		/* connect_to_server() */
#include "gui_main.h"

#include "connectdlg.h"
#include "widget.h"
#include "theme_engine.h"

#ifdef HAVE_WINSOCK
#define ECONNREFUSED WSAECONNREFUSED
#endif

static struct te_screen *screen;

/**************************************************************************
  this regenerates the player information from a loaded game on the server.
**************************************************************************/
void handle_game_load(struct packet_game_load *packet)
{
  /* PORT ME */
}

/**************************************************************************
  ...
**************************************************************************/
static void connect_callback(void)
{
  char errbuf[512];

  sz_strlcpy(user_name, te_edit_get_current_value(screen, "username"));
  sz_strlcpy(server_host, te_edit_get_current_value(screen, "server"));
  sscanf(te_edit_get_current_value(screen, "port"), "%d", &server_port);

  if (connect_to_server(user_name, server_host, server_port,
			errbuf, sizeof(errbuf)) != -1) {
    te_destroy_screen(screen);
    screen = NULL;
  } else {
    output_window_append(FTC_CLIENT_INFO, NULL, errbuf);
  }
}

/**************************************************************************
  ...
**************************************************************************/
static void button_callback(const char *id)
{
  if (strcmp(id, "connect") == 0) {
    connect_callback();
  } else {
    assert(0);
  }
}

/**************************************************************************
  ...
**************************************************************************/
static const char *edit_get_initial_value(const char *id)
{
  if (strcmp(id, "port") == 0) {
    static char buf[10];

    my_snprintf(buf, sizeof(buf), "%d", server_port);
    return buf;
  } else if (strcmp(id, "username") == 0) {
    return user_name;
  } else if (strcmp(id, "password") == 0) {
    return "";
  } else if (strcmp(id, "server") == 0) {
    return server_host;
  } else {
    assert(0);
    return NULL;
  }
}

/**************************************************************************
  ...
**************************************************************************/
static int edit_get_width(const char *id)
{
  if (strcmp(id, "port") == 0) {
    return 5;
  } else if (strcmp(id, "username") == 0) {
    return 10;
  } else if (strcmp(id, "password") == 0) {
    return 10;
  } else if (strcmp(id, "server") == 0) {
    return 10;
  } else {
    assert(0);
    return 0;
  }
}

/**************************************************************************
  ...
**************************************************************************/
static const char *info_get_value(const char *id)
{
  if (0) {

  } else {
    assert(0);
    return NULL;
  }
}

/**************************************************************************
  ...
**************************************************************************/
static void popup_connect_window(void)
{
  struct te_screen_env env;

  env.info_get_value=info_get_value;
  env.edit_get_initial_value=edit_get_initial_value;
  env.edit_get_width=edit_get_width;
  env.button_callback=button_callback;

//  screen = te_get_screen(root_window, "connect", &env, DEPTH_MIN + 2);
}

/**************************************************************************
  Provide an interface for connecting to a Freeciv server.
**************************************************************************/
void gui_server_connect(void)
{
  popup_connect_window();
}

/**************************************************************************
  ...
**************************************************************************/
void close_connection_dialog(void)
{
  /* I do not think this is right, but currently gui-fs depends on it.
   * It is called from client common code when shutting down. So I am
   * not changing it now. - Per */
//  te_destroy_screen(screen);
}

/**************************************************************************
  ...
**************************************************************************/
void handle_authentication_req(enum authentication_type type, char *message)
{
}

/**************************************************************************
  ...
**************************************************************************/
void really_close_connection_dialog(void)
{
  close_connection_dialog();
}

/**************************************************************************
  ...
**************************************************************************/
void gui_set_rulesets(int num_rulesets, char **rulesets)
{
    /* PORTME */
}
