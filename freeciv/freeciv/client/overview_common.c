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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* utility */
#include "log.h"

/* client */
#include "client_main.h" /* can_client_change_view() */
#include "climap.h"
#include "control.h"
#include "options.h"
#include "overview_common.h"

#include "mapview_g.h"

int OVERVIEW_TILE_SIZE = 2;
struct overview overview = {
  /* These are the default values.  All others are zeroed automatically. */
  .fog = TRUE,
  .layers = {[OLAYER_BACKGROUND] = TRUE,
	     [OLAYER_UNITS] = TRUE,
	     [OLAYER_CITIES] = TRUE,
	     [OLAYER_BORDERS_ON_OCEAN] = TRUE}
};

/*
 * Set to TRUE if the backing store is more recent than the version
 * drawn into overview.window.
 */
static bool overview_dirty = FALSE;

/****************************************************************************
  Translate from gui to natural coordinate systems.  This provides natural
  coordinates as a floating-point value so there is no loss of information
  in the resulting values.
****************************************************************************/
static void gui_to_natural_pos(const struct tileset *t,
			       double *ntl_x, double *ntl_y,
			       int gui_x, int gui_y)
{
  const double gui_xd = gui_x, gui_yd = gui_y;
  const double W = tileset_tile_width(t), H = tileset_tile_height(t);
  double map_x, map_y;

  /* First convert to map positions.  This ignores hex conversions; we're
   * not looking for an exact tile. */
  if (tileset_is_isometric(t)) {
    /* Includes hex cases. */
    map_x = (gui_xd * H + gui_yd * W) / (W * H);
    map_y = (gui_yd * W - gui_xd * H) / (W * H);
  } else {
    map_x = gui_xd / W;
    map_y = gui_yd / H;
  }

  /* Now convert to natural positions.  Note this assumes the macro form
   * of the conversion will work with floating-point values. */
  MAP_TO_NATURAL_POS(ntl_x, ntl_y, map_x, map_y);

}

/****************************************************************************
  Translate from gui to overview coordinate systems.
****************************************************************************/
static void gui_to_overview_pos(const struct tileset *t,
				int *ovr_x, int *ovr_y,
				int gui_x, int gui_y)
{
  double ntl_x, ntl_y;

  gui_to_natural_pos(t, &ntl_x, &ntl_y, gui_x, gui_y);

  /* Now convert straight to overview coordinates. */
  *ovr_x = floor((ntl_x - (double)overview.map_x0) * OVERVIEW_TILE_SIZE);
  *ovr_y = floor((ntl_y - (double)overview.map_y0) * OVERVIEW_TILE_SIZE);

  /* Now do additional adjustments.  See map_to_overview_pos(). */
  if (topo_has_flag(TF_WRAPX)) {
    *ovr_x = FC_WRAP(*ovr_x, NATURAL_WIDTH * OVERVIEW_TILE_SIZE);
  } else {
    if (MAP_IS_ISOMETRIC) {
      /* HACK: For iso-maps that don't wrap in the X direction we clip
       * a half-tile off of the left and right of the overview.  This
       * means some tiles only are halfway shown.  However it means we
       * don't show any unreal tiles, which we'd otherwise be doing.  The
       * rest of the code can't handle unreal tiles in the overview. */
      *ovr_x -= OVERVIEW_TILE_SIZE;
    }
  }
  if (topo_has_flag(TF_WRAPY)) {
    *ovr_y = FC_WRAP(*ovr_y, NATURAL_HEIGHT * OVERVIEW_TILE_SIZE);
  }
}

