/***********************************************************************
 Freeciv - Copyright (C) 2004 - The Freeciv Team
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

#include <stdio.h>
#include <string.h>

/* utility */
#include "mem.h"
#include "log.h"
#include "support.h" 
#include "timing.h"

/* common */
#include "city.h"
#include "game.h"
#include "government.h"
#include "map.h"
#include "movement.h"
#include "packets.h"
#include "player.h"

/* common/aicore */
#include "pf_tools.h"

/* server */
#include "citytools.h"
#include "maphand.h"
#include "srv_log.h"
#include "unithand.h"
#include "unittools.h"

/* server/advisors */
#include "advdata.h"
#include "advgoto.h"
#include "advtools.h"
#include "autosettlers.h"
#include "infracache.h"

/* ai */
#include "handicaps.h"

/* ai/default */
#include "aidata.h"
#include "aicity.h"
#include "aiferry.h"
#include "ailog.h"
#include "aiplayer.h"
#include "aitools.h"
#include "aiunit.h"
#include "citymap.h"

#include "aisettler.h"


/* COMMENTS */
/* 
   This code tries hard to do the right thing, including looking
   into the future (wrt to government), and also doing this in a
   modpack friendly manner. However, there are some pieces missing.

   A tighter integration into the city management code would 
   give more optimal city placements, since existing cities could
   move their workers around to give a new city better placement.
   Occasionally you will see cities being placed sub-optimally
   because the best city center tile is taken when another tile
   could have been worked instead by the city that took it.

   The code is not generic enough. It assumes smallpox too much,
   and should calculate with a future city of a bigger size.

   We need to stop the build code from running this code all the
   time and instead try to complete what it has started.
*/

/* Stop looking too hard for better tiles to found a new city at when
 * settler finds location with at least RESULT_IS_ENOUGH want
 * points. See city_desirability() for how base want is computed
 * before amortizing it.
 *
 * This is a big WAG to save a big amount of CPU. */
#define RESULT_IS_ENOUGH 250

#define FERRY_TECH_WANT 500

#define GROWTH_PRIORITY 15

/* Perfection gives us an idea of how long to search for optimal
 * solutions, instead of going for quick and dirty solutions that
 * waste valuable time. Decrease for maps where good city placements
 * are hard to find. Lower means more perfection. */
#define PERFECTION 3

/* How much to deemphasise the potential for city growth for city
 * placements. Decrease this value if large cities are important
 * or planned. Increase if running strict smallpox. */
#define GROWTH_POTENTIAL_DEEMPHASIS 8

/* Percentage bonus to city locations near an ocean. */
#define NAVAL_EMPHASIS 20

/* Modifier for defense bonus that is applied to city location want. 
 * This is % of defense % to increase want by. */
#define DEFENSE_EMPHASIS 20

struct tile_data_cache {
  char food;    /* food output of the tile */
  char trade;   /* trade output of the tile */
  char shield;  /* shield output of the tile */

  int sum;      /* weighted sum of the tile output (used by AI) */

  int reserved; /* reservation for this tile; used by print_citymap() */

  int turn;     /* the turn the values were calculated */
};


struct tile_data_cache *tile_data_cache_new(void);
struct tile_data_cache *
  tile_data_cache_copy(const struct tile_data_cache *ptdc);
static void tile_data_cache_destroy(struct tile_data_cache *ptdc);

/* struct tdcache_hash. */
#define SPECHASH_TAG tile_data_cache
#define SPECHASH_INT_KEY_TYPE
#define SPECHASH_IDATA_TYPE struct tile_data_cache *
#define SPECHASH_IDATA_FREE tile_data_cache_destroy
#include "spechash.h"

struct ai_settler {
  struct tile_data_cache_hash *tdc_hash;

#ifdef FREECIV_DEBUG
  struct {
    int hit;
    int old;
    int miss;
    int save;
  } cache;
#endif /* FREECIV_DEBUG */
};

struct cityresult {
  struct tile *tile;
  int total;              /* total value of position */
  int result;             /* amortized and adjusted total value */
  int corruption, waste;
  bool overseas;          /* have to use boat to get there */
  bool virt_boat;         /* virtual boat was used in search, 
                           * so need to build one */

  struct {
    struct tile_data_cache *tdc;  /* values of city center; link to the data
                                   * in tdc_hash. */
  } city_center;

  struct {
    struct tile *tile;            /* best other tile */
    int cindex;                   /* city-relative index for other tile */
    struct tile_data_cache *tdc;  /* value of best other tile; link to the
                                   * data in tdc_hash. */
  } best_other;

  int remaining;          /* value of all other tiles */

  /* Save the result for print_citymap(). */
  struct tile_data_cache_hash *tdc_hash;

  int city_radius_sq;     /* current squared radius of the city */
};

static const struct tile_data_cache *tdc_plr_get(struct ai_type *ait,
                                                 struct player *plr,
                                                 int tindex);
static void tdc_plr_set(struct ai_type *ait, struct player *plr, int tindex,
                        const struct tile_data_cache *tdcache);

static struct cityresult *cityresult_new(struct tile *ptile);
static void cityresult_destroy(struct cityresult *result);

static struct cityresult *cityresult_fill(struct ai_type *ait,
                                          struct player *pplayer,
                                          struct tile *center);
static bool food_starvation(const struct cityresult *result);
static bool shield_starvation(const struct cityresult *result);
static int result_defense_bonus(struct player *pplayer,
                                const struct cityresult *result);
static int naval_bonus(const struct cityresult *result);
static void print_cityresult(struct player *pplayer,
                             const struct cityresult *cr);
struct cityresult *city_desirability(struct ai_type *ait,
                                     struct player *pplayer,
                                     struct unit *punit, struct tile *ptile);
