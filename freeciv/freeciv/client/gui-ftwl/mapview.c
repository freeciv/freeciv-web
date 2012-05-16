/********************************************************************** 
 Freeciv - Copyright (C) 2004 - The Freeciv Project
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
#include <config.h>
#endif

#include <stdio.h>

/* common & utility */
#include "fcintl.h"
#include "game.h"
#include "log.h"
#include "map.h"
#include "movement.h"
#include "registry.h"
#include "support.h"
#include "unitlist.h"

/* client */
#include "client_main.h"
#include "climap.h"
#include "climisc.h"
#include "colors.h"
#include "combat.h"
#include "control.h"
#include "mapctrl_common.h"
#include "overview_common.h"
#include "text.h"

#include "widget.h"
#include "gui_main.h"
#include "tilespec.h"
#include "theme_engine.h"
#include "back_end.h"
#include "timing.h"

#include "canvas.h"
#include "gui_text.h"
#include "mapctrl.h"
#include "mapview.h"

#define CANVAS_DEPTH DEPTH_MIN+12
#define CITY_DESCR_DEPTH DEPTH_MIN+14
#define OVERVIEW_DEPTH DEPTH_MIN+15
#define SCREEN_DEPTH DEPTH_MIN+16
#define FOCUS_LIST_DEPTH DEPTH_MIN+18

static struct ct_point overview_pos;
static struct ct_point detaildisplay_pos;
static struct te_screen *screen;
static struct sw_widget *mapview_window = NULL;
static struct sw_widget *overview_window = NULL;
static struct widget_list *city_descr_windows;
static int drag_factor = 2;
/*
 * 0 - pixel
 * 1 - tile
 * 2 - 2 tiles
 *...
 */
static int drag_granularity = 0;

/*
 * At most MAX_DRAG_FPS time the mapview is updated if it is
 * dragged. Useful values are in the range 10..100.
 */
#define MAX_DRAG_FPS 100

static double min_drag_time = 1.0 / (float) MAX_DRAG_FPS;

#define MAX_TILE_LIST2_ITEMS	20
#define MAX_ACTION_DEFS		60
#define MAX_CITY_DESCR_STYLES	MAX_CITY_SIZE

enum tile_list2_item_type {
  TLI_TERRAIN,
  TLI_CITY,
  TLI_UNIT
};

struct tile_list2_item {
  enum tile_list2_item_type type;
  struct osda *unselected;
  struct osda *selected;
  struct sw_widget *button;
  struct city *pcity;
  struct unit *punit;
  char *tooltip;
  char *info_text;
};

static struct tile_list2 {
  int items;
  int selected;
  struct tile_list2_item item[MAX_TILE_LIST2_ITEMS];
} tile_list2;

static struct ct_tooltip *tooltip_template = NULL;

static struct {
  struct ct_tooltip *tooltip_template;
  struct ct_placement *placement;
  int actions;
  struct {
    char *name;
    struct sw_widget *widget;
    int order;
    struct be_key *key;
  } action[MAX_ACTION_DEFS];
} actions_defined;

static struct {
  int actions;
  struct shown_action {
    char *name;
    char *tooltip;
    bool enabled;
    struct sw_widget *widget;
    int order;
  } action[MAX_ACTION_DEFS];
} actions_shown;
static struct timer *drag_timer;

static struct {
  int styles;
  struct city_descr_style {
    int min_size, max_size;
    struct ct_string *name_template;
    struct ct_string *growth_template;
    struct ct_string *prod_template;
    int border_width;
    be_color border_color;
    be_color background_color;
  } style[MAX_CITY_DESCR_STYLES];
} city_descr_styles;

struct ct_string *text_templates[FONT_COUNT];

static bool all_dirty = FALSE;

struct region_list *region_list;

/****************************************************************************
  Return the dimensions of the area (container widget; maximum size) for
  the overview.
****************************************************************************/
void get_overview_area_dimensions(int *width, int *height)
{
 /* Hack */
  *width = 120;
  *height = 80;
}

/**************************************************************************
  Typically an info box is provided to tell the player about the state
  of their civilization.  This function is called when the label is
  changed.
**************************************************************************/
void update_info_label(void)
{
  te_info_update(screen, "year");
  te_info_update(screen, "gold");
  te_info_update(screen, "general");
  te_info_update(screen, "nation_name");
  te_info_update(screen, "population");
}

/**************************************************************************
  Update the information label which gives info on the current unit and
  the square under the current unit, for specified unit.  Note that in
  practice punit is always the focus unit.

  Clears label if punit is NULL.

  Typically also updates the cursor for the map_canvas (this is related
  because the info label may includes  "select destination" prompt etc).
  And it may call update_unit_pix_label() to update the icons for units
  on this square.
**************************************************************************/
void update_unit_info_label(struct unit_list *punitlist)
{
  /* PORTME */
}

/**************************************************************************
  Update the timeout in the client window.  The timeout is the time until
  the turn ends, in seconds.
**************************************************************************/
void update_timeout_label(void)
{
  /* PORTME */
  /* set some widget based on get_timeout_label_text() */
  /* ... */
}

/**************************************************************************
  If do_restore is FALSE it should change the turn button style (to draw
  the user's attention to it).  If do_restore is TRUE this should reset
  the turn done button to the default style.
**************************************************************************/
void update_turn_done_button(bool do_restore)
{
  static bool flip = FALSE;
  
  if (!get_turn_done_button_state()) {
    return;
  }

  if ((do_restore && flip) || !do_restore) {
    /* ... */

    flip = !flip;
  }
  /* PORTME */
}

/**************************************************************************
  Set information for the indicator icons typically shown in the main
  client window.  The parameters tell which sprite to use for the
  indicator.
**************************************************************************/
void set_indicator_icons(struct sprite *bulb, struct sprite *sol,
			 struct sprite *flake, struct sprite *gov)
{
  /* PORTME */
}


/* =========== overview ================== */


