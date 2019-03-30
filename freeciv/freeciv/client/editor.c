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

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <stdarg.h>
#include <string.h>

/* utility */
#include "bitvector.h"
#include "fcintl.h"
#include "log.h"
#include "support.h"

/* common */
#include "game.h"
#include "map.h"
#include "movement.h"
#include "packets.h"

/* client */
#include "client_main.h"
#include "climap.h"
#include "control.h"
#include "editor.h"
#include "mapctrl_common.h"
#include "tilespec.h"
#include "zoom.h"

/* client/include */
#include "editgui_g.h"
#include "mapview_g.h"


enum selection_modes {
  SELECTION_MODE_NEW = 0,
  SELECTION_MODE_ADD,
  SELECTION_MODE_REMOVE
};

enum editor_tool_flags {
  ETF_NO_FLAGS  = 0,
  ETF_HAS_VALUE = 1<<0,
  ETF_HAS_SIZE  = 1<<1,
  ETF_HAS_COUNT = 1<<2,
  ETF_HAS_APPLIED_PLAYER = 1<<3,
  ETF_HAS_VALUE_ERASE = 1<<4
};

struct edit_buffer {
  int type_flags;
  struct tile_list *vtiles;
  const struct tile *origin;
};

struct editor_tool {
  int flags;
  enum editor_tool_mode mode;
  int size;
  int count;
  int applied_player_no;
  const char *name;
  int value;
  const char *tooltip;
};

struct editor_state {
  enum editor_tool_type tool;
  struct editor_tool tools[NUM_EDITOR_TOOL_TYPES];

  const struct tile *current_tile;
  bool tool_active;

  bool selrect_active;
  int selrect_start_x;
  int selrect_start_y;
  int selrect_x;
  int selrect_y;
  int selrect_width;
  int selrect_height;

  enum selection_modes selection_mode;

  struct tile_hash *selected_tile_table;
  struct edit_buffer *copybuf;
};

static struct editor_state *editor = NULL;

/************************************************************************//**
  Set tool to some value legal under current ruleset.
****************************************************************************/
static void tool_set_init_value(enum editor_tool_type ett)
{
  struct editor_tool *tool = editor->tools + ett;

  if (ett == ETT_TERRAIN_SPECIAL
      || ett == ETT_ROAD
      || ett == ETT_MILITARY_BASE
      || ett == ETT_TERRAIN_RESOURCE) {
    struct extra_type *first = NULL;

    extra_type_iterate(pextra) {
      if (ett == ETT_ROAD) {
        if (is_extra_caused_by(pextra, EC_ROAD)) {
          first = pextra;
          break;
        }
      } else if (ett == ETT_MILITARY_BASE) {
        if (is_extra_caused_by(pextra, EC_BASE)) {
          first = pextra;
          break;
        }
      } else if (ett == ETT_TERRAIN_RESOURCE) {
        if (is_extra_caused_by(pextra, EC_RESOURCE)) {
          first = pextra;
          break;
        }
      } else {
        /* Considers extras that are neither bases or roads, specials */
        first = pextra;
        break;
      }
    } extra_type_iterate_end;

    if (first != NULL) {
      tool->value = extra_index(first);
    } else {
      tool->value = 0;
    }
  } else {
    tool->value = 0;
  }
}

/************************************************************************//**
  Initialize editor tool data.
****************************************************************************/
static void tool_init(enum editor_tool_type ett, const char *name,
                      int flags, const char *tooltip)
{
  struct editor_tool *tool;

  if (!editor || !(0 <= ett && ett < NUM_EDITOR_TOOL_TYPES)) {
    return;
  }

  tool = editor->tools + ett;

  if (ett == ETT_COPYPASTE) {
    tool->mode = ETM_COPY;
  } else {
    tool->mode = ETM_PAINT;
  }
  tool->name = name;
  tool->flags = flags;
  tool->tooltip = tooltip;
  tool->size = 1;
  tool->count = 1;
  tool->applied_player_no = 0;

  tool_set_init_value(ett);
}

/************************************************************************//**
  Adjust editor for changed ruleset.
****************************************************************************/
void editor_ruleset_changed(void)
{
  int t;

  for (t = 0; t < NUM_EDITOR_TOOL_TYPES; t++) {
    tool_set_init_value(t);
  }
}

/************************************************************************//**
  Initialize the client's editor state information to some suitable default
  values. This only needs to be done once at client start.
****************************************************************************/
void editor_init(void)
{
  fc_assert(editor == NULL);

  editor = fc_calloc(1, sizeof(struct editor_state));

  tool_init(ETT_TERRAIN, _("Terrain"),
            ETF_HAS_VALUE | ETF_HAS_SIZE,
            _("Change tile terrain.\nShortcut: t\n"
              "Select terrain type: shift+t or right-click here."));
  tool_init(ETT_TERRAIN_RESOURCE, _("Terrain Resource"),
            ETF_HAS_VALUE | ETF_HAS_SIZE,
            _("Change tile terrain resources.\nShortcut: r\n"
              "Select resource type: shift+r or right-click here."));
  tool_init(ETT_TERRAIN_SPECIAL, _("Terrain Special"), ETF_HAS_VALUE
            | ETF_HAS_SIZE | ETF_HAS_VALUE_ERASE,
            _("Modify tile specials.\nShortcut: s\n"
              "Select special type: shift+s or right-click here."));
  tool_init(ETT_ROAD, _("Road"), ETF_HAS_VALUE
            | ETF_HAS_SIZE | ETF_HAS_VALUE_ERASE,
            _("Modify roads on tile.\nShortcut: p\n"
              "Select road type: shift+p or right-click here."));
  tool_init(ETT_MILITARY_BASE, _("Military Base"), ETF_HAS_VALUE
            | ETF_HAS_SIZE | ETF_HAS_VALUE_ERASE,
            _("Create a military base.\nShortcut: m\n"
              "Select base type: shift+m or right-click here."));
  tool_init(ETT_UNIT, _("Unit"), ETF_HAS_VALUE | ETF_HAS_COUNT
            | ETF_HAS_APPLIED_PLAYER | ETF_HAS_VALUE_ERASE,
            _("Create unit.\nShortcut: u\nSelect unit "
              "type: shift+u or right-click here."));
  tool_init(ETT_CITY, _("City"), ETF_HAS_SIZE | ETF_HAS_APPLIED_PLAYER,
            _("Create city.\nShortcut: c"));
  tool_init(ETT_VISION, _("Vision"), ETF_HAS_SIZE,
            _("Modify player's tile knowledge.\nShortcut: v"));
  tool_init(ETT_STARTPOS, _("Start Position"), ETF_NO_FLAGS,
            _("Place a start position which allows any nation to "
              "start at the tile. To allow only certain nations to "
              "start there, middle click on the start position on "
              "the map and use the property editor.\nShortcut: b"));

  tool_init(ETT_COPYPASTE, _("Copy/Paste"), ETF_HAS_SIZE,
            _("Copy and paste tiles.\n"
              "Shortcut for copy mode: shift-c\n"
              "Shoftcut for paste mode: shift-v"));
  editor->copybuf = edit_buffer_new(EBT_ALL);

  editor->selected_tile_table = tile_hash_new();
  tile_hash_set_no_shrink(editor->selected_tile_table, TRUE);
}

