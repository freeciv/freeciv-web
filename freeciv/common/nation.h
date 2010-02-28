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
#ifndef FC__NATION_H
#define FC__NATION_H

#include "shared.h"		/* MAX_LEN_NAME */

#include "fc_types.h"
#include "terrain.h"            /* MAX_NUM_TERRAINS */

#define MAX_NUM_TECH_GOALS 10

/* Changing this value will break network compatibility. */
#define NO_NATION_SELECTED (NULL)

/* 
 * Purpose of this constant is to catch invalid ruleset and network
 * data and to allow static allocation of the nation_info packet.
 */
#define MAX_NUM_LEADERS MAX_NUM_ITEMS

#define MAX_NUM_NATION_GROUPS 128

/*
 * The nation_city structure holds information about a default choice for
 * the city name.  The "name" field is, of course, just the name for
 * the city.  The "river" and "terrain" fields are entries recording
 * whether the terrain is present near the city - we give higher priority
 * to cities which have matching terrain.  In the case of a river we only
 * care if the city is _on_ the river, for other terrain features we give
 * the bonus if the city is close to the terrain.  Both of these entries
 * may hold a value of 0 (no preference), 1 (city likes the terrain), or -1
 * (city doesn't like the terrain).
 *
 * This is controlled through the nation's ruleset like this:
 *   cities = "Washington (ocean, river, swamp)", "New York (!mountains)"
 */
typedef int ternary;

struct nation_city {
  char *name;
  ternary river;
  ternary terrain[MAX_NUM_TERRAINS];	
};

struct nation_leader {
  char *name;
  bool is_male;
};

struct nation_group {
  int item_number;

  char name[MAX_LEN_NAME];
  
  /* How much the AI will try to select a nation in the same group */
  int match;
};

/* Pointer values are allocated on load then freed in free_nations(). */
struct nation_type {
  Nation_type_id item_number;
  struct name_translation adjective;
  struct name_translation noun_plural;
  char flag_graphic_str[MAX_LEN_NAME];
  char flag_graphic_alt[MAX_LEN_NAME];
  int  leader_count;
  struct nation_leader *leaders;
  int city_style;
  struct nation_city *city_names;	/* The default city names. */
  char *legend;				/* may be empty */

  bool is_playable;
  enum barbarian_type barb_type;

  /* civilwar_nations is a NO_NATION_SELECTED-terminated list of index of
   * the nations that can fork from this one.  parent_nations is the inverse
   * of this array.  Server only. */
  struct nation_type **civilwar_nations;
  struct nation_type **parent_nations;

  /* Items given to this nation at game start.  Server only. */
  int init_techs[MAX_NUM_TECH_LIST];
  int init_buildings[MAX_NUM_BUILDING_LIST];
  struct government *init_government;
  struct unit_type *init_units[MAX_NUM_UNIT_LIST];

  /* Groups which this nation is assigned to */
  int num_groups;
  struct nation_group **groups;
  
  /* Nations which we don't want in the same game.
   * For example, British and English. */
  int num_conflicts;
  struct nation_type **conflicts_with;

  /* Unavailable nations aren't allowed to be chosen in the scenario. */
  bool is_available;

  struct player *player; /* Who's using the nation, or NULL. */
};

/* General nation accessor functions. */
Nation_type_id nation_count(void);
Nation_type_id nation_index(const struct nation_type *pnation);
Nation_type_id nation_number(const struct nation_type *pnation);

struct nation_type *nation_by_number(const Nation_type_id nation);
struct nation_type *nation_of_player(const struct player *pplayer);
struct nation_type *nation_of_city(const struct city *pcity);
struct nation_type *nation_of_unit(const struct unit *punit);

struct nation_type *find_nation_by_rule_name(const char *name);
struct nation_type *find_nation_by_translated_name(const char *name);

const char *nation_rule_name(const struct nation_type *pnation);

const char *nation_adjective_translation(struct nation_type *pnation);
const char *nation_adjective_for_player(const struct player *pplayer);
const char *nation_plural_translation(struct nation_type *pnation);
const char *nation_plural_for_player(const struct player *pplayer);

int city_style_of_nation(const struct nation_type *nation);

/* Ancillary nation routines */
bool is_nation_playable(const struct nation_type *nation);
enum barbarian_type nation_barbarian_type(const struct nation_type *nation);
bool can_conn_edit_players_nation(const struct connection *pconn,
				  const struct player *pplayer);

/* General nation leader accessor functions. */
struct nation_leader *get_nation_leaders(const struct nation_type *nation, int *dim);
struct nation_type **get_nation_civilwar(const struct nation_type *nation);
bool get_nation_leader_sex(const struct nation_type *nation,
			   const char *name);
bool check_nation_leader_name(const struct nation_type *nation,
			      const char *name);

/* General nation group accessor routines */
int nation_group_count(void);
int nation_group_index(const struct nation_group *pgroup);
int nation_group_number(const struct nation_group *pgroup);

struct nation_group *nation_group_by_number(int id);
struct nation_group *find_nation_group_by_rule_name(const char *name);

bool is_nation_in_group(struct nation_type *nation,
			struct nation_group *group);

/* Initialization and iteration */
void nation_groups_free(void);
struct nation_group *add_new_nation_group(const char *name);

struct nation_group *nation_group_array_first(void);
const struct nation_group *nation_group_array_last(void);

#define nation_groups_iterate(_p)					\
{									\
  struct nation_group *_p = nation_group_array_first();			\
  if (NULL != _p) {							\
    for (; _p <= nation_group_array_last(); _p++) {

#define nation_groups_iterate_end					\
    }									\
  }									\
}

/* Initialization and iteration */
void nations_alloc(int num);
void nations_free(void);
void nation_city_names_free(struct nation_city *city_names);

#include "iterator.h"
struct nation_iter;
size_t nation_iter_sizeof(void);
struct iterator *nation_iter_init(struct nation_iter *it);

/* Iterate over nations.  This iterates over all nations, including
 * unplayable ones (use is_nation_playable to filter if necessary). */
#define nations_iterate(NAME_pnation)\
  generic_iterate(struct nation_iter, struct nation_type *,\
                  NAME_pnation, nation_iter_sizeof, nation_iter_init)
#define nations_iterate_end generic_iterate_end

#endif  /* FC__NATION_H */