/**************************************************************************
  ...
**************************************************************************/
static void overview_mouse_press_callback(struct sw_widget *widget,
					  const struct ct_point *pos,
					  enum be_mouse_button button,
					  int state, void *data)
{
  int xtile, ytile;

  freelog(LOG_DEBUG, "press (%d,%d)", pos->x, pos->y);
  overview_to_map_pos(&xtile,&ytile,pos->x,pos->y);
  freelog(LOG_DEBUG, " --> (%d,%d)", xtile, ytile);
  if (can_client_change_view() && button == 3) {
    center_tile_mapcanvas(map_pos_to_tile(xtile, ytile));
  }
  /* FIXME else if (can_client_issue_orders() && ev->button == 1) {
     do_unit_goto(xtile, ytile);
     }
   */
}

/**************************************************************************
  ...
**************************************************************************/
void overview_size_changed(void)
{
  if (overview_window) {
    sw_widget_destroy(overview_window);
  }
  overview_window =
      sw_window_create(screen->window, overview.width, overview.height, NULL,
		       FALSE, OVERVIEW_DEPTH);
  sw_window_set_mouse_press_notify(overview_window,
				   overview_mouse_press_callback, NULL);
  sw_widget_set_position(overview_window, overview_pos.x, overview_pos.y);
  sw_window_set_draggable(overview_window, FALSE);

  sw_window_set_canvas_background(overview_window, TRUE);
}

/****************************************************************************
  Return a canvas that is the overview window.
****************************************************************************/
struct canvas *get_overview_window(void)
{
  static struct canvas store;

  store.osda = sw_window_get_canvas_background(overview_window);
  store.widget = overview_window;

  return &store;
}

/**************************************************************************
  Refresh (update) the entire map overview.
**************************************************************************/
#if 0
void refresh_overview_canvas(void)
{


  struct ct_rect rect;

  freelog(LOG_DEBUG, "refresh_overview_canvas()");
  whole_map_iterate(x, y) {
    overview_update_tile0(x, y);
  } whole_map_iterate_end;

  rect.x = 0;
  rect.y = 0;
  rect.width = OVERVIEW_SCALE * map.xsize;
  rect.height = OVERVIEW_SCALE * map.ysize;
  sw_window_canvas_background_region_needs_repaint(overview_window, &rect);

  /* PORTME */
}
#endif

/* Disabled for now */
#if 0
/**************************************************************************
  Find the appropriate city description theme style.  This depends on 
  the size of the city, and enables us to have different visual styles
  for city descriptions depending on their sizes.
**************************************************************************/
static struct city_descr_style *find_city_descr_style(struct city *pcity)
{
  int i;

  for (i = 0; i < city_descr_styles.styles; i++) {
    if (pcity->size >= city_descr_styles.style[i].min_size &&
	pcity->size <= city_descr_styles.style[i].max_size) {
      return &city_descr_styles.style[i];
    }
  }
  freelog(LOG_ERROR, "No matching city description style found for %s "
          "which is of size %d", city_name(pcity), pcity->size);
  return &city_descr_styles.style[0];
}

/**************************************************************************
  If necessary, clear the city descriptions out of the buffer.
**************************************************************************/
void prepare_show_city_descriptions(void)
{
  widget_list_iterate(city_descr_windows, widget) {
    widget_list_unlink(city_descr_windows, widget);
    sw_widget_destroy(widget);
  } widget_list_iterate_end;
}

/**************************************************************************
  Draw a description for the given city.  (canvas_x, canvas_y) is the
  canvas position of the city itself.
**************************************************************************/
void show_city_desc(struct canvas *pcanvas, int canvas_x, int canvas_y,
		    struct city *pcity, int *width_, int *height_)
{
  struct sw_widget *window, *label1_a, *label1_b, *label2 = NULL;
  struct ct_string *line1_a, *line1_b, *line2 = NULL;
  enum color_std color;
  char buffer[512], buffer2[32];
  struct ct_rect rect, all_rect;
  int width, height;
  struct city_descr_style *style = find_city_descr_style(pcity);

  sw_widget_get_bounds(screen->window, &all_rect);

#if 0
  /* try to trace that hard-to-find assert that we sometimes get */
  freelog(LOG_TEST, "show_city_desc(%s) pcx=%d->%d (%d) pcy=%d->%d (%d)", city_name(pcity),
          canvas_x, canvas_x+tileset_tile_width(tileset) / 2, all_rect.width,
          canvas_y, canvas_y+tileset_tile_height(tileset), all_rect.height);
#endif

  /* Put text centered below tile */
  canvas_x += tileset_tile_width(tileset) / 2;
  canvas_y += tileset_tile_height(tileset);

  get_city_mapview_name_and_growth(pcity, buffer, sizeof(buffer),
				   buffer2, sizeof(buffer2), &color);

  line1_a =
      ct_string_clone4(style->name_template, buffer,
		      enum_color_to_be_color(COLOR_STD_WHITE));
  line1_b = ct_string_clone4(style->growth_template, buffer2,
			    enum_color_to_be_color(color));

  if (draw_city_productions && (city_owner(pcity) == client.conn.playing)) {
    get_city_mapview_production(pcity, buffer, sizeof(buffer));
    line2 =
	ct_string_clone4(style->prod_template, buffer,
			enum_color_to_be_color(COLOR_STD_WHITE));
  }
  width = 2 + MAX(line1_a->size.width +
		  (line1_b->size.width > 0 ? line1_b->size.width + 10 : 2),
		  line2 ? line2->size.width : 0);
  height = 2 + MAX(line1_a->size.height, line1_b->size.height) +
      (line2 ? line2->size.height + 2 : 0);

  window =
      sw_window_create(screen->window, width, height, NULL,
		       style->border_width > 0,
		       CITY_DESCR_DEPTH);
  sw_widget_set_background_color(window, style->background_color);
  sw_widget_disable_mouse_events(window);
  sw_widget_set_position(window, 0, 0);
  sw_widget_get_bounds(window, &rect);
  canvas_x -= rect.width / 2;

  if (canvas_x < 0) {
    canvas_x = 0;
  }
  if (canvas_y < 0) {
    canvas_y = 0;
  }
  if (canvas_x + rect.width > all_rect.width) {
    canvas_x = all_rect.width - rect.width;
  }
  if (canvas_y + rect.height > all_rect.height) {
    canvas_y = all_rect.height - rect.height;
  }

