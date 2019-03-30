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

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

/* common */
#include "city.h"
#include "game.h"
#include "map.h"
#include "player.h"
#include "tile.h"

/* server */
#include "maphand.h"

/* server/advisors */
#include "advbuilding.h"
#include "autosettlers.h"

#include "infracache.h"

/* cache activities within the city map */
struct worker_activity_cache {
  int act[ACTIVITY_LAST];
  int extra[MAX_EXTRA_TYPES];
  int rmextra[MAX_EXTRA_TYPES];
};

static int adv_calc_irrigate_transform(const struct city *pcity,
                                       const struct tile *ptile);
static int adv_calc_mine_transform(const struct city *pcity, const struct tile *ptile);
static int adv_calc_transform(const struct city *pcity,
                              const struct tile *ptile);
static int adv_calc_extra(const struct city *pcity, const struct tile *ptile,
                          const struct extra_type *pextra);
static int adv_calc_rmextra(const struct city *pcity, const struct tile *ptile,
                            const struct extra_type *pextra);

/**********************************************************************//**
  Calculate the benefit of irrigating the given tile.

  The return value is the goodness of the tile after the irrigation.  This
  should be compared to the goodness of the tile currently (see
  city_tile_value(); note that this depends on the AI's weighting
  values).
**************************************************************************/
static int adv_calc_irrigate_transform(const struct city *pcity,
                                       const struct tile *ptile)
{
  int goodness;
  struct terrain *old_terrain, *new_terrain;

  fc_assert_ret_val(ptile != NULL, -1);

  old_terrain = tile_terrain(ptile);
  new_terrain = old_terrain->irrigation_result;

  if (new_terrain != old_terrain && new_terrain != T_NONE) {
    struct tile *vtile;

    if (tile_city(ptile) && terrain_has_flag(new_terrain, TER_NO_CITIES)) {
      /* Not a valid activity. */
      return -1;
    }
    /* Irrigation would change the terrain type, clearing conflicting
     * extras in the process.  Calculate the benefit of doing so. */
    vtile = tile_virtual_new(ptile);

    tile_change_terrain(vtile, new_terrain);
    goodness = city_tile_value(pcity, vtile, 0, 0);
    tile_virtual_destroy(vtile);

    return goodness;
  } else {
    return -1;
  }
}

/**********************************************************************//**
  Calculate the benefit of mining the given tile.

  The return value is the goodness of the tile after the mining.  This
  should be compared to the goodness of the tile currently (see
  city_tile_value(); note that this depends on the AI's weighting
  values).
**************************************************************************/
static int adv_calc_mine_transform(const struct city *pcity, const struct tile *ptile)
{
  int goodness;
  struct terrain *old_terrain, *new_terrain;

  fc_assert_ret_val(ptile != NULL, -1);

  old_terrain = tile_terrain(ptile);
  new_terrain = old_terrain->mining_result;

  if (old_terrain != new_terrain && new_terrain != T_NONE) {
    struct tile *vtile;

    if (tile_city(ptile) && terrain_has_flag(new_terrain, TER_NO_CITIES)) {
      /* Not a valid activity. */
      return -1;
    }
    /* Mining would change the terrain type, clearing conflicting
     * extras in the process.  Calculate the benefit of doing so. */
    vtile = tile_virtual_new(ptile);

    tile_change_terrain(vtile, new_terrain);
    goodness = city_tile_value(pcity, vtile, 0, 0);
    tile_virtual_destroy(vtile);

    return goodness;
  } else {
    return -1;
  }
}

/**********************************************************************//**
  Calculate the benefit of transforming the given tile.

  The return value is the goodness of the tile after the transform.  This
  should be compared to the goodness of the tile currently (see
  city_tile_value(); note that this depends on the AI's weighting
  values).
**************************************************************************/
static int adv_calc_transform(const struct city *pcity,
                             const struct tile *ptile)
{
  int goodness;
  struct tile *vtile;
  struct terrain *old_terrain, *new_terrain;

  fc_assert_ret_val(ptile != NULL, -1);

  old_terrain = tile_terrain(ptile);
  new_terrain = old_terrain->transform_result;

  if (old_terrain == new_terrain || new_terrain == T_NONE) {
    return -1;
  }

  if (!terrain_surroundings_allow_change(ptile, new_terrain)) {
    /* Can't do this terrain conversion here. */
    return -1;
  }

  if (tile_city(ptile) && terrain_has_flag(new_terrain, TER_NO_CITIES)) {
    return -1;
  }

  vtile = tile_virtual_new(ptile);
  tile_change_terrain(vtile, new_terrain);
  goodness = city_tile_value(pcity, vtile, 0, 0);
  tile_virtual_destroy(vtile);

  return goodness;
}

