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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>

/* utility */
#include "distribute.h"
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "support.h"

/* common */
#include "ai.h"
#include "effects.h"
#include "game.h"
#include "government.h"
#include "improvement.h"
#include "map.h"
#include "movement.h"
#include "packets.h"
#include "specialist.h"
#include "unit.h"

#include "cm.h"

#include "city.h"

/* Define this to add in extra (very slow) assertions for the city code. */
#undef CITY_DEBUGGING

/* Get city tile informations using the city tile index. */
static struct iter_index *city_map_index;

struct citystyle *city_styles = NULL;

static int city_tiles;

/* One day these values may be read in from the ruleset.  In the meantime
 * they're just an easy way to access information about each output type. */
struct output_type output_types[O_LAST] = {
  {O_FOOD, N_("Food"), "food", TRUE, UNHAPPY_PENALTY_SURPLUS},
  {O_SHIELD, N_("Shield"), "shield", TRUE, UNHAPPY_PENALTY_SURPLUS},
  {O_TRADE, N_("Trade"), "trade", TRUE, UNHAPPY_PENALTY_NONE},
  {O_GOLD, N_("Gold"), "gold", FALSE, UNHAPPY_PENALTY_ALL_PRODUCTION},
  {O_LUXURY, N_("Luxury"), "luxury", FALSE, UNHAPPY_PENALTY_NONE},
  {O_SCIENCE, N_("Science"), "science", FALSE, UNHAPPY_PENALTY_ALL_PRODUCTION}
};

/**************************************************************************
  Returns the coordinates for the given city tile index taking into account
  the city radius.
**************************************************************************/
bool city_tile_index_to_xy(int *city_map_x, int *city_map_y,
                           int city_tile_index, int city_radius)
{
  assert(city_radius >= CITY_MAP_RADIUS);
  assert(city_radius <= CITY_MAP_RADIUS);

  /* tile indices are sorted from smallest to largest city radius */
  if (city_tile_index < 0
      || city_tile_index >= city_tiles) {
    return FALSE;
  }

  *city_map_x = CITY_MAP_RADIUS + city_map_index[city_tile_index].dx;
  *city_map_y = CITY_MAP_RADIUS + city_map_index[city_tile_index].dy;

  return TRUE;
}

/**************************************************************************
  Return the number of tiles for the given city radius. Special case is
  the value -1 for no city tiles.
**************************************************************************/
int city_map_tiles(int city_radius)
{
  if (city_radius == -1) {
    /* special case: minimal index value = city center */
    return 0;
  }

  assert(city_radius >= CITY_MAP_RADIUS);
  assert(city_radius <= CITY_MAP_RADIUS);

  return city_tiles;
}

/**************************************************************************
  Return TRUE if the given city coordinate pair is "valid"; that is, if it
  is a part of the citymap and thus is workable by the city.
**************************************************************************/
bool is_valid_city_coords(const int city_x, const int city_y)
{
  int dist = map_vector_to_sq_distance(city_x - CITY_MAP_RADIUS,
				       city_y - CITY_MAP_RADIUS);

  /* The city's valid positions are in a circle of radius CITY_MAP_RADIUS
   * around the city center.  Depending on the value of CITY_MAP_RADIUS
   * this circle will be:
   *
   *   333
   *  32223
   * 3211123
   * 3210123
   * 3211123
   *  32223
   *   333
   *
   * So CITY_MAP_RADIUS==2 corresponds to the "traditional" city map.
   *
   * This diagram is for rectangular topologies only.  But this is taken
   * care of inside map_vector_to_sq_distance so it works for all topologies.
   */
  return dist <= CITY_MAP_RADIUS_SQ;
}

/**************************************************************************
  Finds the city map coordinate for a given map position and a city
  center. Returns whether the map position is inside of the city map.
**************************************************************************/
bool city_tile_to_city_map(int *city_map_x, int *city_map_y,
			   const struct tile *city_center,
			   const struct tile *map_tile)
{
  map_distance_vector(city_map_x, city_map_y, city_center, map_tile);
  *city_map_x += CITY_MAP_RADIUS;
  *city_map_y += CITY_MAP_RADIUS;
  return is_valid_city_coords(*city_map_x, *city_map_y);
}

/**************************************************************************
Finds the city map coordinate for a given map position and a
city. Returns whether the map position is inside of the city map.
**************************************************************************/
bool city_base_to_city_map(int *city_map_x, int *city_map_y,
			   const struct city *const pcity,
			   const struct tile *map_tile)
{
  return city_tile_to_city_map(city_map_x, city_map_y, pcity->tile, map_tile);
}

/**************************************************************************
Finds the map position for a given city map coordinate of a certain
city. Returns true if the map position found is real.
**************************************************************************/
struct tile *city_map_to_tile(const struct tile *city_center,
			      int city_map_x, int city_map_y)
{
  int x, y;

  assert(is_valid_city_coords(city_map_x, city_map_y));
  x = city_center->x + city_map_x - CITY_MAP_SIZE / 2;
  y = city_center->y + city_map_y - CITY_MAP_SIZE / 2;

  return map_pos_to_tile(x, y);
}

/**************************************************************************
  Compare two integer values, as required by qsort.
***************************************************************************/
static int cmp(int v1, int v2)
{
  if (v1 == v2) {
    return 0;
  } else if (v1 > v2) {
    return 1;
  } else {
    return -1;
  }
}

/**************************************************************************
  Compare two iter_index values from the city_map_iterate_outward_indices.

  This function will be passed to qsort().  It should never return zero,
  or the sort order will be left up to qsort and will be undefined.  This
  would mean that server execution would not be reproducable.
***************************************************************************/
int compare_iter_index(const void *a, const void *b)
{
  const struct iter_index *index1 = a, *index2 = b;
  int value;

  value = cmp(index1->dist, index2->dist);
  if (value != 0) {
    return value;
  }

  value = cmp(index1->dx, index2->dx);
  if (value != 0) {
    return value;
  }

  value = cmp(index1->dy, index2->dy);
  assert(value != 0);
  return value;
}

/**************************************************************************
  Fill the iterate_outwards_indices array.  This may depend on topology and
  ruleset settings.
***************************************************************************/
void generate_city_map_indices(void)
{
  int i = 0, dx, dy;
  struct iter_index *array = city_map_index;

  /* We don't use city-map iterators in this function because they may
   * rely on the indices that have not yet been generated. */

  city_tiles = 0;
  for (dx = -CITY_MAP_RADIUS; dx <= CITY_MAP_RADIUS; dx++) {
    for (dy = -CITY_MAP_RADIUS; dy <= CITY_MAP_RADIUS; dy++) {
      if (is_valid_city_coords(dx + CITY_MAP_RADIUS, dy + CITY_MAP_RADIUS)) {
	city_tiles++;
      }
    }
  }

  /* Realloc is used because this function may be called multiple times. */
  array = fc_realloc(array, CITY_TILES * sizeof(*array));

  for (dx = -CITY_MAP_RADIUS; dx <= CITY_MAP_RADIUS; dx++) {
    for (dy = -CITY_MAP_RADIUS; dy <= CITY_MAP_RADIUS; dy++) {
      if (is_valid_city_coords(dx + CITY_MAP_RADIUS, dy + CITY_MAP_RADIUS)) {
	array[i].dx = dx;
	array[i].dy = dy;
	array[i].dist = map_vector_to_sq_distance(dx, dy);
	i++;
      }
    }
  }
  assert(i == CITY_TILES);

  qsort(array, CITY_TILES, sizeof(*array), compare_iter_index);

#ifdef DEBUG
  for (i = 0; i < CITY_TILES; i++) {
    freelog(LOG_DEBUG, "%2d : (%2d,%2d) : %d", i,
	    array[i].dx + CITY_MAP_RADIUS, array[i].dy + CITY_MAP_RADIUS,
	    array[i].dist);
  }
#endif

  city_map_index = array;

  cm_init_citymap();
}


/****************************************************************************
  Return an id string for the output type.  This string can be used
  internally by rulesets and tilesets and should not be changed or
  translated.
*****************************************************************************/
const char *get_output_identifier(Output_type_id output)
{
  if (output < 0 || output >= O_LAST) {
    assert(0);
    return NULL;
  }
  return output_types[output].id;
}

/****************************************************************************
  Return a translated name for the output type.  This name should only be
  used for user display.
*****************************************************************************/
const char *get_output_name(Output_type_id output)
{
  if (output < 0 || output >= O_LAST) {
    assert(0);
    return NULL;
  }
  return _(output_types[output].name);
}

/****************************************************************************
  Return the output type for this index.
****************************************************************************/
struct output_type *get_output_type(Output_type_id output)
{
  if (output < 0 || output >= O_LAST) {
    assert(0);
    return NULL;
  }
  return &output_types[output];
}

/**************************************************************************
  Find the output type for this output identifier.
**************************************************************************/
Output_type_id find_output_type_by_identifier(const char *id)
{
  Output_type_id o;

  for (o = 0; o < O_LAST; o++) {
    if (mystrcasecmp(output_types[o].name, id) == 0) {
      return o;
    }
  }

  return O_LAST;
}

/**************************************************************************
  Return the extended name of the building.
**************************************************************************/
const char *city_improvement_name_translation(const struct city *pcity,
					      struct impr_type *pimprove)
{
  static char buffer[256];
  const char *state = NULL;

  if (is_great_wonder(pimprove)) {
    if (great_wonder_is_available(pimprove)) {
      state = Q_("?wonder:W");
    } else if (great_wonder_is_destroyed(pimprove)) {
      state = Q_("?destroyed:D");
    } else {
      state = Q_("?built:B");
    }
  }
  if (pcity) {
    struct player *pplayer = city_owner(pcity);

    if (improvement_obsolete(pplayer, pimprove)) {
      state = Q_("?obsolete:O");
    } else if (is_building_replaced(pcity, pimprove, RPT_CERTAIN)) {
      /* Mark building redundant only if we are CERTAIN that it has no use. */
      state = Q_("?redundant:*");
    }
  }

  if (state) {
    my_snprintf(buffer, sizeof(buffer), "%s(%s)",
		improvement_name_translation(pimprove),
		state); 
    return buffer;
  } else {
    return improvement_name_translation(pimprove);
  }
}

/**************************************************************************
  Return the extended name of the current production.
**************************************************************************/
const char *city_production_name_translation(const struct city *pcity)
{
  static char buffer[256];

  switch (pcity->production.kind) {
  case VUT_IMPROVEMENT:
    return city_improvement_name_translation(pcity, pcity->production.value.building);
  default:
    /* fallthru */
    break;
  };
  return universal_name_translation(&pcity->production, buffer, sizeof(buffer));
}