  sw_widget_set_position(window, canvas_x, canvas_y);

  label1_a = sw_label_create_text(window, line1_a);
  sw_widget_align_parent(label1_a, A_NW);

  label1_b = sw_label_create_text(window, line1_b);
  sw_widget_align_parent(label1_b, A_NE);

  if (line2) {
    label2 = sw_label_create_text(window, line2);
    sw_widget_align_parent(label2, A_SW);
  }      

  widget_list_prepend(city_descr_windows, window);
  /* PORTME */
}
#endif

/**************************************************************************
  Flush the given part of the canvas buffer (if there is one) to the
  screen.
**************************************************************************/
void flush_mapcanvas(int canvas_x, int canvas_y,
		     int pixel_width, int pixel_height)
{
  struct ct_rect rect = { canvas_x, canvas_y, pixel_width, pixel_height };
  struct ct_size size = { pixel_width, pixel_height };
  struct ct_point pos = { canvas_x, canvas_y };

  freelog(LOG_DEBUG, "flush_mapcanvas=%s", ct_rect_to_string(&rect));
  be_copy_osda_to_osda(sw_window_get_canvas_background(mapview_window),
		       mapview.store->osda, &size, &pos, &pos);
  sw_window_canvas_background_region_needs_repaint(mapview_window, &rect);
}

/**************************************************************************
  Mark the rectangular region as 'dirty' so that we know to flush it
  later.
**************************************************************************/
void dirty_rect(int canvas_x, int canvas_y,
		int pixel_width, int pixel_height)
{
  if (!all_dirty) {
    struct ct_rect *rect = fc_malloc(sizeof(*rect));
  
    *rect = (struct ct_rect){ canvas_x, canvas_y, pixel_width, pixel_height };
  
    //freelog(LOG_TEST, "dirty_rect(...)");
    region_list_append(region_list, rect);
  }
}

/**************************************************************************
  Mark the entire screen area as "dirty" so that we can flush it later.
  It is called by client common code.
**************************************************************************/
void dirty_all(void)
{
  all_dirty = TRUE;
}

/**************************************************************************
  Flush all regions that have been previously marked as dirty.  See
  dirty_rect and dirty_all.  This function is generally called after we've
  processed a batch of drawing operations.
**************************************************************************/
void flush_dirty(void)
{
  if (all_dirty) {
    region_list_iterate(region_list, region) {
      free(region);
    } region_list_iterate_end;
    region_list_clear(region_list);
    
    flush_mapcanvas(0, 0, mapview.width, mapview.height);
    all_dirty = FALSE;
  } else {
    region_list_iterate(region_list, region) {
      flush_mapcanvas(region->x, region->y, region->width, region->height);
      free(region);
    } region_list_iterate_end;
    region_list_clear(region_list);
  }
  sw_paint_all();
}

/**************************************************************************
  Update (refresh) the locations of the mapview scrollbars (if it uses
  them).
**************************************************************************/
void update_map_canvas_scrollbars(void)
{
  /* PORTME */
}

/**************************************************************************
  Update (refresh) all of the city descriptions on the mapview.
**************************************************************************/
void update_city_descriptions(void)
{
  update_map_canvas_visible();
}

/**************************************************************************
  Draw a cross-hair overlay on a tile.
**************************************************************************/
void put_cross_overlay_tile(struct tile *ptile)
{
  /* PORTME */
}

/**************************************************************************
  This function is called when the tileset is changed.
**************************************************************************/
void tileset_changed(void)
{
  /* PORTME */
  /* Here you should do any necessary redraws (for instance, the city
   * dialogs usually need to be resized). */
}

/**************************************************************************
  ...
**************************************************************************/
static struct osda *unit_to_osda(struct unit *punit)
{
  struct osda *result = be_create_osda(tileset_full_tile_width(tileset), tileset_full_tile_height(tileset));
  struct canvas *store = fc_malloc(sizeof(*store));

  store->osda = result;
  store->widget = NULL;

  put_unit(punit, store, 0, 0);

  free(store);
  return result;
}

/**************************************************************************
  ...
**************************************************************************/
static struct osda *terrain_to_osda(struct tile *ptile)
{
  struct osda *result = be_create_osda(tileset_full_tile_width(tileset), tileset_full_tile_height(tileset));
  struct canvas *store = fc_malloc(sizeof(*store));

  store->osda = result;
  store->widget = NULL;

  put_terrain(ptile, store, 0, 0);

  free(store);
  return result;
}

/**************************************************************************
  ...
**************************************************************************/
static struct osda *city_to_osda(struct city *pcity)
{
  struct osda *result = be_create_osda(tileset_full_tile_width(tileset), tileset_full_tile_height(tileset));
  struct canvas *store = fc_malloc(sizeof(*store));

  store->osda = result;
  store->widget = NULL;

  put_city(pcity, store, 0, 0);

  free(store);
  return result;
}

/**************************************************************************
  ...
**************************************************************************/
static struct osda *create_selected_osda(struct osda *osda)
{
  struct osda *result = be_create_osda(tileset_full_tile_width(tileset), tileset_full_tile_height(tileset));
  struct ct_rect spec={0,0,tileset_full_tile_width(tileset), tileset_full_tile_height(tileset)};
  struct ct_size size={tileset_full_tile_width(tileset), tileset_full_tile_height(tileset)};

  be_copy_osda_to_osda(result, osda, &size, NULL, NULL);

  be_draw_rectangle(result, &spec, 1, be_get_color(255, 0, 0, 255));
  return result;
}

static void tile_list2_select(int new_index);

/**************************************************************************
  ...
**************************************************************************/
static void tile_list2_select_callback(struct sw_widget *widget, void *data)
{
    tile_list2_select((int)data);
}

