/********************************************************************** 
 Freeciv - Copyright (C) 2005 - The Freeciv Project
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

#include "log.h"
#include "support.h"

#include "tile.h"

#ifndef tile_index
/****************************************************************************
  Return the tile index.
****************************************************************************/
int tile_index(const struct tile *ptile)
{
  return ptile->index;
}
#endif

#ifndef tile_owner
/****************************************************************************
  Return the player who owns this tile (or NULL if none).
****************************************************************************/
struct player *tile_owner(const struct tile *ptile)
{
  return ptile->owner;
}
#endif

#ifndef tile_claimer
/****************************************************************************
  Return the player who owns this tile (or NULL if none).
****************************************************************************/
struct tile *tile_claimer(const struct tile *ptile)
{
  return ptile->claimer;
}
#endif

/****************************************************************************
  Set the owner of a tile (may be NULL).
****************************************************************************/
void tile_set_owner(struct tile *ptile, struct player *pplayer,
                    struct tile *claimer)
{
  ptile->owner = pplayer;
  ptile->claimer = claimer;
}

/****************************************************************************
  Return the city on this tile (or NULL), checking for city center.
****************************************************************************/
struct city *tile_city(const struct tile *ptile)
{
  struct city *pcity = ptile->worked;

  if (NULL != pcity && is_city_center(pcity, ptile)) {
    return pcity;
  }
  return NULL;
}

#ifndef tile_worked
/****************************************************************************
  Return any city working the specified tile (or NULL).
****************************************************************************/
struct city *tile_worked(const struct tile *ptile)
{
  return ptile->worked;
}
#endif

/****************************************************************************
  Set the city/worker on the tile (may be NULL).
****************************************************************************/
void tile_set_worked(struct tile *ptile, struct city *pcity)
{
  ptile->worked = pcity;
}

#ifndef tile_terrain
/****************************************************************************
  Return the terrain at the specified tile.
****************************************************************************/
struct terrain *tile_terrain(const struct tile *ptile)
{
  return ptile->terrain;
}
#endif

/****************************************************************************
  Set the given terrain at the specified tile.
****************************************************************************/
void tile_set_terrain(struct tile *ptile, struct terrain *pterrain)
{
  ptile->terrain = pterrain;
  if (NULL != pterrain
   && NULL != ptile->resource
   && terrain_has_resource(pterrain, ptile->resource)) {
    /* cannot use set_special() for internal values */
    BV_SET(ptile->special, S_RESOURCE_VALID);
  } else {
    BV_CLR(ptile->special, S_RESOURCE_VALID);
  }
}

/****************************************************************************
  Return the specials of the tile.  See terrain.h.

  Note that this returns a mask of _all_ the specials on the tile.  To
  check a specific special use tile_has_special.
****************************************************************************/
bv_special tile_specials(const struct tile *ptile)
{
  return ptile->special;
}

/****************************************************************************
  Sets the tile's specials to those present in the given bit vector.
****************************************************************************/
void tile_set_specials(struct tile *ptile, bv_special specials)
{
  if (!ptile) {
    return;
  }
  ptile->special = specials;
}

/****************************************************************************
  Returns TRUE iff the given tile has the given special.
****************************************************************************/
bool tile_has_special(const struct tile *ptile,
		      enum tile_special_type special)
{
  return contains_special(ptile->special, special);
}

/****************************************************************************
  Returns TRUE iff the given tile has any specials.
****************************************************************************/
bool tile_has_any_specials(const struct tile *ptile)
{
  return contains_any_specials(ptile->special);
}

/****************************************************************************
  Returns a bit vector of the bases present at the tile.
****************************************************************************/
bv_bases tile_bases(const struct tile *ptile)
{
  if (!ptile) {
    bv_bases empty;
    BV_CLR_ALL(empty);
    return empty;
  }
  return ptile->bases;
}

/****************************************************************************
  Set the bases on the tile to those present in the given bit vector.
****************************************************************************/
void tile_set_bases(struct tile *ptile, bv_bases bases)
{
  if (!ptile) {
    return;
  }
  ptile->bases = bases;
}

/****************************************************************************
  Adds base to tile.
  FIXME: Should remove conflicting old base and return bool indicating that.
****************************************************************************/
void tile_add_base(struct tile *ptile, const struct base_type *pbase)
{
  BV_SET(ptile->bases, base_index(pbase));
}

/****************************************************************************
  Removes base from tile if such exist
****************************************************************************/
void tile_remove_base(struct tile *ptile, const struct base_type *pbase)
{
  BV_CLR(ptile->bases, base_index(pbase));
}

