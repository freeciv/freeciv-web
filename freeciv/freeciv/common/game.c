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

/* utility */
#include "fcintl.h"
#include "ioz.h"
#include "log.h"
#include "mem.h"
#include "shared.h"
#include "support.h"

/* aicore */
#include "cm.h"

/* common */
#include "base.h"
#include "city.h"
#include "connection.h"
#include "government.h"
#include "idex.h"
#include "map.h"
#include "nation.h"
#include "packets.h"
#include "player.h"
#include "spaceship.h"
#include "specialist.h"
#include "tech.h"
#include "unit.h"
#include "unitlist.h"

#include "game.h"

struct civ_game game;

/*
struct player_score {
  int happy;
  int content;
  int unhappy;
  int angry;
  int taxmen;
  int scientists;
  int elvis;
  int wonders;
  int techs;
  int landarea;
  int settledarea;
  int population;
  int cities;
  int units;
  int pollution;
  int literacy;
  int bnp;
  int mfg;
  int spaceship;
};
*/

bool am_i_server = FALSE;


/**************************************************************************
  Is program type server?
**************************************************************************/
bool is_server(void)
{
  return am_i_server;
}

/**************************************************************************
  Set program type to server.
**************************************************************************/
void i_am_server(void)
{
  am_i_server = TRUE;
}

/**************************************************************************
  Set program type to client.
**************************************************************************/
void i_am_client(void)
{
  am_i_server = FALSE;
}

/**************************************************************************
Count the # of thousand citizen in a civilisation.
**************************************************************************/
int civ_population(const struct player *pplayer)
{
  int ppl=0;
  city_list_iterate(pplayer->cities, pcity)
    ppl+=city_population(pcity);
  city_list_iterate_end;
  return ppl;
}


/**************************************************************************
...
**************************************************************************/
struct city *game_find_city_by_name(const char *name)
{
  players_iterate(pplayer) {
    struct city *pcity = city_list_find_name(pplayer->cities, name);

    if (pcity) {
      return pcity;
    }
  } players_iterate_end;

  return NULL;
}


/**************************************************************************
  Often used function to get a city pointer from a city ID.
  City may be any city in the game.  This now always uses fast idex
  method, instead of looking through all cities of all players.
**************************************************************************/
struct city *game_find_city_by_number(int id)
{
  return idex_lookup_city(id);
}


/**************************************************************************
  Find unit out of all units in game: now uses fast idex method,
  instead of looking through all units of all players.
**************************************************************************/
struct unit *game_find_unit_by_number(int id)
{
  return idex_lookup_unit(id);
}

/**************************************************************************
  In the server call wipe_unit(), and never this function directly.
**************************************************************************/
void game_remove_unit(struct unit *punit)
{
  struct city *pcity;

  /* Opaque server-only variable: the server must free this earlier. */
  assert(punit->server.vision == NULL);

  pcity = player_find_city_by_id(unit_owner(punit), punit->homecity);
  if (pcity) {
    unit_list_unlink(pcity->units_supported, punit);

    freelog(LOG_DEBUG, "game_remove_unit()"
	    " at (%d,%d) unit %d, %s %s home (%d,%d) city %d, %s %s",
	    TILE_XY(punit->tile),
	    punit->id, 
	    nation_rule_name(nation_of_unit(punit)),
	    unit_rule_name(punit),
	    TILE_XY(pcity->tile),
	    punit->homecity,
	    nation_rule_name(nation_of_city(pcity)),
	    city_name(pcity));
  } else if (IDENTITY_NUMBER_ZERO == punit->homecity) {
    freelog(LOG_DEBUG, "game_remove_unit()"
	    " at (%d,%d) unit %d, %s %s home %d",
	    TILE_XY(punit->tile),
	    punit->id, 
	    nation_rule_name(nation_of_unit(punit)),
	    unit_rule_name(punit),
	    punit->homecity);
  } else {
    freelog(LOG_ERROR, "game_remove_unit()"
	    " at (%d,%d) unit %d, %s %s home %d invalid",
	    TILE_XY(punit->tile),
	    punit->id, 
	    nation_rule_name(nation_of_unit(punit)),
	    unit_rule_name(punit),
	    punit->homecity);
  }

  unit_list_unlink(punit->tile->units, punit);
  unit_list_unlink(unit_owner(punit)->units, punit);

  idex_unregister_unit(punit);

  if (game.callbacks.unit_deallocate) {
    (game.callbacks.unit_deallocate)(punit->id);
  }
  destroy_unit_virtual(punit);
}

