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
#ifndef FC__MOVEMENT_H
#define FC__MOVEMENT_H

#include "fc_types.h"
#include "unit.h"       /* enum unit_activity */

#define SINGLE_MOVE     3
#define MOVE_COST_RIVER 1
#define MOVE_COST_RAIL  0
#define MOVE_COST_ROAD  1

struct unit_type;
struct terrain;

int unit_move_rate(const struct unit *punit);
bool unit_can_defend_here(const struct unit *punit);
bool can_attack_non_native(const struct unit_type *utype);
bool can_attack_from_non_native(const struct unit_type *utype);

bool is_sailing_unit(const struct unit *punit);
bool is_ground_unit(const struct unit *punit);
bool is_sailing_unittype(const struct unit_type *punittype);
bool is_ground_unittype(const struct unit_type *punittype);

bool is_native_tile(const struct unit_type *punittype,
                    const struct tile *ptile);
bool is_native_tile_to_class(const struct unit_class *punitclass,
                             const struct tile *ptile);
bool is_native_terrain(const struct unit_type *punittype,
                       const struct terrain *pterrain,
                       bv_special special, bv_bases bases);
bool is_native_to_class(const struct unit_class *punitclass,
                        const struct terrain *pterrain,
                        bv_special special, bv_bases bases);
bool is_native_near_tile(const struct unit_type *utype, const struct tile *ptile);
bool can_exist_at_tile(const struct unit_type *utype,
                       const struct tile *ptile);
bool can_unit_exist_at_tile(const struct unit *punit, const struct tile *ptile);
bool can_unit_survive_at_tile(const struct unit *punit,
			      const struct tile *ptile);
bool can_step_taken_wrt_to_zoc(const struct unit_type *punittype,
			       const struct player *unit_owner,
			       const struct tile *src_tile,
			       const struct tile *dst_tile);
bool zoc_ok_move(const struct unit *punit, const struct tile *ptile);
bool can_unit_move_to_tile(const struct unit *punit, const struct tile *ptile,
			   bool igzoc);
enum unit_move_result test_unit_move_to_tile(const struct unit_type *punittype,
					     const struct player *unit_owner,
					     enum unit_activity activity,
					     const struct tile *src_tile,
					     const struct tile *dst_tile,
					     bool igzoc);
bool can_unit_transport(const struct unit *transporter, const struct unit *transported);
bool can_unit_type_transport(const struct unit_type *transporter,
                             const struct unit_class *transported);
int unit_class_transporter_capacity(const struct tile *ptile,
                                    const struct player *pplayer,
                                    const struct unit_class *pclass);
struct unit *find_transport_from_tile(struct unit *punit, struct tile *ptile);

enum unit_move_type move_type_from_str(const char *s);

#endif  /* FC__MOVEMENT_H */
