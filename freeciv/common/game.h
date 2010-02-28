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
#ifndef FC__GAME_H
#define FC__GAME_H

#include <time.h>	/* time_t */

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include "shared.h"
#include "timing.h"

#include "connection.h"		/* struct conn_list */
#include "fc_types.h"
#include "player.h"
#include "packets.h"

enum debug_globals {
  DEBUG_FERRIES,
  DEBUG_LAST
};

/* NB: Must match phasemode setting
 * help text in server/settings.c */
enum phase_mode_types {
  PMT_CONCURRENT = 0,
  PMT_PLAYERS_ALTERNATE = 1,
  PMT_TEAMS_ALTERNATE = 2
};

#define CONTAMINATION_POLLUTION 1
#define CONTAMINATION_FALLOUT   2

/* The number of turns that the first user needs to be attached to a 
 * player for that user to be ranked as that player */
#define TURNS_NEEDED_TO_RANK 10

struct civ_game {
  struct packet_game_info info;
  struct government *government_during_revolution;

  struct packet_ruleset_control control;
  struct packet_scenario_info scenario;

  struct player players[MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS];
  int nplayers;
  struct conn_list *all_connections;        /* including not yet established */
  struct conn_list *est_connections;        /* all established client conns */

  int work_veteran_chance[MAX_VET_LEVELS];
  int veteran_chance[MAX_VET_LEVELS];

  union {
    struct {
      /* Nothing yet. */
    } client;

    struct {
      bool debug[DEBUG_LAST];
      int timeoutint;     /* increase timeout every N turns... */
      int timeoutinc;     /* ... by this amount ... */
      int timeoutincmult; /* ... and multiply timeoutinc by this amount ... */
      int timeoutintinc;  /* ... and increase timeoutint by this amount */
      int timeoutcounter; /* timeoutcounter - timeoutint = turns to next inc. */
      int timeoutaddenemymove; /* minimum timeout after an enemy move is seen */
      time_t last_ping;
      struct timer *phase_timer; /* Time since seconds_to_phase_done was set. */
      /* The .info.phase_mode value indicates the phase mode currently in
       * use.  The "stored" value is a value the player can change; it won't
       * take effect until the next turn. */
      int phase_mode_stored;
      char connectmsg[MAX_LEN_MSG];
      char save_name[MAX_LEN_NAME];
      bool scorelog;
      int scoreturn;    /* next make_history_report() */
      int seed;
      bool fogofwar_old; /* as the fog_of_war bit get changed by setting
                          * the server we need to remember the old setting */
      bool foggedborders;
      char rulesetdir[MAX_LEN_NAME];
      char demography[MAX_LEN_DEMOGRAPHY];
      char allow_take[MAX_LEN_ALLOW_TAKE];

      /* values from game.info.t */
      struct {
        /* Items given to all players at game start.  Server only. */
        int global_init_techs[MAX_NUM_TECH_LIST];
        int global_init_buildings[MAX_NUM_BUILDING_LIST];
      } rgame;

      /* used by the map editor to control game_save. */
      struct {
        bool save_random;
  
        bool save_known; /* loading will just reveal the squares around
                          * cities and units */
        bool save_starts; /* start positions will be auto generated */
        bool save_private_map; /* FoW map; will be created if not saved */
      } save_options;

      struct {
        bool user_message_set;
        char user_message[256];
      } meta_info;
    } server;
  };

  struct {
    /* Function to be called in game_remove_unit when a unit is deleted. */
    void (*unit_deallocate)(int unit_id);
  } callbacks;
};

bool is_server(void);
void i_am_server(void);
void i_am_client(void);

void game_init(void);
void game_map_init(void);
void game_free(void);
void game_reset(void);

void game_ruleset_init(void);
void game_ruleset_free(void);

int game_next_year(int);
void game_advance_year(void);

int civ_population(const struct player *pplayer);
struct city *game_find_city_by_name(const char *name);
struct city *game_find_city_by_number(int id);

struct unit *game_find_unit_by_number(int id);

void game_remove_player(struct player *pplayer);

void game_remove_unit(struct unit *punit);
void game_remove_city(struct city *pcity);
void initialize_globals(void);

bool is_player_phase(const struct player *pplayer, int phase);

const char *population_to_text(int thousand_citizen);

const char *gui_name(enum gui_type);