/**************************************************************************
  Show the detaildisplay information - the information displayed when
  you click on a tile, city or unit.

  FIXME: The number '35' below should be calculated from tile width.
**************************************************************************/
static void tile_list2_select(int new_index)
{
  int i;
  tile_list2.selected = new_index;

  for (i = 0; i < tile_list2.items; i++) {
    struct tile_list2_item *item = &tile_list2.item[i];
    struct osda *osda = i == new_index ? item->selected : item->unselected;
    struct osda *background_faces[4];
    int j;

    for (j = 0; j < 4; j++) {
      background_faces[j] = osda;
    }

    if (item->button) {
      sw_widget_destroy(item->button);
      item->button = NULL;
    }

    item->button =
	sw_button_create(screen->window, NULL, NULL, background_faces);
    sw_widget_set_position(item->button, detaildisplay_pos.x + i * 35,
                           detaildisplay_pos.y);
    sw_button_set_callback(item->button,
			   tile_list2_select_callback, (void *)i);
    sw_widget_set_tooltip(item->button,
			  ct_tooltip_clone1(tooltip_template,
					    item->tooltip));
  }
  te_info_update(screen, "focus_item");
  mapview_update_actions();
}

/**************************************************************************
  ...
**************************************************************************/
static void update_focus_tile_list2(void)
{
  int i;
  struct tile *ptile;

  set_unit_focus(NULL);

  ptile = get_focus_tile();
  unit_list_iterate(ptile->units, aunit) {
    if (unit_owner(aunit) == client.conn.playing) {
      set_unit_focus(aunit);
      break;
    }
  } unit_list_iterate_end;

  for (i = 0; i < tile_list2.items; i++) {
    sw_widget_destroy(tile_list2.item[i].button);
    be_free_osda(tile_list2.item[i].selected);
    be_free_osda(tile_list2.item[i].unselected);
  }

  tile_list2.items = 0;
  tile_list2.selected = -1;

  {
    struct tile_list2_item *item = &tile_list2.item[tile_list2.items];

    tile_list2.items++;

    item->type = TLI_TERRAIN;
    item->unselected = terrain_to_osda(ptile);
    item->selected = create_selected_osda(item->unselected);
    item->button = NULL;
    item->tooltip=mystrdup(mapview_get_terrain_tooltip_text(ptile));
    item->info_text=mystrdup(popup_info_text(ptile));
  }

  if (tile_city(ptile)) {
    struct city *pcity=tile_city(ptile);
    struct tile_list2_item *item = &tile_list2.item[tile_list2.items];

    tile_list2.items++;

    item->type = TLI_CITY;
    item->unselected = city_to_osda(pcity);
    item->selected = create_selected_osda(item->unselected);
    item->button = NULL;
    item->pcity = pcity;
    item->tooltip = mystrdup(mapview_get_city_tooltip_text(pcity));
    item->info_text = mystrdup(mapview_get_city_info_text(pcity));
  }

  unit_list_iterate(ptile->units, punit) {
    struct tile_list2_item *item = &tile_list2.item[tile_list2.items];

    tile_list2.items++;

    item->type = TLI_UNIT;
    item->unselected = unit_to_osda(punit);
    item->selected = create_selected_osda(item->unselected);
    item->button = NULL;
    item->punit = punit;

    item->tooltip = mystrdup(mapview_get_unit_tooltip_text(punit));
    item->info_text = mystrdup(mapview_get_unit_info_text(punit));
  } unit_list_iterate_end;

  if (tile_list2.items > 1) {
    /* Take the city or unit */
    tile_list2_select(1);
  } else {
    tile_list2_select(0);
  }
}

/**************************************************************************
  ...
**************************************************************************/
static void canvas_mouse_press_callback(struct sw_widget *widget,
					const struct ct_point *pos,
					enum be_mouse_button button,
					int state, void *data)
{
  struct tile *ptile;

  if (!can_client_change_view()) {
    return;
  }

#if 0
  if (can_client_issue_orders()
      && button == 1 && (ev->state & GDK_SHIFT_MASK)) {
    adjust_workers(w, ev);
    return TRUE;
  }
#endif

  ptile = canvas_pos_to_tile(pos->x, pos->y);
  if (!ptile) {
    return;
  }

  if (button == BE_MB_LEFT) {
    set_focus_tile(ptile);
    update_focus_tile_list2();
  } else if (button == BE_MB_MIDDLE) {
    //popit(ev, xtile, ytile);
  } else if (button == BE_MB_RIGHT) {
      /*
    struct ct_rect rect;

    sw_widget_get_bounds(mapview_window, &rect);

    center_tile_mapcanvas(xtile, ytile);
      */
  }
}

/**************************************************************************
  ...
**************************************************************************/
static void button_callback(const char *id)
{
  if (strcmp(id, "connect") == 0) {
  } else {
    assert(0);
  }
}

/**************************************************************************
  ...
**************************************************************************/
static const char *edit_get_initial_value(const char *id)
{
  if (strcmp(id, "port") == 0) {
      return NULL;
  } else {
    assert(0);
    return NULL;
  }
}

/**************************************************************************
  ...
**************************************************************************/
static int edit_get_width(const char *id)
{
  if (strcmp(id, "port") == 0) {
      return 0;
  } else {
    assert(0);
    return 0;
  }
}

static int starting_map_position_x;
static int starting_map_position_y;

/**************************************************************************
  ...
**************************************************************************/
static void my_drag_start(struct sw_widget *widget,
			  const struct ct_point *mouse,
			  enum be_mouse_button button)
{
  get_mapview_scroll_pos(&starting_map_position_x,&starting_map_position_y);
  freelog(LOG_DEBUG, "drag start (%d,%d)", starting_map_position_x,
	  starting_map_position_y);
  clear_timer_start(drag_timer);
}

/**************************************************************************
  ...
**************************************************************************/
static void my_drag_move(struct sw_widget *widget,
			 const struct ct_point *start_position,
			 const struct ct_point *current_position,
			 enum be_mouse_button button)
{
  int dx = drag_factor * (current_position->x - start_position->x);
  int dy = drag_factor * (current_position->y - start_position->y);

  int factorx =
      drag_granularity == 0 ? 1 : drag_granularity * tileset_tile_width(tileset);
  int factory =
      drag_granularity == 0 ? 1 : drag_granularity * tileset_tile_height(tileset);