static struct cityresult *settler_map_iterate(struct ai_type *ait,
                                              struct pf_parameter *parameter,
                                              struct unit *punit,
                                              int boat_cost);
static struct cityresult *find_best_city_placement(struct ai_type *ait,
                                                   struct unit *punit,
                                                   bool look_for_boat,
                                                   bool use_virt_boat);
static bool dai_do_build_city(struct ai_type *ait, struct player *pplayer,
                              struct unit *punit);

/*************************************************************************//**
  Allocated a city result.
*****************************************************************************/
static struct cityresult *cityresult_new(struct tile *ptile)
{
  struct cityresult *result;

  fc_assert_ret_val(ptile != NULL, NULL);

  result = fc_calloc(1, sizeof(*result));
  result->tile = ptile;
  result->total = 0;
  result->result = -666;
  result->corruption = 0;
  result->waste = 0;
  result->overseas = FALSE;
  result->virt_boat = FALSE;

  /* city centre */
  result->city_center.tdc = NULL;

  /* first worked tile */
  result->best_other.tile = NULL;
  result->best_other.tdc = NULL;
  result->best_other.cindex = 0;

  result->remaining = 0;
  result->tdc_hash = tile_data_cache_hash_new();
  result->city_radius_sq = game.info.init_city_radius_sq;

  return result;
}

/*************************************************************************//**
  Destroy a city result.
*****************************************************************************/
static void cityresult_destroy(struct cityresult *result)
{
  if (result != NULL) {
    if (result->tdc_hash != NULL) {
      tile_data_cache_hash_destroy(result->tdc_hash);
    }
    free(result);
  }
}

/*************************************************************************//**
  Fill cityresult struct with useful info about the city spot. It must
  contain valid x, y coordinates and total should be zero.

  We assume whatever best government we are aiming for.

  We always return valid other_x and other_y if total > 0.
*****************************************************************************/
static struct cityresult *cityresult_fill(struct ai_type *ait,
                                          struct player *pplayer,
                                          struct tile *center)
{
  struct city *pcity = tile_city(center);
  struct government *curr_govt = government_of_player(pplayer);
  struct player *saved_owner = NULL;
  struct tile *saved_claimer = NULL;
  bool virtual_city = FALSE;
  bool handicap = has_handicap(pplayer, H_MAP);
  struct adv_data *adv = adv_data_get(pplayer, NULL);
  struct ai_plr *ai = dai_plr_data_get(ait, pplayer, NULL);
  struct cityresult *result;

  fc_assert_ret_val(ai != NULL, NULL);
  fc_assert_ret_val(center != NULL, NULL);

  pplayer->government = adv->goal.govt.gov;

  /* Create a city result and set default values. */
  result = cityresult_new(center);

  if (!pcity) {
    pcity = create_city_virtual(pplayer, result->tile, "Virtuaville");
    saved_owner = tile_owner(result->tile);
    saved_claimer = tile_claimer(result->tile);
    tile_set_owner(result->tile, pplayer, result->tile); /* temporarily */
    city_choose_build_default(pcity);  /* ?? */
    virtual_city = TRUE;
  }

  result->city_radius_sq = city_map_radius_sq_get(pcity);

  city_tile_iterate_index(result->city_radius_sq, result->tile, ptile,
                          cindex) {
    int tindex = tile_index(ptile);
    int reserved = citymap_read(ptile);
    bool city_center = (result->tile == ptile); /*is_city_center()*/
    struct tile_data_cache *ptdc;

    if (reserved < 0
        || (handicap && !map_is_known(ptile, pplayer))
        || NULL != tile_worked(ptile)) {
      /* Tile is reserved or we can't see it */
      ptdc = tile_data_cache_new();
      ptdc->shield = 0;
      ptdc->trade = 0;
      ptdc->food = 0;
      ptdc->sum = -1;
      ptdc->reserved = reserved;
      /* ptdc->turn was set by tile_data_cache_new(). */
    } else {
      const struct tile_data_cache *ptdc_hit = tdc_plr_get(ait, pplayer, tindex);
      if (!ptdc_hit || city_center) {
        /* We cannot read city center from cache */
        ptdc = tile_data_cache_new();

        /* Food */
        ptdc->food = city_tile_output(pcity, ptile, FALSE, O_FOOD);
        /* Shields */
        ptdc->shield = city_tile_output(pcity, ptile, FALSE, O_SHIELD);
        /* Trade */
        ptdc->trade = city_tile_output(pcity, ptile, FALSE, O_TRADE);
        /* Weighted sum */
        ptdc->sum = ptdc->food * adv->food_priority
                    + ptdc->trade * adv->science_priority
                    + ptdc->shield * adv->shield_priority;
        /* Balance perfection */
        ptdc->sum *= PERFECTION / 2;
        if (ptdc->food >= 2) {
          ptdc->sum *= 2; /* we need this to grow */
        }

        if (!city_center && virtual_city) {
          /* real cities and any city center will give us spossibly
           * skewed results */
          tdc_plr_set(ait, pplayer, tindex, tile_data_cache_copy(ptdc));
        }
      } else {
        ptdc = tile_data_cache_copy(ptdc_hit);
      }
    }

    /* Save reservation status for debugging. */
    ptdc->reserved = reserved;

    /* Avoid crowdedness, except for city center. */
    if (ptdc->sum > 0) {
      ptdc->sum -= MIN(reserved * GROWTH_PRIORITY, ptdc->sum - 1);
    }

    /* Calculate city center and best other than city center */
    if (city_center) {
      /* Set city center. */
      result->city_center.tdc = ptdc;
    } else if (!result->best_other.tdc) {
      /* Set best other tile. */
      result->best_other.tdc = ptdc;
      result->best_other.tile = ptile;
      result->best_other.cindex = cindex;
    } else if (ptdc->sum > result->best_other.tdc->sum) {
      /* First add other other to remaining */
      result->remaining += result->best_other.tdc->sum
                           / GROWTH_POTENTIAL_DEEMPHASIS;
      /* Then make new best other */
      result->best_other.tdc = ptdc;
      result->best_other.tile = ptile;
      result->best_other.cindex = cindex;
    } else {
      /* Save total remaining calculation, divided by crowdedness
       * of the area and the emphasis placed on space for growth. */
      result->remaining += ptdc->sum / GROWTH_POTENTIAL_DEEMPHASIS;
    }

    tile_data_cache_hash_replace(result->tdc_hash, cindex, ptdc);
  } city_tile_iterate_index_end;

