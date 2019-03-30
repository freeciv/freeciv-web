/***********************************************************************
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
#ifndef FC__HAPPINESS_H
#define FC__HAPPINESS_H

#include <gtk/gtk.h>

/* client/gui-gtk-4.0 */
#include "citydlg.h"

void happiness_dialog_init(void);
void happiness_dialog_done(void);
GtkWidget *get_top_happiness_display(struct city *pcity,
                                     bool low_dlg,
                                     GtkWidget *win);
void close_happiness_dialog(struct city *pcity);
void refresh_happiness_dialog(struct city *pcity);

#endif /* FC__HAPPINESS_H */
