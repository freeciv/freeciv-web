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

#ifdef GGZ_SERVER

#include <unistd.h>

#include <ggzdmod.h>

#include "fciconv.h"
#include "fcintl.h"
#include "log.h"
#include "support.h"

#include "game.h"
#include "player.h"

#include "connecthand.h"
#include "ggzserver.h"
#include "score.h"
#include "sernet.h"
#include "srv_main.h"
#include "stdinhand.h"

bool with_ggz = FALSE;

static GGZdMod *ggzdmod;


/****************************************************************************
  Return the seat occupied by this player (or -1 if the player i
  seatless).
****************************************************************************/
static int get_seat_for_player(const struct player *pplayer)
{
  int num_players = ggzdmod_get_num_seats(ggzdmod), i;

  /* This is pretty inefficient.  It could be faster if a player->seat
   * association was tracked but this is probably overkill. */
  for (i = 0; i < num_players; i++) {
    GGZSeat seat = ggzdmod_get_seat(ggzdmod, i);

    if (mystrcasecmp(pplayer->username, seat.name) == 0) {
      return seat.num;
    }
  }

  return -1;
}

/****************************************************************************
  Return the player sitting at the given seat (or NULL if no player can
  be found or if the seat is empty).
****************************************************************************/
static struct player *get_player_for_seat(int seat_num)
{
  GGZSeat seat = ggzdmod_get_seat(ggzdmod, seat_num);

  switch (seat.type) {
  case GGZ_SEAT_OPEN:
  case GGZ_SEAT_NONE:
  case GGZ_SEAT_RESERVED:
    return NULL;
  case GGZ_SEAT_PLAYER:
  case GGZ_SEAT_BOT:
  case GGZ_SEAT_ABANDONED:
    break;
  }

  players_iterate(pplayer) {
    if (mystrcasecmp(pplayer->username, seat.name)) {
      return pplayer;
    }
  } players_iterate_end;

  /* This is probably a bad bad error. */
  return NULL;
}

/****************************************************************************
  Handles a state event as reported by the GGZ server.
****************************************************************************/
static void handle_ggz_state_event(GGZdMod * ggz, GGZdModEvent event,
				   const void *data)
{
  const GGZdModState *old_state = data;
  GGZdModState new_state = ggzdmod_get_state(ggz);

  freelog(LOG_DEBUG, "ggz changed state to %d.", new_state);

  if (*old_state == GGZDMOD_STATE_CREATED) {
    const char *savegame = ggzdmod_get_savedgame(ggz);

    /* If a savegame is given, load it. */
    freelog(LOG_DEBUG, "Instructed to load \"%s\".", savegame);
    if (savegame) {
      if (!load_command(NULL, savegame, FALSE)) {
	/* no error handling? */
	server_quit();
      }
    }

    /* If we loaded a game that'll include the serverid.  If not we
     * generate one here. */
    if (strlen(srvarg.serverid) == 0) {
      strcpy(srvarg.serverid, "ggz-civ-XXXXXX");
      if (!mkdtemp(srvarg.serverid)) {
	freelog(LOG_ERROR,
		_("Unable to make temporary directory for GGZ game.\n"));
	server_quit();
      }
    }

    /* Change into the server directory */
    if (chdir(srvarg.serverid) < 0) {
      freelog(LOG_ERROR,
	      _("Unable to change into temporary server "
		"directory %s.\n"), srvarg.serverid);
      server_quit();
    }
    freelog(LOG_DEBUG, "Changed into directory %s.", srvarg.serverid);
  }
}