const char *textyear(int year);

extern struct civ_game game;

bool setting_class_is_changeable(enum sset_class class);

#define GAME_DEFAULT_SEED        0
#define GAME_MIN_SEED            0
#define GAME_MAX_SEED            (MAX_UINT32 >> 1)

#define GAME_DEFAULT_GOLD        50
#define GAME_MIN_GOLD            0
#define GAME_MAX_GOLD            5000

#define GAME_DEFAULT_START_UNITS  "ccwwx"

#define GAME_DEFAULT_DISPERSION  0
#define GAME_MIN_DISPERSION      0
#define GAME_MAX_DISPERSION      10

#define GAME_DEFAULT_TECHLEVEL   0
#define GAME_MIN_TECHLEVEL       0
#define GAME_MAX_TECHLEVEL       50

#define GAME_DEFAULT_ANGRYCITIZEN TRUE

#define GAME_DEFAULT_END_TURN    5000
#define GAME_MIN_END_TURN        -32768
#define GAME_MAX_END_TURN        32767

#define GAME_DEFAULT_MIN_PLAYERS     1
#define GAME_MIN_MIN_PLAYERS         0
#define GAME_MAX_MIN_PLAYERS         MAX_NUM_PLAYERS

#define GAME_DEFAULT_MAX_PLAYERS     MAX_NUM_PLAYERS
#define GAME_MIN_MAX_PLAYERS         1
#define GAME_MAX_MAX_PLAYERS         MAX_NUM_PLAYERS

#define GAME_DEFAULT_AIFILL          5
#define GAME_MIN_AIFILL              0
#define GAME_MAX_AIFILL              GAME_MAX_MAX_PLAYERS

#define GAME_DEFAULT_FOODBOX         100
#define GAME_MIN_FOODBOX             1
#define GAME_MAX_FOODBOX             10000

#define GAME_DEFAULT_SHIELDBOX 100
#define GAME_MIN_SHIELDBOX 1
#define GAME_MAX_SHIELDBOX 10000

#define GAME_DEFAULT_SCIENCEBOX 100
#define GAME_MIN_SCIENCEBOX 1
#define GAME_MAX_SCIENCEBOX 10000

#define GAME_DEFAULT_DIPLCOST        0
#define GAME_MIN_DIPLCOST            0
#define GAME_MAX_DIPLCOST            100

#define GAME_DEFAULT_FOGOFWAR        TRUE

#define GAME_DEFAULT_FOGGEDBORDERS   FALSE

/* 0 means no national borders. */
#define GAME_DEFAULT_BORDERS         1
#define GAME_MIN_BORDERS             0
#define GAME_MAX_BORDERS             3

#define GAME_DEFAULT_HAPPYBORDERS    TRUE

#define GAME_DEFAULT_DIPLOMACY       0
#define GAME_MIN_DIPLOMACY           0
#define GAME_MAX_DIPLOMACY           4

#define GAME_DEFAULT_DIPLCHANCE      80
#define GAME_MIN_DIPLCHANCE          40
#define GAME_MAX_DIPLCHANCE          100

#define GAME_DEFAULT_FREECOST        0
#define GAME_MIN_FREECOST            0
#define GAME_MAX_FREECOST            100

#define GAME_DEFAULT_CONQUERCOST     0
#define GAME_MIN_CONQUERCOST         0
#define GAME_MAX_CONQUERCOST         100

#define GAME_DEFAULT_CITYMINDIST     0
#define GAME_MIN_CITYMINDIST         0 /* if 0, ruleset will overwrite this */
#define GAME_MAX_CITYMINDIST         5

#define GAME_DEFAULT_CIVILWARSIZE    10
#define GAME_MIN_CIVILWARSIZE        6
#define GAME_MAX_CIVILWARSIZE        1000

#define GAME_DEFAULT_CONTACTTURNS    20
#define GAME_MIN_CONTACTTURNS        0
#define GAME_MAX_CONTACTTURNS        100

#define GAME_DEFAULT_CELEBRATESIZE    3

#define GAME_DEFAULT_RAPTUREDELAY    1
#define GAME_MIN_RAPTUREDELAY        1
#define GAME_MAX_RAPTUREDELAY        99 /* 99 practicaly disables rapturing */
 
#define GAME_DEFAULT_SAVEPALACE      TRUE

