/********************************************************************** 
 Freeciv - Copyright (C) 1996-2004 - The Freeciv Project
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

#include "fcintl.h"
#include "game.h"
#include "ioz.h"
#include "log.h"

#include "map.h"

#include "ggzserver.h"
#include "plrhand.h"
#include "report.h"
#include "settings.h"
#include "srv_main.h"
#include "stdinhand.h"

/* Category names must match the values in enum sset_category. */
const char *sset_category_names[] = {N_("Geological"),
				     N_("Sociological"),
				     N_("Economic"),
				     N_("Military"),
				     N_("Scientific"),
				     N_("Internal"),
				     N_("Networking")};

/* Level names must match the values in enum sset_level. */
const char *sset_level_names[] = {N_("None"),
				  N_("All"),
				  N_("Vital"),
				  N_("Situational"),
				  N_("Rare"),
				  N_("Changed")};
const int OLEVELS_NUM = ARRAY_SIZE(sset_level_names);

/*************************************************************************
  Verify that a given allowtake string is valid.  See
  game.allow_take.
*************************************************************************/
static bool allowtake_callback(const char *value,
                               struct connection *caller,
                               const char **error_string)
{
  int len = strlen(value), i;
  bool havecharacter_state = FALSE;

  /* We check each character individually to see if it's valid.  This
   * does not check for duplicate entries.
   *
   * We also track the state of the machine.  havecharacter_state is
   * true if the preceeding character was a primary label, e.g.
   * NHhAadb.  It is false if the preceeding character was a modifier
   * or if this is the first character. */

  for (i = 0; i < len; i++) {
    /* Check to see if the character is a primary label. */
    if (strchr("HhAadbOo", value[i])) {
      havecharacter_state = TRUE;
      continue;
    }

    /* If we've already passed a primary label, check to see if the
     * character is a modifier. */
    if (havecharacter_state && strchr("1234", value[i])) {
      havecharacter_state = FALSE;
      continue;
    }

    /* Looks like the character was invalid. */
    *error_string = _("Allowed take string contains invalid\n"
		      "characters.  Try \"help allowtake\".");
    return FALSE;
  }

  /* All characters were valid. */
  *error_string = NULL;
  return TRUE;
}

/*************************************************************************
  Verify that a given startunits string is valid.  See
  game.info.start_units.
*************************************************************************/
static bool startunits_callback(const char *value,
                                struct connection *caller,
                                const char **error_string)
{
  int len = strlen(value), i;
  bool have_founder = FALSE;

  /* We check each character individually to see if it's valid, and
   * also make sure there is at least one city founder. */

  for (i = 0; i < len; i++) {
    /* Check for a city founder */
    if (value[i] == 'c') {
      have_founder = TRUE;
      continue;
    }
    /* TODO: add 'f' back in here when we can support ferry units */
    if (strchr("cwxksdDaA", value[i])) {
      continue;
    }

    /* Looks like the character was invalid. */
    *error_string = _("Starting units string contains invalid\n"
		      "characters.  Try \"help startunits\".");
    return FALSE;
  }

  if (!have_founder) {
    *error_string = _("Starting units string does not contain\n"
		      "at least one city founder.  Try \n"
		      "\"help startunits\".");
    return FALSE;
  }
  /* All characters were valid. */
  *error_string = NULL;
  return TRUE;
}

/*************************************************************************
  Verify that a given endturn is valid.
*************************************************************************/
static bool endturn_callback(int value, struct connection *caller,
                             const char **error_string)
{
  if (value < game.info.turn) {
    /* Tried to set endturn earlier than current turn */
    *error_string = _("Cannot set endturn earlier than current turn.");
    return FALSE;
  }
  return TRUE;
}

/*************************************************************************
  Verify that a given maxplayers string is valid.
*************************************************************************/
static bool maxplayers_callback(int value, struct connection *caller,
                                const char **error_string)
{
#ifdef GGZ_SERVER
  if (with_ggz) {
    /* In GGZ mode the maxplayers is the number of actual players - set
     * when the game is lauched and not changed thereafter.  This may be
     * changed in future. */
    *error_string = _("Cannot change maxplayers in GGZ mode.");
    return FALSE;
  }
#endif
  if (value < player_count()) {
    *error_string =_("Number of players is higher than requested value;\n"
		     "Keeping old value.");
    return FALSE;
  }

  *error_string = NULL;
  return TRUE;
}

/*************************************************************************
  Disallow low timeout values for non-hack connections.
*************************************************************************/
static bool timeout_callback(int value, struct connection *caller,
                             const char **error_string)
{
  if (caller && caller->access_level < ALLOW_HACK && value < 30) {
    *error_string = _("You are not allowed to set timeout values less "
                      "than 30 seconds.");
    return FALSE;
  }

  *error_string = NULL;
  return TRUE;
}

/*************************************************************************
  Check that everyone is on a team for team-alternating simultaneous
  phases. NB: Assumes that it is not possible to first set team
  alternating phase mode then make teamless players.
*************************************************************************/
static bool phasemode_callback(int value, struct connection *caller,
                               const char **error_string)
{
  if (value == PMT_TEAMS_ALTERNATE) {
    players_iterate(pplayer) {
      if (!pplayer->team) {
        *error_string = _("All players must have a team if this option "
                          "value is used.");
        return FALSE;
      }
    } players_iterate_end;
  }
  *error_string = NULL;
  return TRUE;
}

#define GEN_BOOL(name, value, sclass, scateg, slevel, to_client,	\
		 short_help, extra_help, func, _default)		\
  {name, sclass, to_client, short_help, extra_help, SSET_BOOL,		\
      scateg, slevel, &value, _default, func,				\
      NULL, 0, NULL, 0, 0,						\
      NULL, NULL, NULL, 0},

#define GEN_INT(name, value, sclass, scateg, slevel, to_client,		\
		short_help, extra_help, func, _min, _max, _default)	\
  {name, sclass, to_client, short_help, extra_help, SSET_INT,		\
      scateg, slevel,							\
      NULL, FALSE, NULL,						\
      &value, _default, func, _min, _max,				\
      NULL, NULL, NULL, 0},

#define GEN_STRING(name, value, sclass, scateg, slevel, to_client,	\
		   short_help, extra_help, func, _default)		\
  {name, sclass, to_client, short_help, extra_help, SSET_STRING,	\
      scateg, slevel,							\
      NULL, FALSE, NULL,						\
      NULL, 0, NULL, 0, 0,						\
      value, _default, func, sizeof(value)},

#define GEN_END							\
  {NULL, SSET_LAST, SSET_SERVER_ONLY, NULL, NULL, SSET_INT,	\
      SSET_NUM_CATEGORIES, SSET_NONE,				\
      NULL, FALSE, NULL,					\
      NULL, 0, NULL, 0, 0,					\
      NULL, NULL, NULL},

struct settings_s settings[] = {

  /* These should be grouped by sclass */
  
