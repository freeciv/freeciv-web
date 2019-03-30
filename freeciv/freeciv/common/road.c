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

/* common */
#include "extras.h"
#include "fc_types.h"
#include "game.h"
#include "map.h"
#include "movement.h"
#include "name_translation.h"
#include "unittype.h"

#include "road.h"

/************************************************************************//**
  Return the road id.
****************************************************************************/
Road_type_id road_number(const struct road_type *proad)
{
  fc_assert_ret_val(NULL != proad, -1);

  return proad->id;
}

/************************************************************************//**
  Return extra that road is.
****************************************************************************/
struct extra_type *road_extra_get(const struct road_type *proad)
{
  return proad->self;
}

/************************************************************************//**
  Return the number of road_types.
****************************************************************************/
Road_type_id road_count(void)
{
  return game.control.num_road_types;
}

/************************************************************************//**
  Return road type of given id.
****************************************************************************/
struct road_type *road_by_number(Road_type_id id)
{
  struct extra_type_list *roads;

  roads = extra_type_list_by_cause(EC_ROAD);

  if (roads == NULL || id < 0 || id >= extra_type_list_size(roads)) {
    return NULL;
  }

  return extra_road_get(extra_type_list_get(roads, id));
}

/************************************************************************//**
  This function is passed to road_type_list_sort() to sort a list of roads
  in ascending move_cost (faster roads first).
****************************************************************************/
int compare_road_move_cost(const struct extra_type *const *p,
                           const struct extra_type *const *q)
{
  const struct road_type *proad = extra_road_get(*p);
  const struct road_type *qroad = extra_road_get(*q);

  if (proad->move_cost > qroad->move_cost) {
    return -1; /* q is faster */
  } else if (proad->move_cost == qroad->move_cost) {
    return 0;
  } else {
    return 1; /* p is faster */
  }
}

/************************************************************************//**
  Initialize road_type structures.
****************************************************************************/
void road_type_init(struct extra_type *pextra, int idx)
{
  struct road_type *proad;

  proad = fc_malloc(sizeof(*proad));

  pextra->data.road = proad;

  requirement_vector_init(&proad->first_reqs);

  proad->id = idx;
  proad->integrators = NULL;
  proad->self = pextra;
}

/************************************************************************//**
  Initialize the road integrators cache
****************************************************************************/
void road_integrators_cache_init(void)
{
  extra_type_by_cause_iterate(EC_ROAD, pextra) {
    struct road_type *proad = extra_road_get(pextra);

    proad->integrators = extra_type_list_new();
    /* Roads always integrate with themselves. */
    extra_type_list_append(proad->integrators, pextra);
    extra_type_by_cause_iterate(EC_ROAD, oextra) {
      struct road_type *oroad = extra_road_get(oextra);

      if (BV_ISSET(proad->integrates, road_number(oroad))) {
        extra_type_list_append(proad->integrators, oextra);
      }
    } extra_type_by_cause_iterate_end;
    extra_type_list_unique(proad->integrators);
    extra_type_list_sort(proad->integrators, &compare_road_move_cost);
  } extra_type_by_cause_iterate_end;
}

/************************************************************************//**
  Free the memory associated with road types
****************************************************************************/
void road_types_free(void)
{
  extra_type_by_cause_iterate(EC_ROAD, pextra) {
    struct road_type *proad = extra_road_get(pextra);

    requirement_vector_free(&proad->first_reqs);

    if (proad->integrators != NULL) {
      extra_type_list_destroy(proad->integrators);
      proad->integrators = NULL;
    }
  } extra_type_by_cause_iterate_end;
}

/************************************************************************//**
  Return tile special that used to represent this road type.
****************************************************************************/
enum road_compat road_compat_special(const struct road_type *proad)
{
  return proad->compat;
}

/************************************************************************//**
  Return road type represented by given compatibility special, or NULL if
  special does not represent road type at all.
****************************************************************************/
struct road_type *road_by_compat_special(enum road_compat compat)
{
  if (compat == ROCO_NONE) {
    return NULL;
  }

  extra_type_by_cause_iterate(EC_ROAD, pextra) {
    struct road_type *proad = extra_road_get(pextra);
    if (road_compat_special(proad) == compat) {
      return proad;
    }
  } extra_type_by_cause_iterate_end;