/**********************************************************************//**
  Calculate the benefit of building an extra at the given tile.

  The return value is the goodness of the tile after the extra is built.
  This should be compared to the goodness of the tile currently (see
  city_tile_value(); note that this depends on the AI's weighting
  values).

  This function does not calculate the benefit of being able to quickly
  move units (i.e., of connecting the civilization).  See road_bonus() for
  that calculation.
**************************************************************************/
static int adv_calc_extra(const struct city *pcity, const struct tile *ptile,
                          const struct extra_type *pextra)
{
  int goodness = -1;

  fc_assert_ret_val(ptile != NULL, -1);

  if (player_can_build_extra(pextra, city_owner(pcity), ptile)) {
    struct tile *vtile = tile_virtual_new(ptile);

    tile_add_extra(vtile, pextra);

    extra_type_iterate(cextra) {
      if (tile_has_extra(vtile, cextra)
          && !can_extras_coexist(pextra, cextra)) {
        tile_remove_extra(vtile, cextra);
      }
    } extra_type_iterate_end;

    goodness = city_tile_value(pcity, vtile, 0, 0);
    tile_virtual_destroy(vtile);
  }

  return goodness;
}

/**********************************************************************//**
  Calculate the benefit of removing an extra from the given tile.

  The return value is the goodness of the tile after the extra is removed.
  This should be compared to the goodness of the tile currently (see
  city_tile_value(); note that this depends on the AI's weighting
  values).
**************************************************************************/
static int adv_calc_rmextra(const struct city *pcity, const struct tile *ptile,
                            const struct extra_type *pextra)
{
  int goodness = -1;

  fc_assert_ret_val(ptile != NULL, -1);

  if (player_can_remove_extra(pextra, city_owner(pcity), ptile)) {
    struct tile *vtile = tile_virtual_new(ptile);

    tile_remove_extra(vtile, pextra);

    goodness = city_tile_value(pcity, vtile, 0, 0);
    tile_virtual_destroy(vtile);
  }

  return goodness;
}

/**********************************************************************//**
  Do all tile improvement calculations and cache them for later.

  These values are used in settler_evaluate_improvements() so this function
  must be called before doing that.  Currently this is only done when handling
  auto-settlers or when the AI contemplates building worker units.
**************************************************************************/
void initialize_infrastructure_cache(struct player *pplayer)
{
  city_list_iterate(pplayer->cities, pcity) {
    struct tile *pcenter = city_tile(pcity);
    int radius_sq = city_map_radius_sq_get(pcity);

    city_map_iterate(radius_sq, city_index, city_x, city_y) {
      as_transform_action_iterate(act) {
        adv_city_worker_act_set(pcity, city_index, action_id_get_activity(act),
                                -1);
      } as_transform_action_iterate_end;
    } city_map_iterate_end;

    city_tile_iterate_index(radius_sq, pcenter, ptile, cindex) {
      adv_city_worker_act_set(pcity, cindex, ACTIVITY_MINE,
                              adv_calc_mine_transform(pcity, ptile));
      adv_city_worker_act_set(pcity, cindex, ACTIVITY_IRRIGATE,
                              adv_calc_irrigate_transform(pcity, ptile));
      adv_city_worker_act_set(pcity, cindex, ACTIVITY_TRANSFORM,
                              adv_calc_transform(pcity, ptile));

      /* road_bonus() is handled dynamically later; it takes into
       * account settlers that have already been assigned to building
       * roads this turn. */
      extra_type_iterate(pextra) {
        /* We have no use for extra value, if workers cannot be assigned
         * to build it, so don't use time to calculate values otherwise */
        if (pextra->buildable
            && is_extra_caused_by_worker_action(pextra)) {
          adv_city_worker_extra_set(pcity, cindex, pextra,
                                    adv_calc_extra(pcity, ptile, pextra));
        } else {
          adv_city_worker_extra_set(pcity, cindex, pextra, 0);
        }
        if (tile_has_extra(ptile, pextra) && is_extra_removed_by_worker_action(pextra)) {
          adv_city_worker_rmextra_set(pcity, cindex, pextra,
                                      adv_calc_rmextra(pcity, ptile, pextra));
        } else {
          adv_city_worker_rmextra_set(pcity, cindex, pextra, 0);
        }
      } extra_type_iterate_end;
    } city_tile_iterate_index_end;
  } city_list_iterate_end;
}

