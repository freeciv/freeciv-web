/****************************************************************************
 Freeciv - Copyright (C) 2004 - The Freeciv Team
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
****************************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

/* utility */
#include "rand.h"
#include "string_vector.h"

/* common */
#include "base.h"
#include "game.h"
#include "map.h"
#include "research.h"
#include "road.h"

#include "extras.h"

static struct extra_type extras[MAX_EXTRA_TYPES];

static struct user_flag user_extra_flags[MAX_NUM_USER_EXTRA_FLAGS];

static struct extra_type_list *caused_by[EC_LAST];
static struct extra_type_list *removed_by[ERM_COUNT];
static struct extra_type_list *unit_hidden;

/************************************************************************//**
  Initialize extras structures.
****************************************************************************/
void extras_init(void)
{
  int i;

  for (i = 0; i < EC_LAST; i++) {
    caused_by[i] = extra_type_list_new();
  }
  for (i = 0; i < ERM_COUNT; i++) {
    removed_by[i] = extra_type_list_new();
  }
  unit_hidden = extra_type_list_new();

  for (i = 0; i < MAX_EXTRA_TYPES; i++) {
    requirement_vector_init(&(extras[i].reqs));
    requirement_vector_init(&(extras[i].rmreqs));
    requirement_vector_init(&(extras[i].appearance_reqs));
    requirement_vector_init(&(extras[i].disappearance_reqs));
    extras[i].id = i;
    extras[i].hiders = NULL;
    extras[i].bridged = NULL;
    extras[i].data.special_idx = -1;
    extras[i].data.base = NULL;
    extras[i].data.road = NULL;
    extras[i].data.resource = NULL;
    extras[i].causes = 0;
    extras[i].rmcauses = 0;
    extras[i].helptext = NULL;
    extras[i].ruledit_disabled = FALSE;
    extras[i].visibility_req = A_NONE;
  }
}

/************************************************************************//**
  Free the memory associated with extras
****************************************************************************/
void extras_free(void)
{
  int i;

  base_types_free();
  road_types_free();
  resource_types_free();

  for (i = 0; i < game.control.num_extra_types; i++) {
    if (extras[i].data.base != NULL) {
      FC_FREE(extras[i].data.base);
      extras[i].data.base = NULL;
    }
    if (extras[i].data.road != NULL) {
      FC_FREE(extras[i].data.road);
      extras[i].data.road = NULL;
    }
    if (extras[i].data.resource != NULL) {
      FC_FREE(extras[i].data.resource);
      extras[i].data.resource = NULL;
    }
  }

  for (i = 0; i < EC_LAST; i++) {
    extra_type_list_destroy(caused_by[i]);
    caused_by[i] = NULL;
  }

  for (i = 0; i < ERM_COUNT; i++) {
    extra_type_list_destroy(removed_by[i]);
    removed_by[i] = NULL;
  }

  extra_type_list_destroy(unit_hidden);
  unit_hidden = NULL;

  for (i = 0; i < MAX_EXTRA_TYPES; i++) {
    requirement_vector_free(&(extras[i].reqs));
    requirement_vector_free(&(extras[i].rmreqs));
    requirement_vector_free(&(extras[i].appearance_reqs));
    requirement_vector_free(&(extras[i].disappearance_reqs));

    if (NULL != extras[i].helptext) {
      strvec_destroy(extras[i].helptext);
      extras[i].helptext = NULL;
    }
  }

  extra_type_iterate(pextra) {
    if (pextra->hiders != NULL) {
      extra_type_list_destroy(pextra->hiders);
      pextra->hiders = NULL;
    }
    if (pextra->bridged != NULL) {
      extra_type_list_destroy(pextra->bridged);
      pextra->bridged = NULL;
    }
  } extra_type_iterate_end;
}

/************************************************************************//**
  Return the number of extra_types.
****************************************************************************/
int extra_count(void)
{
  return game.control.num_extra_types;
}