/**************************************************************************
...
**************************************************************************/
void game_remove_city(struct city *pcity)
{
  struct tile *pcenter = city_tile(pcity);
  struct player *powner = city_owner(pcity);

  if (NULL != powner) {
    /* always unlink before clearing data */
    city_list_unlink(powner->cities, pcity);
  }

  if (NULL == pcenter) {
    freelog(LOG_DEBUG, "game_remove_city()"
            " virtual city %d, %s",
            pcity->id,
            city_name(pcity));
  } else {
    freelog(LOG_DEBUG, "game_remove_city()"
            " at (%d,%d) city %d, %s %s",
            TILE_XY(pcenter),
            pcity->id,
            nation_rule_name(nation_of_player(powner)),
            city_name(pcity));

    city_tile_iterate(pcenter, ptile) {
      if (tile_worked(ptile) == pcity) {
        tile_set_worked(ptile, NULL);
      }
    } city_tile_iterate_end;
  }
  
  /* Opaque server-only variable: the server must free this earlier. */
  assert(pcity->server.vision == NULL);

  idex_unregister_city(pcity);
  destroy_city_virtual(pcity);
}

/***************************************************************
...
***************************************************************/
void game_init(void)
{
  int i;

  game.info.globalwarming = 0;
  game.info.warminglevel  = 0; /* set later */
  game.info.nuclearwinter = 0;
  game.info.coolinglevel  = 0; /* set later */
  game.info.gold          = GAME_DEFAULT_GOLD;
  game.info.tech          = GAME_DEFAULT_TECHLEVEL;
  game.info.skill_level   = GAME_DEFAULT_SKILL_LEVEL;
  game.info.timeout       = GAME_DEFAULT_TIMEOUT;
  game.info.tcptimeout    = GAME_DEFAULT_TCPTIMEOUT;
  game.info.netwait       = GAME_DEFAULT_NETWAIT;
  game.info.start_year    = GAME_START_YEAR;
  game.info.end_turn      = GAME_DEFAULT_END_TURN;
  game.info.year          = GAME_START_YEAR;
  game.info.year_0_hack   = FALSE;
  game.info.turn          = 0;
  game.info.positive_year_label[0] = '\0';
  game.info.negative_year_label[0] = '\0';
  game.info.min_players   = GAME_DEFAULT_MIN_PLAYERS;
  game.info.max_players   = GAME_DEFAULT_MAX_PLAYERS;
  game.info.pingtimeout   = GAME_DEFAULT_PINGTIMEOUT;
  game.info.pingtime      = GAME_DEFAULT_PINGTIME;
  game.info.diplomacy     = GAME_DEFAULT_DIPLOMACY;
  game.info.diplcost      = GAME_DEFAULT_DIPLCOST;
  game.info.diplchance    = GAME_DEFAULT_DIPLCHANCE;
  game.info.freecost      = GAME_DEFAULT_FREECOST;
  game.info.conquercost   = GAME_DEFAULT_CONQUERCOST;
  game.info.dispersion    = GAME_DEFAULT_DISPERSION;
  game.info.citymindist   = GAME_DEFAULT_CITYMINDIST;
  game.info.civilwarsize  = GAME_DEFAULT_CIVILWARSIZE;
  game.info.contactturns  = GAME_DEFAULT_CONTACTTURNS;
  game.info.rapturedelay  = GAME_DEFAULT_RAPTUREDELAY;
  game.info.celebratesize = GAME_DEFAULT_CELEBRATESIZE;
  game.info.savepalace    = GAME_DEFAULT_SAVEPALACE;
  game.info.natural_city_names = GAME_DEFAULT_NATURALCITYNAMES;
  game.info.migration        = GAME_DEFAULT_MIGRATION;
  game.info.mgr_turninterval = GAME_DEFAULT_MGR_TURNINTERVAL;
  game.info.mgr_foodneeded   = GAME_DEFAULT_MGR_FOODNEEDED;
  game.info.mgr_distance     = GAME_DEFAULT_MGR_DISTANCE;
  game.info.mgr_worldchance  = GAME_DEFAULT_MGR_WORLDCHANCE;
  game.info.mgr_nationchance = GAME_DEFAULT_MGR_NATIONCHANCE;
  game.info.angrycitizen  = GAME_DEFAULT_ANGRYCITIZEN;
  game.info.foodbox       = GAME_DEFAULT_FOODBOX;
  game.info.shieldbox = GAME_DEFAULT_SHIELDBOX;
  game.info.sciencebox = GAME_DEFAULT_SCIENCEBOX;
  game.info.aqueductloss  = GAME_DEFAULT_AQUEDUCTLOSS;
  game.info.killcitizen   = GAME_DEFAULT_KILLCITIZEN;
  game.info.techpenalty   = GAME_DEFAULT_TECHPENALTY;
  game.info.razechance    = GAME_DEFAULT_RAZECHANCE;
  game.info.spacerace     = GAME_DEFAULT_SPACERACE;
  game.info.endspaceship  = GAME_DEFAULT_END_SPACESHIP;
  game.info.turnblock     = GAME_DEFAULT_TURNBLOCK;
  game.info.fogofwar      = GAME_DEFAULT_FOGOFWAR;
  game.info.borders       = GAME_DEFAULT_BORDERS;
  game.info.happyborders  = GAME_DEFAULT_HAPPYBORDERS;
  game.info.slow_invasions= RS_DEFAULT_SLOW_INVASIONS;
  game.info.auto_ai_toggle= GAME_DEFAULT_AUTO_AI_TOGGLE;
  game.info.notradesize   = GAME_DEFAULT_NOTRADESIZE;
  game.info.fulltradesize = GAME_DEFAULT_FULLTRADESIZE;
  game.info.barbarianrate = GAME_DEFAULT_BARBARIANRATE;
  game.info.onsetbarbarian= GAME_DEFAULT_ONSETBARBARIAN;
  game.info.trademindist  = GAME_DEFAULT_TRADEMINDIST;
  game.info.occupychance  = GAME_DEFAULT_OCCUPYCHANCE;
  game.info.autoattack    = GAME_DEFAULT_AUTOATTACK;
  game.info.revolution_length = GAME_DEFAULT_REVOLUTION_LENGTH;
  game.info.heating       = 0;
  game.info.cooling       = 0;
  game.info.allowed_city_names = GAME_DEFAULT_ALLOWED_CITY_NAMES;
  game.info.save_nturns   = GAME_DEFAULT_SAVETURNS;
  game.info.save_compress_level = GAME_DEFAULT_COMPRESS_LEVEL;
#ifdef HAVE_LIBBZ2
  game.info.save_compress_type = FZ_BZIP2;
#elif defined (HAVE_LIBZ)
  game.info.save_compress_type = FZ_ZLIB;
#else
  game.info.save_compress_type = FZ_PLAIN;
#endif
  game.info.government_during_revolution_id = G_MAGIC;   /* flag */

  game.info.calendar_skip_0 = FALSE;

  game.info.is_new_game   = TRUE;
  game.info.is_edit_mode = FALSE;

  game.info.aifill      = GAME_DEFAULT_AIFILL;
  sz_strlcpy(game.info.start_units, GAME_DEFAULT_START_UNITS);

  game.control.num_unit_classes = 0;
  game.control.num_unit_types = 0;
  game.control.num_impr_types = 0;
  game.control.num_tech_types = 0;
  game.control.num_base_types = 0;
  
  game.control.government_count = 0;
  game.control.nation_count = 0;
  game.control.styles_count = 0;
  game.control.terrain_count = 0;
  game.control.resource_count = 0;

  game.control.num_specialist_types = 0;

  game.control.prefered_tileset[0] = '\0';

  game.scenario.is_scenario = FALSE;
  game.scenario.name[0] = '\0';
  game.scenario.description[0] = '\0';
  game.scenario.players = TRUE;

  if (is_server()) {
    game.server.fogofwar_old = game.info.fogofwar;
    game.server.foggedborders = GAME_DEFAULT_FOGGEDBORDERS;
    game.server.phase_mode_stored = GAME_DEFAULT_PHASE_MODE;
    game.server.timeoutint = GAME_DEFAULT_TIMEOUTINT;
    game.server.timeoutintinc = GAME_DEFAULT_TIMEOUTINTINC;
    game.server.timeoutinc = GAME_DEFAULT_TIMEOUTINC;
    game.server.timeoutincmult = GAME_DEFAULT_TIMEOUTINCMULT;
    game.server.timeoutcounter = 1;
    game.server.timeoutaddenemymove = GAME_DEFAULT_TIMEOUTADDEMOVE; 

    game.server.last_ping = 0;
    game.server.scorelog = GAME_DEFAULT_SCORELOG;
    game.server.scoreturn = GAME_DEFAULT_SCORETURN;
    game.server.seed = GAME_DEFAULT_SEED;

    sz_strlcpy(game.server.save_name, GAME_DEFAULT_SAVE_NAME);
    sz_strlcpy(game.server.rulesetdir, GAME_DEFAULT_RULESETDIR);

    sz_strlcpy(game.server.demography, GAME_DEFAULT_DEMOGRAPHY);
    sz_strlcpy(game.server.allow_take, GAME_DEFAULT_ALLOW_TAKE);

    game.server.save_options.save_random = TRUE;
    game.server.save_options.save_known = TRUE;
    game.server.save_options.save_starts = TRUE;
    game.server.save_options.save_private_map = TRUE;

    for (i = 0; i < DEBUG_LAST; i++) {
      game.server.debug[i] = FALSE;
    }

    game.server.meta_info.user_message_set = FALSE;
    game.server.meta_info.user_message[0] = '\0';
    game.server.connectmsg[0] = '\0';
  }

  map_init();
  game_ruleset_init();
  teams_init();
  idex_init();
  cm_init();

  /* players init */
  player_slots_iterate(pslot) {
    player_slot_set_used(pslot, FALSE);
    player_init(pslot);
  } player_slots_iterate_end;
  set_player_count(0);

  for (i = 0; i < A_LAST; i++) {        /* game.num_tech_types = 0 here */
    game.info.global_advances[i] = FALSE;
  }
  for (i = 0; i < B_LAST; i++) {        /* game.num_impr_types = 0 here */
    game.info.great_wonder_owners[i] = WONDER_NOT_OWNED;
  }

  terrain_control.river_help_text[0] = '\0';
}