/**********************************************************************//**
  Returns a measure of goodness of a tile to pcity.

  FIXME: foodneed and prodneed are always 0.
**************************************************************************/
int city_tile_value(const struct city *pcity, const struct tile *ptile,
                    int foodneed, int prodneed)
{
  int food = city_tile_output_now(pcity, ptile, O_FOOD);
  int shield = city_tile_output_now(pcity, ptile, O_SHIELD);
  int trade = city_tile_output_now(pcity, ptile, O_TRADE);
  int value = 0;

  /* Each food, trade, and shield gets a certain weighting.  We also benefit
   * tiles that have at least one of an item - this promotes balance and 
   * also accounts for INC_TILE effects. */
  value += food * FOOD_WEIGHTING;
  if (food > 0) {
    value += FOOD_WEIGHTING / 2;
  }
  value += shield * SHIELD_WEIGHTING;
  if (shield > 0) {
    value += SHIELD_WEIGHTING / 2;
  }
  value += trade * TRADE_WEIGHTING;
  if (trade > 0) {
    value += TRADE_WEIGHTING / 2;
  }

  return value;
}

/**********************************************************************//**
  Set the value for activity 'doing' on tile 'city_tile_index' of
  city 'pcity'.
**************************************************************************/
void adv_city_worker_act_set(struct city *pcity, int city_tile_index,
                             enum unit_activity act_id, int value)
{
  if (pcity->server.adv->act_cache_radius_sq
      != city_map_radius_sq_get(pcity)) {
    log_debug("update activity cache for %s: radius_sq changed from "
              "%d to %d", city_name_get(pcity),
              pcity->server.adv->act_cache_radius_sq,
              city_map_radius_sq_get(pcity));
    adv_city_update(pcity);
  }

  fc_assert_ret(NULL != pcity);
  fc_assert_ret(NULL != pcity->server.adv);
  fc_assert_ret(NULL != pcity->server.adv->act_cache);
  fc_assert_ret(pcity->server.adv->act_cache_radius_sq
                == city_map_radius_sq_get(pcity));
  fc_assert_ret(city_tile_index < city_map_tiles_from_city(pcity));

  (pcity->server.adv->act_cache[city_tile_index]).act[act_id] = value;
}

/**********************************************************************//**
  Return the value for activity 'doing' on tile 'city_tile_index' of
  city 'pcity'.
**************************************************************************/
int adv_city_worker_act_get(const struct city *pcity, int city_tile_index,
                            enum unit_activity act_id)
{
  fc_assert_ret_val(NULL != pcity, 0);
  fc_assert_ret_val(NULL != pcity->server.adv, 0);
  fc_assert_ret_val(NULL != pcity->server.adv->act_cache, 0);
  fc_assert_ret_val(pcity->server.adv->act_cache_radius_sq
                     == city_map_radius_sq_get(pcity), 0);
  fc_assert_ret_val(city_tile_index < city_map_tiles_from_city(pcity), 0);

  return (pcity->server.adv->act_cache[city_tile_index]).act[act_id];
}

/**********************************************************************//**
  Set the value for extra on tile 'city_tile_index' of
  city 'pcity'.
**************************************************************************/
void adv_city_worker_extra_set(struct city *pcity, int city_tile_index,
                               const struct extra_type *pextra, int value)
{
  if (pcity->server.adv->act_cache_radius_sq
      != city_map_radius_sq_get(pcity)) {
    log_debug("update activity cache for %s: radius_sq changed from "
              "%d to %d", city_name_get(pcity),
              pcity->server.adv->act_cache_radius_sq,
              city_map_radius_sq_get(pcity));
    adv_city_update(pcity);
  }

  fc_assert_ret(NULL != pcity);
  fc_assert_ret(NULL != pcity->server.adv);
  fc_assert_ret(NULL != pcity->server.adv->act_cache);
  fc_assert_ret(pcity->server.adv->act_cache_radius_sq
                == city_map_radius_sq_get(pcity));
  fc_assert_ret(city_tile_index < city_map_tiles_from_city(pcity));

  (pcity->server.adv->act_cache[city_tile_index]).extra[extra_index(pextra)] = value;
}