  /* Map size parameters: adjustable if we don't yet have a map */  
  GEN_INT("size", map.server.size, SSET_MAP_SIZE,
	  SSET_GEOLOGY, SSET_VITAL, SSET_TO_CLIENT,
          N_("Map size (in thousands of tiles)"),
          N_("This value is used to determine the map dimensions.\n"
             "  size = 4 is a normal map of 4,000 tiles (default)\n"
             "  size = 20 is a huge map of 20,000 tiles"), NULL,
          MAP_MIN_SIZE, MAP_MAX_SIZE, MAP_DEFAULT_SIZE)
  GEN_INT("topology", map.topology_id, SSET_MAP_SIZE,
	  SSET_GEOLOGY, SSET_VITAL, SSET_TO_CLIENT,
	  N_("Map topology index"),
	  /* TRANS: do not edit the ugly ASCII art */
	  N_("Freeciv maps are always two-dimensional. They may wrap at "
	     "the north-south and east-west directions to form a flat map, "
	     "a cylinder, or a torus (donut). Individual tiles may be "
	     "rectangular or hexagonal, with either a classic or isometric "
	     "alignment - this should be set based on the tileset being "
	     "used.\n"
             "   0 Flat Earth (unwrapped)\n"
             "   1 Earth (wraps E-W)\n"
             "   2 Uranus (wraps N-S)\n"
             "   3 Donut World (wraps N-S, E-W)\n"
	     "   4 Flat Earth (isometric)\n"
	     "   5 Earth (isometric)\n"
	     "   6 Uranus (isometric)\n"
	     "   7 Donut World (isometric)\n"
	     "   8 Flat Earth (hexagonal)\n"
	     "   9 Earth (hexagonal)\n"
	     "  10 Uranus (hexagonal)\n"
	     "  11 Donut World (hexagonal)\n"
	     "  12 Flat Earth (iso-hex)\n"
	     "  13 Earth (iso-hex)\n"
	     "  14 Uranus (iso-hex)\n"
	     "  15 Donut World (iso-hex)\n"
	     "Classic rectangular:       Isometric rectangular:\n"
	     "      _________               /\\/\\/\\/\\/\\ \n"
	     "     |_|_|_|_|_|             /\\/\\/\\/\\/\\/ \n"
	     "     |_|_|_|_|_|             \\/\\/\\/\\/\\/\\\n"
	     "     |_|_|_|_|_|             /\\/\\/\\/\\/\\/ \n"
	     "                             \\/\\/\\/\\/\\/  \n"
	     "Hex tiles:                 Iso-hex:\n"
	     "  /\\/\\/\\/\\/\\/\\               _   _   _   _   _       \n"
	     "  | | | | | | |             / \\_/ \\_/ \\_/ \\_/ \\      \n"
	     "  \\/\\/\\/\\/\\/\\/\\             \\_/ \\_/ \\_/ \\_/ \\_/  \n"
	     "   | | | | | | |            / \\_/ \\_/ \\_/ \\_/ \\      \n"
	     "   \\/\\/\\/\\/\\/\\/             \\_/ \\_/ \\_/ \\_/ \\_/    \n"
          ), NULL,
	  MAP_MIN_TOPO, MAP_MAX_TOPO, MAP_DEFAULT_TOPO)

  /* Map generation parameters: once we have a map these are of historical
   * interest only, and cannot be changed.
   */
  GEN_INT("generator", map.server.generator,
	  SSET_MAP_GEN, SSET_GEOLOGY, SSET_VITAL,  SSET_TO_CLIENT,
	  N_("Method used to generate map"),
	  N_("0 = Scenario map - no generator;\n"
	     "1 = Fully random height generator;              [4]\n"
	     "2 = Pseudo-fractal height generator;             [3]\n"
	     "3 = Island-based generator (fairer but boring)   [1]\n"
	     "\n"
	     "Numbers in [] give the default values for placement of "
	     "starting positions.  If the default value of startpos is "
	     "used then a startpos setting will be chosen based on the "
	     "generator.  See the \"startpos\" setting."), NULL,
	  MAP_MIN_GENERATOR, MAP_MAX_GENERATOR, MAP_DEFAULT_GENERATOR)

  GEN_INT("startpos", map.server.startpos,
	  SSET_MAP_GEN, SSET_GEOLOGY, SSET_VITAL,  SSET_TO_CLIENT,
	  N_("Method used to choose start positions"),
	  N_("0 = Generator's choice.  Selecting this setting means\n"
	     "    the default value will be picked based on the generator\n"
	     "    chosen.  See the \"generator\" setting.\n"
	     "1 = Try to place one player per continent.\n"
	     "2 = Try to place two players per continent.\n"
	     "3 = Try to place all players on a single continent.\n"
	     "4 = Place players depending on size of continents.\n"
	     "Note: generators try to create the right number of continents "
	     "for the choice of start pos and to the number of players"),
	  NULL, MAP_MIN_STARTPOS, MAP_MAX_STARTPOS, MAP_DEFAULT_STARTPOS)

  GEN_BOOL("tinyisles", map.server.tinyisles,
	   SSET_MAP_GEN, SSET_GEOLOGY, SSET_RARE, SSET_TO_CLIENT,
	   N_("Presence of 1x1 islands"),
	   N_("0 = no 1x1 islands; 1 = some 1x1 islands"), NULL,
	   MAP_DEFAULT_TINYISLES)

  GEN_BOOL("separatepoles", map.server.separatepoles,
	   SSET_MAP_GEN, SSET_GEOLOGY, SSET_SITUATIONAL, SSET_TO_CLIENT,
	   N_("Whether the poles are separate continents"),
	   N_("0 = continents may attach to poles; 1 = poles will "
	      "usually be separate"), NULL, 
	   MAP_DEFAULT_SEPARATE_POLES)

  GEN_BOOL("alltemperate", map.server.alltemperate, 
           SSET_MAP_GEN, SSET_GEOLOGY, SSET_RARE, SSET_TO_CLIENT,
	   N_("All the map is temperate"),
	   N_("0 = normal Earth-like planet; 1 = all-temperate planet "),
	   NULL, MAP_DEFAULT_ALLTEMPERATE)

  GEN_INT("temperature", map.server.temperature,
 	  SSET_MAP_GEN, SSET_GEOLOGY, SSET_SITUATIONAL, SSET_TO_CLIENT,
 	  N_("Average temperature of the planet"),
 	  N_("Small values will give a cold map, while larger values will "
             "give a hotter map.\n"
	     "\n"
	     "100 means a very dry and hot planet with no polar arctic "
	     "zones, only tropical and dry zones.\n\n"
	     "70 means a hot planet with little polar ice.\n\n"
             "50 means a temperate planet with normal polar, cold, "
	     "temperate, and tropical zones; a desert zone overlaps "
	     "tropical and temperate zones.\n\n"
	     "30 means a cold planet with small tropical zones.\n\n"
	     "0 means a very cold planet with large polar zones and no "
	     "tropics"), 
          NULL,
  	  MAP_MIN_TEMPERATURE, MAP_MAX_TEMPERATURE, MAP_DEFAULT_TEMPERATURE)
 
  GEN_INT("landmass", map.server.landpercent,
	  SSET_MAP_GEN, SSET_GEOLOGY, SSET_SITUATIONAL, SSET_TO_CLIENT,
	  N_("Percentage of the map that is land"),
	  N_("This setting gives the approximate percentage of the map "
	     "that will be made into land."), NULL,
	  MAP_MIN_LANDMASS, MAP_MAX_LANDMASS, MAP_DEFAULT_LANDMASS)

  GEN_INT("steepness", map.server.steepness,
	  SSET_MAP_GEN, SSET_GEOLOGY, SSET_SITUATIONAL, SSET_TO_CLIENT,
	  N_("Amount of hills/mountains"),
	  N_("Small values give flat maps, while higher values give a "
	     "steeper map with more hills and mountains."), NULL,
	  MAP_MIN_STEEPNESS, MAP_MAX_STEEPNESS, MAP_DEFAULT_STEEPNESS)