/****************************************************************************
  Handles a seat-change event as reported by the GGZ server.

  This typically happens when a human player joins or leaves the game.
  This function links the GGZ seat player (which is just a seat number) to
  the correct player.  In the case of a join event we also get the socket
  for the player's connection, which is treated just like a new connection.
****************************************************************************/
static void handle_ggz_seat_event(GGZdMod *ggz, GGZdModEvent event,
				  const void *data)
{
  const GGZSeat *old_seat = data;
  GGZSeat new_seat = ggzdmod_get_seat(ggz, old_seat->num);
#if 0
  /* These values could be useful at some point. */
  bool is_join = ((new_seat.type == GGZ_SEAT_PLAYER
		   || new_seat.type == GGZ_SEAT_BOT)
		  && (old_seat->type != new_seat.type
		      || strcmp(old_seat->name, new_seat.name)));
  bool is_leave = ((old_seat->type == GGZ_SEAT_PLAYER
		   || old_seat->type == GGZ_SEAT_BOT)
		   && (new_seat.type != old_seat->type
		       || strcmp(old_seat->name, new_seat.name)));
  GGZdModState new_state;

#endif

  if (new_seat.type == GGZ_SEAT_PLAYER
      && old_seat->type != GGZ_SEAT_PLAYER) {
    /* Player joins game. */
    server_make_connection(new_seat.fd, "", "");
  } else if (new_seat.type != GGZ_SEAT_PLAYER
	     && old_seat->type == GGZ_SEAT_PLAYER) {
    /* Player leaves game. */
    struct connection *leaving = NULL;

    players_iterate(pplayer) {
      conn_list_iterate(pplayer->connections, pconn) {
	if (strcmp(pconn->username, old_seat->name) == 0) {
	  leaving = pconn;
	  break;
	}
      } conn_list_iterate_end;
    } players_iterate_end;

    if (leaving) {
      freelog(LOG_DEBUG, "%s is leaving.", old_seat->name);
      leaving->sock = -1;
      lost_connection_to_client(leaving);
      close_connection(leaving);
    } else {
      freelog(LOG_ERROR, "Couldn't match player %s.", old_seat->name);
    }
  }
}

/****************************************************************************
  Handles a spectator seat event as reported by the GGZ server.
****************************************************************************/
static void handle_ggz_spectator_seat_event(GGZdMod *ggz, GGZdModEvent event,
					    const void *data)
{
#if 0
  /* Currently spectators are not supported.  TODO: spectators should be
   * integrated with observers. */
  const GGZSpectator *old = data;
  GGZSpectator new = ggzdmod_get_spectator(ggz, spectator);

  if (new.name) {

  } else {

  }
#endif
}

/****************************************************************************
  Handles a ggzdmod error.  This simply exits the server with an error
  message.
****************************************************************************/
static void handle_ggz_error(GGZdMod * ggz, GGZdModEvent event,
			     const void *data)
{
  const char *err = data;

  freelog(LOG_ERROR, "Error in ggz: %s", err);
  server_quit();
}

/****************************************************************************
  Connect to the GGZ server, if GGZ is being used.
****************************************************************************/
void ggz_initialize(void)
{
  with_ggz = ggzdmod_is_ggz_mode();

  if (with_ggz) {
    int ggz_socket;

    /* Detach from terminal. */
    fclose(stdin);
    fclose(stdout);
    fclose(stderr);

    /* We're in GGZ mode */
    ggzdmod = ggzdmod_new(GGZDMOD_GAME);
    ggzdmod_set_handler(ggzdmod, GGZDMOD_EVENT_STATE,
			&handle_ggz_state_event);
    ggzdmod_set_handler(ggzdmod, GGZDMOD_EVENT_JOIN,
			&handle_ggz_seat_event);
    ggzdmod_set_handler(ggzdmod, GGZDMOD_EVENT_LEAVE,
			&handle_ggz_seat_event);
    ggzdmod_set_handler(ggzdmod, GGZDMOD_EVENT_SEAT,
			&handle_ggz_seat_event);
    ggzdmod_set_handler(ggzdmod, GGZDMOD_EVENT_SPECTATOR_JOIN,
			&handle_ggz_spectator_seat_event);
    ggzdmod_set_handler(ggzdmod, GGZDMOD_EVENT_SPECTATOR_LEAVE,
			&handle_ggz_spectator_seat_event);
    ggzdmod_set_handler(ggzdmod, GGZDMOD_EVENT_SPECTATOR_SEAT,
			&handle_ggz_spectator_seat_event);
    ggzdmod_set_handler(ggzdmod, GGZDMOD_EVENT_ERROR,
    			&handle_ggz_error);
    if (ggzdmod_connect(ggzdmod) < 0) {
      exit(EXIT_FAILURE);
    }
    ggz_socket = ggzdmod_get_fd(ggzdmod);
    if (ggz_socket < 0) {
      fc_fprintf(stderr, _("Only the GGZ client must call civclient"
			   " in ggz mode!\n"));
      exit(EXIT_FAILURE);
    }
  }
}