/****************************************************************************
  Initialize map-specific parts of the game structure.  Maybe these should
  be moved into the map structure?
****************************************************************************/
void game_map_init(void)
{
  /* FIXME: it's not clear where these values should be initialized.  It
   * can't be done in game_init because the map isn't created yet.  Maybe it
   * should be done in the mapgen code or in the maphand code.  It should
   * surely be called when the map is generated. */
  game.info.warminglevel = (map_num_tiles() + 499) / 500;
  game.info.coolinglevel = (map_num_tiles() + 499) / 500;
}

/****************************************************************************
  Remove all that is game-dependent in the player structure.
****************************************************************************/
static void game_player_reset(struct player *pplayer)
{
  if (pplayer->attribute_block.data) {
    free(pplayer->attribute_block.data);
    pplayer->attribute_block.data = NULL;
  }
  pplayer->attribute_block.length = 0;

  if (pplayer->attribute_block_buffer.data) {
    free(pplayer->attribute_block_buffer.data);
    pplayer->attribute_block_buffer.data = NULL;
  }
  pplayer->attribute_block_buffer.length = 0;

  unit_list_iterate(pplayer->units, punit) {
    game_remove_unit(punit);
  } unit_list_iterate_end;
  if (0 != unit_list_size(pplayer->units)) {
    freelog(LOG_ERROR, "game_remove_player() failed to remove %d %s units",
            unit_list_size(pplayer->units),
            nation_rule_name(nation_of_player(pplayer)));
  }

  city_list_iterate(pplayer->cities, pcity) {
    game_remove_city(pcity);
  } city_list_iterate_end;
  if (0 != city_list_size(pplayer->cities)) {
    freelog(LOG_ERROR, "game_remove_player() failed to remove %d %s cities",
            city_list_size(pplayer->cities),
            nation_rule_name(nation_of_player(pplayer)));
  }
}