/**************************************************************************
  Return TRUE when the current production has this flag.
**************************************************************************/
bool city_production_has_flag(const struct city *pcity,
			      enum impr_flag_id flag)
{
  return VUT_IMPROVEMENT == pcity->production.kind
      && improvement_has_flag(pcity->production.value.building, flag);
}

/**************************************************************************
  Return the number of shields it takes to build current city production.
**************************************************************************/
int city_production_build_shield_cost(const struct city *pcity)
{
  return universal_build_shield_cost(&pcity->production);
}

/**************************************************************************
  Return the cost (gold) to buy the current city production.
**************************************************************************/
int city_production_buy_gold_cost(const struct city *pcity)
{
  int build = pcity->shield_stock;

  switch (pcity->production.kind) {
  case VUT_IMPROVEMENT:
    return impr_buy_gold_cost(pcity->production.value.building,
			      build);
  case VUT_UTYPE:
    return utype_buy_gold_cost(pcity->production.value.utype,
			       build);
  default:
    break;
  };
  return FC_INFINITY;
}

/**************************************************************************
 Calculates the turns which are needed to build the requested
 production in the city.  GUI Independent.
**************************************************************************/
int city_production_turns_to_build(const struct city *pcity,
				   bool include_shield_stock)
{
  return city_turns_to_build(pcity, pcity->production, include_shield_stock);
}

/**************************************************************************
  Return whether given city can build given building, ignoring whether
  it is obsolete.
**************************************************************************/
bool can_city_build_improvement_direct(const struct city *pcity,
				       struct impr_type *pimprove)
{
  if (!can_player_build_improvement_direct(city_owner(pcity), pimprove)) {
    return FALSE;
  }

  if (city_has_building(pcity, pimprove)) {
    return FALSE;
  }

  return are_reqs_active(city_owner(pcity), pcity, NULL,
			 pcity->tile, NULL, NULL, NULL,
			 &(pimprove->reqs), RPT_CERTAIN);
}

/**************************************************************************
  Return whether given city can build given building; returns FALSE if
  the building is obsolete.
**************************************************************************/
bool can_city_build_improvement_now(const struct city *pcity,
				    struct impr_type *pimprove)
{  
  if (!can_city_build_improvement_direct(pcity, pimprove)) {
    return FALSE;
  }
  if (improvement_obsolete(city_owner(pcity), pimprove)) {
    return FALSE;
  }
  return TRUE;
}

/**************************************************************************
  Return whether player can eventually build given building in the city;
  returns FALSE if improvement can never possibly be built in this city.
**************************************************************************/
bool can_city_build_improvement_later(const struct city *pcity,
				      struct impr_type *pimprove)
{
  /* Can the _player_ ever build this improvement? */
  if (!can_player_build_improvement_later(city_owner(pcity), pimprove)) {
    return FALSE;
  }

  /* Check for requirements that aren't met and that are unchanging (so
   * they can never be met). */
  requirement_vector_iterate(&pimprove->reqs, preq) {
    if (is_req_unchanging(preq)
	&& !is_req_active(city_owner(pcity), pcity, NULL,
	  		  pcity->tile, NULL, NULL, NULL, preq, RPT_POSSIBLE)) {
      return FALSE;
    }
  } requirement_vector_iterate_end;
  return TRUE;
}

/**************************************************************************
  Return whether given city can build given unit, ignoring whether unit 
  is obsolete.
**************************************************************************/
bool can_city_build_unit_direct(const struct city *pcity,
				const struct unit_type *punittype)
{
  if (!can_player_build_unit_direct(city_owner(pcity), punittype)) {
    return FALSE;
  }

  /* Check to see if the unit has a building requirement. */
  if (punittype->need_improvement
   && !city_has_building(pcity, punittype->need_improvement)) {
    return FALSE;
  }

  /* You can't build naval units inland. */
  if (!uclass_has_flag(utype_class(punittype), UCF_BUILD_ANYWHERE)
      && !is_native_near_tile(punittype, pcity->tile)) {
    return FALSE;
  }
  return TRUE;
}

/**************************************************************************
  Return whether given city can build given unit; returns FALSE if unit is 
  obsolete.
**************************************************************************/
bool can_city_build_unit_now(const struct city *pcity,
			     const struct unit_type *punittype)
{  
  if (!can_city_build_unit_direct(pcity, punittype)) {
    return FALSE;
  }
  while ((punittype = punittype->obsoleted_by) != U_NOT_OBSOLETED) {
    if (can_player_build_unit_direct(city_owner(pcity), punittype)) {
	return FALSE;
    }
  }
  return TRUE;
}

/**************************************************************************
  Return whether player can eventually build given unit in the city;
  returns FALSE if unit can never possibly be built in this city.
**************************************************************************/
bool can_city_build_unit_later(const struct city *pcity,
			       const struct unit_type *punittype)
{
  /* Can the _player_ ever build this unit? */
  if (!can_player_build_unit_later(city_owner(pcity), punittype)) {
    return FALSE;
  }

  /* Some units can be built only in certain cities -- for instance,
     ships may be built only in cities adjacent to ocean. */
  if (!uclass_has_flag(utype_class(punittype), UCF_BUILD_ANYWHERE)
      && !is_native_near_tile(punittype, pcity->tile)) {
    return FALSE;
  }

  return TRUE;
}

/**************************************************************************
  ...
**************************************************************************/
bool can_city_build_direct(const struct city *pcity,
			   struct universal target)
{
  switch (target.kind) {
  case VUT_UTYPE:
    return can_city_build_unit_direct(pcity, target.value.utype);
  case VUT_IMPROVEMENT:
    return can_city_build_improvement_direct(pcity, target.value.building);
  default:
    break;
  };
  return FALSE;
}

/**************************************************************************
  ...
**************************************************************************/
bool can_city_build_now(const struct city *pcity,
			struct universal target)
{
  switch (target.kind) {
  case VUT_UTYPE:
    return can_city_build_unit_now(pcity, target.value.utype);
  case VUT_IMPROVEMENT:
    return can_city_build_improvement_now(pcity, target.value.building);
  default:
    break;
  };
  return FALSE;
}

/**************************************************************************
  ...
**************************************************************************/
bool can_city_build_later(const struct city *pcity,
			  struct universal target)
{
  switch (target.kind) {
  case VUT_UTYPE:
    return can_city_build_unit_later(pcity, target.value.utype);
  case VUT_IMPROVEMENT:
    return can_city_build_improvement_later(pcity, target.value.building);
  default:
    break;
  };
  return FALSE;
}

/****************************************************************************
  Returns TRUE iff the given city can use this kind of specialist.
****************************************************************************/
bool city_can_use_specialist(const struct city *pcity,
			     Specialist_type_id type)
{
  return are_reqs_active(city_owner(pcity), pcity, NULL,
			 NULL, NULL, NULL, NULL,
			 &specialist_by_number(type)->reqs, RPT_POSSIBLE);
}

/****************************************************************************
  Returns TRUE iff the given city can change what it is building
****************************************************************************/
bool city_can_change_build(const struct city *pcity)
{
  return !pcity->did_buy || pcity->shield_stock <= 0;
}

/**************************************************************************
  Always tile_set_owner(ptile, pplayer) sometime before this!
**************************************************************************/
void city_choose_build_default(struct city *pcity)
{
  if (NULL == city_tile(pcity)) {
    /* When a "dummy" city is created with no tile, then choosing a build 
     * target could fail.  This currently might happen during map editing.
     * FIXME: assumes the first unit is always "valid", so check for
     * obsolete units elsewhere. */
    pcity->production.kind = VUT_UTYPE;
    pcity->production.value.utype = utype_by_number(0);
  } else {
    struct unit_type *u = best_role_unit(pcity, L_FIRSTBUILD);

    if (u) {
      pcity->production.kind = VUT_UTYPE;
      pcity->production.value.utype = u;
    } else {
      bool found = FALSE;

      /* Just pick the first available item. */

      improvement_iterate(pimprove) {
	if (can_city_build_improvement_direct(pcity, pimprove)) {
	  found = TRUE;
	  pcity->production.kind = VUT_IMPROVEMENT;
	  pcity->production.value.building = pimprove;
	  break;
	}
      } improvement_iterate_end;

      if (!found) {
	unit_type_iterate(punittype) {
	  if (can_city_build_unit_direct(pcity, punittype)) {
	    found = TRUE;
	    pcity->production.kind = VUT_UTYPE;
	    pcity->production.value.utype = punittype;
	  }
	} unit_type_iterate_end;
      }

      assert(found);
    }
  }
}

#ifndef city_name
/**************************************************************************
  Return the name of the city.
**************************************************************************/
const char *city_name(const struct city *pcity)
{
  return pcity->name;
}
#endif

/**************************************************************************
  Return the owner of the city.
**************************************************************************/
struct player *city_owner(const struct city *pcity)
{
  assert(NULL != pcity);
  assert(NULL != pcity->owner);
  return pcity->owner;
}

#ifndef city_tile
/**************************************************************************
  Return the tile location of the city.
  Not (yet) always used, mostly for debugging.
**************************************************************************/
struct tile *city_tile(const struct city *pcity)
{
  return pcity->tile;
}
#endif

/**************************************************************************
 Returns how many thousand citizen live in this city.
**************************************************************************/
int city_population(const struct city *pcity)
{
  /*  Sum_{i=1}^{n} i  ==  n*(n+1)/2  */
  return pcity->size * (pcity->size + 1) * 5;
}

/**************************************************************************
  Returns the total amount of gold needed to pay for all buildings in the
  city.
**************************************************************************/
int city_total_impr_gold_upkeep(const struct city *pcity)
{
  int gold_needed = 0;

  if (!pcity) {
    return 0;
  }

  city_built_iterate(pcity, pimprove) {
      gold_needed += city_improvement_upkeep(pcity, pimprove);
  } city_built_iterate_end;

  return gold_needed;
}

/***************************************************************************
  Get the total amount of gold needed to pay upkeep costs for all supported
  units of the city. Takes into account EFT_UNIT_UPKEEP_FREE_PER_CITY.
***************************************************************************/
int city_total_unit_gold_upkeep(const struct city *pcity)
{
  int gold_needed = 0;

  if (!pcity || !pcity->units_supported
      || unit_list_size(pcity->units_supported) < 1) {
    return 0;
  }

  unit_list_iterate(pcity->units_supported, punit) {
    gold_needed += punit->upkeep[O_GOLD];
  } unit_list_iterate_end;

  return gold_needed;
}

/**************************************************************************
  Return TRUE iff the city has this building in it.
**************************************************************************/
bool city_has_building(const struct city *pcity,
		       const struct impr_type *pimprove)
{
  if (NULL == pimprove) {
    /* callers should ensure that any external data is tested with 
     * valid_improvement_by_number() */
    return FALSE;
  }
  return (pcity->built[improvement_index(pimprove)].turn > I_NEVER);
}