  GEN_INT("wetness", map.server.wetness,
 	  SSET_MAP_GEN, SSET_GEOLOGY, SSET_SITUATIONAL, SSET_TO_CLIENT,
 	  N_("Amount of water on lands"), 
	  N_("Small values mean lots of dry, desert-like land; "
	     "higher values give a wetter map with more swamps, "
	     "jungles, and rivers."), NULL, 
 	  MAP_MIN_WETNESS, MAP_MAX_WETNESS, MAP_DEFAULT_WETNESS)

  GEN_INT("mapseed", map.server.seed,
	  SSET_MAP_GEN, SSET_INTERNAL, SSET_RARE, SSET_SERVER_ONLY,
	  N_("Map generation random seed"),
	  N_("The same seed will always produce the same map; "
	     "for zero (the default) a seed will be chosen based on "
	     "the time to give a random map. This setting is usually "
	     "only of interest while debugging the game."), NULL, 
	  MAP_MIN_SEED, MAP_MAX_SEED, MAP_DEFAULT_SEED)

  /* Map additional stuff: huts and specials.  gameseed also goes here
   * because huts and specials are the first time the gameseed gets used (?)
   * These are done when the game starts, so these are historical and
   * fixed after the game has started.
   */
  GEN_INT("gameseed", game.server.seed,
	  SSET_MAP_ADD, SSET_INTERNAL, SSET_RARE, SSET_SERVER_ONLY,
	  N_("Game random seed"),
	  N_("For zero (the default) a seed will be chosen based "
	     "on the time. This setting is usually "
	     "only of interest while debugging the game"), NULL, 
	  GAME_MIN_SEED, GAME_MAX_SEED, GAME_DEFAULT_SEED)

  GEN_INT("specials", map.server.riches,
	  SSET_MAP_ADD, SSET_GEOLOGY, SSET_VITAL, SSET_TO_CLIENT,
	  N_("Amount of \"special\" resource squares"), 
	  N_("Special resources improve the basic terrain type they "
	     "are on. The server variable's scale is parts per "
	     "thousand."), NULL,
	  MAP_MIN_RICHES, MAP_MAX_RICHES, MAP_DEFAULT_RICHES)

  GEN_INT("huts", map.server.huts,
	  SSET_MAP_ADD, SSET_GEOLOGY, SSET_VITAL, SSET_TO_CLIENT,
	  N_("Amount of huts (minor tribe villages)"),
	  N_("This setting gives the exact number of huts that will be "
	     "placed on the entire map. Huts are small tribal villages "
	     "that may be investigated by units."),
	  NULL, MAP_MIN_HUTS, MAP_MAX_HUTS, MAP_DEFAULT_HUTS)

  /* Options affecting numbers of players and AI players.  These only
   * affect the start of the game and can not be adjusted after that.
   * (Actually, minplayers does also affect reloads: you can't start a
   * reload game until enough players have connected (or are AI).)
   */
  GEN_INT("minplayers", game.info.min_players,
	  SSET_PLAYERS, SSET_INTERNAL, SSET_VITAL,
          SSET_TO_CLIENT,
	  N_("Minimum number of players"),
	  N_("There must be at least this many players (connected "
	     "human players) before the game can start."),
	  NULL,
	  GAME_MIN_MIN_PLAYERS, GAME_MAX_MIN_PLAYERS, GAME_DEFAULT_MIN_PLAYERS)

  GEN_INT("maxplayers", game.info.max_players,
	  SSET_PLAYERS, SSET_INTERNAL, SSET_VITAL, SSET_TO_CLIENT,
	  N_("Maximum number of players"),
          N_("The maximal number of human and AI players who can be in "
             "the game. When this number of players are connected in "
             "the pregame state, any new players who try to connect "
             "will be rejected."), maxplayers_callback,
	  GAME_MIN_MAX_PLAYERS, GAME_MAX_MAX_PLAYERS, GAME_DEFAULT_MAX_PLAYERS)

  GEN_INT("aifill", game.info.aifill,
	  SSET_PLAYERS, SSET_INTERNAL, SSET_VITAL, SSET_TO_CLIENT,
	  N_("Limited number of AI players"),
	  N_("If set to a positive value, then AI players will be "
	     "automatically created or removed to keep the total "
	     "number of players at this amount.  As more players join, "
	     "these AI players will be replaced.  When set to zero, "
	     "all AI players will be removed."), NULL,
	  GAME_MIN_AIFILL, GAME_MAX_AIFILL, GAME_DEFAULT_AIFILL)

  /* Game initialization parameters (only affect the first start of the game,
   * and not reloads).  Can not be changed after first start of game.
   */
  /* TODO: Add this line back when we can support Ferry units */
  /* "    f   = Ferryboat (eg., Trireme)\n" */
  GEN_STRING("startunits", game.info.start_units,
	     SSET_GAME_INIT, SSET_SOCIOLOGY, SSET_VITAL, SSET_TO_CLIENT,
             N_("List of players' initial units"),
             N_("This should be a string of characters, each of which "
		"specifies a unit role. There must be at least one city "
		"founder in the string. The characters and their "
		"meanings are:\n"
		"    c   = City founder (eg., Settlers)\n"
		"    w   = Terrain worker (eg., Engineers)\n"
		"    x   = Explorer (eg., Explorer)\n"
		"    k   = Gameloss (eg., King)\n"
		"    s   = Diplomat (eg., Diplomat)\n"
		"    d   = Ok defense unit (eg., Warriors)\n"
		"    D   = Good defense unit (eg., Phalanx)\n"
		"    a   = Fast attack unit (eg., Horsemen)\n"
		"    A   = Strong attack unit (eg., Catapult)\n"),
		startunits_callback, GAME_DEFAULT_START_UNITS)

  GEN_INT("dispersion", game.info.dispersion,
	  SSET_GAME_INIT, SSET_SOCIOLOGY, SSET_SITUATIONAL, SSET_TO_CLIENT,
	  N_("Area where initial units are located"),
	  N_("This is the radius within "
	     "which the initial units are dispersed."), NULL,
	  GAME_MIN_DISPERSION, GAME_MAX_DISPERSION, GAME_DEFAULT_DISPERSION)

  GEN_INT("gold", game.info.gold,
	  SSET_GAME_INIT, SSET_ECONOMICS, SSET_VITAL, SSET_TO_CLIENT,
	  N_("Starting gold per player"), 
	  N_("At the beginning of the game, each player is given this "
	     "much gold."), NULL,
	  GAME_MIN_GOLD, GAME_MAX_GOLD, GAME_DEFAULT_GOLD)

  GEN_INT("techlevel", game.info.tech,
	  SSET_GAME_INIT, SSET_SCIENCE, SSET_VITAL, SSET_TO_CLIENT,
	  N_("Number of initial techs per player"), 
	  N_("At the beginning of the game, each player is given this "
	     "many technologies. The technologies chosen are random for "
	     "each player. Depending on the value of tech_cost_style in "
             "the ruleset, a big value for techlevel can make the next "
             "techs really expensive."), NULL,
	  GAME_MIN_TECHLEVEL, GAME_MAX_TECHLEVEL, GAME_DEFAULT_TECHLEVEL)

  GEN_INT("sciencebox", game.info.sciencebox,
	  SSET_RULES, SSET_SCIENCE, SSET_SITUATIONAL, SSET_TO_CLIENT,
	  N_("Technology cost multiplier percentage"),
	  N_("This affects how quickly players can research new "
	     "technology. All tech costs are multiplied by this amount "
	     "(as a percentage). The base tech costs are determined by "
	     "the ruleset or other game settings."),
	  NULL, GAME_MIN_SCIENCEBOX, GAME_MAX_SCIENCEBOX, 
	  GAME_DEFAULT_SCIENCEBOX)

