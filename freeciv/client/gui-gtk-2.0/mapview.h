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

#include <gtk/gtk.h>

#include "fc_types.h"

#include "gtkpixcomm.h"

#include "citydlg_common.h"
#include "mapview_g.h"
#include "mapview_common.h"

#include "canvas.h"
#include "graphics.h"

GdkPixbuf *get_thumb_pixbuf(int onoff);

#define CURSOR_INTERVAL 200 /* milliseconds */

gboolean overview_canvas_expose(GtkWidget *w, GdkEventExpose *ev, gpointer data);
gboolean map_canvas_expose(GtkWidget *w, GdkEventExpose *ev, gpointer data);
gboolean map_canvas_configure(GtkWidget *w, GdkEventConfigure *ev,
			      gpointer data);

void put_unit_gpixmap(struct unit *punit, GtkPixcomm *p);

void put_unit_gpixmap_city_overlays(struct unit *punit, GtkPixcomm *p,
                                    int *upkeep_cost, int happy_cost);

void scrollbar_jump_callback(GtkAdjustment *adj, gpointer hscrollbar);
void update_map_canvas_scrollbars_size(void);

void pixmap_put_overlay_tile(GdkDrawable *pixmap,
			     int canvas_x, int canvas_y,
			     struct sprite *ssprite);

void pixmap_put_overlay_tile_draw(GdkDrawable *pixmap,
				  int canvas_x, int canvas_y,
				  struct sprite *ssprite,
				  bool fog);

#endif  /* FC__MAPVIEW_H */