/**************************************************************************
  Return the upkeep (gold) needed each turn to upkeep the given improvement
  in the given city.
**************************************************************************/
int city_improvement_upkeep(const struct city *pcity,
			    const struct impr_type *b)
{
  int upkeep;

  if (NULL == b)
    return 0;
  if (is_wonder(b))
    return 0;

  upkeep = b->upkeep;
  if (upkeep <= get_building_bonus(pcity, b, EFT_UPKEEP_FREE)) {
    return 0;
  }
  
  return upkeep;
}

/**************************************************************************
  Calculate the output for the tile.
  pcity may be NULL.
  is_celebrating may be speculative.
  otype is the output type (generally O_FOOD, O_TRADE, or O_SHIELD).

  This can be used to calculate the benefits celebration would give.
**************************************************************************/
int city_tile_output(const struct city *pcity, const struct tile *ptile,
		     bool is_celebrating, Output_type_id otype)
{
  int prod;
  struct terrain *pterrain = tile_terrain(ptile);

  assert(otype >= 0 && otype < O_LAST);

  if (T_UNKNOWN == pterrain) {
    /* Special case for the client.  The server doesn't allow unknown tiles
     * to be worked but we don't necessarily know what player is involved. */
    return 0;
  }

  prod = pterrain->output[otype];
  if (tile_resource_is_valid(ptile)) {
    prod += tile_resource(ptile)->output[otype];
  }

  switch (otype) {
  case O_SHIELD:
    if (tile_has_special(ptile, S_MINE)) {
      prod += pterrain->mining_shield_incr;
    }
    break;
  case O_FOOD:
    /* The city center tile is auto-irrigated. */
    if (tile_has_special(ptile, S_IRRIGATION)
        || (NULL != pcity
            && is_city_center(pcity, ptile)
            && pterrain == pterrain->irrigation_result
            && terrain_control.may_irrigate)) {
      prod += pterrain->irrigation_food_incr;
    }
    break;
  case O_TRADE:
    if (tile_has_special(ptile, S_RIVER) && !is_ocean_tile(ptile)) {
      prod += terrain_control.river_trade_incr;
    }
    if (tile_has_special(ptile, S_ROAD)) {
      prod += pterrain->road_trade_incr;
    }
    break;
  case O_GOLD:
  case O_SCIENCE:
  case O_LUXURY:
  case O_LAST:
    break;
  }

  if (tile_has_special(ptile, S_RAILROAD)) {
    prod += (prod * terrain_control.rail_tile_bonus[otype]) / 100;
  }

  if (pcity) {
    const struct output_type *output = &output_types[otype];

    prod += get_city_tile_output_bonus(pcity, ptile, output,
				       EFT_OUTPUT_ADD_TILE);
    if (prod > 0) {
      int penalty_limit = get_city_tile_output_bonus(pcity, ptile, output,
                                                   EFT_OUTPUT_PENALTY_TILE);

      if (is_celebrating) {
        prod += get_city_tile_output_bonus(pcity, ptile, output,
                                           EFT_OUTPUT_INC_TILE_CELEBRATE);
        penalty_limit = 0; /* no penalty if celebrating */
      }
      prod += get_city_tile_output_bonus(pcity, ptile, output,
                                         EFT_OUTPUT_INC_TILE);
      prod += (prod 
               * get_city_tile_output_bonus(pcity, ptile, output,
                                            EFT_OUTPUT_PER_TILE)) 
              / 100;
      if (!is_celebrating && penalty_limit > 0 && prod > penalty_limit) {
        prod--;
      }
    }
  }

  if (tile_has_special(ptile, S_POLLUTION)) {
    prod -= (prod * terrain_control.pollution_tile_penalty[otype]) / 100;
  }

  if (tile_has_special(ptile, S_FALLOUT)) {
    prod -= (prod * terrain_control.fallout_tile_penalty[otype]) / 100;
  }

  if (NULL != pcity && is_city_center(pcity, ptile)) {
    prod = MAX(prod, game.info.min_city_center_output[otype]);
  }

  return prod;
}

/**************************************************************************
  Calculate the production output the given tile is capable of producing
  for the city.  The output type is given by 'otype' (generally O_FOOD,
  O_SHIELD, or O_TRADE).
**************************************************************************/
int city_tile_output_now(const struct city *pcity, const struct tile *ptile,
			 Output_type_id otype)
{
  return city_tile_output(pcity, ptile, city_celebrating(pcity), otype);
}

/**************************************************************************
  Returns TRUE when a tile is available to be worked, or the city itself is
  currently working the tile (and can continue).
**************************************************************************/
bool city_can_work_tile(const struct city *pcity, const struct tile *ptile)
{
  struct player *powner = city_owner(pcity);

  if (NULL == ptile) {
    return FALSE;
  }

  if (NULL != tile_owner(ptile) && tile_owner(ptile) != powner) {
    return FALSE;
  }
  /* TODO: civ3-like option for borders */

  if (NULL != tile_worked(ptile) && tile_worked(ptile) != pcity) {
    return FALSE;
  }

  if (TILE_KNOWN_SEEN != tile_get_known(ptile, powner)) {
    return FALSE;
  }

  if (!is_free_worked(pcity, ptile)
   && NULL != unit_occupies_tile(ptile, powner)) {
    return FALSE;
  }

  if (!get_city_tile_output_bonus(pcity, ptile, NULL, EFT_TILE_WORKABLE)) {
    return FALSE;
  }

  return TRUE;
}

/**************************************************************************
  Returns TRUE if the given unit can build a city at the given map
  coordinates.

  punit is the founding unit.  It may be NULL if a city is built out of the
  blue (e.g., through editing).
***************************************************************************/
bool city_can_be_built_here(const struct tile *ptile, const struct unit *punit)
{
  int citymindist;

  if (terrain_has_flag(tile_terrain(ptile), TER_NO_CITIES)) {
    /* No cities on this terrain. */
    return FALSE;
  }

  if (punit && !can_unit_exist_at_tile(punit, ptile)) {
    /* We allow land units to build land cities and sea units to build
     * ocean cities. Air units can build cities anywhere. */
    return FALSE;
  }

  if (punit && tile_owner(ptile) && tile_owner(ptile) != unit_owner(punit)) {
    /* Cannot steal borders by settling. This has to be settled by
     * force of arms. */
    return FALSE;
  }

  /* game.info.min_dist_bw_cities minimum is 1, meaning adjacent is okay */
  citymindist = game.info.citymindist;
  if (citymindist == 0) {
    citymindist = game.info.min_dist_bw_cities;
  }
  square_iterate(ptile, citymindist - 1, ptile1) {
    if (tile_city(ptile1)) {
      return FALSE;
    }
  } square_iterate_end;

  return TRUE;
}

/**************************************************************************
  Return TRUE iff the two cities are capable of trade; i.e., if a caravan
  from one city can enter the other to sell its goods.

  See also can_establish_trade_route().
**************************************************************************/
bool can_cities_trade(const struct city *pc1, const struct city *pc2)
{
  /* If you change the logic here, make sure to update the help in
   * helptext_unit(). */
  return (pc1 && pc2 && pc1 != pc2
          && (city_owner(pc1) != city_owner(pc2)
              || map_distance(pc1->tile, pc2->tile)
                 >= game.info.trademindist));
}

/**************************************************************************
  Find the worst (minimum) trade route the city has.  The value of the
  trade route is returned and its position (slot) is put into the slot
  variable.
**************************************************************************/
int get_city_min_trade_route(const struct city *pcity, int *slot)
{
  int i, value = pcity->trade_value[0];

  if (slot) {
    *slot = 0;
  }
  /* find min */
  for (i = 1; i < NUM_TRADEROUTES; i++) {
    if (value > pcity->trade_value[i]) {
      if (slot) {
	*slot = i;
      }
      value = pcity->trade_value[i];
    }
  }

  return value;
}

/**************************************************************************
  Returns TRUE iff the two cities can establish a trade route.  We look
  at the distance and ownership of the cities as well as their existing
  trade routes.  Should only be called if you already know that
  can_cities_trade().
**************************************************************************/
bool can_establish_trade_route(const struct city *pc1, const struct city *pc2)
{
  int trade = -1;

  assert(can_cities_trade(pc1, pc2));

  if (!pc1 || !pc2 || pc1 == pc2
      || have_cities_trade_route(pc1, pc2)) {
    return FALSE;
  }
    
  if (city_num_trade_routes(pc1) == NUM_TRADEROUTES) {
    trade = trade_between_cities(pc1, pc2);
    /* can we replace traderoute? */
    if (get_city_min_trade_route(pc1, NULL) >= trade) {
      return FALSE;
    }
  }
  
  if (city_num_trade_routes(pc2) == NUM_TRADEROUTES) {
    if (trade == -1) {
      trade = trade_between_cities(pc1, pc2);
    }
    /* can we replace traderoute? */
    if (get_city_min_trade_route(pc2, NULL) >= trade) {
      return FALSE;
    }
  }  

  return TRUE;
}

/**************************************************************************
  Return the trade that exists between these cities, assuming they have a
  trade route.
**************************************************************************/
int trade_between_cities(const struct city *pc1, const struct city *pc2)
{
  int bonus = 0;

  if (NULL != pc1 && NULL != pc1->tile
   && NULL != pc2 && NULL != pc2->tile) {
    bonus = real_map_distance(pc1->tile, pc2->tile) + pc1->size + pc2->size;
    bonus /= 8;

    if (city_owner(pc1) == city_owner(pc2)) {
      bonus /= 2;
    }
  }
  return bonus;
}

/**************************************************************************
 Return number of trade route city has
**************************************************************************/
int city_num_trade_routes(const struct city *pcity)
{
  int i, n = 0;

  for (i = 0; i < NUM_TRADEROUTES; i++)
    if(pcity->trade[i] != 0) n++;
  
  return n;
}

/**************************************************************************
  Returns the revenue trade bonus - you get this when establishing a
  trade route and also when you simply sell your trade goods at the
  new city.

  Note if you trade with a city you already have a trade route with,
  you'll only get 1/3 of this value.
**************************************************************************/
int get_caravan_enter_city_trade_bonus(const struct city *pc1, 
                                       const struct city *pc2)
{
  int tb, bonus;

  /* Should this be real_map_distance? */
  tb = map_distance(pc1->tile, pc2->tile) + 10;
  tb = (tb * (pc1->surplus[O_TRADE] + pc2->surplus[O_TRADE])) / 24;

  /*  fudge factor to more closely approximate Civ2 behavior (Civ2 is
   * really very different -- this just fakes it a little better) */
  tb *= 3;
  
  /* Trade_revenue_bonus increases revenue by power of 2 in milimes */
  bonus = get_city_bonus(pc1, EFT_TRADE_REVENUE_BONUS);
  
  tb = (float)tb * pow(2.0, (double)bonus / 1000.0);

  return tb;
}