/************************************************************************//**
  Clear the editor data which is game dependent.
****************************************************************************/
void editor_clear(void)
{
  fc_assert_ret(editor != NULL);

  edit_buffer_clear(editor->copybuf);
  tile_hash_clear(editor->selected_tile_table);
}

/************************************************************************//**
  Free the client's editor.
****************************************************************************/
void editor_free(void)
{
  if (editor != NULL) {
    edit_buffer_free(editor->copybuf);
    tile_hash_destroy(editor->selected_tile_table);
    free(editor);
    editor = NULL;
  }
}

/************************************************************************//**
  Set the current tool to be used by the editor.
****************************************************************************/
void editor_set_tool(enum editor_tool_type ett)
{
  if (editor == NULL) {
    return;
  }

  if (!(0 <= ett && ett < NUM_EDITOR_TOOL_TYPES)) {
    return;
  }

  editor->tool = ett;
}

/************************************************************************//**
  Get the current tool used by the editor.
****************************************************************************/
enum editor_tool_type editor_get_tool(void)
{
  if (editor == NULL) {
    return NUM_EDITOR_TOOL_TYPES;
  }

  return editor->tool;
}

/************************************************************************//**
  Set the mode for the editor tool.
****************************************************************************/
void editor_tool_set_mode(enum editor_tool_type ett,
                          enum editor_tool_mode etm)
{
  if (editor == NULL || !(0 <= ett && ett < NUM_EDITOR_TOOL_TYPES)
      || !(0 <= etm && etm < NUM_EDITOR_TOOL_MODES)
      || !editor_tool_has_mode(ett, etm)) {
    return;
  }

  editor->tools[ett].mode = etm;
}

/************************************************************************//**
  Return TRUE if the given tool supports the given mode.
****************************************************************************/
bool editor_tool_has_mode(enum editor_tool_type ett,
                          enum editor_tool_mode etm)
{
  if (editor == NULL || !(0 <= ett && ett < NUM_EDITOR_TOOL_TYPES)
      || !(0 <= etm && etm < NUM_EDITOR_TOOL_MODES)) {
    return FALSE;
  }

  if (etm == ETM_COPY || etm == ETM_PASTE) {
    return ett == ETT_COPYPASTE;
  }

  if (ett == ETT_COPYPASTE) {
    return etm == ETM_COPY || etm == ETM_PASTE;
  }

  return TRUE;
}

/************************************************************************//**
  Get the mode for the tool.
****************************************************************************/
enum editor_tool_mode editor_tool_get_mode(enum editor_tool_type ett)
{
  if (editor == NULL || !(0 <= ett && ett < NUM_EDITOR_TOOL_TYPES)) {
    return NUM_EDITOR_TOOL_MODES;
  }
  return editor->tools[ett].mode;
}

/************************************************************************//**
  Returns TRUE if the *client* is in edit mode.
****************************************************************************/
bool editor_is_active(void)
{
  return can_conn_edit(&client.conn);
}

/************************************************************************//**
  Returns TRUE if the given tool should be made available to the user via
  the editor GUI. For example, this will return FALSE for ETT_MILITARY_BASE
  if there are no bases defined in the ruleset.

  NB: This depends on the ruleset information received from the server, so
  it will return FALSE if the client does not have it yet.
****************************************************************************/
bool editor_tool_is_usable(enum editor_tool_type ett)
{
  if (!editor || !(0 <= ett && ett < NUM_EDITOR_TOOL_TYPES)) {
    return FALSE;
  }

  switch (ett) {
  case ETT_MILITARY_BASE:
    return base_count() > 0;
  case ETT_ROAD:
    return road_count() > 0;
  case ETT_TERRAIN_RESOURCE:
    return game.control.num_resource_types > 0;
  case ETT_UNIT:
    return utype_count() > 0;
  default:
    break;
  }

  return TRUE;
}

/************************************************************************//**
  Returns TRUE if the given tool type has sub-values (e.g. the terrain
  tool has values corresponding to the terrain types).
****************************************************************************/
bool editor_tool_has_value(enum editor_tool_type ett)
{
  if (!editor || !(0 <= ett && ett < NUM_EDITOR_TOOL_TYPES)) {
    return FALSE;
  }
  return editor->tools[ett].flags & ETF_HAS_VALUE;
}

/************************************************************************//**
  Set the value ID for the given tool. How the value is interpreted depends
  on the tool type.
****************************************************************************/
void editor_tool_set_value(enum editor_tool_type ett, int value)
{
  if (!editor || !(0 <= ett && ett < NUM_EDITOR_TOOL_TYPES)
      || !editor_tool_has_value(ett)) {
    return;
  }
  editor->tools[ett].value = value;
}

/************************************************************************//**
  Get the current tool sub-value.
****************************************************************************/
int editor_tool_get_value(enum editor_tool_type ett)
{
  if (!editor || !(0 <= ett && ett < NUM_EDITOR_TOOL_TYPES)
      || !editor_tool_has_value(ett)) {
    return 0;
  }

  return editor->tools[ett].value;
}

/************************************************************************//**
  Record the start of the selection rectangle.
****************************************************************************/
static void editor_start_selection_rectangle(int canvas_x, int canvas_y)
{
  if (!editor) {
    return;
  }

  if (editor->selection_mode == SELECTION_MODE_NEW
      && editor_selection_count() > 0) {
    editor_selection_clear();
    update_map_canvas_visible();
  }

  editor->selrect_start_x = canvas_x;
  editor->selrect_start_y = canvas_y;
  editor->selrect_width = 0;
  editor->selrect_height = 0;
  editor->selrect_active = TRUE;
}

/************************************************************************//**
  Temporary convenience function to work-around the fact that certain
  special values (S_RESOURCE_VALID) do not in fact
  correspond to drawable special types.
****************************************************************************/
static inline bool tile_really_has_any_specials(const struct tile *ptile)
{
  if (!ptile) {
    return FALSE;
  }

  extra_type_by_cause_iterate(EC_SPECIAL, pextra) {
    if (tile_has_extra(ptile, pextra)) {
      return TRUE;
    }
  } extra_type_by_cause_iterate_end;

  return FALSE;
}

/************************************************************************//**
  Set the editor's current applied player number according to what exists
  on the given tile.
****************************************************************************/
static void editor_grab_applied_player(const struct tile *ptile)
{
  int apno = -1;

  if (!editor || !ptile) {
    return;
  }

  if (client_has_player()
      && tile_get_known(ptile, client_player()) == TILE_UNKNOWN) {
    return;
  }

  if (tile_city(ptile) != NULL) {
    apno = player_number(city_owner(tile_city(ptile)));
  } else if (unit_list_size(ptile->units) > 0) {
    struct unit *punit = unit_list_get(ptile->units, 0);

    apno = player_number(unit_owner(punit));
  } else if (tile_owner(ptile) != NULL) {
    apno = player_number(tile_owner(ptile));
  }
  
  if (player_by_number(apno) != NULL) {
    editor_tool_set_applied_player(editor_get_tool(), apno);
    editgui_refresh();
  }
}

