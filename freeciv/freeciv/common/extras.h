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
#ifndef FC__EXTRAS_H
#define FC__EXTRAS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* common */
#include "base.h"
#include "fc_types.h"
#include "road.h"
#include "terrain.h"

/* Used in the network protocol. */
#define SPECENUM_NAME extra_flag_id
/* Tile with this extra is considered native for units in tile. */
#define SPECENUM_VALUE0 EF_NATIVE_TILE
#define SPECENUM_VALUE0NAME N_("?extraflag:NativeTile")
/* Refuel native units */
#define SPECENUM_VALUE1 EF_REFUEL
#define SPECENUM_VALUE1NAME N_("?extraflag:Refuel")
#define SPECENUM_VALUE2 EF_TERR_CHANGE_REMOVES
#define SPECENUM_VALUE2NAME N_("?extraflag:TerrChangeRemoves")
/* Extra will be built in cities automatically */
#define SPECENUM_VALUE3 EF_AUTO_ON_CITY_CENTER
#define SPECENUM_VALUE3NAME N_("?extraflag:AutoOnCityCenter")
/* Extra is always present in cities */
#define SPECENUM_VALUE4 EF_ALWAYS_ON_CITY_CENTER
#define SPECENUM_VALUE4NAME N_("?extraflag:AlwaysOnCityCenter")
/* Road style gfx from ocean extra connects to nearby land */
#define SPECENUM_VALUE5 EF_CONNECT_LAND
#define SPECENUM_VALUE5NAME N_("?extraflag:ConnectLand")
/* Counts towards Global Warming */
#define SPECENUM_VALUE6 EF_GLOBAL_WARMING
#define SPECENUM_VALUE6NAME N_("?extraflag:GlobalWarming")
/* Counts towards Nuclear Winter */
#define SPECENUM_VALUE7 EF_NUCLEAR_WINTER
#define SPECENUM_VALUE7NAME N_("?extraflag:NuclearWinter")
/* Owner's flag will be shown on the tile */
#define SPECENUM_VALUE8 EF_SHOW_FLAG
#define SPECENUM_VALUE8NAME N_("?extraflag:ShowFlag")
/* Extra's defense bonus will be counted to
 * separate "Natural" defense layer. */
#define SPECENUM_VALUE9 EF_NATURAL_DEFENSE
#define SPECENUM_VALUE9NAME N_("?extraflag:NaturalDefense")
/* Units inside will not die all at once */
#define SPECENUM_VALUE10 EF_NO_STACK_DEATH
#define SPECENUM_VALUE10NAME N_("NoStackDeath")

#define SPECENUM_VALUE11 EF_USER_FLAG_1
#define SPECENUM_VALUE12 EF_USER_FLAG_2
#define SPECENUM_VALUE13 EF_USER_FLAG_3
#define SPECENUM_VALUE14 EF_USER_FLAG_4
#define SPECENUM_VALUE15 EF_USER_FLAG_5
#define SPECENUM_VALUE16 EF_USER_FLAG_6
#define SPECENUM_VALUE17 EF_USER_FLAG_7
#define SPECENUM_VALUE18 EF_USER_FLAG_8

#define SPECENUM_COUNT EF_COUNT
#define SPECENUM_NAMEOVERRIDE
#define SPECENUM_BITVECTOR bv_extra_flags
#include "specenum_gen.h"

#define EF_LAST_USER_FLAG EF_USER_FLAG_8
#define MAX_NUM_USER_EXTRA_FLAGS (EF_LAST_USER_FLAG - EF_USER_FLAG_1 + 1)

#define EXTRA_NONE (-1)

struct extra_type
{
  int id;
  struct name_translation name;
  bool ruledit_disabled;
  enum extra_category category;
  uint16_t causes;
  uint8_t rmcauses;

  char graphic_str[MAX_LEN_NAME];
  char graphic_alt[MAX_LEN_NAME];
  char activity_gfx[MAX_LEN_NAME];
  char act_gfx_alt[MAX_LEN_NAME];
  char act_gfx_alt2[MAX_LEN_NAME];
  char rmact_gfx[MAX_LEN_NAME];
  char rmact_gfx_alt[MAX_LEN_NAME];

  struct requirement_vector reqs;
  struct requirement_vector rmreqs;
  struct requirement_vector appearance_reqs;
  struct requirement_vector disappearance_reqs;