/************************************************************************//**
  Return the extra id.
****************************************************************************/
int extra_number(const struct extra_type *pextra)
{
  fc_assert_ret_val(NULL != pextra, -1);

  return pextra->id;
}

#ifndef extra_index
/************************************************************************//**
  Return the extra index.
****************************************************************************/
int extra_index(const struct extra_type *pextra)
{
  fc_assert_ret_val(NULL != pextra, -1);

  return pextra - extras;
}
#endif /* extra_index */

/************************************************************************//**
  Return extras type of given id.
****************************************************************************/
struct extra_type *extra_by_number(int id)
{
  fc_assert_ret_val(id >= 0 && id < MAX_EXTRA_TYPES, NULL);

  return &extras[id];
}

/************************************************************************//**
  Return the (translated) name of the extra type.
  You don't have to free the return pointer.
****************************************************************************/
const char *extra_name_translation(const struct extra_type *pextra)
{
  return name_translation_get(&pextra->name);
}

/************************************************************************//**
  Return the (untranslated) rule name of the extra type.
  You don't have to free the return pointer.
****************************************************************************/
const char *extra_rule_name(const struct extra_type *pextra)
{
  return rule_name_get(&pextra->name);
}

/************************************************************************//**
  Returns extra type matching rule name or NULL if there is no extra type
  with such name.
****************************************************************************/
struct extra_type *extra_type_by_rule_name(const char *name)
{
  const char *qs;

  if (name == NULL) {
    return NULL;
  }

  qs = Qn_(name);

  extra_type_iterate(pextra) {
    if (!fc_strcasecmp(extra_rule_name(pextra), qs)) {
      return pextra;
    }
  } extra_type_iterate_end;

  return NULL;
}

/************************************************************************//**
  Returns extra type matching the translated name, or NULL if there is no
  extra type with that name.
****************************************************************************/
struct extra_type *extra_type_by_translated_name(const char *name)
{
  extra_type_iterate(pextra) {
    if (0 == strcmp(extra_name_translation(pextra), name)) {
      return pextra;
    }
  } extra_type_iterate_end;

  return NULL;
}

/************************************************************************//**
  Returns extra type for given cause.
****************************************************************************/
struct extra_type_list *extra_type_list_by_cause(enum extra_cause cause)
{
  fc_assert(cause < EC_LAST);

  return caused_by[cause];
}

/************************************************************************//**
  Returns extra types that hide units.
****************************************************************************/
struct extra_type_list *extra_type_list_of_unit_hiders(void)
{
  return unit_hidden;
}

/************************************************************************//**
  Return random extra type for given cause that is native to the tile.
****************************************************************************/
struct extra_type *rand_extra_for_tile(struct tile *ptile, enum extra_cause cause,
                                       bool generated)
{
  struct extra_type_list *full_list = extra_type_list_by_cause(cause);
  struct extra_type_list *potential = extra_type_list_new();
  int options;
  struct extra_type *selected = NULL;

  extra_type_list_iterate(full_list, pextra) {
    if ((!generated || pextra->generated)
        && is_native_tile_to_extra(pextra, ptile)) {
      extra_type_list_append(potential, pextra);
    }
  } extra_type_list_iterate_end;

  options = extra_type_list_size(potential);

  if (options > 0) {
    selected = extra_type_list_get(potential, fc_rand(options));
  }

  extra_type_list_destroy(potential);

  return selected;
}

/************************************************************************//**
  Add extra type to list of extra caused by given cause.
****************************************************************************/
void extra_to_caused_by_list(struct extra_type *pextra, enum extra_cause cause)
{
  fc_assert(cause < EC_LAST);

  extra_type_list_append(caused_by[cause], pextra);
}

/************************************************************************//**
  Returns extra type for given rmcause.
****************************************************************************/
struct extra_type_list *extra_type_list_by_rmcause(enum extra_rmcause rmcause)
{
  fc_assert(rmcause < ERM_COUNT);

  return removed_by[rmcause];
}