  dx /= factorx;
  dy /= factory;

  if (read_timer_seconds(drag_timer) > min_drag_time && (dx != 0 || dy != 0)) {
    int new_x = starting_map_position_x - dx * factorx;
    int new_y = starting_map_position_y - dy * factory;

    freelog(LOG_DEBUG, "drag map canvas (%d,%d) -> (%d,%d)", dx,
	    dy, new_x, new_y);
    set_mapview_scroll_pos(new_x, new_y);
    flush_dirty();
    redraw_selection_rectangle();
    clear_timer_start(drag_timer);
  }
}

/**************************************************************************
  ...
**************************************************************************/
static void action_button_callback(struct sw_widget *widget, void *data)
{
  char *action = (char *) data;

  freelog(LOG_VERBOSE, "action_button_callback(): action '%s' requested", action);  
  
  if (strcmp(action, "unit_fortifying") == 0) {
    key_unit_fortify();
  } else if (strcmp(action, "unit_add_to_city") == 0) {
    key_unit_build_city();
  } else if (strcmp(action, "unit_build_city") == 0) {
    key_unit_build_city();
  } else if (strcmp(action, "unit_auto_explore") == 0) {
    key_unit_auto_explore();
  } else if (strcmp(action, "unit_irrigate") == 0) {
    key_unit_irrigate();
  } else if (strcmp(action, "unit_sentry") == 0) {
    key_unit_sentry();
  } else if (strcmp(action, "unit_return_nearest") == 0) {
/*    key_? */
  } else if (strcmp(action, "unit_disband") == 0) {
    key_unit_disband();
  } else if (strcmp(action, "unit_mine") == 0) {
    key_unit_mine();
  } else if (strcmp(action, "unit_road") == 0) {
    key_unit_road();
  } else if (strcmp(action, "unit_auto_settler") == 0) {
    key_unit_auto_settle();
  } else if (strcmp(action, "unit_connect") == 0) {
/*    key_unit_connect(enum unit_activity activity);*/
  } else if (strcmp(action, "unit_auto_attack") == 0) {
/*    key_? */    
  } else if (strcmp(action, "unit_homecity") == 0) {
    key_unit_homecity();
  } else if (strcmp(action, "unit_fortress") == 0) {
    key_unit_fortress();
  } else if (strcmp(action, "unit_pillage") == 0) {
    key_unit_pillage();
  } else if (strcmp(action, "unit_airbase") == 0) {
    key_unit_airbase();
  } else if (strcmp(action, "unit_transform") == 0) {
    key_unit_transform();
  } else if (strcmp(action, "unit_unload") == 0) {
    key_unit_unload_all();
  } else if (strcmp(action, "unit_goto") == 0) {
    key_unit_goto();
  } else if (strcmp(action, "unit_goto_city") == 0) {
/*    key_? */
  } else if (strcmp(action, "unit_airlift") == 0) {
/*    key_? */
  } else if (strcmp(action, "unit_patrol") == 0) {
    key_unit_patrol();
  } else if (strcmp(action, "unit_wait") == 0) {
    key_unit_wait();
  } else if (strcmp(action, "unit_railroad") == 0) {
/*    key_? */
  } else if (strcmp(action, "unit_done") == 0) {
    key_unit_done();
  } else {
    freelog(LOG_VERBOSE,
      "action_button_callback(): action request '%s' not handled", action);    
  }
}

/**************************************************************************
  glibc's scanf is localised. As such it will parse "1.345" as 1345 
  with the german locale and not as 1.345.  The theme files should be
  locale independent, and therefore since function is needed.
**************************************************************************/
static bool is_float(char *str, double *dest)
{
  char *p;
  char *copy = mystrdup(str);
  struct lconv *locale_data;
  double res;
  int i;

  locale_data = localeconv();
  assert(strlen(locale_data->decimal_point) == 1);

  for (i = 0; i < strlen(copy); i++) {
    if (copy[i] == '.') {
      copy[i] = locale_data->decimal_point[0];
    }
  }
  res = strtod(copy, &p);

  if (p != copy + strlen(copy)) {
    free(copy);
    return FALSE;
  }
  free(copy);
  *dest = res;
  return TRUE;
}

/**************************************************************************
  Handle keypresses.  This is where we carry out mapview scrolling with
  arrow keys etc..
**************************************************************************/
static void action_callback(const char *action)
{
  if (strcmp(action, "center_unit") == 0) {
    request_center_focus_unit();
  } else if (strcmp(action, "center_capital") == 0) {
    key_center_capital();
  } else if (strncmp(action, "move_unit(", strlen("move_unit(")) == 0) {
    enum direction8 dir = -1;
    char dir_str[20];
    int i;
    bool found = FALSE;

    if (sscanf(action, "move_unit(%20[^)])", dir_str) != 1) {
      freelog(LOG_ERROR, "move_unit command misformed");
      return;
    }

    for (i = 0; i < 8; i++) {
      if (mystrcasecmp(dir_str, dir_get_name(i)) == 0) {
	dir = i;
	found = TRUE;
	break;
      }
    }

    if (!found) {
      freelog(LOG_ERROR, "move_unit direction unknown");
      return;
    }
    key_unit_move(dir);
  } else if (strncmp(action, "scroll(", strlen("scroll(")) == 0) {
    char dir[20], fact[20], gran[20];
    double factor;
    bool right;
    int base, total;

    if (sscanf(action, "scroll(%20[^,],%20[^,],%20[^)])", dir, fact,
	       gran) != 3) {
      freelog(LOG_ERROR, "scroll command misformed");
      return;
    }
    if (!is_float(fact,&factor)) {
      freelog(LOG_ERROR, "scroll: factor '%s' isn't a float",fact);
      return;
    }

    if (strcmp(dir, "left") == 0) {
      right = TRUE;
      factor = -factor;
    } else if (strcmp(dir, "right") == 0) {
      right = TRUE;
    } else if (strcmp(dir, "up") == 0) {
      right = FALSE;
      factor = -factor;
    } else if (strcmp(dir, "down") == 0) {
      right = FALSE;
    } else {
      freelog(LOG_ERROR, "scroll: direction unknown");
      return;
    }

    if (strcmp(gran, "pixel") == 0) {
      base = 1;
    } else if (strcmp(gran, "tile") == 0) {
      int xstep, ystep;

      get_mapview_scroll_step(&xstep, &ystep);
      if (right) {
	base = xstep;
      } else {
	base = ystep;
      }
    } else if (strcmp(gran, "screen") == 0) {
      struct ct_size size;
      be_screen_get_size(&size);
      if (right) {
	base = size.width;
      } else {
	base = size.height;
      }
    } else {
      freelog(LOG_ERROR, "scroll: granularity unknown");
      return;
    }

    total = (int) (factor * (float) base);
    {
      int x, y;
      get_mapview_scroll_pos(&x, &y);

      if (right) {
	x += total;
      } else {
	y += total;
      }
      set_mapview_scroll_pos(x, y);
      flush_dirty();
      redraw_selection_rectangle();
    }
  } else {
    freelog(LOG_VERBOSE, "action_callback() action '%s' requested", action);
  }
}