/****************************************************************************
  Reset a player's data to its initial state.  After calling this you
  must call player_init before the player can be used again.
****************************************************************************/
void game_remove_player(struct player *pplayer)
{
  game_player_reset(pplayer);

#if 0
  assert(conn_list_size(pplayer->connections) == 0);
  /* FIXME: Connections that are unlinked here are left dangling.  It's up to
   * the caller to fix them.  This happens when /loading a game while a
   * client is connected. */
#endif
  conn_list_free(pplayer->connections);
  pplayer->connections = NULL;

  unit_list_free(pplayer->units);
  pplayer->units = NULL;

  city_list_free(pplayer->cities);
  pplayer->cities = NULL;

  /* This comes last because log calls in the above functions may use it. */
  if (pplayer->nation != NULL) {
    player_set_nation(pplayer, NULL);
  }
}

/***************************************************************
  Remove all initialized players. This is all player slots, 
  since we initialize them all on game initialization.
***************************************************************/
static void game_remove_all_players(void)
{
  player_slots_iterate(pslot) {
    game_remove_player(pslot);
    player_slot_set_used(pslot, FALSE);
  } player_slots_iterate_end;

  set_player_count(0);
}

/***************************************************************
  Frees all memory of the game.
***************************************************************/
void game_free(void)
{
  game_remove_all_players();
  map_free();
  idex_free();
  game_ruleset_free();
  cm_free();
}