/************************************************************************//**
  Set the editor's current tool according to what exists at the given tile.
****************************************************************************/
static void editor_grab_tool(const struct tile *ptile)
{
  int ett = -1, value = 0;
  struct extra_type *first_base = NULL;
  struct extra_type *first_road = NULL;
  struct extra_type *first_resource = NULL;

  if (!editor) {
    return;
  }

  if (!ptile) {
    return;
  }

  extra_type_by_cause_iterate(EC_BASE, pextra) {
    if (tile_has_extra(ptile, pextra)) {
      first_base = pextra;
      break;
    }
  } extra_type_by_cause_iterate_end;

  extra_type_by_cause_iterate(EC_ROAD, pextra) {
    if (tile_has_extra(ptile, pextra)) {
      first_road = pextra;
      break;
    }
  } extra_type_by_cause_iterate_end;

  extra_type_by_cause_iterate(EC_RESOURCE, pextra) {
    if (tile_has_extra(ptile, pextra)) {
      first_resource = pextra;
      break;
    }
  } extra_type_by_cause_iterate_end;

  if (client_has_player()
      && tile_get_known(ptile, client_player()) == TILE_UNKNOWN) {
    ett = ETT_VISION;

  } else if (tile_city(ptile)) {
    ett = ETT_CITY;

  } else if (unit_list_size(ptile->units) > 0) {
    int max_score = 0, score;
    struct unit *grabbed_punit = NULL;

    unit_list_iterate(ptile->units, punit) {
      if (uclass_has_flag(unit_class_get(punit), UCF_UNREACHABLE)) {
        score = 5;
      } else if (utype_move_type(unit_type_get(punit)) == UMT_LAND) {
        score = 4;
      } else if (utype_move_type(unit_type_get(punit)) == UMT_SEA) {
        score = 3;
      } else {
        score = 2;
      }
      if (unit_transported(punit)) {
        score = 1;
      }

      if (score > max_score) {
        max_score = score;
        grabbed_punit = punit;
      }
    } unit_list_iterate_end;

    if (grabbed_punit) {
      ett = ETT_UNIT;
      value = utype_number(unit_type_get(grabbed_punit));
    }
  } else if (first_base != NULL) {
    ett = ETT_MILITARY_BASE;
    value = extra_index(first_base);

  } else if (first_road != NULL) {
    ett = ETT_ROAD;
    value = extra_index(first_road);

  } else if (tile_really_has_any_specials(ptile)) {
    struct extra_type *specials_array[MAX_EXTRA_TYPES];
    int count = 0, i;
    struct extra_type *special = NULL;

    extra_type_by_cause_iterate(EC_SPECIAL, s) {
      specials_array[count++] = s;
    } extra_type_by_cause_iterate_end;

    /* Grab specials in reverse order of enum tile_special_type. */

    for (i = count - 1; i >= 0; i--) {
      if (tile_has_extra(ptile, specials_array[i])) {
        special = specials_array[i];
        break;
      }
    }

    if (special != NULL) {
      ett = ETT_TERRAIN_SPECIAL;
      value = extra_index(special);
    }
  } else if (tile_resource(ptile) != NULL) {
    ett = ETT_TERRAIN_RESOURCE;
    value = extra_index(first_resource);

  } else if (tile_terrain(ptile) != NULL) {
    ett = ETT_TERRAIN;
    value = terrain_number(tile_terrain(ptile));
  }

  if (ett < 0) {
    return;
  }

  editor_set_tool(ett);
  if (editor_tool_has_value(ett)) {
    editor_tool_set_value(ett, value);
  }
  editgui_refresh();
}

/************************************************************************//**
  Returns TRUE if the given tile has some objects with editable properties.
****************************************************************************/
static inline bool can_edit_tile_properties(struct tile *ptile)
{
  return ptile != NULL;
}

/************************************************************************//**
  Handle a request to edit the properties for the given tile. If the tile
  is part of a selection, then all selected tiles are passed to the
  property editor.
****************************************************************************/
static void popup_properties(struct tile *ptile)
{
  struct tile_list *tiles;

  if (!ptile) {
    return;
  }

  tiles = tile_list_new();

  if (editor_tile_is_selected(ptile)) {
    tile_hash_iterate(editor->selected_tile_table, sel_tile) {
      if (can_edit_tile_properties(sel_tile)) {
        tile_list_append(tiles, sel_tile);
      }
    } tile_hash_iterate_end;
  } else {
    if (can_edit_tile_properties(ptile)) {
      tile_list_append(tiles, ptile);
    }
  }

  editgui_popup_properties(tiles, NUM_OBJTYPES);

  tile_list_destroy(tiles);
}

/************************************************************************//**
  Handle a user's mouse button press at the given point on the map canvas.
****************************************************************************/
void editor_mouse_button_press(int canvas_x, int canvas_y,
                               int button, int modifiers)
{
  struct tile *ptile;

  if (editor == NULL) {
    return;
  }

  ptile = canvas_pos_to_tile(canvas_x, canvas_y);
  if (ptile == NULL) {
    return;
  }

  switch (button) {

  case MOUSE_BUTTON_LEFT:
    if (modifiers == EKM_SHIFT) {
      editor_grab_tool(ptile);
    } else if (modifiers == EKM_CTRL) {
      editor_grab_applied_player(ptile);
    } else if (modifiers == EKM_NONE) {
      editor->tool_active = TRUE;
      editor_apply_tool(ptile, FALSE);
      editor_notify_edit_finished();
      editor_set_current_tile(ptile);
    }
    break;

  case MOUSE_BUTTON_RIGHT:
    if (modifiers == (EKM_ALT | EKM_CTRL)) {
      popup_properties(ptile);
      break;
    }
    
    if (modifiers == EKM_SHIFT) {
      editor->selection_mode = SELECTION_MODE_ADD;
    } else if (modifiers == EKM_ALT) {
      editor->selection_mode = SELECTION_MODE_REMOVE;
    } else if (modifiers == EKM_NONE) {
      editor->selection_mode = SELECTION_MODE_NEW;
    } else {
      break;
    }
    editor_start_selection_rectangle(canvas_x, canvas_y);
    break;

  case MOUSE_BUTTON_MIDDLE:
    if (modifiers == EKM_NONE) {
      popup_properties(ptile);
    }
    break;

  default:
    break;
  }
}

