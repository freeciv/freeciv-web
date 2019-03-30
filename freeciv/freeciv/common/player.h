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
#ifndef FC__PLAYER_H
#define FC__PLAYER_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* utility */
#include "bitvector.h"

/* common */
#include "city.h"
#include "connection.h"
#include "fc_types.h"
#include "multipliers.h"
#include "nation.h"
#include "shared.h"
#include "spaceship.h"
#include "style.h"
#include "tech.h"
#include "traits.h"
#include "unitlist.h"
#include "vision.h"

#define PLAYER_DEFAULT_TAX_RATE 0
#define PLAYER_DEFAULT_SCIENCE_RATE 100
#define PLAYER_DEFAULT_LUXURY_RATE 0

#define ANON_PLAYER_NAME "noname"

/* If changing this, be sure to adjust loading of pre-2.6 savegames
 * which depend on saved username to tell if user (or ranked_user) is
 * anonymous. */
#define ANON_USER_NAME N_("Unassigned")

enum plrcolor_mode {
  PLRCOL_PLR_ORDER,
  PLRCOL_PLR_RANDOM,
  PLRCOL_PLR_SET,
  PLRCOL_TEAM_ORDER,
  PLRCOL_NATION_ORDER
};

#define SPECENUM_NAME plr_flag_id
#define SPECENUM_VALUE0 PLRF_AI
#define SPECENUM_VALUE0NAME "ai"
#define SPECENUM_VALUE1 PLRF_SCENARIO_RESERVED
#define SPECENUM_VALUE1NAME "ScenarioReserved"
#define SPECENUM_COUNT  PLRF_COUNT
#define SPECENUM_BITVECTOR bv_plr_flags
#include "specenum_gen.h"

struct player_slot;

struct player_economic {
  int gold;
  int tax;
  int science;
  int luxury;
};

#define SPECENUM_NAME player_status
/* 'normal' status */
#define SPECENUM_VALUE0      PSTATUS_NORMAL
/* set once the player is in the process of dying */
#define SPECENUM_VALUE1      PSTATUS_DYING
/* this player is winner in scenario game */
#define SPECENUM_VALUE2      PSTATUS_WINNER
/* has indicated willingness to surrender */
#define SPECENUM_VALUE3      PSTATUS_SURRENDER
/* keep this last */
#define SPECENUM_COUNT       PSTATUS_COUNT
#include "specenum_gen.h"

BV_DEFINE(bv_pstatus, PSTATUS_COUNT);

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
  int units_built;      /* Number of units this player produced. */
  int units_killed;     /* Number of enemy units killed. */
  int units_lost;       /* Number of own units that died,
                         * by combat or otherwise. */
  int culture;
  int game;             /* Total score you get in player dialog. */
};

struct player_ai {
  int maxbuycost;
  void *handicaps;
  enum ai_level skill_level;   	/* 0-10 value for save/load/display */
  int fuzzy;			/* chance in 1000 to mis-decide */
  int expand;			/* percentage factor to value new cities */
  int science_cost;             /* Cost in bulbs to get new tech, relative
                                   to non-AI players (100: Equal cost) */
  int warmth, frost; /* threat of global warming / nuclear winter */
  enum barbarian_type barbarian_type;

  int love[MAX_NUM_PLAYER_SLOTS];

  struct ai_trait *traits;
};

/* Diplomatic states (how one player views another).
 * (Some diplomatic states are "pacts" (mutual agreements), others aren't.)
 *
 * Adding to or reordering this array will break many things.
 *
 * Used in the network protocol.
 */
#define SPECENUM_NAME diplstate_type
#define SPECENUM_VALUE0 DS_ARMISTICE
#define SPECENUM_VALUE0NAME N_("?diplomatic_state:Armistice")
#define SPECENUM_VALUE1 DS_WAR
#define SPECENUM_VALUE1NAME N_("?diplomatic_state:War")
#define SPECENUM_VALUE2 DS_CEASEFIRE
#define SPECENUM_VALUE2NAME N_("?diplomatic_state:Cease-fire")
#define SPECENUM_VALUE3 DS_PEACE
#define SPECENUM_VALUE3NAME N_("?diplomatic_state:Peace")
#define SPECENUM_VALUE4 DS_ALLIANCE
#define SPECENUM_VALUE4NAME N_("?diplomatic_state:Alliance")
#define SPECENUM_VALUE5 DS_NO_CONTACT
#define SPECENUM_VALUE5NAME N_("?diplomatic_state:Never met")
#define SPECENUM_VALUE6 DS_TEAM
#define SPECENUM_VALUE6NAME N_("?diplomatic_state:Team")
  /* When adding or removing entries, note that first value
   * of diplrel_other should be next to last diplstate_type */