/****************************************************************************
  Return color for overview map tile.
****************************************************************************/
static struct color *overview_tile_color(struct tile *ptile)
{
  if (overview.layers[OLAYER_CITIES]) {
    struct city *pcity = tile_city(ptile);

    if (pcity) {
      if (NULL == client.conn.playing
          || city_owner(pcity) == client.conn.playing) {
	return get_color(tileset, COLOR_OVERVIEW_MY_CITY);
      } else if (pplayers_allied(city_owner(pcity), client.conn.playing)) {
	/* Includes teams. */
	return get_color(tileset, COLOR_OVERVIEW_ALLIED_CITY);
      } else {
	return get_color(tileset, COLOR_OVERVIEW_ENEMY_CITY);
      }
    }
  }
  if (overview.layers[OLAYER_UNITS]) {
    struct unit *punit = find_visible_unit(ptile);

    if (punit) {
      if (NULL == client.conn.playing
          || unit_owner(punit) == client.conn.playing) {
	return get_color(tileset, COLOR_OVERVIEW_MY_UNIT);
      } else if (pplayers_allied(unit_owner(punit), client.conn.playing)) {
	/* Includes teams. */
	return get_color(tileset, COLOR_OVERVIEW_ALLIED_UNIT);
      } else {
	return get_color(tileset, COLOR_OVERVIEW_ENEMY_UNIT);
      }
    }
  }
  if (overview.layers[OLAYER_BORDERS]) {
    struct player *owner = tile_owner(ptile);

    if (owner) {
      if (overview.layers[OLAYER_BORDERS_ON_OCEAN]) {
        return get_player_color(tileset, owner);
      } else if (!is_ocean_tile(ptile)) {
        return get_player_color(tileset, owner);
      }
    }
  }
  if (overview.layers[OLAYER_RELIEF] && tile_terrain(ptile) != T_UNKNOWN) {
    return get_terrain_color(tileset, tile_terrain(ptile));
  }
  if (overview.layers[OLAYER_BACKGROUND] && tile_terrain(ptile) != T_UNKNOWN) {
    if (is_ocean_tile(ptile)) {
      return get_color(tileset, COLOR_OVERVIEW_OCEAN);
    } else {
      return get_color(tileset, COLOR_OVERVIEW_LAND);
    }
  }

  return get_color(tileset, COLOR_OVERVIEW_UNKNOWN);
}

/**************************************************************************
  Copies the overview image from the backing store to the window and
  draws the viewrect on top of it.
**************************************************************************/
static void redraw_overview(void)
{
  struct canvas *dest = get_overview_window();
  int i, x[4], y[4];

  if (!dest || !overview.map) {
    return;
  }

  {
    struct canvas *src = overview.map, *dst = overview.window;
    int x = overview.map_x0 * OVERVIEW_TILE_SIZE;
    int y = overview.map_y0 * OVERVIEW_TILE_SIZE;
    int ix = overview.width - x;
    int iy = overview.height - y;

    canvas_copy(dst, src, 0, 0, ix, iy, x, y);
    canvas_copy(dst, src, 0, y, ix, 0, x, iy);
    canvas_copy(dst, src, x, 0, 0, iy, ix, y);
    canvas_copy(dst, src, x, y, 0, 0, ix, iy);
  }

  gui_to_overview_pos(tileset, &x[0], &y[0],
		      mapview.gui_x0, mapview.gui_y0);
  gui_to_overview_pos(tileset, &x[1], &y[1],
		      mapview.gui_x0 + mapview.width, mapview.gui_y0);
  gui_to_overview_pos(tileset, &x[2], &y[2],
		      mapview.gui_x0 + mapview.width,
		      mapview.gui_y0 + mapview.height);
  gui_to_overview_pos(tileset, &x[3], &y[3],
		      mapview.gui_x0, mapview.gui_y0 + mapview.height);

  for (i = 0; i < 4; i++) {
    int src_x = x[i];
    int src_y = y[i];
    int dst_x = x[(i + 1) % 4];
    int dst_y = y[(i + 1) % 4];

    canvas_put_line(overview.window,
		    get_color(tileset, COLOR_OVERVIEW_VIEWRECT),
		    LINE_NORMAL,
		    src_x, src_y, dst_x - src_x, dst_y - src_y);
  }

  canvas_copy(dest, overview.window,
	      0, 0, 0, 0, overview.width, overview.height);

  overview_dirty = FALSE;
}

/****************************************************************************
  Mark the overview as "dirty" so that it will be redrawn soon.
****************************************************************************/
static void dirty_overview(void)
{
  overview_dirty = TRUE;
}

/****************************************************************************
  Redraw the overview if it is "dirty".
****************************************************************************/
void flush_dirty_overview(void)
{
  /* Currently this function is called from mapview_common.  However it
   * should be made static eventually. */
  if (overview_dirty) {
    redraw_overview();
  }
}