/************************************************************************//**
  Record and handle the end of the selection rectangle.
****************************************************************************/
static void editor_end_selection_rectangle(int canvas_x, int canvas_y)
{
  int w, h;

  if (!editor) {
    return;
  }

  editor->selrect_active = FALSE;

  if (editor->selrect_width <= 0 || editor->selrect_height <= 0) {
    struct tile *ptile;
    
    ptile = canvas_pos_to_tile(canvas_x, canvas_y);
    if (ptile && editor->selection_mode == SELECTION_MODE_ADD) {
      editor_selection_add(ptile);
    } else if (ptile && editor->selection_mode == SELECTION_MODE_REMOVE) {
      editor_selection_remove(ptile);
    } else {
      recenter_button_pressed(canvas_x, canvas_y);
      return;
    }

    if (ptile) {
      refresh_tile_mapcanvas(ptile, TRUE, TRUE);
    }

    return;
  }

  gui_rect_iterate(mapview.gui_x0 + editor->selrect_x,
                   mapview.gui_y0 + editor->selrect_y,
                   editor->selrect_width, editor->selrect_height,
                   ptile, pedge, pcorner, map_zoom) {
    if (ptile == NULL) {
      continue;
    }
    if (editor->selection_mode == SELECTION_MODE_NEW
        || editor->selection_mode == SELECTION_MODE_ADD) {
      editor_selection_add(ptile);
    } else if (editor->selection_mode == SELECTION_MODE_REMOVE) {
      editor_selection_remove(ptile);
    }
  } gui_rect_iterate_end;

  w = tileset_tile_width(tileset);
  h = tileset_tile_height(tileset);

  update_map_canvas(editor->selrect_x - w,
                    editor->selrect_y - h,
                    editor->selrect_width + 2 * w,
                    editor->selrect_height + 2 * h);
  flush_dirty();
}

/************************************************************************//**
  Draws the editor selection rectangle using draw_selection_rectangle().
****************************************************************************/
static void editor_draw_selrect(void)
{
  if (!editor) {
    return;
  }

  if (editor->selrect_active && editor->selrect_width > 0
      && editor->selrect_height > 0) {
    draw_selection_rectangle(editor->selrect_x,
                             editor->selrect_y,
                             editor->selrect_width,
                             editor->selrect_height);
  }
}

/************************************************************************//**
  Handle the release of a mouse button click.
****************************************************************************/
void editor_mouse_button_release(int canvas_x, int canvas_y,
                                 int button, int modifiers)
{
  switch (button) {

  case MOUSE_BUTTON_LEFT:
    editor_set_current_tile(NULL);
    editor->tool_active = FALSE;
    break;

  case MOUSE_BUTTON_RIGHT:
    if (editor->selrect_active) {
      editor_end_selection_rectangle(canvas_x, canvas_y);
    }
    break;

  case MOUSE_BUTTON_MIDDLE:
    break;

  default:
    break;
  }
}

/************************************************************************//**
  Handle a change in the size of the selection rectangle. The given point
  is the new extremity of the rectangle.
****************************************************************************/
static void editor_resize_selection_rectangle(int canvas_x, int canvas_y)
{
  int xl, yt, xr, yb;

  if (editor->selrect_start_x <= canvas_x) {
    xl = editor->selrect_start_x;
    xr = canvas_x;
  } else {
    xl = canvas_x;
    xr = editor->selrect_start_x;
  }

  if (editor->selrect_start_y <= canvas_y) {
    yt = editor->selrect_start_y;
    yb = canvas_y;
  } else {
    yt = canvas_y;
    yb = editor->selrect_start_y;
  }

  /* Erase the previously drawn rectangle. */
  editor_draw_selrect();

  if (xl == xr || yt == yb) {
    editor->selrect_width = 0;
    editor->selrect_height = 0;
    return;
  }

  editor->selrect_x = xl;
  editor->selrect_y = yt;
  editor->selrect_width = xr - xl;
  editor->selrect_height = yb - yt;

  editor_draw_selrect();
}

/************************************************************************//**
  Handle the mouse moving over the map canvas.
****************************************************************************/
void editor_mouse_move(int canvas_x, int canvas_y, int modifiers)
{
  const struct tile *ptile, *old;

  if (!editor) {
    return;
  }

  old = editor_get_current_tile();
  ptile = canvas_pos_to_tile(canvas_x, canvas_y);

  if (!ptile) {
    return;
  }

  if (editor->tool_active && old != NULL && old != ptile) {
    editor_apply_tool(ptile, FALSE);
    editor_notify_edit_finished();
    editor_set_current_tile(ptile);
  }

  if (editor->selrect_active) {
    editor_resize_selection_rectangle(canvas_x, canvas_y);
  }
}

/************************************************************************//**
  Notify the server that a batch of edits has completed. This is used as
  a hint for the server to now do any checks it has saved while the batch
  was being processed.
****************************************************************************/
void editor_notify_edit_finished(void)
{
  send_packet_edit_check_tiles(&client.conn);
}

/************************************************************************//**
  Apply the current editor tool to the given tile. This function is
  suitable to called over multiple tiles at once. Once the batch of
  operations is finished you should call editor_notify_edit_finished.
  The 'part_of_selection' parameter should be TRUE if the tool is
  being applied to a tile from a selection.
****************************************************************************/
void editor_apply_tool(const struct tile *ptile,
                       bool part_of_selection)
{
  enum editor_tool_type ett;
  enum editor_tool_mode etm;
  int value, size, count, apno, tile, id;
  bool erase;
  struct connection *my_conn = &client.conn;

  if (editor == NULL || ptile == NULL) {
    return;
  }

  ett = editor_get_tool();
  etm = editor_tool_get_mode(ett);
  size = editor_tool_get_size(ett);
  count = editor_tool_get_count(ett);
  value = editor_tool_get_value(ett);
  apno = editor_tool_get_applied_player(ett);

  if (ett != ETT_VISION && !client_is_global_observer()
      && client_has_player()
      && tile_get_known(ptile, client_player()) == TILE_UNKNOWN) {
    return;
  }

  if (editor_tool_has_applied_player(ett)
      && player_by_number(apno) == NULL) {
    return;
  }

  if (ett == ETT_COPYPASTE) {
    struct edit_buffer *ebuf;
    ebuf = editor_get_copy_buffer();
    if (etm == ETM_COPY) {
      if (part_of_selection) {
        edit_buffer_copy(ebuf, ptile);
      } else {
        edit_buffer_clear(ebuf);
        edit_buffer_copy_square(ebuf, ptile, size);
        editgui_refresh();
      }
    } else if (etm == ETM_PAINT || etm == ETM_PASTE) {
      edit_buffer_paste(ebuf, ptile);
    }
    return;
  }

  if (part_of_selection && ett != ETT_CITY) {
    size = 1;
  }

  erase = (etm == ETM_ERASE);
  tile = tile_index(ptile);

  switch (ett) {

  case ETT_TERRAIN:
    dsend_packet_edit_tile_terrain(my_conn, tile, erase ? 0 : value, size);
    break;

  case ETT_TERRAIN_RESOURCE:
  case ETT_TERRAIN_SPECIAL:
  case ETT_ROAD:
  case ETT_MILITARY_BASE:
    dsend_packet_edit_tile_extra(my_conn, tile, value, erase, size);
    break;

  case ETT_UNIT:
    if (erase) {
      dsend_packet_edit_unit_remove(my_conn, apno, tile, value, count);
    } else {
      dsend_packet_edit_unit_create(my_conn, apno, tile, value, count, 0);
    }
    break;

  case ETT_CITY:
    if (erase) {
      struct city *pcity = tile_city(ptile);
      if (pcity != NULL) {
        id = pcity->id;
        dsend_packet_edit_city_remove(my_conn, id);
      }
    } else {
      dsend_packet_edit_city_create(my_conn, apno, tile, size, 0);
    }
    break;

  case ETT_VISION:
    if (client_has_player()) {
      id = client_player_number();
      dsend_packet_edit_player_vision(my_conn, id, tile, !erase, size);
    }
    break;

  case ETT_STARTPOS:
    dsend_packet_edit_startpos(my_conn, tile, erase, 0);
    break;

  default:
    break;
  }
}