/************************************************************************//**
  Add extra type to list of extra removed by given cause.
****************************************************************************/
void extra_to_removed_by_list(struct extra_type *pextra,
                              enum extra_rmcause rmcause)
{
  fc_assert(rmcause < ERM_COUNT);

  extra_type_list_append(removed_by[rmcause], pextra);
}

/************************************************************************//**
  Is given cause one of the removal causes for the given extra?
****************************************************************************/
bool is_extra_removed_by(const struct extra_type *pextra,
                         enum extra_rmcause rmcause)
{
  return (pextra->rmcauses & (1 << rmcause));
}

/************************************************************************//**
  Is there extra of the given type cardinally near tile?
  (Does not check ptile itself.)
****************************************************************************/
bool is_extra_card_near(const struct tile *ptile, const struct extra_type *pextra)
{
  cardinal_adjc_iterate(&(wld.map), ptile, adjc_tile) {
    if (tile_has_extra(adjc_tile, pextra)) {
      return TRUE;
    }
  } cardinal_adjc_iterate_end;

  return FALSE;
}

/************************************************************************//**
  Is there extra of the given type near tile?
  (Does not check ptile itself.)
****************************************************************************/
bool is_extra_near_tile(const struct tile *ptile, const struct extra_type *pextra)
{
  adjc_iterate(&(wld.map), ptile, adjc_tile) {
    if (tile_has_extra(adjc_tile, pextra)) {
      return TRUE;
    }
  } adjc_iterate_end;

  return FALSE;
}

/************************************************************************//**
  Tells if extra can build to tile if all other requirements are met.
****************************************************************************/
bool extra_can_be_built(const struct extra_type *pextra,
                        const struct tile *ptile)
{
  if (!pextra->buildable) {
    /* Extra type not buildable */
    return FALSE;
  }

  if (tile_has_extra(ptile, pextra)) {
    /* Extra exist already */
    return FALSE;
  }

  return TRUE;
}

/************************************************************************//**
  Tells if player can build extra to tile with suitable unit.
****************************************************************************/
bool can_build_extra_base(const struct extra_type *pextra,
                          const struct player *pplayer,
                          const struct tile *ptile)
{
  if (!extra_can_be_built(pextra, ptile)) {
    return FALSE;
  }

  if (is_extra_caused_by(pextra, EC_BASE)) {
    if (tile_terrain(ptile)->base_time == 0) {
      return FALSE;
    }
    if (tile_city(ptile) != NULL && extra_base_get(pextra)->border_sq >= 0) {
      return FALSE;
    }
  }

  if (is_extra_caused_by(pextra, EC_ROAD)
      && tile_terrain(ptile)->road_time == 0) {
    return FALSE;
  }

  if (pplayer != NULL && !player_knows_techs_with_flag(pplayer, TF_BRIDGE)) {
    /* Player does not know technology to bridge over extras that require it. */
    extra_type_list_iterate(pextra->bridged, pbridged) {
      if (tile_has_extra(ptile, pbridged)) {
        /* Tile has extra that would require bridging over. */
        return FALSE;
      }
    } extra_type_list_iterate_end;
  }

  return TRUE;
}

/************************************************************************//**
  Tells if player can build extra to tile with suitable unit.
****************************************************************************/
bool player_can_build_extra(const struct extra_type *pextra,
                            const struct player *pplayer,
                            const struct tile *ptile)
{
  if (!can_build_extra_base(pextra, pplayer, ptile)) {
    return FALSE;
  }

  return are_reqs_active(pplayer, tile_owner(ptile), NULL, NULL, ptile,
                         NULL, NULL, NULL, NULL, NULL, &pextra->reqs,
                         RPT_POSSIBLE);
}

/************************************************************************//**
  Tells if unit can build extra on tile.
****************************************************************************/
bool can_build_extra(const struct extra_type *pextra,
                     const struct unit *punit,
                     const struct tile *ptile)
{
  struct player *pplayer = unit_owner(punit);

  if (!can_build_extra_base(pextra, pplayer, ptile)) {
    return FALSE;
  }

  return are_reqs_active(pplayer, tile_owner(ptile), NULL, NULL, ptile,
                         punit, unit_type_get(punit), NULL, NULL, NULL, &pextra->reqs,
                         RPT_CERTAIN);
}

