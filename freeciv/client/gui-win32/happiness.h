/********************************************************************** 
 Freeciv - Copyright (C) 2002-2007 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifndef FC__HAPPINESS_H
#define FC__HAPPINESS_H

struct happiness_dlg;
struct happiness_dlg *create_happiness_box(struct city *pcity,
					   struct fcwin_box *hbox,
					   HWND win);
void cleanup_happiness_box(struct happiness_dlg *dlg);
void repaint_happiness_box(struct happiness_dlg *dlg, HDC hdc);
void refresh_happiness_box(struct happiness_dlg *dlg);

#endif				/* FC__HAPPINESS_H */
