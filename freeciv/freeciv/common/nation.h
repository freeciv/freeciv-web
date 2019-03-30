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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* utility */
#include "iterator.h"

/* common */
#include "fc_types.h"
#include "name_translation.h"
#include "rgbcolor.h"
#include "terrain.h"            /* MAX_NUM_TERRAINS */

#define NO_NATION_SELECTED (NULL)

/* Changing this value will break network compatibility. */
#define NATION_NONE -1
#define NATION_ANY  -2

/* Nation city (server only). */
struct nation_city;

enum nation_city_preference {
  NCP_DISLIKE = -1,
  NCP_NONE = 0,
  NCP_LIKE = 1
};

#define SPECLIST_TAG nation_city
#define SPECLIST_TYPE struct nation_city
#include "speclist.h"
#define nation_city_list_iterate(citylist, pncity)                          \
  TYPED_LIST_ITERATE(struct nation_city, citylist, pncity)
#define nation_city_list_iterate_end LIST_ITERATE_END

/* Nation leader. */
struct nation_leader;
#define SPECLIST_TAG nation_leader
#define SPECLIST_TYPE struct nation_leader
#include "speclist.h"
#define nation_leader_list_iterate(leaderlist, pleader)                     \
  TYPED_LIST_ITERATE(struct nation_leader, leaderlist, pleader)
#define nation_leader_list_iterate_end LIST_ITERATE_END

/* Nation set. */
struct nation_set;
#define SPECLIST_TAG nation_set
#define SPECLIST_TYPE struct nation_set
#include "speclist.h"
#define nation_set_list_iterate(setlist, pset)                        \
  TYPED_LIST_ITERATE(struct nation_set, setlist, pset)
#define nation_set_list_iterate_end LIST_ITERATE_END

/* Nation group. */
struct nation_group;
#define SPECLIST_TAG nation_group
#define SPECLIST_TYPE struct nation_group
#include "speclist.h"
#define nation_group_list_iterate(grouplist, pgroup)                        \
  TYPED_LIST_ITERATE(struct nation_group, grouplist, pgroup)
#define nation_group_list_iterate_end LIST_ITERATE_END

/* Nation list. */
struct nation_type;
#define SPECLIST_TAG nation
#define SPECLIST_TYPE struct nation_type
#include "speclist.h"
#define nation_list_iterate(nationlist, pnation)                            \
  TYPED_LIST_ITERATE(struct nation_type, nationlist, pnation)
#define nation_list_iterate_end LIST_ITERATE_END

/* Nation hash. */
#define SPECHASH_TAG nation
#define SPECHASH_IKEY_TYPE struct nation_type *
#define SPECHASH_IDATA_TYPE void *
#include "spechash.h"
#define nation_hash_iterate(nationhash, pnation)                            \
  TYPED_HASH_KEYS_ITERATE(struct nation_type *, nationhash, pnation)
#define nation_hash_iterate_end HASH_KEYS_ITERATE_END

/* Pointer values are allocated on load then freed in free_nations(). */
struct nation_type {
  Nation_type_id item_number;
  char *translation_domain;
  struct name_translation adjective;
  struct name_translation noun_plural;
  char flag_graphic_str[MAX_LEN_NAME];
  char flag_graphic_alt[MAX_LEN_NAME];
  struct nation_leader_list *leaders;
  struct nation_style *style;
  char *legend;                         /* may be empty */

  bool is_playable;
  enum barbarian_type barb_type;

  /* Sets which this nation is assigned to */
  struct nation_set_list *sets;

  /* Groups which this nation is assigned to */
  struct nation_group_list *groups;

  struct player *player; /* Who's using the nation, or NULL. */

  /* Items given to this nation at game start. */
  /* (Only used in the client for documentation purposes.) */
  int init_techs[MAX_NUM_TECH_LIST];
  int init_buildings[MAX_NUM_BUILDING_LIST];
  struct government *init_government; /* use game default_government if NULL */
  struct unit_type *init_units[MAX_NUM_UNIT_LIST];