/************************************************************************//**
  Is it possible at all to remove this extra now
****************************************************************************/
static bool can_extra_be_removed(const struct extra_type *pextra,
                                 const struct tile *ptile)
{
  struct city *pcity = tile_city(ptile);

  /* Cannot remove EF_ALWAYS_ON_CITY_CENTER extras from city center. */
  if (pcity != NULL) {
    if (extra_has_flag(pextra, EF_ALWAYS_ON_CITY_CENTER)) {
      return FALSE;
    }
    if (extra_has_flag(pextra, EF_AUTO_ON_CITY_CENTER)) {
      struct tile *vtile = tile_virtual_new(ptile);

      /* Would extra get rebuilt if removed */ 
      tile_remove_extra(vtile, pextra);
      if (player_can_build_extra(pextra, city_owner(pcity), vtile)) {
        /* No need to worry about conflicting extras - extra would had
         * not been here if conflicting one is. */
        tile_virtual_destroy(vtile);

        return FALSE;
      }

      tile_virtual_destroy(vtile);
    }
  }

  return TRUE;
}

/************************************************************************//**
  Tells if player can remove extra from tile with suitable unit.
****************************************************************************/
bool player_can_remove_extra(const struct extra_type *pextra,
                             const struct player *pplayer,
                             const struct tile *ptile)
{
  if (!can_extra_be_removed(pextra, ptile)) {
    return FALSE;
  }

  return are_reqs_active(pplayer, tile_owner(ptile), NULL, NULL, ptile,
                         NULL, NULL, NULL, NULL, NULL, &pextra->rmreqs,
                         RPT_POSSIBLE);
}

/************************************************************************//**
  Tells if unit can remove extra from tile.
****************************************************************************/
bool can_remove_extra(struct extra_type *pextra,
                      const struct unit *punit,
                      const struct tile *ptile)
{
  struct player *pplayer;

  if (!can_extra_be_removed(pextra, ptile)) {
    return FALSE;
  } 

  pplayer = unit_owner(punit);

  return are_reqs_active(pplayer, tile_owner(ptile), NULL, NULL, ptile,
                         punit, unit_type_get(punit), NULL, NULL, NULL,
                         &pextra->rmreqs, RPT_CERTAIN);
}

/************************************************************************//**
  Is tile native to extra?
****************************************************************************/
bool is_native_tile_to_extra(const struct extra_type *pextra,
                             const struct tile *ptile)
{
  struct terrain *pterr = tile_terrain(ptile);

  if (terrain_has_resource(pterr, pextra)) {
    return TRUE;
  }

  if (is_extra_caused_by(pextra, EC_IRRIGATION)
      && pterr->irrigation_result != pterr) {
    return FALSE;
  }

  if (is_extra_caused_by(pextra, EC_MINE)
      && pterr->mining_result != pterr) {
    return FALSE;
  }

  if (is_extra_caused_by(pextra, EC_BASE)) {
    if (pterr->base_time == 0) {
      return FALSE;
    }
    if (tile_city(ptile) != NULL && extra_base_get(pextra)->border_sq >= 0) {
      return FALSE;
    }
  }

  if (is_extra_caused_by(pextra, EC_ROAD)) {
    struct road_type *proad = extra_road_get(pextra);

    if (road_has_flag(proad, RF_RIVER)) {
      if (!terrain_has_flag(pterr, TER_CAN_HAVE_RIVER)) {
        return FALSE;
      }
    } else if (pterr->road_time == 0) {
      return FALSE;
    }
  }

  return are_reqs_active(NULL, NULL, NULL, NULL, ptile,
                         NULL, NULL, NULL, NULL, NULL,
                         &pextra->reqs, RPT_POSSIBLE);
}