  GEN_INT("techpenalty", game.info.techpenalty,
	  SSET_RULES, SSET_SCIENCE, SSET_RARE, SSET_TO_CLIENT,
	  N_("Percentage penalty when changing tech"),
	  N_("If you change your current research technology, and you have "
	     "positive research points, you lose this percentage of those "
	     "research points. This does not apply when you have just gained "
	     "a technology this turn."), NULL,
	  GAME_MIN_TECHPENALTY, GAME_MAX_TECHPENALTY,
	  GAME_DEFAULT_TECHPENALTY)

  GEN_INT("diplcost", game.info.diplcost,
	  SSET_RULES, SSET_SCIENCE, SSET_RARE, SSET_TO_CLIENT,
	  N_("Penalty when getting tech or gold from treaty"),
	  N_("For each technology you gain from a diplomatic treaty, you "
	     "lose research points equal to this percentage of the cost to "
	     "research a new technology. If this is non-zero, you can end up "
	     "with negative research points. Also applies to gold "
             "transfers in diplomatic treaties."),
          NULL,
	  GAME_MIN_DIPLCOST, GAME_MAX_DIPLCOST, GAME_DEFAULT_DIPLCOST)

  GEN_INT("conquercost", game.info.conquercost,
	  SSET_RULES, SSET_SCIENCE, SSET_RARE, SSET_TO_CLIENT,
	  N_("Penalty when getting tech from conquering"),
	  N_("For each technology you gain by conquering an enemy city, you "
	     "lose research points equal to this percentage of the cost to "
	     "research a new technology. If this is non-zero, you can end up "
	     "with negative research points."),
	  NULL,
	  GAME_MIN_CONQUERCOST, GAME_MAX_CONQUERCOST,
	  GAME_DEFAULT_CONQUERCOST)

  GEN_INT("freecost", game.info.freecost,
	  SSET_RULES, SSET_SCIENCE, SSET_RARE, SSET_TO_CLIENT,
	  N_("Penalty when getting a free tech"),
	  N_("For each technology you gain \"for free\" (other than "
	     "covered by diplcost or conquercost: specifically, from huts "
	     "or from Great Library effects), you lose research points "
	     "equal to this percentage of the cost to research a new "
	     "technology. If this is non-zero, you can end up "
	     "with negative research points."), 
	  NULL, 
	  GAME_MIN_FREECOST, GAME_MAX_FREECOST, GAME_DEFAULT_FREECOST)

  GEN_INT("foodbox", game.info.foodbox,
	  SSET_RULES, SSET_ECONOMICS, SSET_SITUATIONAL, SSET_TO_CLIENT,
	  N_("Food required for a city to grow"),
	  N_("This is the base amount of food required to grow a city. "
	     "This value is multiplied by another factor that comes from "
	     "the ruleset and is dependent on the size of the city."),
	  NULL,
	  GAME_MIN_FOODBOX, GAME_MAX_FOODBOX, GAME_DEFAULT_FOODBOX)

  GEN_INT("aqueductloss", game.info.aqueductloss,
	  SSET_RULES, SSET_ECONOMICS, SSET_RARE, SSET_TO_CLIENT,
	  N_("Percentage food lost when building needed"),
	  N_("If a city would expand, but it can't because it needs "
	     "an Aqueduct (or Sewer System), it loses this percentage "
	     "of its foodbox (or half that amount when it has a "
	     "Granary)."), NULL, 
	  GAME_MIN_AQUEDUCTLOSS, GAME_MAX_AQUEDUCTLOSS, 
	  GAME_DEFAULT_AQUEDUCTLOSS)

  GEN_INT("shieldbox", game.info.shieldbox,
	  SSET_RULES, SSET_ECONOMICS, SSET_SITUATIONAL, SSET_TO_CLIENT,
	  N_("Multiplier percentage for production costs"),
	  N_("This affects how quickly units and buildings can be "
	     "produced.  The base costs are multiplied by this value (as "
	     "a percentage)."),
	  NULL, GAME_MIN_SHIELDBOX, GAME_MAX_SHIELDBOX,
	  GAME_DEFAULT_SHIELDBOX)

  /* Notradesize and fulltradesize used to have callbacks to prevent them
   * from being set illegally (notradesize > fulltradesize).  However this
   * provided a problem when setting them both through the client's settings
   * dialog, since they cannot both be set atomically.  So the callbacks were
   * removed and instead the game now knows how to deal with invalid
   * settings. */
  GEN_INT("fulltradesize", game.info.fulltradesize,
	  SSET_RULES, SSET_ECONOMICS, SSET_RARE, SSET_TO_CLIENT,
	  N_("Minimum city size to get full trade"),
	  N_("There is a trade penalty in all cities smaller than this. "
	     "The penalty is 100% (no trade at all) for sizes up to "
	     "notradesize, and decreases gradually to 0% (no penalty "
	     "except the normal corruption) for size=fulltradesize. "
	     "See also notradesize."), NULL, 
	  GAME_MIN_FULLTRADESIZE, GAME_MAX_FULLTRADESIZE, 
	  GAME_DEFAULT_FULLTRADESIZE)

  GEN_INT("notradesize", game.info.notradesize,
	  SSET_RULES, SSET_ECONOMICS, SSET_RARE, SSET_TO_CLIENT,
	  N_("Maximum size of a city without trade"),
	  N_("Cities do not produce any trade at all unless their size "
	     "is larger than this amount. The produced trade increases "
	     "gradually for cities larger than notradesize and smaller "
	     "than fulltradesize. See also fulltradesize."), NULL,
	  GAME_MIN_NOTRADESIZE, GAME_MAX_NOTRADESIZE,
	  GAME_DEFAULT_NOTRADESIZE)

  GEN_INT("citymindist", game.info.citymindist,
	  SSET_RULES, SSET_SOCIOLOGY, SSET_SITUATIONAL, SSET_TO_CLIENT,
	  N_("Minimum distance between cities"),
	  N_("When a player attempts to found a new city, there may be "
	     "no other city in this distance. For example, when "
	     "this value is 3, there have to be at least two empty "
	     "fields between two cities in every direction. If set "
	     "to 0 (default), the ruleset value will be used."),
	  NULL,
	  GAME_MIN_CITYMINDIST, GAME_MAX_CITYMINDIST,
	  GAME_DEFAULT_CITYMINDIST)

  GEN_INT("trademindist", game.info.trademindist,
          SSET_RULES_FLEXIBLE, SSET_ECONOMICS, SSET_RARE, SSET_TO_CLIENT,
          N_("Minimum distance for traderoutes"),
          N_("In order to establish a traderoute, cities must be at "
             "least this far apart on the map. The distance is calculated "
             "as \"manhattan distance\", that is, the sum of the "
             "displacements along the x and y directions."), NULL,
          GAME_MIN_TRADEMINDIST, GAME_MAX_TRADEMINDIST,
          GAME_DEFAULT_TRADEMINDIST)

  GEN_INT("rapturedelay", game.info.rapturedelay,
	  SSET_RULES, SSET_SOCIOLOGY, SSET_SITUATIONAL, SSET_TO_CLIENT,
          N_("Number of turns between rapture effect"),
          N_("Sets the number of turns between rapture growth of a city. "
             "If set to n a city will grow after celebrating for n+1 "
	     "turns."),
	  NULL,
          GAME_MIN_RAPTUREDELAY, GAME_MAX_RAPTUREDELAY,
          GAME_DEFAULT_RAPTUREDELAY)

