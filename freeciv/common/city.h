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
#ifndef FC__CITY_H
#define FC__CITY_H

/* common */
#include "fc_types.h"
#include "improvement.h"
#include "unitlist.h"
#include "vision.h"
#include "worklist.h"

enum production_class_type {
  PCT_UNIT,
  PCT_NORMAL_IMPROVEMENT,
  PCT_WONDER,
  PCT_LAST
};

enum city_tile_type {
  C_TILE_UNUSABLE = 0,		/* memset(), calloc(), and debugging */
  C_TILE_UNAVAILABLE,
  C_TILE_EMPTY,
  C_TILE_WORKER,
};

/* Various city options.  These are stored by the server and can be
 * toggled by the user.  Each one defaults to off.  Adding new ones
 * will break network compatibility.  Reordering them will break savegame
 * compatibility.  If you want to remove one you should replace it with
 * a CITYO_UNUSED entry; new options can just be added at the end.*/
enum city_options {
  CITYO_DISBAND,      /* If building a settler at size 1 disbands the city */
  CITYO_NEW_EINSTEIN, /* If new citizens are science specialists */
  CITYO_NEW_TAXMAN,   /* If new citizens are gold specialists */
  CITYO_LAST
};
BV_DEFINE(bv_city_options, CITYO_LAST);

/* Changing this requires updating CITY_TILES and network capabilities. */
#define CITY_MAP_RADIUS 2

/* The city includes all tiles dx^2 + dy^2 <= CITY_MAP_RADIUS_SQ */
#define CITY_MAP_RADIUS_SQ (CITY_MAP_RADIUS * CITY_MAP_RADIUS + 1)

/* Diameter of the workable city area.  Some places harcdode this number. */
#define CITY_MAP_SIZE (CITY_MAP_RADIUS * 2 + 1) 

/* Number of tiles a city can use */
#define CITY_TILES city_tiles

#define INCITE_IMPOSSIBLE_COST (1000 * 1000 * 1000)

/*
 * Number of traderoutes a city can have.
 */
#define NUM_TRADEROUTES		4

/*
 * Size of the biggest possible city.
 *
 * The constant may be changed since it isn't externally visible.
 */
#define MAX_CITY_SIZE					100

/* Iterate a city map, from the center (the city) outwards */
struct iter_index {
  int dx, dy, dist;
};

bool city_tile_index_to_xy(int *city_map_x, int *city_map_y,
                           int city_tile_index, int city_radius);
int city_map_tiles(int city_radius);

/* Iterate over the tiles of a city map. Starting at a given city radius
 * (starting at the city center is possible using _radius_min = -1) outward
 * to the tiles of _radius_max. (_x, _y) will be the valid elements of
 * [0, CITY_MAP_SIZE] taking into account the city radius. */
#define city_map_iterate_outwards_radius(_radius_min, _radius_max,	\
                                         _index, _x, _y)		\
{									\
  assert(_radius_min <= _radius_max);					\
  int _x = 0, _y = 0, _index;						\
  int _x##_y##_index = city_map_tiles(_radius_min);			\
  while (city_tile_index_to_xy(&_x, &_y, _x##_y##_index,		\
                               _radius_max)) {				\
    _index = _x##_y##_index;						\
    _x##_y##_index++;

#define city_map_iterate_outwards_radius_end				\
  }									\
}

/* Iterate a city map. This iterates over all city positions in the city
 * map starting at the city center (i.e., positions that are workable by
 * the city) using the coordinates (_x, _y). It is an abbreviation for
 * city_map_iterate_outwards_radius(_end). */
#define city_map_iterate(_x, _y)					\
  city_map_iterate_outwards_radius(-1, CITY_MAP_RADIUS, _index##_x_y,	\
                                   _x, _y)

#define city_map_iterate_end						\
  city_map_iterate_outwards_radius_end

