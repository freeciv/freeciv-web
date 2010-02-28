/********************************************************************** 
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
#include <config.h>
#endif

#include <assert.h>
#include <stdio.h> /* for remove() */ 

/* utility */
#include "capability.h"
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "rand.h"
#include "registry.h"
#include "shared.h"
#include "support.h"

/* common */
#include "events.h"
#include "game.h"
#include "improvement.h"
#include "movement.h"
#include "packets.h"

/* server */
#include "connecthand.h"
#include "ggzserver.h"
#include "maphand.h"
#include "notify.h"
#include "plrhand.h"
#include "srv_main.h"
#include "unittools.h"

#include "gamehand.h"

#define CHALLENGE_ROOT "challenge"


/****************************************************************************
  Get unit_type for given role character
****************************************************************************/
struct unit_type *crole_to_unit_type(char crole,struct player *pplayer)
{
  struct unit_type *utype = NULL;
  enum unit_role_id role;

  switch(crole) {
  case 'c':
    role = L_CITIES;
    break;
  case 'w':
    role = L_SETTLERS;
    break;
  case 'x':
    role = L_EXPLORER;
    break;
  case 'k':
    role = L_GAMELOSS;
    break;
  case 's':
    role = L_DIPLOMAT;
    break;
  case 'f':
    role = L_FERRYBOAT;
    break;
  case 'd':
    role = L_DEFEND_OK;
    break;
  case 'D':
    role = L_DEFEND_GOOD;
    break;
  case 'a':
    role = L_ATTACK_FAST;
    break;
  case 'A':
    role = L_ATTACK_STRONG;
    break;
  default: 
    assert(FALSE);
    return NULL;
  }

  /* Create the unit of an appropriate type, if it exists */
  if (num_role_units(role) > 0) {
    if (pplayer != NULL) {
      utype = first_role_unit_for_player(pplayer, role);
    }
    if (utype == NULL) {
      utype = get_role_unit(role, 0);
    }
  }

  return utype;
}

/****************************************************************************
  Place a starting unit for the player. Returns tile where unit was really
  placed.
****************************************************************************/
static struct tile *place_starting_unit(struct tile *starttile,
                                        struct player *pplayer,
                                        char crole)
{
  struct tile *ptile = NULL;
  struct unit_type *utype = crole_to_unit_type(crole, pplayer);

  if (utype != NULL) {
    iterate_outward(starttile, map.xsize + map.ysize, itertile) {
      if (!is_non_allied_unit_tile(itertile, pplayer)
          && is_native_tile(utype, itertile)) {
        ptile = itertile;
        break;
      }
    } iterate_outward_end;
  }

  if (ptile == NULL) {
    /* No place where unit may exist. */
    return NULL;
  }

  assert(!is_non_allied_unit_tile(ptile, pplayer));

  /* For scenarios or dispersion, huts may coincide with player starts (in 
   * other cases, huts are avoided as start positions).  Remove any such hut,
   * and make sure to tell the client, since we may have already sent this
   * tile (with the hut) earlier: */
  if (tile_has_special(ptile, S_HUT)) {
    tile_clear_special(ptile, S_HUT);
    update_tile_knowledge(ptile);
    freelog(LOG_VERBOSE, "Removed hut on start position for %s",
	    player_name(pplayer));
  }

  /* Expose visible area. */
  map_show_circle(pplayer, ptile, game.info.init_vis_radius_sq);

  if (utype != NULL) {
    /* We cannot currently handle sea units as start units.
     * TODO: remove this code block when we can. */
    if (utype_move_type(utype) == SEA_MOVING) {
      freelog(LOG_ERROR, "Sea moving start units are not yet supported, "
                           "%s not created.",
                         utype_rule_name(utype));
      notify_player(pplayer, NULL, E_BAD_COMMAND, FTC_SERVER_INFO, NULL,
		    _("Sea moving start units are not yet supported. "
		      "Nobody gets %s."),
		    utype_name_translation(utype));
      return NULL;
    }

    (void) create_unit(pplayer, ptile, utype, FALSE, 0, 0);
    return ptile;
  }

  return NULL;
}

/****************************************************************************
  Find a valid position not far from our starting position.
****************************************************************************/
static struct tile *find_dispersed_position(struct player *pplayer,
                                            struct start_position *p)
{
  struct tile *ptile;
  int x, y;

