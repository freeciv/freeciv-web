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
#ifndef FC__GAME_H
#define FC__GAME_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <time.h>	/* time_t */

#ifdef FREECIV_HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

/* utility */
#include "fcthread.h"
#include "shared.h"
#include "timing.h"

/* common */
#include "connection.h"		/* struct conn_list */
#include "fc_types.h"
#include "player.h"
#include "packets.h"
#include "world_object.h"

enum debug_globals {
  DEBUG_FERRIES,
  DEBUG_LAST
};

enum city_names_mode {
  CNM_NO_RESTRICTIONS = 0,
  CNM_PLAYER_UNIQUE,
  CNM_GLOBAL_UNIQUE,
  CNM_NO_STEALING
};

enum barbarians_rate {
  BARBS_DISABLED = 0,
  BARBS_HUTS_ONLY,
  BARBS_NORMAL,
  BARBS_FREQUENT,
  BARBS_HORDES
};

enum autosave_type {
  AS_TURN = 0,
  AS_GAME_OVER,
  AS_QUITIDLE,
  AS_INTERRUPT,
  AS_TIMER
};

enum scorelog_level {
  SL_ALL = 0,
  SL_HUMANS
};

struct user_flag
{
  char *name;
  char *helptxt;
};

/* The number of turns that the first user needs to be attached to a 
 * player for that user to be ranked as that player */
#define TURNS_NEEDED_TO_RANK 10

struct civ_game {
  struct packet_ruleset_control control;
  char *ruleset_summary;
  char *ruleset_description;
  char *ruleset_capabilities;
  struct packet_scenario_info scenario;
  struct packet_scenario_description scenario_desc;
  struct packet_game_info info;
  struct packet_calendar_info calendar;
  struct packet_timeout_info tinfo;

  struct government *default_government; /* may be overridden by nation */
  struct government *government_during_revolution;

  struct conn_list *all_connections;   /* including not yet established */
  struct conn_list *est_connections;   /* all established client conns */
  struct conn_list *glob_observers;    /* global observers */

  struct veteran_system *veteran; /* veteran system */

  struct rgbcolor *plr_bg_color;

  struct {
    /* Items given to all players at game start.
     * Client gets this info for help purposes only. */
    int global_init_techs[MAX_NUM_TECH_LIST];
    int global_init_buildings[MAX_NUM_BUILDING_LIST];
  } rgame;

  union {
    struct {
      /* Only used at the client (./client/). */

      bool ruleset_init;
      bool ruleset_ready;
    } client;

    struct {
      /* Only used in the server (./ai/ and ./server/). */

      /* Defined in the ruleset. */

      /* Game settings & other data. */

      enum city_names_mode allowed_city_names;
      enum plrcolor_mode plrcolormode;
      int aqueductloss;
      bool auto_ai_toggle;
      bool autoattack;
      int autoupgrade_veteran_loss;
      enum barbarians_rate barbarianrate;
      int base_incite_cost;
      int civilwarsize;
      int conquercost;
      int contactturns;
      int diplchance;
      int diplbulbcost;
      int diplgoldcost;
      int dispersion;
      int end_turn;
      bool endspaceship;
      bool fixedlength;
      bool foggedborders;
      int freecost;
      int incite_improvement_factor;
      int incite_total_factor;
      int incite_unit_factor;
      int init_vis_radius_sq;
      int kick_time;
      int killunhomed;    /* slowly killing unhomed units */
      int maxconnectionsperhost;
      int max_players;
      char nationset[MAX_LEN_NAME];
      int mgr_distance;
      bool mgr_foodneeded;
      int mgr_nationchance;
      int mgr_turninterval;
      int mgr_worldchance;
      bool multiresearch;
      bool migration;
      enum trait_dist_mode trait_dist;
      int min_players;
      bool natural_city_names;
      int netwait;
      int num_phases;
      int occupychance;
      int onsetbarbarian;
      int pingtime;
      int pingtimeout;
      int ransom_gold;
      int razechance;
      unsigned revealmap;
      int revolution_length;
      bool threaded_save;
      int save_compress_level;
      enum fz_method save_compress_type;
      int save_nturns;
      int save_frequency;
      unsigned autosaves; /* FIXME: char would be enough, but current settings.c code wants to
                             write sizeof(unsigned) bytes */
      bool savepalace;
      bool homecaughtunits;
      char start_units[MAX_LEN_STARTUNIT];
      bool start_city;
      int start_year;
      int techloss_restore;
      int techlost_donor;
      int techlost_recv;
      int tcptimeout;
      int techpenalty;
      bool turnblock;
      int unitwaittime;   /* minimal time between two movements of a unit */
      int upgrade_veteran_loss;
      bool vision_reveal_tiles;