#define SPECENUM_COUNT DS_LAST	/* leave this last */
#include "specenum_gen.h"

/* Other diplomatic relation properties.
 *
 * The first element here is numbered DS_LAST
 *
 * Used in the network protocol.
 */
#define SPECENUM_NAME diplrel_other
#define SPECENUM_VALUE7 DRO_GIVES_SHARED_VISION
#define SPECENUM_VALUE7NAME N_("Gives shared vision")
#define SPECENUM_VALUE8 DRO_RECEIVES_SHARED_VISION
#define SPECENUM_VALUE8NAME N_("Receives shared vision")
#define SPECENUM_VALUE9 DRO_HOSTS_EMBASSY
#define SPECENUM_VALUE9NAME N_("Hosts embassy")
#define SPECENUM_VALUE10 DRO_HAS_EMBASSY
#define SPECENUM_VALUE10NAME N_("Has embassy")
#define SPECENUM_VALUE11 DRO_HOSTS_REAL_EMBASSY
#define SPECENUM_VALUE11NAME N_("Hosts real embassy")
#define SPECENUM_VALUE12 DRO_HAS_REAL_EMBASSY
#define SPECENUM_VALUE12NAME N_("Has real embassy")
#define SPECENUM_VALUE13 DRO_HAS_CASUS_BELLI
#define SPECENUM_VALUE13NAME N_("Has Casus Belli")
#define SPECENUM_VALUE14 DRO_PROVIDED_CASUS_BELLI
#define SPECENUM_VALUE14NAME N_("Provided Casus Belli")
#define SPECENUM_VALUE15 DRO_FOREIGN
#define SPECENUM_VALUE15NAME N_("Foreign")
#define SPECENUM_COUNT DRO_LAST
#include "specenum_gen.h"

BV_DEFINE(bv_diplrel_all_reqs,
          /* Reserve a location for each possible DiplRel requirement. */
          ((DRO_LAST - 1) * 2) * REQ_RANGE_COUNT);

enum dipl_reason {
  DIPL_OK, DIPL_ERROR, DIPL_SENATE_BLOCKING,
  DIPL_ALLIANCE_PROBLEM_US, DIPL_ALLIANCE_PROBLEM_THEM
};

/* the following are for "pacts" */
struct player_diplstate {
  enum diplstate_type type; /* this player's disposition towards other */
  enum diplstate_type max_state; /* maximum treaty level ever had */
  int first_contact_turn; /* turn we had first contact with this player */
  int turns_left; /* until pact (e.g., cease-fire) ends */
  int has_reason_to_cancel; /* 0: no, 1: this turn, 2: this or next turn */
  int contact_turns_left; /* until contact ends */

  int auto_cancel_turn; /* used to avoid asymmetric turns_left */
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
struct ai_data;

bool player_has_flag(const struct player *pplayer, enum plr_flag_id flag);

#define is_human(plr) !player_has_flag((plr), PLRF_AI)
#define is_ai(plr) player_has_flag((plr), PLRF_AI)
#define set_as_human(plr) BV_CLR((plr)->flags, PLRF_AI)
#define set_as_ai(plr) BV_SET((plr)->flags, PLRF_AI)

struct player {
  struct player_slot *slot;
  char name[MAX_LEN_NAME];
  char username[MAX_LEN_NAME];
  bool unassigned_user;
  char ranked_username[MAX_LEN_NAME]; /* the user who will be ranked */
  bool unassigned_ranked;
  int user_turns;            /* number of turns this user has played */
  bool is_male;
  struct government *government; /* may be NULL in pregame */
  struct government *target_government; /* may be NULL */
  struct nation_type *nation;
  struct team *team;
  bool is_ready; /* Did the player click "start" yet? */
  bool phase_done;    /* Has human player finished */
  bool ai_phase_done; /* Has AI type finished */
  int nturns_idle;
  int turns_alive;    /* Number of turns this player has spent alive;
                       * 0 when created, increment at the end of each turn */
  bool is_alive;
  bool is_winner;
  int last_war_action;

  /* Turn in which the player's revolution is over; see update_revolution. */
  int revolution_finishes;

  bv_player real_embassy;
  const struct player_diplstate **diplstates;
  struct nation_style *style;
  int music_style;
  struct city_list *cities;
  struct unit_list *units;
  struct player_score score;
  struct player_economic economic;

  struct player_spaceship spaceship;

  struct player_ai ai_common;
  const struct ai_type *ai;
  char *savegame_ai_type_name;