/* Iterate the tiles between two radii of a city map. */
#define city_map_iterate_radius(_radius_min, _radius_max, _x, _y)	\
  city_map_iterate_outwards_radius(_radius_min, _radius_max,		\
                                   _index##_x_y, _x, _y)

#define city_map_iterate_radius_end					\
  city_map_iterate_outwards_radius_end

/* Iterate a city map in checked real map coordinates.
 * _radius is the city radius.
 * _city_tile is the center of the (possible) city.
 * (_x, _y) will be the valid elements of [0, CITY_MAP_SIZE] taking
 * into account the city radius. */
#define city_tile_iterate_cxy(_city_tile, _tile, _x, _y) {		\
  city_map_iterate(_x, _y) {						\
    struct tile *_tile = city_map_to_tile(_city_tile, _x, _y);		\
    if (NULL != _tile) {

#define city_tile_iterate_cxy_end					\
    }									\
  } city_map_iterate_end						\
}

/* simple extension to skip is_free_worked() tiles. */
#define city_tile_iterate_skip_free_cxy(_city_tile, _tile,		\
                                        _x, _y) {			\
  city_map_iterate(_x, _y) {						\
    if (!is_free_worked_cxy(_x, _y)) {					\
      struct tile *_tile = city_map_to_tile(_city_tile, _x, _y);	\
      if (NULL != _tile) {

#define city_tile_iterate_skip_free_cxy_end				\
      }									\
    }									\
 } city_map_iterate_end;						\
}

/* Does the same thing as city_tile_iterate_cxy, but keeps the city
 * coordinates hidden. */
#define city_tile_iterate(_city_tile, _tile)				\
{									\
  city_tile_iterate_cxy(_city_tile, _tile, _tile##_x, _tile##_y) {

 #define city_tile_iterate_end						\
   } city_tile_iterate_cxy_end;						\
 }

/* Iterate a city map. This iterates over all city positions in the city
 * map starting at the city center (i.e., positions that are workable by
 * the city) using the index (_index). It is an abbreviation for
 * city_map_iterate_outwards_radius(_end). */
#define city_map_iterate_index_xy(_index, _x, _y)			\
  city_map_iterate_outwards_radius(-1, CITY_MAP_RADIUS, _index, _x, _y)

#define city_map_iterate_index_xy_end					\
  city_map_iterate_outwards_radius_end

/* Improvement status (for cities' lists of improvements)
 * (replaced Impr_Status) */

struct built_status {
  int turn;			/* turn built, negative for old state */
#define I_NEVER		(-1)	/* Improvement never built */
#define I_DESTROYED	(-2)	/* Improvement built and destroyed */
};

/* How much this output type is penalized for unhappy cities: not at all,
 * surplus knocked down to 0, or all production removed. */
enum output_unhappy_penalty {
  UNHAPPY_PENALTY_NONE,
  UNHAPPY_PENALTY_SURPLUS,
  UNHAPPY_PENALTY_ALL_PRODUCTION
};

struct output_type {
  int index;
  const char *name; /* Untranslated name */
  const char *id;   /* Identifier string (for rulesets, etc.) */
  bool harvested;   /* Is this output type gathered by city workers? */
  enum output_unhappy_penalty unhappy_penalty;
};

enum choice_type {
  CT_NONE = 0,
  CT_BUILDING = 1,
  CT_CIVILIAN,
  CT_ATTACKER,
  CT_DEFENDER,
  CT_LAST
};

#define ASSERT_CHOICE(c)                                                 \
  do {                                                                   \
    if ((c).want > 0) {                                                  \
      assert((c).type > CT_NONE && (c).type < CT_LAST);                  \
      if ((c).type == CT_BUILDING) {                                     \
        int _iindex = improvement_index((c).value.building);             \
        assert(_iindex >= 0 && _iindex < improvement_count());           \
      } else {                                                           \
        int _uindex = utype_index((c).value.utype);                      \
        assert(_uindex >= 0 && _uindex < utype_count());                 \
      }                                                                  \
    }                                                                    \
  } while(0);

struct ai_choice {
  enum choice_type type;
  universals_u value; /* what the advisor wants */
  int want;              /* how much it wants it (0-100) */
  bool need_boat;        /* unit being built wants a boat */
};

/* Who's coming to kill us, for attack co-ordination */
struct ai_invasion {
  int attack;         /* Units capable of attacking city */
  int occupy;         /* Units capable of occupying city */
};

struct ai_city {
  int building_turn;            /* only recalculate every Nth turn */
  int building_wait;            /* for weighting values */
#define BUILDING_WAIT_MINIMUM (1)