/****************************************************************************
  Equivalent to FC_WRAP, but it works for doubles.
****************************************************************************/
static double wrap_double(double value, double wrap)
{
  while (value < 0) {
    value += wrap;
  }
  while (value >= wrap) {
    value -= wrap;
  }
  return value;
}

/**************************************************************************
  Center the overview around the mapview.
**************************************************************************/
void center_tile_overviewcanvas(void)
{
  double ntl_x, ntl_y;

  gui_to_natural_pos(tileset, &ntl_x, &ntl_y,
		     mapview.gui_x0 + mapview.width / 2,
		     mapview.gui_y0 + mapview.height / 2);

  /* NOTE: this embeds the map wrapping in the overview code.  This is
   * basically necessary for the overview to be efficiently
   * updated. */
  if (topo_has_flag(TF_WRAPX)) {
    overview.map_x0 = wrap_double(ntl_x - (double)NATURAL_WIDTH / 2.0,
				  NATURAL_WIDTH);
  } else {
    overview.map_x0 = 0;
  }
  if (topo_has_flag(TF_WRAPY)) {
    overview.map_y0 = wrap_double(ntl_y - (double)NATURAL_HEIGHT / 2.0,
				  NATURAL_HEIGHT);
  } else {
    overview.map_y0 = 0;
  }

  redraw_overview();
}

/**************************************************************************
  Finds the overview (canvas) coordinates for a given map position.
**************************************************************************/
void map_to_overview_pos(int *overview_x, int *overview_y,
			 int map_x, int map_y)
{
  /* The map position may not be normal, for instance when the mapview
   * origin is not a normal position.
   *
   * NOTE: this embeds the map wrapping in the overview code. */
  do_in_natural_pos(ntl_x, ntl_y, map_x, map_y) {
    int ovr_x = ntl_x - overview.map_x0, ovr_y = ntl_y - overview.map_y0;

    if (topo_has_flag(TF_WRAPX)) {
      ovr_x = FC_WRAP(ovr_x, NATURAL_WIDTH);
    } else {
      if (MAP_IS_ISOMETRIC) {
	/* HACK: For iso-maps that don't wrap in the X direction we clip
	 * a half-tile off of the left and right of the overview.  This
	 * means some tiles only are halfway shown.  However it means we
	 * don't show any unreal tiles, which we'd otherwise be doing.  The
	 * rest of the code can't handle unreal tiles in the overview. */
	ovr_x--;
      }
    }
    if (topo_has_flag(TF_WRAPY)) {
      ovr_y = FC_WRAP(ovr_y, NATURAL_HEIGHT);
    }
    *overview_x = OVERVIEW_TILE_SIZE * ovr_x;
    *overview_y = OVERVIEW_TILE_SIZE * ovr_y;
  } do_in_natural_pos_end;
}

/**************************************************************************
  Finds the map coordinates for a given overview (canvas) position.
**************************************************************************/
void overview_to_map_pos(int *map_x, int *map_y,
			 int overview_x, int overview_y)
{
  int ntl_x = overview_x / OVERVIEW_TILE_SIZE + overview.map_x0;
  int ntl_y = overview_y / OVERVIEW_TILE_SIZE + overview.map_y0;

  if (MAP_IS_ISOMETRIC && !topo_has_flag(TF_WRAPX)) {
    /* Clip half tile left and right.  See comment in map_to_overview_pos. */
    ntl_x++;
  }

  NATURAL_TO_MAP_POS(map_x, map_y, ntl_x, ntl_y);
  if (!normalize_map_pos(map_x, map_y)) {
    /* All positions on the overview should be valid. */
    assert(FALSE);
  }
}

/**************************************************************************
  Redraw the entire backing store for the overview minimap.
**************************************************************************/
void refresh_overview_canvas(void)
{
  if (!can_client_change_view()) {
    return;
  }
  whole_map_iterate(ptile) {
    overview_update_tile(ptile);
  } whole_map_iterate_end;
  redraw_overview();
}

