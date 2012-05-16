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
#ifndef FC__VISION_H
#define FC__VISION_H

#include "fc_types.h"

#include "improvement.h"		/* bv_imprs */

/****************************************************************************
  Vision for cities and units:

  A vision source has a fixed owner and tile; it changes only in range.
  Vision range is given in radius squared; most such values will come from
  the ruleset.  All vision is circular.

  A vision source is created using vision_new; this creates the source
  without any sight points.  Call vision_change_sight to change the sight
  points of a vision source (generally called from city_refresh_vision
  and unit_refresh vision; this can be called liberally to do updates after
  an effect may have changed the source's vision range).  Clear the sight
  using vision_clear_sight before freeing it with vision_free.

  vision_get_sight returns the sight points of the source.  This should
  only rarely be necessary since all fogging and unfogging operations
  are taken care of internally.

  vision_reveal_tiles() controls whether the vision source can discover
  new (unknown) tiles or simply maintain vision on already-known tiles.
  By default, cities should pass FALSE for this since they cannot
  discover new tiles.

  ***** IMPORTANT *****
  To change any of the parameters given to vision_new - that is, to change
  the vision source's position (tile) or owner - you must create a new
  vision and then clear and free the old vision.  Order is very important
  here since you do not want to fog tiles intermediately.  You must store
  a copy of the old vision source, then create and attach and fill out the
  sight for a new vision source, and only then may you clear and free the
  old vision source.  In most operations you'll want to stick some other
  code in between so that for the bulk of the operation all tiles are
  visible.  For instance to move a unit:

    old_vision = punit->server.vision;
    punit->server.vision = vision_new(unit_owner(punit), dest_tile);
    vision_change_sight(punit->server.vision,
                        get_unit_vision_at(punit, dest_tile));

    ...then do all the work of moving the unit...

    vision_clear_sight(old_vision);
    vision_free(old_vision);

  note that for all the code in the middle both the new and the old
  vision sources are active.  The same process applies when transferring
  a unit or city between players, etc.
****************************************************************************/

/* Invariants: V_MAIN vision ranges must always be more than V_INVIS
 * ranges. */
enum vision_layer {
  V_MAIN,
  V_INVIS,
  V_COUNT
};

struct vision {
  /* These values cannot be changed after initialization. */
  struct player *player;
  struct tile *tile;
  bool can_reveal_tiles;

  /* The radius of the vision source. */
  int radius_sq[V_COUNT];
};

#define ASSERT_VISION(v)						\
 do {									\
   assert((v)->radius_sq[V_MAIN] >= (v)->radius_sq[V_INVIS]);		\
 } while(FALSE);

struct vision *vision_new(struct player *pplayer, struct tile *ptile);
void vision_free(struct vision *vision);

int vision_get_sight(const struct vision *vision, enum vision_layer vlayer);
bool vision_reveal_tiles(struct vision *vision, bool reveal_tiles);

#define vision_layer_iterate(vision)					\
{									\
  enum vision_layer vision = 0;						\
  for (; vision < V_COUNT; vision++) {

#define vision_layer_iterate_end					\
  }									\
}


/* This is copied in maphand.c really_give_tile_info_from_player_to_player(),
 * so be careful with pointers!
 */
struct vision_site {
  char name[MAX_LEN_NAME];
  struct tile *location;		/* Cannot be NULL */
  struct player *owner;			/* May be NULL, always check! */

  int identity;				/* city > IDENTITY_NUMBER_ZERO */
  int size;				/* city size */

  bool occupied;
  bool walls;
  bool happy;
  bool unhappy;

  bv_imprs improvements;
};

#define vision_owner(v) ((v)->owner)
void free_vision_site(struct vision_site *psite);
struct vision_site *create_vision_site(int identity, struct tile *location,
				       struct player *owner);
struct vision_site *create_vision_site_from_city(const struct city *pcity);
void update_vision_site_from_city(struct vision_site *psite,
				  const struct city *pcity);

#endif  /* FC__VISION_H */