  return NULL;
}

/************************************************************************//**
  Tells if road can build to tile if all other requirements are met.
****************************************************************************/
bool road_can_be_built(const struct road_type *proad, const struct tile *ptile)
{

  if (!(road_extra_get(proad)->buildable)) {
    /* Road type not buildable. */
    return FALSE;
  }

  if (tile_has_road(ptile, proad)) {
    /* Road exist already */
    return FALSE;
  }

  if (tile_terrain(ptile)->road_time == 0) {
    return FALSE;
  }

  return TRUE;
}

/************************************************************************//**
  Tells if player and optionally unit have road building requirements
  fulfilled.
****************************************************************************/
static bool are_road_reqs_fulfilled(const struct road_type *proad,
                                    const struct player *pplayer,
                                    const struct unit *punit,
                                    const struct tile *ptile)
{
  struct extra_type *pextra = road_extra_get(proad);
  const struct unit_type *utype;

  if (punit == NULL) {
    utype = NULL;
  } else {
    utype = unit_type_get(punit);
  }

  if (requirement_vector_size(&proad->first_reqs) > 0) {
    bool beginning = TRUE;

    extra_type_list_iterate(proad->integrators, iroad) {
      /* FIXME: mixing cardinal and non-cardinal roads as integrators is
       * probably not a good idea. */
      if (is_cardinal_only_road(iroad)) {
        cardinal_adjc_iterate(&(wld.map), ptile, adjc_tile) {
          if (tile_has_extra(adjc_tile, iroad)) {
            beginning = FALSE;
            break;
          }
        } cardinal_adjc_iterate_end;
      } else {
        adjc_iterate(&(wld.map), ptile, adjc_tile) {
          if (tile_has_extra(adjc_tile, iroad)) {
            beginning = FALSE;
            break;
          }
        } adjc_iterate_end;
      }

      if (!beginning) {
        break;
      }
    } extra_type_list_iterate_end;

    if (beginning) {
      if (!are_reqs_active(pplayer, tile_owner(ptile), NULL, NULL, ptile,
                           punit, utype, NULL, NULL, NULL,
                           &proad->first_reqs, RPT_POSSIBLE)) {
        return FALSE;
      }
    }
  }

  return are_reqs_active(pplayer, tile_owner(ptile), NULL, NULL, ptile,
                         punit, utype, NULL, NULL, NULL,
                         &pextra->reqs, RPT_POSSIBLE);
}

/************************************************************************//**
  Tells if player can build road to tile with suitable unit.
****************************************************************************/
bool player_can_build_road(const struct road_type *proad,
                           const struct player *pplayer,
                           const struct tile *ptile)
{
  if (!can_build_extra_base(road_extra_get(proad), pplayer, ptile)) {
    return FALSE;
  }

  return are_road_reqs_fulfilled(proad, pplayer, NULL, ptile);
}

/************************************************************************//**
  Tells if unit can build road on tile.
****************************************************************************/
bool can_build_road(struct road_type *proad,
                    const struct unit *punit,
                    const struct tile *ptile)
{
  struct player *pplayer = unit_owner(punit);

  if (!can_build_extra_base(road_extra_get(proad), pplayer, ptile)) {
    return FALSE;
  }

  return are_road_reqs_fulfilled(proad, pplayer, punit, ptile);
}

/************************************************************************//**
  Count tiles with specified road near the tile. Can be called with NULL
  road.
****************************************************************************/
int count_road_near_tile(const struct tile *ptile, const struct road_type *proad)
{
  int count = 0;

  if (proad == NULL) {
    return 0;
  }

  adjc_iterate(&(wld.map), ptile, adjc_tile) {
    if (tile_has_road(adjc_tile, proad)) {
      count++;
    }
  } adjc_iterate_end;

  return count;
}

/************************************************************************//**
  Count tiles with any river near the tile.
****************************************************************************/
int count_river_near_tile(const struct tile *ptile,
                          const struct extra_type *priver)
{
  int count = 0;

  cardinal_adjc_iterate(&(wld.map), ptile, adjc_tile) {
    if (priver == NULL && tile_has_river(adjc_tile)) {
      /* Some river */
      count++;
    } else if (priver != NULL && tile_has_extra(adjc_tile, priver)) {
      /* Specific river */
      count++;
    }
  } cardinal_adjc_iterate_end;

  return count;
}

