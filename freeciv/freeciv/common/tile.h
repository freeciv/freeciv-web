/***********************************************************************
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

#ifndef FC__TILE_H
#define FC__TILE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* utility */
#include "bitvector.h"

/* common */
#include "base.h"
#include "extras.h"
#include "fc_types.h"
#include "player.h"
#include "road.h"
#include "terrain.h"
#include "unitlist.h"

/* network, order dependent */
enum known_type {
 TILE_UNKNOWN = 0,
 TILE_KNOWN_UNSEEN = 1,
 TILE_KNOWN_SEEN = 2,
};

/* Convenience macro for accessing tile coordinates.  This should only be
 * used for debugging. */
#define TILE_XY(ptile) ((ptile) ? index_to_map_pos_x(tile_index(ptile))      \
                                : -1),                                       \
                       ((ptile) ? index_to_map_pos_y(tile_index(ptile))      \
                                : -1)

#define TILE_INDEX_NONE (-1)

struct tile {
  int index; /* Index coordinate of the tile. Used to calculate (x, y) pairs
              * (index_to_map_pos()) and (nat_x, nat_y) pairs
              * (index_to_native_pos()). */
  Continent_id continent;
  bv_extras extras;
  struct extra_type *resource;          /* NULL for no resource */
  struct terrain *terrain;		/* NULL for unknown tiles */
  struct unit_list *units;
  struct city *worked;			/* NULL for not worked */
  struct player *owner;			/* NULL for not owned */
  struct player *extras_owner;
  struct tile *claimer;
  char *label;                          /* NULL for no label */
  char *spec_sprite;
};

/* 'struct tile_list' and related functions. */
#define SPECLIST_TAG tile
#define SPECLIST_TYPE struct tile
#include "speclist.h"
#define tile_list_iterate(tile_list, ptile)                                 \
  TYPED_LIST_ITERATE(struct tile, tile_list, ptile)
#define tile_list_iterate_end LIST_ITERATE_END

/* 'struct tile_hash' and related functions. */
#define SPECHASH_TAG tile
#define SPECHASH_IKEY_TYPE struct tile *
#define SPECHASH_IDATA_TYPE void *
#include "spechash.h"
#define tile_hash_iterate(hash, ptile)                                      \
  TYPED_HASH_KEYS_ITERATE(struct tile *, hash, ptile)
#define tile_hash_iterate_end HASH_KEYS_ITERATE_END


/* Tile accessor functions. */
#define tile_index(_pt_) (_pt_)->index

struct city *tile_city(const struct tile *ptile);

#define tile_continent(_tile) ((_tile)->continent)
/*Continent_id tile_continent(const struct tile *ptile);*/
void tile_set_continent(struct tile *ptile, Continent_id val);

#define tile_owner(_tile) ((_tile)->owner)
/*struct player *tile_owner(const struct tile *ptile);*/
void tile_set_owner(struct tile *ptile, struct player *pplayer,
                    struct tile *claimer);
#define tile_claimer(_tile) ((_tile)->claimer)

#define tile_resource(_tile) ((_tile)->resource)
static inline bool tile_resource_is_valid(const struct tile *ptile)
{ return ptile->resource != NULL
    && BV_ISSET(ptile->extras, ptile->resource->id);
}
/*const struct resource *tile_resource(const struct tile *ptile);*/
void tile_set_resource(struct tile *ptile, struct extra_type *presource);

#define tile_terrain(_tile) ((_tile)->terrain)
/*struct terrain *tile_terrain(const struct tile *ptile);*/
void tile_set_terrain(struct tile *ptile, struct terrain *pterrain);

#define tile_worked(_tile) ((_tile)->worked)
/* struct city *tile_worked(const struct tile *ptile); */
void tile_set_worked(struct tile *ptile, struct city *pcity);

const bv_extras *tile_extras_safe(const struct tile *ptile);
const bv_extras *tile_extras_null(void);
static inline const bv_extras *tile_extras(const struct tile *ptile)
{
  return &(ptile->extras);
}

