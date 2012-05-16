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
#ifndef FC__AIUNIT_H
#define FC__AIUNIT_H

#include "combat.h"
#include "fc_types.h"
#include "unittype.h"

/*
 * To prevent integer overflows the product "power * hp * firepower"
 * is divided by POWER_DIVIDER.
 *
 * The constant may be changed since it isn't externally visible used.
 */
#define POWER_DIVIDER 	(POWER_FACTOR * 3)

/* Simple military macros */

/* pplayers_at_war() thinks no contacts equals war, which often is
 * very annoying. */
#define WAR(plr1, plr2) \
  (plr1->diplstates[player_index(plr2)].type == DS_WAR)
#define NEVER_MET(plr1, plr2) \
  (plr1->diplstates[player_index(plr2)].type == DS_NO_CONTACT)
#define DEFENCE_POWER(punit) \
 (unit_type(punit)->defense_strength * unit_type(punit)->hp \
  * unit_type(punit)->firepower)
#define ATTACK_POWER(punit) \
 (unit_type(punit)->attack_strength * unit_type(punit)->hp \
  * unit_type(punit)->firepower)
#define IS_ATTACKER(punit) \
  (unit_type(punit)->attack_strength \
        > unit_type(punit)->transport_capacity)
#define HOSTILE_PLAYER(pplayer, ai, aplayer) \
  (WAR(pplayer, aplayer)         \
   || ai->diplomacy.player_intel[player_index(aplayer)].countdown >= 0)
#define UNITTYPE_COSTS(ut)						\
  (ut->pop_cost * 3 + ut->happy_cost					\
   + ut->upkeep[O_SHIELD] + ut->upkeep[O_FOOD] + ut->upkeep[O_GOLD])

/* Invasion types */
#define INVASION_OCCUPY  0
#define INVASION_ATTACK  1

extern struct unit_type *simple_ai_types[U_LAST];

#define RAMPAGE_ANYTHING                 1
#define RAMPAGE_HUT_OR_BETTER        99998
#define RAMPAGE_FREE_CITY_OR_BETTER  99999
#define BODYGUARD_RAMPAGE_THRESHOLD (SHIELD_WEIGHTING * 4)
bool ai_military_rampage(struct unit *punit, int thresh_adj,
                         int thresh_move);
void ai_manage_units(struct player *pplayer); 
void ai_manage_unit(struct player *pplayer, struct unit *punit);
void ai_manage_military(struct player *pplayer,struct unit *punit);
struct city *find_nearest_safe_city(struct unit *punit);
int could_unit_move_to_tile(struct unit *punit, struct tile *dst_tile);
int look_for_charge(struct player *pplayer, struct unit *punit,
                    struct unit **aunit, struct city **acity);

int turns_to_enemy_city(const struct unit_type *our_type, struct city *acity,
                        int speed, bool go_by_boat, 
                        struct unit *boat, const struct unit_type *boattype);
int turns_to_enemy_unit(const struct unit_type *our_type,
			int speed, struct tile *ptile, 
                        const struct unit_type *enemy_type);
int find_something_to_kill(struct player *pplayer, struct unit *punit, 
			   struct tile **ptile);

int build_cost_balanced(const struct unit_type *punittype);
int unittype_att_rating(const struct unit_type *punittype, int veteran,
                        int moves_left, int hp);
int unit_att_rating(struct unit *punit);
int unit_def_rating_basic(struct unit *punit);
int unit_def_rating_basic_sq(struct unit *punit);
int unittype_def_rating_sq(const struct unit_type *att_type,
			   const struct unit_type *def_type,
			   const struct player *def_player,
                           struct tile *ptile, bool fortified, int veteran);
int kill_desire(int benefit, int attack, int loss, int vuln, int attack_count);

bool is_on_unit_upgrade_path(const struct unit_type *test,
			     const struct unit_type *base);

void update_simple_ai_types(void);

/* Call this after rulesets are loaded */
void unit_class_ai_init(void);

#define simple_ai_unit_type_iterate(_ut)				\
{									\
  struct unit_type *_ut;						\
  int _ut##_index = 0;							\
  while (NULL != (_ut = simple_ai_types[_ut##_index++])) {

#define simple_ai_unit_type_iterate_end					\
  }									\
}

#endif  /* FC__AIUNIT_H */