/**************************************************************************
  Read theme files and store their results for quick lookup.
**************************************************************************/
static void read_properties(void)
{
  struct section_file *file = te_open_themed_file("mapview.prop");

  /* Overview position */
  overview_pos = te_read_point(file, "overview", "position");

  /* Unit, city and tile view position */
  detaildisplay_pos = te_read_point(file, "detaildisplay", "position");

  tooltip_template = te_read_tooltip(file, "focus", FALSE);

  /* Actions */
  {
    char **sec;
    int num;
    int i;

    actions_defined.tooltip_template =
	te_read_tooltip(file, "actions", FALSE);
    actions_defined.placement = te_read_placement(file, "actions");


    sec = secfile_get_secnames_prefix(file, "action_", &num);
    assert(num < MAX_ACTION_DEFS);
    actions_defined.actions = 0;

    for (i = 0; i < num; i++) {
      struct ct_string *string = NULL;
      struct sw_widget *widget;
      char *id = mystrdup(strchr(sec[i], '_') + 1);
      char *background;
      int order = secfile_lookup_int(file, "%s.order", sec[i]);
      struct be_key *key;

      if (secfile_lookup_str_default(file, NULL, "%s.text", sec[i])) {
	string = te_read_string(file, sec[i], "text", FALSE, TRUE);
      }
      background = secfile_lookup_str(file, "%s.background", sec[i]);
      widget =
	  sw_button_create_text_and_background(NULL,
					       string,
					       te_load_gfx(background));
      sw_button_set_callback(widget, action_button_callback, id);
      key = te_read_key(file, sec[i], "shortcut");
      if (key) {
	sw_button_set_shortcut(widget, key);
      }

      actions_defined.action[actions_defined.actions].key = key;
      actions_defined.action[actions_defined.actions].name = id;
      actions_defined.action[actions_defined.actions].widget = widget;
      actions_defined.action[actions_defined.actions].order = order;
      actions_defined.actions++;
    }
    free(sec);
  }

  /* City description styles */
  {
    char **sec;
    int num;
    int i;

    sec = secfile_get_secnames_prefix(file, "city_descr_style_", &num);
    assert(num < MAX_CITY_DESCR_STYLES);
    city_descr_styles.styles = 0;

    for (i = 0; i < num; i++) {
      int min_size = secfile_lookup_int(file, "%s.size-min", sec[i]);
      int max_size = secfile_lookup_int(file, "%s.size-max", sec[i]);
      int border_width = secfile_lookup_int(file, "%s.border-width", sec[i]);
      struct ct_string *name =
	  te_read_string(file, sec[i], "name", FALSE, FALSE);
      struct ct_string *growth =
	  te_read_string(file, sec[i], "growth", FALSE, FALSE);
      struct ct_string *prod =
	  te_read_string(file, sec[i], "production", FALSE, FALSE);
      be_color border_color = te_read_color(file, sec[i],"border-color","");

      assert(min_size > 0 && max_size >= min_size
	     && max_size <= MAX_CITY_SIZE);
      assert(border_width >= 0 && border_width <= 1);

      /* HACK */
      if (i == 0) {
	text_templates[FONT_CITY_NAME] = name;
	text_templates[FONT_CITY_PROD] = prod;
      }

      city_descr_styles.style[city_descr_styles.styles].min_size = min_size;
      city_descr_styles.style[city_descr_styles.styles].max_size = max_size;
      city_descr_styles.style[city_descr_styles.styles].border_width =
	  border_width;
      city_descr_styles.style[city_descr_styles.styles].border_color =
	  border_color;
      city_descr_styles.style[city_descr_styles.styles].name_template = name;
      city_descr_styles.style[city_descr_styles.styles].growth_template =
	  growth;
      city_descr_styles.style[city_descr_styles.styles].prod_template = prod;
      city_descr_styles.styles++;
    }
    free(sec);
  }
}

/**************************************************************************
  ...
**************************************************************************/
static const char *info_get_value(const char *id)
{
  static char buffer[100];

  if (C_S_RUNNING > client_state()) {
    return "pregame";
  }

  if (strcmp(id, "year") == 0) {
    return textyear(game.info.year);
  } else if (strcmp(id, "gold") == 0) {
    my_snprintf(buffer, sizeof(buffer),
		"%d", client.conn.playing->economic.gold);
    return buffer;
  } else if (strcmp(id, "nation_name") == 0) {
      return nation_plural_for_player(client.conn.playing);
  } else if (strcmp(id, "population") == 0) {
      return population_to_text(civ_population(client.conn.playing));
  } else if (strcmp(id, "general") == 0) {
      my_snprintf(buffer, sizeof(buffer),
		  _("Population: %s\n"
		"Year: %s\n"
		"Gold %d\n"
		"Tax: %d Lux: %d Sci: %d"),
	      population_to_text(civ_population(client.conn.playing)),
	      textyear(game.info.year),
	      client.conn.playing->economic.gold,
	      client.conn.playing->economic.tax,
	      client.conn.playing->economic.luxury,
	      client.conn.playing->economic.science);
      return buffer;
  } else if (strcmp(id, "focus_item") == 0) {
      return tile_list2.item[tile_list2.selected].info_text;
#if 0      
      my_snprintf(buffer, sizeof(buffer),
		  _("Population: %s "
		"Year: %s "
		"Gold %d "
		"Tax: %d Lux: %d Sci: %d"),
	      population_to_text(civ_population(client.conn.playing)),
	      textyear(game.info.year),
	      client.conn.playing->economic.gold,
	      client.conn.playing->economic.tax,
	      client.conn.playing->economic.luxury,
	      client.conn.playing->economic.science);
      return buffer;
#endif
  } else {
    assert(0);
    return NULL;
  }
}