/****************************************************************************
  Draws the color for this tile onto the given rectangle of the canvas.

  This is just a simple helper function for overview_update_tile, since
  sometimes a tile may cover more than one rectangle.
****************************************************************************/
static void put_overview_tile_area(struct canvas *pcanvas,
				   struct tile *ptile,
				   int x, int y, int w, int h)
{
  canvas_put_rectangle(pcanvas,
		       overview_tile_color(ptile),
		       x, y, w, h);
  if (overview.fog
      && TILE_KNOWN_UNSEEN == client_tile_get_known(ptile)) {
    canvas_put_sprite(pcanvas, x, y, get_basic_fog_sprite(tileset),
		      0, 0, w, h);
  }
}

/**************************************************************************
  Redraw the given map position in the overview canvas.
**************************************************************************/
void overview_update_tile(struct tile *ptile)
{
  /* Base overview positions are just like natural positions, but scaled to
   * the overview tile dimensions. */
  do_in_natural_pos(ntl_x, ntl_y, ptile->x, ptile->y) {
    int overview_y = ntl_y * OVERVIEW_TILE_SIZE;
    int overview_x = ntl_x * OVERVIEW_TILE_SIZE;

    if (MAP_IS_ISOMETRIC) {
      if (topo_has_flag(TF_WRAPX)) {
	if (overview_x > overview.width - OVERVIEW_TILE_WIDTH) {
	  /* This tile is shown half on the left and half on the right
	   * side of the overview.  So we have to draw it in two parts. */
	  put_overview_tile_area(overview.map, ptile,
				 overview_x - overview.width, overview_y,
				 OVERVIEW_TILE_WIDTH, OVERVIEW_TILE_HEIGHT); 
	}     
      } else {
	/* Clip half tile left and right.
	 * See comment in map_to_overview_pos. */
	overview_x -= OVERVIEW_TILE_SIZE;
      }
    } 

    put_overview_tile_area(overview.map, ptile,
			   overview_x, overview_y,
			   OVERVIEW_TILE_WIDTH, OVERVIEW_TILE_HEIGHT);

    dirty_overview();
  } do_in_natural_pos_end;
}

/**************************************************************************
  Called if the map size is know or changes.
**************************************************************************/
void calculate_overview_dimensions(void)
{
  int w, h;
  int xfact = MAP_IS_ISOMETRIC ? 2 : 1;

  static int recursion = 0; /* Just to be safe. */

  /* Clip half tile left and right.  See comment in map_to_overview_pos. */
  int shift = (MAP_IS_ISOMETRIC && !topo_has_flag(TF_WRAPX)) ? -1 : 0;

  if (recursion > 0 || map.xsize <= 0 || map.ysize <= 0) {
    return;
  }
  recursion++;

  get_overview_area_dimensions(&w, &h);

  freelog(LOG_DEBUG, "Map size %d,%d - area size %d,%d",
	  map.xsize, map.ysize, w, h);

  /* Set the scale of the overview map.  This attempts to limit the
   * overview to the size of the area available.
   *
   * It rounds up since this gives good results with the default settings.
   * It may need tweaking if the panel resizes itself. */
  OVERVIEW_TILE_SIZE = MIN((w - 1) / (map.xsize * xfact) + 1,
			   (h - 1) / map.ysize + 1);
  OVERVIEW_TILE_SIZE = MAX(OVERVIEW_TILE_SIZE, 1);

  overview.width
    = OVERVIEW_TILE_WIDTH * map.xsize + shift * OVERVIEW_TILE_SIZE; 
  overview.height = OVERVIEW_TILE_HEIGHT * map.ysize;

  if (overview.map) {
    canvas_free(overview.map);
    canvas_free(overview.window);
  }
  overview.map = canvas_create(overview.width, overview.height);
  overview.window = canvas_create(overview.width, overview.height);
  canvas_put_rectangle(overview.map,
		       get_color(tileset, COLOR_OVERVIEW_UNKNOWN),
		       0, 0, overview.width, overview.height);
  update_map_canvas_scrollbars_size();

  /* Call gui specific function. */
  overview_size_changed();

  if (can_client_change_view()) {
    refresh_overview_canvas();
  }

  recursion--;
}

/****************************************************************************
  Callback to be called when an overview option is changed.
****************************************************************************/
void overview_redraw_callback(struct client_option *option)
{
  /* This is called once for each option changed so it is slower than
   * necessary.  If this becomes a problem it could be switched to use a
   * queue system like the mapview drawing code does. */
  refresh_overview_canvas();
}
