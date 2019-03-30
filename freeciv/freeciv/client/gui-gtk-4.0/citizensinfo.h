/*****************************************************************************
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*****************************************************************************/
#ifndef FC__CITIZENSINFO_H
#define FC__CITIZENSINFO_H

#include <gtk/gtk.h>

struct city;

void citizens_dialog_init(void);
void citizens_dialog_done(void);

GtkWidget *citizens_dialog_display(const struct city *pcity);
void citizens_dialog_refresh(const struct city *pcity);
void citizens_dialog_close(const struct city *pcity);

#endif  /* FC__CITIZENSINFO_H */
