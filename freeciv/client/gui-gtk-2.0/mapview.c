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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdio.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gtk/gtk.h>

/* utility */
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "rand.h"
#include "support.h"
#include "timing.h"

/* common */
#include "government.h"		/* government_graphic() */
#include "map.h"
#include "player.h"

/* client */
#include "client_main.h"
#include "climap.h"
#include "climisc.h"
#include "colors.h"
#include "control.h" /* get_unit_in_focus() */
#include "editgui.h"
#include "editor.h"
#include "graphics.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "mapctrl.h"
#include "options.h"
#include "overview_common.h"
#include "tilespec.h"
#include "text.h"
#include "wldlg.h"

#include "citydlg.h" /* For reset_city_dialogs() */
#include "mapview.h"

static GtkObject *map_hadj, *map_vadj;
static int cursor_timer_id = 0, cursor_type = -1, cursor_frame = 0;

/**************************************************************************
  If do_restore is FALSE it will invert the turn done button style. If
  called regularly from a timer this will give a blinking turn done
  button. If do_restore is TRUE this will reset the turn done button
  to the default style.
**************************************************************************/
void update_turn_done_button(bool do_restore)
{
  static bool flip = FALSE;

  if (!get_turn_done_button_state()) {
    return;
  }

  if ((do_restore && flip) || !do_restore) {
    GdkGC *fore = turn_done_button->style->bg_gc[GTK_STATE_NORMAL];
    GdkGC *back = turn_done_button->style->light_gc[GTK_STATE_NORMAL];

    turn_done_button->style->bg_gc[GTK_STATE_NORMAL] = back;
    turn_done_button->style->light_gc[GTK_STATE_NORMAL] = fore;

    gtk_expose_now(turn_done_button);

    flip = !flip;
  }
}

/**************************************************************************
...
**************************************************************************/
void update_timeout_label(void)
{
  gtk_label_set_text(GTK_LABEL(timeout_label), get_timeout_label_text());
}

/**************************************************************************
...
**************************************************************************/
void update_info_label( void )
{
  GtkWidget *label;
  const struct player *pplayer = client.conn.playing;

  label = gtk_frame_get_label_widget(GTK_FRAME(main_frame_civ_name));
  if (pplayer != NULL) {
    char nation[MAX_LEN_NAME];

    /* Capitalize the first character of the translated nation
     * plural name so that the frame label looks good. I assume
     * that in the case that capitalization does not make sense
     * (e.g. multi-byte characters or no "upper case" form in
     * the translation language) my_toupper() will just return
     * the same value as was passed into it. */
    sz_strlcpy(nation, nation_plural_for_player(pplayer));
    nation[0] = my_toupper(nation[0]);

    gtk_label_set_text(GTK_LABEL(label), nation);
  } else {
    gtk_label_set_text(GTK_LABEL(label), "-");
  }

  gtk_label_set_text(GTK_LABEL(main_label_info), get_info_label_text());

  set_indicator_icons(client_research_sprite(),
		      client_warming_sprite(),
		      client_cooling_sprite(),
		      client_government_sprite());

  if (NULL != client.conn.playing) {
    int d = 0;

    for (; d < client.conn.playing->economic.luxury /10; d++) {
      struct sprite *sprite = get_tax_sprite(tileset, O_LUXURY);

      gtk_image_set_from_pixbuf(GTK_IMAGE(econ_label[d]),
				sprite_get_pixbuf(sprite));
    }
 
    for (; d < (client.conn.playing->economic.science
		+ client.conn.playing->economic.luxury) / 10; d++) {
      struct sprite *sprite = get_tax_sprite(tileset, O_SCIENCE);

      gtk_image_set_from_pixbuf(GTK_IMAGE(econ_label[d]),
				sprite_get_pixbuf(sprite));
    }
 
    for (; d < 10; d++) {
      struct sprite *sprite = get_tax_sprite(tileset, O_GOLD);

      gtk_image_set_from_pixbuf(GTK_IMAGE(econ_label[d]),
				sprite_get_pixbuf(sprite));
    }
  }
 
  update_timeout_label();

  /* update tooltips. */
  gtk_tooltips_set_tip(main_tips, econ_ebox,
		       _("Shows your current luxury/science/tax rates;"
			 "click to toggle them."), "");

  gtk_tooltips_set_tip(main_tips, bulb_ebox, get_bulb_tooltip(), "");
  gtk_tooltips_set_tip(main_tips, sun_ebox, get_global_warming_tooltip(),
		       "");
  gtk_tooltips_set_tip(main_tips, flake_ebox, get_nuclear_winter_tooltip(),
		       "");
  gtk_tooltips_set_tip(main_tips, government_ebox, get_government_tooltip(),
		       "");
}