/**************************************************************************
  Check if cities have an established trade route.
**************************************************************************/
bool have_cities_trade_route(const struct city *pc1, const struct city *pc2)
{
  int i;
  
  for (i = 0; i < NUM_TRADEROUTES; i++) {
    if (pc1->trade[i] == pc2->id || pc2->trade[i] == pc1->id) {
      /* Looks like they do have a traderoute. */
      return TRUE;
    }
  }
  return FALSE;
}

/**************************************************************************
  Return TRUE iff this city is its nation's capital.  The capital city is
  special-cased in a number of ways.
**************************************************************************/
bool is_capital(const struct city *pcity)
{
  return (get_city_bonus(pcity, EFT_CAPITAL_CITY) != 0);
}

/**************************************************************************
 Whether a city should have visible walls
**************************************************************************/
bool city_got_citywalls(const struct city *pcity)
{
  return (get_city_bonus(pcity, EFT_VISIBLE_WALLS) > 0);
}

/**************************************************************************
 This can be City Walls, Coastal defense... depending on attacker type.
 If attacker type is not given, just any defense effect will do.
**************************************************************************/
bool city_got_defense_effect(const struct city *pcity,
                             const struct unit_type *attacker)
{
  if (!attacker) {
    /* Any defense building will do */
    return get_city_bonus(pcity, EFT_DEFEND_BONUS) > 0;
  }

  return get_unittype_bonus(city_owner(pcity), pcity->tile, attacker,
                            EFT_DEFEND_BONUS) > 0;
}

/**************************************************************************
  Return TRUE iff the city is happy.  A happy city will start celebrating
  soon.
  A city can only be happy if half or more of the population is happy,
  none of the population is unhappy or angry, and it has sufficient size.
**************************************************************************/
bool city_happy(const struct city *pcity)
{
  return (pcity->size >= game.info.celebratesize
	  && pcity->feel[CITIZEN_ANGRY][FEELING_FINAL] == 0
	  && pcity->feel[CITIZEN_UNHAPPY][FEELING_FINAL] == 0
	  && pcity->feel[CITIZEN_HAPPY][FEELING_FINAL] >= (pcity->size + 1) / 2);
}

/**************************************************************************
  Return TRUE iff the city is unhappy.  An unhappy city will start
  revolting soon.
**************************************************************************/
bool city_unhappy(const struct city *pcity)
{
  return (pcity->feel[CITIZEN_HAPPY][FEELING_FINAL]
	< pcity->feel[CITIZEN_UNHAPPY][FEELING_FINAL]
	  + 2 * pcity->feel[CITIZEN_ANGRY][FEELING_FINAL]);
}

/**************************************************************************
  Return TRUE if the city was celebrating at the start of the turn,
  and it still has sufficient size to be in rapture.
**************************************************************************/
bool base_city_celebrating(const struct city *pcity)
{
  return (pcity->size >= game.info.celebratesize && pcity->was_happy);
}

/**************************************************************************
cities celebrate only after consecutive happy turns
**************************************************************************/
bool city_celebrating(const struct city *pcity)
{
  return base_city_celebrating(pcity) && city_happy(pcity);
}

/**************************************************************************
.rapture is checked instead of city_celebrating() because this function is
called after .was_happy was updated.
**************************************************************************/
bool city_rapture_grow(const struct city *pcity)
{
  return (pcity->rapture > 0 && pcity->surplus[O_FOOD] > 0
	  && (pcity->rapture % game.info.rapturedelay) == 0
          && get_city_bonus(pcity, EFT_RAPTURE_GROW) > 0);
}

/**************************************************************************
...
**************************************************************************/
struct city *city_list_find_id(struct city_list *This, int id)
{
  if (id != 0) {
    city_list_iterate(This, pcity) {
      if (pcity->id == id) {
	return pcity;
      }
    } city_list_iterate_end;
  }

  return NULL;
}

/**************************************************************************
...
**************************************************************************/
struct city *city_list_find_name(struct city_list *This, const char *name)
{
  city_list_iterate(This, pcity) {
    if (mystrcasecmp(name, pcity->name) == 0) {
      return pcity;
    }
  } city_list_iterate_end;

  return NULL;
}

/**************************************************************************
Comparison function for qsort for city _pointers_, sorting by city name.
Args are really (struct city**), to sort an array of pointers.
(Compare with old_city_name_compare() in game.c, which use city_id's)
**************************************************************************/
int city_name_compare(const void *p1, const void *p2)
{
  return mystrcasecmp( (*(const struct city**)p1)->name,
		       (*(const struct city**)p2)->name );
}

/**************************************************************************
Evaluate which style should be used to draw a city.
**************************************************************************/
int style_of_city(const struct city *pcity)
{
  return city_style_of_player(city_owner(pcity));
}

/**************************************************************************
  Return the city style (used for drawing the city on the mapview in
  the client) for this city.  The city style depends on the
  start-of-game choice by the player as well as techs researched.
**************************************************************************/
int city_style_of_player(const struct player *plr)
{
  int replace, style, prev;

  style = plr->city_style;
  prev = style;

  while ((replace = city_styles[prev].replaced_by) != -1) {
    prev = replace;
    if (are_reqs_active(plr, NULL, NULL, NULL, NULL, NULL, NULL,
			&city_styles[replace].reqs, RPT_CERTAIN)) {
      style = replace;
    }
  }
  return style;
}

/****************************************************************************
  Returns the city style that has the given (translated) name.
  Returns -1 if none match.
****************************************************************************/
int find_city_style_by_translated_name(const char *s)
{
  int i;

  for (i = 0; i < game.control.styles_count; i++) {
    if (0 == strcmp(city_style_name_translation(i), s)) {
      return i;
    }
  }

  return -1;
}

/****************************************************************************
  Returns the city style that has the given (untranslated) rule name.
  Returns -1 if none match.
****************************************************************************/
int find_city_style_by_rule_name(const char *s)
{
  const char *qs = Qn_(s);
  int i;

  for (i = 0; i < game.control.styles_count; i++) {
    if (0 == mystrcasecmp(city_style_rule_name(i), qs)) {
      return i;
    }
  }

  return -1;
}

/****************************************************************************
  Return the (translated) name of the given city style. 
  You don't have to free the return pointer.
****************************************************************************/
const char *city_style_name_translation(const int style)
{
  struct citystyle *csp = &city_styles[style];

  if (NULL == csp->name.translated) {
    /* delayed (unified) translation */
    csp->name.translated = ('\0' == csp->name.vernacular[0])
			   ? csp->name.vernacular
			   : Q_(csp->name.vernacular);
  }
  return csp->name.translated;
}


/****************************************************************************
  Return the (untranslated) rule name of the city style.
  You don't have to free the return pointer.
****************************************************************************/
const char* city_style_rule_name(const int style)
{
   return Qn_(city_styles[style].name.vernacular);
}

/****************************************************************************
  Return whether the style has any requirements.  Styles without requirements
  are special cases since only these may be used as starting city styles.
****************************************************************************/
bool city_style_has_requirements(const struct citystyle *style)
{
  return (requirement_vector_size(&style->reqs) > 0);
}

/**************************************************************************
 Compute and optionally apply the change-production penalty for the given
 production change (to target) in the given city (pcity).
 Always returns the number of shields which would be in the stock if
 the penalty had been applied.

 If we switch the "class" of the target sometime after a city has produced
 (i.e., not on the turn immediately after), then there's a shield loss.
 But only on the first switch that turn.  Also, if ever change back to
 original improvement class of this turn, restore lost production.
**************************************************************************/
int city_change_production_penalty(const struct city *pcity,
				   struct universal target)
{
  int shield_stock_after_adjustment;
  enum production_class_type orig_class;
  enum production_class_type new_class;
  int unpenalized_shields = 0, penalized_shields = 0;

  switch (pcity->changed_from.kind) {
  case VUT_IMPROVEMENT:
    if (is_wonder(pcity->changed_from.value.building)) {
      orig_class = PCT_WONDER;
    } else {
      orig_class = PCT_NORMAL_IMPROVEMENT;
    }
    break;
  case VUT_UTYPE:
    orig_class = PCT_UNIT;
    break;
  default:
    orig_class = PCT_LAST;
    break;
  };

  switch (target.kind) {
  case VUT_IMPROVEMENT:
    if (is_wonder(target.value.building)) {
      new_class = PCT_WONDER;
    } else {
      new_class = PCT_NORMAL_IMPROVEMENT;
    }
    break;
  case VUT_UTYPE:
    new_class = PCT_UNIT;
    break;
  default:
    new_class = PCT_LAST;
    break;
  };

  /* Changing production is penalized under certain circumstances. */
  if (orig_class == new_class
   || orig_class == PCT_LAST) {
    /* There's never a penalty for building something of the same class. */
    unpenalized_shields = pcity->before_change_shields;
  } else if (city_built_last_turn(pcity)) {
    /* Surplus shields from the previous production won't be penalized if
     * you change production on the very next turn.  But you can only use
     * up to the city's surplus amount of shields in this way. */
    unpenalized_shields = MIN(pcity->last_turns_shield_surplus,
			      pcity->before_change_shields);
    penalized_shields = pcity->before_change_shields - unpenalized_shields;
  } else {
    /* Penalize 50% of the production. */
    penalized_shields = pcity->before_change_shields;
  }

  /* Do not put penalty on these. It shouldn't matter whether you disband unit
     before or after changing production...*/
  unpenalized_shields += pcity->disbanded_shields;

  /* Caravan shields are penalized (just as if you disbanded the caravan)
   * if you're not building a wonder. */
  if (new_class == PCT_WONDER) {
    unpenalized_shields += pcity->caravan_shields;
  } else {
    penalized_shields += pcity->caravan_shields;
  }

  shield_stock_after_adjustment =
    unpenalized_shields + penalized_shields / 2;

  return shield_stock_after_adjustment;
}

/**************************************************************************
 Calculates the turns which are needed to build the requested
 improvement in the city.  GUI Independent.
**************************************************************************/
int city_turns_to_build(const struct city *pcity,
			struct universal target,
			bool include_shield_stock)
{
  int city_shield_surplus = pcity->surplus[O_SHIELD];
  int city_shield_stock = include_shield_stock ?
      city_change_production_penalty(pcity, target) : 0;
  int cost = universal_build_shield_cost(&target);

  if (target.kind == VUT_IMPROVEMENT
      && is_great_wonder(target.value.building)
      && !great_wonder_is_available(target.value.building)) {
    return FC_INFINITY;
  }

  if (include_shield_stock && (city_shield_stock >= cost)) {
    return 1;
  } else if (city_shield_surplus > 0) {
    return (cost - city_shield_stock - 1) / city_shield_surplus + 1;
  } else {
    return FC_INFINITY;
  }
}

