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
#ifndef FC__PLAYER_H
#define FC__PLAYER_H

#include "fc_types.h"

#include "city.h"
#include "connection.h"
#include "nation.h"
#include "shared.h"
#include "spaceship.h"
#include "tech.h"
#include "unitlist.h"
#include "vision.h"

#define PLAYER_DEFAULT_TAX_RATE 0
#define PLAYER_DEFAULT_SCIENCE_RATE 100
#define PLAYER_DEFAULT_LUXURY_RATE 0

#define ANON_PLAYER_NAME "noname"
#define ANON_USER_NAME "Unassigned"

enum handicap_type {
  H_DIPLOMAT = 0,     /* Can't build offensive diplomats */
  H_AWAY,             /* Away mode */
  H_LIMITEDHUTS,      /* Can get only 25 gold and barbs from huts */
  H_DEFENSIVE,        /* Build defensive buildings without calculating need */
  H_EXPERIMENTAL,     /* Enable experimental AI features (for testing) */
  H_RATES,            /* Can't set its rates beyond government limits */
  H_TARGETS,          /* Can't target anything it doesn't know exists */
  H_HUTS,             /* Doesn't know which unseen tiles have huts on them */
  H_FOG,              /* Can't see through fog of war */
  H_NOPLANES,         /* Doesn't build air units */
  H_MAP,              /* Only knows map_is_known tiles */
  H_DIPLOMACY,        /* Not very good at diplomacy */
  H_REVOLUTION,       /* Cannot skip anarchy */
  H_EXPANSION,        /* Don't like being much larger than human */
  H_DANGER,           /* Always thinks its city is in danger */
  H_LAST
};

BV_DEFINE(bv_player, MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS);

BV_DEFINE(bv_handicap, H_LAST);

struct player_economic {
  int gold;
  int tax;
  int science;
  int luxury;
};

struct player_score {
  int happy;
  int content;
  int unhappy;
  int angry;
  int specialists[SP_MAX];
  int wonders;
  int techs;
  int techout;
  int landarea;
  int settledarea;
  int population; 	/* in thousand of citizen */
  int cities;
  int units;
  int pollution;
  int literacy;
  int bnp;
  int mfg;
  int spaceship;
  int game;             /* total score you get in player dialog */
};

struct player_ai {
  bool control;

  /* 
   * Valid values for tech_goal are:
   *  - any existing tech but not A_NONE or
   *  - A_UNSET.
   */
  int prev_gold;
  int maxbuycost;
  int est_upkeep; /* estimated upkeep of buildings in cities */
  /* The units of tech_want seem to be shields */
  int tech_want[A_LAST+1];
  bv_handicap handicaps;        /* sum of enum handicap_type */
  enum ai_level skill_level;   	/* 0-10 value for save/load/display */
  int fuzzy;			/* chance in 1000 to mis-decide */
  int expand;			/* percentage factor to value new cities */
  int science_cost;             /* Cost in bulbs to get new tech, relative
                                   to non-AI players (100: Equal cost) */
  int warmth, frost; /* threat of global warming / nuclear winter */
  enum barbarian_type barbarian_type;

  int love[MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS];
};

/* Diplomatic states (how one player views another).
 * (Some diplomatic states are "pacts" (mutual agreements), others aren't.)
 *
 * Adding to or reordering this array will break many things.
 */
enum diplstate_type {
  DS_ARMISTICE = 0,
  DS_WAR,
  DS_CEASEFIRE,
  DS_PEACE,
  DS_ALLIANCE,
  DS_NO_CONTACT,
  DS_TEAM,
  DS_LAST	/* leave this last */
};

enum dipl_reason {
  DIPL_OK, DIPL_ERROR, DIPL_SENATE_BLOCKING, DIPL_ALLIANCE_PROBLEM
};

