/***********************************************************************
 Freeciv - Copyright (C) 2005 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#ifndef FC__EDITPROP_H
#define FC__EDITPROP_H

/* client */
#include "editor.h"

/* client/include */
#include "editgui_g.h"

struct tile_list;
struct property_editor;

void property_editor_clear(struct property_editor *pe);
void property_editor_load_tiles(struct property_editor *pe,
                                const struct tile_list *tiles);
void property_editor_reload(struct property_editor *pe,
                            enum editor_object_type objtype);
void property_editor_popup(struct property_editor *pe,
                           enum editor_object_type objtype);
void property_editor_popdown(struct property_editor *pe);
void property_editor_handle_object_changed(struct property_editor *pe,
                                           enum editor_object_type objtype,
                                           int object_id,
                                           bool remove);
void property_editor_handle_object_created(struct property_editor *pe,
                                           int tag, int object_id);

struct property_editor *editprop_get_property_editor(void);

#endif /* FC__EDITPROP_H */