/**************************************************************************
 Calculates the turns which are needed for the city to grow.  A value
 of FC_INFINITY means the city will never grow.  A value of 0 means
 city growth is blocked.  A negative value of -x means the city will
 shrink in x turns.  A positive value of x means the city will grow in
 x turns.
**************************************************************************/
int city_turns_to_grow(const struct city *pcity)
{
  if (pcity->surplus[O_FOOD] > 0) {
    return (city_granary_size(pcity->size) - pcity->food_stock +
	    pcity->surplus[O_FOOD] - 1) / pcity->surplus[O_FOOD];
  } else if (pcity->surplus[O_FOOD] < 0) {
    /* turns before famine loss */
    return -1 + (pcity->food_stock / pcity->surplus[O_FOOD]);
  } else {
    return FC_INFINITY;
  }
}

/****************************************************************************
  Return TRUE iff the city can grow to the given size.
****************************************************************************/
bool city_can_grow_to(const struct city *pcity, int pop_size)
{
  return (get_city_bonus(pcity, EFT_SIZE_UNLIMIT) > 0
	  || pop_size <= get_city_bonus(pcity, EFT_SIZE_ADJ));
}

/**************************************************************************
 is there an enemy city on this tile?
**************************************************************************/
struct city *is_enemy_city_tile(const struct tile *ptile,
				const struct player *pplayer)
{
  struct city *pcity = tile_city(ptile);

  if (pcity && pplayers_at_war(pplayer, city_owner(pcity)))
    return pcity;
  else
    return NULL;
}

/**************************************************************************
 is there an friendly city on this tile?
**************************************************************************/
struct city *is_allied_city_tile(const struct tile *ptile,
				 const struct player *pplayer)
{
  struct city *pcity = tile_city(ptile);

  if (pcity && pplayers_allied(pplayer, city_owner(pcity)))
    return pcity;
  else
    return NULL;
}

/**************************************************************************
 is there an enemy city on this tile?
**************************************************************************/
struct city *is_non_attack_city_tile(const struct tile *ptile,
				     const struct player *pplayer)
{
  struct city *pcity = tile_city(ptile);

  if (pcity && pplayers_non_attack(pplayer, city_owner(pcity)))
    return pcity;
  else
    return NULL;
}

/**************************************************************************
 is there an non_allied city on this tile?
**************************************************************************/
struct city *is_non_allied_city_tile(const struct tile *ptile,
				     const struct player *pplayer)
{
  struct city *pcity = tile_city(ptile);

  if (pcity && !pplayers_allied(pplayer, city_owner(pcity)))
    return pcity;
  else
    return NULL;
}

/**************************************************************************
  Return TRUE if there is a friendly city near to this unit (within 3
  steps).
**************************************************************************/
bool is_unit_near_a_friendly_city(const struct unit *punit)
{
  return is_friendly_city_near(unit_owner(punit), punit->tile);
}

/**************************************************************************
  Return TRUE if there is a friendly city near to this tile (within 3
  steps).
**************************************************************************/
bool is_friendly_city_near(const struct player *owner,
			   const struct tile *ptile)
{
  square_iterate(ptile, 3, ptile1) {
    struct city *pcity = tile_city(ptile1);
    if (pcity && pplayers_allied(owner, city_owner(pcity))) {
      return TRUE;
    }
  } square_iterate_end;

  return FALSE;
}

/**************************************************************************
  Return true iff a city exists within a city radius of the given 
  location. may_be_on_center determines if a city at x,y counts.
**************************************************************************/
bool city_exists_within_city_radius(const struct tile *ptile,
				    bool may_be_on_center)
{
  city_tile_iterate(ptile, ptile1) {
    if (may_be_on_center || !same_pos(ptile, ptile1)) {
      if (tile_city(ptile1)) {
	return TRUE;
      }
    }
  } city_tile_iterate_end;

  return FALSE;
}

/****************************************************************************
  Generalized formula used to calculate granary size.

  The AI may not deal well with non-default settings.  See food_weighting().
****************************************************************************/
int city_granary_size(int city_size)
{
  int food_inis = game.info.granary_num_inis;
  int food_inc = game.info.granary_food_inc;
  int base_value;

  /* Granary sizes for the first food_inis citizens are given directly.
   * After that we increase the granary size by food_inc per citizen. */
  if (city_size > food_inis) {
    base_value = game.info.granary_food_ini[food_inis - 1];
    base_value += food_inc * (city_size - food_inis);
  } else {
    base_value = game.info.granary_food_ini[city_size - 1];
  }

  return MAX(base_value * game.info.foodbox / 100, 1);
}

/**************************************************************************
  Give base number of content citizens in any city owner by pplayer.
**************************************************************************/
static int content_citizens(const struct player *pplayer)
{
  int cities = city_list_size(pplayer->cities);
  int content = get_player_bonus(pplayer, EFT_CITY_UNHAPPY_SIZE);
  int basis = get_player_bonus(pplayer, EFT_EMPIRE_SIZE_BASE);
  int step = get_player_bonus(pplayer, EFT_EMPIRE_SIZE_STEP);

  if (basis + step <= 0) {
    return content; /* Value of zero means effect is inactive */
  }

  if (cities > basis) {
    content--;
    if (step != 0) {
      /* the first penalty is at (basis + 1) cities;
         the next is at (basis + step + 1), _not_ (basis + step) */
      content -= (cities - basis - 1) / step;
    }
  }
  return content;
}

/**************************************************************************
 Return the factor (in %) by which the city's output should be multiplied.
**************************************************************************/
int get_final_city_output_bonus(const struct city *pcity, Output_type_id otype)
{
  struct output_type *output = &output_types[otype];
  int bonus1 = 100 + get_city_tile_output_bonus(pcity, NULL, output,
						EFT_OUTPUT_BONUS);
  int bonus2 = 100 + get_city_tile_output_bonus(pcity, NULL, output,
						EFT_OUTPUT_BONUS_2);

  return MAX(bonus1 * bonus2 / 100, 0);
}

/**************************************************************************
  Return the amount of gold generated by buildings under "tithe" attribute
  governments.
**************************************************************************/
int get_city_tithes_bonus(const struct city *pcity)
{
  int tithes_bonus = 0;

  if (!get_city_bonus(pcity, EFT_HAPPINESS_TO_GOLD)) {
    return 0;
  }

  tithes_bonus += get_city_bonus(pcity, EFT_MAKE_CONTENT);
  tithes_bonus += get_city_bonus(pcity, EFT_FORCE_CONTENT);

  return tithes_bonus;
}

/**************************************************************************
  Add the incomes of a city according to the taxrates (ignore # of 
  specialists). trade should be in output[O_TRADE].
**************************************************************************/
void add_tax_income(const struct player *pplayer, int trade, int *output)
{
  const int SCIENCE = 0, TAX = 1, LUXURY = 2;
  int rates[3], result[3];

  if (game.info.changable_tax) {
    rates[SCIENCE] = pplayer->economic.science;
    rates[LUXURY] = pplayer->economic.luxury;
    rates[TAX] = 100 - rates[SCIENCE] - rates[LUXURY];
  } else {
    rates[SCIENCE] = game.info.forced_science;
    rates[LUXURY] = game.info.forced_luxury;
    rates[TAX] = game.info.forced_gold;
  }
  
  /* ANARCHY */
  if (government_of_player(pplayer) == game.government_during_revolution) {
    rates[SCIENCE] = 0;
    rates[LUXURY] = 100;
    rates[TAX] = 0;
  }

  distribute(trade, 3, rates, result);

  output[O_SCIENCE] += result[SCIENCE];
  output[O_GOLD] += result[TAX];
  output[O_LUXURY] += result[LUXURY];
}

/**************************************************************************
  Return TRUE if the city built something last turn (meaning production
  was completed between last turn and this).
**************************************************************************/
bool city_built_last_turn(const struct city *pcity)
{
  return pcity->turn_last_built + 1 >= game.info.turn;
}

/****************************************************************************
  Calculate output (food, trade and shields) generated by the worked tiles
  of a city.  This will completely overwrite the output[] array.

  The city_map[] is only used for AI testing (not full_refresh).
****************************************************************************/
static inline void get_worked_tile_output(const struct city *pcity,
					  int *output, bool main_map)
{
  bool is_worked;
#ifdef CITY_DEBUGGING
  bool is_celebrating = base_city_celebrating(pcity);
#endif
  struct tile *pcenter = city_tile(pcity);

  memset(output, 0, O_LAST * sizeof(*output));
  
  city_tile_iterate_cxy(pcenter, ptile, x, y) {
    if (main_map) {
      struct city *pwork = tile_worked(ptile);

      is_worked = (NULL != pwork && pwork == pcity);
    } else {
      is_worked = (C_TILE_WORKER == pcity->city_map[x][y]);
    }

    if (is_worked) {
      output_type_iterate(o) {
#ifdef CITY_DEBUGGING
	/* This assertion never fails, but it's so slow that we disable
	 * it by default. */
	assert(pcity->tile_output[x][y][o]
	       == city_tile_output(pcity, ptile, is_celebrating, o));
#endif
	output[o] += pcity->tile_output[x][y][o];
      } output_type_iterate_end;
    }
  } city_tile_iterate_cxy_end;
}

/****************************************************************************
  Calculate output (gold, science, and luxury) generated by the specialists
  of a city.  The output[] array is not cleared but is just added to.
****************************************************************************/
void add_specialist_output(const struct city *pcity, int *output)
{
  specialist_type_iterate(sp) {
    int count = pcity->specialists[sp];

    output_type_iterate(stat) {
      int amount = get_specialist_output(pcity, sp, stat);

      output[stat] += count * amount;
    } output_type_iterate_end;
  } specialist_type_iterate_end;
}

/****************************************************************************
  This function sets all the values in the pcity->bonus[] array.
  Called near the beginning of city_refresh_from_main_map().

  It doesn't depend on anything else in the refresh and doesn't change
  as workers are moved around, but does change when buildings are built,
  etc.
****************************************************************************/
static inline void set_city_bonuses(struct city *pcity)
{
  output_type_iterate(o) {
    pcity->bonus[o] = get_final_city_output_bonus(pcity, o);
  } output_type_iterate_end;
}

/****************************************************************************
  This function sets all the values in the pcity->tile_output[] array.
  Called near the beginning of city_refresh_from_main_map().

  It doesn't depend on anything else in the refresh and doesn't change
  as workers are moved around, but does change when buildings are built,
  etc.
****************************************************************************/
static inline void set_city_tile_output(struct city *pcity)
{
  bool is_celebrating = base_city_celebrating(pcity);

  /* Any unreal tiles are skipped - these values should have been memset
   * to 0 when the city was created. */
  city_tile_iterate_cxy(pcity->tile, ptile, x, y) {
    output_type_iterate(o) {
      pcity->tile_output[x][y][o] =
	city_tile_output(pcity, ptile, is_celebrating, o);
    } output_type_iterate_end;
  } city_tile_iterate_cxy_end;
}

