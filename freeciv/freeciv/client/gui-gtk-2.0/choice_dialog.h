/********************************************************************** 
 Freeciv - Copyright (C) 1996-2005 - Freeciv Development Team
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#ifndef FC__CHOICE_DIALOG_H
#define FC__CHOICE_DIALOG_H

#include <gtk/gtk.h>

GtkWidget *popup_choice_dialog(GtkWindow *parent, const gchar *dialogname,
			       const gchar *text, ...);

void choice_dialog_set_hide(GtkWidget *dshell, gboolean setting);

GtkWidget *choice_dialog_start(GtkWindow *parent, const gchar *name,
				       const gchar *text);
void choice_dialog_add(GtkWidget *dshell, const gchar *label,
			      GCallback handler, gpointer data);
void choice_dialog_end(GtkWidget *dshell);
void choice_dialog_button_set_sensitive(GtkWidget *shl, int button,
					 gboolean state);
void choice_dialog_button_set_label(GtkWidget *cd, int button,
                                    const char* label);
#endif