/**********************************************************************//**
  Set the value for extra removal on tile 'city_tile_index' of
  city 'pcity'.
**************************************************************************/
void adv_city_worker_rmextra_set(struct city *pcity, int city_tile_index,
                                 const struct extra_type *pextra, int value)
{
  if (pcity->server.adv->act_cache_radius_sq
      != city_map_radius_sq_get(pcity)) {
    log_debug("update activity cache for %s: radius_sq changed from "
              "%d to %d", city_name_get(pcity),
              pcity->server.adv->act_cache_radius_sq,
              city_map_radius_sq_get(pcity));
    adv_city_update(pcity);
  }

  fc_assert_ret(NULL != pcity);
  fc_assert_ret(NULL != pcity->server.adv);
  fc_assert_ret(NULL != pcity->server.adv->act_cache);
  fc_assert_ret(pcity->server.adv->act_cache_radius_sq
                == city_map_radius_sq_get(pcity));
  fc_assert_ret(city_tile_index < city_map_tiles_from_city(pcity));

  (pcity->server.adv->act_cache[city_tile_index]).rmextra[extra_index(pextra)] = value;
}

/**********************************************************************//**
  Return the value for extra on tile 'city_tile_index' of
  city 'pcity'.
**************************************************************************/
int adv_city_worker_extra_get(const struct city *pcity, int city_tile_index,
                              const struct extra_type *pextra)
{
  fc_assert_ret_val(NULL != pcity, 0);
  fc_assert_ret_val(NULL != pcity->server.adv, 0);
  fc_assert_ret_val(NULL != pcity->server.adv->act_cache, 0);
  fc_assert_ret_val(pcity->server.adv->act_cache_radius_sq
                     == city_map_radius_sq_get(pcity), 0);
  fc_assert_ret_val(city_tile_index < city_map_tiles_from_city(pcity), 0);

  return (pcity->server.adv->act_cache[city_tile_index]).extra[extra_index(pextra)];
}

/**********************************************************************//**
  Return the value for extra removal on tile 'city_tile_index' of
  city 'pcity'.
**************************************************************************/
int adv_city_worker_rmextra_get(const struct city *pcity, int city_tile_index,
                                const struct extra_type *pextra)
{
  fc_assert_ret_val(NULL != pcity, 0);
  fc_assert_ret_val(NULL != pcity->server.adv, 0);
  fc_assert_ret_val(NULL != pcity->server.adv->act_cache, 0);
  fc_assert_ret_val(pcity->server.adv->act_cache_radius_sq
                     == city_map_radius_sq_get(pcity), 0);
  fc_assert_ret_val(city_tile_index < city_map_tiles_from_city(pcity), 0);

  return (pcity->server.adv->act_cache[city_tile_index]).rmextra[extra_index(pextra)];
}

/**********************************************************************//**
  Update the memory allocated for AI city handling.
**************************************************************************/
void adv_city_update(struct city *pcity)
{
  int radius_sq = city_map_radius_sq_get(pcity);

  fc_assert_ret(NULL != pcity);
  fc_assert_ret(NULL != pcity->server.adv);

  /* initialize act_cache if needed */
  if (pcity->server.adv->act_cache == NULL
      || pcity->server.adv->act_cache_radius_sq == -1
      || pcity->server.adv->act_cache_radius_sq != radius_sq) {
    pcity->server.adv->act_cache
      = fc_realloc(pcity->server.adv->act_cache,
                   city_map_tiles(radius_sq)
                   * sizeof(*(pcity->server.adv->act_cache)));
    /* initialize with 0 */
    memset(pcity->server.adv->act_cache, 0,
           city_map_tiles(radius_sq)
           * sizeof(*(pcity->server.adv->act_cache)));
    pcity->server.adv->act_cache_radius_sq = radius_sq;
  }
}

/**********************************************************************//**
  Allocate advisors related city data
**************************************************************************/
void adv_city_alloc(struct city *pcity)
{
  pcity->server.adv = fc_calloc(1, sizeof(*pcity->server.adv));

  pcity->server.adv->act_cache = NULL;
  pcity->server.adv->act_cache_radius_sq = -1;
  /* allocate memory for pcity->ai->act_cache */
  adv_city_update(pcity);
}

/**********************************************************************//**
  Free advisors related city data
**************************************************************************/
void adv_city_free(struct city *pcity)
{
  fc_assert_ret(NULL != pcity);

  if (pcity->server.adv) {
    if (pcity->server.adv->act_cache) {
      FC_FREE(pcity->server.adv->act_cache);
    }
    FC_FREE(pcity->server.adv);
  }
}