/**************************************************************************
  Set the final surplus[] array from the prod[] and usage[] values.
**************************************************************************/
static void set_surpluses(struct city *pcity)
{
  output_type_iterate(o) {
    pcity->surplus[o] = pcity->prod[o] - pcity->usage[o];
  } output_type_iterate_end;
}

/**************************************************************************
  Copy the happyness array in the city to index i from index i-1.
**************************************************************************/
static void happy_copy(struct city *pcity, enum citizen_feeling i)
{
  int c = 0;
  for (; c < CITIZEN_LAST; c++) {
    pcity->feel[c][i] = pcity->feel[c][i - 1];
  }
}

/**************************************************************************
  Move up to 'count' citizens from the source to the destination
  happiness categories. For instance

    make_citizens_happy(&pcity->angry[0], &pcity->unhappy[0], 1)

  will make up to 1 angry citizen unhappy.  The number converted will be
  returned.
**************************************************************************/
static inline int make_citizens_happy(int *from, int *to, int count)
{
  count = MIN(count, *from);
  *from -= count;
  *to += count;
  return count;
}

/**************************************************************************
  Create content, unhappy and angry citizens.
**************************************************************************/
static void citizen_base_mood(struct city *pcity)
{
  struct player *pplayer = city_owner(pcity);
  int *happy = &pcity->feel[CITIZEN_HAPPY][FEELING_BASE];
  int *content = &pcity->feel[CITIZEN_CONTENT][FEELING_BASE];
  int *unhappy = &pcity->feel[CITIZEN_UNHAPPY][FEELING_BASE];
  int *angry = &pcity->feel[CITIZEN_ANGRY][FEELING_BASE];
  int size = pcity->size;
  int specialists = city_specialists(pcity);

  /* This is the number of citizens that may start out content, depending
   * on empire size and game's city unhappysize. This may be bigger than
   * the size of the city, since this is a potential. */
  int base_content = content_citizens(pplayer);

  /* Create content citizens. Take specialists from their ranks. */
  *content = MAX(0, MIN(size, base_content) - specialists);

  /* Create angry citizens only if we have a negative number of possible
   * content citizens. This happens when empires grow really big. */
  if (game.info.angrycitizen == FALSE) {
    *angry = 0;
  } else {
    *angry = MIN(MAX(0, -base_content), size - specialists);
  }

  /* Create unhappy citizens. In the beginning, all who are not content,
   * specialists or angry are unhappy. This is changed by luxuries and 
   * buildings later. */
  *unhappy = (size - specialists - *content - *angry);

  /* No one is born happy. */
  *happy = 0;
}

/**************************************************************************
  Make people happy: 
   * angry citizen are eliminated first
   * then content are made happy, then unhappy content, etc.
   * each conversions costs 2 or 4 luxuries.
**************************************************************************/
static inline void citizen_luxury_happy(struct city *pcity, int *luxuries)
{
  int *happy = &pcity->feel[CITIZEN_HAPPY][FEELING_LUXURY];
  int *content = &pcity->feel[CITIZEN_CONTENT][FEELING_LUXURY];
  int *unhappy = &pcity->feel[CITIZEN_UNHAPPY][FEELING_LUXURY];
  int *angry = &pcity->feel[CITIZEN_ANGRY][FEELING_LUXURY];

  while (*luxuries >= game.info.happy_cost && *angry > 0) {
    /* Upgrade angry to unhappy: costs HAPPY_COST each. */
    (*angry)--;
    (*unhappy)++;
    *luxuries -= game.info.happy_cost;
  }
  while (*luxuries >= game.info.happy_cost && *content > 0) {
    /* Upgrade content to happy: costs HAPPY_COST each. */
    (*content)--;
    (*happy)++;
    *luxuries -= game.info.happy_cost;
  }
  while (*luxuries >= 2 * game.info.happy_cost && *unhappy > 0) {
    /* Upgrade unhappy to happy.  Note this is a 2-level upgrade with
     * double the cost. */
    (*unhappy)--;
    (*happy)++;
    *luxuries -= 2 * game.info.happy_cost;
  }
  if (*luxuries >= game.info.happy_cost && *unhappy > 0) {
    /* Upgrade unhappy to content: costs HAPPY_COST each. */
    (*unhappy)--;
    (*content)++;
    *luxuries -= game.info.happy_cost;
  }
}

/**************************************************************************
  Make citizens happy due to luxury.
**************************************************************************/
static inline void citizen_happy_luxury(struct city *pcity)
{
  int x = pcity->prod[O_LUXURY];

  citizen_luxury_happy(pcity, &x);
}

/**************************************************************************
  Make citizens content due to city improvements.
**************************************************************************/
static inline void citizen_content_buildings(struct city *pcity)
{
  int *content = &pcity->feel[CITIZEN_CONTENT][FEELING_EFFECT];
  int *unhappy = &pcity->feel[CITIZEN_UNHAPPY][FEELING_EFFECT];
  int *angry = &pcity->feel[CITIZEN_ANGRY][FEELING_EFFECT];
  int faces = get_city_bonus(pcity, EFT_MAKE_CONTENT);

  /* make people content (but not happy):
     get rid of angry first, then make unhappy content. */
  while (faces > 0 && *angry > 0) {
    (*angry)--;
    (*unhappy)++;
    faces--;
  }
  while (faces > 0 && *unhappy > 0) {
    (*unhappy)--;
    (*content)++;
    faces--;
  }
}

/**************************************************************************
  Make citizens happy/unhappy due to units.

  This function requires that pcity->martial_law and
  pcity->unit_happy_cost have already been set in city_support().
**************************************************************************/
static inline void citizen_happy_units(struct city *pcity)
{
  int *happy = &pcity->feel[CITIZEN_HAPPY][FEELING_MARTIAL];
  int *content = &pcity->feel[CITIZEN_CONTENT][FEELING_MARTIAL];
  int *unhappy = &pcity->feel[CITIZEN_UNHAPPY][FEELING_MARTIAL];
  int *angry = &pcity->feel[CITIZEN_ANGRY][FEELING_MARTIAL];
  int amt = pcity->martial_law;

  /* Pacify discontent citizens through martial law.  First convert
   * angry => unhappy, then unhappy => content. */
  while (amt > 0 && *angry > 0) {
    (*angry)--;
    (*unhappy)++;
    amt--;
  }
  while (amt > 0 && *unhappy > 0) {
    (*unhappy)--;
    (*content)++;
    amt--;
  }

  /* Now make citizens unhappier because of military units away from home.
   * First make content => unhappy, then happy => unhappy,
   * then happy => content. */
  amt = pcity->unit_happy_upkeep;
  while (amt > 0 && *content > 0) {
    (*content)--;
    (*unhappy)++;
    amt--;
  }
  while (amt > 1 && *happy > 0) {
    (*happy)--;
    (*unhappy)++;
    amt -= 2;
  }
  while (amt > 0 && *content > 0) {
    (*content)--;
    (*unhappy)++;
    amt--;
  }
  /* Any remaining unhappiness is lost since angry citizens aren't created
   * here. */
  /* FIXME: Why not? - Per */
}

/**************************************************************************
  Make citizens happy due to wonders.
**************************************************************************/
static inline void citizen_happy_wonders(struct city *pcity)
{
  int *happy = &pcity->feel[CITIZEN_HAPPY][FEELING_FINAL];
  int *content = &pcity->feel[CITIZEN_CONTENT][FEELING_FINAL];
  int *unhappy = &pcity->feel[CITIZEN_UNHAPPY][FEELING_FINAL];
  int *angry = &pcity->feel[CITIZEN_ANGRY][FEELING_FINAL];
  int bonus = get_city_bonus(pcity, EFT_MAKE_HAPPY);

  /* First create happy citizens from content, then from unhappy
   * citizens; we cannot help angry citizens here. */
  while (bonus > 0 && *content > 0) {
    (*content)--;
    (*happy)++;
    bonus--;
  }
  while (bonus > 1 && *unhappy > 0) {
    (*unhappy)--;
    (*happy)++;
    bonus -= 2;
  }
  /* The rest falls through and lets unhappy people become content. */

  if (get_city_bonus(pcity, EFT_NO_UNHAPPY) > 0) {
    *content += *unhappy + *angry;
    *unhappy = 0;
    *angry = 0;
    return;
  }

  bonus += get_city_bonus(pcity, EFT_FORCE_CONTENT);

  /* get rid of angry first, then make unhappy content */
  while (bonus > 0 && *angry > 0) {
    (*angry)--;
    (*unhappy)++;
    bonus--;
  }
  while (bonus > 0 && *unhappy > 0) {
    (*unhappy)--;
    (*content)++;
    bonus--;
  }
}

/**************************************************************************
  Set food, tax, science and shields production to zero if city is in
  revolt.
**************************************************************************/
static inline void unhappy_city_check(struct city *pcity)
{
  if (city_unhappy(pcity)) {
    output_type_iterate(o) {
      switch (output_types[o].unhappy_penalty) {
      case UNHAPPY_PENALTY_NONE:
	pcity->unhappy_penalty[o] = 0;
	break;
      case UNHAPPY_PENALTY_SURPLUS:
	pcity->unhappy_penalty[o] = MAX(pcity->prod[o] - pcity->usage[o], 0);
	break;
      case UNHAPPY_PENALTY_ALL_PRODUCTION:
	pcity->unhappy_penalty[o] = pcity->prod[o];
	break;
      }

      pcity->prod[o] -= pcity->unhappy_penalty[o];
    } output_type_iterate_end;
  } else {
    memset(pcity->unhappy_penalty, 0,
 	   O_LAST * sizeof(*pcity->unhappy_penalty));
  }
}

/**************************************************************************
  Calculate the pollution from production and population in the city.
**************************************************************************/
int city_pollution_types(const struct city *pcity, int shield_total,
			 int *pollu_prod, int *pollu_pop, int *pollu_mod)
{
  struct player *pplayer = city_owner(pcity);
  int prod, pop, mod;

  /* Add one one pollution per shield, multipled by the bonus. */
  prod = 100 + get_city_bonus(pcity, EFT_POLLU_PROD_PCT);
  prod = shield_total * MAX(prod, 0) / 100;

  /* Add one 1/4 pollution per citizen per tech, multiplied by the bonus. */
  pop = 100 + get_city_bonus(pcity, EFT_POLLU_POP_PCT);
  pop = (pcity->size
	 * num_known_tech_with_flag(pplayer, TF_POPULATION_POLLUTION_INC)
	 * MAX(pop, 0)) / (4 * 100);

  /* Then there is base pollution (usually a negative number). */
  mod = game.info.base_pollution;

  if (pollu_prod) {
    *pollu_prod = prod;
  }
  if (pollu_pop) {
    *pollu_pop = pop;
  }
  if (pollu_mod) {
    *pollu_mod = mod;
  }
  return MAX(prod + pop + mod, 0);
}