  /* We need a city center. */
  fc_assert_ret_val(result->city_center.tdc != NULL, NULL);

  if (virtual_city) {
    /* Baseline is a size one city (city center + best extra tile). */
    result->total = result->city_center.tdc->sum
                    + (result->best_other.tdc != NULL
                       ? result->best_other.tdc->sum : 0);
  } else if (result->best_other.tdc != NULL) {
    /* Baseline is best extra tile only. This is why making new cities 
     * is so darn good. */
    result->total = result->best_other.tdc->sum;
  } else {
    /* There is no available tile in this city. All is worked. */
    result->total = 0;
    return result;
  }

  /* Now we have a valid city center as well as best other tile. */

  if (virtual_city) {
    /* Corruption and waste of a size one city deducted. Notice that we
     * don't do this if 'fulltradesize' is changed, since then we'd
     * never make cities. */
    int shield = result->city_center.tdc->shield
                 + result->best_other.tdc->shield;
    result->waste = adv->shield_priority
                    * city_waste(pcity, O_SHIELD, shield, NULL);

    if (game.info.fulltradesize == 1) {
      int trade = result->city_center.tdc->trade
                  + result->best_other.tdc->trade;
      result->corruption = adv->science_priority
                           * city_waste(pcity, O_TRADE, trade, NULL);
    } else {
      result->corruption = 0;
    }
  } else {
    /* Deduct difference in corruption and waste for real cities. Note that it
     * is possible (with notradesize) that we _gain_ value here. */
    city_size_add(pcity, 1);
    result->corruption = adv->science_priority
      * (city_waste(pcity, O_TRADE, result->best_other.tdc->trade, NULL)
         - pcity->waste[O_TRADE]);
    result->waste = adv->shield_priority
      * (city_waste(pcity, O_SHIELD, result->best_other.tdc->shield, NULL)
         - pcity->waste[O_SHIELD]);
    city_size_add(pcity, -1);
  }
  result->total -= result->corruption;
  result->total -= result->waste;
  result->total = MAX(0, result->total);

  pplayer->government = curr_govt;
  if (virtual_city) {
    destroy_city_virtual(pcity);
    tile_set_owner(result->tile, saved_owner, saved_claimer);
  }

  fc_assert_ret_val(result->city_center.tdc->sum >= 0, NULL);
  fc_assert_ret_val(result->remaining >= 0, NULL);

  return result;
}

/*************************************************************************//**
  Allocate tile data cache
*****************************************************************************/
struct tile_data_cache *tile_data_cache_new(void)
{
  struct tile_data_cache *ptdc_copy = fc_calloc(1, sizeof(*ptdc_copy));

  /* Set the turn the tile data cache was created. */
  ptdc_copy->turn = game.info.turn;

  return ptdc_copy;
}

/*************************************************************************//**
  Make copy of tile data cache
*****************************************************************************/
struct tile_data_cache *
  tile_data_cache_copy(const struct tile_data_cache *ptdc)
{
  struct tile_data_cache *ptdc_copy = tile_data_cache_new();

  fc_assert_ret_val(ptdc, NULL);

  ptdc_copy->shield = ptdc->shield;
  ptdc_copy->trade = ptdc->trade;
  ptdc_copy->food = ptdc->food;

  ptdc_copy->sum = ptdc->sum;
  ptdc_copy->reserved = ptdc->reserved;
  ptdc_copy->turn = ptdc->turn;

  return ptdc_copy;
}

/*************************************************************************//**
  Free resources allocated for tile data cache
*****************************************************************************/
static void tile_data_cache_destroy(struct tile_data_cache *ptdc)
{
  if (ptdc) {
    free(ptdc);
  }
}

/*************************************************************************//**
  Return player's tile data cache
*****************************************************************************/
static const struct tile_data_cache *tdc_plr_get(struct ai_type *ait,
                                                 struct player *plr,
                                                 int tindex)
{
  struct ai_plr *ai = dai_plr_data_get(ait, plr, NULL);

  fc_assert_ret_val(ai != NULL, NULL);
  fc_assert_ret_val(ai->settler != NULL, NULL);
  fc_assert_ret_val(ai->settler->tdc_hash != NULL, NULL);

  struct tile_data_cache *ptdc;

  tile_data_cache_hash_lookup(ai->settler->tdc_hash, tindex, &ptdc);

  if (!ptdc) {
#ifdef FREECIV_DEBUG
    ai->settler->cache.miss++;
#endif /* FREECIV_DEBUG */
    return NULL;
  } else if (ptdc->turn != game.info.turn) {
#ifdef FREECIV_DEBUG
    ai->settler->cache.old++;
#endif /* FREECIV_DEBUG */
    return NULL;
  } else {
#ifdef FREECIV_DEBUG
    ai->settler->cache.hit++;
#endif /* FREECIV_DEBUG */
    return ptdc;
  }
}

