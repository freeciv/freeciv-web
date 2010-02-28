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
#ifndef FC__WLDLG_H
#define FC__WLDLG_H

#include <gtk/gtk.h>

#include "worklist.h"
#include "improvement.h"
#include "unittype.h"
#include "climisc.h"

#include "wldlg_g.h"

/* the global worklist view. */
void popup_worklists_report(void);

/* an individual worklist. */
GtkWidget *create_worklist(void);
void reset_worklist(GtkWidget *editor, struct worklist *pwl,
		    struct city *pcity);
void refresh_worklist(GtkWidget *editor);

void add_worklist_dnd_target(GtkWidget *w);

void blank_max_unit_size(void);

#endif				/* FC__WLDLG_H */