  union {
    struct {
      /* Only used in the server (./ai/ and ./server/). */

      struct nation_city_list *default_cities;

      /* 'civilwar_nations' is a list of the nations that can fork from
       * this one. 'parent_nations' is the inverse of this list. */
      struct nation_list *civilwar_nations;
      struct nation_list *parent_nations;

      /* Nations which we don't want in the same game. For example,
       * British and English. */
      struct nation_list *conflicts_with;

      /* Nation's associated player color (NULL if none). */
      struct rgbcolor *rgb;

      struct trait_limits *traits;

      /* This nation has no start position in the current scenario. */
      bool no_startpos;
    } server;

    struct {
      /* Only used at the client. */

      /* Whether the client is allowed to try to pick the nation at game
       * start. Reasons for restricting this include lack of start positions
       * in a scenario, or a nation outside the current nationset. However,
       * in some circumstances the server may decide to put a nation with this
       * flag in the game anyway, so the client can't rely on its absence.
       * (On the server this is calculated on the fly from other values.
       * Use is_nation_pickable() to get the answer on client or server.) */
      bool is_pickable;
    } client;
  };
};

/* Nation group structure. */
struct nation_group {
  struct name_translation name;
  bool hidden;

  union {
    struct {
      /* Only used in the server (./server/). */

      /* How much the AI will try to select a nation in the same group */
      int match;
    } server;

    /* Add client side when needed */
  };
};

/* General nation accessor functions. */
Nation_type_id nation_count(void);
Nation_type_id nation_index(const struct nation_type *pnation);
Nation_type_id nation_number(const struct nation_type *pnation);

struct nation_type *nation_by_number(const Nation_type_id nation);
struct nation_type *nation_of_player(const struct player *pplayer);
struct nation_type *nation_of_city(const struct city *pcity);
struct nation_type *nation_of_unit(const struct unit *punit);

struct nation_type *nation_by_rule_name(const char *name);
struct nation_type *nation_by_translated_plural(const char *name);

const char *nation_rule_name(const struct nation_type *pnation);

const char *nation_adjective_translation(const struct nation_type *pnation);
const char *nation_adjective_for_player(const struct player *pplayer);
const char *nation_plural_translation(const struct nation_type *pnation);
const char *nation_plural_for_player(const struct player *pplayer);

struct government *init_government_of_nation(const struct nation_type *pnation);

struct nation_style *style_of_nation(const struct nation_type *pnation);

const struct rgbcolor *nation_color(const struct nation_type *pnation);

/* Ancillary nation routines */
bool is_nation_pickable(const struct nation_type *nation);
bool is_nation_playable(const struct nation_type *nation);
enum barbarian_type nation_barbarian_type(const struct nation_type *nation);
bool can_conn_edit_players_nation(const struct connection *pconn,
				  const struct player *pplayer);

/* General nation leader accessor functions. */
const struct nation_leader_list *
nation_leaders(const struct nation_type *pnation);
struct nation_leader *nation_leader_new(struct nation_type *pnation,
                                        const char *name, bool is_male);
struct nation_leader *
nation_leader_by_name(const struct nation_type *pnation, const char *name);
const char *nation_leader_name(const struct nation_leader *pleader);
bool nation_leader_is_male(const struct nation_leader *pleader);

const char *nation_legend_translation(const struct nation_type *pnation,
                                      const char *legend);

/* General nation city accessor functions. */
struct terrain;

const struct nation_city_list *
nation_cities(const struct nation_type *pnation);
struct nation_city *nation_city_new(struct nation_type *pnation,
                                    const char *name);

const char *nation_city_name(const struct nation_city *pncity);

enum nation_city_preference
nation_city_preference_revert(enum nation_city_preference prefer);
void nation_city_set_terrain_preference(struct nation_city *pncity,
                                        const struct terrain *pterrain,
                                        enum nation_city_preference prefer);