/*************************************************************************//**
  Store player's tile data cache
*****************************************************************************/
static void tdc_plr_set(struct ai_type *ait, struct player *plr, int tindex,
                        const struct tile_data_cache *ptdc)
{
  struct ai_plr *ai = dai_plr_data_get(ait, plr, NULL);

  fc_assert_ret(ai != NULL);
  fc_assert_ret(ai->settler != NULL);
  fc_assert_ret(ai->settler->tdc_hash != NULL);
  fc_assert_ret(ptdc != NULL);

#ifdef FREECIV_DEBUG
    ai->settler->cache.save++;
#endif /* FREECIV_DEBUG */

  tile_data_cache_hash_replace(ai->settler->tdc_hash, tindex, ptdc);
}

/*************************************************************************//**
  Check if a city on this location would starve.
*****************************************************************************/
static bool food_starvation(const struct cityresult *result)
{
  /* Avoid starvation: We must have enough food to grow.
   *   Note: this does not handle the case of a newly founded city breaking
   * even but being immediately able to build an improvement increasing its
   * yield (such as supermarkets and harbours in the classic ruleset).
   * /MSS */
  return (result->city_center.tdc->food
          + (result->best_other.tdc ? result->best_other.tdc->food
                                    : 0) <= game.info.food_cost);
}

/*************************************************************************//**
  Check if a city on this location would lack shields.
*****************************************************************************/
static bool shield_starvation(const struct cityresult *result)
{
  /* Avoid resource starvation. */
  return (result->city_center.tdc->shield
          + (result->best_other.tdc ? result->best_other.tdc->shield
                                    : 0) == 0);
}

/*************************************************************************//**
  Calculate defense bonus, which is a % of total results equal to a
  given % of the defense bonus %.
*****************************************************************************/
static int result_defense_bonus(struct player *pplayer,
                                const struct cityresult *result)
{
  /* Defense modification (as tie breaker mostly) */
  int defense_bonus =
    10 + tile_terrain(result->tile)->defense_bonus / 10;
  int extra_bonus = 0;
  struct tile *vtile = tile_virtual_new(result->tile);
  struct city *vcity = create_city_virtual(pplayer, vtile, "");

  tile_set_worked(vtile, vcity); /* Link tile_city(vtile) to vcity. */
  upgrade_city_extras(vcity, NULL); /* Give city free extras. */
  extra_type_iterate(pextra) {
    if (tile_has_extra(vtile, pextra)) {
      /* TODO: Do not use full bonus of those road types
       *       that are not native to all important units. */
      extra_bonus += pextra->defense_bonus;
    }
  } extra_type_iterate_end;
  tile_virtual_destroy(vtile);

  defense_bonus += (defense_bonus * extra_bonus) / 100;

  return 100 / (result->total + 1) * (100 / defense_bonus * DEFENSE_EMPHASIS);
}

/*************************************************************************//**
  Add bonus for coast.
*****************************************************************************/
static int naval_bonus(const struct cityresult *result)
{
  bool ocean_adjacent = is_terrain_class_near_tile(result->tile, TC_OCEAN);

  /* Adjust for ocean adjacency, which is nice */
  if (ocean_adjacent) {
    return (result->total * NAVAL_EMPHASIS) / 100;
  } else {
    return 0;
  }
}

/*************************************************************************//**
  For debugging, print the city result table.
*****************************************************************************/
static void print_cityresult(struct player *pplayer,
                             const struct cityresult *cr)
{
  int *city_map_reserved, *city_map_food, *city_map_shield, *city_map_trade;
  int tiles = city_map_tiles(cr->city_radius_sq);
  struct tile_data_cache *ptdc;

  fc_assert_ret(cr->tdc_hash != NULL);
  fc_assert_ret(tiles > 0);

  city_map_reserved = fc_calloc(tiles, sizeof(*city_map_reserved));
  city_map_food = fc_calloc(tiles, sizeof(*city_map_food));
  city_map_shield = fc_calloc(tiles, sizeof(*city_map_shield));
  city_map_trade = fc_calloc(tiles, sizeof(*city_map_trade));

  city_map_iterate(cr->city_radius_sq, cindex, x, y) {
    tile_data_cache_hash_lookup(cr->tdc_hash, cindex, &ptdc);
    fc_assert_ret(ptdc);
    city_map_reserved[cindex] = ptdc->reserved;
    city_map_food[cindex] = ptdc->reserved;
    city_map_shield[cindex] = ptdc->reserved;
    city_map_trade[cindex] = ptdc->reserved;
  } city_map_iterate_end;

  /* print reservations */
  log_test("cityresult for (x,y,radius_sq) = (%d, %d, %d) - Reservations:",
           TILE_XY(cr->tile), cr->city_radius_sq);
  citylog_map_data(LOG_TEST, cr->city_radius_sq, city_map_reserved);

  /*  print food */
  log_test("cityresult for (x,y,radius_sq) = (%d, %d, %d) - Food:",
           TILE_XY(cr->tile), cr->city_radius_sq);
  citylog_map_data(LOG_TEST, cr->city_radius_sq, city_map_food);

  /* print shield */
  log_test("cityresult for (x,y,radius_sq) = (%d, %d, %d) - Shield:",
           TILE_XY(cr->tile), cr->city_radius_sq);
  citylog_map_data(LOG_TEST, cr->city_radius_sq, city_map_shield);

  /* print trade */
  log_test("cityresult for (x,y,radius_sq) = (%d, %d, %d) - Trade:",
           TILE_XY(cr->tile), cr->city_radius_sq);
  citylog_map_data(LOG_TEST, cr->city_radius_sq, city_map_trade);

  free(city_map_reserved);
  free(city_map_food);
  free(city_map_shield);
  free(city_map_trade);

  log_test("city center (%d, %d) %d + best other (abs: %d, %d)"
           " (cindex: %d) %d", TILE_XY(cr->tile),
           cr->city_center.tdc->sum, TILE_XY(cr->best_other.tile),
           cr->best_other.cindex, cr->best_other.tdc->sum);
  log_test("- corr %d - waste %d + remaining %d"
           " + defense bonus %d + naval bonus %d", cr->corruption,
           cr->waste, cr->remaining, result_defense_bonus(pplayer, cr),
           naval_bonus(cr));
  log_test("= %d (%d)", cr->total, cr->result);

  if (food_starvation(cr)) {
    log_test(" ** FOOD STARVATION **");
  }
  if (shield_starvation(cr)) {
    log_test(" ** RESOURCE STARVATION **");
  }
}