/************************************************************************//**
  Sets the tile currently assumed to be under the user's mouse pointer.
****************************************************************************/
void editor_set_current_tile(const struct tile *ptile)
{
  if (editor == NULL) {
    return;
  }
  
  editor->current_tile = ptile;
}

/************************************************************************//**
  Get the tile that the user's mouse pointer is currently over.
****************************************************************************/
const struct tile *editor_get_current_tile(void)
{
  if (editor == NULL) {
    return NULL;
  }
  
  return editor->current_tile;
}

/************************************************************************//**
  Toggle the current tool mode between the given mode and ETM_PAINT (or
  ETM_COPY for the copy & paste tool).
****************************************************************************/
void editor_tool_toggle_mode(enum editor_tool_type ett,
                             enum editor_tool_mode etm)
{
  if (!editor_tool_has_mode(ett, etm)) {
    return;
  }
  if (editor_tool_get_mode(ett) == etm) {
    editor_tool_set_mode(ett, ett == ETT_COPYPASTE
                         ? ETM_COPY : ETM_PAINT);
  } else {
    editor_tool_set_mode(ett, etm);
  }
}

/************************************************************************//**
  Set the editor tool mode to the next available mode.
****************************************************************************/
void editor_tool_cycle_mode(enum editor_tool_type ett)
{
  int mode, count;
  bool found = FALSE;

  mode = editor_tool_get_mode(ett);
  if (!(0 <= mode && mode < NUM_EDITOR_TOOL_MODES)) {
    return;
  }

  for (count = 0; count < NUM_EDITOR_TOOL_MODES; count++) {
    mode = (mode + 1) % NUM_EDITOR_TOOL_MODES;
    if (editor_tool_has_mode(ett, mode)) {
      found = TRUE;
      break;
    }
  }

  if (found) {
    editor_tool_set_mode(ett, mode);
  }
}

/************************************************************************//**
  Unselect all selected tiles.
****************************************************************************/
void editor_selection_clear(void)
{
  if (!editor) {
    return;
  }
  tile_hash_clear(editor->selected_tile_table);
}

/************************************************************************//**
  Add the given tile to the current selection.
****************************************************************************/
void editor_selection_add(const struct tile *ptile)
{
  if (!editor || !ptile) {
    return;
  }
  tile_hash_insert(editor->selected_tile_table, ptile, NULL);
}

/************************************************************************//**
  Remove the given tile from the current selection.
****************************************************************************/
void editor_selection_remove(const struct tile *ptile)
{
  if (!editor || !ptile) {
    return;
  }
  tile_hash_remove(editor->selected_tile_table, ptile);
}

/************************************************************************//**
  Returns TRUE if the given tile is selected.
****************************************************************************/
bool editor_tile_is_selected(const struct tile *ptile)
{
  if (!editor || !ptile) {
    return FALSE;
  }
  return tile_hash_lookup(editor->selected_tile_table, ptile, NULL);
}

/************************************************************************//**
  Apply the current editor tool to all tiles in the current selection.
****************************************************************************/
void editor_apply_tool_to_selection(void)
{
  enum editor_tool_type ett;

  if (!editor || editor_selection_count() <= 0) {
    return;
  }

  ett = editor_get_tool();
  if (editor_tool_get_mode(ett) == ETM_COPY) {
    struct edit_buffer *ebuf;
    ebuf = editor_get_copy_buffer();
    edit_buffer_clear(ebuf);
    edit_buffer_set_origin(ebuf, editor_get_selection_center());
  }

  connection_do_buffer(&client.conn);
  tile_hash_iterate(editor->selected_tile_table, ptile) {
    editor_apply_tool(ptile, TRUE);
  } tile_hash_iterate_end;
  editor_notify_edit_finished();
  connection_do_unbuffer(&client.conn);

  if (editor_tool_get_mode(ett) == ETM_COPY) {
    editgui_refresh();
  }
}

/************************************************************************//**
  Get the translated name of the given tool type.
****************************************************************************/
const char *editor_tool_get_name(enum editor_tool_type ett)
{
  if (!editor || !(0 <= ett && ett < NUM_EDITOR_TOOL_TYPES)) {
    return "";
  }

  return editor->tools[ett].name;
}

/************************************************************************//**
  Get the translated name of the given tool value. If no such name exists,
  returns an empty string.
****************************************************************************/
const char *editor_tool_get_value_name(enum editor_tool_type emt, int value)
{
  struct terrain *pterrain;
  struct unit_type *putype;
  struct extra_type *pextra;

  if (!editor) {
    return "";
  }

  switch (emt) {
  case ETT_TERRAIN:
    pterrain = terrain_by_number(value);
    return pterrain ? terrain_name_translation(pterrain) : "";
    break;
  case ETT_TERRAIN_RESOURCE:
  case ETT_TERRAIN_SPECIAL:
  case ETT_ROAD:
  case ETT_MILITARY_BASE:
    pextra = extra_by_number(value);
    return pextra != NULL ? extra_name_translation(pextra) : "";
    break;
  case ETT_UNIT:
    putype = utype_by_number(value);
    return putype ? utype_name_translation(putype) : "";
    break;
  default:
    break;
  }
  return "";
}

/************************************************************************//**
  Return TRUE if the given editor tool uses the 'size' parameter.
****************************************************************************/
bool editor_tool_has_size(enum editor_tool_type ett)
{
  if (!editor || !(0 <= ett && ett < NUM_EDITOR_TOOL_TYPES)) {
    return FALSE;
  }
  return editor->tools[ett].flags & ETF_HAS_SIZE;
}

/************************************************************************//**
  Returns the current size parameter for the given editor tools.
****************************************************************************/
int editor_tool_get_size(enum editor_tool_type ett)
{
  if (!editor || !(0 <= ett && ett < NUM_EDITOR_TOOL_TYPES)) {
    return 1;
  }
  return editor->tools[ett].size;
}