  do {
    x = p->tile->x + myrand(2 * game.info.dispersion + 1) 
        - game.info.dispersion;
    y = p->tile->y + myrand(2 * game.info.dispersion + 1)
        - game.info.dispersion;
  } while (!((ptile = map_pos_to_tile(x, y))
             && tile_continent(p->tile) == tile_continent(ptile)
             && !is_ocean_tile(ptile)
             && !is_non_allied_unit_tile(ptile, pplayer)));

  return ptile;
}

/****************************************************************************
  Initialize a new game: place the players' units onto the map, etc.
****************************************************************************/
void init_new_game(void)
{
  const int NO_START_POS = -1;
  int start_pos[player_slot_count()];
  int placed_units[player_slot_count()];
  bool pos_used[map.server.num_start_positions];
  int i, num_used = 0;

  randomize_base64url_string(server.game_identifier,
                             sizeof(server.game_identifier));

  /* Shuffle starting positions around so that they match up with the
   * desired players. */

  /* First set up some data fields. */
  freelog(LOG_VERBOSE, "Placing players at start positions.");
  for (i = 0; i < map.server.num_start_positions; i++) {
    struct nation_type *n = map.server.start_positions[i].nation;

    pos_used[i] = FALSE;
    freelog(LOG_VERBOSE, "%3d : (%2d,%2d) : \"%s\" (%d)",
	    i, TILE_XY(map.server.start_positions[i].tile),
	    n ? nation_rule_name(n) : "", n ? nation_number(n) : -1);
  }
  players_iterate(pplayer) {
    start_pos[player_index(pplayer)] = NO_START_POS;
  } players_iterate_end;

  /* Second, assign a nation to a start position for that nation. */
  freelog(LOG_VERBOSE, "Assigning matching nations.");
  players_iterate(pplayer) {
    for (i = 0; i < map.server.num_start_positions; i++) {
      assert(pplayer->nation != NO_NATION_SELECTED);
      if (pplayer->nation == map.server.start_positions[i].nation) {
	freelog(LOG_VERBOSE, "Start_pos %d matches player %d (%s).",
		i,
		player_number(pplayer),
		nation_rule_name(nation_of_player(pplayer)));
	start_pos[player_index(pplayer)] = i;
	pos_used[i] = TRUE;
	num_used++;
      }
    }
  } players_iterate_end;

  /* Third, assign players randomly to the remaining start positions. */
  freelog(LOG_VERBOSE, "Assigning random nations.");
  players_iterate(pplayer) {
    if (start_pos[player_index(pplayer)] == NO_START_POS) {
      int which = myrand(map.server.num_start_positions - num_used);

      for (i = 0; i < map.server.num_start_positions; i++) {
	if (!pos_used[i]) {
	  if (which == 0) {
	    freelog(LOG_VERBOSE,
		    "Randomly assigning player %d (%s) to pos %d.",
		    player_number(pplayer),
		    nation_rule_name(nation_of_player(pplayer)),
		    i);
	    start_pos[player_index(pplayer)] = i;
	    pos_used[i] = TRUE;
	    num_used++;
	    break;
	  }
	  which--;
	}
      }
    }
    assert(start_pos[player_index(pplayer)] != NO_START_POS);
  } players_iterate_end;

  /* Loop over all players, creating their initial units... */
  players_iterate(pplayer) {
    struct start_position pos
      = map.server.start_positions[start_pos[player_index(pplayer)]];

    /* Place the first unit. */
    if (place_starting_unit(pos.tile, pplayer,
                            game.info.start_units[0]) != NULL) {
      placed_units[player_index(pplayer)] = 1;
    } else {
      placed_units[player_index(pplayer)] = 0;
    }
  } players_iterate_end;

  /* Place all other units. */
  players_iterate(pplayer) {
    int i;
    struct tile *ptile;
    struct nation_type *nation = nation_of_player(pplayer);
    struct start_position p
      = map.server.start_positions[start_pos[player_index(pplayer)]];

    /* Place global start units */
    for (i = 1; i < strlen(game.info.start_units); i++) {
      ptile = find_dispersed_position(pplayer, &p);

      /* Create the unit of an appropriate type. */
      if (place_starting_unit(ptile, pplayer,
                              game.info.start_units[i]) != NULL) {
        placed_units[player_index(pplayer)]++;
      }
    }

    /* Place nation specific start units (not role based!) */
    i = 0;
    while (nation->init_units[i] != NULL && i < MAX_NUM_UNIT_LIST) {
      ptile = find_dispersed_position(pplayer, &p);
      create_unit(pplayer, ptile, nation->init_units[i], FALSE, 0, 0);
      placed_units[player_index(pplayer)]++;
      i++;
    }
  } players_iterate_end;

  players_iterate(pplayer) {
    if (placed_units[player_index(pplayer)] == 0) {
      /* No units at all for some player! */
      die(_("No units placed for %s!"), player_name(pplayer));
    }
  } players_iterate_end;

  shuffle_players();
}