  /* 'buildable' is unclean. Clean solution would be to rely solely on extra_cause:
   * if the extra cannot be built, it's not in the cause's list.
   * But we currently rely on actually-not-buildable extras to be on the lists,
   * for example for the editor to list non-buildable but editor-placeable
   * extras. */
  bool buildable;
  bool generated;
  int build_time;
  int build_time_factor;
  int removal_time;
  int removal_time_factor;

  int defense_bonus;
  int appearance_chance;
  int disappearance_chance;

  enum extra_unit_seen_type eus;

  bv_unit_classes native_to;

  bv_extra_flags flags;
  bv_extras conflicts;
  bv_extras hidden_by;
  bv_extras bridged_over; /* Needs "bridge" to get built over these extras */

  Tech_type_id visibility_req;

  /* Same information as in hidden_by, but iterating through this list is much
   * faster than through all extra types to check which ones are hiding this one.
   * Only used client side. */
  struct extra_type_list *hiders;

  /* Same information as bridged_over */
  struct extra_type_list *bridged;

  struct strvec *helptext;

  struct
  {
    int special_idx;
    struct base_type *base;
    struct road_type *road;
    struct resource_type *resource;
  } data;
};

/* get 'struct extra_type_list' and related functions: */
#define SPECLIST_TAG extra_type
#define SPECLIST_TYPE struct extra_type
#include "speclist.h"

#define extra_type_list_iterate(extralist, pextra) \
    TYPED_LIST_ITERATE(struct extra_type, extralist, pextra)
#define extra_type_list_iterate_end LIST_ITERATE_END

#define extra_type_list_iterate_rev(extralist, pextra) \
    TYPED_LIST_ITERATE_REV(struct extra_type, extralist, pextra)
#define extra_type_list_iterate_rev_end LIST_ITERATE_REV_END

void extras_init(void);
void extras_free(void);

int extra_count(void);
int extra_number(const struct extra_type *pextra);
struct extra_type *extra_by_number(int id);

/* For optimization purposes (being able to have it as macro instead of function
 * call) this is now same as extra_number(). extras.c does have semantically correct
 * implementation too. */
#define extra_index(_e_) (_e_)->id

const char *extra_name_translation(const struct extra_type *pextra);
const char *extra_rule_name(const struct extra_type *pextra);
struct extra_type *extra_type_by_rule_name(const char *name);
struct extra_type *extra_type_by_translated_name(const char *name);

#define extra_base_get(_e_) (_e_)->data.base
#define extra_road_get(_e_) (_e_)->data.road
#define extra_resource_get(_e_) (_e_)->data.resource

void extra_to_caused_by_list(struct extra_type *pextra, enum extra_cause cause);
struct extra_type_list *extra_type_list_by_cause(enum extra_cause cause);
struct extra_type *rand_extra_for_tile(struct tile *ptile, enum extra_cause cause,
                                       bool generated);

struct extra_type_list *extra_type_list_of_unit_hiders(void);

#define is_extra_caused_by(e, c) (e->causes & (1 << c))
bool is_extra_caused_by_worker_action(const struct extra_type *pextra);
bool is_extra_caused_by_action(const struct extra_type *pextra,
                               enum unit_activity act);

void extra_to_removed_by_list(struct extra_type *pextra, enum extra_rmcause rmcause);
struct extra_type_list *extra_type_list_by_rmcause(enum extra_rmcause rmcause);

bool is_extra_removed_by(const struct extra_type *pextra, enum extra_rmcause rmcause);
bool is_extra_removed_by_worker_action(const struct extra_type *pextra);
bool is_extra_removed_by_action(const struct extra_type *pextra,
                                enum unit_activity act);

bool is_extra_card_near(const struct tile *ptile, const struct extra_type *pextra);
bool is_extra_near_tile(const struct tile *ptile, const struct extra_type *pextra);

bool extra_can_be_built(const struct extra_type *pextra, const struct tile *ptile);
bool can_build_extra(const struct extra_type *pextra,
                     const struct unit *punit,
                     const struct tile *ptile);
bool can_build_extra_base(const struct extra_type *pextra,
                          const struct player *pplayer,
                          const struct tile *ptile);
bool player_can_build_extra(const struct extra_type *pextra,
                            const struct player *pplayer,
                            const struct tile *ptile);

bool can_remove_extra(struct extra_type *pextra,
                      const struct unit *punit,
                      const struct tile *ptile);