/**************************************************************************
  This function is used to animate the mouse cursor. 
**************************************************************************/
static gint anim_cursor_cb(gpointer data)
{
  if (!cursor_timer_id) {
    return FALSE;
  }

  cursor_frame++;
  if (cursor_frame == NUM_CURSOR_FRAMES) {
    cursor_frame = 0;
  }

  if (cursor_type == CURSOR_DEFAULT) {
    gdk_window_set_cursor(root_window, NULL);
    cursor_timer_id = 0;
    return FALSE; 
  }

  gdk_window_set_cursor(root_window,
                fc_cursors[cursor_type][cursor_frame]);
  control_mouse_cursor(NULL);
  return TRUE;
}

/**************************************************************************
  This function will change the current mouse cursor.
**************************************************************************/
void update_mouse_cursor(enum cursor_type new_cursor_type)
{
  cursor_type = new_cursor_type;
  if (!cursor_timer_id) {
    cursor_timer_id = gtk_timeout_add(CURSOR_INTERVAL, anim_cursor_cb, NULL);
  }
}

/**************************************************************************
  Update the information label which gives info on the current unit and the
  square under the current unit, for specified unit.  Note that in practice
  punit is always the focus unit.
  Clears label if punit is NULL.
  Also updates the cursor for the map_canvas (this is related because the
  info label includes a "select destination" prompt etc).
  Also calls update_unit_pix_label() to update the icons for units on this
  square.
**************************************************************************/
void update_unit_info_label(struct unit_list *punits)
{
  GtkWidget *label;

  label = gtk_frame_get_label_widget(GTK_FRAME(unit_info_frame));
  gtk_label_set_text(GTK_LABEL(label),
		     get_unit_info_label_text1(punits));

  gtk_label_set_text(GTK_LABEL(unit_info_label),
		     get_unit_info_label_text2(punits, 0));

  update_unit_pix_label(punits);
}


/**************************************************************************
...
**************************************************************************/
GdkPixbuf *get_thumb_pixbuf(int onoff)
{
  return sprite_get_pixbuf(get_treaty_thumb_sprite(tileset, BOOL_VAL(onoff)));
}

/****************************************************************************
  Set information for the indicator icons typically shown in the main
  client window.  The parameters tell which sprite to use for the
  indicator.
****************************************************************************/
void set_indicator_icons(struct sprite *bulb, struct sprite *sol,
			 struct sprite *flake, struct sprite *gov)
{
  gtk_image_set_from_pixbuf(GTK_IMAGE(bulb_label),
			    sprite_get_pixbuf(bulb));
  gtk_image_set_from_pixbuf(GTK_IMAGE(sun_label),
			    sprite_get_pixbuf(sol));
  gtk_image_set_from_pixbuf(GTK_IMAGE(flake_label),
			    sprite_get_pixbuf(flake));
  gtk_image_set_from_pixbuf(GTK_IMAGE(government_label),
			    sprite_get_pixbuf(gov));
}

/****************************************************************************
  Return the dimensions of the area (container widget; maximum size) for
  the overview.
****************************************************************************/
void get_overview_area_dimensions(int *width, int *height)
{
  gdk_drawable_get_size(overview_canvas->window, width, height);
}

/**************************************************************************
...
**************************************************************************/
void overview_size_changed(void)
{
  gtk_widget_set_size_request(overview_canvas,
			      overview.width, overview.height);
  update_map_canvas_scrollbars_size();
}

/****************************************************************************
  Return a canvas that is the overview window.
****************************************************************************/
struct canvas *get_overview_window(void)
{
  static struct canvas store;

  store.type = CANVAS_PIXMAP;
  store.v.pixmap = overview_canvas->window;
  return &store;
}

/**************************************************************************
...
**************************************************************************/
gboolean overview_canvas_expose(GtkWidget *w, GdkEventExpose *ev, gpointer data)
{
  if (!can_client_change_view()) {
    if (radar_gfx_sprite) {
      gdk_draw_drawable(overview_canvas->window, civ_gc,
			radar_gfx_sprite->pixmap, ev->area.x, ev->area.y,
			ev->area.x, ev->area.y,
			ev->area.width, ev->area.height);
    }
    return TRUE;
  }
  
  refresh_overview_canvas();
  return TRUE;
}

