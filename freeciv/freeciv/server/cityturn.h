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

#ifndef FC__CITYTURN_H
#define FC__CITYTURN_H

#include "shared.h"		/* bool type */

#include "fc_types.h"

struct conn_list;
struct cm_result;

void city_refresh(struct city *pcity);          /* call if city has changed */
void city_refresh_for_player(struct player *pplayer); /* tax/govt changed */

void city_refresh_queue_add(struct city *pcity);
void city_refresh_queue_processing(void);

void auto_arrange_workers(struct city *pcity); /* will arrange the workers */
void apply_cmresult_to_city(struct city *pcity,
			    const struct cm_result *const cmr);

bool city_change_size(struct city *pcity, int new_size);
bool city_reduce_size(struct city *pcity, int pop_loss,
                      struct player *destroyer);
void city_repair_size(struct city *pcity, int change);

void send_global_city_turn_notifications(struct conn_list *dest);
void send_city_turn_notifications(struct conn_list *dest, struct city *pcity);
void update_city_activities(struct player *pplayer);
int city_incite_cost(struct player *pplayer, struct city *pcity);
void remove_obsolete_buildings_city(struct city *pcity, bool refresh);
void remove_obsolete_buildings(struct player *pplayer);

void advisor_choose_build(struct player *pplayer, struct city *pcity);

void nullify_prechange_production(struct city *pcity);

void check_city_migrations(void);

#endif  /* FC__CITYTURN_H */