  GEN_INT("razechance", game.info.razechance,
	  SSET_RULES, SSET_MILITARY, SSET_RARE, SSET_TO_CLIENT,
	  N_("Chance for conquered building destruction"),
	  N_("When a player conquers a city, each city improvement has this "
	     "percentage chance to be destroyed."), NULL, 
	  GAME_MIN_RAZECHANCE, GAME_MAX_RAZECHANCE, GAME_DEFAULT_RAZECHANCE)

  GEN_INT("occupychance", game.info.occupychance,
	  SSET_RULES, SSET_MILITARY, SSET_RARE, SSET_TO_CLIENT,
	  N_("Chance of moving into tile after attack"),
	  N_("If set to 0, combat is Civ1/2-style (when you attack, "
	     "you remain in place). If set to 100, attacking units "
	     "will always move into the tile they attacked when they win "
	     "the combat (and no enemy units remain in the tile). If "
	     "set to a value between 0 and 100, this will be used as "
	     "the percent chance of \"occupying\" territory."), NULL, 
	  GAME_MIN_OCCUPYCHANCE, GAME_MAX_OCCUPYCHANCE, 
	  GAME_DEFAULT_OCCUPYCHANCE)

  GEN_BOOL("autoattack", game.info.autoattack, SSET_RULES_FLEXIBLE, SSET_MILITARY,
         SSET_SITUATIONAL, SSET_TO_CLIENT,
         N_("Turn on/off server-side autoattack"),
         N_("If set to on, units with move left will automatically "
            "consider attacking enemy units that move adjacent to them."), 
         NULL, GAME_DEFAULT_AUTOATTACK)

  GEN_INT("killcitizen", game.info.killcitizen,
	  SSET_RULES, SSET_MILITARY, SSET_RARE, SSET_TO_CLIENT,
	  N_("Reduce city population after attack"),
	  N_("This flag indicates whether city population is reduced "
	     "after successful attack of enemy unit, depending on "
	     "its movement type (OR-ed):\n"
	     "  1 = land\n"
	     "  2 = sea\n"
	     "  4 = heli\n"
	     "  8 = air"), NULL,
	  GAME_MIN_KILLCITIZEN, GAME_MAX_KILLCITIZEN,
	  GAME_DEFAULT_KILLCITIZEN)

  GEN_INT("borders", game.info.borders,
	  SSET_RULES, SSET_MILITARY, SSET_SITUATIONAL, SSET_TO_CLIENT,
	  N_("National borders"),
	  N_("If this is set to greater than 0, then any land tiles "
	     "around a fortress or city will be owned by that nation.\n"
             "  0 = Disabled\n"
             "  1 = Enabled\n"
             "  2 = See everything inside borders\n"
             "  3 = Borders expand to unknown, revealing tiles"),
	  NULL,
	  GAME_MIN_BORDERS, GAME_MAX_BORDERS, GAME_DEFAULT_BORDERS)

  GEN_BOOL("happyborders", game.info.happyborders,
	   SSET_RULES, SSET_MILITARY, SSET_SITUATIONAL,
	   SSET_TO_CLIENT,
	   N_("Units inside borders cause no unhappiness"),
	   N_("If this is set, units will not cause unhappiness when "
	      "inside your own borders."), NULL,
	   GAME_DEFAULT_HAPPYBORDERS)

  GEN_INT("diplomacy", game.info.diplomacy,
	  SSET_RULES, SSET_MILITARY, SSET_SITUATIONAL, SSET_TO_CLIENT,
	  N_("Ability to do diplomacy with other players"),
	  N_("0 = default; diplomacy is enabled for everyone.\n"
	     "1 = diplomacy is only allowed between human players.\n"
	     "2 = diplomacy is only allowed between AI players.\n"
             "3 = diplomacy is restricted to teams.\n"
             "4 = diplomacy is disabled for everyone."), NULL,
	  GAME_MIN_DIPLOMACY, GAME_MAX_DIPLOMACY, GAME_DEFAULT_DIPLOMACY)

  GEN_INT("citynames", game.info.allowed_city_names,
	  SSET_RULES, SSET_SOCIOLOGY, SSET_RARE, SSET_TO_CLIENT,
	  N_("Allowed city names"),
	  N_("0 = There are no restrictions: players can have "
	     "multiple cities with the same names.\n\n"
	     "1 = City names have to be unique to a player: "
	     "one player can't have multiple cities with the same name.\n\n"
	     "2 = City names have to be globally unique: "
	     "all cities in a game have to have different names.\n\n"
	     "3 = Like setting 2, but a player isn't allowed to use a "
	     "default city name of another nations unless it is a default "
	     "for their nation also."),
	  NULL,
	  GAME_MIN_ALLOWED_CITY_NAMES, GAME_MAX_ALLOWED_CITY_NAMES, 
	  GAME_DEFAULT_ALLOWED_CITY_NAMES)
  
  /* Flexible rules: these can be changed after the game has started.
   *
   * The distinction between "rules" and "flexible rules" is not always
   * clearcut, and some existing cases may be largely historical or
   * accidental.  However some generalizations can be made:
   *
   *   -- Low-level game mechanics should not be flexible (eg, rulesets).
   *   -- Options which would affect the game "state" (city production etc)
   *      should not be flexible (eg, foodbox).
   *   -- Options which are explicitly sent to the client (eg, in
   *      packet_game_info) should probably not be flexible, or at
   *      least need extra care to be flexible.
   */
  GEN_INT("barbarians", game.info.barbarianrate,
	  SSET_RULES_FLEXIBLE, SSET_MILITARY, SSET_VITAL, SSET_TO_CLIENT,
	  N_("Barbarian appearance frequency"),
	  N_("0 = no barbarians \n"
	     "1 = barbarians only in huts \n"
	     "2 = normal rate of barbarian appearance \n"
	     "3 = frequent barbarian uprising \n"
	     "4 = raging hordes, lots of barbarians"), NULL, 
	  GAME_MIN_BARBARIANRATE, GAME_MAX_BARBARIANRATE, 
	  GAME_DEFAULT_BARBARIANRATE)

  GEN_INT("onsetbarbs", game.info.onsetbarbarian,
	  SSET_RULES_FLEXIBLE, SSET_MILITARY, SSET_VITAL, SSET_TO_CLIENT,
	  N_("Barbarian onset turn"),
	  N_("Barbarians will not appear before this turn."), NULL,
	  GAME_MIN_ONSETBARBARIAN, GAME_MAX_ONSETBARBARIAN, 
	  GAME_DEFAULT_ONSETBARBARIAN)

  GEN_INT("revolen", game.info.revolution_length,
	  SSET_RULES_FLEXIBLE, SSET_SOCIOLOGY, SSET_RARE, SSET_TO_CLIENT,
	  N_("Length in turns of revolution"),
	  N_("When changing governments, a period of anarchy lasting this "
	     "many turns will occur. "
             "Setting this value to 0 will give a random "
             "length of 1-6 turns."), NULL, 
	  GAME_MIN_REVOLUTION_LENGTH, GAME_MAX_REVOLUTION_LENGTH, 
	  GAME_DEFAULT_REVOLUTION_LENGTH)