/**************************************************************************
  Tell clients the year, and also update turn_done and nturns_idle fields
  for all players.
**************************************************************************/
void send_year_to_clients(int year)
{
  struct packet_new_year apacket;
  
  players_iterate(pplayer) {
    pplayer->nturns_idle++;
  } players_iterate_end;

  apacket.year = year;
  apacket.turn = game.info.turn;
  lsend_packet_new_year(game.est_connections, &apacket);

  /* Hmm, clients could add this themselves based on above packet? */
  notify_conn(game.est_connections, NULL, E_NEXT_YEAR, NULL, NULL,
              _("Year: %s"), textyear(year));
}

/**************************************************************************
  Send game_info packet; some server options and various stuff...
  dest==NULL means game.est_connections

  It may be sent at any time. It MUST be sent before any player info, 
  as it contains the number of players.  To avoid inconsistency, it
  SHOULD be sent after rulesets and any other server settings.
**************************************************************************/
void send_game_info(struct conn_list *dest)
{
  struct packet_game_info ginfo;

  if (!dest) {
    dest = game.est_connections;
  }

  ginfo = game.info;

  /* the following values are computed every
     time a packet_game_info packet is created */
  if (game.info.timeout > 0 && game.server.phase_timer) {
    /* Sometimes this function is called before the phase_timer is
     * initialized.  In that case we want to send the dummy value. */
    ginfo.seconds_to_phasedone = game.info.seconds_to_phasedone
        - read_timer_seconds(game.server.phase_timer);
  } else {
    /* unused but at least initialized */
    ginfo.seconds_to_phasedone = -1.0;
  }

  conn_list_iterate(dest, pconn) {
    send_packet_game_info(pconn, &ginfo);
  }
  conn_list_iterate_end;
}

/**************************************************************************
  Send current scenario info. dest NULL causes send to everyone
**************************************************************************/
void send_scenario_info(struct conn_list *dest)
{
  struct packet_scenario_info sinfo;

  if (!dest) {
    dest = game.est_connections;
  }

  sinfo = game.scenario;

  conn_list_iterate(dest, pconn) {
    send_packet_scenario_info(pconn, &sinfo);
  } conn_list_iterate_end;
}

/**************************************************************************
  adjusts game.info.timeout based on various server options

  timeoutint: adjust game.info.timeout every timeoutint turns
  timeoutinc: adjust game.info.timeout by adding timeoutinc to it.
  timeoutintinc: every time we adjust game.info.timeout, we add timeoutintinc
                 to timeoutint.
  timeoutincmult: every time we adjust game.info.timeout, we multiply timeoutinc
                  by timeoutincmult
**************************************************************************/
int update_timeout(void)
{
  /* if there's no timer or we're doing autogame, do nothing */
  if (game.info.timeout < 1 || game.server.timeoutint == 0) {
    return game.info.timeout;
  }

  if (game.server.timeoutcounter >= game.server.timeoutint) {
    game.info.timeout += game.server.timeoutinc;
    game.server.timeoutinc *= game.server.timeoutincmult;

    game.server.timeoutcounter = 1;
    game.server.timeoutint += game.server.timeoutintinc;

    if (game.info.timeout > GAME_MAX_TIMEOUT) {
      notify_conn(game.est_connections, NULL, E_SETTING,
                  FTC_SERVER_INFO, NULL,
                  _("The turn timeout has exceeded its maximum value, "
                    "fixing at its maximum."));
      freelog(LOG_DEBUG, "game.info.timeout exceeded maximum value");
      game.info.timeout = GAME_MAX_TIMEOUT;
      game.server.timeoutint = 0;
      game.server.timeoutinc = 0;
    } else if (game.info.timeout < 0) {
      notify_conn(game.est_connections, NULL, E_SETTING,
                  FTC_SERVER_INFO, NULL,
                  _("The turn timeout is smaller than zero, "
                    "fixing at zero."));
      freelog(LOG_DEBUG, "game.info.timeout less than zero");
      game.info.timeout = 0;
    }
  } else {
    game.server.timeoutcounter++;
  }

  freelog(LOG_DEBUG, "timeout=%d, inc=%d incmult=%d\n   "
	  "int=%d, intinc=%d, turns till next=%d",
	  game.info.timeout, game.server.timeoutinc,
          game.server.timeoutincmult, game.server.timeoutint,
          game.server.timeoutintinc,
          game.server.timeoutint - game.server.timeoutcounter);

  return game.info.timeout;
}