/**************************************************************************
...
**************************************************************************/
static bool map_center = TRUE;
static bool map_configure = FALSE;

gboolean map_canvas_configure(GtkWidget * w, GdkEventConfigure * ev,
			      gpointer data)
{
  if (map_canvas_resized(ev->width, ev->height)) {
    map_configure = TRUE;
  }

  return TRUE;
}

/**************************************************************************
...
**************************************************************************/
gboolean map_canvas_expose(GtkWidget *w, GdkEventExpose *ev, gpointer data)
{
  static bool cleared = FALSE;

  if (!can_client_change_view()) {
    if (!cleared) {
      gtk_widget_queue_draw(w);
      cleared = TRUE;
    }
    map_center = TRUE;
  }
  else
  {
    if (map_exists()) { /* do we have a map at all */
      /* First we mark the area to be updated as dirty.  Then we unqueue
       * any pending updates, to make sure only the most up-to-date data
       * is written (otherwise drawing bugs happen when old data is copied
       * to screen).  Then we draw all changed areas to the screen. */
      unqueue_mapview_updates(FALSE);
      gdk_draw_drawable(map_canvas->window, civ_gc, mapview.store->v.pixmap,
			ev->area.x, ev->area.y, ev->area.x, ev->area.y,
			ev->area.width, ev->area.height);
      cleared = FALSE;
    } else {
      if (!cleared) {
        gtk_widget_queue_draw(w);
	cleared = TRUE;
      }
    }

    if (!map_center) {
      center_on_something();
      map_center = FALSE;
    }
  }

  map_configure = FALSE;

  return TRUE;
}

/**************************************************************************
  Flush the given part of the canvas buffer (if there is one) to the
  screen.
**************************************************************************/
void flush_mapcanvas(int canvas_x, int canvas_y,
		     int pixel_width, int pixel_height)
{
  gdk_draw_drawable(map_canvas->window, civ_gc, mapview.store->v.pixmap,
		    canvas_x, canvas_y, canvas_x, canvas_y,
		    pixel_width, pixel_height);
}

#define MAX_DIRTY_RECTS 20
static int num_dirty_rects = 0;
static GdkRectangle dirty_rects[MAX_DIRTY_RECTS];
static bool is_flush_queued = FALSE;

/**************************************************************************
  A callback invoked as a result of gtk_idle_add, this function simply
  flushes the mapview canvas.
**************************************************************************/
static gint unqueue_flush(gpointer data)
{
  flush_dirty();
  is_flush_queued = FALSE;
  return 0;
}

/**************************************************************************
  Called when a region is marked dirty, this function queues a flush event
  to be handled later by GTK.  The flush may end up being done
  by freeciv before then, in which case it will be a wasted call.
**************************************************************************/
static void queue_flush(void)
{
  if (!is_flush_queued) {
    gtk_idle_add(unqueue_flush, NULL);
    is_flush_queued = TRUE;
  }
}

/**************************************************************************
  Mark the rectangular region as "dirty" so that we know to flush it
  later.
**************************************************************************/
void dirty_rect(int canvas_x, int canvas_y,
		int pixel_width, int pixel_height)
{
  if (num_dirty_rects < MAX_DIRTY_RECTS) {
    dirty_rects[num_dirty_rects].x = canvas_x;
    dirty_rects[num_dirty_rects].y = canvas_y;
    dirty_rects[num_dirty_rects].width = pixel_width;
    dirty_rects[num_dirty_rects].height = pixel_height;
    num_dirty_rects++;
    queue_flush();
  }
}

/**************************************************************************
  Mark the entire screen area as "dirty" so that we can flush it later.
**************************************************************************/
void dirty_all(void)
{
  num_dirty_rects = MAX_DIRTY_RECTS;
  queue_flush();
}

/**************************************************************************
  Flush all regions that have been previously marked as dirty.  See
  dirty_rect and dirty_all.  This function is generally called after we've
  processed a batch of drawing operations.
**************************************************************************/
void flush_dirty(void)
{
  if (num_dirty_rects == MAX_DIRTY_RECTS) {
    flush_mapcanvas(0, 0, map_canvas->allocation.width,
		    map_canvas->allocation.height);
  } else {
    int i;

    for (i = 0; i < num_dirty_rects; i++) {
      flush_mapcanvas(dirty_rects[i].x, dirty_rects[i].y,
		      dirty_rects[i].width, dirty_rects[i].height);
    }
  }
  num_dirty_rects = 0;
}