/************************************************************************//**
  Returns TRUE iff an extra that conflicts with pextra exists at ptile.
****************************************************************************/
bool extra_conflicting_on_tile(const struct extra_type *pextra,
                               const struct tile *ptile)
{
  extra_type_iterate(old_extra) {
    if (tile_has_extra(ptile, old_extra)
        && !can_extras_coexist(old_extra, pextra)) {
      return TRUE;
    }
  } extra_type_iterate_end;

  return FALSE;
}

/************************************************************************//**
  Returns next extra by cause that unit or player can build to tile.
****************************************************************************/
struct extra_type *next_extra_for_tile(const struct tile *ptile, enum extra_cause cause,
                                       const struct player *pplayer,
                                       const struct unit *punit)
{
  if (cause == EC_IRRIGATION) {
    struct terrain *pterrain = tile_terrain(ptile);

    if (pterrain->irrigation_result != pterrain) {
      /* No extra can be created by irrigation the tile */
      return NULL;
    }
  }
  if (cause == EC_MINE) {
    struct terrain *pterrain = tile_terrain(ptile);

    if (pterrain->mining_result != pterrain) {
      /* No extra can be created by mining the tile */
      return NULL;
    }
  }

  extra_type_by_cause_iterate(cause, pextra) {
    if (!tile_has_extra(ptile, pextra)) {
      if (punit != NULL) {
        if (can_build_extra(pextra, punit, ptile)) {
          return pextra;
        }
      } else {
        /* punit is certainly NULL, pplayer can be too */
        if (player_can_build_extra(pextra, pplayer, ptile)) {
          return pextra;
        }
      }
    }
  } extra_type_by_cause_iterate_end;

  return NULL;
}

/************************************************************************//**
  Returns prev extra by cause that unit or player can remove from tile.
****************************************************************************/
struct extra_type *prev_extra_in_tile(const struct tile *ptile,
                                      enum extra_rmcause rmcause,
                                      const struct player *pplayer,
                                      const struct unit *punit)
{
  fc_assert(punit != NULL || pplayer != NULL);

  extra_type_by_rmcause_iterate(rmcause, pextra) {
    if (tile_has_extra(ptile, pextra)) {
      if (punit != NULL) {
        if (can_remove_extra(pextra, punit, ptile)) {
          return pextra;
        }
      } else {
        if (player_can_remove_extra(pextra, pplayer, ptile)) {
          return pextra;
        }
      }
    }
  } extra_type_by_rmcause_iterate_end;

  return NULL;
}

/************************************************************************//**
  Is extra native to unit class?
****************************************************************************/
bool is_native_extra_to_uclass(const struct extra_type *pextra,
                               const struct unit_class *pclass)
{
  return BV_ISSET(pextra->native_to, uclass_index(pclass));
}

/************************************************************************//**
  Is extra native to unit type?
****************************************************************************/
bool is_native_extra_to_utype(const struct extra_type *pextra,
                              const struct unit_type *punittype)
{
  return is_native_extra_to_uclass(pextra, utype_class(punittype));
}

/************************************************************************//**
  Check if extra has given flag
****************************************************************************/
bool extra_has_flag(const struct extra_type *pextra, enum extra_flag_id flag)
{
  return BV_ISSET(pextra->flags, flag);
}

/************************************************************************//**
  Returns TRUE iff any cardinally adjacent tile contains an extra with
  the given flag (does not check ptile itself).
****************************************************************************/
bool is_extra_flag_card_near(const struct tile *ptile, enum extra_flag_id flag)
{
  extra_type_iterate(pextra) {
    if (extra_has_flag(pextra, flag)) {
      cardinal_adjc_iterate(&(wld.map), ptile, adjc_tile) {
        if (tile_has_extra(adjc_tile, pextra)) {
          return TRUE;
        }
      } cardinal_adjc_iterate_end;
    }
  } extra_type_iterate_end;

  return FALSE;
}