/*************************************************************************//**
  Calculates the desire for founding a new city at 'ptile'. The citymap
  ensures that we do not build cities too close to each other. Returns NULL
  if no place was found.
*****************************************************************************/
struct cityresult *city_desirability(struct ai_type *ait, struct player *pplayer,
                                     struct unit *punit, struct tile *ptile)
{
  struct city *pcity = tile_city(ptile);
  struct adv_data *ai = adv_data_get(pplayer, NULL);
  struct cityresult *cr = NULL;

  fc_assert_ret_val(punit, NULL);
  fc_assert_ret_val(pplayer, NULL);
  fc_assert_ret_val(ai, NULL);

  if (!city_can_be_built_here(ptile, punit)
      || (has_handicap(pplayer, H_MAP)
          && !map_is_known(ptile, pplayer))) {
    return NULL;
  }

  /* Check if another settler has taken a spot within mindist */
  square_iterate(&(wld.map), ptile, game.info.citymindist-1, tile1) {
    if (citymap_is_reserved(tile1)) {
      return NULL;
    }
  } square_iterate_end;

  if (adv_danger_at(punit, ptile)) {
    return NULL;
  }

  if (pcity && (city_size_get(pcity) + unit_pop_value(punit)
                > game.info.add_to_size_limit)) {
    /* Can't exceed population limit. */
    return NULL;
  }

  if (!pcity && citymap_is_reserved(ptile)) {
    return NULL; /* reserved, go away */
  }

  /* If (x, y) is an existing city, consider immigration */
  if (pcity && city_owner(pcity) == pplayer) {
    return NULL;
  }

  cr = cityresult_fill(ait, pplayer, ptile); /* Burn CPU, burn! */
  if (!cr) {
    /* Failed to find a good spot */
    return NULL;
  }

  /*** Alright: Now consider building a new city ***/

  if (food_starvation(cr) || shield_starvation(cr)) {
    cityresult_destroy(cr);
    return NULL;
  }

  cr->total += result_defense_bonus(pplayer, cr);
  cr->total += naval_bonus(cr);

  /* Add remaining points, which is our potential */
  cr->total += cr->remaining;

  fc_assert_ret_val(cr->total >= 0, NULL); /* Does not frees cr! */

  return cr;
}

/*************************************************************************//**
  Find nearest and best city placement in a PF iteration according to
  "parameter".  The value in "boat_cost" is both the penalty to pay for
  using a boat and an indicator (boat_cost != 0) if a boat was used at all.
  The result is returned in "best".

  Return value is a 'struct cityresult' if found something better than what
  was originally in "best" was found; else NULL.

  TODO: Transparently check if we should add ourselves to an existing city.
*****************************************************************************/
static struct cityresult *settler_map_iterate(struct ai_type *ait,
                                              struct pf_parameter *parameter,
                                              struct unit *punit,
                                              int boat_cost)
{
  struct cityresult *cr = NULL, *best = NULL;
  int best_turn = 0; /* Which turn we found the best fit */
  struct player *pplayer = unit_owner(punit);
  struct pf_map *pfm;

  pfm = pf_map_new(parameter);
  pf_map_move_costs_iterate(pfm, ptile, move_cost, FALSE) {
    int turns;

    if (boat_cost == 0 && unit_class_get(punit)->adv.sea_move == MOVE_NONE
        && tile_continent(ptile) != tile_continent(unit_tile(punit))) {
      /* We have an accidential land bridge. Ignore it. It will in all
       * likelihood go away next turn, or even in a few nanoseconds. */
      continue;
    }
    if (BORDERS_DISABLED != game.info.borders) {
      struct player *powner = tile_owner(ptile);
      if (NULL != powner
       && powner != pplayer
       && pplayers_in_peace(powner, pplayer)) {
        /* Land theft does not make for good neighbours. */
        continue;
      }
    }

    /* Calculate worth */
    cr = city_desirability(ait, pplayer, punit, ptile);

    /* Check if actually found something */
    if (!cr) {
      continue;
    }

    /* This algorithm punishes long treks */
    turns = move_cost / parameter->move_rate;
    cr->result = amortize(cr->total, PERFECTION * turns);

    /* Reduce want by settler cost. Easier than amortize, but still
     * weeds out very small wants. ie we create a threshold here. */
    /* We also penalise here for using a boat (either virtual or real)
     * it's crude but what isn't? */
    cr->result -= unit_build_shield_cost_base(punit) + boat_cost;

    /* Find best spot */
    if ((!best && cr->result > 0)
        || (best && cr->result > best->result)) {
      /* Destroy the old 'best' value. */
      cityresult_destroy(best);
      /* save the new 'best' value. */
      best = cr;
      cr = NULL;
      best_turn = turns;

      log_debug("settler map search (search): (%d,%d) %d",
                TILE_XY(best->tile), best->result);
    } else {
      /* Destroy the unused result. */
      cityresult_destroy(cr);
      cr = NULL;
    }

    /* Can we terminate early? We have a 'good enough' spot, and
     * we don't block the establishment of a better city just one
     * further step away. */
    if (best && best->result > RESULT_IS_ENOUGH
        && turns > parameter->move_rate /* sic -- yeah what an explanation! */
        && best_turn < turns /*+ game.info.min_dist_bw_cities*/) {
      break;
    }
  } pf_map_move_costs_iterate_end;

  pf_map_destroy(pfm);

  if (best) {
    log_debug("settler map search (final): (%d,%d) %d", TILE_XY(best->tile),
              best->result);
  } else {
    log_debug("settler map search (final): no result");
  }

  return best;
}

