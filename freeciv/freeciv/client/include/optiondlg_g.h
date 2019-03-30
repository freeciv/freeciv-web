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
#ifndef FC__OPTIONDLG_G_H
#define FC__OPTIONDLG_G_H

#include "gui_proto_constructor.h"

struct option;
struct option_set;

GUI_FUNC_PROTO(void, option_dialog_popup, const char *name,
               const struct option_set *poptset)
GUI_FUNC_PROTO(void, option_dialog_popdown, const struct option_set *poptset)

GUI_FUNC_PROTO(void, option_gui_update, struct option *poption)
GUI_FUNC_PROTO(void, option_gui_add, struct option *poption)
GUI_FUNC_PROTO(void, option_gui_remove, struct option *poption)

#endif  /* FC__OPTIONDLG_G_H */