      bool debug[DEBUG_LAST];
      int timeoutint;     /* increase timeout every N turns... */
      int timeoutinc;     /* ... by this amount ... */
      int timeoutincmult; /* ... and multiply timeoutinc by this amount ... */
      int timeoutintinc;  /* ... and increase timeoutint by this amount */
      int timeoutcounter; /* timeoutcounter - timeoutint = turns to next inc. */
      int timeoutaddenemymove; /* minimum timeout after an enemy move is seen */

      time_t last_ping;
      struct timer *phase_timer;   /* Time since seconds_to_phase_done was set. */
      int additional_phase_seconds;
      /* The game.info.phase_mode value indicates the phase mode currently in
       * use. The "stored" value is a value the player can change; it won't
       * take effect until the next turn. */
      int phase_mode_stored;
      struct timer *save_timer;
      float turn_change_time;
      char connectmsg[MAX_LEN_MSG];
      char save_name[MAX_LEN_NAME];
      bool scorelog;
      enum scorelog_level scoreloglevel;
      char scorefile[100];
      int scoreturn;    /* next make_history_report() */
      int seed_setting;
      int seed;

      bool global_warming;
      int global_warming_percent;
      bool nuclear_winter;
      int nuclear_winter_percent;

      bool fogofwar_old; /* as the fog_of_war bit get changed by setting
                          * the server we need to remember the old setting */
      bool last_updated_year; /* last_updated is still counted as year in this
                               * game. */
      char rulesetdir[MAX_LEN_NAME];
      char demography[MAX_LEN_DEMOGRAPHY];
      char allow_take[MAX_LEN_ALLOW_TAKE];

      bool settings_gamestart_valid; /* Valid settings from the game start. */

      struct rgbcolor_list *plr_colors;

      struct section_file *luadata;

      struct {
        int turns;
        int max_size;
        bool chat;
        bool info;
      } event_cache;

      /* used by the map editor to control game_save. */
      struct {
        bool save_known; /* loading will just reveal the squares around
                          * cities and units */
        bool save_starts; /* start positions will be auto generated */
        bool save_private_map; /* FoW map; will be created if not saved */
      } save_options;

      struct {
        char user_message[256];
        char type[20];
      } meta_info;

      struct {
        fc_mutex city_list;
      } mutexes;

      struct trait_limits default_traits[TRAIT_COUNT];