/*************************************************************************//**
  Find nearest and best city placement or (TODO) a city to immigrate to.

  Option look_for_boat forces us to find a (real) boat before considering
  going overseas.  Option use_virt_boat allows to use virtual boat but only
  if punit is in a coastal city right now (should only be used by
  virtual units).  I guess it won't hurt to remove this condition, PF
  will just give no positions.
  If (!look_for_boat && !use_virt_boat), will not consider placements
  overseas.

  Returns the better cityresult or NULL if no result was found.
*****************************************************************************/
static struct cityresult *find_best_city_placement(struct ai_type *ait,
                                                   struct unit *punit,
                                                   bool look_for_boat,
                                                   bool use_virt_boat)
{
  struct pf_parameter parameter;
  struct player *pplayer = unit_owner(punit);
  struct unit *ferry = NULL;
  struct cityresult *cr1 = NULL, *cr2 = NULL;

  fc_assert_ret_val(is_ai(pplayer), NULL);
  /* Only virtual units may use virtual boats: */
  fc_assert_ret_val(0 == punit->id || !use_virt_boat, NULL);

  /* Phase 1: Consider building cities on our continent */

  pft_fill_unit_parameter(&parameter, punit);
  parameter.omniscience = !has_handicap(pplayer, H_MAP);
  cr1 = settler_map_iterate(ait, &parameter, punit, 0);

  if (cr1 && cr1->result > RESULT_IS_ENOUGH) {
    /* skip further searches */
    return cr1;
  }

  /* Phase 2: Consider travelling to another continent */

  if (look_for_boat) {
    int ferry_id = aiferry_find_boat(ait, punit, 1, NULL);

    ferry = game_unit_by_number(ferry_id);
  }

  if (ferry || (use_virt_boat
                && is_terrain_class_near_tile(unit_tile(punit), TC_OCEAN)
                && tile_city(unit_tile(punit)))) {
    if (!ferry) {
      /* No boat?  Get a virtual one! */
      struct unit_type *boattype
        = best_role_unit_for_player(pplayer, L_FERRYBOAT);

      if (boattype == NULL) {
        /* Sea travel not possible yet. Bump tech want for ferries. */
        boattype = get_role_unit(L_FERRYBOAT, 0);

        if (NULL != boattype
            && A_NEVER != boattype->require_advance) {
          struct ai_plr *plr_data = def_ai_player_data(pplayer, ait);

          plr_data->tech_want[advance_index(boattype->require_advance)]
            += FERRY_TECH_WANT;
          TECH_LOG(ait, LOG_DEBUG, pplayer, boattype->require_advance,
                   "+ %d for %s to ferry settler",
                   FERRY_TECH_WANT,
                   utype_rule_name(boattype));
        }
        /* return the result from the search on our current continent */
        return cr1;
      }
      ferry = unit_virtual_create(pplayer, NULL, boattype, 0);
      unit_tile_set(ferry, unit_tile(punit));
    }

    fc_assert(dai_is_ferry(ferry, ait));
    pft_fill_unit_overlap_param(&parameter, ferry);
    parameter.omniscience = !has_handicap(pplayer, H_MAP);
    parameter.get_TB = no_fights_or_unknown;

    /* FIXME: Maybe penalty for using an existing boat is too high?
     * We shouldn't make the penalty for building a new boat too high though.
     * Building a new boat is like a war against a weaker enemy -- 
     * good for the economy. (c) Bush family */
    cr2 = settler_map_iterate(ait, &parameter, punit,
                              unit_build_shield_cost_base(ferry));
    if (cr2) {
      cr2->overseas = TRUE;
      cr2->virt_boat = (ferry->id == 0);
    }

    if (ferry->id == 0) {
      unit_virtual_destroy(ferry);
    }

    /* If we use a virtual boat, we must have permission and be emigrating: */
    /* FIXME: These assert do not frees cr2! */
    fc_assert_ret_val(!cr2 || (!cr2->virt_boat || use_virt_boat), NULL);
    fc_assert_ret_val(!cr2 || (!cr2->virt_boat || cr2->overseas), NULL);
  }

  if (!cr1) {
    /* No want for a new city on our current continent; return the result for
     * traveling by boat. */
    return cr2;
  } else if (!cr2) {
    /* No want for an overseas city; return the result for a city on our
     * current continent. */
    return cr1;
  }

  /* We want an overseas city and a city on the current continent - select the
   * best! */
  if (cr1->result > cr2->result) {
    cityresult_destroy(cr2);
    return cr1;
  } else {
    cityresult_destroy(cr1);
    return cr2;
  }
}

/*************************************************************************//**
  Initialize ai settler engine.
*****************************************************************************/
void dai_auto_settler_init(struct ai_plr *ai)
{
  fc_assert_ret(ai != NULL);
  fc_assert_ret(ai->settler == NULL);

  ai->settler = fc_calloc(1, sizeof(*ai->settler));
  ai->settler->tdc_hash = tile_data_cache_hash_new();

#ifdef FREECIV_DEBUG
  ai->settler->cache.hit = 0;
  ai->settler->cache.old = 0;
  ai->settler->cache.miss = 0;
  ai->settler->cache.save = 0;
#endif /* FREECIV_DEBUG */
}

/*************************************************************************//**
  Auto settler that can also build cities.
*****************************************************************************/
void dai_auto_settler_run(struct ai_type *ait, struct player *pplayer,
                          struct unit *punit, struct settlermap *state)
{
  int best_impr = 0;            /* best terrain improvement we can do */
  enum unit_activity best_act;
  struct extra_type *best_target;
  struct tile *best_tile = NULL;
  struct pf_path *path = NULL;
  struct city *pcity = NULL;