/****************************************************************************
  Check if tile contains base providing effect
****************************************************************************/
bool tile_has_base_flag(const struct tile *ptile, enum base_flag_id flag)
{
  base_type_iterate(pbase) {
    if (tile_has_base(ptile, pbase) && base_has_flag(pbase, flag)) {
      return TRUE;
    }
  } base_type_iterate_end;

  return FALSE;
}

/****************************************************************************
  Check if tile contains base providing effect for unit
****************************************************************************/
bool tile_has_base_flag_for_unit(const struct tile *ptile,
                                 const struct unit_type *punittype,
                                 enum base_flag_id flag)
{
  base_type_iterate(pbase) {
    if (tile_has_base(ptile, pbase)
        && base_has_flag_for_utype(pbase, flag, punittype)) {
      return TRUE;
    }
  } base_type_iterate_end;

  return FALSE;
}

/****************************************************************************
  Check if tile contains base providing effect for unit
****************************************************************************/
bool tile_has_claimable_base(const struct tile *ptile,
                             const struct unit_type *punittype)
{
  base_type_iterate(pbase) {
    if (tile_has_base(ptile, pbase)
        && territory_claiming_base(pbase)
        && is_native_base_to_uclass(pbase, utype_class(punittype))) {
      return TRUE;
    }
  } base_type_iterate_end;

  return FALSE;
}

/****************************************************************************
  Calculate defense bonus given by bases
****************************************************************************/
int tile_bases_defense_bonus(const struct tile *ptile,
                             const struct unit_type *punittype)
{
  int bonus = 0;

  base_type_iterate(pbase) {
    if (tile_has_base(ptile, pbase)
        && is_native_base_to_uclass(pbase, utype_class(punittype))) {
      bonus += pbase->defense_bonus;
    }
  } base_type_iterate_end;

  return bonus;
}

/****************************************************************************
  Check if tile contains base native for unit
****************************************************************************/
bool tile_has_native_base(const struct tile *ptile,
                          const struct unit_type *punittype)
{
  base_type_iterate(pbase) {
    if (tile_has_base(ptile, pbase)
        && is_native_base_to_utype(pbase, punittype)) {
      return TRUE;
    }
  } base_type_iterate_end;

  return FALSE;
}

/****************************************************************************
  Add the given special or specials to the tile.

  Note that this does not erase any existing specials already on the tile
  (see tile_clear_special or tile_clear_all_specials for that).  Also note
  the special to be set may be a mask, so you can set more than one
  special at a time (but this is not recommended).
****************************************************************************/
void tile_set_special(struct tile *ptile, enum tile_special_type spe)
{
  set_special(&ptile->special, spe);
}

#ifndef tile_resource
/****************************************************************************
  Return the resource at the specified tile.
****************************************************************************/
const struct resource *tile_resource(const struct tile *ptile)
{
  return ptile->resource;
}
#endif

/****************************************************************************
  Set the given resource at the specified tile.
****************************************************************************/
void tile_set_resource(struct tile *ptile, struct resource *presource)
{
  ptile->resource = presource;
  if (NULL != ptile->terrain
   && NULL != presource
   && terrain_has_resource(ptile->terrain, presource)) {
    /* cannot use set_special() for internal values */
    BV_SET(ptile->special, S_RESOURCE_VALID);
  } else {
    BV_CLR(ptile->special, S_RESOURCE_VALID);
  }
}

/****************************************************************************
  Clear the given special or specials from the tile.

  This function clears all the specials set in the 'spe' mask from the
  tile's set of specials.  All other specials are unaffected.
****************************************************************************/
void tile_clear_special(struct tile *ptile, enum tile_special_type spe)
{
  clear_special(&ptile->special, spe);
}

/****************************************************************************
  Remove any and all specials from this tile.
****************************************************************************/
void tile_clear_all_specials(struct tile *ptile)
{
  clear_all_specials(&ptile->special);
}

#ifndef tile_continent
/****************************************************************************
  Return the continent ID of the tile.  Typically land has a positive
  continent number and ocean has a negative number; no tile should have
  a 0 continent number.
****************************************************************************/
Continent_id tile_continent(const struct tile *ptile)
{
  return ptile->continent;
}
#endif

/****************************************************************************
  Set the continent ID of the tile.  See tile_continent.
****************************************************************************/
void tile_set_continent(struct tile *ptile, Continent_id val)
{
  ptile->continent = val;
}