/**************************************************************************
  Initialize map canvas.
**************************************************************************/
void popup_mapcanvas(void)
{
  struct te_screen_env env;
  struct ct_rect rect;
  struct ct_size screen_size;

  region_list = region_list_new();
  
  be_screen_get_size(&screen_size);

  env.info_get_value = info_get_value;
  env.edit_get_initial_value = edit_get_initial_value;
  env.edit_get_width = edit_get_width;
  env.button_callback = button_callback;
  env.action_callback = action_callback;
  city_descr_windows = widget_list_new();

  screen = te_get_screen(root_window, "mapview", &env, SCREEN_DEPTH);

  mapview_window =
      sw_window_create(screen->window, screen_size.width, screen_size.height,
		       NULL, FALSE, CANVAS_DEPTH);
  sw_widget_set_position(mapview_window, 0,0);

  sw_window_set_canvas_background(mapview_window, TRUE);
  sw_window_set_mouse_press_notify(mapview_window,
				   canvas_mouse_press_callback, NULL);
  sw_window_set_draggable(mapview_window,FALSE);
  sw_window_set_user_drag(mapview_window, my_drag_start, my_drag_move,
			  NULL);

  sw_widget_get_bounds(mapview_window, &rect);
  be_draw_region(sw_window_get_canvas_background(mapview_window), &rect,
		 enum_color_to_be_color(COLOR_STD_BACKGROUND));

  init_mapcanvas_and_overview();

  map_canvas_resized(screen_size.width, screen_size.height);

  read_properties();

  tile_list2.items = 0;
  drag_timer = new_timer(TIMER_USER, TIMER_ACTIVE);
}

/**************************************************************************
  ...
**************************************************************************/
void popdown_mapcanvas(void)
{
  region_list_iterate(region_list, region) {
    free(region);
  } region_list_iterate_end;
  region_list_free(region_list);
  
  te_destroy_screen(screen);
}

/**************************************************************************
  ...
**************************************************************************/
void draw_selection_rectangle(int canvas_x, int canvas_y, int w, int h)
{
  /* ?? */
}

/**************************************************************************
  ...
**************************************************************************/
static void unshow_actions(void)
{
  struct ct_rect rect;
  int i;

  for (i = 0; i < actions_shown.actions; i++) {
    sw_widget_get_bounds(actions_shown.action[i].widget, &rect);    
    be_draw_region(sw_widget_get_osda(screen->window), &rect,
                                       be_get_color(0, 0, 0, MIN_OPACITY));
    sw_window_remove(actions_shown.action[i].widget);
    actions_shown.action[i].widget = NULL;
    free(actions_shown.action[i].name);
    actions_shown.action[i].name = NULL;
    free(actions_shown.action[i].tooltip);
    actions_shown.action[i].tooltip = NULL;
  }
}

/**************************************************************************
  ...
**************************************************************************/
static const char *format_shortcut(const char *action)
{

  int j;

  for (j = 0; j < actions_defined.actions; j++) {
    if (strcmp(action, actions_defined.action[j].name) == 0) {
      if (actions_defined.action[j].key) {
	return ct_key_format(actions_defined.action[j].key);
      }
    }
  }

  return NULL;
}

#define ADD(x) \
  actions_shown.action[actions_shown.actions].name = mystrdup(x);\
  actions_shown.action[actions_shown.actions].enabled = TRUE;\
  actions_shown.actions++;

#define ADD_DIS(x) \
  actions_shown.action[actions_shown.actions].name = mystrdup(x);\
  actions_shown.action[actions_shown.actions].enabled = FALSE;\
  actions_shown.actions++;

#define X(x,y) 				\
  if (can_unit_do_activity(punit, x)) {	\
    ADD(y);				\
  }