/************************************************************************//**
  Sets the size parameter for the given tool.
****************************************************************************/
void editor_tool_set_size(enum editor_tool_type ett, int size)
{
  if (!editor || !(0 <= ett && ett < NUM_EDITOR_TOOL_TYPES)) {
    return;
  }
  editor->tools[ett].size = MAX(1, size);
}

/************************************************************************//**
  Return TRUE if it is meaningful for the given tool to use the 'count'
  parameter.
****************************************************************************/
bool editor_tool_has_count(enum editor_tool_type ett)
{
  if (!editor || !(0 <= ett && ett < NUM_EDITOR_TOOL_TYPES)) {
    return FALSE;
  }
  return editor->tools[ett].flags & ETF_HAS_COUNT;
}

/************************************************************************//**
  Returns the 'count' parameter for the editor tool.
****************************************************************************/
int editor_tool_get_count(enum editor_tool_type ett)
{
  if (!editor || !(0 <= ett && ett < NUM_EDITOR_TOOL_TYPES)) {
    return 1;
  }
  return editor->tools[ett].count;
}

/************************************************************************//**
  Sets the 'count' parameter of the tool to the given value.
****************************************************************************/
void editor_tool_set_count(enum editor_tool_type ett, int count)
{
  if (!editor || !(0 <= ett && ett < NUM_EDITOR_TOOL_TYPES)) {
    return;
  }
  editor->tools[ett].count = MAX(1, count);
}

/************************************************************************//**
  Returns a sprite containing an icon for the given tool type. Returns
  NULL if no such sprite exists.
****************************************************************************/
struct sprite *editor_tool_get_sprite(enum editor_tool_type ett)
{
  const struct editor_sprites *sprites;

  if (!tileset || !(0 <= ett && ett < NUM_EDITOR_TOOL_TYPES)) {
    return NULL;
  }

  sprites = get_editor_sprites(tileset);
  if (!sprites) {
    return NULL;
  }

  switch (ett) {
  case ETT_COPYPASTE:
    return sprites->copypaste;
    break;
  case ETT_TERRAIN:
    return sprites->terrain;
    break;
  case ETT_TERRAIN_RESOURCE:
    return sprites->terrain_resource;
    break;
  case ETT_TERRAIN_SPECIAL:
    return sprites->terrain_special;
    break;
  case ETT_ROAD:
    return sprites->road;
  case ETT_MILITARY_BASE:
    return sprites->military_base;
  case ETT_UNIT:
    return sprites->unit;
    break;
  case ETT_CITY:
    return sprites->city;
    break;
  case ETT_VISION:
    return sprites->vision;
    break;
  case ETT_STARTPOS:
    return sprites->startpos;
    break;
  default:
    break;
  }

  return NULL;
}

/************************************************************************//**
  Returns a translated "tooltip" description for the given tool type.
****************************************************************************/
const char *editor_tool_get_tooltip(enum editor_tool_type ett)
{
  if (!editor || !(0 <= ett && ett < NUM_EDITOR_TOOL_TYPES)
      || !editor->tools[ett].tooltip) {
    return "";
  }
  return editor->tools[ett].tooltip;
}

/************************************************************************//**
  Returns the current applied player number for the editor tool.

  May return a player number for which player_by_number returns NULL.
****************************************************************************/
int editor_tool_get_applied_player(enum editor_tool_type ett)
{
  if (!editor || !(0 <= ett && ett < NUM_EDITOR_TOOL_TYPES)) {
    return -1;
  }
  return editor->tools[ett].applied_player_no;
}

/************************************************************************//**
  Sets the editor tool's applied player number to the given value.
****************************************************************************/
void editor_tool_set_applied_player(enum editor_tool_type ett,
                                    int player_no)
{
  if (!editor || !(0 <= ett && ett < NUM_EDITOR_TOOL_TYPES)) {
    return;
  }
  editor->tools[ett].applied_player_no = player_no;
}

/************************************************************************//**
  Returns TRUE if the given tool makes use of the editor's applied player
  number.
****************************************************************************/
bool editor_tool_has_applied_player(enum editor_tool_type ett)
{
  if (!editor || !(0 <= ett && ett < NUM_EDITOR_TOOL_TYPES)) {
    return FALSE;
  }
  return editor->tools[ett].flags & ETF_HAS_APPLIED_PLAYER;
}

/************************************************************************//**
  Returns TRUE if erase mode for the given tool erases by sub-value instead
  of any object corresponding to the tool type.
****************************************************************************/
bool editor_tool_has_value_erase(enum editor_tool_type ett)
{
  if (!editor || !(0 <= ett && ett < NUM_EDITOR_TOOL_TYPES)) {
    return FALSE;
  }
  return editor->tools[ett].flags & ETF_HAS_VALUE_ERASE;
}

/************************************************************************//**
  Returns the number of currently selected tiles.
****************************************************************************/
int editor_selection_count(void)
{
  if (!editor) {
    return 0;
  }
  return tile_hash_size(editor->selected_tile_table);
}

/************************************************************************//**
  Creates a virtual unit (like unit_virtual_create) based on the current
  editor state. You should free() the unit when it is no longer needed.
  If creation is not possible, then NULL is returned.

  The virtual unit has no homecity or tile. It is owned by the player
  corresponding to the current 'applied player' parameter and has unit type
  given by the sub-value of the unit tool (ETT_UNIT).
****************************************************************************/
struct unit *editor_unit_virtual_create(void)
{
  struct unit *vunit;
  struct player *pplayer;
  struct unit_type *putype;
  int apno, value;

  value = editor_tool_get_value(ETT_UNIT);
  putype = utype_by_number(value);

  if (!putype) {
    return NULL;
  }

  apno = editor_tool_get_applied_player(ETT_UNIT);
  pplayer = player_by_number(apno);
  if (!pplayer) {
    return NULL;
  }

  vunit = unit_virtual_create(pplayer, NULL, putype, 0);

  return vunit;
}

/************************************************************************//**
  Create a new edit buffer corresponding to all types set in 'type_flags'.
****************************************************************************/
struct edit_buffer *edit_buffer_new(int type_flags)
{
  struct edit_buffer *ebuf;

  if (!(0 <= type_flags && type_flags <= EBT_ALL)) {
    return NULL;
  }

  ebuf = fc_calloc(1, sizeof(*ebuf));
  ebuf->type_flags = type_flags;
  ebuf->vtiles = tile_list_new();

  return ebuf;
}

/************************************************************************//**
  Free all memory allocated for the edit buffer.
****************************************************************************/
void edit_buffer_free(struct edit_buffer *ebuf)
{
  if (!ebuf) {
    return;
  }

  if (ebuf->vtiles) {
    tile_list_iterate(ebuf->vtiles, vtile) {
      tile_virtual_destroy(vtile);
    } tile_list_iterate_end;
    tile_list_destroy(ebuf->vtiles);
    ebuf->vtiles = NULL;
  }
  free(ebuf);
}