  GEN_BOOL("fogofwar", game.info.fogofwar,
	   SSET_RULES, SSET_MILITARY, SSET_RARE, SSET_TO_CLIENT,
	   N_("Whether to enable fog of war"),
	   N_("If this is set to 1, only those units and cities within "
	      "the vision range of your own units and cities will be "
	      "revealed to you. You will not see new cities or terrain "
	      "changes in tiles not observed."), NULL, 
	   GAME_DEFAULT_FOGOFWAR)

  GEN_BOOL("foggedborders", game.server.foggedborders,
           SSET_RULES, SSET_MILITARY, SSET_RARE, SSET_TO_CLIENT,
           N_("Whether border changes are seen through fog of war"),
           N_("If this setting is enabled, players will not be able "
              "to see changes in tile ownership if they do not have "
              "direct sight of the affected tiles. Otherwise, players "
              "can see any or all changes to borders as long as they "
              "have previously seen the tiles."), NULL,
           GAME_DEFAULT_FOGGEDBORDERS)

  GEN_INT("diplchance", game.info.diplchance,
	  SSET_RULES_FLEXIBLE, SSET_MILITARY, SSET_SITUATIONAL, SSET_TO_CLIENT,
	  N_("Base chance for diplomats and spies to succeed."),
	  /* xgettext:no-c-format */
	  N_("The chance of a spy returning from a successful mission and "
	     "the base chance of success for diplomats and spies."),
          NULL,
	  GAME_MIN_DIPLCHANCE, GAME_MAX_DIPLCHANCE, GAME_DEFAULT_DIPLCHANCE)

  GEN_BOOL("spacerace", game.info.spacerace,
	   SSET_RULES_FLEXIBLE, SSET_SCIENCE, SSET_VITAL, SSET_TO_CLIENT,
	   N_("Whether to allow space race"),
	   N_("If this option is set to 1, players can build spaceships."),
	   NULL, 
	   GAME_DEFAULT_SPACERACE)

  GEN_BOOL("endspaceship", game.info.endspaceship, SSET_RULES_FLEXIBLE,
           SSET_SCIENCE, SSET_VITAL, SSET_TO_CLIENT,
           N_("Should the game end if the spaceship arrives?"),
           N_("If this option is set to 1, the game will end with the "
              "arrival of a spaceship at Alpha Centauri."), NULL,
           GAME_DEFAULT_END_SPACESHIP)

  GEN_INT("civilwarsize", game.info.civilwarsize,
	  SSET_RULES_FLEXIBLE, SSET_SOCIOLOGY, SSET_RARE, SSET_TO_CLIENT,
	  N_("Minimum number of cities for civil war"),
	  N_("A civil war is triggered when a player has at least this "
	     "many cities and the player's capital is captured. If "
	     "this option is set to the maximum value, civil wars are "
	     "turned off altogether."), NULL, 
	  GAME_MIN_CIVILWARSIZE, GAME_MAX_CIVILWARSIZE, 
	  GAME_DEFAULT_CIVILWARSIZE)

  GEN_INT("contactturns", game.info.contactturns,
	  SSET_RULES_FLEXIBLE, SSET_MILITARY, SSET_RARE, SSET_TO_CLIENT,
	  N_("Turns until player contact is lost"),
	  N_("Players may meet for diplomacy this number of turns "
	     "after their units have last met, even when they do not have "
	     "an embassy. If set to zero, then players cannot meet unless "
	     "they have an embassy."),
	  NULL,
	  GAME_MIN_CONTACTTURNS, GAME_MAX_CONTACTTURNS, 
	  GAME_DEFAULT_CONTACTTURNS)

  GEN_BOOL("savepalace", game.info.savepalace,
	   SSET_RULES_FLEXIBLE, SSET_MILITARY, SSET_RARE, SSET_TO_CLIENT,
	   N_("Rebuild palace whenever capital is conquered"),
	   N_("If this is set to 1, when the capital is conquered the "
	      "palace is automatically rebuilt for free in another randomly "
	      "choosen city. This is significant because the technology "
	      "requirement for building a palace will be ignored."),
	   NULL,
	   GAME_DEFAULT_SAVEPALACE)

  GEN_BOOL("naturalcitynames", game.info.natural_city_names,
           SSET_RULES_FLEXIBLE, SSET_SOCIOLOGY, SSET_RARE, SSET_TO_CLIENT,
           N_("Whether to use natural city names"),
           N_("If enabled, the default city names will be determined based "
              "on the surrounding terrain."),
           NULL, GAME_DEFAULT_NATURALCITYNAMES)

  GEN_BOOL("migration", game.info.migration,
           SSET_RULES_FLEXIBLE, SSET_SOCIOLOGY, SSET_RARE, SSET_TO_CLIENT,
           N_("Whether to enable citizen migration"),
           N_("This is the master setting that controls whether citizen "
              "migration is active in the game. If enabled, citizens may "
              "automatically move from less desirable cities to more "
              "desirable ones. The \"desirability\" of a given city is "
              "calculated from a number of factors. In general larger "
              "cities with more income and improvements will be preferred. "
              "Citizens will never migrate out of the capital, or cause "
              "a wonder to be lost by disbanding a city. A number of other "
              "settings control how migration behaves:\n"
              "  mgr_turninterval - How often citizens try to migrate.\n"
              "  mgr_foodneeded   - Whether destination food is checked.\n"
              "  mgr_distance     - How far citizens will migrate.\n"
              "  mgr_worldchance  - Chance for inter-nation migration.\n"
              "  mgr_nationchance - Chance for intra-nation migration."),
           NULL, GAME_DEFAULT_MIGRATION)

  GEN_INT("mgr_turninterval", game.info.mgr_turninterval,
          SSET_RULES_FLEXIBLE, SSET_SOCIOLOGY, SSET_RARE, SSET_TO_CLIENT,
          N_("Number of turns between migrations from a city"),
          N_("This setting controls the number of turns between migration "
             "checks for a given city. The interval is calculated from "
             "the founding turn of the city. So for example if this "
             "setting is 5, citizens will look for a suitable migration "
             "destination every five turns from the founding of their "
             "current city. Migration will never occur the same turn "
             "that a city is built. This setting has no effect unless "
             "migration is enabled by the 'migration' setting."), NULL,
          GAME_MIN_MGR_TURNINTERVAL, GAME_MAX_MGR_TURNINTERVAL,
          GAME_DEFAULT_MGR_TURNINTERVAL)

  GEN_BOOL("mgr_foodneeded", game.info.mgr_foodneeded,
          SSET_RULES_FLEXIBLE, SSET_SOCIOLOGY, SSET_RARE, SSET_TO_CLIENT,
           N_("Whether migration is limited by food"),
           N_("If this setting is enabled, citizens will not migrate to "
              "cities which would not have enough food to support them. "
              "This setting has no effect unless migration is enabled by "
              "the 'migration' setting."), NULL,
           GAME_DEFAULT_MGR_FOODNEEDED)

  GEN_INT("mgr_distance", game.info.mgr_distance,
          SSET_RULES_FLEXIBLE, SSET_SOCIOLOGY, SSET_RARE, SSET_TO_CLIENT,
          N_("Maximum distance citizens may migrate"),
          N_("This setting controls how far citizens may look for a "
             "suitable migration destination when deciding which city "
             "to migrate to. For example, with a value of 3, there can "
             "be at most a 2 tile distance between cities undergoing "
             "migration. This setting has no effect unless migration "
             "is activated by the 'migration' setting."), NULL,
          GAME_MIN_MGR_DISTANCE, GAME_MAX_MGR_DISTANCE,
          GAME_DEFAULT_MGR_DISTANCE)