  /* building desirabilities - easiest to handle them here -- Syela */
  /* The units of building_want are output
   * (shields/gold/luxuries) multiplied by a priority
   * (SHIELD_WEIGHTING, etc or ai->shields_priority, etc)
   */
  int building_want[B_LAST];

  struct ai_choice choice;      /* to spend gold in the right place only */

  int worth; /* Cache city worth here, sum of all weighted incomes */

  struct ai_invasion invasion;
  int attack, bcost; /* This is also for invasion - total power and value of
                      * all units coming to kill us. */

  unsigned int danger;          /* danger to be compared to assess_defense */
  unsigned int grave_danger;    /* danger, should show positive feedback */
  unsigned int urgency;         /* how close the danger is; if zero, 
                                   bodyguards can leave */
  int wallvalue;                /* how much it helps for defenders to be 
                                   ground units */

  int downtown;                 /* distance from neighbours, for locating 
                                   wonders wisely */
  int distance_to_wonder_city;  /* wondercity will set this for us, 
                                   avoiding paradox */

  bool celebrate;               /* try to celebrate in this city */
  bool diplomat_threat;         /* enemy diplomat or spy is near the city */
  bool has_diplomat;            /* this city has diplomat or spy defender */

  /* so we can contemplate with warmap fresh and decide later */
  /* These values are for builder (F_SETTLERS) and founder (F_CITIES) units.
   * Negative values indicate that the city needs a boat first;
   * -value is the degree of want in that case. */
  bool founder_boat;            /* city founder will need a boat */
  int founder_turn;             /* only recalculate every Nth turn */
  int founder_want;
  int settler_want;
  int trade_want;               /* saves a zillion calculations */

  /* Used for caching change in value from a worker performing
   * a particular activity on a particular tile. */
  int act_value[ACTIVITY_LAST][CITY_MAP_SIZE][CITY_MAP_SIZE];
};

enum citizen_category {
  CITIZEN_HAPPY,
  CITIZEN_CONTENT,
  CITIZEN_UNHAPPY,
  CITIZEN_ANGRY,
  CITIZEN_LAST,
  CITIZEN_SPECIALIST = CITIZEN_LAST,
};

/* changing this order will break network compatibility,
 * and clients that don't use the symbols. */
enum citizen_feeling {
  FEELING_BASE,		/* before any of the modifiers below */
  FEELING_LUXURY,	/* after luxury */
  FEELING_EFFECT,	/* after building effects */
  FEELING_MARTIAL,	/* after units enforce martial order */
  FEELING_FINAL,	/* after wonders (final result) */
  FEELING_LAST
};

struct city {
  char name[MAX_LEN_NAME];
  struct tile *tile; /* May be NULL, should check! */
  struct player *owner; /* Cannot be NULL. */
  struct player *original; /* Cannot be NULL. */
  int id;

  /* the people */
  int size;

  int feel[CITIZEN_LAST][FEELING_LAST];

  /* Specialists */
  int specialists[SP_MAX];

  /* trade routes */
  int trade[NUM_TRADEROUTES], trade_value[NUM_TRADEROUTES];

  /* Tile output, regardless of if the tile is actually worked. */
  unsigned char tile_output[CITY_MAP_SIZE][CITY_MAP_SIZE][O_LAST];

  /* the productions */
  int surplus[O_LAST]; /* Final surplus in each category. */
  int waste[O_LAST]; /* Waste/corruption in each category. */
  int unhappy_penalty[O_LAST]; /* Penalty from unhappy cities. */
  int prod[O_LAST]; /* Production is total minus waste and penalty. */
  int citizen_base[O_LAST]; /* Base production from citizens. */
  int usage[O_LAST]; /* Amount of each resource being used. */

  /* Cached values for CPU savings. */
  int bonus[O_LAST];

  int martial_law; /* Number of citizens pacified by martial law. */
  int unit_happy_upkeep; /* Number of citizens angered by military action. */

  /* the physics */
  int food_stock;
  int shield_stock;
  int pollution;                /* not saved */
  int illness;                  /* not saved */

  /* turn states */
  int airlift;
  bool debug;                   /* not saved */
  bool did_buy;
  bool did_sell;
  bool is_updated;              /* not saved */
  bool was_happy;
  int turn_plague;              /* last turn with plague in the city */