/****************************************************************************
  Return a known_type enumeration value for the tile.

  Note that the client only has known data about its own player.
****************************************************************************/
enum known_type tile_get_known(const struct tile *ptile,
			       const struct player *pplayer)
{
  if (!BV_ISSET(ptile->tile_known, player_index(pplayer))) {
    return TILE_UNKNOWN;
  } else if (!BV_ISSET(ptile->tile_seen[V_MAIN], player_index(pplayer))) {
    return TILE_KNOWN_UNSEEN;
  } else {
    return TILE_KNOWN_SEEN;
  }
}

/****************************************************************************
  Time to complete the given activity on the given tile.
****************************************************************************/
int tile_activity_time(enum unit_activity activity, const struct tile *ptile)
{
  struct terrain *pterrain = tile_terrain(ptile);

  /* Make sure nobody uses old activities */
  assert(activity != ACTIVITY_FORTRESS && activity != ACTIVITY_AIRBASE);

  /* ACTIVITY_BASE not handled here */
  assert(activity != ACTIVITY_BASE);

  switch (activity) {
  case ACTIVITY_POLLUTION:
    return pterrain->clean_pollution_time * ACTIVITY_FACTOR;
  case ACTIVITY_ROAD:
    return pterrain->road_time * ACTIVITY_FACTOR;
  case ACTIVITY_MINE:
    return pterrain->mining_time * ACTIVITY_FACTOR;
  case ACTIVITY_IRRIGATE:
    return pterrain->irrigation_time * ACTIVITY_FACTOR;
  case ACTIVITY_RAILROAD:
    return pterrain->rail_time * ACTIVITY_FACTOR;
  case ACTIVITY_TRANSFORM:
    return pterrain->transform_time * ACTIVITY_FACTOR;
  case ACTIVITY_FALLOUT:
    return pterrain->clean_fallout_time * ACTIVITY_FACTOR;
  default:
    return 0;
  }
}

/****************************************************************************
  Time to complete the given activity on the given tile.
****************************************************************************/
int tile_activity_base_time(const struct tile *ptile,
                            Base_type_id base)
{
  return base_by_number(base)->build_time * ACTIVITY_FACTOR;
}

/****************************************************************************
  Clear all infrastructure (man-made specials) from the tile.
****************************************************************************/
static void tile_clear_infrastructure(struct tile *ptile)
{
  int i;

  for (i = 0; infrastructure_specials[i] != S_LAST; i++) {
    tile_clear_special(ptile, infrastructure_specials[i]);
  }
}

/****************************************************************************
  Clear all "dirtiness" (pollution and fallout) from the tile.
****************************************************************************/
static void tile_clear_dirtiness(struct tile *ptile)
{
  tile_clear_special(ptile, S_POLLUTION);
  tile_clear_special(ptile, S_FALLOUT);
}

/****************************************************************************
  Change the terrain to the given type.  This does secondary tile updates to
  the tile (as will happen when mining/irrigation/transforming changes the
  tile's terrain).
****************************************************************************/
void tile_change_terrain(struct tile *ptile, struct terrain *pterrain)
{
  tile_set_terrain(ptile, pterrain);
  if (is_ocean(pterrain)) {
    tile_clear_infrastructure(ptile);
    tile_clear_dirtiness(ptile);

    /* The code can't handle these specials in ocean. */
    tile_clear_special(ptile, S_RIVER);
    tile_clear_special(ptile, S_HUT);
  }

  /* Clear mining/irrigation if resulting terrain type cannot support
   * that feature. */
  
  if (pterrain->mining_result != pterrain) {
    tile_clear_special(ptile, S_MINE);
  }

  if (pterrain->irrigation_result != pterrain) {
    tile_clear_special(ptile, S_IRRIGATION);
    tile_clear_special(ptile, S_FARMLAND);
  }
}

/****************************************************************************
  Add the special to the tile.  This does secondary tile updates to
  the tile.
****************************************************************************/
void tile_add_special(struct tile *ptile, enum tile_special_type special)
{
  assert(special != S_OLD_FORTRESS && special != S_OLD_AIRBASE);

  tile_set_special(ptile, special);

  switch (special) {
  case S_FARMLAND:
    tile_add_special(ptile, S_IRRIGATION);
    /* Fall through to irrigation */
  case S_IRRIGATION:
    tile_clear_special(ptile, S_MINE);
    break;
  case S_RAILROAD:
    tile_add_special(ptile, S_ROAD);
    break;
  case S_MINE:
    tile_clear_special(ptile, S_IRRIGATION);
    tile_clear_special(ptile, S_FARMLAND);
    break;

  case S_ROAD:
  case S_POLLUTION:
  case S_HUT:
  case S_RIVER:
  case S_FALLOUT:
  default:
    break;
  }
}

