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
#ifndef FC__VISION_H
#define FC__VISION_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* common */
#include "fc_types.h"

#include "improvement.h" /* bv_imprs */

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

#define vision_layer_iterate(v) {                                           \
  enum vision_layer v;                                                      \
  for (v = 0; v < V_COUNT; v++) {
#define vision_layer_iterate_end }}


typedef short int v_radius_t[V_COUNT];

struct vision {
  /* These values cannot be changed after initialization. */
  struct player *player;
  struct tile *tile;
  bool can_reveal_tiles;

  /* The radius of the vision source. */
  v_radius_t radius_sq;
};

/* Initialize a vision radius array. */
#define V_RADIUS(main_sq, invis_sq, subs_sq) { (main_sq), (invis_sq), (subs_sq) }

#define ASSERT_VISION(v)						\
 do {									\
   fc_assert((v)->radius_sq[V_MAIN] >= (v)->radius_sq[V_INVIS]);	\
   fc_assert((v)->radius_sq[V_MAIN] >= (v)->radius_sq[V_SUBSURFACE]);   \
 } while (FALSE);

struct vision *vision_new(struct player *pplayer, struct tile *ptile);
void vision_free(struct vision *vision);

bool vision_reveal_tiles(struct vision *vision, bool reveal_tiles);

/* This is copied in maphand.c really_give_tile_info_from_player_to_player(),
 * so be careful with pointers!
 */
struct vision_site {
  char name[MAX_LEN_NAME];
  struct tile *location; /* Cannot be NULL */
  struct player *owner;  /* May be NULL, always check! */

  int identity;          /* city > IDENTITY_NUMBER_ZERO */
  citizens size;         /* city size (0 <= size <= MAX_CITY_SIZE) */

  bool occupied;
  bool walls;
  bool happy;
  bool unhappy;
  int style;
  int city_image;

  bv_imprs improvements;
};

#define vision_site_owner(v) ((v)->owner)
void vision_site_destroy(struct vision_site *psite);
struct vision_site *vision_site_new(int identity, struct tile *location,
                                    struct player *owner);
struct vision_site *vision_site_new_from_city(const struct city *pcity);
void vision_site_update_from_city(struct vision_site *psite,
                                  const struct city *pcity);

citizens vision_site_size_get(const struct vision_site *psite);
void vision_site_size_set(struct vision_site *psite, citizens size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__VISION_H */