/**************************************************************************
  ...
**************************************************************************/
static void fill_actions(void)
{
  struct tile_list2_item *item = &tile_list2.item[tile_list2.selected];  

  actions_shown.actions = 0;

  if (item->type == TLI_UNIT) {
    struct unit *punit = item->punit;
    int i;

    ADD("unit_goto");
    ADD("unit_goto_city");
    ADD("unit_airlift");
    ADD("unit_patrol");
    ADD("unit_wait");
    ADD("unit_done");

    X(ACTIVITY_AIRBASE, "unit_airbase");
    X(ACTIVITY_EXPLORE, "unit_auto_explore");
    X(ACTIVITY_FALLOUT, "unit_fallout");
    X(ACTIVITY_FORTIFYING, "unit_fortifying");
    X(ACTIVITY_FORTRESS, "unit_fortress");
    //	ACTIVITY_GOTO, 
    X(ACTIVITY_IRRIGATE,  "unit_irrigate");
    X(ACTIVITY_MINE,  "unit_mine");
    X(ACTIVITY_PILLAGE,  "unit_pillage");
    X(ACTIVITY_POLLUTION,  "unit_pollution");
    X(ACTIVITY_RAILROAD,  "unit_railroad");
    X(ACTIVITY_ROAD,  "unit_road");
    X(ACTIVITY_SENTRY,  "unit_sentry");
    X(ACTIVITY_TRANSFORM,  "unit_transform");

    if (can_unit_do_autosettlers(punit)) {
      if (unit_has_type_flag(punit, F_SETTLERS)) {
	ADD("unit_auto_settler");
      } else {
	ADD("unit_auto_attack");
      }
    }

    if (unit_can_help_build_wonder_here(punit)) {
      ADD("unit_help_wonder");
    }
    if (unit_can_est_traderoute_here(punit)) {
      ADD("unit_traderoute");
    }

    if (can_unit_add_or_build_city(punit)) {
      if (tile_city(punit->tile)) {
	ADD("unit_add_to_city");
      } else {
	ADD("unit_build_city");
      }
    }

    if (can_unit_paradrop(punit)) {
      ADD("unit_paradrop");
    }

    if (can_unit_change_homecity(punit)) {
      ADD("unit_homecity");
    }

    if (get_transporter_occupancy(punit) > 0) {
      ADD("unit_unload");
    }
    if (is_unit_activity_on_tile(ACTIVITY_SENTRY, punit->tile)) {
      ADD("unit_wake_others");
    }
    if (can_unit_do_connect(punit, ACTIVITY_IDLE)) {
      ADD("unit_connect");
    }
    if (!utype_fuel(unit_type(punit))) {
      ADD("unit_return_nearest");
    }
    if (!unit_has_type_flag(punit, F_UNDISBANDABLE)) {
      ADD("unit_disband");
    }
    if (unit_has_type_flag(punit, F_NUCLEAR)) {
      ADD("unit_explode");
    }
    if ((is_diplomat_unit(punit)
	 && diplomat_can_do_action(punit, DIPLOMAT_ANY_ACTION,
				   punit->tile))) {
      ADD("unit_spy");
    }
    
    /* Add tooltips */
    for (i = 0; i < actions_shown.actions; i++) {
      actions_shown.action[i].tooltip =
	  mystrdup(mapview_get_unit_action_tooltip
		   (punit, actions_shown.action[i].name,
		    format_shortcut(actions_shown.action[i].name)));
    }
  } else if (item->type == TLI_CITY) {
    struct city *pcity = item->pcity;
    int i;

    if (city_production_buy_gold_cost(pcity) <= client.conn.playing->economic.gold) {
      ADD("city_buy");
    } else {
      ADD_DIS("city_buy");
    }
    ADD("city_popup");
    ADD("city_popup_queue");

    /* Add tooltips */
    for (i = 0; i < actions_shown.actions; i++) {
      actions_shown.action[i].tooltip =
	  mystrdup(mapview_get_city_action_tooltip
		   (pcity, actions_shown.action[i].name,
		    format_shortcut(actions_shown.action[i].name)));
    }
  }

}
#undef ADD

/**************************************************************************
  ...
**************************************************************************/
static int mysort(const void *p1, const void *p2)
{
  const struct shown_action *a1 = (const struct shown_action *) p1;
  const struct shown_action *a2 = (const struct shown_action *) p2;

  return a1->order - a2->order;
}

/**************************************************************************
  ...
**************************************************************************/
static void show_actions(void)
{
  int i;

  for (i = 0; i < actions_shown.actions; i++) {
    struct sw_widget *widget = NULL;
    int j, order = 0;

    for (j = 0; j < actions_defined.actions; j++) {
      if (strcmp(actions_shown.action[i].name, 
		 actions_defined.action[j].name) == 0) {
	widget = actions_defined.action[j].widget;
	order=actions_defined.action[j].order;
	break;
      }
    }
    if (!widget) {
      die("Can't find action button for '%s'. Define it in 'mapview.prop'.", 
	  actions_shown.action[i].name);
    }
    
    actions_shown.action[i].widget = widget;
    actions_shown.action[i].order = order;

    sw_widget_set_tooltip(widget,
			  ct_tooltip_clone1(actions_defined.tooltip_template,
					    actions_shown.action[i].tooltip));
    sw_widget_set_enabled(widget,actions_shown.action[i].enabled);
    sw_window_add(screen->window,widget);
  }

  qsort(actions_shown.action, actions_shown.actions,
	sizeof(actions_shown.action[0]), mysort);

  for (i = 0; i < actions_shown.actions; i++) {    
    struct ct_point point;

    ct_get_placement(actions_defined.placement,&point,i,actions_shown.actions);
    sw_widget_set_position(actions_shown.action[i].widget, point.x,point.y);
  }    
}

/**************************************************************************
  ...
**************************************************************************/
void mapview_update_actions(void)
{
    unshow_actions();
    fill_actions();
    show_actions();
}

/****************************************************************************
  Do any necessary synchronization to make sure the screen is up-to-date.
  The canvas should have already been flushed to screen via flush_dirty -
  all this function does is make sure the hardware has caught up.
****************************************************************************/
void gui_flush(void)
{
  /* TESTME */
  sw_paint_all();
}

/**************************************************************************
...
**************************************************************************/
void update_map_canvas_scrollbars_size(void)
{
  /* Nothing */
}

struct tile *focus_tile;

/****************************************************************************
  Set the position of the focus tile, and update the mapview.
****************************************************************************/
void set_focus_tile(struct tile *ptile)
{
  struct tile *old = focus_tile;

  assert(ptile != NULL);
  focus_tile = ptile;

  if (old) {
      refresh_tile_mapcanvas(old, FALSE, TRUE);
  }
  refresh_tile_mapcanvas(focus_tile, FALSE, TRUE);
}

/****************************************************************************
  Clear the focus tile, and update the mapview.
****************************************************************************/
void clear_focus_tile(void)
{
  struct tile *old = focus_tile;

  focus_tile = NULL;

  if (map_exists() && old) {
      refresh_tile_mapcanvas(old, FALSE, TRUE);
  }
}

/****************************************************************************
  Find the focus tile.  Returns FALSE if there is no focus tile.
****************************************************************************/
struct tile *get_focus_tile(void)
{
  return focus_tile;
}

/****************************************************************************
  Called by the tileset code to set the font size that should be used to
  draw the city names and productions.
****************************************************************************/
void set_city_names_font_sizes(int my_city_names_font_size,
			       int my_city_productions_font_size)
{
  freelog(LOG_ERROR, "Ignore set_city_names_font_sizes call.");
  /* PORTME */
}

/**************************************************************************
  This function will change the current mouse cursor.
**************************************************************************/
void update_mouse_cursor(enum cursor_type new_cursor_type)
{
  /* PORT ME */
}