/****************************************************************************
  Do any necessary synchronization to make sure the screen is up-to-date.
  The canvas should have already been flushed to screen via flush_dirty -
  all this function does is make sure the hardware has caught up.
****************************************************************************/
void gui_flush(void)
{
  gdk_flush();
}

/**************************************************************************
 Update display of descriptions associated with cities on the main map.
**************************************************************************/
void update_city_descriptions(void)
{
  update_map_canvas_visible();
}

/**************************************************************************
...
**************************************************************************/
void put_unit_gpixmap(struct unit *punit, GtkPixcomm *p)
{
  struct canvas canvas_store;

  canvas_store.type = CANVAS_PIXCOMM;
  canvas_store.v.pixcomm = p;

  gtk_pixcomm_freeze(p);
  gtk_pixcomm_clear(p);

  put_unit(punit, &canvas_store, 0, 0);

  gtk_pixcomm_thaw(p);
}


/**************************************************************************
  FIXME:
  For now only two food, two gold one shield and two masks can be drawn per
  unit, the proper way to do this is probably something like what Civ II does.
  (One food/shield/mask drawn N times, possibly one top of itself. -- SKi 
**************************************************************************/
void put_unit_gpixmap_city_overlays(struct unit *punit, GtkPixcomm *p,
                                    int *upkeep_cost, int happy_cost)
{
  struct canvas store;
 
  store.type = CANVAS_PIXCOMM;
  store.v.pixcomm = p;

  gtk_pixcomm_freeze(p);

  put_unit_city_overlays(punit, &store, 0, tileset_tile_height(tileset),
                         upkeep_cost, happy_cost);

  gtk_pixcomm_thaw(p);
}

/**************************************************************************
...
**************************************************************************/
void pixmap_put_overlay_tile(GdkDrawable *pixmap,
			     int canvas_x, int canvas_y,
			     struct sprite *ssprite)
{
  if (!ssprite) {
    return;
  }

  if (ssprite->pixmap) {
    gdk_gc_set_clip_origin(civ_gc, canvas_x, canvas_y);
    gdk_gc_set_clip_mask(civ_gc, ssprite->mask);

    gdk_draw_drawable(pixmap, civ_gc, ssprite->pixmap,
		      0, 0,
		      canvas_x, canvas_y,
		      ssprite->width, ssprite->height);
    gdk_gc_set_clip_mask(civ_gc, NULL);
  } else {
    gdk_draw_pixbuf(pixmap, civ_gc, ssprite->pixbuf,
		    0, 0,
		  canvas_x, canvas_y,
		  ssprite->width, ssprite->height,
		  GDK_RGB_DITHER_NONE, 0, 0);
  }
}

/**************************************************************************
  Place part of a (possibly masked) sprite on a pixmap.
**************************************************************************/
static void pixmap_put_sprite(GdkDrawable *pixmap,
			      int pixmap_x, int pixmap_y,
			      struct sprite *ssprite,
			      int offset_x, int offset_y,
			      int width, int height)
{
#ifdef DEBUG
  static int sprites = 0, pixbufs = 0;
#endif

  if (ssprite->pixmap) {
    if (ssprite->mask) {
      gdk_gc_set_clip_origin(civ_gc, pixmap_x, pixmap_y);
      gdk_gc_set_clip_mask(civ_gc, ssprite->mask);
    }

    gdk_draw_drawable(pixmap, civ_gc, ssprite->pixmap,
		      offset_x, offset_y,
		      pixmap_x + offset_x, pixmap_y + offset_y,
		      MIN(width, MAX(0, ssprite->width - offset_x)),
		      MIN(height, MAX(0, ssprite->height - offset_y)));

    gdk_gc_set_clip_mask(civ_gc, NULL);
  } else {
    gdk_draw_pixbuf(pixmap, civ_gc, ssprite->pixbuf,
		    offset_x, offset_y,
		    pixmap_x + offset_x, pixmap_y + offset_y,
		    MIN(width, MAX(0, ssprite->width - offset_x)),
		    MIN(height, MAX(0, ssprite->height - offset_y)),
		    GDK_RGB_DITHER_NONE, 0, 0);
#ifdef DEBUG
    pixbufs++;
#endif
  }

#ifdef DEBUG
  sprites++;
  if (sprites % 1000 == 0) {
    freelog(LOG_DEBUG, "%5d / %5d pixbufs = %d%%",
	    pixbufs, sprites, 100 * pixbufs / sprites);
  }
#endif
}