  int anarchy;                  /* anarchy rounds count */ 
  int rapture;                  /* rapture rounds count */ 
  int steal;                    /* diplomats steal once; for spies, gets harder */
  int turn_founded;
  int turn_last_built;
  float migration_score;        /* not saved; updated by city_migration_score. */
  int mgr_score_calc_turn;      /* not saved */

  int before_change_shields;    /* If changed this turn, shields before penalty */
  int caravan_shields;          /* If caravan has helped city to build wonder. */
  int disbanded_shields;        /* If you disband unit in a city. Count them */
  int last_turns_shield_surplus; /* The surplus we had last turn. */

  struct built_status built[B_LAST];

  struct universal production;

  /* If changed this turn, what we changed from */
  struct universal changed_from;

  struct worklist worklist;

  bv_city_options city_options;

  /* The vestigial city_map[] is only temporary, while loading saved
     games or evaluating worker output for possible tile arrangements.
     Otherwise, the derived city_map[] is no longer updated and
     propagated to the clients. Instead, tile_worked() points directly
     to the affected city.
   */
  enum city_tile_type city_map[CITY_MAP_SIZE][CITY_MAP_SIZE];

  struct {
    /* Only used at the client (the server is omniscient). */
    bool occupied;
    bool walls;
    bool happy;
    bool unhappy;

    /* The color is an index into the city_colors array in mapview_common */
    bool colored;
    int color_index;
  } client;

  struct {
    /* If > 0, workers will not be rearranged until they are unfrozen. */
    int workers_frozen;

    /* If set, workers need to be arranged when the city is unfrozen.
     * Set inside auto_arrange_workers() and city_freeze_workers_queue().
     */
    bool needs_arrange;

    /* If set, city needs to be refreshed at a later time.
     * Set inside city_refresh() and city_refresh_queue_add().
     */
    bool needs_refresh;

    /* the city map is synced with the client. */
    bool synced;

    struct vision *vision;
  } server;

  struct ai_city *ai;

  /* info for dipl/spy investigation -- used only in client */
  struct unit_list *info_units_supported;
  struct unit_list *info_units_present;

  struct unit_list *units_supported;
};

struct citystyle {
  struct name_translation name;
  char graphic[MAX_LEN_NAME];
  char graphic_alt[MAX_LEN_NAME];
  char oceanic_graphic[MAX_LEN_NAME];
  char oceanic_graphic_alt[MAX_LEN_NAME];
  char citizens_graphic[MAX_LEN_NAME];
  char citizens_graphic_alt[MAX_LEN_NAME];
  struct requirement_vector reqs;
  int replaced_by;              /* index to replacing style          */
};                              /* not incl. wall and occupied tiles */

extern struct citystyle *city_styles;
extern const Output_type_id num_output_types;
extern struct output_type output_types[];

/* get 'struct city_list' and related functions: */
#define SPECLIST_TAG city
#define SPECLIST_TYPE struct city
#include "speclist.h"

#define city_list_iterate(citylist, pcity) \
    TYPED_LIST_ITERATE(struct city, citylist, pcity)
#define city_list_iterate_end  LIST_ITERATE_END

#define cities_iterate(pcity)                                               \
{                                                                           \
  players_iterate(pcity##_player) {                                         \
    city_list_iterate(pcity##_player->cities, pcity) {

#define cities_iterate_end                                                  \
    } city_list_iterate_end;                                                \
  } players_iterate_end;                                                    \
}