/**************************************************************************
  adjusts game.seconds_to_turn_done when enemy moves a unit, we see it and
  the remaining timeout is smaller than the timeoutaddenemymove option.

  It's possible to use a similar function to do that per-player.  In
  theory there should be a separate timeout for each player and the
  added time should only go onto the victim's timer.
**************************************************************************/
void increase_timeout_because_unit_moved(void)
{
  if (game.info.timeout > 0 && game.server.timeoutaddenemymove > 0) {
    double maxsec = (read_timer_seconds(game.server.phase_timer)
		     + (double) game.server.timeoutaddenemymove);

    if (maxsec > game.info.seconds_to_phasedone) {
      game.info.seconds_to_phasedone = maxsec;
      send_game_info(NULL);
    }	
  }
}

/************************************************************************** 
  generate challenge filename for this connection, cannot fail.
**************************************************************************/
static void gen_challenge_filename(struct connection *pc)
{
}

/************************************************************************** 
  get challenge filename for this connection.
**************************************************************************/
static const char *get_challenge_filename(struct connection *pc)
{
  static char filename[MAX_LEN_PATH];

  my_snprintf(filename, sizeof(filename), "%s_%d_%d",
      CHALLENGE_ROOT, srvarg.port, pc->id);

  return filename;
}

/************************************************************************** 
  get challenge full filename for this connection.
**************************************************************************/
static const char *get_challenge_fullname(struct connection *pc)
{
  static char fullname[MAX_LEN_PATH];

  interpret_tilde(fullname, sizeof(fullname), "~/.freeciv/");
  sz_strlcat(fullname, get_challenge_filename(pc));

  return fullname;
}

/************************************************************************** 
  find a file that we can write too, and return it's name.
**************************************************************************/
const char *new_challenge_filename(struct connection *pc)
{
  gen_challenge_filename(pc);
  return get_challenge_filename(pc);
}


/************************************************************************** 
  Call this on a connection with HACK access to send it a set of ruleset
  choices.  Probably this should be called immediately when granting
  HACK access to a connection.
**************************************************************************/
static void send_ruleset_choices(struct connection *pc)
{
  struct packet_ruleset_choices packet;
  static char **rulesets = NULL;
  int i;

  if (!rulesets) {
    /* This is only read once per server invocation.  Add a new ruleset
     * and you have to restart the server. */
    rulesets = datafilelist(RULESET_SUFFIX);
  }

  for (i = 0; i < MAX_NUM_RULESETS && rulesets[i]; i++) {
    sz_strlcpy(packet.rulesets[i], rulesets[i]);
  }
  packet.ruleset_count = i;

  send_packet_ruleset_choices(pc, &packet);
}


/************************************************************************** 
opens a file specified by the packet and compares the packet values with
the file values. Sends an answer to the client once it's done.
**************************************************************************/
void handle_single_want_hack_req(struct connection *pc,
    				 struct packet_single_want_hack_req *packet)
{
  struct section_file file;
  char *token = NULL;
  bool you_have_hack = FALSE;

  if (!with_ggz) {
    if (section_file_load_nodup(&file, get_challenge_fullname(pc))) {
      token = secfile_lookup_str_default(&file, NULL, "challenge.token");
      you_have_hack = (token && strcmp(token, packet->token) == 0);
      section_file_free(&file);
    }

    if (!token) {
      freelog(LOG_DEBUG, "Failed to read authentication token");
    }
  }

  if (you_have_hack) {
    pc->server.granted_access_level = ALLOW_HACK;
    pc->access_level = ALLOW_HACK;
  }

  dsend_packet_single_want_hack_reply(pc, you_have_hack);

  send_ruleset_choices(pc);
  send_conn_info(pc->self, NULL);
}