#define GAME_DEFAULT_NATURALCITYNAMES TRUE

#define GAME_DEFAULT_MIGRATION        FALSE

#define GAME_DEFAULT_MGR_TURNINTERVAL 5
#define GAME_MIN_MGR_TURNINTERVAL     1
#define GAME_MAX_MGR_TURNINTERVAL     100

#define GAME_DEFAULT_MGR_FOODNEEDED   TRUE

#define GAME_DEFAULT_MGR_DISTANCE     3
#define GAME_MIN_MGR_DISTANCE         1
#define GAME_MAX_MGR_DISTANCE         7

#define GAME_DEFAULT_MGR_NATIONCHANCE 50
#define GAME_MIN_MGR_NATIONCHANCE     0
#define GAME_MAX_MGR_NATIONCHANCE     100

#define GAME_DEFAULT_MGR_WORLDCHANCE  10
#define GAME_MIN_MGR_WORLDCHANCE      0
#define GAME_MAX_MGR_WORLDCHANCE      100

#define GAME_DEFAULT_AQUEDUCTLOSS    0
#define GAME_MIN_AQUEDUCTLOSS        0
#define GAME_MAX_AQUEDUCTLOSS        100

#define GAME_DEFAULT_KILLCITIZEN     1
#define GAME_MIN_KILLCITIZEN         0
#define GAME_MAX_KILLCITIZEN         15

#define GAME_DEFAULT_TECHPENALTY     100
#define GAME_MIN_TECHPENALTY         0
#define GAME_MAX_TECHPENALTY         100

#define GAME_DEFAULT_RAZECHANCE      20
#define GAME_MIN_RAZECHANCE          0
#define GAME_MAX_RAZECHANCE          100

#define GAME_DEFAULT_SCORELOG        FALSE
#define GAME_DEFAULT_SCORETURN       20

#define GAME_DEFAULT_SPACERACE       TRUE
#define GAME_DEFAULT_END_SPACESHIP   TRUE

#define GAME_DEFAULT_TURNBLOCK       TRUE

#define GAME_DEFAULT_AUTO_AI_TOGGLE  FALSE

#define GAME_DEFAULT_TIMEOUT         0
#define GAME_DEFAULT_TIMEOUTINT      0
#define GAME_DEFAULT_TIMEOUTINTINC   0
#define GAME_DEFAULT_TIMEOUTINC      0
#define GAME_DEFAULT_TIMEOUTINCMULT  1
#define GAME_DEFAULT_TIMEOUTADDEMOVE 0

#define GAME_MIN_TIMEOUT             -1
#define GAME_MAX_TIMEOUT             8639999

#define GAME_DEFAULT_PHASE_MODE 0
#define GAME_MIN_PHASE_MODE 0
#define GAME_MAX_PHASE_MODE 2

#define GAME_DEFAULT_TCPTIMEOUT      10
#define GAME_MIN_TCPTIMEOUT          0
#define GAME_MAX_TCPTIMEOUT          120

#define GAME_DEFAULT_NETWAIT         4
#define GAME_MIN_NETWAIT             0
#define GAME_MAX_NETWAIT             20

#define GAME_DEFAULT_PINGTIME        20
#define GAME_MIN_PINGTIME            1
#define GAME_MAX_PINGTIME            1800

#define GAME_DEFAULT_PINGTIMEOUT     60
#define GAME_MIN_PINGTIMEOUT         60
#define GAME_MAX_PINGTIMEOUT         1800

#define GAME_DEFAULT_NOTRADESIZE     0
#define GAME_MIN_NOTRADESIZE         0
#define GAME_MAX_NOTRADESIZE         49

#define GAME_DEFAULT_FULLTRADESIZE   1
#define GAME_MIN_FULLTRADESIZE       1
#define GAME_MAX_FULLTRADESIZE       50

#define GAME_DEFAULT_TRADEMINDIST    9
#define GAME_MIN_TRADEMINDIST        1
#define GAME_MAX_TRADEMINDIST        999

#define GAME_DEFAULT_BARBARIANRATE   2
#define GAME_MIN_BARBARIANRATE       0
#define GAME_MAX_BARBARIANRATE       4

#define GAME_DEFAULT_ONSETBARBARIAN  60
#define GAME_MIN_ONSETBARBARIAN      0
#define GAME_MAX_ONSETBARBARIAN      GAME_MAX_END_TURN