bool player_can_remove_extra(const struct extra_type *pextra,
                             const struct player *pplayer,
                             const struct tile *ptile);

bool is_native_extra_to_uclass(const struct extra_type *pextra,
                               const struct unit_class *pclass);
bool is_native_extra_to_utype(const struct extra_type *pextra,
                              const struct unit_type *punittype);
bool is_native_tile_to_extra(const struct extra_type *pextra,
                             const struct tile *ptile);
bool extra_conflicting_on_tile(const struct extra_type *pextra,
                               const struct tile *ptile);

bool extra_has_flag(const struct extra_type *pextra, enum extra_flag_id flag);
bool is_extra_flag_card_near(const struct tile *ptile,
                             enum extra_flag_id flag);
bool is_extra_flag_near_tile(const struct tile *ptile,
                             enum extra_flag_id flag);

void user_extra_flags_init(void);
void extra_flags_free(void);
void set_user_extra_flag_name(enum extra_flag_id id,
                              const char *name,
                              const char *helptxt);
const char *extra_flag_helptxt(enum extra_flag_id id);

bool extra_causes_env_upset(struct extra_type *pextra,
                            enum environment_upset_type upset);

bool can_extras_coexist(const struct extra_type *pextra1,
                        const struct extra_type *pextra2);

bool can_extra_appear(const struct extra_type *pextra, const struct tile *ptile);
bool can_extra_disappear(const struct extra_type *pextra, const struct tile *ptile);

struct extra_type *next_extra_for_tile(const struct tile *ptile, enum extra_cause cause,
                                       const struct player *pplayer,
                                       const struct unit *punit);
struct extra_type *prev_extra_in_tile(const struct tile *ptile, enum extra_rmcause rmcause,
                                      const struct player *pplayer,
                                      const struct unit *punit);

enum extra_cause activity_to_extra_cause(enum unit_activity act);
enum extra_rmcause activity_to_extra_rmcause(enum unit_activity act);

struct player *extra_owner(const struct tile *ptile);

bool player_knows_extra_exist(const struct player *pplayer,
			      const struct extra_type *pextra,
			      const struct tile *ptile);

#define extra_type_iterate(_p)                                \
{                                                             \
  int _i_##_p;                                                \
  for (_i_##_p = 0; _i_##_p < game.control.num_extra_types; _i_##_p++) {  \
    struct extra_type *_p = extra_by_number(_i_##_p);

#define extra_type_iterate_end                    \
  }                                               \
}

#define extra_type_re_active_iterate(_p)                      \
  extra_type_iterate(_p) {                                    \
    if (!_p->ruledit_disabled) {

#define extra_type_re_active_iterate_end                      \
    }                                                         \
  } extra_type_iterate_end;

#define extra_type_by_cause_iterate(_cause, _extra)                 \
{                                                                   \
  struct extra_type_list *_etl_##_extra = extra_type_list_by_cause(_cause); \
  if (_etl_##_extra != NULL) {                                              \
    extra_type_list_iterate(_etl_##_extra, _extra) {

#define extra_type_by_cause_iterate_end                    \
    } extra_type_list_iterate_end                          \
  }                                                        \
}

#define extra_type_by_cause_iterate_rev(_cause, _extra)                 \
{                                                                      \
 struct extra_type_list *_etl_ = extra_type_list_by_cause(_cause);     \
  extra_type_list_iterate_rev(_etl_, _extra) {

#define extra_type_by_cause_iterate_rev_end                \
  } extra_type_list_iterate_rev_end                        \
}

#define extra_type_by_rmcause_iterate(_rmcause, _extra)                   \
{                                                                         \
  struct extra_type_list *_etl_ = extra_type_list_by_rmcause(_rmcause);   \
  extra_type_list_iterate_rev(_etl_, _extra) {

#define extra_type_by_rmcause_iterate_end                \
  } extra_type_list_iterate_rev_end                      \
}

#define extra_deps_iterate(_reqs, _dep)                 \
{                                                       \
  requirement_vector_iterate(_reqs, preq) {             \
    if (preq->source.kind == VUT_EXTRA                  \
        && preq->present) {                             \
      struct extra_type *_dep;                          \
      _dep = preq->source.value.extra;

#define extra_deps_iterate_end                          \
    }                                                   \
  } requirement_vector_iterate_end;                     \
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__EXTRAS_H */
