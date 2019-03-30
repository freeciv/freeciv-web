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
#ifndef FC__EDITGUI_G_H
#define FC__EDITGUI_G_H

#include "gui_proto_constructor.h"

struct tile_list;

GUI_FUNC_PROTO(void, editgui_tileset_changed, void)
GUI_FUNC_PROTO(void, editgui_refresh, void)
GUI_FUNC_PROTO(void, editgui_popup_properties,
               const struct tile_list *tiles, int objtype)
GUI_FUNC_PROTO(void, editgui_popdown_all, void)

/* OBJTYPE_* enum values defined in client/editor.h */
GUI_FUNC_PROTO(void, editgui_notify_object_changed,
               int objtype, int id, bool removal)
GUI_FUNC_PROTO(void, editgui_notify_object_created, int tag, int id)

#endif  /* FC__EDITGUI_G_H */
