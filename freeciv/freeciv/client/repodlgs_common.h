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

#ifndef FC__REPODLGS_COMMON_H
#define FC__REPODLGS_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "fc_types.h"
#include "improvement.h"
#include "unittype.h"

#include "citydlg_common.h" /* for city request functions */

struct improvement_entry
{
  struct impr_type *type;
  int count, redundant, cost, total_cost;
};

struct unit_entry
{
  struct unit_type *type;
  int count, cost, total_cost;
};

void get_economy_report_data(struct improvement_entry *entries,
			     int *num_entries_used, int *total_cost,
			     int *total_income);
/* This function returns an array with the gold upkeeped units.
 * FIXME: Many clients doesn't yet use this function and show also only the
 * buildings in the economy reports 
 * I think that there should be only one function which returns an array of
 * char* arrays like some other common functions but that means updating all
 * client simultaneously and I simply can't */
void get_economy_report_units_data(struct unit_entry *entries,
				   int *num_entries_used, int *total_cost);

void sell_all_improvements(struct impr_type *pimprove, bool redundant_only,
			   char *message, size_t message_sz);
void disband_all_units(struct unit_type *punittype, bool in_cities_only,
		       char *message, size_t message_sz);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FC__REPODLGS_COMMON_H */
