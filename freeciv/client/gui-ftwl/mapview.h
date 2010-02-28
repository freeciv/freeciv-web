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

#include "mapview_g.h"

void mapview_update_actions(void);
void popup_mapcanvas(void);
void popdown_mapcanvas(void);

void set_focus_tile(struct tile *ptile);
void clear_focus_tile(void);
struct tile *get_focus_tile(void);

extern struct ct_string *text_templates[FONT_COUNT];

#endif				/* FC__MAPVIEW_H */