/************************************************************************//**
  Count tiles with river of specific type cardinally adjacent to the tile.
****************************************************************************/
int count_river_type_tile_card(const struct tile *ptile,
                               const struct extra_type *priver,
                               bool percentage)
{
  int count = 0;
  int total = 0;

  fc_assert(priver != NULL);

  cardinal_adjc_iterate(&(wld.map), ptile, adjc_tile) {
    if (tile_has_extra(adjc_tile, priver)) {
      count++;
    }
    total++;
  } cardinal_adjc_iterate_end;

  if (percentage) {
    count = count * 100 / total;
  }
  return count;
}

/************************************************************************//**
  Count tiles with river of specific type near the tile.
****************************************************************************/
int count_river_type_near_tile(const struct tile *ptile,
                               const struct extra_type *priver,
                               bool percentage)
{
  int count = 0;
  int total = 0;

  fc_assert(priver != NULL);

  adjc_iterate(&(wld.map), ptile, adjc_tile) {
    if (tile_has_extra(adjc_tile, priver)) {
      count++;
    }
    total++;
  } adjc_iterate_end;

  if (percentage) {
    count = count * 100 / total;
  }
  return count;
}

/************************************************************************//**
  Check if road provides effect
****************************************************************************/
bool road_has_flag(const struct road_type *proad, enum road_flag_id flag)
{
  return BV_ISSET(proad->flags, flag);
}

/************************************************************************//**
  Returns TRUE iff any cardinally adjacent tile contains a road with
  the given flag (does not check ptile itself).
****************************************************************************/
bool is_road_flag_card_near(const struct tile *ptile, enum road_flag_id flag)
{
  extra_type_by_cause_iterate(EC_ROAD, pextra) {
    if (road_has_flag(extra_road_get(pextra), flag)) {
      cardinal_adjc_iterate(&(wld.map), ptile, adjc_tile) {
        if (tile_has_extra(adjc_tile, pextra)) {
          return TRUE;
        }
      } cardinal_adjc_iterate_end;
    }
  } extra_type_by_cause_iterate_end;

  return FALSE;
}

/************************************************************************//**
  Returns TRUE iff any adjacent tile contains a road with the given flag
  (does not check ptile itself).
****************************************************************************/
bool is_road_flag_near_tile(const struct tile *ptile, enum road_flag_id flag)
{
  extra_type_by_cause_iterate(EC_ROAD, pextra) {
    if (road_has_flag(extra_road_get(pextra), flag)) {
      adjc_iterate(&(wld.map), ptile, adjc_tile) {
        if (tile_has_extra(adjc_tile, pextra)) {
          return TRUE;
        }
      } adjc_iterate_end;
    }
  } extra_type_by_cause_iterate_end;

  return FALSE;
}

/************************************************************************//**
  Is tile native to road?
****************************************************************************/
bool is_native_tile_to_road(const struct road_type *proad,
                            const struct tile *ptile)
{
  struct extra_type *pextra;

  if (road_has_flag(proad, RF_RIVER)) {
    if (!terrain_has_flag(tile_terrain(ptile), TER_CAN_HAVE_RIVER)) {
      return FALSE;
    }
  } else if (tile_terrain(ptile)->road_time == 0) {
    return FALSE;
  }

  pextra = road_extra_get(proad);

  return are_reqs_active(NULL, NULL, NULL, NULL, ptile,
                         NULL, NULL, NULL, NULL, NULL,
                         &pextra->reqs, RPT_POSSIBLE);
}

/************************************************************************//**
  Is extra cardinal only road.
****************************************************************************/
bool is_cardinal_only_road(const struct extra_type *pextra)
{
  const struct road_type *proad;

  if (!is_extra_caused_by(pextra, EC_ROAD)) {
    return FALSE;
  }

  proad = extra_road_get(pextra);

  return proad->move_mode == RMM_CARDINAL || proad->move_mode == RMM_RELAXED;
}

/************************************************************************//**
  Does road type provide move bonus
****************************************************************************/
bool road_provides_move_bonus(const struct road_type *proad)
{
  return proad->move_cost >= 0;
}
