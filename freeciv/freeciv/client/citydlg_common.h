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

#ifndef FC__CITYDLG_COMMON_H
#define FC__CITYDLG_COMMON_H

#include <stddef.h>		/* size_t */

#include "shared.h"		/* bool type */
#include "fc_types.h"

#include "city.h"

struct canvas;
struct worklist;

int get_citydlg_canvas_width(void);
int get_citydlg_canvas_height(void);
void generate_citydlg_dimensions(void);

bool city_to_canvas_pos(int *canvas_x, int *canvas_y,
			int city_x, int city_y);
bool canvas_to_city_pos(int *city_x, int *city_y,
			int canvas_x, int canvas_y);
void city_dialog_redraw_map(struct city *pcity,
			    struct canvas *pcanvas);

void get_city_dialog_production(struct city *pcity,
                                char *buffer, size_t buffer_len);
void get_city_dialog_production_full(char *buffer, size_t buffer_len,
				     struct universal target,
				     struct city *pcity);
void get_city_dialog_production_row(char *buf[], size_t column_size,
				    struct universal target,
				    struct city *pcity);

void get_city_dialog_output_text(const struct city *pcity,
				 Output_type_id otype,
				 char *buffer, size_t bufsz);
void get_city_dialog_pollution_text(const struct city *pcity,
				    char *buffer, size_t bufsz);
void get_city_dialog_illness_text(const struct city *pcity,
                                  char *buf, size_t bufsz);

int get_city_citizen_types(struct city *pcity, enum citizen_feeling index,
			   enum citizen_category *citizens);
void city_rotate_specialist(struct city *pcity, int citizen_index);

void activate_all_units(struct tile *ptile);

int city_change_production(struct city *pcity, struct universal target);
int city_set_worklist(struct city *pcity, struct worklist *pworklist);
void city_worklist_commit(struct city *pcity, struct worklist *pwl);

bool city_queue_insert(struct city *pcity, int position,
		       struct universal target);
bool city_queue_clear(struct city *pcity);
bool city_queue_insert_worklist(struct city *pcity, int position,
				struct worklist *worklist);
void city_get_queue(struct city *pcity, struct worklist *pqueue);
bool city_set_queue(struct city *pcity, struct worklist *pqueue);
bool city_can_buy(const struct city *pcity);
int city_sell_improvement(struct city *pcity, Impr_type_id sell_id);
int city_buy_production(struct city *pcity);
int city_change_specialist(struct city *pcity, Specialist_type_id from,
			   Specialist_type_id to);
int city_toggle_worker(struct city *pcity, int city_x, int city_y);
int city_rename(struct city *pcity, const char *name);

#endif /* FC__CITYDLG_COMMON_H */
