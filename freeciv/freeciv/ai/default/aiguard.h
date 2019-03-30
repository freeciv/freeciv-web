/********************************************************************** 
 Freeciv - Copyright (C) 2002 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#ifndef FC__AIGUARD_H
#define FC__AIGUARD_H

#include "support.h"            /* bool type */

#include "fc_types.h"

#ifndef FREECIV_NDEBUG
#define CHECK_GUARD(ait, guard) aiguard_check_guard(ait, guard)
#define CHECK_CHARGE_UNIT(ait, charge) aiguard_check_charge_unit(ait, charge)
#else  /* FREECIV_NDEBUG */
#define CHECK_GUARD(ait, guard) (void)0
#define CHECK_CHARGE_UNIT(ait, charge) (void)0
#endif /* FREECIV_NDEBUG */

void aiguard_check_guard(struct ai_type *ait, const struct unit *guard);
void aiguard_check_charge_unit(struct ai_type *ait, const struct unit *charge);
void aiguard_clear_charge(struct ai_type *ait, struct unit *guard);
void aiguard_clear_guard(struct ai_type *ait, struct unit *charge);
void aiguard_assign_guard_unit(struct ai_type *ait, struct unit *charge,
                               struct unit *guard);
void aiguard_assign_guard_city(struct ai_type *ait, struct city *charge,
                               struct unit *guard);
void aiguard_request_guard(struct ai_type *ait, struct unit *punit);
bool aiguard_wanted(struct ai_type *ait, struct unit *charge);
bool aiguard_has_charge(struct ai_type *ait, struct unit *charge);
bool aiguard_has_guard(struct ai_type *ait, struct unit *charge);
struct unit *aiguard_guard_of(struct ai_type *ait, struct unit *charge);
struct unit *aiguard_charge_unit(struct ai_type *ait, struct unit *guard);
struct city *aiguard_charge_city(struct ai_type *ait, struct unit *guard);
void aiguard_update_charge(struct ai_type *ait, struct unit *guard);

#endif	/* FC__AIGUARD_H */