void tile_set_bases(struct tile *ptile, bv_bases bases);
bool tile_has_base(const struct tile *ptile, const struct base_type *pbase);
void tile_add_base(struct tile *ptile, const struct base_type *pbase);
void tile_remove_base(struct tile *ptile, const struct base_type *pbase);
bool tile_has_base_flag(const struct tile *ptile, enum base_flag_id flag);
bool tile_has_base_flag_for_unit(const struct tile *ptile,
                                 const struct unit_type *punittype,
                                 enum base_flag_id flag);
bool tile_has_refuel_extra(const struct tile *ptile,
                           const struct unit_type *punittype);
bool tile_has_native_base(const struct tile *ptile,
                          const struct unit_type *punittype);
bool tile_has_claimable_base(const struct tile *ptile,
                             const struct unit_type *punittype);
int tile_extras_defense_bonus(const struct tile *ptile,
                              const struct unit_type *punittype);
int tile_extras_class_defense_bonus(const struct tile *ptile,
                                    const struct unit_class *pclass);

bool tile_has_road(const struct tile *ptile, const struct road_type *proad);
void tile_add_road(struct tile *ptile, const struct road_type *proad);
void tile_remove_road(struct tile *ptile, const struct road_type *proad);
bool tile_has_road_flag(const struct tile *ptile, enum road_flag_id flag);
int tile_roads_output_incr(const struct tile *ptile, enum output_type_id o);
int tile_roads_output_bonus(const struct tile *ptile, enum output_type_id o);
bool tile_has_river(const struct tile *tile);

bool tile_extra_apply(struct tile *ptile, struct extra_type *tgt);
bool tile_extra_rm_apply(struct tile *ptile, struct extra_type *tgt);
#define tile_has_extra(ptile, pextra) BV_ISSET(ptile->extras, extra_index(pextra))
bool tile_has_conflicting_extra(const struct tile *ptile, const struct extra_type *pextra);
bool tile_has_visible_extra(const struct tile *ptile, const struct extra_type *pextra);
bool tile_has_cause_extra(const struct tile *ptile, enum extra_cause cause);
void tile_add_extra(struct tile *ptile, const struct extra_type *pextra);
void tile_remove_extra(struct tile *ptile, const struct extra_type *pextra);
bool tile_has_extra_flag(const struct tile *ptile, enum extra_flag_id flag);;

/* Vision related */
enum known_type tile_get_known(const struct tile *ptile,
			      const struct player *pplayer);

bool tile_is_seen(const struct tile *target_tile,
                  const struct player *pow_player);

/* A somewhat arbitrary integer value.  Activity times are multiplied by
 * this amount, and divided by them later before being used.  This may
 * help to avoid rounding errors; however it should probably be removed. */
#define ACTIVITY_FACTOR 10
int tile_activity_time(enum unit_activity activity,
		       const struct tile *ptile,
                       struct extra_type *tgt);

/* These are higher-level functions that handle side effects on the tile. */
void tile_change_terrain(struct tile *ptile, struct terrain *pterrain);
bool tile_apply_activity(struct tile *ptile, Activity_type_id act,
                         struct extra_type *tgt);

#define TILE_LB_TERRAIN_RIVER    (1 << 0)
#define TILE_LB_RIVER_RESOURCE   (1 << 1)
#define TILE_LB_RESOURCE_POLL    (1 << 2)
const char *tile_get_info_text(const struct tile *ptile,
                               bool include_nuisances, int linebreaks);

/* Virtual tiles are tiles that do not exist on the game map. */
struct tile *tile_virtual_new(const struct tile *ptile);
void tile_virtual_destroy(struct tile *vtile);
bool tile_virtual_check(struct tile *vtile);

void *tile_hash_key(const struct tile *ptile);

bool tile_set_label(struct tile *ptile, const char *label);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FC__TILE_H */