/****************************************************************************
  Called by the network code when there is data to be read on the given
  GGZ socket.
****************************************************************************/
void input_from_ggz(int socket)
{
  ggzdmod_dispatch(ggzdmod);
}

/****************************************************************************
  Return the file descriptor for the GGZ socket.  The network code needs to
  monitor this socket and call input_from_ggz when data is to be read.
****************************************************************************/
int get_ggz_socket(void)
{
  return ggzdmod_get_fd(ggzdmod);
}

static const struct player *victors[MAX_NUM_PLAYERS];
static int num_victors;

/****************************************************************************
  Register a single player as the game victor.  See ggz_report_victory().
****************************************************************************/
void ggz_report_victor(const struct player *winner)
{
  freelog(LOG_VERBOSE, "Victor: %s", winner->name);

  if (!with_ggz) {
    return;
  }

  /* All players, including AI, are included on this list. */
  victors[num_victors] = winner;
  num_victors++;
}

/****************************************************************************
  Report victory to the GGZ server.

  One or more victor players may have been registered already using
  ggz_report_victor; all other players are assumed to be losers.

  Currently AI players are not considered at all.
****************************************************************************/
void ggz_report_victory(void)
{
  int num_players = ggzdmod_get_num_seats(ggzdmod), i;
  int teams[num_players], num_teams = 0, scores[num_players];
  GGZGameResult results[num_players], default_result;

  freelog(LOG_VERBOSE, "Victory...");

  if (!with_ggz) {
    return;
  }

  /* Assign teams.  First put players who are on teams. */
  team_iterate(pteam) {
    players_iterate(pplayer) {
      if (pplayer->team == pteam) {
	int seat = get_seat_for_player(pplayer);

	if (seat < 0) {
	  /* FIXME: this can happen for AI players */
	} else {
	  teams[get_seat_for_player(pplayer)] = num_teams;
	}
      }
    } players_iterate_end;
    num_teams++;
  } team_iterate_end;

  /* Then assign team numbers for non-team players. */
  for (i = 0; i < num_players; i++) {
    const struct player *pplayer = get_player_for_seat(i);

    if (!pplayer) {
      teams[i] = -1;
    } else if (!pplayer->team) {
      teams[i] = num_teams;
      num_teams++;
    } else {
      assert(teams[i] >= 0 && teams[i] < num_teams);
    }
  }

  /* Scores. */
  for (i = 0; i < num_players; i++) {
    const struct player *pplayer = get_player_for_seat(i);

    if (pplayer) {
      scores[i] = pplayer->score.game;
    } else {
      scores[i] = -1;
    }
  }

  if (num_victors == 0) {
    default_result = GGZ_GAME_TIE;
  } else {
    default_result = GGZ_GAME_LOSS;
  }
  for (i = 0; i < num_players; i++) {
    results[i] = default_result;
  }
  for (i = 0; i < num_victors; i++) {
    int seat_num = get_seat_for_player(victors[i]);

    if (seat_num < 0) {
      /* FIXME: this can happen for AI players */
    } else {
      results[seat_num] = GGZ_GAME_WIN;
    }
  }

  ggzdmod_report_game(ggzdmod, teams, results, scores);

  num_victors = 0; /* In case there's another game. */
}

/****************************************************************************
  Reports a savegame file to the GGZ server.  GGZ will allow
  reloading from a file later by providing the savegame at launch time
  (in the STATE event when leaving the CREATED state).
****************************************************************************/
void ggz_game_saved(const char *filename)
{
  char full_filename[strlen(filename) + strlen(srvarg.serverid) + 2];

  if (!path_is_absolute(filename)) {
    snprintf(full_filename, sizeof(full_filename), "%s/%s",
	     srvarg.serverid, filename);
  } else {
    sz_strlcpy(full_filename, filename);
  }
  freelog(LOG_DEBUG, "Reporting filename %s => %s to ggz.",
	  filename, full_filename);

  if (with_ggz) {
    ggzdmod_report_savegame(ggzdmod, full_filename);
  }
}

#endif