/***************************************************************
  Do all changes to change view, and not full
  game_free()/game_init().
***************************************************************/
void game_reset(void)
{
  if (is_server()) {
    game_free();
    game_init();
  } else {
    /* Should do exactly the same as players_iterate here. */
    player_slots_iterate(pslot) {
      game_player_reset(pslot);
    } player_slots_iterate_end;

    map_free();
    idex_free();

    map_init();
    idex_init();
  }
}

/***************************************************************
  Initialize the objects which will read from a ruleset.
***************************************************************/
void game_ruleset_init(void)
{
  ruleset_cache_init();
  terrains_init();
  base_types_init();
  improvements_init();
  techs_init();
  unit_classes_init();
  unit_types_init();
  specialists_init();
}

/***************************************************************
  Frees all memory which in objects which are read from a ruleset.
***************************************************************/
void game_ruleset_free(void)
{
  specialists_free();
  techs_free();
  governments_free();
  nations_free();
  unit_types_free();
  unit_flags_free();
  improvements_free();
  base_types_free();
  city_styles_free();
  terrains_free();
  ruleset_cache_free();
  nation_groups_free();
}


/***************************************************************
...
***************************************************************/
void initialize_globals(void)
{
  players_iterate(pplayer) {
    city_list_iterate(pplayer->cities, pcity) {
      city_built_iterate(pcity, pimprove) {
        if (is_wonder(pimprove)) {
          if (is_great_wonder(pimprove)) {
            game.info.great_wonder_owners[improvement_index(pimprove)] =
                player_number(pplayer);
          }
          pplayer->wonders[improvement_index(pimprove)] = pcity->id;
        }
      } city_built_iterate_end;
    } city_list_iterate_end;
  } players_iterate_end;
}

/***************************************************************
  Returns the next year in the game.
***************************************************************/
int game_next_year(int year)
{
  int increase = get_world_bonus(EFT_TURN_YEARS);
  const int slowdown = (game.info.spacerace
			? get_world_bonus(EFT_SLOW_DOWN_TIMELINE) : 0);

  if (game.info.year_0_hack) {
    /* hacked it to get rid of year 0 */
    year = 0;
    game.info.year_0_hack = FALSE;
  }

    /* !McFred: 
       - want year += 1 for spaceship.
    */

  /* test game with 7 normal AI's, gen 4 map, foodbox 10, foodbase 0: 
   * Gunpowder about 0 AD
   * Railroad  about 500 AD
   * Electricity about 1000 AD
   * Refining about 1500 AD (212 active units)
   * about 1750 AD
   * about 1900 AD
   */

  /* Note the slowdown operates even if Enable_Space is not active.  See
   * README.effects for specifics. */
  if (slowdown >= 3) {
    if (increase > 1) {
      increase = 1;
    }
  } else if (slowdown >= 2) {
    if (increase > 2) {
      increase = 2;
    }
  } else if (slowdown >= 1) {
    if (increase > 5) {
      increase = 5;
    }
  }

  year += increase;

  if (year == 0 && game.info.calendar_skip_0) {
    year = 1;
    game.info.year_0_hack = TRUE;
  }

  return year;
}

