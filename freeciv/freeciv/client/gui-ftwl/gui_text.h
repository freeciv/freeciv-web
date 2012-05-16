/********************************************************************** 
 Freeciv - Copyright (C) 2004 - The Freeciv Poject
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifndef FC__GUI_TEXT_H
#define FC__GUI_TEXT_H

#include "fc_types.h"

const char *mapview_get_terrain_tooltip_text(struct tile *ptile);
const char *mapview_get_unit_action_tooltip(struct unit *punit,
					    const char *action,
					    const char *shortcut_);
const char *mapview_get_city_action_tooltip(struct city *pcity,
					    const char *action,
					    const char *shortcut_);
const char *mapview_get_city_tooltip_text(struct city *pcity);
const char *mapview_get_city_info_text(struct city *pcity);
const char *mapview_get_unit_tooltip_text(struct unit *punit);
const char *mapview_get_unit_info_text(struct unit *punit);

#endif				/* FC__GUI_TEXT_H */