/************************************************************************//**
  Remove all copy data stored in the edit buffer.
****************************************************************************/
void edit_buffer_clear(struct edit_buffer *ebuf)
{
  if (!ebuf || !ebuf->vtiles) {
    return;
  }

  tile_list_iterate(ebuf->vtiles, vtile) {
    tile_virtual_destroy(vtile);
  } tile_list_iterate_end;
  tile_list_clear(ebuf->vtiles);

  edit_buffer_set_origin(ebuf, NULL);
}

/************************************************************************//**
  Copy from a square region of half-width 'radius' centered around 'center'
  into the buffer.
****************************************************************************/
void edit_buffer_copy_square(struct edit_buffer *ebuf,
                             const struct tile *center,
                             int radius)
{
  if (!ebuf || !center || radius < 1) {
    return;
  }

  edit_buffer_set_origin(ebuf, center);
  square_iterate(&(wld.map), center, radius - 1, ptile) {
    edit_buffer_copy(ebuf, ptile);
  } square_iterate_end;
}

/************************************************************************//**
  Append a single tile to the copy buffer.
****************************************************************************/
void edit_buffer_copy(struct edit_buffer *ebuf, const struct tile *ptile)
{
  struct tile *vtile;
  struct unit *vunit;
  bool copied = FALSE;

  if (!ebuf || !ptile) {
    return;
  }

  vtile = tile_virtual_new(NULL);
  vtile->index = tile_index(ptile);

  edit_buffer_type_iterate(ebuf, type) {
    switch (type) {
    case EBT_TERRAIN:
      if (tile_terrain(ptile)) {
        tile_set_terrain(vtile, tile_terrain(ptile));
        copied = TRUE;
      }
      break;
    case EBT_RESOURCE:
      if (tile_resource(ptile)) {
        tile_set_resource(vtile, tile_resource(ptile));
        copied = TRUE;
      }
      break;
    case EBT_SPECIAL:
      extra_type_by_cause_iterate(EC_SPECIAL, pextra) {
        if (tile_has_extra(ptile, pextra)) {
          tile_add_extra(vtile, pextra);
          copied = TRUE;
        }
      } extra_type_by_cause_iterate_end;
      break;
    case EBT_BASE:
     extra_type_iterate(pextra) {
        if (tile_has_extra(ptile, pextra)
            && is_extra_caused_by(pextra, EC_BASE)) {
          tile_add_extra(vtile, pextra);
          copied = TRUE;
        }
      } extra_type_iterate_end;
    case EBT_ROAD:
     extra_type_iterate(pextra) {
        if (tile_has_extra(ptile, pextra)
            && is_extra_caused_by(pextra, EC_ROAD)) {
          tile_add_extra(vtile, pextra);
          copied = TRUE;
        }
      } extra_type_iterate_end;
    case EBT_UNIT:
      unit_list_iterate(ptile->units, punit) {
        if (!punit) {
          continue;
        }
        vunit = unit_virtual_create(unit_owner(punit), NULL,
                                    unit_type_get(punit), punit->veteran);
        vunit->homecity = punit->homecity;
        vunit->hp = punit->hp;
        unit_list_append(vtile->units, vunit);
        copied = TRUE;
      } unit_list_iterate_end;
      break;
    case EBT_CITY:
      if (tile_city(ptile)) {
        struct city *pcity, *vcity;
        char name[MAX_LEN_NAME];

        pcity = tile_city(ptile);
        fc_snprintf(name, sizeof(name), "Copy of %s",
                    city_name_get(pcity));
        vcity = create_city_virtual(city_owner(pcity), NULL, name);
        city_size_set(vcity, city_size_get(pcity));
        improvement_iterate(pimprove) {
          if (!is_improvement(pimprove)
              || !city_has_building(pcity, pimprove)) {
            continue;
          }
          city_add_improvement(vcity, pimprove);
        } improvement_iterate_end;
        tile_set_worked(vtile, vcity);
        copied = TRUE;
      }
      break;
    default:
      break;
    }
  } edit_buffer_type_iterate_end;

  if (copied) {
    tile_list_append(ebuf->vtiles, vtile);
  } else {
    tile_virtual_destroy(vtile);
  }
}

/************************************************************************//**
  Helper function to fill in an edit packet with the tile's current values.
****************************************************************************/
static void fill_tile_edit_packet(struct packet_edit_tile *packet,
                                  const struct tile *ptile)
{
  const struct extra_type *presource;
  const struct terrain *pterrain;

  if (!packet || !ptile) {
    return;
  }
  packet->tile = tile_index(ptile);
  packet->extras = *tile_extras(ptile);

  presource = tile_resource(ptile);
  packet->resource = presource
                     ? extra_number(presource)
                     : extra_count();

  pterrain = tile_terrain(ptile);
  packet->terrain = pterrain
                    ? terrain_number(pterrain)
                    : terrain_count();

  if (ptile->label == NULL) {
    packet->label[0] = '\0';
  } else {
    sz_strlcpy(packet->label, ptile->label);
  }
}

/************************************************************************//**
  Helper function for edit_buffer_paste(). Do a single paste of the stuff set
  in the buffer on the virtual tile to the destination tile 'ptile_dest'.
****************************************************************************/
static void paste_tile(struct edit_buffer *ebuf,
                       const struct tile *vtile,
                       const struct tile *ptile_dest)
{
  struct connection *my_conn = &client.conn;
  struct packet_edit_tile tile_packet;
  struct city *vcity;
  int value, owner, tile;
  bool send_edit_tile = FALSE;

  if (!ebuf || !vtile || !ptile_dest) {
    return;
  }

  tile = tile_index(ptile_dest);

  fill_tile_edit_packet(&tile_packet, ptile_dest);

  edit_buffer_type_iterate(ebuf, type) {
    switch (type) {
    case EBT_TERRAIN:
      if (!tile_terrain(vtile)) {
        continue;
      }
      value = terrain_number(tile_terrain(vtile));
      dsend_packet_edit_tile_terrain(my_conn, tile, value, 1);
      break;
    case EBT_RESOURCE:
      extra_type_by_cause_iterate(EC_RESOURCE, pextra) {
        if (tile_has_extra(vtile, pextra)) {
          BV_SET(tile_packet.extras, extra_index(pextra));
          send_edit_tile = TRUE;
        }
      } extra_type_by_cause_iterate_end;
      break;
    case EBT_SPECIAL:
      extra_type_by_cause_iterate(EC_SPECIAL, pextra) {
        if (tile_has_extra(vtile, pextra)) {
          BV_SET(tile_packet.extras, extra_index(pextra));
          send_edit_tile = TRUE;
        }
      } extra_type_by_cause_iterate_end;
      break;
    case EBT_BASE:
      extra_type_iterate(pextra) {
        if (tile_has_extra(vtile, pextra)
            && is_extra_caused_by(pextra, EC_BASE)) {
          BV_SET(tile_packet.extras, extra_index(pextra));
          send_edit_tile = TRUE;
        }
      } extra_type_iterate_end;
      break;
    case EBT_ROAD:
      extra_type_iterate(pextra) {
        if (tile_has_extra(vtile, pextra)
            && is_extra_caused_by(pextra, EC_ROAD)) {
          BV_SET(tile_packet.extras, extra_index(pextra));
          send_edit_tile = TRUE;
        }
      } extra_type_iterate_end;
      break;
    case EBT_UNIT:
      unit_list_iterate(vtile->units, vunit) {
        value = utype_number(unit_type_get(vunit));
        owner = player_number(unit_owner(vunit));
        dsend_packet_edit_unit_create(my_conn, owner, tile, value, 1, 0);
      } unit_list_iterate_end;
      break;
    case EBT_CITY:
      vcity = tile_city(vtile);
      if (!vcity) {
        continue;
      }
      owner = player_number(city_owner(vcity));
      value = city_size_get(vcity);
      dsend_packet_edit_city_create(my_conn, owner, tile, value, 0);
      break;
    default:
      break;
    }
  } edit_buffer_type_iterate_end;

  if (send_edit_tile) {
    send_packet_edit_tile(my_conn, &tile_packet);
  }
}

