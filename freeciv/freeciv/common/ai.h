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
#ifndef FC__AI_H
#define FC__AI_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* common */
#include "fc_types.h" /* MAX_LEN_NAME */

/* Update this capability string when ever there is changes to ai_type
 * structure below. When changing mandatory capability part, check that
 * there's enough reserved_xx pointers in the end of the structure for
 * taking to use without need to bump mandatory capability again. */
#define FC_AI_MOD_CAPSTR "+Freeciv-3.1-ai-module-2018.Feb.06"

/* Timers for all AI activities. Define it to get statistics about the AI. */
#ifdef FREECIV_DEBUG
#  undef DEBUG_AITIMERS
#endif /* FREECIV_DEBUG */

struct Treaty;
struct player;
struct adv_choice;
struct city;
struct unit;
struct tile;
struct settlermap;
struct pf_path;
struct section_file;
struct adv_data;

enum incident_type {
  INCIDENT_DIPLOMAT = 0, INCIDENT_WAR, INCIDENT_PILLAGE,
  INCIDENT_NUCLEAR, INCIDENT_NUCLEAR_NOT_TARGET,
  INCIDENT_NUCLEAR_SELF, INCIDENT_LAST
};

struct ai_type
{
  char name[MAX_LEN_NAME];

  void *private;

  struct {
    /* Called for every AI type when server quits. */
    void (*module_close)(void);

    /* Called for every AI type when game starts. Game is not necessarily new one,
       it can also be an old game loaded from a savegame. */
    void (*game_start)(void);

    /* Called for every AI type when game has ended. */
    void (*game_free)(void);

    /* Called for every AI type when map tiles are allocated. */
    void (*map_alloc)(void);

    /* Called for every AI type when the game map is ready, either generated
     * or, in case of scenario, loaded. */
    void (*map_ready)(void);

    /* Called for every AI type when map tiles are freed. */
    void (*map_free)(void);

    /* Called for every AI type when new player is added to game. */
    void (*player_alloc)(struct player *pplayer);

    /* Called for every AI type when player is freed from game. */
    void (*player_free)(struct player *pplayer);

    /* Called for every AI type for each player in game when game saved. */
    void (*player_save)(struct player *pplayer, struct section_file *file,
                        int plrno);

    /* Called for every AI type for each player in game when game loaded. */
    void (*player_load)(struct player *pplayer, const struct section_file *file,
                        int plrno);

    /* Called for every AI type for each player in game when game saved,
     * with each other player as parameter.
     * In practice it's good to use player_save_relations when you
     * want to add entries to "player%d.ai%d", but player_iterate() inside
     * player_save is better otherwise. The difference is in how clean
     * structure the produced savegame will have. */
    void (*player_save_relations)(struct player *pplayer, struct player *other,
                                  struct section_file *file, int plrno);

    /* Called for every AI type for each player in game when game loaded,
     * with each other player as parameter. */
    void (*player_load_relations)(struct player *pplayer, struct player *other,
                                  const struct section_file *file, int plrno);

    /* AI console. */
    void (*player_console)(struct player *pplayer, const char *cmd);

    /* Called for AI type that gains control of player. */
    void (*gained_control)(struct player *pplayer);

    /* Called for AI type that loses control of player. */
    void (*lost_control)(struct player *pplayer);

    /* Called for AI type of the player who gets split to two. */
    void (*split_by_civil_war)(struct player *original, struct player *created);

   /* Called for AI type of the player who got created from the split. */
    void (*created_by_civil_war)(struct player *original, struct player *created);

    /* Called for player AI type when player phase begins. This is in the
     * beginning of phase setup. See also first_activities. */
    void (*phase_begin)(struct player *pplayer, bool new_phase);

    /* Called for player AI type when player phase ends. */
    void (*phase_finished)(struct player *pplayer);

    /* Called for every AI type when new city is added to game. Called even for
     * virtual cities. */
    void (*city_alloc)(struct city *pcity);

    /* Called for every AI type when new city is removed from game. Called even for
     * virtual cities. */
    void (*city_free)(struct city *pcity);

    /* Called for every AI type when new city is added to game. Called for real cities
     * only. */
    void (*city_created)(struct city *pcity);

    /* Called for every AI type when new city is removed from game. Called for real
     * cities only. */
    void (*city_destroyed)(struct city *pcity);

    /* Called for player AI type when player gains control of city. */
    void (*city_got)(struct player *pplayer, struct city *pcity);

    /* Called for player AI type when player loses control of city. */
    void (*city_lost)(struct player *pplayer, struct city *pcity);