/**************************************************************************
  Calculate pollution for the city.  The shield_total must be passed in
  (most callers will want to pass pcity->shield_prod).
**************************************************************************/
int city_pollution(const struct city *pcity, int shield_total)
{
  return city_pollution_types(pcity, shield_total, NULL, NULL, NULL);
}

/**************************************************************************
  Gets whether cities that pcity trades with had the plague. If so, it 
  returns the health penalty in tenth of percent which depends on the size
  of both cities. The health penalty is given as the product of the ruleset
  option 'game.info.illness_trade_infection' (in percent) and the square
  root of the product of the size of both cities.
 *************************************************************************/
static int get_trade_illness(const struct city *pcity)
{
  int i;
  int total_penalty = 0;

  for (i = 0 ; i < NUM_TRADEROUTES ; i++) {
    struct city *trade_city = game_find_city_by_number(pcity->trade[i]);
    if (trade_city != NULL
        && trade_city->turn_plague != -1
        && trade_city->turn_plague - game.info.turn < 5) {
      total_penalty += game.info.illness_trade_infection / 100
                       * sqrt(pcity->size * trade_city->size);
    }
  }

  return total_penalty;
}

/**************************************************************************
  Get any effects regarding health from the buildings of the city. The
  effect defines the reduction of the possibility of an illness in percent.
**************************************************************************/
static int get_city_health(const struct city *pcity)
{
  return get_city_bonus(pcity, EFT_HEALTH_PCT);
}

/**************************************************************************
  Calculate city's illness in tenth of percent:

  base illness (maximum illness given by 'game.info.illness_base_factor')
  + trade illness (see get_trade_illness())
  + pollution illness (ruleset option 'game.info.illness_pollution_factor'
  times the pollution in the city)

  The illness is reduced by the percentage given by the health effect.
  Illness cannot exceed 999, or be less then 0
 *************************************************************************/
int city_illness(const struct city *pcity, int *ill_base, int *ill_size,
                 int *ill_trade, int *ill_pollution)
{
  int illness_size = (1- exp(- pcity->size / 10)) * 10
                     * game.info.illness_base_factor;
  int illness_trade = get_trade_illness(pcity);
  int illness_pollution = game.info.illness_pollution_factor / 100
                          * pcity->pollution;
  int illness_base = illness_size + illness_trade + illness_pollution;

  int illness_percent = 100 - get_city_health(pcity);

  /* returning other data */
  if (ill_size) {
    *ill_size = illness_size;
  }

  if (ill_trade) {
    *ill_trade = illness_trade;
  }

  if (ill_pollution) {
    *ill_pollution = illness_pollution;
  }

  if (ill_base) {
    *ill_base = illness_base;
  }

  return CLIP(0, illness_base*illness_percent/100 , 999);
}

/**************************************************************************
   Set food, trade and shields production in a city.

   This initializes the prod[] and waste[] arrays.  It assumes that
   the bonus[] and citizen_base[] arrays are alread built.
**************************************************************************/
static inline void set_city_production(struct city *pcity)
{
  int i;

  /* Calculate city production!
   *
   * This is a rather complicated process if we allow rules to become
   * more generalized.  We can assume that there are no recursive dependency
   * loops, but there are some dependencies that do not follow strict
   * ordering.  For instance corruption must be calculated before 
   * trade taxes can be counted up, which must occur before the science bonus
   * is added on.  But the calculation of corruption must include the
   * trade bonus.  To do this without excessive special casing means that in
   * this case the bonuses are multiplied on twice (but only saved the second
   * time).
   */

  output_type_iterate(o) {
    pcity->prod[o] = pcity->citizen_base[o];
  } output_type_iterate_end;

  /* Add on special extra incomes: trade routes and tithes. */
  for (i = 0; i < NUM_TRADEROUTES; i++) {
    pcity->trade_value[i] =
	trade_between_cities(pcity, game_find_city_by_number(pcity->trade[i]));
    pcity->prod[O_TRADE] += pcity->trade_value[i];
  }
  pcity->prod[O_GOLD] += get_city_tithes_bonus(pcity);

  /* Account for waste.  Note that waste is calculated before tax income is
   * calculated, so if you had "science waste" it would not include taxed
   * science.  However waste is calculated after the bonuses are multiplied
   * on, so shield waste will include shield bonuses. */
  output_type_iterate(o) {
    pcity->waste[o] = city_waste(pcity, o,
				 pcity->prod[o] * pcity->bonus[o] / 100);
  } output_type_iterate_end;

  /* Convert trade into science/luxury/gold, and add this on to whatever
   * science/luxury/gold is already there. */
  add_tax_income(city_owner(pcity),
		 pcity->prod[O_TRADE] * pcity->bonus[O_TRADE] / 100
		 - pcity->waste[O_TRADE] - pcity->usage[O_TRADE],
		 pcity->prod);

  /* Add on effect bonuses and waste.  Note that the waste calculation
   * (above) already includes the bonus multiplier. */
  output_type_iterate(o) {
    pcity->prod[o] = pcity->prod[o] * pcity->bonus[o] / 100;
    pcity->prod[o] -= pcity->waste[o];
  } output_type_iterate_end;
}

/**************************************************************************
  Query unhappiness caused by a given unit.
**************************************************************************/
int city_unit_unhappiness(struct unit *punit, int *free_unhappy)
{
  struct city *pcity = game_find_city_by_number(punit->homecity);
  struct unit_type *ut = unit_type(punit);
  struct player *plr = unit_owner(punit);
  int happy_cost = utype_happy_cost(ut, plr);

  if (!punit || !pcity || !free_unhappy || happy_cost <= 0) {
    return 0;
  }
  assert(free_unhappy >= 0);

  happy_cost -= get_city_bonus(pcity, EFT_MAKE_CONTENT_MIL_PER);

  if (!unit_being_aggressive(punit) && !is_field_unit(punit)) {
    return 0;
  }
  if (happy_cost <= 0) {
    return 0;
  }
  if (*free_unhappy > happy_cost) {
    *free_unhappy -= happy_cost;
    return 0;
  }
  return happy_cost;
}

/**************************************************************************
  Calculate upkeep costs.  This builds the pcity->usage[] array as well
  as setting some happiness values.
**************************************************************************/
static inline void city_support(struct city *pcity)
{
  int free_unhappy = get_city_bonus(pcity, EFT_MAKE_CONTENT_MIL);

  /* Clear all usage values. */
  memset(pcity->usage, 0, O_LAST * sizeof(*pcity->usage));
  pcity->martial_law = 0;
  pcity->unit_happy_upkeep = 0;

  /* Building gold upkeep depends on the setting
   * 'game.info.gold_upkeep_style':
   * 0,1 - The upkeep for buildings is paid by the city.
   * 2   - The upkeep for buildings is paid by the nation. */
  if (game.info.gold_upkeep_style < 2) {
    pcity->usage[O_GOLD] += city_total_impr_gold_upkeep(pcity);
  }
  /* Food consumption by citizens. */
  pcity->usage[O_FOOD] += game.info.food_cost * pcity->size;

  /* military units in this city (need _not_ be home city) can make
     unhappy citizens content
   */
  if (get_city_bonus(pcity, EFT_MARTIAL_LAW_EACH) > 0) {
    int max = get_city_bonus(pcity, EFT_MARTIAL_LAW_MAX);

    unit_list_iterate(pcity->tile->units, punit) {
      if ((pcity->martial_law < max || max == 0)
	  && is_military_unit(punit)
	  && unit_owner(punit) == city_owner(pcity)) {
	pcity->martial_law++;
      }
    } unit_list_iterate_end;
    pcity->martial_law *= get_city_bonus(pcity, EFT_MARTIAL_LAW_EACH);
  }

  unit_list_iterate(pcity->units_supported, punit) {
    int happy_cost = city_unit_unhappiness(punit, &free_unhappy);

    output_type_iterate(o) {
      /* Unit gold upkeep depends on the setting
       * 'game.info.gold_upkeep_style':
       * 0   - The upkeep for units is paid by the homecity.
       * 1,2 - The upkeep for units is paid by the nation. */
      if (!(game.info.gold_upkeep_style > 0 && o == O_GOLD)) {
        pcity->usage[o] += punit->upkeep[o];
      }
    } output_type_iterate_end;
    pcity->unit_happy_upkeep += happy_cost;
  } unit_list_iterate_end;
}

/**************************************************************************
  Refreshes the internal cached data in the city structure.

  !full_refresh will not update tile_output[] or bonus[].  These two
  values do not need to be recalculated for AI CMA testing.
**************************************************************************/
void city_refresh_from_main_map(struct city *pcity, bool full_refresh)
{
  if (full_refresh) {
    set_city_bonuses(pcity);	/* Calculate the bonus[] array values. */
    set_city_tile_output(pcity); /* Calculate the tile_output[] values. */
    city_support(pcity); /* manage settlers, and units */
  }

  /* Calculate output from citizens. */
  get_worked_tile_output(pcity, pcity->citizen_base, full_refresh);
  add_specialist_output(pcity, pcity->citizen_base);

  set_city_production(pcity);
  citizen_base_mood(pcity);
  pcity->pollution = city_pollution(pcity, pcity->prod[O_SHIELD]);

  /* This must be after pollution */
  pcity->illness = city_illness(pcity, NULL, NULL, NULL, NULL);

  happy_copy(pcity, FEELING_LUXURY);
  citizen_happy_luxury(pcity);	/* with our new found luxuries */

  happy_copy(pcity, FEELING_EFFECT);
  citizen_content_buildings(pcity);

  /* Martial law & unrest from units */
  happy_copy(pcity, FEELING_MARTIAL);
  citizen_happy_units(pcity);

  /* Building (including wonder) happiness effects */
  happy_copy(pcity, FEELING_FINAL);
  citizen_happy_wonders(pcity);

  unhappy_city_check(pcity);
  set_surpluses(pcity);
}