/* the following are for "pacts" */
struct player_diplstate {
  enum diplstate_type type;	/* this player's disposition towards other */
  enum diplstate_type max_state; /* maximum treaty level ever had */
  int first_contact_turn; /* turn we had first contact with this player */
  int turns_left;		/* until pact (e.g., cease-fire) ends */
  int has_reason_to_cancel;	/* 0: no, 1: this turn, 2: this or next turn */
  int contact_turns_left;	/* until contact ends */
};

/***************************************************************************
  On the distinction between nations(formerly races), players, and users,
  see doc/HACKING
***************************************************************************/

enum player_debug_types {
  PLAYER_DEBUG_DIPLOMACY, PLAYER_DEBUG_TECH, PLAYER_DEBUG_LAST
};

BV_DEFINE(bv_debug, PLAYER_DEBUG_LAST);

struct attribute_block_s {
  void *data;
  int length;
#define MAX_ATTRIBUTE_BLOCK     (256*1024)	/* largest attribute block */
};

struct ai_type;

struct player {
  bool used;
  char name[MAX_LEN_NAME];
  char username[MAX_LEN_NAME];
  char ranked_username[MAX_LEN_NAME]; /* the user who will be ranked */
  int user_turns;            /* number of turns this user has played */
  bool is_male;
  struct government *government; /* may be NULL in pregame */
  struct government *target_government; /* may be NULL */
  struct nation_type *nation;
  struct team *team;
  bool is_ready; /* Did the player click "start" yet? */
  bool phase_done;
  int nturns_idle;
  bool is_alive;
  bool is_dying; /* set once the player is in the process of dying */
  bool is_winner; /* This player is winner in scenario game */
  bool surrendered; /* has indicated willingness to surrender */

  /* Turn in which the player's revolution is over; see update_revolution. */
  int revolution_finishes;

  bool capital; /* used to give player init_buildings in first city. */
  bv_player embassy;
  struct player_diplstate diplstates[MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS];
  int city_style;
  struct city_list *cities;
  struct unit_list *units;
  struct player_score score;
  struct player_economic economic;

  int bulbs_last_turn;    /* # bulbs researched last turn only */
  struct player_spaceship spaceship;
  struct player_ai ai_data;
  struct ai_type *ai;
  bool was_created;                    /* if the player was /created */
  bool is_connected;
  struct connection *current_conn;     /* non-null while handling packet */
  struct conn_list *connections;       /* will replace conn */
  struct player_tile *private_map;
  unsigned int gives_shared_vision; /* bitvector those that give you shared vision */
  unsigned int really_gives_vision; /* takes into account that p3 may see what p1 has via p2 */
  int wonders[B_LAST];              /* contains city id's or WONDER_NOT_BUILT */
  struct attribute_block_s attribute_block;
  struct attribute_block_s attribute_block_buffer;
  bv_debug debug;
};

/* A slot is a possibly uninitialized player. */
int player_slot_count(void);
bool player_slot_is_used(const struct player *pslot);
void player_slot_set_used(struct player *pslot, bool used);
struct player *player_slot_by_number(int player_id);

/* General player accessor functions. */
int player_count(void);
void set_player_count(int count);

int player_index(const struct player *pplayer);
int player_number(const struct player *pplayer);

struct player *player_by_number(const int player_id);
struct player *valid_player_by_number(const int player_id);

const char *player_name(const struct player *pplayer);
struct player *find_player_by_name(const char *name);
struct player *find_player_by_name_prefix(const char *name,
					  enum m_pre_result *result);
struct player *find_player_by_user(const char *name);

bool player_set_nation(struct player *pplayer, struct nation_type *pnation);
void player_set_unit_focus_status(struct player *pplayer);
bool player_has_embassy(const struct player *pplayer,
			const struct player *pplayer2);

bool can_player_see_unit(const struct player *pplayer,
			 const struct unit *punit);
bool can_player_see_unit_at(const struct player *pplayer,
			    const struct unit *punit,
			    const struct tile *ptile);

