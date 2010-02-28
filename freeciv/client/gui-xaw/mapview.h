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
#ifndef FC__MAPVIEW_H
#define FC__MAPVIEW_H

#include <X11/Intrinsic.h>

#include "fc_types.h"

#include "mapview_g.h"

#include "citydlg_common.h"

#include "graphics.h"

Pixmap get_thumb_pixmap(int onoff);
Pixmap get_citizen_pixmap(enum citizen_category type, int cnum,
			  struct city *pcity);

void put_unit_pixmap_city_overlays(struct unit *punit, Pixmap pm,
                                   int *upkeep_cost, int happy_cost);

void overview_canvas_expose(Widget w, XEvent *event, Region exposed, 
			    void *client_data);
void map_canvas_expose(Widget w, XEvent *event, Region exposed, 
		       void *client_data);

void scrollbar_jump_callback(Widget scrollbar, XtPointer client_data,
			     XtPointer percent_ptr);
void scrollbar_scroll_callback(Widget w, XtPointer client_data,
			     XtPointer position_ptr);

#endif  /* FC__MAPVIEW_H */