  /* time it will take worker to complete its given task */
  int completion_time = 0;

  CHECK_UNIT(punit);

  /*** If we are on a city mission: Go where we should ***/

BUILD_CITY:

  if (def_ai_unit_data(punit, ait)->task == AIUNIT_BUILD_CITY) {
    struct tile *ptile = punit->goto_tile;
    int sanity = punit->id;

    /* Check that the mission is still possible.  If the tile has become
     * unavailable, call it off. */
    if (!city_can_be_built_here(ptile, punit)) {
      dai_unit_new_task(ait, punit, AIUNIT_NONE, NULL);
      set_unit_activity(punit, ACTIVITY_IDLE);
      send_unit_info(NULL, punit);
      return; /* avoid recursion at all cost */
    } else {
     /* Go there */
      if ((!dai_gothere(ait, pplayer, punit, ptile)
           && NULL == game_unit_by_number(sanity))
          || punit->moves_left <= 0) {
        return;
      }
      if (same_pos(unit_tile(punit), ptile)) {
        if (!dai_do_build_city(ait, pplayer, punit)) {
          UNIT_LOG(LOG_DEBUG, punit, "could not make city on %s",
                   tile_get_info_text(unit_tile(punit), TRUE, 0));
          dai_unit_new_task(ait, punit, AIUNIT_NONE, NULL);
          /* Only known way to end in here is that hut turned in to a city
           * when settler entered tile. So this is not going to lead in any
           * serious recursion. */
          dai_auto_settler_run(ait, pplayer, punit, state);

          return;
       } else {
          return; /* We came, we saw, we built... */
        }
      } else {
        UNIT_LOG(LOG_DEBUG, punit, "could not go to target");
        /* ai_unit_new_role(punit, AIUNIT_NONE, NULL); */
        return;
      }
    }
  }

  /*** Try find some work ***/

  if (unit_has_type_flag(punit, UTYF_SETTLERS)) {
    struct worker_task *best_task;

    TIMING_LOG(AIT_WORKERS, TIMER_START);

    /* Have nearby cities requests? */
    pcity = settler_evaluate_city_requests(punit, &best_task, &path, state);

    if (pcity != NULL) {
      if (path != NULL) {
        completion_time = pf_path_last_position(path)->turn;
        best_impr = 1;
        best_act = best_task->act;
        best_target = best_task->tgt;
        best_tile = best_task->ptile;
      } else {
        pcity = NULL;
      }
    }

    if (pcity == NULL) {
      best_impr = settler_evaluate_improvements(punit, &best_act, &best_target,
                                                &best_tile, &path, state);
      if (path) {
        completion_time = pf_path_last_position(path)->turn;
      }
    }
    UNIT_LOG(LOG_DEBUG, punit, "impr want %d", best_impr);
    TIMING_LOG(AIT_WORKERS, TIMER_STOP);
  }

  if (unit_is_cityfounder(punit)) {
    struct cityresult *result;

    /* may use a boat: */
    TIMING_LOG(AIT_SETTLERS, TIMER_START);
    result = find_best_city_placement(ait, punit, TRUE, FALSE);
    TIMING_LOG(AIT_SETTLERS, TIMER_STOP);
    if (result && result->result > best_impr) {
      UNIT_LOG(LOG_DEBUG, punit, "city want %d", result->result);
      if (tile_city(result->tile)) {
        UNIT_LOG(LOG_DEBUG, punit, "immigrates to %s (%d, %d)", 
                 city_name_get(tile_city(result->tile)),
                 TILE_XY(result->tile));
      } else {
        UNIT_LOG(LOG_DEBUG, punit, "makes city at (%d, %d)", 
                 TILE_XY(result->tile));
        if (punit->server.debug) {
          print_cityresult(pplayer, result);
        }
      }
      /* Go make a city! */
      adv_unit_new_task(punit, AUT_BUILD_CITY, result->tile);
      if (result->best_other.tile && result->best_other.tdc->sum >= 0) {
        /* Reserve best other tile (if there is one). It is the tile where the
         * first citizen of the city is working. */
        citymap_reserve_tile(result->best_other.tile, punit->id);
      }
      punit->goto_tile = result->tile; /* TMP */

      cityresult_destroy(result);

      /*** Go back to and found a city ***/
      pf_path_destroy(path);
      path = NULL;
      goto BUILD_CITY;
    } else if (best_impr > 0) {
      UNIT_LOG(LOG_DEBUG, punit, "improves terrain instead of founding");
      /* Terrain improvements follows the old model, and is recalculated
       * each turn. */
      if (result) {
        /* We had a city result, just worse than best impr */
        cityresult_destroy(result);
      }
      adv_unit_new_task(punit, AUT_AUTO_SETTLER, best_tile);
    } else {
      UNIT_LOG(LOG_DEBUG, punit, "cannot find work");
      fc_assert(result == NULL);
      dai_unit_new_task(ait, punit, AIUNIT_NONE, NULL);
      goto CLEANUP;
    }
  } else {
    /* We are a worker or engineer */
    adv_unit_new_task(punit, AUT_AUTO_SETTLER, best_tile);
  }

  if (auto_settler_setup_work(pplayer, punit, state, 0, path,
                              best_tile, best_act, &best_target,
                              completion_time)) {
    if (pcity != NULL) {
      clear_worker_tasks(pcity);
    }
  }

CLEANUP:

  if (NULL != path) {
    pf_path_destroy(path);
  }
}

/*************************************************************************//**
  Auto settler continuing its work.
*****************************************************************************/
void dai_auto_settler_cont(struct ai_type *ait, struct player *pplayer,
                           struct unit *punit, struct settlermap *state)
{
  if (!adv_settler_safe_tile(pplayer, punit, unit_tile(punit))) {
    unit_activity_handling(punit, ACTIVITY_IDLE);
  }
}