#define GAME_DEFAULT_OCCUPYCHANCE    0
#define GAME_MIN_OCCUPYCHANCE        0
#define GAME_MAX_OCCUPYCHANCE        100

#define GAME_DEFAULT_AUTOATTACK      FALSE

#define GAME_DEFAULT_RULESETDIR      "default"

#define GAME_DEFAULT_SAVE_NAME       "civgame"
#define GAME_DEFAULT_SAVETURNS       1
#define GAME_MIN_SAVETURNS           0
#define GAME_MAX_SAVETURNS           200

#define GAME_DEFAULT_SKILL_LEVEL 3      /* easy */
#define GAME_OLD_DEFAULT_SKILL_LEVEL 5  /* normal; for old save games */

#define GAME_DEFAULT_DEMOGRAPHY      "NASRLPEMOqrb"
#define GAME_DEFAULT_ALLOW_TAKE      "HAhadOo"

#define GAME_DEFAULT_COMPRESS_LEVEL 6    /* if we have compression */
#define GAME_MIN_COMPRESS_LEVEL     1
#define GAME_MAX_COMPRESS_LEVEL     9

#if defined(HAVE_LIBBZ2)
#  define GAME_MIN_COMPRESS_TYPE FZ_PLAIN
#  define GAME_MAX_COMPRESS_TYPE FZ_BZIP2
#  define GAME_DEFAULT_COMPRESS_TYPE FZ_BZIP2
#elif defined(HAVE_LIBZ)
#  define GAME_MIN_COMPRESS_TYPE FZ_PLAIN
#  define GAME_MAX_COMPRESS_TYPE FZ_ZLIB
#  define GAME_DEFAULT_COMPRESS_TYPE FZ_ZLIB
#else
#  define GAME_MIN_COMPRESS_TYPE FZ_PLAIN
#  define GAME_MAX_COMPRESS_TYPE FZ_PLAIN
#  define GAME_DEFAULT_COMPRESS_TYPE FZ_PLAIN
#endif

#define GAME_DEFAULT_ALLOWED_CITY_NAMES 1
#define GAME_MIN_ALLOWED_CITY_NAMES 0
#define GAME_MAX_ALLOWED_CITY_NAMES 3

#define GAME_DEFAULT_REVOLUTION_LENGTH 0
#define GAME_MIN_REVOLUTION_LENGTH 0
#define GAME_MAX_REVOLUTION_LENGTH 10

#define GAME_START_YEAR -4000

#define GAME_MAX_READ_RECURSION 10 /* max recursion for the read command */

/* ruleset settings */

#define RS_MAX_VALUE                             10000

#define RS_DEFAULT_POS_YEAR_LABEL                "AD"
#define RS_DEFAULT_NEG_YEAR_LABEL                "BC"

#define RS_DEFAULT_ILLNESS_ON                    FALSE

#define RS_DEFAULT_ILLNESS_BASE_FACTOR           25
#define RS_MIN_ILLNESS_BASE_FACTOR               0
#define RS_MAX_ILLNESS_BASE_FACTOR               RS_MAX_VALUE

#define RS_DEFAULT_ILLNESS_MIN_SIZE              3
#define RS_MIN_ILLNESS_MIN_SIZE                  1
#define RS_MAX_ILLNESS_MIN_SIZE                  100

#define RS_DEFAULT_ILLNESS_TRADE_INFECTION_PCT   50
#define RS_MIN_ILLNESS_TRADE_INFECTION_PCT       0
#define RS_MAX_ILLNESS_TRADE_INFECTION_PCT       500

#define RS_DEFAULT_ILLNESS_POLLUTION_PCT         50
#define RS_MIN_ILLNESS_POLLUTION_PCT             0
#define RS_MAX_ILLNESS_POLLUTION_PCT             500

#define RS_DEFAULT_CALENDAR_SKIP_0               TRUE

#define RS_DEFAULT_BORDER_RADIUS_SQ_CITY         17 /* city radius 4 */
#define RS_MIN_BORDER_RADIUS_SQ_CITY             0
#define RS_MAX_BORDER_RADIUS_SQ_CITY             401 /* city radius 20 */

#define RS_DEFAULT_BORDER_SIZE_EFFECT            1
#define RS_MIN_BORDER_SIZE_EFFECT                0
#define RS_MAX_BORDER_SIZE_EFFECT                100