/************************************************************************//**
  Returns TRUE iff any adjacent tile contains an extra with the given flag
  (does not check ptile itself).
****************************************************************************/
bool is_extra_flag_near_tile(const struct tile *ptile, enum extra_flag_id flag)
{
  extra_type_iterate(pextra) {
    if (extra_has_flag(pextra, flag)) {
      adjc_iterate(&(wld.map), ptile, adjc_tile) {
        if (tile_has_extra(adjc_tile, pextra)) {
          return TRUE;
        }
      } adjc_iterate_end;
    }
  } extra_type_iterate_end;

  return FALSE;
}

/************************************************************************//**
  Initialize user extra flags.
****************************************************************************/
void user_extra_flags_init(void)
{
  int i;

  for (i = 0; i < MAX_NUM_USER_EXTRA_FLAGS; i++) {
    user_flag_init(&user_extra_flags[i]);
  }
}

/************************************************************************//**
  Frees the memory associated with all extra flags
****************************************************************************/
void extra_flags_free(void)
{
  int i;

  for (i = 0; i < MAX_NUM_USER_EXTRA_FLAGS; i++) {
    user_flag_free(&user_extra_flags[i]);
  }
}

/************************************************************************//**
  Sets user defined name for extra flag.
****************************************************************************/
void set_user_extra_flag_name(enum extra_flag_id id, const char *name,
                              const char *helptxt)
{
  int efid = id - EF_USER_FLAG_1;

  fc_assert_ret(id >= EF_USER_FLAG_1 && id <= EF_LAST_USER_FLAG);

  if (user_extra_flags[efid].name != NULL) {
    FC_FREE(user_extra_flags[efid].name);
    user_extra_flags[efid].name = NULL;
  }

  if (name && name[0] != '\0') {
    user_extra_flags[efid].name = fc_strdup(name);
  }

  if (user_extra_flags[efid].helptxt != NULL) {
    free(user_extra_flags[efid].helptxt);
    user_extra_flags[efid].helptxt = NULL;
  }

  if (helptxt && helptxt[0] != '\0') {
    user_extra_flags[efid].helptxt = fc_strdup(helptxt);
  }
}

/************************************************************************//**
  Extra flag name callback, called from specenum code.
****************************************************************************/
const char *extra_flag_id_name_cb(enum extra_flag_id flag)
{
  if (flag < EF_USER_FLAG_1 || flag > EF_LAST_USER_FLAG) {
    return NULL;
  }

  return user_extra_flags[flag - EF_USER_FLAG_1].name;
}

/************************************************************************//**
  Return the (untranslated) help text of the user extra flag.
****************************************************************************/
const char *extra_flag_helptxt(enum extra_flag_id id)
{
  fc_assert(id >= EF_USER_FLAG_1 && id <= EF_LAST_USER_FLAG);

  return user_extra_flags[id - EF_USER_FLAG_1].helptxt;
}

/************************************************************************//**
  Can two extras coexist in same tile?
****************************************************************************/
bool can_extras_coexist(const struct extra_type *pextra1,
                        const struct extra_type *pextra2)
{
  if (pextra1 == pextra2) {
    return TRUE;
  }

  return !BV_ISSET(pextra1->conflicts, extra_index(pextra2));
}

/************************************************************************//**
  Does the extra count toward environment upset?
****************************************************************************/
bool extra_causes_env_upset(struct extra_type *pextra,
                            enum environment_upset_type upset)
{
  switch (upset) {
  case EUT_GLOBAL_WARMING:
    return extra_has_flag(pextra, EF_GLOBAL_WARMING);
  case EUT_NUCLEAR_WINTER:
    return extra_has_flag(pextra, EF_NUCLEAR_WINTER);
  }

  return FALSE;
}

/************************************************************************//**
  Is the extra caused by some kind of worker action?
****************************************************************************/
bool is_extra_caused_by_worker_action(const struct extra_type *pextra)
{
  /* Is any of the worker build action bits set? */
  return (pextra->causes
          & (1 << EC_IRRIGATION
             | 1 << EC_MINE
             | 1 << EC_BASE
             | 1 << EC_ROAD));
}