/************************************************************************//**
  Paste the entire contents of the edit buffer using 'dest' as the origin.
****************************************************************************/
void edit_buffer_paste(struct edit_buffer *ebuf, const struct tile *dest)
{
  struct connection *my_conn = &client.conn;
  const struct tile *origin, *ptile;
  int dx, dy;

  if (!ebuf || !dest) {
    return;
  }

  /* Calculate vector. */
  origin = edit_buffer_get_origin(ebuf);
  fc_assert_ret(origin != NULL);
  map_distance_vector(&dx, &dy, origin, dest);

  connection_do_buffer(my_conn);
  tile_list_iterate(ebuf->vtiles, vtile) {
    int virt_x, virt_y;

    index_to_map_pos(&virt_x, &virt_y, tile_index(vtile));
    ptile = map_pos_to_tile(&(wld.map), virt_x + dx, virt_y + dy);
    if (!ptile) {
      continue;
    }
    paste_tile(ebuf, vtile, ptile);
  } tile_list_iterate_end;
  connection_do_unbuffer(my_conn);
}

/************************************************************************//**
  Returns the copy buffer for the given tool.
****************************************************************************/
struct edit_buffer *editor_get_copy_buffer(void)
{
  if (!editor) {
    return NULL;
  }
  return editor->copybuf;
}

/************************************************************************//**
  Returns the translated string name for the given mode.
****************************************************************************/
const char *editor_tool_get_mode_name(enum editor_tool_type ett,
                                      enum editor_tool_mode etm)
{
  bool value_erase;

  value_erase = editor_tool_has_value_erase(ett);

  switch (etm) {
  case ETM_PAINT:
    return _("Paint");
    break;
  case ETM_ERASE:
    if (value_erase) {
      return _("Erase Value");
    } else {
      return _("Erase");
    }
    break;
  case ETM_COPY:
    return _("Copy");
    break;
  case ETM_PASTE:
    return _("Paste");
    break;
  default:
    log_error("Unrecognized editor tool mode %d "
              "in editor_tool_get_mode_name().", etm);
    break;
  }

  return "";
}

/************************************************************************//**
  Returns a translated tooltip string assumed to be used for the toggle
  button for this tool mode in the editor gui.
****************************************************************************/
const char *editor_get_mode_tooltip(enum editor_tool_mode etm)
{
  switch (etm) {
  case ETM_ERASE:
    return _("Toggle erase mode.\nShortcut: shift-d");
    break;
  case ETM_COPY:
    return _("Toggle copy mode.\nShortcut: shift-c");
    break;
  case ETM_PASTE:
    return _("Toggle paste mode.\nShortcut: shift-v");
    break;
  default:
    break;
  }

  return NULL;
}

/************************************************************************//**
  Returns the editor sprite corresponding to the tool mode.
****************************************************************************/
struct sprite *editor_get_mode_sprite(enum editor_tool_mode etm)
{
  const struct editor_sprites *sprites;

  sprites = get_editor_sprites(tileset);
  if (!sprites) {
    return NULL;
  }

  switch (etm) {
  case ETM_PAINT:
    return sprites->brush;
    break;
  case ETM_ERASE:
    return sprites->erase;
    break;
  case ETM_COPY:
    return sprites->copy;
    break;
  case ETM_PASTE:
    return sprites->paste;
    break;
  default:
    break;
  }

  return NULL;
}

/************************************************************************//**
  Fill the supplied buffer with a translated string describing the edit
  buffer's current state. Returns the number of bytes used.
****************************************************************************/
int edit_buffer_get_status_string(const struct edit_buffer *ebuf,
                                  char *buf, int buflen)
{
  int ret, total;
  const char *fmt;

  if (!buf || buflen < 1) {
    return 0;
  }

  ret = fc_strlcpy(buf, _("Buffer empty."), buflen);
  if (!ebuf || !ebuf->vtiles) {
    return ret;
  }

  total = tile_list_size(ebuf->vtiles);
  if (total > 0) {
    fmt = PL_("%d tile copied.", "%d tiles copied.", total);
    ret = fc_snprintf(buf, buflen, fmt, total);
  }

  return ret;
}

/************************************************************************//**
  Set the "origin" for subsequent copy operations. This controls the x and
  y offset of newly created virtual tiles in the buffer.
****************************************************************************/
void edit_buffer_set_origin(struct edit_buffer *ebuf,
                            const struct tile *ptile)
{
  if (!ebuf) {
    return;
  }
  ebuf->origin = ptile;
}

/************************************************************************//**
  Return the previously set origin, or NULL if none.
****************************************************************************/
const struct tile *edit_buffer_get_origin(const struct edit_buffer *ebuf)
{
  if (!ebuf) {
    return NULL;
  }
  return ebuf->origin;
}

/************************************************************************//**
  Returns TRUE if the edit buffer was created with the given type flag.
****************************************************************************/
bool edit_buffer_has_type(const struct edit_buffer *ebuf, int type)
{
  if (!ebuf) {
    return FALSE;
  }
  return ebuf->type_flags & type;
}

/************************************************************************//**
  Returns the "center" tile of a group of selected tiles, or NULL.
  The center is calculated as the vector sum divided by the number of tiles,
  i.e. the average of the map distance vectors of the selected tiles.
****************************************************************************/
const struct tile *editor_get_selection_center(void)
{
  int count;
  const struct tile *origin, *center;
  int dx, dy, cx, cy;
  int xsum = 0, ysum = 0;

  if (!editor || !editor->selected_tile_table) {
    return NULL;
  }

  count = tile_hash_size(editor->selected_tile_table);
  if (count < 1) {
    return NULL;
  }

  origin = map_pos_to_tile(&(wld.map), 0, 0);
  tile_hash_iterate(editor->selected_tile_table, ptile) {
    map_distance_vector(&dx, &dy, origin, ptile);
    xsum += dx;
    ysum += dy;
  } tile_hash_iterate_end;

  cx = xsum / count;
  cy = ysum / count;
  center = map_pos_to_tile(&(wld.map), cx, cy);

  return center;
}
