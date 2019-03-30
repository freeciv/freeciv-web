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
#ifndef FC__CITYREP_G_H
#define FC__CITYREP_G_H

#include "support.h"            /* bool type */

#include "fc_types.h"

#include "gui_proto_constructor.h"

GUI_FUNC_PROTO(void, city_report_dialog_popup, bool raise)
GUI_FUNC_PROTO(void, real_city_report_dialog_update, void *unused)
GUI_FUNC_PROTO(void, real_city_report_update_city, struct city *pcity)

/* Area Selection on canvas */
GUI_FUNC_PROTO(void, hilite_cities_from_canvas, void)
GUI_FUNC_PROTO(void, toggle_city_hilite, struct city *pcity, bool on_off)

/* Actually defined in update_queue.c */
void city_report_dialog_update_city(struct city *pcity);
void city_report_dialog_update(void);

#endif  /* FC__CITYREP_G_H */