bool can_player_see_units_in_city(const struct player *pplayer,
				  const struct city *pcity);
bool can_player_see_city_internals(const struct player *pplayer,
				   const struct city *pcity);

bool player_owns_city(const struct player *pplayer,
		      const struct city *pcity);
bool player_can_invade_tile(const struct player *pplayer,
                            const struct tile *ptile);

struct city *player_find_city_by_id(const struct player *pplayer,
				    int city_id);
struct unit *player_find_unit_by_id(const struct player *pplayer,
				    int unit_id);

bool player_in_city_radius(const struct player *pplayer,
			   const struct tile *ptile);
bool player_knows_techs_with_flag(const struct player *pplayer,
				  enum tech_flag_id flag);
int num_known_tech_with_flag(const struct player *pplayer,
			     enum tech_flag_id flag);
int player_get_expected_income(const struct player *pplayer);

struct player_economic player_limit_to_max_rates(struct player *pplayer);

struct city *find_palace(const struct player *pplayer);

bool ai_handicap(const struct player *pplayer, enum handicap_type htype);
bool ai_fuzzy(const struct player *pplayer, bool normal_decision);

const char *diplstate_text(const enum diplstate_type type);
const char *love_text(const int love);

const struct player_diplstate *pplayer_get_diplstate(const struct player
						     *pplayer,
						     const struct player
						     *pplayer2);
bool are_diplstates_equal(const struct player_diplstate *pds1,
			  const struct player_diplstate *pds2);
enum dipl_reason pplayer_can_make_treaty(const struct player *p1,
                                         const struct player *p2,
                                         enum diplstate_type treaty);
enum dipl_reason pplayer_can_cancel_treaty(const struct player *p1,
                                           const struct player *p2);
bool pplayers_at_war(const struct player *pplayer,
		    const struct player *pplayer2);
bool pplayers_allied(const struct player *pplayer,
		    const struct player *pplayer2);
bool pplayers_in_peace(const struct player *pplayer,
                    const struct player *pplayer2);
bool players_non_invade(const struct player *pplayer1,
			const struct player *pplayer2);
bool pplayers_non_attack(const struct player *pplayer,
			const struct player *pplayer2);
bool players_on_same_team(const struct player *pplayer1,
                          const struct player *pplayer2);
int player_in_territory(const struct player *pplayer,
			const struct player *pplayer2);

bool is_barbarian(const struct player *pplayer);

bool gives_shared_vision(const struct player *me, const struct player *them);

struct player_research *get_player_research(const struct player *p1);

void player_set_winner(struct player *plr);

/* Initialization and iteration */
void player_init(struct player *plr);

#define player_slots_iterate(NAME_pslot)\
do {\
  struct player *NAME_pslot;\
  int MY_i;\
  for (MY_i = 0; MY_i < player_slot_count(); MY_i++) {\
    NAME_pslot = player_slot_by_number(MY_i);\
    if (NAME_pslot != NULL) {\

#define player_slots_iterate_end\
    }\
  }\
} while (0)

/* NB!!! This will only iterate over players for
 * which player_slot_is_used() returns TRUE. */
#define players_iterate(NAME_pplayer)\
do {\
  player_slots_iterate(NAME_pplayer) {\
    if (player_slot_is_used(NAME_pplayer)) {

#define players_iterate_end\
    }\
  } player_slots_iterate_end;\
} while (0)

/* ai love values should be in range [-MAX_AI_LOVE..MAX_AI_LOVE] */
#define MAX_AI_LOVE 1000


/* User functions. */
bool is_valid_username(const char *name);

enum ai_level find_ai_level_by_name(const char *name);
const char *ai_level_name(enum ai_level level);
const char *ai_level_cmd(enum ai_level level);
bool is_settable_ai_level(enum ai_level level);
int number_of_ai_levels(void);

#endif  /* FC__PLAYER_H */
