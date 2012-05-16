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

#include "shared.h"		/* bool type */

#include "fc_types.h"

#ifndef NDEBUG
#define CHECK_GUARD(guard) aiguard_check_guard(guard)
#define CHECK_CHARGE_UNIT(charge) aiguard_check_charge_unit(charge)
#else
#define CHECK_GUARD(guard) (void)0
#define CHECK_CHARGE_UNIT(charge) (void)0
#endif

void aiguard_check_guard(const struct unit *guard);
void aiguard_check_charge_unit(const struct unit *charge);
void aiguard_clear_charge(struct unit *guard);
void aiguard_clear_guard(struct unit *charge);
void aiguard_assign_guard_unit(struct unit *charge, struct unit *guard);
void aiguard_assign_guard_city(struct city *charge, struct unit *guard);
void aiguard_request_guard(struct unit *punit);
bool aiguard_wanted(struct unit *charge);
bool aiguard_has_charge(struct unit *charge);
bool aiguard_has_guard(struct unit *charge);
struct unit *aiguard_guard_of(struct unit *charge);
struct unit *aiguard_charge_unit(struct unit *guard);
struct city *aiguard_charge_city(struct unit *guard);
void aiguard_update_charge(struct unit *guard);

#endif	/* FC__AIGUARD_H */
