/********************************************************************** 
 Freeciv - Copyright (C) 2005 - Freeciv Development Team
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

/* utility */
#include "fciconv.h"
#include "fcintl.h"
#include "rand.h"

/* client */
#include "gui_main_g.h"
#include "ggz_g.h"

#include "clinet.h"
#include "ggzclient.h"

#ifdef GGZ_CLIENT
#  include <ggzmod.h>
#endif

bool with_ggz;

/****************************************************************************
  Initializations for GGZ, including checks to see if we are being run from
  inside GGZ.
****************************************************************************/
void ggz_initialize(void)
{
#ifdef GGZ_CLIENT
  with_ggz = ggzmod_is_ggz_mode();

  if (with_ggz) {
    ggz_begin();
  }
#endif

  gui_ggz_embed_ensure_server();
}

#ifdef GGZ_CLIENT

static GGZMod *ggzmod;

/****************************************************************************
  A callback that GGZ calls when we are connected to the freeciv
  server.
****************************************************************************/
static void handle_ggzmod_server(GGZMod * ggzmod, GGZModEvent e,
				 const void *data)
{
  const int *socket = data;
  const char *username = ggzmod_get_player(ggzmod, NULL, NULL);

  if (!username) {
    username = "NONE";
  }
  make_connection(*socket, username);
}

/****************************************************************************
  Callback for when the GGZ client tells us player stats.
****************************************************************************/
static void handle_ggzmod_stats(GGZMod *ggzmod, GGZModEvent e,
				const void *data)
{
  update_conn_list_dialog();
}

/****************************************************************************
  Connect to the GGZ client.
****************************************************************************/
void ggz_begin(void)
{
  int ggz_socket;

  /* We're in GGZ mode */
  ggzmod = ggzmod_new(GGZMOD_GAME);
  ggzmod_set_handler(ggzmod, GGZMOD_EVENT_SERVER, &handle_ggzmod_server);
  ggzmod_set_handler(ggzmod, GGZMOD_EVENT_STATS, &handle_ggzmod_stats);
  if (ggzmod_connect(ggzmod) < 0) {
    exit(EXIT_FAILURE);
  }
  ggz_socket = ggzmod_get_fd(ggzmod);
  if (ggz_socket < 0) {
    fc_fprintf(stderr, _("Only the GGZ client must call civclient"
			 " in ggz mode!\n"));
    exit(EXIT_FAILURE);
  }
  add_ggz_input(ggz_socket);
}

/****************************************************************************
  Called when the ggz socket has data pending.
****************************************************************************/
void input_from_ggz(int socket)
{
  if (ggzmod_dispatch(ggzmod) < 0) {
    disconnect_from_server();
  }
}

/****************************************************************************
  Find the seat for the given player (in the seat parameter), or returns
  FALSE if no seat is found.
****************************************************************************/
static bool user_get_seat(const char *name, GGZSeat *seat)
{
  int i;
  int num = ggzmod_get_num_seats(ggzmod);

  for (i = 0; i < num; i++) {
    *seat = ggzmod_get_seat(ggzmod, i);

    if (seat->type == GGZ_SEAT_PLAYER
	&& strcasecmp(seat->name, name) == 0) {
      return TRUE;
    }
  }

  return FALSE;
}

/****************************************************************************
  Find the player's rating, or return FALSE if there is no rating.
****************************************************************************/
bool user_get_rating(const char *name, int *rating)
{
  GGZSeat seat;

  if (user_get_seat(name, &seat)) {
    return ggzmod_player_get_rating(ggzmod, &seat, rating);
  }

  return FALSE;
}

/****************************************************************************
  Find the player's record, or return FALSE if there is no record.
****************************************************************************/
bool user_get_record(const char *name,
		       int *wins, int *losses, int *ties, int *forfeits)
{
  GGZSeat seat;

  if (user_get_seat(name, &seat)) {
    return ggzmod_player_get_record(ggzmod, &seat,
				    wins, losses, ties, forfeits);
  }

  return FALSE;

}

#endif