  bv_plr_flags flags;

  bool was_created;                    /* if the player was /created */
  bool random_name;
  bool is_connected;
  struct connection *current_conn;     /* non-null while handling packet */
  struct conn_list *connections;       /* will replace conn */
  bv_player gives_shared_vision;       /* bitvector those that give you
                                        * shared vision */
  int wonders[B_LAST];              /* contains city id's, WONDER_NOT_BUILT,
                                     * or WONDER_LOST */
  struct attribute_block_s attribute_block;
  struct attribute_block_s attribute_block_buffer;

  struct dbv tile_known;

  struct rgbcolor *rgb;

  /* Values currently in force. */
  int multipliers[MAX_NUM_MULTIPLIERS];
  /* Values to be used next turn. */
  int multipliers_target[MAX_NUM_MULTIPLIERS];

  int culture; /* National level culture - does not include culture of individual
                * cities. */

  union {
    struct {
      /* Only used in the server (./ai/ and ./server/). */
      bv_pstatus status;

      bool got_first_city; /* used to give player init_buildings in first
                            * city. Once set, never becomes unset.
                            * (Previously 'capital'.) */

      struct player_tile *private_map;

      /* Player can see inside his borders. */
      bool border_vision;

      bv_player really_gives_vision; /* takes into account that p3 may see
                                      * what p1 has via p2 */

      bv_debug debug;

      struct adv_data *adv;

      void *ais[FREECIV_AI_MOD_LAST];

      /* This user is allowed to take over the player. */
      char delegate_to[MAX_LEN_NAME];
      /* This is set when a player is 'involved' in a delegation.
       * There are two cases:
       *  - if delegate_to[] is set, it records the original owner, with
       *    'username' temporarily holding the delegate's name;
       *  - otherwise, it's set when a delegate's original player is 'put
       *    aside' while the delegate user controls a delegated player.
       *    (In this case orig_username == username.) */
      char orig_username[MAX_LEN_NAME];

      int huts; /* How many huts this player has found */

      int bulbs_last_turn; /* Number of bulbs researched last turn only. */
    } server;

    struct {
      /* Only used at the client (the server is omniscient; ./client/). */

      /* Corresponds to the result of
         (player:server:private_map[tile_index]:seen_count[vlayer] != 0). */
      struct dbv tile_vision[V_COUNT];

      enum mood_type mood;

      int tech_upkeep;

