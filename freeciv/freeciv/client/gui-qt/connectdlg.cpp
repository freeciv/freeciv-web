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

/* utility */
#include "fcintl.h"
#include "log.h"
#include "support.h"

#include "connectdlg_g.h"

/* client */
#include "chatline_common.h"	/* for append_output_window */
#include "client_main.h"
#include "packhand_gen.h"

// gui-qt
#include "connectdlg.h"
#include "fc_client.h"


/**********************************************************************//**
  Close and destroy the dialog. But only if we don't have a local
  server running (that we started).
**************************************************************************/
void qtg_close_connection_dialog()
{
  if (gui()->current_page() != PAGE_NETWORK) {
    qtg_real_set_client_page(PAGE_MAIN);
  }
}

/**********************************************************************//**
  Configure the dialog depending on what type of authentication request the
  server is making.
**************************************************************************/
void handle_authentication_req(enum authentication_type type,
                               const char *message)
{
  gui()->handle_authentication_req(type, message);
}

/**********************************************************************//**
  Provide a packet handler for packet_game_load.

  This regenerates the player information from a loaded game on the
  server.
**************************************************************************/
void handle_game_load(bool load_successful, const char *filename)
{
  if (load_successful) {
    set_client_page(PAGE_START);

    if (game.info.is_new_game) {
      /* It's pregame. Create a player and connect to him */
      send_chat("/take -");
    }
  }
}

/**********************************************************************//**
  Provide an interface for connecting to a Freeciv server.
**************************************************************************/
void qtg_server_connect()
{
  /* PORTME */
}