      struct {
        char *description_file;
        char *nationlist;
        char **embedded_nations;
        size_t embedded_nations_count;
        const char **allowed_govs;
        char **nc_agovs;
        size_t ag_count;
        const char **allowed_terrains;
        char **nc_aterrs;
        size_t at_count;
        const char **allowed_styles;
        char **nc_astyles;
        size_t as_count;
        int named_teams;
      } ruledit;
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
static inline void i_am_tool(void)
{
  i_am_server(); /* No difference between a tool and server at the moment */
}

void game_init(bool keep_ruleset_value);
void game_map_init(void);
void game_free(void);
void game_reset(void);

void game_ruleset_init(void);
void game_ruleset_free(void);

int civ_population(const struct player *pplayer);
struct city *game_city_by_name(const char *name);
struct city *game_city_by_number(int id);

struct unit *game_unit_by_number(int id);

void game_remove_unit(struct world *gworld, struct unit *punit);
void game_remove_city(struct world *gworld, struct city *pcity);
void initialize_globals(void);

bool is_player_phase(const struct player *pplayer, int phase);

const char *population_to_text(int thousand_citizen);

int generate_save_name(const char *format, char *buf, int buflen,
                       const char *reason);

void user_flag_init(struct user_flag *flag);
void user_flag_free(struct user_flag *flag);

int current_turn_timeout(void);

extern struct civ_game game;
extern struct world wld;

#define GAME_DEFAULT_SEED        0
#define GAME_MIN_SEED            0
#define GAME_MAX_SEED            (MAX_UINT32 >> 1)

#define GAME_DEFAULT_GOLD        50
#define GAME_MIN_GOLD            0
#define GAME_MAX_GOLD            50000

#define GAME_DEFAULT_START_UNITS  "ccwwx"
#define GAME_DEFAULT_START_CITY  FALSE

#define GAME_DEFAULT_DISPERSION  0
#define GAME_MIN_DISPERSION      0
#define GAME_MAX_DISPERSION      10

#define GAME_DEFAULT_TECHLEVEL   0
#define GAME_MIN_TECHLEVEL       0
#define GAME_MAX_TECHLEVEL       100

#define GAME_DEFAULT_ANGRYCITIZEN TRUE

#define GAME_DEFAULT_END_TURN    5000
#define GAME_MIN_END_TURN        0
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

#define GAME_DEFAULT_NATIONSET       ""

#define GAME_DEFAULT_FOODBOX         100
#define GAME_MIN_FOODBOX             1
#define GAME_MAX_FOODBOX             10000

#define GAME_DEFAULT_SHIELDBOX 100
#define GAME_MIN_SHIELDBOX 1
#define GAME_MAX_SHIELDBOX 10000

#define GAME_DEFAULT_SCIENCEBOX 100
#define GAME_MIN_SCIENCEBOX 1
#define GAME_MAX_SCIENCEBOX 10000

#define GAME_DEFAULT_DIPLBULBCOST    0
#define GAME_MIN_DIPLBULBCOST        0
#define GAME_MAX_DIPLBULBCOST        100

#define GAME_DEFAULT_DIPLGOLDCOST    0
#define GAME_MIN_DIPLGOLDCOST        0
#define GAME_MAX_DIPLGOLDCOST        100

#define GAME_DEFAULT_FOGOFWAR        TRUE

#define GAME_DEFAULT_FOGGEDBORDERS   FALSE

#define GAME_DEFAULT_GLOBAL_WARMING  TRUE

#define GAME_DEFAULT_GLOBAL_WARMING_PERCENT 100
#define GAME_MIN_GLOBAL_WARMING_PERCENT 1
#define GAME_MAX_GLOBAL_WARMING_PERCENT 10000

#define GAME_DEFAULT_NUCLEAR_WINTER  TRUE

#define GAME_DEFAULT_NUCLEAR_WINTER_PERCENT 100
#define GAME_MIN_NUCLEAR_WINTER_PERCENT 1
#define GAME_MAX_NUCLEAR_WINTER_PERCENT 10000

#define GAME_DEFAULT_BORDERS         BORDERS_ENABLED

#define GAME_DEFAULT_HAPPYBORDERS    HB_NATIONAL

#define GAME_DEFAULT_DIPLOMACY       DIPLO_FOR_ALL

#define GAME_DEFAULT_DIPLCHANCE      80
#define GAME_MIN_DIPLCHANCE          40
#define GAME_MAX_DIPLCHANCE          100

#define GAME_DEFAULT_FREECOST        0
#define GAME_MIN_FREECOST            0
#define GAME_MAX_FREECOST            100

#define GAME_DEFAULT_CONQUERCOST     0
#define GAME_MIN_CONQUERCOST         0
#define GAME_MAX_CONQUERCOST         100

#define GAME_DEFAULT_TECHLOSSFG      -1
#define GAME_MIN_TECHLOSSFG          -1
#define GAME_MAX_TECHLOSSFG          200

#define GAME_DEFAULT_TECHLOSSREST    50
#define GAME_MIN_TECHLOSSREST        -1
#define GAME_MAX_TECHLOSSREST        100

#define GAME_DEFAULT_TECHLEAK        100
#define GAME_MIN_TECHLEAK            0
#define GAME_MAX_TECHLEAK            300

#define GAME_DEFAULT_CITYMINDIST     2
#define GAME_MIN_CITYMINDIST         1
#define GAME_MAX_CITYMINDIST         11

#define GAME_DEFAULT_CIVILWARSIZE    10
#define GAME_MIN_CIVILWARSIZE        2 /* can't split an empire of 1 city */
#define GAME_MAX_CIVILWARSIZE        1000

#define GAME_DEFAULT_RESTRICTINFRA   FALSE
#define GAME_DEFAULT_UNRPROTECTS     TRUE

#define GAME_DEFAULT_CONTACTTURNS    20
#define GAME_MIN_CONTACTTURNS        0
#define GAME_MAX_CONTACTTURNS        100

#define GAME_DEFAULT_CELEBRATESIZE    3

#define GAME_DEFAULT_RAPTUREDELAY    1
#define GAME_MIN_RAPTUREDELAY        1
#define GAME_MAX_RAPTUREDELAY        99 /* 99 practicaly disables rapturing */

#define GAME_DEFAULT_DISASTERS       10
#define GAME_MIN_DISASTERS           0
#define GAME_MAX_DISASTERS           1000

#define GAME_DEFAULT_TRAIT_DIST_MODE TDM_FIXED

#define GAME_DEFAULT_SAVEPALACE      TRUE

#define GAME_DEFAULT_HOMECAUGHTUNITS TRUE

#define GAME_DEFAULT_NATURALCITYNAMES TRUE

#define GAME_DEFAULT_MIGRATION        FALSE

#define GAME_DEFAULT_MGR_TURNINTERVAL 5
#define GAME_MIN_MGR_TURNINTERVAL     1
#define GAME_MAX_MGR_TURNINTERVAL     100

#define GAME_DEFAULT_MGR_FOODNEEDED   TRUE

/* definition of the migration distance in relation to the current city
 * radius; 0 means that migration is possible if the second city is within
 * the city radius */
#define GAME_DEFAULT_MGR_DISTANCE     0
#define GAME_MIN_MGR_DISTANCE         (0 - CITY_MAP_MAX_RADIUS)
#define GAME_MAX_MGR_DISTANCE         (1 + CITY_MAP_MAX_RADIUS)

#define GAME_DEFAULT_MGR_NATIONCHANCE 50
#define GAME_MIN_MGR_NATIONCHANCE     0
#define GAME_MAX_MGR_NATIONCHANCE     100

#define GAME_DEFAULT_MGR_WORLDCHANCE  10
#define GAME_MIN_MGR_WORLDCHANCE      0
#define GAME_MAX_MGR_WORLDCHANCE      100

#define GAME_DEFAULT_AQUEDUCTLOSS    0
#define GAME_MIN_AQUEDUCTLOSS        0
#define GAME_MAX_AQUEDUCTLOSS        100

#define GAME_DEFAULT_KILLSTACK       TRUE
#define GAME_DEFAULT_KILLCITIZEN     TRUE

#define GAME_DEFAULT_KILLUNHOMED     0
#define GAME_MIN_KILLUNHOMED         0
#define GAME_MAX_KILLUNHOMED         100

#define GAME_DEFAULT_TECHPENALTY     100
#define GAME_MIN_TECHPENALTY         0
#define GAME_MAX_TECHPENALTY         100

#define GAME_DEFAULT_TECHLOST_RECV   0
#define GAME_MIN_TECHLOST_RECV       0
#define GAME_MAX_TECHLOST_RECV       100

#define GAME_DEFAULT_TECHLOST_DONOR  0
#define GAME_MIN_TECHLOST_DONOR      0
#define GAME_MAX_TECHLOST_DONOR      100

#define GAME_DEFAULT_TEAM_POOLED_RESEARCH TRUE
#define GAME_DEFAULT_MULTIRESEARCH   FALSE

#define GAME_DEFAULT_RAZECHANCE      20
#define GAME_MIN_RAZECHANCE          0
#define GAME_MAX_RAZECHANCE          100

#define GAME_DEFAULT_REVEALMAP       REVEAL_MAP_NONE

#define GAME_DEFAULT_SCORELOG        FALSE
#define GAME_DEFAULT_SCORELOGLEVEL   SL_ALL
#define GAME_DEFAULT_SCOREFILE       "freeciv-score.log"

/* Turns between reports is random between SCORETURN and (2 x SCORETURN).
 * First report is shown at SCORETURN. As report is generated in the end of the turn,
 * first report is already generated at (SCORETURN - 1) */
#define GAME_DEFAULT_SCORETURN       20

#define GAME_DEFAULT_VICTORY_CONDITIONS (1 << VC_SPACERACE | 1 << VC_ALLIED)
#define GAME_DEFAULT_END_SPACESHIP   TRUE

#define GAME_DEFAULT_TURNBLOCK       TRUE

#define GAME_DEFAULT_AUTO_AI_TOGGLE  FALSE

#define GAME_DEFAULT_TIMEOUT         0
#define GAME_DEFAULT_FIRST_TIMEOUT   -1
#define GAME_DEFAULT_TIMEOUTINT      0
#define GAME_DEFAULT_TIMEOUTINTINC   0
#define GAME_DEFAULT_TIMEOUTINC      0
#define GAME_DEFAULT_TIMEOUTINCMULT  1
#define GAME_DEFAULT_TIMEOUTADDEMOVE 0
#define GAME_DEFAULT_TIMEOUTCOUNTER  1

#define GAME_DEFAULT_MAXCONNECTIONSPERHOST 4
#define GAME_MIN_MAXCONNECTIONSPERHOST     0
#define GAME_MAX_MAXCONNECTIONSPERHOST     MAX_NUM_CONNECTIONS

#define GAME_MIN_TIMEOUT             -1
#define GAME_MAX_TIMEOUT             8639999
#define GAME_MIN_FIRST_TIMEOUT       -1
#define GAME_MAX_FIRST_TIMEOUT       GAME_MAX_TIMEOUT

#define GAME_MIN_UNITWAITTIME        0
#define GAME_MAX_UNITWAITTIME        GAME_MAX_TIMEOUT
#define GAME_DEFAULT_UNITWAITTIME    0

#define GAME_DEFAULT_PHASE_MODE 0

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

#define GAME_DEFAULT_TRADEWORLDRELPCT 50
#define GAME_MIN_TRADEWORLDRELPCT     0
#define GAME_MAX_TRADEWORLDRELPCT     100

#define GAME_DEFAULT_FULLTRADESIZE   1
#define GAME_MIN_FULLTRADESIZE       1
#define GAME_MAX_FULLTRADESIZE       50

#define GAME_DEFAULT_TRADING_TECH    TRUE
#define GAME_DEFAULT_TRADING_GOLD    TRUE
#define GAME_DEFAULT_TRADING_CITY    TRUE

#define GAME_DEFAULT_CARAVAN_BONUS_STYLE CBS_CLASSIC

#define GAME_DEFAULT_TRADEMINDIST    9
#define GAME_MIN_TRADEMINDIST        1
#define GAME_MAX_TRADEMINDIST        999

#define GAME_DEFAULT_TRADE_REVENUE_STYLE TRS_CLASSIC

#define GAME_DEFAULT_BARBARIANRATE   BARBS_NORMAL

#define GAME_DEFAULT_ONSETBARBARIAN  60
#define GAME_MIN_ONSETBARBARIAN      1
#define GAME_MAX_ONSETBARBARIAN      GAME_MAX_END_TURN

#define GAME_DEFAULT_OCCUPYCHANCE    0
#define GAME_MIN_OCCUPYCHANCE        0
#define GAME_MAX_OCCUPYCHANCE        100

#define GAME_DEFAULT_AUTOATTACK      FALSE

#ifdef FREECIV_WEB
#define GAME_DEFAULT_RULESETDIR      "classic"
#else  /* FREECIV_WEB */
#define GAME_DEFAULT_RULESETDIR      "civ2civ3"
#endif /* FREECIV_WEB */

#define GAME_DEFAULT_SAVE_NAME       "freeciv"
#define GAME_DEFAULT_SAVETURNS       1
#define GAME_MIN_SAVETURNS           1
#define GAME_MAX_SAVETURNS           200
#define GAME_DEFAULT_SAVEFREQUENCY   15
#define GAME_MIN_SAVEFREQUENCY       2
#define GAME_MAX_SAVEFREQUENCY       1440

#ifdef FREECIV_WEB
#define GAME_DEFAULT_AUTOSAVES       0
#else  /* FREECIV_WEB */
#define GAME_DEFAULT_AUTOSAVES       (1 << AS_TURN | 1 << AS_GAME_OVER | 1 << AS_QUITIDLE | 1 << AS_INTERRUPT)
#endif /* FREECIV_WEB */

#define GAME_DEFAULT_THREADED_SAVE   FALSE

#define GAME_DEFAULT_USER_META_MESSAGE ""

#define GAME_DEFAULT_SKILL_LEVEL     AI_LEVEL_EASY
#define GAME_HARDCODED_DEFAULT_SKILL_LEVEL 3 /* that was 'easy' in old saves */
#define GAME_OLD_DEFAULT_SKILL_LEVEL 5  /* normal; for oldest save games */

#define GAME_DEFAULT_DEMOGRAPHY      "NASRLPEMOCqrb"
#define GAME_DEFAULT_ALLOW_TAKE      "HAhadOo"

#define GAME_DEFAULT_EVENT_CACHE_TURNS    1
#define GAME_MIN_EVENT_CACHE_TURNS        0
#define GAME_MAX_EVENT_CACHE_TURNS        (GAME_MAX_END_TURN + 1)

#define GAME_DEFAULT_EVENT_CACHE_MAX_SIZE   256
#define GAME_MIN_EVENT_CACHE_MAX_SIZE        10
#define GAME_MAX_EVENT_CACHE_MAX_SIZE     20000

#define GAME_DEFAULT_EVENT_CACHE_CHAT     TRUE

#define GAME_DEFAULT_EVENT_CACHE_INFO     FALSE

#define GAME_DEFAULT_COMPRESS_LEVEL 6    /* if we have compression */
#define GAME_MIN_COMPRESS_LEVEL     1
#define GAME_MAX_COMPRESS_LEVEL     9

#if defined(FREECIV_HAVE_LIBLZMA)
#  define GAME_DEFAULT_COMPRESS_TYPE FZ_XZ
#elif defined(FREECIV_HAVE_LIBZ)
#  define GAME_DEFAULT_COMPRESS_TYPE FZ_ZLIB
#else
#  define GAME_DEFAULT_COMPRESS_TYPE FZ_PLAIN
#endif

#define GAME_DEFAULT_ALLOWED_CITY_NAMES CNM_PLAYER_UNIQUE

#define GAME_DEFAULT_PLRCOLORMODE PLRCOL_PLR_ORDER

#define GAME_DEFAULT_REVOLENTYPE        REVOLEN_RANDOM
#define GAME_DEFAULT_REVOLUTION_LENGTH  5
#define GAME_MIN_REVOLUTION_LENGTH      1
#define GAME_MAX_REVOLUTION_LENGTH      20

#define GAME_START_YEAR -4000

#define GAME_DEFAULT_AIRLIFTINGSTYLE AIRLIFTING_CLASSICAL
#define GAME_DEFAULT_PERSISTENTREADY PERSISTENTR_DISABLED

#define GAME_MAX_READ_RECURSION 10 /* max recursion for the read command */

#define GAME_DEFAULT_KICK_TIME 1800     /* 1800 seconds = 30 minutes. */
#define GAME_MIN_KICK_TIME 0            /* 0 = disabling. */
#define GAME_MAX_KICK_TIME 86400        /* 86400 seconds = 24 hours. */

/* Max distance from the capital used to calculat the bribe cost. */
#define GAME_UNIT_BRIBE_DIST_MAX 32

/* Max number of recursive transports. */
#define GAME_TRANSPORT_MAX_RECURSIVE 5

/* ruleset settings */

#define RS_MAX_VALUE                             10000

/* TRANS: year label (Anno Domini) */
#define RS_DEFAULT_POS_YEAR_LABEL                N_("AD")
/* TRANS: year label (Before Christ) */
#define RS_DEFAULT_NEG_YEAR_LABEL                N_("BC")

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

#define RS_DEFAULT_BORDER_RADIUS_SQ_CITY_PERMANENT 0
#define RS_MIN_BORDER_RADIUS_SQ_CITY_PERMANENT   (-CITY_MAP_MAX_RADIUS_SQ)
#define RS_MAX_BORDER_RADIUS_SQ_CITY_PERMANENT   401 /* city radius 20 */

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

/* the constants CITY_MAP_*_RADIUS are defined in city.c */
#define RS_DEFAULT_CITY_RADIUS_SQ                CITY_MAP_DEFAULT_RADIUS_SQ
#define RS_MIN_CITY_RADIUS_SQ                    CITY_MAP_MIN_RADIUS_SQ
#define RS_MAX_CITY_RADIUS_SQ                    CITY_MAP_MAX_RADIUS_SQ

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

#define RS_DEFAULT_SLOW_INVASIONS                TRUE

#define RS_DEFAULT_TIRED_ATTACK                  FALSE

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

#define RS_DEFAULT_TECH_UPKEEP_DIVIDER   100
#define RS_MIN_TECH_UPKEEP_DIVIDER       1
#define RS_MAX_TECH_UPKEEP_DIVIDER       100000

#define RS_DEFAULT_BASE_TECH_COST                20
#define RS_MIN_BASE_TECH_COST                    0
#define RS_MAX_BASE_TECH_COST                    200

#define RS_DEFAULT_FORCE_TRADE_ROUTE             FALSE
#define RS_DEFAULT_FORCE_CAPTURE_UNITS           FALSE
#define RS_DEFAULT_FORCE_BOMBARD                 FALSE
#define RS_DEFAULT_FORCE_EXPLODE_NUCLEAR         FALSE

#define RS_DEFAULT_POISON_EMPTIES_FOOD_STOCK     FALSE
#define RS_DEFAULT_BOMBARD_MAX_RANGE             1

#define RS_ACTION_NO_MAX_DISTANCE                "unlimited"

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__GAME_H */