  GEN_INT("mgr_nationchance", game.info.mgr_nationchance,
          SSET_RULES_FLEXIBLE, SSET_SOCIOLOGY, SSET_RARE, SSET_TO_CLIENT,
          N_("Percent probability for migration within the same nation"),
          N_("This setting controls how likely it is for citizens to "
             "migrate between cities owned by the same player. Zero "
             "indicates migration will never occur, 100 means that "
             "migration will always occur if the citizens find a suitable "
             "destination. This setting has no effect unless migration "
             "is activated by the 'migration' setting."), NULL,
          GAME_MIN_MGR_NATIONCHANCE, GAME_MAX_MGR_NATIONCHANCE,
          GAME_DEFAULT_MGR_NATIONCHANCE)

  GEN_INT("mgr_worldchance", game.info.mgr_worldchance,
          SSET_RULES_FLEXIBLE, SSET_SOCIOLOGY, SSET_RARE, SSET_TO_CLIENT,
          N_("Percent probability for migration between foreign cities"),
          N_("This setting controls how likely it is for migration "
             "to occur between cities owned by different players. "
             "Zero indicates migration will never occur, 100 means "
             "that citizens will always migrate if they find a suitable "
             "destination. This setting has no effect if migration is "
             "not enabled by the 'migration' setting."), NULL,
          GAME_MIN_MGR_WORLDCHANCE, GAME_MAX_MGR_WORLDCHANCE,
          GAME_DEFAULT_MGR_WORLDCHANCE)

  /* Meta options: these don't affect the internal rules of the game, but
   * do affect players.  Also options which only produce extra server
   * "output" and don't affect the actual game.
   * ("endturn" is here, and not RULES_FLEXIBLE, because it doesn't
   * affect what happens in the game, it just determines when the
   * players stop playing and look at the score.)
   */
  GEN_STRING("allowtake", game.server.allow_take,
	     SSET_META, SSET_NETWORK, SSET_RARE, SSET_TO_CLIENT,
             N_("Players that users are allowed to take"),
             N_("This should be a string of characters, each of which "
                "specifies a type or status of a civilization (player). "
                "Clients will only be permitted to take "
                "or observe those players which match one of the specified "
                "letters. This only affects future uses of the take or "
                "observe command; it is not retroactive. The characters "
		"and their meanings are:\n"
                "    o,O = Global observer\n"
                "    b   = Barbarian players\n"
                "    d   = Dead players\n"
                "    a,A = AI players\n"
                "    h,H = Human players\n"
                "The first description on this list which matches a "
                "player is the one which applies. Thus 'd' does not "
                "include dead barbarians, 'a' does not include dead AI "
                "players, and so on. Upper case letters apply before "
                "the game has started, lower case letters afterwards.\n\n"
                "Each character above may be followed by one of the "
                "following numbers to allow or restrict the manner "
                "of connection:\n\n"
                "(none) = Controller allowed, observers allowed, "
                "can displace connections. (Displacing a connection means "
		"that you may take over a player, even when another user "
		"already controls that player.)\n\n"
                "1 = Controller allowed, observers allowed, "
                "can't displace connections;\n\n"
                "2 = Controller allowed, no observers allowed, "
                "can displace connections;\n\n"
                "3 = Controller allowed, no observers allowed, "
                "can't displace connections;\n\n"
                "4 = No controller allowed, observers allowed;\n\n"),
                allowtake_callback, GAME_DEFAULT_ALLOW_TAKE)

  GEN_BOOL("autotoggle", game.info.auto_ai_toggle,
	   SSET_META, SSET_NETWORK, SSET_SITUATIONAL, SSET_TO_CLIENT,
	   N_("Whether AI-status toggles with connection"),
	   N_("If this is set to 1, AI status is turned off when a player "
	      "connects, and on when a player disconnects."),
	   NULL, GAME_DEFAULT_AUTO_AI_TOGGLE)

  GEN_INT("endturn", game.info.end_turn,
	  SSET_META, SSET_SOCIOLOGY, SSET_VITAL, SSET_TO_CLIENT,
	  N_("Turn the game ends"),
          N_("The game will end at the end of the given turn."),
          endturn_callback,
          GAME_MIN_END_TURN, GAME_MAX_END_TURN, GAME_DEFAULT_END_TURN)

  GEN_INT("timeout", game.info.timeout,
	  SSET_META, SSET_INTERNAL, SSET_VITAL, SSET_TO_CLIENT,
	  N_("Maximum seconds per turn"),
	  N_("If all players have not hit \"Turn Done\" before this "
	     "time is up, then the turn ends automatically. Zero "
	     "means there is no timeout. In servers compiled with "
             "debugging, a timeout of -1 sets the autogame test mode. "
             "Only connections with hack level access may set the "
             "timeout to lower than 30 seconds. Use this with the "
             "command \"timeoutincrease\" to have a dynamic timer."),
          timeout_callback,
          GAME_MIN_TIMEOUT, GAME_MAX_TIMEOUT, GAME_DEFAULT_TIMEOUT)

  GEN_INT("timeaddenemymove", game.server.timeoutaddenemymove,
	  SSET_META, SSET_INTERNAL, SSET_VITAL, SSET_TO_CLIENT,
	  N_("Timeout at least n seconds when enemy moved"),
	  N_("Any time a unit moves while in sight of an enemy player, "
	     "the remaining timeout is increased to this value."),
	  NULL, 0, GAME_MAX_TIMEOUT, GAME_DEFAULT_TIMEOUTADDEMOVE)
  
  /* This setting points to the "stored" value; changing it won't have
   * an effect until the next synchronization point (i.e., the start of
   * the next turn). */
  GEN_INT("phasemode", game.server.phase_mode_stored,
	  SSET_META, SSET_INTERNAL, SSET_SITUATIONAL, SSET_TO_CLIENT,
	  N_("Whether to have simultaneous player/team phases."),
          /* NB: The values must match enum phase_mode_types
           * defined in common/game.h */
	  N_("This setting controls whether players may make "
             "moves at the same time during a turn.\n"
             "  0 = All players move concurrently.\n"
             "  1 = All players alternate movement.\n"
             "  2 = Only players on the same team move concurrently."),
          phasemode_callback, GAME_MIN_PHASE_MODE,
          GAME_MAX_PHASE_MODE, GAME_DEFAULT_PHASE_MODE)

  GEN_INT("nettimeout", game.info.tcptimeout,
	  SSET_META, SSET_NETWORK, SSET_RARE, SSET_TO_CLIENT,
	  N_("Seconds to let a client's network connection block"),
	  N_("If a network connection is blocking for a time greater than "
	     "this value, then the connection is closed. Zero "
	     "means there is no timeout (although connections will be "
	     "automatically disconnected eventually)."),
	  NULL,
	  GAME_MIN_TCPTIMEOUT, GAME_MAX_TCPTIMEOUT, GAME_DEFAULT_TCPTIMEOUT)

  GEN_INT("netwait", game.info.netwait,
	  SSET_META, SSET_NETWORK, SSET_RARE, SSET_TO_CLIENT,
	  N_("Max seconds for network buffers to drain"),
	  N_("The server will wait for up to the value of this "
	     "parameter in seconds, for all client connection network "
	     "buffers to unblock. Zero means the server will not "
	     "wait at all."), NULL, 
	  GAME_MIN_NETWAIT, GAME_MAX_NETWAIT, GAME_DEFAULT_NETWAIT)