void nation_city_set_river_preference(struct nation_city *pncity,
                                      enum nation_city_preference prefer);
enum nation_city_preference
nation_city_terrain_preference(const struct nation_city *pncity,
                               const struct terrain *pterrain);
enum nation_city_preference
nation_city_river_preference(const struct nation_city *pncity);

/* General nation set accessor routines */
int nation_set_count(void);
int nation_set_index(const struct nation_set *pset);
int nation_set_number(const struct nation_set *pset);

struct nation_set *nation_set_new(const char *set_name,
                                  const char *set_rule_name,
                                  const char *set_description);
struct nation_set *nation_set_by_number(int id);
struct nation_set *nation_set_by_rule_name(const char *name);

const char *nation_set_untranslated_name(const struct nation_set *pset);
const char *nation_set_rule_name(const struct nation_set *pset);
const char *nation_set_name_translation(const struct nation_set *pset);
const char *nation_set_description(const struct nation_set *pset);

bool nation_is_in_set(const struct nation_type *pnation,
                      const struct nation_set *pset);

struct nation_set *nation_set_by_setting_value(const char *setting);

/* General nation group accessor routines */
int nation_group_count(void);
int nation_group_index(const struct nation_group *pgroup);
int nation_group_number(const struct nation_group *pgroup);

struct nation_group *nation_group_new(const char *name);
struct nation_group *nation_group_by_number(int id);
struct nation_group *nation_group_by_rule_name(const char *name);

void nation_group_set_hidden(struct nation_group *pgroup, bool hidden);
void nation_group_set_match(struct nation_group *pgroup, int match);
bool is_nation_group_hidden(struct nation_group *pgroup);

const char *nation_group_untranslated_name(const struct nation_group *pgroup);
const char *nation_group_rule_name(const struct nation_group *pgroup);
const char *nation_group_name_translation(const struct nation_group *pgroup);

bool nation_is_in_group(const struct nation_type *pnation,
                        const struct nation_group *pgroup);

/* Initialization and iteration */
void nation_sets_groups_init(void);
void nation_sets_groups_free(void);

struct nation_set_iter;
size_t nation_set_iter_sizeof(void);
struct iterator *nation_set_iter_init(struct nation_set_iter *it);

#define nation_sets_iterate(NAME_pset)                                      \
  generic_iterate(struct nation_set_iter, struct nation_set *,              \
                  NAME_pset, nation_set_iter_sizeof,                        \
                  nation_set_iter_init)
#define nation_sets_iterate_end generic_iterate_end

struct nation_group_iter;
size_t nation_group_iter_sizeof(void);
struct iterator *nation_group_iter_init(struct nation_group_iter *it);

#define nation_groups_iterate(NAME_pgroup)                                  \
  generic_iterate(struct nation_group_iter, struct nation_group *,          \
                  NAME_pgroup, nation_group_iter_sizeof,                    \
                  nation_group_iter_init)
#define nation_groups_iterate_end generic_iterate_end

/* Initialization and iteration */
void nations_alloc(int num);
void nations_free(void);

int nations_match(const struct nation_type *pnation1,
                  const struct nation_type *pnation2,
                  bool ignore_conflicts);

struct nation_iter;
size_t nation_iter_sizeof(void);
struct iterator *nation_iter_init(struct nation_iter *it);

/* Iterate over nations.  This iterates over _all_ nations, including
 * unplayable ones (use is_nation_playable to filter if necessary).
 * This does not take account of the current nationset! -- on the
 * server, use allowed_nations_iterate() for that. */
#define nations_iterate(NAME_pnation)\
  generic_iterate(struct nation_iter, struct nation_type *,\
                  NAME_pnation, nation_iter_sizeof, nation_iter_init)
#define nations_iterate_end generic_iterate_end

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__NATION_H */