/************************************************************************//**
  Is the extra removed by some kind of worker action?
****************************************************************************/
bool is_extra_removed_by_worker_action(const struct extra_type *pextra)
{
  /* Is any of the worker remove action bits set? */
  return (pextra->rmcauses
          & (1 << ERM_CLEANPOLLUTION
             | 1 << ERM_CLEANFALLOUT
             | 1 << ERM_PILLAGE));
}

/************************************************************************//**
  Is the extra caused by specific worker action?
****************************************************************************/
bool is_extra_caused_by_action(const struct extra_type *pextra,
                               enum unit_activity act)
{
  return is_extra_caused_by(pextra, activity_to_extra_cause(act));
}

/************************************************************************//**
  Is the extra removed by specific worker action?
****************************************************************************/
bool is_extra_removed_by_action(const struct extra_type *pextra,
                                enum unit_activity act)
{
  return is_extra_removed_by(pextra, activity_to_extra_rmcause(act));
}

/************************************************************************//**
  What extra cause activity is considered to be?
****************************************************************************/
enum extra_cause activity_to_extra_cause(enum unit_activity act)
{
  switch(act) {
  case ACTIVITY_IRRIGATE:
    return EC_IRRIGATION;
  case ACTIVITY_MINE:
    return EC_MINE;
  case ACTIVITY_BASE:
    return EC_BASE;
  case ACTIVITY_GEN_ROAD:
    return EC_ROAD;
  default:
    break;
  }

  return EC_NONE;
}

/************************************************************************//**
  What extra rmcause activity is considered to be?
****************************************************************************/
enum extra_rmcause activity_to_extra_rmcause(enum unit_activity act)
{
  switch(act) {
  case ACTIVITY_PILLAGE:
    return ERM_PILLAGE;
  case ACTIVITY_POLLUTION:
    return ERM_CLEANPOLLUTION;
  case ACTIVITY_FALLOUT:
    return ERM_CLEANFALLOUT;
  default:
    break;
  }

  return ERM_NONE;
}

/************************************************************************//**
  Who owns extras on tile
****************************************************************************/
struct player *extra_owner(const struct tile *ptile)
{
  return ptile->extras_owner;
}

/************************************************************************//**
  Are all the requirements for extra to appear on tile fulfilled.
****************************************************************************/
bool can_extra_appear(const struct extra_type *pextra, const struct tile *ptile)
{
  return !tile_has_extra(ptile, pextra)
    && is_extra_caused_by(pextra, EC_APPEARANCE)
    && is_native_tile_to_extra(pextra, ptile)
    && !extra_conflicting_on_tile(pextra, ptile)
    && are_reqs_active(NULL, tile_owner(ptile), NULL, NULL, ptile,
                       NULL, NULL, NULL, NULL, NULL,
                       &pextra->appearance_reqs, RPT_CERTAIN);
}

/************************************************************************//**
  Are all the requirements for extra to disappear from tile fulfilled.
****************************************************************************/
bool can_extra_disappear(const struct extra_type *pextra, const struct tile *ptile)
{
  return tile_has_extra(ptile, pextra)
    && is_extra_removed_by(pextra, ERM_DISAPPEARANCE)
    && can_extra_be_removed(pextra, ptile)
    && are_reqs_active(NULL, tile_owner(ptile), NULL, NULL, ptile,
                       NULL, NULL, NULL, NULL, NULL,
                       &pextra->disappearance_reqs, RPT_CERTAIN);
}

/************************************************************************//**
  Extra is not hidden from the user.
****************************************************************************/
bool player_knows_extra_exist(const struct player *pplayer,
                              const struct extra_type *pextra,
                              const struct tile *ptile)
{
  if (!tile_has_extra(ptile, pextra)) {
    return FALSE;
  }

  return research_invention_state(research_get(pplayer),
                                  pextra->visibility_req) == TECH_KNOWN;
}