/**************************************************************************
  Created a fogged version of the sprite.  This can fail on older systems
  in which case the callers needs a fallback.
**************************************************************************/
static void fog_sprite(struct sprite *sprite)
{
  int x, y;
  GdkPixbuf *fogged;
  guchar *pixel;
  const int bright = 65; /* Brightness percentage */

  if (sprite->pixmap) {
    fogged = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8,
			    sprite->width, sprite->height);
    gdk_pixbuf_get_from_drawable(fogged, sprite->pixmap, NULL,
				 0, 0, 0, 0, sprite->width, sprite->height);
  } else {
    fogged = gdk_pixbuf_copy(sprite->pixbuf);
  }

  /* Iterate over all pixels, reducing brightness by 50%. */
  for (x = 0; x < sprite->width; x++) {
    for (y = 0; y < sprite->height; y++) {
      pixel = gdk_pixbuf_get_pixels(fogged)
	+ y * gdk_pixbuf_get_rowstride(fogged)
	+ x * gdk_pixbuf_get_n_channels(fogged);

      pixel[0] = pixel[0] * bright / 100;
      pixel[1] = pixel[1] * bright / 100;
      pixel[2] = pixel[2] * bright / 100;
    }
  }

  if (sprite->pixmap) {
    gdk_pixbuf_render_pixmap_and_mask(fogged, &sprite->pixmap_fogged,
				      NULL, 0);
    g_object_unref(fogged);
  } else {
    sprite->pixbuf_fogged = fogged;
  }
}

/**************************************************************************
Only used for isometric view.
**************************************************************************/
void pixmap_put_overlay_tile_draw(GdkDrawable *pixmap,
				  int canvas_x, int canvas_y,
				  struct sprite *ssprite,
				  bool fog)
{
  if (!ssprite) {
    return;
  }

  if (fog && gui_gtk2_better_fog
      && ((ssprite->pixmap && !ssprite->pixmap_fogged)
	  || (!ssprite->pixmap && !ssprite->pixbuf_fogged))) {
    fog_sprite(ssprite);
    if ((ssprite->pixmap && !ssprite->pixmap_fogged)
	|| (!ssprite->pixmap && !ssprite->pixbuf_fogged)) {
      freelog(LOG_NORMAL,
	      _("Better fog will only work in truecolor.  Disabling it"));
      gui_gtk2_better_fog = FALSE;
    }
  }

  if (fog && gui_gtk2_better_fog) {
    if (ssprite->pixmap) {
      if (ssprite->mask) {
	gdk_gc_set_clip_origin(civ_gc, canvas_x, canvas_y);
	gdk_gc_set_clip_mask(civ_gc, ssprite->mask);
      }
      gdk_draw_drawable(pixmap, civ_gc,
			ssprite->pixmap_fogged,
			0, 0,
			canvas_x, canvas_y,
			ssprite->width, ssprite->height);
      gdk_gc_set_clip_mask(civ_gc, NULL);
    } else {
      gdk_draw_pixbuf(pixmap, civ_gc, ssprite->pixbuf_fogged,
		      0, 0, canvas_x, canvas_y, 
		      ssprite->width, ssprite->height,
		      GDK_RGB_DITHER_NONE, 0, 0);
    }
    return;
  }

  pixmap_put_sprite(pixmap, canvas_x, canvas_y, ssprite,
		    0, 0, ssprite->width, ssprite->height);

  /* I imagine this could be done more efficiently. Some pixels We first
     draw from the sprite, and then draw black afterwards. It would be much
     faster to just draw every second pixel black in the first place. */
  if (fog) {
    gdk_gc_set_clip_origin(fill_tile_gc, canvas_x, canvas_y);
    gdk_gc_set_clip_mask(fill_tile_gc, sprite_get_mask(ssprite));
    gdk_gc_set_foreground(fill_tile_gc,
			  &get_color(tileset, COLOR_MAPVIEW_UNKNOWN)->color);
    gdk_gc_set_ts_origin(fill_tile_gc, canvas_x, canvas_y);
    gdk_gc_set_stipple(fill_tile_gc, black50);

    gdk_draw_rectangle(pixmap, fill_tile_gc, TRUE,
		       canvas_x, canvas_y, ssprite->width, ssprite->height);
    gdk_gc_set_clip_mask(fill_tile_gc, NULL);
  }
}