/**************************************************************************
  Give corruption/waste generated by city.  otype gives the output type
  (O_SHIELD/O_TRADE).  'total' gives the total output of this type in the
  city.
**************************************************************************/
int city_waste(const struct city *pcity, Output_type_id otype, int total)
{
  int penalty = 0;
  int waste_level = get_city_output_bonus(pcity, get_output_type(otype),
                                          EFT_OUTPUT_WASTE);
  int waste_by_dist = get_city_output_bonus(pcity, get_output_type(otype),
                                            EFT_OUTPUT_WASTE_BY_DISTANCE);
  int waste_pct = get_city_output_bonus(pcity, get_output_type(otype), 
                                        EFT_OUTPUT_WASTE_PCT);

  if (otype == O_TRADE) {
    /* FIXME: special case for trade: it is affected by notradesize and
     * fulltradesize server settings.
     *
     * If notradesize and fulltradesize are equal then the city gets no
     * trade at that size. */
    int notradesize = MIN(game.info.notradesize, game.info.fulltradesize);
    int fulltradesize = MAX(game.info.notradesize, game.info.fulltradesize);

    if (pcity->size <= notradesize) {
      penalty = total;
    } else if (pcity->size >= fulltradesize) {
      penalty = 0;
    } else {
      penalty = total * (fulltradesize - pcity->size)
	/ (fulltradesize - notradesize);
    }
  }

  if (waste_by_dist > 0) {
    const struct city *capital = find_palace(city_owner(pcity));

    if (!capital) {
      return total; /* no capital - no income */
    } else {
      waste_level += waste_by_dist 
                     * real_map_distance(capital->tile, pcity->tile);
    }
  }

  if (waste_level > 0) {
    penalty += total * waste_level / 100;
  }

  penalty -= penalty * waste_pct / 100;

  return MIN(MAX(penalty, 0), total);
}

/**************************************************************************
  Give the number of specialists in a city.
**************************************************************************/
int city_specialists(const struct city *pcity)
{
  int count = 0;

  specialist_type_iterate(sp) {
    count += pcity->specialists[sp];
  } specialist_type_iterate_end;

  return count;
}

/****************************************************************************
  Return the "best" specialist available in the game.  This specialist will
  have the most of the given type of output.  If pcity is given then only
  specialists usable by pcity will be considered.
****************************************************************************/
Specialist_type_id best_specialist(Output_type_id otype,
				   const struct city *pcity)
{
  int best = DEFAULT_SPECIALIST;
  int val = get_specialist_output(pcity, best, otype);

  specialist_type_iterate(i) {
    if (!pcity || city_can_use_specialist(pcity, i)) {
      int val2 = get_specialist_output(pcity, i, otype);

      if (val2 > val) {
	best = i;
	val = val2;
      }
    }
  } specialist_type_iterate_end;

  return best;
}

/**************************************************************************
 Adds an improvement (and its effects) to a city.
**************************************************************************/
void city_add_improvement(struct city *pcity,
			  const struct impr_type *pimprove)
{
  pcity->built[improvement_index(pimprove)].turn = game.info.turn; /*I_ACTIVE*/

  if (is_server() && is_wonder(pimprove)) {
    /* Client just read the info from the packets. */
    wonder_built(pcity, pimprove);
  }
}

/**************************************************************************
 Removes an improvement (and its effects) from a city.
**************************************************************************/
void city_remove_improvement(struct city *pcity,
			     const struct impr_type *pimprove)
{
  freelog(LOG_DEBUG,"Improvement %s removed from city %s",
          improvement_rule_name(pimprove),
          pcity->name);
  
  pcity->built[improvement_index(pimprove)].turn = I_DESTROYED;

  if (is_server() && is_wonder(pimprove)) {
    /* Client just read the info from the packets. */
    wonder_destroyed(pcity, pimprove);
  }
}

/**************************************************************************
 Returns TRUE iff the city has set the given option.
**************************************************************************/
bool is_city_option_set(const struct city *pcity, enum city_options option)
{
  return BV_ISSET(pcity->city_options, option);
}

/**************************************************************************
 Allocate memory for this amount of city styles.
**************************************************************************/
void city_styles_alloc(int num)
{
  int i;

  city_styles = fc_calloc(num, sizeof(*city_styles));
  game.control.styles_count = num;

  for (i = 0; i < game.control.styles_count; i++) {
    requirement_vector_init(&city_styles[i].reqs);
  }
}

/**************************************************************************
 De-allocate the memory used by the city styles.
**************************************************************************/
void city_styles_free(void)
{
  int i;

  for (i = 0; i < game.control.styles_count; i++) {
    requirement_vector_free(&city_styles[i].reqs);
  }

  free(city_styles);
  city_styles = NULL;
  game.control.styles_count = 0;
}

/**************************************************************************
  Create virtual skeleton for a city.
  Values are mostly sane defaults.

  Always tile_set_owner(ptile, pplayer) sometime after this!
**************************************************************************/
struct city *create_city_virtual(struct player *pplayer,
		                 struct tile *ptile, const char *name)
{
  int i;

  /* Make sure that contents of city structure are correctly
   * initialized, if you ever allocate it by some other mean than fc_calloc() */
  struct city *pcity = fc_calloc(1, sizeof(*pcity));

  assert(pplayer != NULL); /* No unowned cities! */
  pcity->original = pplayer;
  pcity->owner = pplayer;

  pcity->tile = ptile;

  assert(name != NULL);
  sz_strlcpy(pcity->name, name);

  /* City structure was allocated with fc_calloc(), so
   * contents are initially zero. No need to initialize second time. */
#ifdef ZERO_VARIABLES_FOR_SEARCHING
  /* This does not register the city, so the identity defaults to 0. */
  pcity->id = IDENTITY_NUMBER_ZERO;

  memset(pcity->feel, 0, sizeof(pcity->feel));
  memset(pcity->specialists, 0, sizeof(pcity->specialists));
#endif

  /* assume some non-working population: */
  pcity->specialists[DEFAULT_SPECIALIST] =
  pcity->size = 1;

#ifdef ZERO_VARIABLES_FOR_SEARCHING
  for (i = 0; i < NUM_TRADEROUTES; i++) {
    pcity->trade_value[i] = pcity->trade[i] = 0;
  }
  memset(pcity->tile_output, 0, sizeof(pcity->tile_output));

  memset(pcity->surplus, 0, O_LAST * sizeof(*pcity->surplus));
  memset(pcity->waste, 0, O_LAST * sizeof(*pcity->waste));
  memset(pcity->unhappy_penalty, 0,
	 O_LAST * sizeof(*pcity->unhappy_penalty));
  memset(pcity->prod, 0, O_LAST * sizeof(*pcity->prod));
  memset(pcity->citizen_base, 0, O_LAST * sizeof(*pcity->citizen_base));
  memset(pcity->usage, 0, O_LAST * sizeof(*pcity->usage));
#endif

  output_type_iterate(o) {
    pcity->bonus[o] = 100;
  } output_type_iterate_end;

#ifdef ZERO_VARIABLES_FOR_SEARCHING
  pcity->martial_law = 0;
  pcity->unit_happy_upkeep = 0;

  pcity->food_stock = 0;
  pcity->shield_stock = 0;
  pcity->pollution = 0;
  pcity->illness = 0;

  pcity->airlift = 0;
  pcity->debug = FALSE;
#endif
  pcity->did_buy = TRUE; /* You cannot buy production same turn city is
                          * founded. */
#ifdef ZERO_VARIABLES_FOR_SEARCHING
  pcity->did_sell = FALSE;
  pcity->is_updated = FALSE;
  pcity->was_happy = FALSE;

  pcity->anarchy = 0;
  pcity->rapture = 0;
  pcity->steal = 0;
#endif

  pcity->turn_plague = -1; /* -1 = never */

  pcity->turn_founded = game.info.turn;
  pcity->turn_last_built = game.info.turn;

  pcity->migration_score = 0.0; /* Updated by check_city_migrations. */
  pcity->mgr_score_calc_turn = -1; /* -1 = never */

#ifdef ZERO_VARIABLES_FOR_SEARCHING
  pcity->before_change_shields = 0;
  pcity->caravan_shields = 0;
  pcity->disbanded_shields = 0;
  pcity->last_turns_shield_surplus = 0;
#endif

  /* Initialise improvements list */
  for (i = 0; i < ARRAY_SIZE(pcity->built); i++) {
    pcity->built[i].turn = I_NEVER;
  }

#ifdef ZERO_VARIABLES_FOR_SEARCHING
  /* just setting the entry to zero: */
  pcity->production.kind = VUT_NONE;
  /* all the union pointers should be in the same place: */ 
  pcity->production.value.building = NULL;
  pcity->changed_from = pcity->production;
#endif

  /* Set up the worklist */
  worklist_init(&pcity->worklist);

#ifdef ZERO_VARIABLES_FOR_SEARCHING
  BV_CLR_ALL(pcity->city_options);

  /* city_map; placeholder for searching */

  pcity->client.occupied = FALSE;
  pcity->client.walls = FALSE;
  pcity->client.happy = FALSE;
  pcity->client.unhappy = FALSE;
  pcity->client.colored = FALSE;
  pcity->client.color_index = FALSE;

  pcity->server.workers_frozen = 0;
  pcity->server.needs_arrange = FALSE;
  pcity->server.needs_refresh = FALSE;
  pcity->server.synced = FALSE;
  pcity->server.vision = NULL; /* No vision. */

#endif

  ai_type_iterate(ai) {
    if (ai->funcs.init_city) {
      ai->funcs.init_city(pcity);
    }
  } ai_type_iterate_end;

  /* pcity->ai.act_value; placeholder for searching */
  /* info_units_present; placeholder for searching */
  /* info_units_supported; placeholder for searching */

  pcity->units_supported = unit_list_new();

  return pcity;
}

/**************************************************************************
  Removes the virtual skeleton of a city. You should already have removed
  all buildings and units you have added to the city before this.
**************************************************************************/
void destroy_city_virtual(struct city *pcity)
{
  ai_type_iterate(ai) {
    if (ai->funcs.close_city) {
      ai->funcs.close_city(pcity);
    }
  } ai_type_iterate_end;

  unit_list_free(pcity->units_supported);
  memset(pcity, 0, sizeof(*pcity)); /* ensure no pointers remain */
  free(pcity);
}

/**************************************************************************
  Check if city with given id still exist. Use this before using
  old city pointers when city might have disappeared.
**************************************************************************/
bool city_exist(int id)
{
  /* Check if city exist in game */
  if (game_find_city_by_number(id)) {
    return TRUE;
  }

  return FALSE;
}

/**************************************************************************
  Return TRUE if the city is a virtual city. That is, it is a valid city
  pointer but does not correspond to a city that exists in the game.

  NB: A return value of FALSE implies that either the pointer is NULL or
  that the city exists in the game.
**************************************************************************/
bool city_is_virtual(const struct city *pcity)
{
  if (!pcity) {
    return FALSE;
  }

  return pcity != game_find_city_by_number(pcity->id);
}

/**************************************************************************
  Return citytile type for a given rule name
**************************************************************************/
enum citytile_type find_citytile_by_rule_name(const char *name)
{
  if (!mystrcasecmp(name, "center")) {
    return CITYT_CENTER;
  }

  return CITYT_LAST;
}