/****************************************************************************
  Remove the special from the tile.  This does secondary tile updates to
  the tile.
****************************************************************************/
void tile_remove_special(struct tile *ptile, enum tile_special_type special)
{
  assert(special != S_OLD_FORTRESS && special != S_OLD_AIRBASE);

  tile_clear_special(ptile, special);

  switch (special) {
  case S_IRRIGATION:
    tile_clear_special(ptile, S_FARMLAND);
    break;
  case S_ROAD:
    tile_clear_special(ptile, S_RAILROAD);
    break;

  case S_RAILROAD:
  case S_MINE:
  case S_POLLUTION:
  case S_HUT:
  case S_RIVER:
  case S_FARMLAND:
  case S_FALLOUT:
  default:
    break;
  }
}

/****************************************************************************
  Build irrigation on the tile.  This may change the specials of the tile
  or change the terrain type itself.
****************************************************************************/
static void tile_irrigate(struct tile *ptile)
{
  struct terrain *pterrain = tile_terrain(ptile);

  if (pterrain == pterrain->irrigation_result) {
    if (tile_has_special(ptile, S_IRRIGATION)) {
      tile_add_special(ptile, S_FARMLAND);
    } else {
      tile_add_special(ptile, S_IRRIGATION);
    }
  } else if (pterrain->irrigation_result) {
    tile_change_terrain(ptile, pterrain->irrigation_result);
  }
}

/****************************************************************************
  Build a mine on the tile.  This may change the specials of the tile
  or change the terrain type itself.
****************************************************************************/
static void tile_mine(struct tile *ptile)
{
  struct terrain *pterrain = tile_terrain(ptile);

  if (pterrain == pterrain->mining_result) {
    tile_set_special(ptile, S_MINE);
    tile_clear_special(ptile, S_FARMLAND);
    tile_clear_special(ptile, S_IRRIGATION);
  } else if (pterrain->mining_result) {
    tile_change_terrain(ptile, pterrain->mining_result);
  }
}

/****************************************************************************
  Transform (ACTIVITY_TRANSFORM) the tile.  This usually changes the tile's
  terrain type.
****************************************************************************/
static void tile_transform(struct tile *ptile)
{
  struct terrain *pterrain = tile_terrain(ptile);

  if (pterrain->transform_result != T_NONE) {
    tile_change_terrain(ptile, pterrain->transform_result);
  }
}

/****************************************************************************
  Apply an activity (Activity_type_id, e.g., ACTIVITY_TRANSFORM) to a tile.
  Return false if there was a error or if the activity is not implemented
  by this function.
****************************************************************************/
bool tile_apply_activity(struct tile *ptile, Activity_type_id act) 
{
  /* FIXME: for irrigate, mine, and transform we always return TRUE
   * even if the activity fails. */
  switch(act) {
  case ACTIVITY_POLLUTION:
  case ACTIVITY_FALLOUT: 
    tile_clear_dirtiness(ptile);
    return TRUE;
    
  case ACTIVITY_MINE:
    tile_mine(ptile);
    return TRUE;

  case ACTIVITY_IRRIGATE: 
    tile_irrigate(ptile);
    return TRUE;

  case ACTIVITY_ROAD: 
    if (!is_ocean_tile(ptile)
	&& !tile_has_special(ptile, S_ROAD)) {
      tile_set_special(ptile, S_ROAD);
      return TRUE;
    }
    return FALSE;

  case ACTIVITY_RAILROAD:
    if (!is_ocean_tile(ptile)
	&& !tile_has_special(ptile, S_RAILROAD)
	&& tile_has_special(ptile, S_ROAD)) {
      tile_set_special(ptile, S_RAILROAD);
      return TRUE;
    }
    return FALSE;

  case ACTIVITY_TRANSFORM:
    tile_transform(ptile);
    return TRUE;
    
  case ACTIVITY_FORTRESS:
  case ACTIVITY_PILLAGE: 
  case ACTIVITY_AIRBASE:   
  case ACTIVITY_BASE:
    /* do nothing  - not implemented */
    return FALSE;

  case ACTIVITY_IDLE:
  case ACTIVITY_FORTIFIED:
  case ACTIVITY_SENTRY:
  case ACTIVITY_GOTO:
  case ACTIVITY_EXPLORE:
  case ACTIVITY_UNKNOWN:
  case ACTIVITY_FORTIFYING:
  case ACTIVITY_PATROL_UNUSED:
  case ACTIVITY_LAST:
    /* do nothing - these activities have no effect
       on terrain type or tile specials */
    return FALSE;
  }
  assert(0);
  return FALSE;
}