/**************************************************************************
 Draws a cross-hair overlay on a tile
**************************************************************************/
void put_cross_overlay_tile(struct tile *ptile)
{
  int canvas_x, canvas_y;

  if (tile_to_canvas_pos(&canvas_x, &canvas_y, ptile)) {
    pixmap_put_overlay_tile(map_canvas->window,
			    canvas_x, canvas_y,
			    get_attention_crosshair_sprite(tileset));
  }
}

/**************************************************************************
...
**************************************************************************/
void update_map_canvas_scrollbars(void)
{
  int scroll_x, scroll_y;

  get_mapview_scroll_pos(&scroll_x, &scroll_y);
  gtk_adjustment_set_value(GTK_ADJUSTMENT(map_hadj), scroll_x);
  gtk_adjustment_set_value(GTK_ADJUSTMENT(map_vadj), scroll_y);
}

/**************************************************************************
...
**************************************************************************/
void update_map_canvas_scrollbars_size(void)
{
  int xmin, ymin, xmax, ymax, xsize, ysize, xstep, ystep;

  get_mapview_scroll_window(&xmin, &ymin, &xmax, &ymax, &xsize, &ysize);
  get_mapview_scroll_step(&xstep, &ystep);

  map_hadj = gtk_adjustment_new(-1, xmin, xmax, xstep, xsize, xsize);
  map_vadj = gtk_adjustment_new(-1, ymin, ymax, ystep, ysize, ysize);

  gtk_range_set_adjustment(GTK_RANGE(map_horizontal_scrollbar),
	GTK_ADJUSTMENT(map_hadj));
  gtk_range_set_adjustment(GTK_RANGE(map_vertical_scrollbar),
	GTK_ADJUSTMENT(map_vadj));

  g_signal_connect(map_hadj, "value_changed",
	G_CALLBACK(scrollbar_jump_callback),
	GINT_TO_POINTER(TRUE));
  g_signal_connect(map_vadj, "value_changed",
	G_CALLBACK(scrollbar_jump_callback),
	GINT_TO_POINTER(FALSE));
}

/**************************************************************************
...
**************************************************************************/
void scrollbar_jump_callback(GtkAdjustment *adj, gpointer hscrollbar)
{
  int scroll_x, scroll_y;

  if (!can_client_change_view()) {
    return;
  }

  get_mapview_scroll_pos(&scroll_x, &scroll_y);

  if (hscrollbar) {
    scroll_x = adj->value;
  } else {
    scroll_y = adj->value;
  }

  set_mapview_scroll_pos(scroll_x, scroll_y);
}

/**************************************************************************
  Draws a rectangle with top left corner at (canvas_x, canvas_y), and
  width 'w' and height 'h'. It is drawn using the 'selection_gc' context,
  so the pixel combining function is XOR. This means that drawing twice
  in the same place will restore the image to its original state.

  NB: A side effect of this function is to set the 'selection_gc' color
  to COLOR_MAPVIEW_SELECTION.
**************************************************************************/
void draw_selection_rectangle(int canvas_x, int canvas_y, int w, int h)
{
  GdkPoint points[5];
  struct color *pcolor;

  if (w == 0 || h == 0) {
    return;
  }

  pcolor = get_color(tileset, COLOR_MAPVIEW_SELECTION);
  if (!pcolor) {
    return;
  }

  /* gdk_draw_rectangle() must start top-left.. */
  points[0].x = canvas_x;
  points[0].y = canvas_y;

  points[1].x = canvas_x + w;
  points[1].y = canvas_y;

  points[2].x = canvas_x + w;
  points[2].y = canvas_y + h;

  points[3].x = canvas_x;
  points[3].y = canvas_y + h;

  points[4].x = canvas_x;
  points[4].y = canvas_y;

  gdk_gc_set_foreground(selection_gc, &pcolor->color);
  gdk_draw_lines(map_canvas->window, selection_gc,
                 points, ARRAY_SIZE(points));
}

/**************************************************************************
  This function is called when the tileset is changed.
**************************************************************************/
void tileset_changed(void)
{
  reset_city_dialogs();
  reset_unit_table();
  blank_max_unit_size();
  editgui_tileset_changed();

  /* keep the icon of the executable on Windows (see PR#36491) */
#ifndef WIN32_NATIVE
  gtk_window_set_icon(GTK_WINDOW(toplevel),
		sprite_get_pixbuf(get_icon_sprite(tileset, ICON_FREECIV)));
#endif
}