  GEN_INT("pingtime", game.info.pingtime,
	  SSET_META, SSET_NETWORK, SSET_RARE, SSET_TO_CLIENT,
	  N_("Seconds between PINGs"),
	  N_("The civserver will poll the clients with a PING request "
	     "each time this period elapses."), NULL, 
	  GAME_MIN_PINGTIME, GAME_MAX_PINGTIME, GAME_DEFAULT_PINGTIME)

  GEN_INT("pingtimeout", game.info.pingtimeout,
	  SSET_META, SSET_NETWORK, SSET_RARE,
          SSET_TO_CLIENT,
	  N_("Time to cut a client"),
	  N_("If a client doesn't reply to a PING in this time the "
	     "client is disconnected."), NULL, 
	  GAME_MIN_PINGTIMEOUT, GAME_MAX_PINGTIMEOUT, GAME_DEFAULT_PINGTIMEOUT)

  GEN_BOOL("turnblock", game.info.turnblock,
	   SSET_META, SSET_INTERNAL, SSET_SITUATIONAL, SSET_TO_CLIENT,
	   N_("Turn-blocking game play mode"),
	   N_("If this is set to 1 the game turn is not advanced "
	      "until all players have finished their turn, including "
	      "disconnected players."), NULL, 
	   GAME_DEFAULT_TURNBLOCK)

  GEN_BOOL("fixedlength", game.info.fixedlength,
	   SSET_META, SSET_INTERNAL, SSET_SITUATIONAL, SSET_TO_CLIENT,
	   N_("Fixed-length turns play mode"),
	   N_("If this is set to 1 the game turn will not advance "
	      "until the timeout has expired, even after all players "
	      "have clicked on \"Turn Done\"."), NULL,
	   FALSE)

  GEN_STRING("demography", game.server.demography,
	     SSET_META, SSET_INTERNAL, SSET_SITUATIONAL, SSET_TO_CLIENT,
	     N_("What is in the Demographics report"),
	     N_("This should be a string of characters, each of which "
		"specifies the inclusion of a line of information "
		"in the Demographics report.\n"
		"The characters and their meanings are:\n"
		"    N = include Population\n"
		"    P = include Production\n"
		"    A = include Land Area\n"
		"    L = include Literacy\n"
		"    R = include Research Speed\n"
		"    S = include Settled Area\n"
		"    E = include Economics\n"
		"    M = include Military Service\n"
		"    O = include Pollution\n"
		"Additionally, the following characters control whether "
		"or not certain columns are displayed in the report:\n"
		"    q = display \"quantity\" column\n"
		"    r = display \"rank\" column\n"
		"    b = display \"best nation\" column\n"
		"The order of characters is not significant, but "
		"their capitalization is."),
	     is_valid_demography, GAME_DEFAULT_DEMOGRAPHY)

  GEN_INT("saveturns", game.info.save_nturns,
	  SSET_META, SSET_INTERNAL, SSET_VITAL, SSET_SERVER_ONLY,
	  N_("Turns per auto-save"),
	  N_("The game will be automatically saved per this number of "
	     "turns. Zero means never auto-save."), NULL, 
          GAME_MIN_SAVETURNS, GAME_MAX_SAVETURNS, GAME_DEFAULT_SAVETURNS)

  GEN_INT("compress", game.info.save_compress_level,
	  SSET_META, SSET_INTERNAL, SSET_RARE, SSET_SERVER_ONLY,
	  N_("Savegame compression level"),
	  N_("If non-zero, saved games will be compressed using zlib "
	     "(gzip format) or bzip2. Larger values will give better "
	     "compression but take longer."), NULL,
	  GAME_MIN_COMPRESS_LEVEL, GAME_MAX_COMPRESS_LEVEL,
	  GAME_DEFAULT_COMPRESS_LEVEL)

  GEN_INT("compresstype", game.info.save_compress_type,
          SSET_META, SSET_INTERNAL, SSET_RARE, SSET_SERVER_ONLY,
          N_("Savegame compression algorithm"),
          N_("Compression library to use for savegames.\n"
             " 0 - none\n"
             " 1 - zlib (gzip format)\n"
             " 2 - bzip2\n"
             "Not all servers support all compression methods."), NULL,
	  GAME_MIN_COMPRESS_TYPE, GAME_MAX_COMPRESS_TYPE,
	  GAME_DEFAULT_COMPRESS_TYPE)

  GEN_STRING("savename", game.server.save_name,
	     SSET_META, SSET_INTERNAL, SSET_VITAL, SSET_SERVER_ONLY,
	     N_("Auto-save name prefix"),
	     N_("Automatically saved games will have name "
		"\"<prefix>-T<turn>-Y<year>.sav\". This setting sets "
		"the <prefix> part."), NULL,
	     GAME_DEFAULT_SAVE_NAME)

  GEN_BOOL("scorelog", game.server.scorelog,
	   SSET_META, SSET_INTERNAL, SSET_SITUATIONAL, SSET_SERVER_ONLY,
	   N_("Whether to log player statistics"),
	   N_("If this is set to 1, player statistics are appended to "
	      "the file \"civscore.log\" every turn. These statistics "
	      "can be used to create power graphs after the game."), NULL,
	   GAME_DEFAULT_SCORELOG)

  GEN_END
};

#undef GEN_BOOL
#undef GEN_INT
#undef GEN_STRING
#undef GEN_END

/* The number of settings, not including the END. */
const int SETTINGS_NUM = ARRAY_SIZE(settings) - 1;

/****************************************************************************
  Returns whether the specified server setting (option) can currently
  be changed.  Does not indicate whether it can be changed by clients.
****************************************************************************/
bool setting_is_changeable(int setting_id)
{
  return setting_class_is_changeable(settings[setting_id].sclass);
}
/********************************************************************
  Update the setting to the default value
*********************************************************************/
static void setting_set_to_default(int idx)
{
  struct settings_s *pset = &settings[idx];

  switch (pset->stype) {
    case SSET_BOOL:
      (*pset->bool_value) = pset->bool_default_value;
      break;
    case SSET_INT:
      (*pset->int_value) = pset->int_default_value;
      break;
    case SSET_STRING:
      mystrlcpy(pset->string_value, pset->string_default_value,
                pset->string_value_size);
      break;
  }

  /* FIXME: duplicates stdinhand.c:set_command() */
  if (pset->int_value == &game.info.aifill) {
    aifill(*pset->int_value);
  } else if (pset->bool_value == &game.info.auto_ai_toggle) {
    if (*pset->bool_value) {
      players_iterate(pplayer) {
        if (!pplayer->ai_data.control && !pplayer->is_connected) {
           toggle_ai_player_direct(NULL, pplayer);
          send_player_info_c(pplayer, game.est_connections);
        }
      } players_iterate_end;
    }
  }
}

/**************************************************************************
  Initialize stuff related to this code module.
**************************************************************************/
void settings_init(void)
{
  int i;

  for (i = 0; i < SETTINGS_NUM; i++) {
    setting_set_to_default(i);
  }
}

/********************************************************************
  Reset all settings iff they are changeable.
*********************************************************************/
void settings_reset(void)
{
  int i;

  for (i = 0; i < SETTINGS_NUM; i++) {
    if (setting_is_changeable(i)) {
      setting_set_to_default(i);
    }
  }
}

/**************************************************************************
  Update stuff every turn that is related to this code module. Run this
  on turn end.
**************************************************************************/
void settings_turn(void)
{
  /* Nothing at the moment. */
}

/**************************************************************************
  Deinitialize stuff related to this code module.
**************************************************************************/
void settings_free(void)
{
  /* Nothing at the moment. */
}