/****************************************************************************
  Add one entry about pollution situation to buffer.
  Return if there has been any pollution (even prior calling this)
****************************************************************************/
static bool tile_info_pollution(char *buf, int bufsz,
                                const struct tile *ptile,
                                enum tile_special_type special,
                                bool prevp, bool linebreak)
{
  if (tile_has_special(ptile, special)) {
    if (!prevp) {
      if (linebreak) {
        mystrlcat(buf, "\n[", bufsz);
      } else {
        mystrlcat(buf, " [", bufsz);
      }
    } else {
      mystrlcat(buf, "/", bufsz);
    }

    mystrlcat(buf, special_name_translation(special), bufsz);

    return TRUE;
  }

  return prevp;
}

/****************************************************************************
  Return a (static) string with tile name describing terrain and specials.

  Examples:
    "Hills"
    "Hills (Coals)"
    "Hills (Coals) [Pollution]"
****************************************************************************/
const char *tile_get_info_text(const struct tile *ptile, int linebreaks)
{
  static char s[256];
  bool pollution;
  bool lb = FALSE;
  int bufsz = sizeof(s);

  sz_strlcpy(s, terrain_name_translation(tile_terrain(ptile)));
  if (linebreaks & TILE_LB_TERRAIN_RIVER) {
    /* Linebreak needed before next text */
    lb = TRUE;
  }

  if (tile_has_special(ptile, S_RIVER)) {
    if (lb) {
      sz_strlcat(s, "\n");
      lb = FALSE;
    } else {
      sz_strlcat(s, "/");
    }
    sz_strlcat(s, special_name_translation(S_RIVER));
  }
  if (linebreaks & TILE_LB_RIVER_RESOURCE) {
    /* New linebreak requested */
    lb = TRUE;
  }

  if (tile_resource_is_valid(ptile)) {
    if (lb) {
      sz_strlcat(s, "\n");
      lb = FALSE;
    } else {
      sz_strlcat(s, " ");
    }
    cat_snprintf(s, sizeof(s), "(%s)",
		 resource_name_translation(ptile->resource));
  }
  if (linebreaks & TILE_LB_RESOURCE_POLL) {
    /* New linebreak requested */
    lb = TRUE;
  }

  pollution = FALSE;
  pollution = tile_info_pollution(s, bufsz, ptile, S_POLLUTION, pollution, lb);
  pollution = tile_info_pollution(s, bufsz, ptile, S_FALLOUT, pollution, lb);
  if (pollution) {
    sz_strlcat(s, "]");
  }

  return s;
}

/****************************************************************************
  Returns TRUE if the given tile has a base of given type on it.
****************************************************************************/
bool tile_has_base(const struct tile *ptile, const struct base_type *pbase)
{
  return BV_ISSET(ptile->bases, base_index(pbase));
}

/****************************************************************************
  Returns TRUE if the given tile has any bases on it.
****************************************************************************/
bool tile_has_any_bases(const struct tile *ptile)
{
  if (!ptile) {
    return FALSE;
  }
  return BV_ISSET_ANY(ptile->bases);
}

/****************************************************************************
  Returns a completely blank virtual tile (except for the unit list
  vtile->units, which is created for you). Be sure to call virtual_tile_free
  on it when it is no longer needed.
****************************************************************************/
struct tile *create_tile_virtual(void)
{
  struct tile *vtile;

  vtile = fc_calloc(1, sizeof(*vtile));
  vtile->units = unit_list_new();

  return vtile;
}

/****************************************************************************
  Frees all memory used by the virtual tile, including freeing virtual
  units in the tile's unit list and the virtual city on this tile if one
  exists.

  NB: Do not call this on real tiles!
****************************************************************************/
void destroy_tile_virtual(struct tile *vtile)
{
  struct city *vcity;

  if (!vtile) {
    return;
  }

  if (vtile->units) {
    unit_list_iterate(vtile->units, vunit) {
      if (unit_is_virtual(vunit)) {
        destroy_unit_virtual(vunit);
      }
    } unit_list_iterate_end;
    unit_list_free(vtile->units);
    vtile->units = NULL;
  }

  vcity = tile_city(vtile);
  if (vcity) {
    if (city_is_virtual(vcity)) {
      destroy_city_virtual(vcity);
    }
    tile_set_worked(vtile, NULL);
  }

  free(vtile);
}