    /* Called for every AI type for each city in game when game saved. */
    void (*city_save)(struct section_file *file, const struct city *pcity,
                      const char *citystr);

    /* Called for every AI type for each city in game when game loaded. */
    void (*city_load)(const struct section_file *file, struct city *pcity,
                      const char *citystr);

    /* Called for player AI type when building advisor has chosen something
     * to be built in a city. This can override that decision. */
    void (*choose_building)(struct city *pcity, struct adv_choice *choice);

    /* Called for player AI when building advisor prepares to make decisions. */
    void (*build_adv_prepare)(struct player *pplayer, struct adv_data *adv);

    /* Called for every AI type when building advisor is first initialized
     * for the turn. */
    void (*build_adv_init)(struct player *pplayer);

    /* Called for player AI when building advisor should set wants for buildings.
     * Without this implemented in AI type building advisor does not adjust wants
     * at all. */
    void (*build_adv_adjust_want)(struct player *pplayer, struct city *wonder_city);

    /* Called for player AI when evaluating governments. */
    void (*gov_value)(struct player *pplayer, struct government *gov,
                      adv_want *val, bool *override);

    /* Called for every AI type when unit ruleset has been loaded. */
    void (*units_ruleset_init)(void);

    /* Called for every AI type before unit ruleset gets reloaded. */
    void (*units_ruleset_close)(void);

    /* Called for every AI type when new unit is added to game. Called even for
     * virtual units. */
    void (*unit_alloc)(struct unit *punit);

    /* Called for every AI type when unit is removed from game. Called even for
     * virtual units. */
    void (*unit_free)(struct unit *punit);

    /* Called for every AI type when new unit is added to game. Called for real
     * units only. */
    void (*unit_created)(struct unit *punit);

    /* Called for every AI type when unit is removed from game. Called for real
     * units only. */
    void (*unit_destroyed)(struct unit *punit);

    /* Called for player AI type when player gains control of unit. */
    void (*unit_got)(struct unit *punit);

    /* Called for player AI type when unit changes type. */
    void (*unit_transformed)(struct unit *punit, struct unit_type *old_type);

    /* Called for player AI type when player loses control of unit. */
    void (*unit_lost)(struct unit *punit);

    /* Called for unit owner AI type for each unit when turn ends. */
    void (*unit_turn_end)(struct unit *punit);

    /* Called for unit owner AI type when advisors goto moves unit. */
    void (*unit_move)(struct unit *punit, struct tile *ptile,
                      struct pf_path *path, int step);

    /* Called for all AI types when ever unit has moved. */
    void (*unit_move_seen)(struct unit *punit);

    /* Called for unit owner AI type when new advisor task is set for unit. */
    void (*unit_task)(struct unit *punit, enum adv_unit_task task,
                      struct tile *ptile);

    /* Called for every AI type for each unit in game when game saved. */
    void (*unit_save)(struct section_file *file, const struct unit *punit,
                      const char *unitstr);

    /* Called for every AI type for each unit in game when game loaded. */
    void (*unit_load)(const struct section_file *file, struct unit *punit,
                      const char *unitstr);

    /* Called for player AI type when autosettlers have been handled for the turn. */
    void (*settler_reset)(struct player *pplayer);

    /* Called for player AI type when autosettlers should find new work. */
    void (*settler_run)(struct player *pplayer, struct unit *punit,
                        struct settlermap *state);

    /* Called for player AI type for each autosettler still working.
       Cancelling current work there will result in settler_run() call. */
    void (*settler_cont)(struct player *pplayer, struct unit *punit,
                         struct settlermap *state);

    /* Called for player AI type when unit wants to autoexplore towards a tile. */
    void (*want_to_explore)(struct unit *punit, struct tile *target,
                            enum override_bool *allow);

    /* Called for player AI type in the beginning of player phase.
     * Unlike with phase_begin, everything is set up for phase already. */
    void (*first_activities)(struct player *pplayer);

    /* Called for player AI when player phase is already active when AI gains control. */
    void (*restart_phase)(struct player *pplayer);

    /* Called for player AI type in the beginning of player phase. Not for barbarian
     * players. */
    void (*diplomacy_actions)(struct player *pplayer);

    /* Called for player AI type in the end of player phase. */
    void (*last_activities)(struct player *pplayer);

    /* Called for player AI type when diplomatic treaty requires evaluation. */
    void (*treaty_evaluate)(struct player *pplayer, struct player *aplayer,
                            struct Treaty *ptreaty);

    /* Called for player AI type when diplomatic treaty has been accepted
     * by both parties. */
    void (*treaty_accepted)(struct player *pplayer, struct player *aplayer,
                            struct Treaty *ptreaty);