/***************************************************************
  Advance the game year.
***************************************************************/
void game_advance_year(void)
{
  game.info.year = game_next_year(game.info.year);
  game.info.turn++;
}

/**************************************************************************
  Return TRUE if it is this player's phase.
  NB: The meaning of the 'phase' argument must match its use in the
  function begin_turn() in server/srv_main.c.
  NB: The phase mode PMT_TEAMS_ALTERNATE assumes that every player is
  on a team, i.e. that pplayer->team is never NULL.
**************************************************************************/
bool is_player_phase(const struct player *pplayer, int phase)
{
  switch (game.info.phase_mode) {
  case PMT_CONCURRENT:
    return TRUE;
    break;
  case PMT_PLAYERS_ALTERNATE:
    return player_number(pplayer) == phase;
    break;
  case PMT_TEAMS_ALTERNATE:
    assert(pplayer->team != NULL);
    return team_number(pplayer->team) == phase;
    break;
  default:
    break;
  }

  freelog(LOG_FATAL, "Unrecognized phase mode %d in is_player_phase().",
          phase);
  assert(FALSE);
  return TRUE;
}

/****************************************************************************
  Return a prettily formatted string containing the population text.  The
  population is passed in as the number of citizens, in thousands.
****************************************************************************/
const char *population_to_text(int thousand_citizen)
{
  /* big_int_to_text can't handle negative values, and in any case we'd
   * better not have a negative population. */
  assert(thousand_citizen >= 0);
  return big_int_to_text(thousand_citizen, 3);
}

/****************************************************************************
  Returns gui name string
****************************************************************************/
const char *gui_name(enum gui_type gui)
{
  /* This must be in same order as enum gui_type in fc_types.h */
  const char *gui_names[] = {
    "stub", "gtk2", "sdl", "xaw", "win32", "ftwl" };

  if (gui < GUI_LAST) {
    return gui_names[gui];
  } else {
    return "Unknown";
  }
}

/****************************************************************************
  Returns whether the specified server setting class can currently
  be changed.  Does not indicate whether it can be changed by clients.
****************************************************************************/
bool setting_class_is_changeable(enum sset_class class)
{
  switch (class) {
  case SSET_MAP_SIZE:
  case SSET_MAP_GEN:
    /* Only change map options if we don't yet have a map: */
    return map_is_empty();

  case SSET_MAP_ADD:
  case SSET_PLAYERS:
  case SSET_GAME_INIT:
  case SSET_RULES:
    /* Only change start params and most rules if we don't yet have a map,
     * or if we do have a map but its a scenario one (ie, the game has
     * never actually been started).
     */
    return (map_is_empty() || game.info.is_new_game);

  case SSET_RULES_FLEXIBLE:
  case SSET_META:
    /* These can always be changed: */
    return TRUE;

  case SSET_LAST:
    break;
  }
  freelog(LOG_ERROR, "Unexpected case %d in %s line %d",
	  class, __FILE__, __LINE__);
  return FALSE;
}

/****************************************************************************
  Produce a statically allocated textual representation of the given
  year.
****************************************************************************/
const char *textyear(int year)
{
  static char y[32];
  if (year < 0) {
    /* TRANS: <year> <label> -> "1000 BC" */
    my_snprintf(y, sizeof(y), _("%d %s"), -year,
                game.info.negative_year_label);
  } else {
    /* TRANS: <year> <label> -> "1000 AD" */
    my_snprintf(y, sizeof(y), _("%d %s"), year,
                game.info.positive_year_label);
  }
  return y;
}