#define RS_DEFAULT_INCITE_BASE_COST              1000
#define RS_MIN_INCITE_BASE_COST                  0
#define RS_MAX_INCITE_BASE_COST                  RS_MAX_VALUE

#define RS_DEFAULT_INCITE_IMPROVEMENT_FCT        1
#define RS_MIN_INCITE_IMPROVEMENT_FCT            0
#define RS_MAX_INCITE_IMPROVEMENT_FCT            RS_MAX_VALUE

#define RS_DEFAULT_INCITE_UNIT_FCT               2
#define RS_MIN_INCITE_UNIT_FCT                   0
#define RS_MAX_INCITE_UNIT_FCT                   RS_MAX_VALUE

#define RS_DEFAULT_INCITE_TOTAL_FCT              100
#define RS_MIN_INCITE_TOTAL_FCT                  0
#define RS_MAX_INCITE_TOTAL_FCT                  RS_MAX_VALUE

#define RS_DEFAULT_GRANARY_FOOD_INI              20

#define RS_DEFAULT_GRANARY_FOOD_INC              10
#define RS_MIN_GRANARY_FOOD_INC                  0
#define RS_MAX_GRANARY_FOOD_INC                  RS_MAX_VALUE

#define RS_DEFAULT_CITY_CENTER_OUTPUT            0
#define RS_MIN_CITY_CENTER_OUTPUT                0
#define RS_MAX_CITY_CENTER_OUTPUT                RS_MAX_VALUE

#define RS_DEFAULT_CITIES_MIN_DIST               2
#define RS_MIN_CITIES_MIN_DIST                   1
#define RS_MAX_CITIES_MIN_DIST                   RS_MAX_VALUE

#define RS_DEFAULT_VIS_RADIUS_SQ                 5 /* city radius 2 */
#define RS_MIN_VIS_RADIUS_SQ                     0
#define RS_MAX_VIS_RADIUS_SQ                     401 /* city radius 20 */

#define RS_DEFAULT_BASE_POLLUTION                -20
/* no min / max values as it can be set to high negative values to
 * deactiveate pollution and high positive values to create much
 * pollution */

#define RS_DEFAULT_HAPPY_COST                    2
#define RS_MIN_HAPPY_COST                        0
#define RS_MAX_HAPPY_COST                        100

#define RS_DEFAULT_FOOD_COST                     2
#define RS_MIN_FOOD_COST                         0
#define RS_MAX_FOOD_COST                         100

#define RS_DEFAULT_GOLD_UPKEEP_STYLE             0
#define RS_MIN_GOLD_UPKEEP_STYLE                 0
#define RS_MAX_GOLD_UPKEEP_STYLE                 2

#define RS_DEFAULT_SLOW_INVASIONS                TRUE

#define RS_DEFAULT_TIRED_ATTACK                  FALSE

#define RS_DEFAULT_KILLSTACK                     TRUE

#define RS_DEFAULT_BASE_BRIBE_COST               750
#define RS_MIN_BASE_BRIBE_COST                   0
#define RS_MAX_BASE_BRIBE_COST                   RS_MAX_VALUE

#define RS_DEFAULT_RANSOM_GOLD                   100
#define RS_MIN_RANSOM_GOLD                       0
#define RS_MAX_RANSOM_GOLD                       RS_MAX_VALUE

#define RS_DEFAULT_PILLAGE_SELECT                TRUE

#define RS_DEFAULT_UPGRADE_VETERAN_LOSS          0
#define RS_MIN_UPGRADE_VETERAN_LOSS              0
#define RS_MAX_UPGRADE_VETERAN_LOSS              MAX_VET_LEVELS

#define RS_DEFAULT_BASE_TECH_COST                20
#define RS_MIN_BASE_TECH_COST                    0
#define RS_MAX_BASE_TECH_COST                    200

#define RS_DEFAULT_TECH_COST_STYLE               0
#define RS_MIN_TECH_COST_STYLE                   0
#define RS_MAX_TECH_COST_STYLE                   2

#define RS_DEFAULT_TECH_LEAKAGE                  0
#define RS_MIN_TECH_LEAKAGE                      0
#define RS_MAX_TECH_LEAKAGE                      3

#endif  /* FC__GAME_H */
