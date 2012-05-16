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
#ifndef FC__GOTOHAND_H
#define FC__GOTOHAND_H

#include "map.h"		/* MAP_MAX_ */

/* 
 * The below GOTO result values are ordered by priority, e.g. if unit
 * fought and run out of movepoints, GR_OUT_OF_MOVEPOINTS should be
 * returned.  Not that it is of any importance...
 */
enum goto_result {
  GR_DIED,               /* pretty obvious that */ 
  GR_ARRIVED,            /* arrived to the destination */
  GR_OUT_OF_MOVEPOINTS,  /* no moves left */ 
  GR_WAITING,            /* waiting due to danger, has moves */
  GR_FOUGHT,             /* was stopped due to fighting, has moves */
  GR_FAILED              /* failed for some other reason, has moves */
};

void free_mapqueue(void);

bool is_dist_finite(int dist);
void generate_warmap(struct city *pcity, struct unit *punit);
int air_can_move_between(int moves, struct tile *src_tile,
			 struct tile *dst_tile, struct player *pplayer);

/* all other functions are internal */

#define THRESHOLD 12


bool goto_is_sane(struct unit *punit, struct tile *ptile, bool omni);

struct move_cost_map {
  unsigned char *cost;
  unsigned char *seacost;
  unsigned char *vector;
  int size;

  struct city *warcity; /* so we know what we're dealing with here */
  struct unit *warunit; /* so we know what we're dealing with here */
  struct tile *orig_tile;

  bool invalid;         /* We have invalidated warmap */
};

extern struct move_cost_map warmap;

#define WARMAP_COST(_tile) (assert(!warmap.invalid), warmap.cost[tile_index(_tile)])
#define WARMAP_SEACOST(_tile) (assert(!warmap.invalid), warmap.seacost[tile_index(_tile)])
#define WARMAP_VECTOR(_tile) (assert(!warmap.invalid), warmap.vector[tile_index(_tile)])

#define WARMAP_INVALIDATE {warmap.invalid = TRUE;}

#endif  /* FC__GOTOHAND_H */