/*************************************************************************//**
  Reset ai settler engine.
*****************************************************************************/
void dai_auto_settler_reset(struct ai_type *ait, struct player *pplayer)
{
  bool caller_closes;
  struct ai_plr *ai = dai_plr_data_get(ait, pplayer, &caller_closes);

  fc_assert_ret(ai != NULL);
  fc_assert_ret(ai->settler != NULL);
  fc_assert_ret(ai->settler->tdc_hash != NULL);

#ifdef FREECIV_DEBUG
  log_debug("[aisettler cache for %s] save: %d, miss: %d, old: %d, hit: %d",
            player_name(pplayer), ai->settler->cache.save,
            ai->settler->cache.miss, ai->settler->cache.old,
            ai->settler->cache.hit);

  ai->settler->cache.hit = 0;
  ai->settler->cache.old = 0;
  ai->settler->cache.miss = 0;
  ai->settler->cache.save = 0;
#endif /* FREECIV_DEBUG */

  tile_data_cache_hash_clear(ai->settler->tdc_hash);

  if (caller_closes) {
    dai_data_phase_finished(ait, pplayer);
  }
}

/*************************************************************************//**
  Deinitialize ai settler engine.
*****************************************************************************/
void dai_auto_settler_free(struct ai_plr *ai)
{
  fc_assert_ret(ai != NULL);

  if (ai->settler) {
    if (ai->settler->tdc_hash) {
      tile_data_cache_hash_destroy(ai->settler->tdc_hash);
    }
    free(ai->settler);
  }
  ai->settler = NULL;
}

/*************************************************************************//**
  Build a city and initialize AI infrastructure cache.
*****************************************************************************/
static bool dai_do_build_city(struct ai_type *ait, struct player *pplayer,
                              struct unit *punit)
{
  struct tile *ptile = unit_tile(punit);
  struct city *pcity;

  fc_assert_ret_val(pplayer == unit_owner(punit), FALSE);
  unit_activity_handling(punit, ACTIVITY_IDLE);

  /* Free city reservations */
  dai_unit_new_task(ait, punit, AIUNIT_NONE, NULL);

  pcity = tile_city(ptile);
  if (pcity) {
    /* This can happen for instance when there was hut at this tile
     * and it turned in to a city when settler entered tile. */
    log_debug("%s: There is already a city at (%d, %d)!",
              player_name(pplayer), TILE_XY(ptile));
    return FALSE;
  }
  unit_do_action(pplayer, punit->id, ptile->index,
                 0, city_name_suggestion(pplayer, ptile),
                 ACTION_FOUND_CITY);
  pcity = tile_city(ptile);
  if (!pcity && punit) {
    enum ane_kind reason = action_not_enabled_reason(punit,
                                                     ACTION_FOUND_CITY,
                                                     ptile, NULL, NULL);

    if (reason == ANEK_CITY_TOO_CLOSE_TGT) {
      /* This is acceptable. A hut in the path to the tile may have created
       * a city that now is too close. */
      log_debug("%s: Failed to build city at (%d, %d)",
                player_name(pplayer), TILE_XY(ptile));
    } else {
      /* The request was illegal to begin with. */
      log_error("%s: Failed to build city at (%d, %d). Reason id: %d",
                player_name(pplayer), TILE_XY(ptile), reason);
    }
    return FALSE;
  }

  /* We have to rebuild at least the cache for this city.  This event is
   * rare enough we might as well build the whole thing.  Who knows what
   * else might be cached in the future? */
  fc_assert_ret_val(pplayer == city_owner(pcity), FALSE);
  initialize_infrastructure_cache(pplayer);

  /* Init ai.choice. Handling ferryboats might use it. */
  adv_init_choice(&def_ai_city_data(pcity, ait)->choice);

  return TRUE;
}

/*************************************************************************//**
  Return want for city settler. Note that we rely here on the fact that
  ai_settler_init() has been run while doing autosettlers.
*****************************************************************************/
void contemplate_new_city(struct ai_type *ait, struct city *pcity)
{
  struct unit *virtualunit;
  struct tile *pcenter = city_tile(pcity);
  struct player *pplayer = city_owner(pcity);
  struct unit_type *unit_type;

  if (game.scenario.prevent_new_cities) {
    return;
  }

  unit_type = best_role_unit(pcity, action_id_get_role(ACTION_FOUND_CITY));

  if (unit_type == NULL) {
    log_debug("No ACTION_FOUND_CITY role unit available");
    return;
  }

  /* Create a localized "virtual" unit to do operations with. */
  virtualunit = unit_virtual_create(pplayer, pcity, unit_type, 0);
  unit_tile_set(virtualunit, pcenter);

  if (is_ai(pplayer)) {
    struct cityresult *result;
    bool is_coastal = is_terrain_class_near_tile(pcenter, TC_OCEAN);
    struct ai_city *city_data = def_ai_city_data(pcity, ait);

    result = find_best_city_placement(ait, virtualunit, is_coastal, is_coastal);

    if (result) {
      fc_assert_ret(0 <= result->result); /* 'result' is not freed! */

      CITY_LOG(LOG_DEBUG, pcity, "want(%d) to establish city at"
               " (%d, %d) and will %s to get there", result->result,
               TILE_XY(result->tile), 
               (result->virt_boat ? "build a boat" : 
                (result->overseas ? "use a boat" : "walk")));

      city_data->founder_want = (result->virt_boat ? 
                                 -result->result : result->result);
      city_data->founder_boat = result->overseas;

      cityresult_destroy(result);
    } else {
      CITY_LOG(LOG_DEBUG, pcity, "want no city");
      city_data->founder_want = 0;
    }
  } else {
    /* Always failing */
    fc_assert_ret(is_ai(pplayer));
  }

  unit_virtual_destroy(virtualunit);
}