#define city_list_iterate_safe(citylist, _city)                        \
{                                                                      \
  int _city##_size = city_list_size(citylist);                         \
                                                                       \
  if (_city##_size > 0) {                                              \
    int _city##_numbers[_city##_size];                                 \
    int _city##_index = 0;                                             \
                                                                       \
    city_list_iterate(citylist, _city) {                               \
      _city##_numbers[_city##_index++] = _city->id;                    \
    } city_list_iterate_end;                                           \
                                                                       \
    for (_city##_index = 0;                                            \
        _city##_index < _city##_size;                                  \
        _city##_index++) {                                             \
      struct city *_city =                                             \
       game_find_city_by_number(_city##_numbers[_city##_index]);       \
                                                                       \
      if (NULL != _city) {

#define city_list_iterate_safe_end                                     \
      }                                                                \
    }                                                                  \
  }                                                                    \
}

/* output type functions */

const char *get_output_identifier(Output_type_id output);
const char *get_output_name(Output_type_id output);
struct output_type *get_output_type(Output_type_id output);
Output_type_id find_output_type_by_identifier(const char *id);
void add_specialist_output(const struct city *pcity, int *output);

/* properties */

const char *city_name(const struct city *pcity);
struct player *city_owner(const struct city *pcity);
struct tile *city_tile(const struct city *pcity);

int city_population(const struct city *pcity);
int city_total_impr_gold_upkeep(const struct city *pcity);
int city_total_unit_gold_upkeep(const struct city *pcity);
int city_unit_unhappiness(struct unit *punit, int *free_happy);
bool city_happy(const struct city *pcity);  /* generally use celebrating instead */
bool city_unhappy(const struct city *pcity);                /* anarchy??? */
bool base_city_celebrating(const struct city *pcity);
bool city_celebrating(const struct city *pcity);            /* love the king ??? */
bool city_rapture_grow(const struct city *pcity);

/* city related improvement and unit functions */

int city_improvement_upkeep(const struct city *pcity,
			    const struct impr_type *pimprove);

bool can_city_build_improvement_direct(const struct city *pcity,
				       struct impr_type *pimprove);
bool can_city_build_improvement_later(const struct city *pcity,
				      struct impr_type *pimprove);
bool can_city_build_improvement_now(const struct city *pcity,
				    struct impr_type *pimprove);

bool can_city_build_unit_direct(const struct city *pcity,
				const struct unit_type *punittype);
bool can_city_build_unit_later(const struct city *pcity,
			       const struct unit_type *punittype);
bool can_city_build_unit_now(const struct city *pcity,
			     const struct unit_type *punittype);

bool can_city_build_direct(const struct city *pcity,
			   struct universal target);
bool can_city_build_later(const struct city *pcity,
			  struct universal target);
bool can_city_build_now(const struct city *pcity,
			struct universal target);

bool city_can_use_specialist(const struct city *pcity,
			     Specialist_type_id type);
bool city_has_building(const struct city *pcity,
		       const struct impr_type *pimprove);
bool is_capital(const struct city *pcity);
bool city_got_citywalls(const struct city *pcity);
bool city_got_defense_effect(const struct city *pcity,
                             const struct unit_type *attacker);

int city_production_build_shield_cost(const struct city *pcity);
int city_production_buy_gold_cost(const struct city *pcity);

bool city_production_has_flag(const struct city *pcity,
			      enum impr_flag_id flag);
int city_production_turns_to_build(const struct city *pcity,
				   bool include_shield_stock);

int city_change_production_penalty(const struct city *pcity,
				   struct universal target);
int city_turns_to_build(const struct city *pcity,
			struct universal target,
                        bool include_shield_stock);
int city_turns_to_grow(const struct city *pcity);
bool city_can_grow_to(const struct city *pcity, int pop_size);
bool city_can_change_build(const struct city *pcity);

void city_choose_build_default(struct city *pcity);

/* textual representation of buildings */

const char *city_improvement_name_translation(const struct city *pcity,
					      struct impr_type *pimprove);
const char *city_production_name_translation(const struct city *pcity);

/* city map functions */
bool is_valid_city_coords(const int city_x, const int city_y);
bool city_base_to_city_map(int *city_map_x, int *city_map_y,
			   const struct city *const pcity,
			   const struct tile *map_tile);
bool city_tile_to_city_map(int *city_map_x, int *city_map_y,
			   const struct tile *city_center,
			   const struct tile *map_tile);

struct tile *city_map_to_tile(const struct tile *city_center,
			      int city_map_x, int city_map_y);

/* Initialization functions */
int compare_iter_index(const void *a, const void *b);
void generate_city_map_indices(void);

/* output on spot */
int city_tile_output(const struct city *pcity, const struct tile *ptile,
		     bool is_celebrating, Output_type_id otype);
int city_tile_output_now(const struct city *pcity, const struct tile *ptile,
			 Output_type_id otype);

bool city_can_work_tile(const struct city *pcity, const struct tile *ptile);

bool city_can_be_built_here(const struct tile *ptile,
			    const struct unit *punit);

/* trade functions */
bool can_cities_trade(const struct city *pc1, const struct city *pc2);
bool can_establish_trade_route(const struct city *pc1, const struct city *pc2);
bool have_cities_trade_route(const struct city *pc1, const struct city *pc2);
int trade_between_cities(const struct city *pc1, const struct city *pc2);
int city_num_trade_routes(const struct city *pcity);
int get_caravan_enter_city_trade_bonus(const struct city *pc1, 
                                       const struct city *pc2);
int get_city_min_trade_route(const struct city *pcity, int *slot);
  
/* list functions */
struct city *city_list_find_id(struct city_list *This, int id);
struct city *city_list_find_name(struct city_list *This, const char *name);

int city_name_compare(const void *p1, const void *p2);

/* city style functions */
const char *city_style_rule_name(const int style);
const char *city_style_name_translation(const int style);

int find_city_style_by_rule_name(const char *s);
int find_city_style_by_translated_name(const char *s);

bool city_style_has_requirements(const struct citystyle *style);
int city_style_of_player(const struct player *plr);
int style_of_city(const struct city *pcity);

struct city *is_enemy_city_tile(const struct tile *ptile,
				const struct player *pplayer);
struct city *is_allied_city_tile(const struct tile *ptile,
				 const struct player *pplayer);
struct city *is_non_attack_city_tile(const struct tile *ptile,
				     const struct player *pplayer);
struct city *is_non_allied_city_tile(const struct tile *ptile,
				     const struct player *pplayer);

bool is_unit_near_a_friendly_city(const struct unit *punit);
bool is_friendly_city_near(const struct player *owner,
			   const struct tile *ptile);
bool city_exists_within_city_radius(const struct tile *ptile,
				    bool may_be_on_center);

/* granary size as a function of city size */
int city_granary_size(int city_size);

void city_add_improvement(struct city *pcity,
			  const struct impr_type *pimprove);
void city_remove_improvement(struct city *pcity,
			     const struct impr_type *pimprove);

/* city update functions */
void city_refresh_from_main_map(struct city *pcity, bool full_refresh);

int city_waste(const struct city *pcity, Output_type_id otype, int total);
int city_specialists(const struct city *pcity);                 /* elv+tax+scie */
Specialist_type_id best_specialist(Output_type_id otype,
				   const struct city *pcity);
int get_final_city_output_bonus(const struct city *pcity, Output_type_id otype);
bool city_built_last_turn(const struct city *pcity);

/* city creation / destruction */
struct city *create_city_virtual(struct player *pplayer,
				 struct tile *ptile, const char *name);
void destroy_city_virtual(struct city *pcity);
bool city_is_virtual(const struct city *pcity);

/* misc */
bool is_city_option_set(const struct city *pcity, enum city_options option);
void city_styles_alloc(int num);
void city_styles_free(void);

void add_tax_income(const struct player *pplayer, int trade, int *output);
int get_city_tithes_bonus(const struct city *pcity);
int city_pollution_types(const struct city *pcity, int shield_total,
			 int *pollu_prod, int *pollu_pop, int *pollu_mod);
int city_pollution(const struct city *pcity, int shield_total);
int city_illness(const struct city *pcity, int *ill_base, int *ill_size,
                 int *ill_trade, int *ill_pollution);

bool city_exist(int id);

/*
 * Iterates over all improvements, skipping those not yet built in the
 * given city.
 */
#define city_built_iterate(_pcity, _p)				\
  improvement_iterate(_p) {						\
    if ((_pcity)->built[improvement_index(_p)].turn <= I_NEVER) {	\
      continue;								\
    }

#define city_built_iterate_end					\
  } improvement_iterate_end;


/* Iterates over all output types in the game. */
#define output_type_iterate(output)					    \
{									    \
  Output_type_id output;						    \
									    \
  for (output = 0; output < O_LAST; output++) {

#define output_type_iterate_end						    \
  }									    \
}


/* === */

#define is_city_center(_city, _tile) (_city->tile == _tile)
#define is_free_worked(_city, _tile) (_city->tile == _tile)
#define is_free_worked_cxy(city_x, city_y) \
	(CITY_MAP_RADIUS == city_x && CITY_MAP_RADIUS == city_y)
#define FREE_WORKED_TILES (1)

enum citytile_type find_citytile_by_rule_name(const char *name);

#endif  /* FC__CITY_H */