      bool color_changeable;
    } client;
  };
};

/* Initialization and iteration */
void player_slots_init(void);
bool player_slots_initialised(void);
void player_slots_free(void);

struct player_slot *player_slot_first(void);
struct player_slot *player_slot_next(struct player_slot *pslot);

/* A player slot contains a possibly uninitialized player. */
int player_slot_count(void);
int player_slot_index(const struct player_slot *pslot);
struct player *player_slot_get_player(const struct player_slot *pslot);
bool player_slot_is_used(const struct player_slot *pslot);
struct player_slot *player_slot_by_number(int player_id);
int player_slot_max_used_number(void);

/* General player accessor functions. */
struct player *player_new(struct player_slot *pslot);
void player_set_color(struct player *pplayer,
                      const struct rgbcolor *prgbcolor);
void player_clear(struct player *pplayer, bool full);
void player_ruleset_close(struct player *pplayer);
void player_destroy(struct player *pplayer);

int player_count(void);
int player_index(const struct player *pplayer);
int player_number(const struct player *pplayer);
struct player *player_by_number(const int player_id);

const char *player_name(const struct player *pplayer);
struct player *player_by_name(const char *name);
struct player *player_by_name_prefix(const char *name,
                                     enum m_pre_result *result);
struct player *player_by_user(const char *name);

bool player_set_nation(struct player *pplayer, struct nation_type *pnation);

bool player_has_embassy(const struct player *pplayer,
                        const struct player *pplayer2);
bool player_has_real_embassy(const struct player *pplayer,
                             const struct player *pplayer2);
bool player_has_embassy_from_effect(const struct player *pplayer,
                                    const struct player *pplayer2);

int player_age(const struct player *pplayer);

bool player_can_trust_tile_has_no_units(const struct player *pplayer,
                                        const struct tile *ptile);
bool can_player_see_hypotetic_units_at(const struct player *pplayer,
                                       const struct tile *ptile);
bool can_player_see_unit(const struct player *pplayer,
			 const struct unit *punit);
bool can_player_see_unit_at(const struct player *pplayer,
			    const struct unit *punit,
                            const struct tile *ptile,
                            bool is_transported);

bool can_player_see_units_in_city(const struct player *pplayer,
				  const struct city *pcity);
bool can_player_see_city_internals(const struct player *pplayer,
				   const struct city *pcity);
bool player_can_see_city_externals(const struct player *pow_player,
                                   const struct city *target_city);

bool player_owns_city(const struct player *pplayer,
		      const struct city *pcity);
bool player_can_invade_tile(const struct player *pplayer,
                            const struct tile *ptile);

struct city *player_city_by_number(const struct player *pplayer,
                                   int city_id);
struct unit *player_unit_by_number(const struct player *pplayer,
                                    int unit_id);

bool player_in_city_map(const struct player *pplayer,
                        const struct tile *ptile);
bool player_knows_techs_with_flag(const struct player *pplayer,
				  enum tech_flag_id flag);
int num_known_tech_with_flag(const struct player *pplayer,
			     enum tech_flag_id flag);
int player_get_expected_income(const struct player *pplayer);

struct city *player_capital(const struct player *pplayer);

const char *love_text(const int love);

enum diplstate_type cancel_pact_result(enum diplstate_type oldstate);

struct player_diplstate *player_diplstate_get(const struct player *plr1,
                                              const struct player *plr2);
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

/**************************************************************************
  Return TRUE iff player is any kind of barbarian
**************************************************************************/
static inline bool is_barbarian(const struct player *pplayer)
{
  return pplayer->ai_common.barbarian_type != NOT_A_BARBARIAN;
}

bool gives_shared_vision(const struct player *me, const struct player *them);

void diplrel_mess_close(void);
bool is_diplrel_between(const struct player *player1,
                        const struct player *player2,
                        int diplrel);
bool is_diplrel_to_other(const struct player *pplayer, int diplrel);
int diplrel_by_rule_name(const char *value);
const char *diplrel_rule_name(int value);
const char *diplrel_name_translation(int value);

bv_diplrel_all_reqs diplrel_req_contradicts(const struct requirement *req);

int player_multiplier_value(const struct player *pplayer,
                            const struct multiplier *pmul);
int player_multiplier_effect_value(const struct player *pplayer,
                                   const struct multiplier *pmul);
int player_multiplier_target_value(const struct player *pplayer,
                                   const struct multiplier *pmul);

/* iterate over all player slots */
#define player_slots_iterate(_pslot)                                        \
  if (player_slots_initialised()) {                                         \
    struct player_slot *_pslot = player_slot_first();                       \
    for (; NULL != _pslot; _pslot = player_slot_next(_pslot)) {
#define player_slots_iterate_end                                            \
    }                                                                       \
  }

/* iterate over all players, which are used at the moment */
#define players_iterate(_pplayer)                                           \
  player_slots_iterate(_pslot##_pplayer) {                                  \
    struct player *_pplayer = player_slot_get_player(_pslot##_pplayer);     \
    if (_pplayer != NULL) {

#define players_iterate_end                                                 \
    }                                                                       \
  } player_slots_iterate_end;

/* iterate over all players, which are used at the moment and are alive */
#define players_iterate_alive(_pplayer)                                     \
  players_iterate(_pplayer) {                                                \
    if (!_pplayer->is_alive) {                                              \
      continue;                                                             \
    }
#define players_iterate_alive_end                                           \
  } players_iterate_end;

/* get 'struct player_list' and related functions: */
#define SPECLIST_TAG player
#define SPECLIST_TYPE struct player
#include "speclist.h"

#define player_list_iterate(playerlist, pplayer)                            \
  TYPED_LIST_ITERATE(struct player, playerlist, pplayer)
#define player_list_iterate_end                                             \
  LIST_ITERATE_END

/* ai love values should be in range [-MAX_AI_LOVE..MAX_AI_LOVE] */
#define MAX_AI_LOVE 1000


/* User functions. */
bool is_valid_username(const char *name);

#define ai_level_cmd(_level_) ai_level_name(_level_)
bool is_settable_ai_level(enum ai_level level);
int number_of_ai_levels(void);

void *player_ai_data(const struct player *pplayer, const struct ai_type *ai);
void player_set_ai_data(struct player *pplayer, const struct ai_type *ai,
                        void *data);

static inline bool player_is_cpuhog(const struct player *pplayer)
{
  /* You have to make code change here to enable cpuhog AI. There is no even
   * configure option to change this. That's intentional.
   * Enabling them causes game to proceed differently, and for reproducing
   * reported bugs we want to know if this has been changed. People are more
   * likely to report that they have made code changes than remembering some
   * specific configure option they happened to pass to build this time - or even
   * knowing what configure options somebody else used when building freeciv for them. */
  return FALSE;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__PLAYER_H */