    /* Called for player AI type when first contact with another player has been
     * established. Note that when contact is between two AI players, callback
     * might be already called for the other party, so you can't assume
     * relations to be all-pristine when this gets called. */
    void (*first_contact)(struct player *pplayer, struct player *aplayer);

    /* Called for player AI type of the victim when someone does some violation
     * against him/her. */
    void (*incident)(enum incident_type type, struct player *violator,
                     struct player *victim);

    /* Called for player AI type of city owner when logging requires city debug
     * information. */
    void (*log_fragment_city)(char *buffer, int buflength, const struct city *pcity);

    /* Called for player AI type of unit owner when logging requires unit debug
     * information. */
    void (*log_fragment_unit)(char *buffer, int buflength, const struct unit *punit);

    /* Called for player AI type to decide if another player is dangerous. */
    void (*consider_plr_dangerous)(struct player *plr1, struct player *plr2,
                                   enum override_bool *result);

    /* Called for player AI type to decide if it's dangerous for unit to enter tile. */
    void (*consider_tile_dangerous)(struct tile *ptile, struct unit *punit,
                                    enum override_bool *result);

    /* Called for player AI to decide if city can be chosen to act as wonder city
     * for building advisor. */
    void (*consider_wonder_city)(struct city *pcity, bool *result);

    /* Called for player AI type with short internval */
    void (*refresh)(struct player *pplayer);

    /* Called for every AI type when tile has changed */
    void (*tile_info)(struct tile *ptile);

    /* These are here reserving space for future optional callbacks.
     * This way we don't need to change the mandatory capability of the AI module
     * interface when adding such callbacks, but existing modules just have these
     * set to NULL. Optional capability should be set when taking one of these to use,
     * so that new modules know if the server is going to call these or is it too old
     * version to do so.
     * When mandatory capability then changes again, please add new reservations to
     * replace those taken to use. */
    void (*reserved_01)(void);
    void (*reserved_02)(void);
    void (*reserved_03)(void);
    void (*reserved_04)(void);
    void (*reserved_05)(void);
  } funcs;
};

struct ai_type *ai_type_alloc(void);
void ai_type_dealloc(void);
struct ai_type *get_ai_type(int id);
int ai_type_number(const struct ai_type *ai);
void init_ai(struct ai_type *ai);
int ai_type_get_count(void);
const char *ai_name(const struct ai_type *ai);

struct ai_type *ai_type_by_name(const char *search);
const char *ai_type_name_or_fallback(const char *orig_name);

#ifdef DEBUG_AITIMERS
void ai_timer_init(void);
void ai_timer_free(void);
void ai_timer_start(const struct ai_type *ai);
void ai_timer_stop(const struct ai_type *ai);
void ai_timer_player_start(const struct player *pplayer);
void ai_timer_player_stop(const struct player *pplayer);
#else
#define ai_timer_init(...) (void) 0
#define ai_timer_free(...) (void) 0
#define ai_timer_start(...) (void) 0
#define ai_timer_stop(...) (void) 0
#define ai_timer_player_start(...) (void) 0
#define ai_timer_player_stop(...) (void) 0
#endif /* DEBUG_AITIMERS */

#define ai_type_iterate(NAME_ai)                        \
  do {                                                  \
    int _aii_;                                          \
    int _aitotal_ = ai_type_get_count();                \
    for (_aii_ = 0; _aii_ < _aitotal_ ; _aii_++) {      \
      struct ai_type *NAME_ai = get_ai_type(_aii_);

#define ai_type_iterate_end \
    }                       \
  } while (FALSE);

/* FIXME: This should also check if player is ai controlled */
#define CALL_PLR_AI_FUNC(_func, _player, ...)                           \
  do {                                                                  \
    struct player *_plr_ = _player; /* _player expanded just once */    \
    if (_plr_ && _plr_->ai && _plr_->ai->funcs._func) {                 \
      ai_timer_player_start(_plr_);                                     \
      _plr_->ai->funcs._func( __VA_ARGS__ );                            \
      ai_timer_player_stop(_plr_);                                      \
    }                                                                   \
  } while (FALSE)

#define CALL_FUNC_EACH_AI(_func, ...)           \
  do {                                          \
    ai_type_iterate(_ait_) {                    \
      if (_ait_->funcs._func) {                 \
        ai_timer_start(_ait_);                  \
        _ait_->funcs._func( __VA_ARGS__ );      \
        ai_timer_stop(_ait_);                   \
      }                                         \
    } ai_type_iterate_end;                      \
  } while (FALSE)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__AI_H */
