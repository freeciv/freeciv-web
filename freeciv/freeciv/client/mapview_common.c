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

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

/* utility */
#include "fcintl.h"
#include "log.h"
#include "rand.h"
#include "support.h"
#include "timing.h"

/* common */
#include "featured_text.h"
#include "game.h"
#include "map.h"
#include "traderoutes.h"
#include "unitlist.h"

/* client/include */
#include "graphics_g.h"
#include "gui_main_g.h"
#include "mapctrl_g.h"
#include "mapview_g.h"

/* client */
#include "client_main.h"
#include "climap.h"
#include "control.h"
#include "editor.h"
#include "goto.h"
#include "citydlg_common.h"
#include "overview_common.h"
#include "tilespec.h"
#include "zoom.h"

#include "mapview_common.h"


struct tile_hash *mapdeco_highlight_table;
struct tile_hash *mapdeco_crosshair_table;

struct gotoline_counter {
  int line_count[DIR8_MAGIC_MAX];
};

static inline struct gotoline_counter *gotoline_counter_new(void);
static void gotoline_counter_destroy(struct gotoline_counter *pglc);

#define SPECHASH_TAG gotoline
#define SPECHASH_IKEY_TYPE struct tile *
#define SPECHASH_IDATA_TYPE struct gotoline_counter *
#define SPECHASH_IDATA_FREE gotoline_counter_destroy
#include "spechash.h"
#define gotoline_hash_iterate(hash, ptile, pglc)                            \
  TYPED_HASH_ITERATE(struct tile *, struct gotoline_counter *,              \
                     hash, ptile, pglc)
#define gotoline_hash_iterate_end HASH_ITERATE_END

struct gotoline_hash *mapdeco_gotoline_table;

struct view mapview;
bool can_slide = TRUE;

static bool frame_by_frame_animation = FALSE;

struct tile *center_tile = NULL;

static void base_canvas_to_map_pos(int *map_x, int *map_y,
                                   float canvas_x, float canvas_y);

enum update_type {
  /* Masks */
  UPDATE_NONE = 0,
  UPDATE_CITY_DESCRIPTIONS  = 1,
  UPDATE_MAP_CANVAS_VISIBLE = 2,
  UPDATE_TILE_LABELS        = 4
};

/* A tile update has a tile associated with it as well as an area type.
 * See unqueue_mapview_updates for a thorough explanation. */
enum tile_update_type {
  TILE_UPDATE_TILE_SINGLE,
  TILE_UPDATE_TILE_FULL,
  TILE_UPDATE_UNIT,
  TILE_UPDATE_CITY_DESC,
  TILE_UPDATE_CITYMAP,
  TILE_UPDATE_TILE_LABEL,
  TILE_UPDATE_COUNT
};
static void queue_mapview_update(enum update_type update);
static void queue_mapview_tile_update(struct tile *ptile,
				      enum tile_update_type type);

/* Helper struct for drawing trade routes. */
struct trade_route_line {
  float x, y, width, height;
};

/* A trade route line might need to be drawn in two parts. */
static const int MAX_TRADE_ROUTE_DRAW_LINES = 2;

static struct timer *anim_timer = NULL;

enum animation_type { ANIM_MOVEMENT, ANIM_BATTLE, ANIM_EXPL, ANIM_NUKE };

struct animation
{
  enum animation_type type;
  int id;
  bool finished;
  int old_x;
  int old_y;
  int width;
  int height;
  union {
    struct {
      struct unit *mover;
      struct tile *src;
      struct tile *dest;
      float canvas_dx;
      float canvas_dy;
    } movement;
    struct {
      struct unit *virt_loser;
      struct tile *loser_tile;
      int loser_hp_start;
      struct unit *virt_winner;
      struct tile *winner_tile;
      int winner_hp_start;
      int winner_hp_end;
      int steps;
    } battle;
    struct {
      struct tile *tile;
      const struct sprite_vector *sprites;
      int sprite_count;
    } expl;
    struct {
      bool shown;
      struct tile *nuke_tile;
    } nuke;
  };
};

#define SPECLIST_TAG animation
#define SPECLIST_TYPE struct animation
#include "speclist.h"

struct animation_list *animations = NULL;

/************************************************************************//**
  Initialize animations system
****************************************************************************/
void animations_init(void)
{
  animations = animation_list_new();
}

/************************************************************************//**
  Clean up animations system
****************************************************************************/
void animations_free(void)
{
  if (animations != NULL) {
    animation_list_destroy(animations);
  }
}

/************************************************************************//**
  Add new animation to the queue
****************************************************************************/
static void animation_add(struct animation *anim)
{
  if (animation_list_size(animations) == 0) {
    anim_timer = timer_renew(anim_timer, TIMER_USER, TIMER_ACTIVE);
    timer_start(anim_timer);
  }

  anim->finished = FALSE;
  anim->old_x = -1; /* Initial frame */
  animation_list_append(animations, anim);
}

/************************************************************************//**
  Progress animation of type 'movement'
****************************************************************************/
static bool movement_animation(struct animation *anim, double time_gone)
{
  float start_x, start_y;
  int new_x, new_y;
  double timing_sec = (double)gui_options.smooth_move_unit_msec / 1000.0;
  double mytime = MIN(time_gone, timing_sec);
  struct unit *punit = anim->movement.mover;

  if (punit != NULL) {
    tile_to_canvas_pos(&start_x, &start_y, anim->movement.src);
    if (tileset_is_isometric(tileset) && tileset_hex_height(tileset) == 0) {
      start_y -= tileset_tile_height(tileset) / 2 * map_zoom;
      start_y -= (tileset_unit_height(tileset) - tileset_full_tile_height(tileset)) * map_zoom;
    }
    new_x = start_x + anim->movement.canvas_dx * (mytime / timing_sec);
    new_y = start_y + anim->movement.canvas_dy * (mytime / timing_sec);

    if (anim->old_x >= 0) {
      update_map_canvas(anim->old_x, anim->old_y,
                        anim->width, anim->height);
    }
    put_unit(punit, mapview.store, map_zoom, new_x, new_y);
    dirty_rect(new_x, new_y, anim->width, anim->height);
    anim->old_x = new_x;
    anim->old_y = new_y;

    if (time_gone >= timing_sec) {
      /* Animation over */
      if (--anim->movement.mover->refcount <= 0) {
        FC_FREE(anim->movement.mover);
      }
      return TRUE;
    }
  } else {
    return TRUE;
  }

  return FALSE;
}

/************************************************************************//**
  Progress animation of type 'battle'
****************************************************************************/
static bool battle_animation(struct animation *anim, double time_gone)
{
  double time_per_step;
  int step;
  float canvas_x, canvas_y;
  double timing_sec = (double)gui_options.smooth_combat_step_msec
    * anim->battle.steps / 1000.0;

  if (time_gone >= timing_sec) {
    /* Animation over */

    unit_virtual_destroy(anim->battle.virt_winner);
    unit_virtual_destroy(anim->battle.virt_loser);

    return TRUE;
  }

  time_per_step = timing_sec / anim->battle.steps;
  step = time_gone / time_per_step;

  if (tile_to_canvas_pos(&canvas_x, &canvas_y, anim->battle.loser_tile)) {
    anim->battle.virt_loser->hp
      = anim->battle.loser_hp_start - (anim->battle.loser_hp_start
                                       * step / anim->battle.steps);

    if (tileset_is_isometric(tileset) && tileset_hex_height(tileset) == 0) {
      canvas_y -= tileset_tile_height(tileset) / 2 * map_zoom;
      canvas_y -= (tileset_unit_height(tileset) - tileset_full_tile_height(tileset)) * map_zoom;
    }

    put_unit(anim->battle.virt_loser, mapview.store, map_zoom,
             canvas_x, canvas_y);
    dirty_rect(canvas_x, canvas_y,
               tileset_tile_width(tileset),
               tileset_tile_height(tileset));
  }

  if (tile_to_canvas_pos(&canvas_x, &canvas_y, anim->battle.winner_tile)) {
    anim->battle.virt_winner->hp
      = anim->battle.winner_hp_start - ((anim->battle.winner_hp_start
                                         - anim->battle.winner_hp_end)
                                        * step / anim->battle.steps);

    if (tileset_is_isometric(tileset) && tileset_hex_height(tileset) == 0) {
      canvas_y -= tileset_tile_height(tileset) / 2 * map_zoom;
      canvas_y -= (tileset_unit_height(tileset) - tileset_full_tile_height(tileset)) * map_zoom;
    }
    put_unit(anim->battle.virt_winner, mapview.store, map_zoom,
             canvas_x, canvas_y);
    dirty_rect(canvas_x, canvas_y,
               tileset_tile_width(tileset),
               tileset_tile_height(tileset));
  }

  return FALSE;
}

/************************************************************************//**
  Progress animation of type 'explosion'
****************************************************************************/
static bool explosion_animation(struct animation *anim, double time_gone)
{
  float canvas_x, canvas_y;
  double timing_sec;

  if (anim->expl.sprite_count <= 0) {
    return TRUE;
  }

  timing_sec = (double)gui_options.smooth_combat_step_msec
    * anim->expl.sprite_count / 1000.0;

  if (tile_to_canvas_pos(&canvas_x, &canvas_y, anim->expl.tile)) {
    double time_per_frame = timing_sec / anim->expl.sprite_count;
    int frame = time_gone / time_per_frame;
    struct sprite *spr;
    int w, h;

    frame = MIN(frame, anim->expl.sprite_count - 1);

    if (anim->old_x >= 0) {
      update_map_canvas(anim->old_x, anim->old_y,
                        anim->width, anim->height);
    }

    spr = *sprite_vector_get(anim->expl.sprites, frame);
    get_sprite_dimensions(spr, &w, &h);

    canvas_put_sprite_full(mapview.store,
                           canvas_x + tileset_tile_width(tileset) / 2 * map_zoom
                           - w / 2,
                           canvas_y + tileset_tile_height(tileset) / 2 * map_zoom
                           - h / 2,
                           spr);
    dirty_rect(canvas_x, canvas_y, tileset_tile_width(tileset) * map_zoom,
               tileset_tile_height(tileset) * map_zoom);

    anim->old_x = canvas_x;
    anim->old_y = canvas_y;
  }

  if (time_gone >= timing_sec) {
    /* Animation over */
    return TRUE;
  }

  return FALSE;
}

/************************************************************************//**
  Progress animation of type 'nuke'
****************************************************************************/
static bool nuke_animation(struct animation *anim, double time_gone)
{
  if (!anim->nuke.shown) {
    float canvas_x, canvas_y;
    struct sprite *nuke_spr = get_nuke_explode_sprite(tileset);
    int w, h;

    (void) tile_to_canvas_pos(&canvas_x, &canvas_y, anim->nuke.nuke_tile);
    get_sprite_dimensions(nuke_spr, &w, &h);

    canvas_put_sprite_full(mapview.store,
                           canvas_x + tileset_tile_width(tileset) / 2 * map_zoom
                           - w / 2,
                           canvas_y + tileset_tile_height(tileset) / 2 * map_zoom
                           - h / 2,
                           nuke_spr);
    dirty_rect(canvas_x, canvas_y, tileset_tile_width(tileset) * map_zoom,
               tileset_tile_height(tileset) * map_zoom);

    anim->old_x = canvas_x;
    anim->old_y = canvas_y;

    anim->nuke.shown = TRUE;

    return FALSE;
  }

  if (time_gone > 1.0) {
    update_map_canvas_visible();

    return FALSE;
  }

  return FALSE;
}

/************************************************************************//**
  Progress current animation
****************************************************************************/
void update_animation(void)
{
  if (animation_list_size(animations) > 0) {
    struct animation *anim = animation_list_get(animations, 0);

    if (anim->finished) {
      /* Animation over */

      anim->id = -1;
      if (anim->old_x >= 0) {
        update_map_canvas(anim->old_x, anim->old_y, anim->width, anim->height);
      }
      animation_list_remove(animations, anim);
      free(anim);

      if (animation_list_size(animations) > 0) {
        /* Start next */
        anim_timer = timer_renew(anim_timer, TIMER_USER, TIMER_ACTIVE);
        timer_start(anim_timer);
      }
    } else {
      double time_gone = timer_read_seconds(anim_timer);
      bool finished = FALSE;

      switch (anim->type) {
      case ANIM_MOVEMENT:
        finished = movement_animation(anim, time_gone);
        break;
      case ANIM_BATTLE:
        finished = battle_animation(anim, time_gone);
        break;
      case ANIM_EXPL:
        finished = explosion_animation(anim, time_gone);
        break;
      case ANIM_NUKE:
        finished = nuke_animation(anim, time_gone);
      }

      if (finished) {
        anim->finished = TRUE;
      }
    }
  }
}

/************************************************************************//**
  Create a new goto line counter.
****************************************************************************/
static inline struct gotoline_counter *gotoline_counter_new(void)
{
  struct gotoline_counter *pglc = fc_calloc(1, sizeof(*pglc));
  return pglc;
}

/************************************************************************//**
  Create a new goto line counter.
****************************************************************************/
static void gotoline_counter_destroy(struct gotoline_counter *pglc)
{
  fc_assert_ret(NULL != pglc);
  free(pglc);
}

/************************************************************************//**
  Refreshes a single tile on the map canvas.
****************************************************************************/
void refresh_tile_mapcanvas(struct tile *ptile,
                            bool full_refresh, bool write_to_screen)
{
  if (full_refresh) {
    queue_mapview_tile_update(ptile, TILE_UPDATE_TILE_FULL);
  } else {
    queue_mapview_tile_update(ptile, TILE_UPDATE_TILE_SINGLE);
  }
  if (write_to_screen) {
    unqueue_mapview_updates(TRUE);
  }
}

/************************************************************************//**
  Refreshes a single unit on the map canvas.
****************************************************************************/
void refresh_unit_mapcanvas(struct unit *punit, struct tile *ptile,
			    bool full_refresh, bool write_to_screen)
{
  if (full_refresh && gui_options.draw_native) {
    queue_mapview_update(UPDATE_MAP_CANVAS_VISIBLE);
  } else if (full_refresh && unit_drawn_with_city_outline(punit, TRUE)) {
    queue_mapview_tile_update(ptile, TILE_UPDATE_CITYMAP);
  } else {
    queue_mapview_tile_update(ptile, TILE_UPDATE_UNIT);
  }
  if (write_to_screen) {
    unqueue_mapview_updates(TRUE);
  }
}

/************************************************************************//**
  Refreshes a single city on the map canvas.

  If full_refresh is given then the citymap area and the city text will
  also be refreshed.  Otherwise only the base city sprite is refreshed.
****************************************************************************/
void refresh_city_mapcanvas(struct city *pcity, struct tile *ptile,
                            bool full_refresh, bool write_to_screen)
{
  if (full_refresh && (gui_options.draw_map_grid || gui_options.draw_borders)) {
    queue_mapview_tile_update(ptile, TILE_UPDATE_CITYMAP);
  } else {
    queue_mapview_tile_update(ptile, TILE_UPDATE_UNIT);
  }
  if (write_to_screen) {
    unqueue_mapview_updates(TRUE);
  }
}

/************************************************************************//**
  Translate from a cartesian system to the GUI system.  This function works
  on vectors, meaning it can be passed a (dx,dy) pair and will return the
  change in GUI coordinates corresponding to this vector.  It is thus more
  general than map_to_gui_pos.

  Note that a gui_to_map_vector function is not possible, since the
  resulting map vector may differ based on the origin of the gui vector.
****************************************************************************/
void map_to_gui_vector(const struct tileset *t, float zoom,
		       float *gui_dx, float *gui_dy, int map_dx, int map_dy)
{
  if (tileset_is_isometric(t)) {
    /*
     * Convert the map coordinates to isometric GUI
     * coordinates.  We'll make tile map(0,0) be the origin, and
     * transform like this:
     * 
     *                     3
     * 123                2 6
     * 456 -> becomes -> 1 5 9
     * 789                4 8
     *                     7
     */
    *gui_dx = (map_dx - map_dy) * tileset_tile_width(t) / 2 * zoom;
    *gui_dy = (map_dx + map_dy) * tileset_tile_height(t) / 2 * zoom;
  } else {
    *gui_dx = map_dx * tileset_tile_height(t) * zoom;
    *gui_dy = map_dy * tileset_tile_width(t) * zoom;
  }
}

/************************************************************************//**
  Translate from map to gui coordinate systems.

  GUI coordinates are comparable to canvas coordinates but extend in all
  directions.  gui(0,0) == map(0,0).
****************************************************************************/
static void map_to_gui_pos(const struct tileset *t,
                           float *gui_x, float *gui_y, int map_x, int map_y)
{
  /* Since the GUI origin is the same as the map origin we can just do a
   * vector conversion. */
  map_to_gui_vector(t, map_zoom, gui_x, gui_y, map_x, map_y);
}

/************************************************************************//**
  Translate from gui to map coordinate systems.  See map_to_gui_pos().

  Note that you lose some information in this conversion.  If you convert
  from a gui position to a map position and back, you will probably not get
  the same value you started with.
****************************************************************************/
static void gui_to_map_pos(const struct tileset *t,
                           int *map_x, int *map_y, float gui_x, float gui_y)
{
  const float W = tileset_tile_width(t) * map_zoom, H = tileset_tile_height(t) * map_zoom;
  const float HH = tileset_hex_height(t) * map_zoom, HW = tileset_hex_width(t) * map_zoom;

  if (HH > 0 || HW > 0) {
    /* To handle hexagonal cases we have to revert to a less elegant method
     * of calculation. */
    float x, y;
    int dx, dy;
    int xmult, ymult, mod, compar;

    fc_assert(tileset_is_isometric(t));

    x = DIVIDE((int)gui_x, (int)W);
    y = DIVIDE((int)gui_y, (int)H);
    dx = gui_x - x * W;
    dy = gui_y - y * H;
    fc_assert(dx >= 0 && dx < W);
    fc_assert(dy >= 0 && dy < H);

    /* Now fold so we consider only one-quarter tile. */
    xmult = (dx >= W / 2) ? -1 : 1;
    ymult = (dy >= H / 2) ? -1 : 1;
    dx = (dx >= W / 2) ? (W - 1 - dx) : dx;
    dy = (dy >= H / 2) ? (H - 1 - dy) : dy;

    /* Next compare to see if we're across onto the next tile. */
    if (HW > 0) {
      compar = (dx - HW / 2) * (H / 2) - (H / 2 - 1 - dy) * (W / 2 - HW);
    } else {
      compar = (dy - HH / 2) * (W / 2) - (W / 2 - 1 - dx) * (H / 2 - HH);
    }
    mod = (compar < 0) ? -1 : 0;

    *map_x = (x + y) + mod * (xmult + ymult) / 2;
    *map_y = (y - x) + mod * (ymult - xmult) / 2;
  } else if (tileset_is_isometric(t)) {
    /* The basic operation here is a simple pi/4 rotation; however, we
     * have to first scale because the tiles have different width and
     * height.  Mathematically, this looks like
     *   | 1/W  1/H | |x|    |x`|
     *   |          | | | -> |  |
     *   |-1/W  1/H | |y|    |y`|
     *
     * Where W is the tile width and H the height.
     *
     * In simple terms, this is
     *   map_x = [   x / W + y / H ]
     *   map_y = [ - x / W + y / H ]
     * where [q] stands for integer part of q.
     *
     * Here the division is proper mathematical floating point division.
     *
     * A picture demonstrating this can be seen at
     * http://bugs.freeciv.org/Ticket/Attachment/16782/9982/grid1.png.
     *
     * We have to subtract off a half-tile in the X direction before doing
     * the transformation.  This is because, although the origin of the tile
     * is the top-left corner of the bounding box, after the transformation
     * the top corner of the diamond-shaped tile moves into this position.
     *
     * The calculation is complicated somewhat because of two things: we
     * only use integer math, and C integer division rounds toward zero
     * instead of rounding down.
     *
     * For another example of this math, see canvas_to_city_pos().
     */
    gui_x -= W / 2;
    *map_x = DIVIDE((int)(gui_x * H + gui_y * W), (int)(W * H));
    *map_y = DIVIDE((int)(gui_y * W - gui_x * H), (int)(W * H));
  } else {			/* tileset_is_isometric(t) */
    /* We use DIVIDE so that we will get the correct result even
     * for negative coordinates. */
    *map_x = DIVIDE((int)gui_x, (int)W);
    *map_y = DIVIDE((int)gui_y, (int)H);
  }
}

/************************************************************************//**
  Finds the canvas coordinates for a map position. Beside setting the results
  in canvas_x, canvas_y it returns whether the tile is inside the
  visible mapview canvas.

  The result represents the upper left pixel (origin) of the bounding box of
  the tile.  Note that in iso-view this origin is not a part of the tile
  itself - so to make the operation reversible you would have to call
  canvas_to_map_pos on the center of the tile, not the origin.

  The center of a tile is defined as:
  {
    tile_to_canvas_pos(&canvas_x, &canvas_y, ptile);
    canvas_x += tileset_tile_width(tileset) * map_zoom / 2;
    canvas_y += tileset_tile_height(tileset) * map_zoom / 2;
  }

  This pixel is one position closer to the lower right, which may be
  important to remember when doing some round-off operations. Other
  parts of the code assume tileset_tile_width(tileset) and tileset_tile_height(tileset)
  to be even numbers.
****************************************************************************/
bool tile_to_canvas_pos(float *canvas_x, float *canvas_y, struct tile *ptile)
{
  int center_map_x, center_map_y, dx, dy, tile_x, tile_y;

  /*
   * First we wrap the coordinates to hopefully be within the mapview
   * window.  We do this by finding the position closest to the center
   * of the window.
   */
  /* TODO: Cache the value of this position */
  base_canvas_to_map_pos(&center_map_x, &center_map_y,
			 mapview.width / 2,
			 mapview.height / 2);
  index_to_map_pos(&tile_x, &tile_y, tile_index(ptile));
  base_map_distance_vector(&dx, &dy, center_map_x, center_map_y, tile_x,
                           tile_y);

  map_to_gui_pos(tileset,
		 canvas_x, canvas_y, center_map_x + dx, center_map_y + dy);
  *canvas_x -= mapview.gui_x0;
  *canvas_y -= mapview.gui_y0;

  /*
   * Finally we clip.
   *
   * This check is tailored to work for both iso-view and classic view.  Note
   * that (canvas_x, canvas_y) need not be aligned to a tile boundary, and
   * that the position is at the top-left of the NORMAL (not UNIT) tile.
   * This checks to see if _any part_ of the tile is present on the backing
   * store.  Even if it's not visible on the canvas, if it's present on the
   * backing store we need to draw it in case the canvas is resized.
   */
  return (*canvas_x > -tileset_tile_width(tileset) * map_zoom
	  && *canvas_x < mapview.store_width
	  && *canvas_y > -tileset_tile_height(tileset) * map_zoom
	  && *canvas_y < (mapview.store_height
			  + (tileset_full_tile_height(tileset)
                             - tileset_tile_height(tileset)) * map_zoom));
}

/************************************************************************//**
  Finds the map coordinates corresponding to pixel coordinates.  The
  resulting position is unwrapped and may be unreal.
****************************************************************************/
static void base_canvas_to_map_pos(int *map_x, int *map_y,
                                   float canvas_x, float canvas_y)
{
  gui_to_map_pos(tileset, map_x, map_y,
                 canvas_x + mapview.gui_x0,
                 canvas_y + mapview.gui_y0);
}

/************************************************************************//**
  Finds the tile corresponding to pixel coordinates.  Returns that tile,
  or NULL if the position is off the map.
****************************************************************************/
struct tile *canvas_pos_to_tile(float canvas_x, float canvas_y)
{
  int map_x, map_y;

  base_canvas_to_map_pos(&map_x, &map_y, canvas_x, canvas_y);
  if (normalize_map_pos(&(wld.map), &map_x, &map_y)) {
    return map_pos_to_tile(&(wld.map), map_x, map_y);
  } else {
    return NULL;
  }
}

/************************************************************************//**
  Finds the tile corresponding to pixel coordinates.  Returns that tile,
  or the one nearest is the position is off the map.  Will never return NULL.
****************************************************************************/
struct tile *canvas_pos_to_nearest_tile(float canvas_x, float canvas_y)
{
  int map_x, map_y;

  base_canvas_to_map_pos(&map_x, &map_y, canvas_x, canvas_y);
  return nearest_real_tile(&(wld.map), map_x, map_y);
}

/************************************************************************//**
  Normalize (wrap) the GUI position.  This is equivalent to a map wrapping,
  but in GUI coordinates so that pixel accuracy is preserved.
****************************************************************************/
static void normalize_gui_pos(const struct tileset *t,
                              float *gui_x, float *gui_y)
{
  int map_x, map_y, nat_x, nat_y, diff_x, diff_y;
  float gui_x0, gui_y0;

  /* Convert the (gui_x, gui_y) into a (map_x, map_y) plus a GUI offset
   * from this tile. */
  gui_to_map_pos(t, &map_x, &map_y, *gui_x, *gui_y);
  map_to_gui_pos(t, &gui_x0, &gui_y0, map_x, map_y);
  diff_x = *gui_x - gui_x0;
  diff_y = *gui_y - gui_y0;

  /* Perform wrapping without any realness check.  It's important that
   * we wrap even if the map position is unreal, which normalize_map_pos
   * doesn't necessarily do. */
  MAP_TO_NATIVE_POS(&nat_x, &nat_y, map_x, map_y);
  if (current_topo_has_flag(TF_WRAPX)) {
    nat_x = FC_WRAP(nat_x, wld.map.xsize);
  }
  if (current_topo_has_flag(TF_WRAPY)) {
    nat_y = FC_WRAP(nat_y, wld.map.ysize);
  }
  NATIVE_TO_MAP_POS(&map_x, &map_y, nat_x, nat_y);

  /* Now convert the wrapped map position back to a GUI position and add the
   * offset back on. */
  map_to_gui_pos(t, gui_x, gui_y, map_x, map_y);
  *gui_x += diff_x;
  *gui_y += diff_y;
}

/************************************************************************//**
  Find the vector with minimum "real" distance between two GUI positions.
  This corresponds to map_to_distance_vector but works for GUI coordinates.
****************************************************************************/
static void gui_distance_vector(const struct tileset *t,
                                float *gui_dx, float *gui_dy,
                                float gui_x0, float gui_y0,
                                float gui_x1, float gui_y1)
{
  int map_x0, map_y0, map_x1, map_y1;
  float gui_x0_base, gui_y0_base, gui_x1_base, gui_y1_base;
  int gui_x0_diff, gui_y0_diff, gui_x1_diff, gui_y1_diff;
  int map_dx, map_dy;

  /* Make sure positions are canonical.  Yes, this is the only way. */
  normalize_gui_pos(t, &gui_x0, &gui_y0);
  normalize_gui_pos(t, &gui_x1, &gui_y1);

  /* Now we have to find the offset of each GUI position from its tile
   * origin.  This is complicated: it means converting to a map position and
   * then back to the GUI position to find the tile origin, then subtracting
   * to get the offset. */
  gui_to_map_pos(t, &map_x0, &map_y0, gui_x0, gui_y0);
  gui_to_map_pos(t, &map_x1, &map_y1, gui_x1, gui_y1);

  map_to_gui_pos(t, &gui_x0_base, &gui_y0_base, map_x0, map_y0);
  map_to_gui_pos(t, &gui_x1_base, &gui_y1_base, map_x1, map_y1);

  gui_x0_diff = gui_x0 - gui_x0_base;
  gui_y0_diff = gui_y0 - gui_y0_base;
  gui_x1_diff = gui_x1 - gui_x1_base;
  gui_y1_diff = gui_y1 - gui_y1_base;

  /* Next we find the map distance vector and convert this into a GUI
   * vector. */
  base_map_distance_vector(&map_dx, &map_dy, map_x0, map_y0, map_x1, map_y1);
  map_to_gui_pos(t, gui_dx, gui_dy, map_dx, map_dy);

  /* Finally we add on the difference in offsets to retain pixel
   * resolution. */
  *gui_dx += gui_x1_diff - gui_x0_diff;
  *gui_dy += gui_y1_diff - gui_y0_diff;
}

/************************************************************************//**
  Move the GUI origin to the given normalized, clipped origin.  This may
  be called many times when sliding the mapview.
****************************************************************************/
static void base_set_mapview_origin(float gui_x0, float gui_y0)
{
  float old_gui_x0, old_gui_y0;
  float dx, dy;
  const int width = mapview.width, height = mapview.height;
  int common_x0, common_x1, common_y0, common_y1;
  int update_x0, update_x1, update_y0, update_y1;

  /* Then update everything.  This does some tricky math to avoid having
   * to do unnecessary redraws in update_map_canvas.  This makes for ugly
   * code but speeds up the mapview by a large factor. */

  /* We need to calculate the vector of movement of the mapview.  So
   * we find the GUI distance vector and then use this to calculate
   * the original mapview origin relative to the current position.  Thus
   * if we move one tile to the left, even if this causes GUI positions
   * to wrap the distance vector is only one tile. */
  normalize_gui_pos(tileset, &gui_x0, &gui_y0);
  gui_distance_vector(tileset, &dx, &dy,
		      mapview.gui_x0, mapview.gui_y0,
		      gui_x0, gui_y0);
  old_gui_x0 = gui_x0 - dx;
  old_gui_y0 = gui_y0 - dy;

  mapview.gui_x0 = gui_x0;
  mapview.gui_y0 = gui_y0;

  /* Find the overlapping area of the new and old mapview.  This is
   * done in GUI coordinates.  Note that if the GUI coordinates wrap
   * no overlap will be found. */
  common_x0 = MAX(old_gui_x0, gui_x0);
  common_x1 = MIN(old_gui_x0, gui_x0) + width;
  common_y0 = MAX(old_gui_y0, gui_y0);
  common_y1 = MIN(old_gui_y0, gui_y0) + height;

  if (mapview.can_do_cached_drawing && !zoom_is_enabled()
      && common_x1 > common_x0 && common_y1 > common_y0) {
    /* Do a partial redraw only.  This means the area of overlap (a
     * rectangle) is copied.  Then the remaining areas (two rectangles)
     * are updated through update_map_canvas. */
    struct canvas *target = mapview.tmp_store;

    if (old_gui_x0 < gui_x0) {
      update_x0 = MAX(old_gui_x0 + width, gui_x0);
      update_x1 = gui_x0 + width;
    } else {
      update_x0 = gui_x0;
      update_x1 = MIN(old_gui_x0, gui_x0 + width);
    }
    if (old_gui_y0 < gui_y0) {
      update_y0 = MAX(old_gui_y0 + height, gui_y0);
      update_y1 = gui_y0 + height;
    } else {
      update_y0 = gui_y0;
      update_y1 = MIN(old_gui_y0, gui_y0 + height);
    }

    dirty_all();
    canvas_copy(target, mapview.store,
		common_x0 - old_gui_x0,
		common_y0 - old_gui_y0,
		common_x0 - gui_x0, common_y0 - gui_y0,
		common_x1 - common_x0, common_y1 - common_y0);
    mapview.tmp_store = mapview.store;
    mapview.store = target;

    if (update_y1 > update_y0) {
      update_map_canvas(0, update_y0 - gui_y0,
			width, update_y1 - update_y0);
    }
    if (update_x1 > update_x0) {
      update_map_canvas(update_x0 - gui_x0, common_y0 - gui_y0,
			update_x1 - update_x0, common_y1 - common_y0);
    }
  } else {
    dirty_all();
    update_map_canvas(0, 0, mapview.store_width, mapview.store_height);
  }

  center_tile_overviewcanvas();
  switch (hover_state) {
  case HOVER_GOTO:
  case HOVER_PATROL:
  case HOVER_CONNECT:
    create_line_at_mouse_pos();
  case HOVER_NONE:
  case HOVER_PARADROP:
  case HOVER_ACT_SEL_TGT:
    break;
  };
  if (rectangle_active) {
    update_rect_at_mouse_pos();
  }
}

/************************************************************************//**
  Adjust mapview origin values. Returns TRUE iff values are different from
  current mapview.
****************************************************************************/
static bool calc_mapview_origin(float *gui_x0, float *gui_y0)
{
  float xmin, ymin, xmax, ymax;
  int xsize, ysize;

  /* Normalize (wrap) the mapview origin. */
  normalize_gui_pos(tileset, gui_x0, gui_y0);

  /* First wrap/clip the position.  Wrapping is done in native positions
   * while clipping is done in scroll (native) positions. */
  get_mapview_scroll_window(&xmin, &ymin, &xmax, &ymax, &xsize, &ysize);

  if (!current_topo_has_flag(TF_WRAPX)) {
    *gui_x0 = CLIP(xmin, *gui_x0, xmax - xsize);
  }

  if (!current_topo_has_flag(TF_WRAPY)) {
    *gui_y0 = CLIP(ymin, *gui_y0, ymax - ysize);
  }

  if (mapview.gui_x0 == *gui_x0 && mapview.gui_y0 == *gui_y0) {
    return FALSE;
  }

  return TRUE;
}

/************************************************************************//**
  Change the mapview origin, clip it, and update everything.
****************************************************************************/
void set_mapview_origin(float gui_x0, float gui_y0)
{
  if (!calc_mapview_origin(&gui_x0, &gui_y0)) {
    return;
  }

  if (can_slide && gui_options.smooth_center_slide_msec > 0) {
    if (frame_by_frame_animation) {
      /* TODO: Implement animation */
      base_set_mapview_origin(gui_x0, gui_y0);
    } else {
      int start_x = mapview.gui_x0, start_y = mapview.gui_y0;
      float diff_x, diff_y;
      double timing_sec = (double)gui_options.smooth_center_slide_msec / 1000.0;
      double currtime;
      int frames = 0;

      /* We track the average FPS, which is used to predict how long the
       * next draw will take.  We start with a 100 FPS estimate - this
       * value will quickly become irrelevant as the correct value is
       * calculated, but it's needed to give an estimate of the FPS for
       * the first draw.
       *
       * Note that the initial value shouldn't be larger than the sliding
       * time, or we'll jump straight to the last frame.  The FPS should
       * therefore be a "high" estimate. */
      static double total_frames = 0.01;
      static double total_time = 0.0001;

      gui_distance_vector(tileset,
                          &diff_x, &diff_y, start_x, start_y, gui_x0, gui_y0);
      anim_timer = timer_renew(anim_timer, TIMER_USER, TIMER_ACTIVE);
      timer_start(anim_timer);

      unqueue_mapview_updates(TRUE);

      do {
        double mytime;

        /* Get the current time, and add on the average 1/FPS, which is the
         * expected time this frame will take.  This is done so that the
         * frame's position is calculated from the expected time when the
         * frame will complete, rather than the time when the frame drawing
         * is started. */
        currtime = timer_read_seconds(anim_timer);
        currtime += total_time / total_frames;

        mytime = MIN(currtime, timing_sec);
        base_set_mapview_origin(start_x + diff_x * (mytime / timing_sec),
                                start_y + diff_y * (mytime / timing_sec));
        flush_dirty();
        gui_flush();
        frames++;
      } while (currtime < timing_sec);

      currtime = timer_read_seconds(anim_timer);
      total_frames += frames;
      total_time += currtime;
      log_debug("Got %d frames in %f seconds: %f FPS (avg %f).",
                frames, currtime, (double)frames / currtime,
                total_frames / total_time);

      /* A very small decay factor to make things more accurate when something
       * changes (mapview size, tileset change, etc.).  This gives a
       * half-life of 68 slides. */
      total_frames *= 0.99;
      total_time *= 0.99;
    }
  } else {
    base_set_mapview_origin(gui_x0, gui_y0);
  }

  update_map_canvas_scrollbars();
}

/************************************************************************//**
  Return the scroll dimensions of the clipping window for the mapview window..

  Imagine the entire map in scroll coordinates.  It is a rectangle.  Now
  imagine the mapview "window" sliding around through this rectangle.  How
  far can it slide?  In most cases it has to be able to slide past the
  ends of the map rectangle so that it's capable of reaching the whole
  area.

  This function gives constraints on how far the window is allowed to
  slide.  xmin and ymin are the minimum values for the window origin.
  xsize and ysize give the scroll dimensions of the mapview window.
  xmax and ymax give the maximum values that the bottom/left ends of the
  window may reach.  The constraints, therefore, are that:

    get_mapview_scroll_pos(&scroll_x, &scroll_y);
    xmin <= scroll_x < xmax - xsize
    ymin <= scroll_y < ymax - ysize

  This function should be used anywhere and everywhere that scrolling is
  constrained.

  Note that scroll coordinates, not map coordinates, are used.  Currently
  these correspond to native coordinates.
****************************************************************************/
void get_mapview_scroll_window(float *xmin, float *ymin,
                               float *xmax, float *ymax,
                               int *xsize, int *ysize)
{
  int diff;

  *xsize = mapview.width;
  *ysize = mapview.height;

  if (MAP_IS_ISOMETRIC == tileset_is_isometric(tileset)) {
    /* If the map and view line up, it's easy. */
    NATIVE_TO_MAP_POS(xmin, ymin, 0, 0);
    map_to_gui_pos(tileset, xmin, ymin, *xmin, *ymin);

    NATIVE_TO_MAP_POS(xmax, ymax, wld.map.xsize - 1, wld.map.ysize - 1);
    map_to_gui_pos(tileset, xmax, ymax, *xmax, *ymax);
    *xmax += tileset_tile_width(tileset) * map_zoom;
    *ymax += tileset_tile_height(tileset) * map_zoom;

    /* To be able to center on positions near the edges, we have to be
     * allowed to scroll all the way to those edges.  To allow wrapping the
     * clipping boundary needs to extend past the edge - a half-tile in
     * iso-view or a full tile in non-iso view.  The above math already has
     * taken care of some of this so all that's left is to fix the corner
     * cases. */
    if (current_topo_has_flag(TF_WRAPX)) {
      *xmax += *xsize;

      /* We need to be able to scroll a little further to the left. */
      *xmin -= tileset_tile_width(tileset) * map_zoom;
    }
    if (current_topo_has_flag(TF_WRAPY)) {
      *ymax += *ysize;

      /* We need to be able to scroll a little further up. */
      *ymin -= tileset_tile_height(tileset) * map_zoom;
    }
  } else {
    /* Otherwise it's hard.  Very hard.  Impossible, in fact.  This is just
     * an approximation - a huge bounding box. */
    float gui_x1, gui_y1, gui_x2, gui_y2, gui_x3, gui_y3, gui_x4, gui_y4;
    int map_x, map_y;

    NATIVE_TO_MAP_POS(&map_x, &map_y, 0, 0);
    map_to_gui_pos(tileset, &gui_x1, &gui_y1, map_x, map_y);

    NATIVE_TO_MAP_POS(&map_x, &map_y, wld.map.xsize - 1, 0);
    map_to_gui_pos(tileset, &gui_x2, &gui_y2, map_x, map_y);

    NATIVE_TO_MAP_POS(&map_x, &map_y, 0, wld.map.ysize - 1);
    map_to_gui_pos(tileset, &gui_x3, &gui_y3, map_x, map_y);

    NATIVE_TO_MAP_POS(&map_x, &map_y, wld.map.xsize - 1, wld.map.ysize - 1);
    map_to_gui_pos(tileset, &gui_x4, &gui_y4, map_x, map_y);

    *xmin = MIN(gui_x1, MIN(gui_x2, gui_x3)) - mapview.width / 2;
    *ymin = MIN(gui_y1, MIN(gui_y2, gui_y3)) - mapview.height / 2;

    *xmax = MAX(gui_x4, MAX(gui_x2, gui_x3)) + mapview.width / 2;
    *ymax = MAX(gui_y4, MAX(gui_y2, gui_y3)) + mapview.height / 2;
  }

  /* Make sure the scroll window is big enough to hold the mapview.  If
   * not scrolling will be very ugly and the GUI may become confused. */
  diff = *xsize - (*xmax - *xmin);
  if (diff > 0) {
    *xmin -= diff / 2;
    *xmax += (diff + 1) / 2;
  }

  diff = *ysize - (*ymax - *ymin);
  if (diff > 0) {
    *ymin -= diff / 2;
    *ymax += (diff + 1) / 2;
  }

  log_debug("x: %f<-%d->%f; y: %f<-%f->%d",
            *xmin, *xsize, *xmax, *ymin, *ymax, *ysize);
}

/************************************************************************//**
  Find the scroll step for the mapview.  This is the amount to scroll (in
  scroll coordinates) on each "step".  See also get_mapview_scroll_window.
****************************************************************************/
void get_mapview_scroll_step(int *xstep, int *ystep)
{
  *xstep = tileset_tile_width(tileset) * map_zoom;
  *ystep = tileset_tile_height(tileset) * map_zoom;

  if (tileset_is_isometric(tileset)) {
    *xstep /= 2;
    *ystep /= 2;
  }
}

/************************************************************************//**
  Find the current scroll position (origin) of the mapview.
****************************************************************************/
void get_mapview_scroll_pos(int *scroll_x, int *scroll_y)
{
  *scroll_x = mapview.gui_x0;
  *scroll_y = mapview.gui_y0;
}

/************************************************************************//**
  Set the scroll position (origin) of the mapview, and update the GUI.
****************************************************************************/
void set_mapview_scroll_pos(int scroll_x, int scroll_y)
{
  int gui_x0 = scroll_x, gui_y0 = scroll_y;

  can_slide = FALSE;
  set_mapview_origin(gui_x0, gui_y0);
  can_slide = TRUE;
}

/************************************************************************//**
  Finds the current center tile of the mapcanvas.
****************************************************************************/
struct tile *get_center_tile_mapcanvas(void)
{
  return canvas_pos_to_nearest_tile(mapview.width / 2,
                                    mapview.height / 2);
}

/************************************************************************//**
  Centers the mapview around (map_x, map_y).
****************************************************************************/
void center_tile_mapcanvas(struct tile *ptile)
{
  float gui_x, gui_y;
  int tile_x, tile_y;
  static bool first = TRUE;

  if (first && can_slide) {
    return;
  }
  first = FALSE;

  index_to_map_pos(&tile_x, &tile_y, tile_index(ptile));
  map_to_gui_pos(tileset, &gui_x, &gui_y, tile_x, tile_y);

  /* Put the center pixel of the tile at the exact center of the mapview. */
  gui_x -= (mapview.width - tileset_tile_width(tileset) * map_zoom) / 2;
  gui_y -= (mapview.height - tileset_tile_height(tileset) * map_zoom) / 2;

  set_mapview_origin(gui_x, gui_y);

  center_tile = ptile;
}

/************************************************************************//**
  Return TRUE iff the given map position has a tile visible on the
  map canvas.
****************************************************************************/
bool tile_visible_mapcanvas(struct tile *ptile)
{
  float dummy_x, dummy_y; /* well, it needs two pointers... */

  return tile_to_canvas_pos(&dummy_x, &dummy_y, ptile);
}

/************************************************************************//**
  Return TRUE iff the given map position has a tile visible within the
  interior of the map canvas. This information is used to determine
  when we need to recenter the map canvas.

  The logic of this function is simple: if a tile is within 1.5 tiles
  of a border of the canvas and that border is not aligned with the
  edge of the map, then the tile is on the "border" of the map canvas.

  This function is only correct for the current topology.
****************************************************************************/
bool tile_visible_and_not_on_border_mapcanvas(struct tile *ptile)
{
  float canvas_x, canvas_y;
  float xmin, ymin, xmax, ymax;
  int xsize, ysize, scroll_x, scroll_y;
  const int border_x = (tileset_is_isometric(tileset) ? tileset_tile_width(tileset) / 2 * map_zoom
			: 2 * tileset_tile_width(tileset) * map_zoom);
  const int border_y = (tileset_is_isometric(tileset) ? tileset_tile_height(tileset) / 2 * map_zoom
			: 2 * tileset_tile_height(tileset) * map_zoom);
  bool same = (tileset_is_isometric(tileset) == MAP_IS_ISOMETRIC);

  get_mapview_scroll_window(&xmin, &ymin, &xmax, &ymax, &xsize, &ysize);
  get_mapview_scroll_pos(&scroll_x, &scroll_y);

  if (!tile_to_canvas_pos(&canvas_x, &canvas_y, ptile)) {
    /* The tile isn't visible at all. */
    return FALSE;
  }

  /* For each direction: if the tile is too close to the mapview border
   * in that direction, and scrolling can get us any closer to the
   * border, then it's a border tile.  We can only really check the
   * scrolling when the mapview window lines up with the map. */
  if (canvas_x < border_x
      && (!same || scroll_x > xmin || current_topo_has_flag(TF_WRAPX))) {
    return FALSE;
  }
  if (canvas_y < border_y
      && (!same || scroll_y > ymin || current_topo_has_flag(TF_WRAPY))) {
    return FALSE;
  }
  if (canvas_x + tileset_tile_width(tileset) * map_zoom > mapview.width - border_x
      && (!same || scroll_x + xsize < xmax || current_topo_has_flag(TF_WRAPX))) {
    return FALSE;
  }
  if (canvas_y + tileset_tile_height(tileset) * map_zoom > mapview.height - border_y
      && (!same || scroll_y + ysize < ymax || current_topo_has_flag(TF_WRAPY))) {
    return FALSE;
  }

  return TRUE;
}

/************************************************************************//**
  Draw an array of drawn sprites onto the canvas.
****************************************************************************/
void put_drawn_sprites(struct canvas *pcanvas, float zoom,
                       int canvas_x, int canvas_y,
                       int count, struct drawn_sprite *pdrawn,
                       bool fog)
{
  int i;

  for (i = 0; i < count; i++) {
    if (!pdrawn[i].sprite) {
      /* This can happen, although it should probably be avoided. */
      continue;
    }

    if (fog && pdrawn[i].foggable) {
      canvas_put_sprite_fogged(pcanvas,
                               canvas_x / zoom + pdrawn[i].offset_x,
                               canvas_y / zoom + pdrawn[i].offset_y,
			       pdrawn[i].sprite,
			       TRUE,
			       canvas_x, canvas_y);
    } else {
      /* We avoid calling canvas_put_sprite_fogged, even though it
       * should be a valid thing to do, because gui-gtk-2.0 doesn't have
       * a full implementation. */
      canvas_put_sprite_full(pcanvas,
                             canvas_x / zoom + pdrawn[i].offset_x,
                             canvas_y / zoom + pdrawn[i].offset_y,
			     pdrawn[i].sprite);
    }
  }
}

/************************************************************************//**
  Draw one layer of a tile, edge, corner, unit, and/or city onto the
  canvas at the given position.
****************************************************************************/
void put_one_element(struct canvas *pcanvas, float zoom,
                     enum mapview_layer layer,
                     const struct tile *ptile,
                     const struct tile_edge *pedge,
                     const struct tile_corner *pcorner,
                     const struct unit *punit, const struct city *pcity,
                     int canvas_x, int canvas_y,
                     const struct city *citymode,
                     const struct unit_type *putype)
{
  struct drawn_sprite tile_sprs[80];
  int count = fill_sprite_array(tileset, tile_sprs, layer,
                                ptile, pedge, pcorner,
                                punit, pcity, citymode, putype);
  bool fog = (ptile && gui_options.draw_fog_of_war
	      && TILE_KNOWN_UNSEEN == client_tile_get_known(ptile));

  /*** Draw terrain and specials ***/
  put_drawn_sprites(pcanvas, zoom, canvas_x, canvas_y, count, tile_sprs, fog);
}

/************************************************************************//**
  Draw the given unit onto the canvas store at the given location. The area
  of drawing is tileset_unit_height(tileset) x tileset_unit_width(tileset).
****************************************************************************/
void put_unit(const struct unit *punit, struct canvas *pcanvas, float zoom,
              int canvas_x, int canvas_y)
{
  canvas_y += (tileset_unit_height(tileset) - tileset_tile_height(tileset)) * zoom;
  mapview_layer_iterate(layer) {
    put_one_element(pcanvas, zoom, layer, NULL, NULL, NULL,
                    punit, NULL, canvas_x, canvas_y, NULL, NULL);
  } mapview_layer_iterate_end;
}

/************************************************************************//**
  Draw the given unit onto the canvas store at the given location. The area
  of drawing is tileset_unit_height(tileset) x tileset_unit_width(tileset).
****************************************************************************/
void put_unittype(const struct unit_type *putype, struct canvas *pcanvas, float zoom,
                  int canvas_x, int canvas_y)
{
  canvas_y += (tileset_unit_height(tileset) - tileset_tile_height(tileset)) * zoom;
  mapview_layer_iterate(layer) {
    put_one_element(pcanvas, zoom, layer, NULL, NULL, NULL,
                    NULL, NULL, canvas_x, canvas_y, NULL, putype);
  } mapview_layer_iterate_end;
}

/************************************************************************//**
  Draw the given city onto the canvas store at the given location.  The
  area of drawing is
  tileset_full_tile_height(tileset) x tileset_full_tile_width(tileset).
****************************************************************************/
void put_city(struct city *pcity, struct canvas *pcanvas, float zoom,
              int canvas_x, int canvas_y)
{
  canvas_y += (tileset_full_tile_height(tileset) - tileset_tile_height(tileset)) * zoom;
  mapview_layer_iterate(layer) {
    put_one_element(pcanvas, zoom, layer,
		    NULL, NULL, NULL, NULL, pcity,
		    canvas_x, canvas_y, NULL, NULL);
  } mapview_layer_iterate_end;
}

/************************************************************************//**
  Draw the given tile terrain onto the canvas store at the given location.
  The area of drawing is
  tileset_full_tile_height(tileset) x tileset_full_tile_width(tileset)
  (even though most tiles are not this tall).
****************************************************************************/
void put_terrain(struct tile *ptile, struct canvas *pcanvas, float zoom,
                 int canvas_x, int canvas_y)
{
  /* Use full tile height, even for terrains. */
  canvas_y += (tileset_full_tile_height(tileset) - tileset_tile_height(tileset)) * zoom;
  mapview_layer_iterate(layer) {
    put_one_element(pcanvas, zoom, layer, ptile, NULL, NULL, NULL, NULL,
		    canvas_x, canvas_y, NULL, NULL);
  } mapview_layer_iterate_end;
}

/************************************************************************//**
  Draw food, gold, and shield upkeep values on the unit.

  The proper way to do this is probably something like what Civ II does
  (one sprite drawn N times on top of itself), but we just use separate
  sprites (limiting the number of combinations).
****************************************************************************/
void put_unit_city_overlays(struct unit *punit,
                            struct canvas *pcanvas,
                            int canvas_x, int canvas_y, int *upkeep_cost,
                            int happy_cost)
{
  struct sprite *sprite;

  sprite = get_unit_unhappy_sprite(tileset, punit, happy_cost);
  if (sprite) {
    canvas_put_sprite_full(pcanvas, canvas_x, canvas_y, sprite);
  }

  output_type_iterate(o) {
    sprite = get_unit_upkeep_sprite(tileset, o, punit, upkeep_cost);
    if (sprite) {
      canvas_put_sprite_full(pcanvas, canvas_x, canvas_y, sprite);
    }
  } output_type_iterate_end;
}

/*
 * pcity->client.color_index is an index into the city_colors array.
 * When toggle_city_color is called the city's coloration is toggled.  When
 * a city is newly colored its color is taken from color_index and
 * color_index is moved forward one position. Each color in the array
 * tells what color the citymap will be drawn on the mapview.
 *
 * This array can be added to without breaking anything elsewhere.
 */
static int color_index = 0;
#define NUM_CITY_COLORS tileset_num_city_colors(tileset)


/************************************************************************//**
  Toggle the city color.  This cycles through the possible colors for the
  citymap as shown on the mapview.  These colors are listed in the
  city_colors array; above.
****************************************************************************/
void toggle_city_color(struct city *pcity)
{
  if (pcity->client.colored) {
    pcity->client.colored = FALSE;
  } else {
    pcity->client.colored = TRUE;
    pcity->client.color_index = color_index;
    color_index = (color_index + 1) % NUM_CITY_COLORS;
  }

  refresh_city_mapcanvas(pcity, pcity->tile, TRUE, FALSE);
}

/************************************************************************//**
  Toggle the unit color.  This cycles through the possible colors for the
  citymap as shown on the mapview.  These colors are listed in the
  city_colors array; above.
****************************************************************************/
void toggle_unit_color(struct unit *punit)
{
  if (punit->client.colored) {
    punit->client.colored = FALSE;
  } else {
    punit->client.colored = TRUE;
    punit->client.color_index = color_index;
    color_index = (color_index + 1) % NUM_CITY_COLORS;
  }

  refresh_unit_mapcanvas(punit, unit_tile(punit), TRUE, FALSE);
}

/************************************************************************//**
  Animate the nuke explosion at map(x, y).
****************************************************************************/
void put_nuke_mushroom_pixmaps(struct tile *ptile)
{
  float canvas_x, canvas_y;
  struct sprite *mysprite = get_nuke_explode_sprite(tileset);
  int width, height;

  get_sprite_dimensions(mysprite, &width, &height);

  if (frame_by_frame_animation) {
    struct animation *anim = fc_malloc(sizeof(struct animation));

    anim->type = ANIM_NUKE;
    anim->id = -1;
    anim->nuke.shown = FALSE;
    anim->nuke.nuke_tile = ptile;

    (void) tile_to_canvas_pos(&canvas_x, &canvas_y, ptile);
    get_sprite_dimensions(mysprite, &width, &height);

    anim->width = width * map_zoom;
    anim->height = height * map_zoom;
    animation_add(anim);
  } else {
    /* We can't count on the return value of tile_to_canvas_pos since the
     * sprite may span multiple tiles. */
    (void) tile_to_canvas_pos(&canvas_x, &canvas_y, ptile);

    canvas_x += (tileset_tile_width(tileset) - width) / 2 * map_zoom;
    canvas_y += (tileset_tile_height(tileset) - height) / 2 * map_zoom;

    /* Make sure everything is flushed and synced before proceeding.  First
     * we update everything to the store, but don't write this to screen.
     * Then add the nuke graphic to the store.  Finally flush everything to
     * the screen and wait 1 second. */
    unqueue_mapview_updates(FALSE);

    canvas_put_sprite_full(mapview.store, canvas_x, canvas_y, mysprite);
    dirty_rect(canvas_x, canvas_y, width, height);

    flush_dirty();
    gui_flush();

    fc_usleep(1000000);

    update_map_canvas_visible();
  }
}

/************************************************************************//**
  Draw some or all of a tile onto the canvas.
****************************************************************************/
static void put_one_tile(struct canvas *pcanvas, enum mapview_layer layer,
                         struct tile *ptile, int canvas_x, int canvas_y,
                         const struct city *citymode)
{
  if (client_tile_get_known(ptile) != TILE_UNKNOWN
      || (editor_is_active() && editor_tile_is_selected(ptile))) {
    struct unit *punit = get_drawable_unit(tileset, ptile, citymode);
    struct animation *anim = NULL;

    if (animation_list_size(animations) > 0) {
      anim = animation_list_get(animations, 0);
    }

    if (anim != NULL && punit != NULL
        && punit->id == anim->id) {
      punit = NULL;
    }

    put_one_element(pcanvas, map_zoom, layer, ptile, NULL, NULL, punit,
                    tile_city(ptile), canvas_x, canvas_y, citymode, NULL);
  }
}

/************************************************************************//**
  Depending on where ptile1 and ptile2 are on the map canvas, a trade route
  line may need to be drawn as two disjointed line segments. This function
  fills the given line array 'lines' with the necessary line segments.

  The return value is the number of line segments that need to be drawn.

  NB: It is assumed ptile1 and ptile2 are already consistently ordered.
  NB: 'lines' must be able to hold least MAX_TRADE_ROUTE_DRAW_LINES
  elements.
****************************************************************************/
static int trade_route_to_canvas_lines(const struct tile *ptile1,
                                       const struct tile *ptile2,
                                       struct trade_route_line *lines)
{
  int dx, dy;

  if (!ptile1 || !ptile2 || !lines) {
    return 0;
  }

  base_map_distance_vector(&dx, &dy, TILE_XY(ptile1), TILE_XY(ptile2));
  map_to_gui_pos(tileset, &lines[0].width, &lines[0].height, dx, dy);

  /* FIXME: Remove these casts. */
  tile_to_canvas_pos(&lines[0].x, &lines[0].y, (struct tile *)ptile1);
  tile_to_canvas_pos(&lines[1].x, &lines[1].y, (struct tile *)ptile2);

  if (lines[1].x - lines[0].x == lines[0].width
      && lines[1].y - lines[0].y == lines[0].height) {
    return 1;
  }

  lines[1].width = -lines[0].width;
  lines[1].height = -lines[0].height;
  return 2;
}

/************************************************************************//**
  Draw a colored trade route line from one tile to another.
****************************************************************************/
static void draw_trade_route_line(const struct tile *ptile1,
                                  const struct tile *ptile2,
                                  enum color_std color)
{
  struct trade_route_line lines[MAX_TRADE_ROUTE_DRAW_LINES];
  int line_count, i;
  struct color *pcolor;

  if (!ptile1 || !ptile2) {
    return;
  }

  pcolor = get_color(tileset, color);
  if (!pcolor) {
    return;
  }

  /* Order the source and destination tiles consistently
   * so that if a line is drawn twice it does not produce
   * ugly effects due to dashes not lining up. */
  if (tile_index(ptile2) > tile_index(ptile1)) {
    const struct tile *tmp;
    tmp = ptile1;
    ptile1 = ptile2;
    ptile2 = tmp;
  }

  line_count = trade_route_to_canvas_lines(ptile1, ptile2, lines);
  for (i = 0; i < line_count; i++) {
    /* XXX: canvas_put_line doesn't currently take map_zoom into account
     * itself, but it probably should? */
    canvas_put_line(mapview.store, pcolor, LINE_BORDER,
                    lines[i].x + tileset_tile_width(tileset) / 2 * map_zoom,
                    lines[i].y + tileset_tile_height(tileset) / 2 * map_zoom,
                    lines[i].width, lines[i].height);
  }
}

/************************************************************************//**
  Draw all trade routes for the given city.
****************************************************************************/
static void draw_trade_routes_for_city(const struct city *pcity_src)
{
  if (!pcity_src) {
    return;
  }

  trade_partners_iterate(pcity_src, pcity_dest) {
    draw_trade_route_line(city_tile(pcity_src), city_tile(pcity_dest),
                          COLOR_MAPVIEW_TRADE_ROUTE_LINE);
  } trade_partners_iterate_end;
}

/************************************************************************//**
  Draw trade routes between cities as lines on the main map canvas.
****************************************************************************/
static void draw_trade_routes(void)
{
  if (!gui_options.draw_city_trade_routes) {
    return;
  }

  if (client_is_global_observer()) {
    cities_iterate(pcity) {
      draw_trade_routes_for_city(pcity);
    } cities_iterate_end;
  } else {
    struct player *pplayer = client_player();
    if (!pplayer) {
      return;
    }
    city_list_iterate(pplayer->cities, pcity) {
      draw_trade_routes_for_city(pcity);
    } city_list_iterate_end;
  }
}

/************************************************************************//**
  Update (refresh) the map canvas starting at the given tile (in map
  coordinates) and with the given dimensions (also in map coordinates).

  In non-iso view, this is easy.  In iso view, we have to use the
  Painter's Algorithm to draw the tiles in back first.  When we draw
  a tile, we tell the GUI which part of the tile to draw - which is
  necessary unless we have an extra buffering step.

  After refreshing the backing store tile-by-tile, we write the store
  out to the display if write_to_screen is specified.

  x, y, width, and height are in map coordinates; they need not be
  normalized or even real.
****************************************************************************/
void update_map_canvas(int canvas_x, int canvas_y, int width, int height)
{
  int gui_x0, gui_y0;
  bool full;
  struct canvas *tmp;

  canvas_x = MAX(canvas_x, 0);
  canvas_y = MAX(canvas_y, 0);
  width = MIN(mapview.store_width - canvas_x, width);
  height = MIN(mapview.store_height - canvas_y, height);

  gui_x0 = mapview.gui_x0 + canvas_x;
  gui_y0 = mapview.gui_y0 + canvas_y;
  full = (canvas_x == 0 && canvas_y == 0
	  && width == mapview.store_width
	  && height == mapview.store_height);

  log_debug("update_map_canvas(pos=(%d,%d), size=(%d,%d))",
            canvas_x, canvas_y, width, height);

  /* If a full redraw is done, we just draw everything onto the canvas.
   * However if a partial redraw is done we draw everything onto the
   * tmp_canvas then copy *just* the area of update onto the canvas. */
  if (!full) {
    /* Swap store and tmp_store. */
    tmp = mapview.store;
    mapview.store = mapview.tmp_store;
    mapview.tmp_store = tmp;
  }

  /* Clear the area.  This is necessary since some parts of the rectangle
   * may not actually have any tiles drawn on them.  This will happen when
   * the mapview is large enough so that the tile is visible in multiple
   * locations.  In this case it will only be drawn in one place.
   *
   * Of course it's necessary to draw to the whole area to cover up any old
   * drawing that was done there. */
  canvas_put_rectangle(mapview.store,
		       get_color(tileset, COLOR_MAPVIEW_UNKNOWN),
		       canvas_x, canvas_y, width / map_zoom, height / map_zoom);

  mapview_layer_iterate(layer) {
    if (layer == LAYER_TILELABEL) {
      show_tile_labels(canvas_x, canvas_y, width, height);
    }
    if (layer == LAYER_CITYBAR) {
      show_city_descriptions(canvas_x, canvas_y, width, height);
      continue;
    }
    gui_rect_iterate_coord(gui_x0, gui_y0, width,
			   height + (tileset_is_isometric(tileset)
				     ? (tileset_tile_height(tileset) / 2 * map_zoom) : 0),
			   ptile, pedge, pcorner, gui_x, gui_y, map_zoom) {
      const int cx = gui_x - mapview.gui_x0, cy = gui_y - mapview.gui_y0;

      if (ptile) {
	put_one_tile(mapview.store, layer, ptile, cx, cy, NULL);
      } else if (pedge) {
        put_one_element(mapview.store, map_zoom, layer, NULL, pedge, NULL,
                        NULL, NULL, cx, cy, NULL, NULL);
      } else if (pcorner) {
        put_one_element(mapview.store, map_zoom, layer, NULL, NULL, pcorner,
                        NULL, NULL, cx, cy, NULL, NULL);
      } else {
	/* This can happen, for instance for unreal tiles. */
      }
    } gui_rect_iterate_coord_end;
  } mapview_layer_iterate_end;

  draw_trade_routes();
  link_marks_draw_all();

  /* Draw the goto lines on top of the whole thing. This is done last as
   * we want it completely on top.
   *
   * Note that a pixel right on the border of a tile may actually contain a
   * goto line from an adjacent tile.  Thus we draw any extra goto lines
   * from adjacent tiles (if they're close enough). */
  gui_rect_iterate(gui_x0 - GOTO_WIDTH, gui_y0 - GOTO_WIDTH,
		   width + 2 * GOTO_WIDTH, height + 2 * GOTO_WIDTH,
		   ptile, pedge, pcorner, map_zoom) {
    if (!ptile) {
      continue;
    }
    adjc_dir_base_iterate(&(wld.map), ptile, dir) {
      if (mapdeco_is_gotoline_set(ptile, dir)) {
        draw_segment(ptile, dir);
      }
    } adjc_dir_base_iterate_end;
  } gui_rect_iterate_end;

  if (!full) {
    /* Swap store and tmp_store back. */
    tmp = mapview.store;
    mapview.store = mapview.tmp_store;
    mapview.tmp_store = tmp;

    /* And copy store to tmp_store. */
    canvas_copy(mapview.store, mapview.tmp_store,
		canvas_x, canvas_y, canvas_x, canvas_y, width, height);
  }

  dirty_rect(canvas_x, canvas_y, width, height);
}

/************************************************************************//**
  Update (only) the visible part of the map
****************************************************************************/
void update_map_canvas_visible(void)
{
  queue_mapview_update(UPDATE_MAP_CANVAS_VISIBLE);
}

/* The maximum city description width and height.  This gives the dimensions
 * of a rectangle centered directly beneath the tile a city is on, that
 * contains the city description.
 *
 * These values are increased when drawing is done.  This may mean that
 * the change (from increasing the value) won't take place until the
 * next redraw. */
static int max_desc_width = 0, max_desc_height = 0;

/* Same for tile labels */
static int max_label_width = 0, max_label_height = 0 ;

/************************************************************************//**
  Update the city description for the given city.
****************************************************************************/
void update_city_description(struct city *pcity)
{
  queue_mapview_tile_update(pcity->tile, TILE_UPDATE_CITY_DESC);
}

/************************************************************************//**
  Update the label for the given tile
****************************************************************************/
void update_tile_label(struct tile *ptile)
{
  queue_mapview_tile_update(ptile, TILE_UPDATE_TILE_LABEL);
}

/************************************************************************//**
  Draw a "full" city bar for the city.  This is a subcase of show_city_desc
  (see that function for more info) for tilesets that have a full city bar.
****************************************************************************/
static void show_full_citybar(struct canvas *pcanvas,
                              const int canvas_x0, const int canvas_y0,
                              struct city *pcity, int *width, int *height)
{
  const struct citybar_sprites *citybar = get_citybar_sprites(tileset);
  static char name[512], growth[32], prod[512], size[32], trade_routes[32];
  enum color_std growth_color;
  enum color_std production_color;
  /* trade_routes_color initialized just to get rid of gcc warning
   * on optimization level 3 when it misdiagnoses that it would be used
   * uninitialized otherwise. Funny thing here is that warning would
   * go away also by *not* setting it to values other than
   * COLOR_MAPVIEW_CITYTEXT in get_city_mapview_trade_routes() */
  enum color_std trade_routes_color = COLOR_MAPVIEW_CITYTEXT;
  struct color *owner_color;
  struct {
    int x, y, w, h;
  } name_rect = {0, 0, 0, 0}, growth_rect = {0, 0, 0, 0},
    prod_rect = {0, 0, 0, 0}, size_rect = {0, 0, 0, 0},
    flag_rect = {0, 0, 0, 0}, occupy_rect = {0, 0, 0, 0},
    food_rect = {0, 0, 0, 0}, shield_rect = {0, 0, 0, 0},
    trade_routes_rect = {0,}, trade_rect = {0,};
  int width1 = 0, width2 = 0, height1 = 0, height2 = 0;
  struct sprite *bg = citybar->background;
  struct sprite *flag = get_city_flag_sprite(tileset, pcity);
  struct sprite *occupy = NULL;
  int bg_w, bg_h, x, y;
  const int canvas_x = canvas_x0 + tileset_tile_width(tileset) / 2 * map_zoom;
  const int canvas_y = canvas_y0 + tileset_citybar_offset_y(tileset) * map_zoom;
  const int border = 6;
  const enum client_font FONT_CITY_SIZE = FONT_CITY_NAME; /* TODO: new font */

  /* We can see the city's production or growth values if
   * we are observing or playing as the owner of the city. */
  const bool can_see_inside
    = (client_is_global_observer() || city_owner(pcity) == client_player());
  const bool should_draw_productions
    = can_see_inside && gui_options.draw_city_productions;
  const bool should_draw_growth = can_see_inside && gui_options.draw_city_growth;
  const bool should_draw_trade_routes = can_see_inside
    && gui_options.draw_city_trade_routes;
  const bool should_draw_lower_bar
    = should_draw_productions || should_draw_growth
    || should_draw_trade_routes;


  if (width != NULL) {
    *width = 0;
  }
  if (height != NULL) {
    *height = 0;
  }

  if (!gui_options.draw_city_names && !should_draw_lower_bar) {
    return;
  }


  /* First: calculate rect dimensions (but not positioning). */

  get_sprite_dimensions(bg, &bg_w, &bg_h);
  bg_w *= map_zoom; bg_h *= map_zoom;
  get_city_mapview_name_and_growth(pcity, name, sizeof(name),
				   growth, sizeof(growth), &growth_color, &production_color);

  if (gui_options.draw_city_names) {
    fc_snprintf(size, sizeof(size), "%d", city_size_get(pcity));

    get_text_size(&size_rect.w, &size_rect.h, FONT_CITY_SIZE, size);
    get_text_size(&name_rect.w, &name_rect.h, FONT_CITY_NAME, name);

    if (can_player_see_units_in_city(client.conn.playing, pcity)) {
      int count = unit_list_size(pcity->tile->units);

      count = CLIP(0, count, citybar->occupancy.size - 1);
      occupy = citybar->occupancy.p[count];
    } else {
      if (pcity->client.occupied) {
	occupy = citybar->occupied;
      } else {
	occupy = citybar->occupancy.p[0];
      }
    }

    get_sprite_dimensions(flag, &flag_rect.w, &flag_rect.h);
    flag_rect.w *= map_zoom; flag_rect.h *= map_zoom;
    get_sprite_dimensions(occupy, &occupy_rect.w, &occupy_rect.h);
    occupy_rect.w *= map_zoom; occupy_rect.h *= map_zoom;

    width1 = (flag_rect.w + occupy_rect.w + name_rect.w
	      + 2 * border + size_rect.w);
    height1 = MAX(flag_rect.h,
		  MAX(occupy_rect.h,
		      MAX(name_rect.h + border,
			  size_rect.h + border)));
  }

  if (should_draw_lower_bar) {
    width2 = 0;
    height2 = 0;

    if (should_draw_productions) {
      get_city_mapview_production(pcity, prod, sizeof(prod));
      get_text_size(&prod_rect.w, &prod_rect.h, FONT_CITY_PROD, prod);

      get_sprite_dimensions(citybar->shields, &shield_rect.w, &shield_rect.h);
      shield_rect.w *= map_zoom; shield_rect.h *= map_zoom;
      width2 += shield_rect.w + prod_rect.w + border;
      height2 = MAX(height2, shield_rect.h);
      height2 = MAX(height2, prod_rect.h + border);
    }

    if (should_draw_growth) {
      get_text_size(&growth_rect.w, &growth_rect.h, FONT_CITY_PROD, growth);
      get_sprite_dimensions(citybar->food, &food_rect.w, &food_rect.h);
      food_rect.w *= map_zoom; food_rect.h *= map_zoom;
      width2 += food_rect.w + growth_rect.w + border;
      height2 = MAX(height2, food_rect.h);
      height2 = MAX(height2, growth_rect.h + border);
    }

    if (should_draw_trade_routes) {
      get_city_mapview_trade_routes(pcity, trade_routes,
                                    sizeof(trade_routes),
                                    &trade_routes_color);
      get_text_size(&trade_routes_rect.w, &trade_routes_rect.h,
                    FONT_CITY_PROD, trade_routes);
      get_sprite_dimensions(citybar->trade, &trade_rect.w, &trade_rect.h);
      trade_rect.w *= map_zoom; trade_rect.h *= map_zoom;
      width2 += trade_rect.w + trade_routes_rect.w + border;
      height2 = MAX(height2, trade_rect.h);
      height2 = MAX(height2, trade_routes_rect.h + border);
    }
  }

  *width = MAX(width1, width2);
  *height = height1 + height2;


  /* Next fill in X and Y locations. */

  if (gui_options.draw_city_names) {
    flag_rect.x = canvas_x - *width / 2;
    flag_rect.y = canvas_y + (height1 - flag_rect.h) / 2;

    occupy_rect.x = flag_rect.x + flag_rect.w;
    occupy_rect.y = canvas_y + (height1 - occupy_rect.h) / 2;

    name_rect.x = canvas_x + (flag_rect.w + occupy_rect.w
			      - name_rect.w - size_rect.w - border) / 2;
    name_rect.y = canvas_y + (height1 - name_rect.h) / 2;

    size_rect.x = canvas_x + (*width + 1) / 2 - size_rect.w - border / 2;
    size_rect.y = canvas_y + (height1 - size_rect.h) / 2;
  }

  if (should_draw_lower_bar) {
    if (should_draw_productions) {
      shield_rect.x = canvas_x - *width / 2;
      shield_rect.y = canvas_y + height1 + (height2 - shield_rect.h) / 2;

      prod_rect.x = shield_rect.x + shield_rect.w + border / 2;
      prod_rect.y = canvas_y + height1 + (height2 - prod_rect.h) / 2;
    }

    if (should_draw_trade_routes) {
      trade_routes_rect.x = canvas_x + (*width + 1) / 2
        - trade_routes_rect.w - border / 2;
      trade_routes_rect.y = canvas_y + height1
        + (height2 - trade_routes_rect.h) / 2;

      trade_rect.x = trade_routes_rect.x - border / 2 - trade_rect.w;
      trade_rect.y = canvas_y + height1 + (height2 - trade_rect.h) / 2;
    }

    if (should_draw_growth) {
      growth_rect.x = canvas_x + (*width + 1) / 2
        - growth_rect.w - border / 2;
      if (trade_routes_rect.w > 0) {
        growth_rect.x = growth_rect.x
          - trade_routes_rect.w - border / 2 - trade_rect.w - border / 2;
      }
      growth_rect.y = canvas_y + height1 + (height2 - growth_rect.h) / 2;

      food_rect.x = growth_rect.x - border / 2 - food_rect.w;
      food_rect.y = canvas_y + height1 + (height2 - food_rect.h) / 2;
    }
  }


  /* Now draw. */

  /* Draw the city bar's background. */
  for (x = 0; x < *width; x += bg_w) {
    for (y = 0; y < *height; y += bg_h) {
      canvas_put_sprite(pcanvas, (canvas_x - *width / 2 + x) / map_zoom,
                        (canvas_y + y) / map_zoom,
			bg, 0, 0,
                        (*width - x) / map_zoom, (*height - y) / map_zoom);
    }
  }

  owner_color = get_player_color(tileset, city_owner(pcity));

  if (gui_options.draw_city_names) {
    canvas_put_sprite_full(pcanvas,
                           flag_rect.x / map_zoom, flag_rect.y / map_zoom,
                           flag);
    /* XXX: canvas_put_line() doesn't currently take map_zoom into account.
     * Should it?
     * In the meantime, don't compensate with '/ map_zoom' here, unlike
     * for canvas_put_sprite/text/rectangle */
    canvas_put_line(pcanvas, owner_color, LINE_NORMAL,
		    (flag_rect.x + flag_rect.w) /* / map_zoom */ - 1,
                    canvas_y /* / map_zoom */,
		    0, height1 /* / map_zoom */);
    canvas_put_sprite_full(pcanvas,
                           occupy_rect.x / map_zoom, occupy_rect.y / map_zoom,
                           occupy);
    canvas_put_text(pcanvas, name_rect.x / map_zoom, name_rect.y / map_zoom,
                    FONT_CITY_NAME,
                    get_color(tileset, COLOR_MAPVIEW_CITYTEXT), name);
    canvas_put_rectangle(pcanvas, owner_color,
			 (size_rect.x - border / 2) / map_zoom,
                         canvas_y / map_zoom,
			 (size_rect.w + border) / map_zoom,
                         height1 / map_zoom);
    {
      /* Try to pick a color for city size text that contrasts with
       * player color */
      struct color *textcolors[2] = {
        get_color(tileset, COLOR_MAPVIEW_CITYTEXT),
        get_color(tileset, COLOR_MAPVIEW_CITYTEXT_DARK)
      };

      canvas_put_text(pcanvas, size_rect.x / map_zoom, size_rect.y / map_zoom,
                      FONT_CITY_NAME,
                      color_best_contrast(owner_color, textcolors,
                                          ARRAY_SIZE(textcolors)), size);
    }
  }

  if (should_draw_lower_bar) {

    if (should_draw_productions) {
      canvas_put_sprite_full(pcanvas,
                             shield_rect.x / map_zoom, shield_rect.y / map_zoom,
                             citybar->shields);
      canvas_put_text(pcanvas, prod_rect.x / map_zoom, prod_rect.y / map_zoom,
                      FONT_CITY_PROD,
                      get_color(tileset, production_color), prod);
    }

    if (should_draw_trade_routes) {
      canvas_put_sprite_full(pcanvas,
                             trade_rect.x / map_zoom, trade_rect.y / map_zoom,
                             citybar->trade);
      canvas_put_text(pcanvas,
                      trade_routes_rect.x / map_zoom, trade_routes_rect.y / map_zoom,
                      FONT_CITY_PROD,
                      get_color(tileset, trade_routes_color), trade_routes);
    }

    if (should_draw_growth) {
      canvas_put_sprite_full(pcanvas,
                             food_rect.x / map_zoom, food_rect.y / map_zoom,
                             citybar->food);
      canvas_put_text(pcanvas, growth_rect.x / map_zoom, growth_rect.y / map_zoom,
                      FONT_CITY_PROD,
                      get_color(tileset, growth_color), growth);
    }
  }

  /* Draw the city bar's outline. */
  /* XXX not scaling by map_zoom, see above */
  canvas_put_line(pcanvas, owner_color, LINE_NORMAL,
		  (canvas_x - *width / 2) /* / map_zoom */,
                  canvas_y /* / map_zoom */,
		  *width /* / map_zoom */, 0);
  canvas_put_line(pcanvas, owner_color, LINE_NORMAL,
		  (canvas_x - *width / 2) /* / map_zoom */,
                  canvas_y /* / map_zoom */,
		  0, *height /* / map_zoom */);
  canvas_put_line(pcanvas, owner_color, LINE_NORMAL,
		  (canvas_x - *width / 2) /* / map_zoom */,
                  (canvas_y + *height) /* / map_zoom */ - 1,
		  *width /* / map_zoom */, 0);
  canvas_put_line(pcanvas, owner_color, LINE_NORMAL,
		  (canvas_x - *width / 2 + *width) /* / map_zoom */,
                  canvas_y /* / map_zoom */,
		  0, *height /* / map_zoom */);
  
  /* Draw the dividing line if we drew both the
   * upper and lower parts. */
  if (gui_options.draw_city_names && should_draw_lower_bar) {
    canvas_put_line(pcanvas, owner_color, LINE_NORMAL,
		    (canvas_x - *width / 2) /* / map_zoom */,
                    (canvas_y + height1) /* / map_zoom */ - 1,
		    *width /* / map_zoom */, 0);
  }
}

/************************************************************************//**
  Draw a "small" city bar for the city.  This is a subcase of show_city_desc
  (see that function for more info) for tilesets that do not have a full
  city bar.
****************************************************************************/
static void show_small_citybar(struct canvas *pcanvas,
                               int canvas_x, int canvas_y,
                               struct city *pcity, int *width, int *height)
{
  static char name[512], growth[32], prod[512], trade_routes[32];
  enum color_std growth_color;
  enum color_std production_color;
 /* trade_routes_color initialized just to get rid off gcc warning
   * on optimization level 3 when it misdiagnoses that it would be used
   * uninitialized otherwise. Funny thing here is that warning would
   * go away also by *not* setting it to values other than
   * COLOR_MAPVIEW_CITYTEXT in get_city_mapview_trade_routes() */
  enum color_std trade_routes_color = COLOR_MAPVIEW_CITYTEXT;
  struct {
    int x, y, w, h;
  } name_rect = {0, 0, 0, 0}, growth_rect = {0, 0, 0, 0},
    prod_rect = {0, 0, 0, 0}, trade_routes_rect = {0,};
  int total_width, total_height;
  int spacer_width = 0;
  const bool can_see_inside = (client_is_global_observer()
                               || city_owner(pcity) == client_player());

  *width = *height = 0;

  canvas_x += tileset_tile_width(tileset) / 2 * map_zoom;
  canvas_y += tileset_citybar_offset_y(tileset) * map_zoom;

  get_city_mapview_name_and_growth(pcity, name, sizeof(name),
                                   growth, sizeof(growth), &growth_color,
                                   &production_color);

  if (gui_options.draw_city_names) {
    int drawposx;

    /* HACK: put a character's worth of space between the two
     * strings if needed. */
    get_text_size(&spacer_width, NULL, FONT_CITY_NAME, "M");

    total_width = 0;
    total_height = 0;

    get_text_size(&name_rect.w, &name_rect.h, FONT_CITY_NAME, name);
    total_width += name_rect.w;
    total_height = MAX(total_height, name_rect.h);

    if (gui_options.draw_city_growth && can_see_inside) {
      get_text_size(&growth_rect.w, &growth_rect.h, FONT_CITY_PROD, growth);
      total_width += spacer_width + growth_rect.w;
      total_height = MAX(total_height, growth_rect.h);
    }

    if (gui_options.draw_city_trade_routes && can_see_inside) {
      get_city_mapview_trade_routes(pcity, trade_routes,
                                    sizeof(trade_routes),
                                    &trade_routes_color);
      get_text_size(&trade_routes_rect.w, &trade_routes_rect.h,
                    FONT_CITY_PROD, trade_routes);
      total_width += spacer_width + trade_routes_rect.w;
      total_height = MAX(total_height, trade_routes_rect.h);
    }

    drawposx = canvas_x;
    drawposx -= total_width / 2;
    canvas_put_text(pcanvas, drawposx / map_zoom, canvas_y / map_zoom,
		    FONT_CITY_NAME,
		    get_color(tileset, COLOR_MAPVIEW_CITYTEXT), name);
    drawposx += name_rect.w;

    if (gui_options.draw_city_growth && can_see_inside) {
      drawposx += spacer_width;
      canvas_put_text(pcanvas, drawposx / map_zoom,
		      (canvas_y + total_height - growth_rect.h) / map_zoom,
		      FONT_CITY_PROD,
		      get_color(tileset, growth_color), growth);
      drawposx += growth_rect.w;
    }

    if (gui_options.draw_city_trade_routes && can_see_inside) {
      drawposx += spacer_width;
      canvas_put_text(pcanvas, drawposx / map_zoom,
		      (canvas_y + total_height - trade_routes_rect.h) / map_zoom,
		      FONT_CITY_PROD,
		      get_color(tileset, trade_routes_color), trade_routes);
      drawposx += trade_routes_rect.w;
    }

    canvas_y += total_height + 3;

    *width = MAX(*width, total_width);
    *height += total_height + 3;
  }
  if (gui_options.draw_city_productions && can_see_inside) {
    get_city_mapview_production(pcity, prod, sizeof(prod));
    get_text_size(&prod_rect.w, &prod_rect.h, FONT_CITY_PROD, prod);

    total_width = prod_rect.w;
    total_height = prod_rect.h;

    canvas_put_text(pcanvas, (canvas_x - total_width / 2) / map_zoom,
                    canvas_y / map_zoom,
                    FONT_CITY_PROD,
                    get_color(tileset, production_color), prod);

    canvas_y += total_height;
    *width = MAX(*width, total_width);
    *height += total_height;
  }
}

/************************************************************************//**
  Draw a description for the given city.  This description may include the
  name, turns-to-grow, production, and city turns-to-build (depending on
  client options).

  (canvas_x, canvas_y) gives the location on the given canvas at which to
  draw the description.  This is the location of the city itself so the
  text must be drawn underneath it.  pcity gives the city to be drawn,
  while (*width, *height) should be set by show_city_desc to contain the
  width and height of the text block (centered directly underneath the
  city's tile).
****************************************************************************/
static void show_city_desc(struct canvas *pcanvas,
                           int canvas_x, int canvas_y,
                           struct city *pcity, int *width, int *height)
{
  if (gui_options.draw_full_citybar) {
    show_full_citybar(pcanvas, canvas_x, canvas_y, pcity, width, height);
  } else {
    show_small_citybar(pcanvas, canvas_x, canvas_y, pcity, width, height);
  }
}

/************************************************************************//**
  Draw a label for the given tile.

  (canvas_x, canvas_y) gives the location on the given canvas at which to
  draw the label.  This is the location of the tile itself so the
  text must be drawn underneath it.  pcity gives the city to be drawn,
  while (*width, *height) should be set by show_tile_label to contain the
  width and height of the text block (centered directly underneath the
  city's tile).
****************************************************************************/
static void show_tile_label(struct canvas *pcanvas,
                            int canvas_x, int canvas_y,
                            struct tile *ptile, int *width, int *height)
{
  const enum client_font FONT_TILE_LABEL = FONT_CITY_NAME; /* TODO: new font */
#define COLOR_MAPVIEW_TILELABEL COLOR_MAPVIEW_CITYTEXT

  canvas_x += tileset_tile_width(tileset) / 2 * map_zoom;
  canvas_y += tileset_tilelabel_offset_y(tileset) * map_zoom;

  get_text_size(width, height, FONT_TILE_LABEL, ptile->label);

  canvas_put_text(pcanvas, (canvas_x - * width / 2) / map_zoom, canvas_y / map_zoom,
                  FONT_TILE_LABEL,
                  get_color(tileset, COLOR_MAPVIEW_TILELABEL), ptile->label);
#undef COLOR_MAPVIEW_TILELABEL
}

/************************************************************************//**
  Show descriptions for all cities visible on the map canvas.
****************************************************************************/
void show_city_descriptions(int canvas_base_x, int canvas_base_y,
                            int width_base, int height_base)
{
  const int dx = max_desc_width - tileset_tile_width(tileset) * map_zoom;
  const int dy = max_desc_height;
  const int offset_y = tileset_citybar_offset_y(tileset) * map_zoom;
  int new_max_width = max_desc_width, new_max_height = max_desc_height;

  if (gui_options.draw_full_citybar && !(gui_options.draw_city_names
                                         || gui_options.draw_city_productions
                                         || gui_options.draw_city_growth)) {
    return;
  }

  if (!gui_options.draw_full_citybar && !(gui_options.draw_city_names
                                          || gui_options.draw_city_productions)) {
    return;
  }

  /* A city description is shown below the city.  It has a specified
   * maximum width and height (although these are only estimates).  Thus
   * we need to update some tiles above the mapview and some to the left
   * and right.
   *
   *                    /--W1--\   (W1 = tileset_tile_width(tileset))
   *                    -------- \
   *                    | CITY | H1 (H1 = tileset_tile_height(tileset))
   *                    |      | /
   *               ------------------ \
   *               |  DESCRIPTION   | H2  (H2 = MAX_CITY_DESC_HEIGHT)
   *               |                | /
   *               ------------------
   *               \-------W2-------/    (W2 = MAX_CITY_DESC_WIDTH)
   *
   * We must draw H2 extra pixels above and (W2 - W1) / 2 extra pixels
   * to each side of the mapview.
   */
  gui_rect_iterate_coord(mapview.gui_x0 + canvas_base_x - dx / 2,
			 mapview.gui_y0 + canvas_base_y - dy,
			 width_base + dx, height_base + dy - offset_y,
			 ptile, pedge, pcorner, gui_x, gui_y, map_zoom) {
    const int canvas_x = gui_x - mapview.gui_x0;
    const int canvas_y = gui_y - mapview.gui_y0;

    if (ptile && tile_city(ptile)) {
      int width = 0, height = 0;
      struct city *pcity = tile_city(ptile);

      show_city_desc(mapview.store, canvas_x, canvas_y,
                     pcity, &width, &height);
      log_debug("Drawing %s.", city_name_get(pcity));

      if (width > max_desc_width || height > max_desc_height) {
        /* The update was incomplete! We queue a new update. Note that
         * this is recursively queueing an update within a dequeuing of an
         * update. This is allowed specifically because of the code in
         * unqueue_mapview_updates. See that function for more. */
        log_debug("Re-queuing %s.", city_name_get(pcity));
        update_city_description(pcity);
      }
      new_max_width = MAX(width, new_max_width);
      new_max_height = MAX(height, new_max_height);
    }
  } gui_rect_iterate_coord_end;

  /* We don't update the new max values until the end, so that the
   * check above to see what cities need redrawing will be complete. */
  max_desc_width = MAX(max_desc_width, new_max_width);
  max_desc_height = MAX(max_desc_height, new_max_height);
}

/************************************************************************//**
  Show labels for all tiles visible on the map canvas.
****************************************************************************/
void show_tile_labels(int canvas_base_x, int canvas_base_y,
                      int width_base, int height_base)
{
  const int dx = max_label_width - tileset_tile_width(tileset) * map_zoom;
  const int dy = max_label_height;
  int new_max_width = max_label_width, new_max_height = max_label_height;

  gui_rect_iterate_coord(mapview.gui_x0 + canvas_base_x - dx / 2,
			 mapview.gui_y0 + canvas_base_y - dy,
			 width_base + dx, height_base + dy,
			 ptile, pedge, pcorner, gui_x, gui_y, map_zoom) {
    const int canvas_x = gui_x - mapview.gui_x0;
    const int canvas_y = gui_y - mapview.gui_y0;

    if (ptile && ptile->label != NULL) {
      int width = 0, height = 0;

      show_tile_label(mapview.store, canvas_x, canvas_y,
                      ptile, &width, &height);
      log_debug("Drawing label %s.", ptile->label);

      if (width > max_label_width || height > max_label_height) {
        /* The update was incomplete! We queue a new update. Note that
         * this is recursively queueing an update within a dequeuing of an
         * update. This is allowed specifically because of the code in
         * unqueue_mapview_updates. See that function for more. */
        log_debug("Re-queuing tile label %s drawing.", ptile->label);
        update_tile_label(ptile);
      }
      new_max_width = MAX(width, new_max_width);
      new_max_height = MAX(height, new_max_height);
    }
  } gui_rect_iterate_coord_end;

  /* We don't update the new max values until the end, so that the
   * check above to see what cities need redrawing will be complete. */
  max_label_width = MAX(max_label_width, new_max_width);
  max_label_height = MAX(max_label_height, new_max_height);
}

/************************************************************************//**
  Draw the goto route for the unit.  Return TRUE if anything is drawn.

  This duplicates drawing code that is run during the hover state.
****************************************************************************/
bool show_unit_orders(struct unit *punit)
{
  if (punit && unit_has_orders(punit)) {
    struct tile *ptile = unit_tile(punit);
    int i;

    for (i = 0; i < punit->orders.length; i++) {
      int idx = (punit->orders.index + i) % punit->orders.length;
      struct unit_order *order;

      if (punit->orders.index + i >= punit->orders.length
          && !punit->orders.repeat) {
        break;
      }

      order = &punit->orders.list[idx];

      switch (order->order) {
      case ORDER_MOVE:
        draw_segment(ptile, order->dir);
        ptile = mapstep(&(wld.map), ptile, order->dir);
        if (!ptile) {
          /* This shouldn't happen unless the server gives us invalid
           * data.  To avoid disaster we need to break out of the
           * switch and the enclosing for loop. */
          fc_assert(NULL != ptile);
          i = punit->orders.length;
        }
        break;
      default:
        /* TODO: graphics for other orders. */
        break;
      }
    }
    return TRUE;
  } else {
    return FALSE;
  }
}

/************************************************************************//**
  Draw a goto line at the given location and direction.  The line goes from
  the source tile to the adjacent tile in the given direction.
****************************************************************************/
void draw_segment(struct tile *src_tile, enum direction8 dir)
{
  float canvas_x, canvas_y, canvas_dx, canvas_dy;

  /* Determine the source position of the segment. */
  (void) tile_to_canvas_pos(&canvas_x, &canvas_y, src_tile);
  canvas_x += tileset_tile_width(tileset) / 2 * map_zoom;
  canvas_y += tileset_tile_height(tileset) / 2 * map_zoom;

  /* Determine the vector of the segment. */
  map_to_gui_vector(tileset, map_zoom, &canvas_dx, &canvas_dy,
                    DIR_DX[dir], DIR_DY[dir]);

  /* Draw the segment. */
  /* XXX: canvas_put_line doesn't currently take map_zoom into account
   * itself, but it probably should? If so this will need adjusting */
  canvas_put_line(mapview.store,
		  get_color(tileset, COLOR_MAPVIEW_GOTO), LINE_GOTO,
		  canvas_x, canvas_y, canvas_dx, canvas_dy);

  /* The actual area drawn will extend beyond the base rectangle, since
   * the goto lines have width. */
  dirty_rect(MIN(canvas_x, canvas_x + canvas_dx) - GOTO_WIDTH,
	     MIN(canvas_y, canvas_y + canvas_dy) - GOTO_WIDTH,
	     ABS(canvas_dx) + 2 * GOTO_WIDTH,
	     ABS(canvas_dy) + 2 * GOTO_WIDTH);

  /* It is possible that the mapview wraps between the source and dest
   * tiles.  In this case they will not be next to each other; they'll be
   * on the opposite sides of the screen.  If this happens then the dest
   * tile will not be updated.  This is consistent with the mapview design
   * which fails when the size of the mapview approaches that of the map. */
}

/************************************************************************//**
  This function is called to decrease a unit's HP smoothly in battle
  when combat_animation is turned on.
****************************************************************************/
void decrease_unit_hp_smooth(struct unit *punit0, int hp0, 
                             struct unit *punit1, int hp1)
{
  struct unit *losing_unit = (hp0 == 0 ? punit0 : punit1);
  float canvas_x, canvas_y;
  int i;

  set_units_in_combat(punit0, punit1);

  /* Make sure we don't start out with fewer HP than we're supposed to
   * end up with (which would cause the following loop to break). */
  punit0->hp = MAX(punit0->hp, hp0);
  punit1->hp = MAX(punit1->hp, hp1);

  unqueue_mapview_updates(TRUE);

  if (frame_by_frame_animation) {
    struct animation *anim = fc_malloc(sizeof(struct animation));
    struct unit *winning_unit;
    int winner_end_hp;

    if (losing_unit == punit1) {
      winning_unit = punit0;
      winner_end_hp = hp0;
    } else {
      winning_unit = punit1;
      winner_end_hp = hp1;
    }

    anim->type = ANIM_BATTLE;
    anim->id = -1;
    anim->battle.virt_loser = unit_virtual_create(unit_owner(losing_unit),
                                                  NULL, unit_type_get(losing_unit),
                                                  losing_unit->veteran);
    anim->battle.loser_tile = unit_tile(losing_unit);
    anim->battle.virt_loser->facing = losing_unit->facing;
    anim->battle.loser_hp_start = losing_unit->hp;
    anim->battle.virt_winner = unit_virtual_create(unit_owner(winning_unit),
                                                   NULL, unit_type_get(winning_unit),
                                                   winning_unit->veteran);
    anim->battle.winner_tile = unit_tile(winning_unit);
    anim->battle.virt_winner->facing = winning_unit->facing;
    anim->battle.winner_hp_start = MAX(winning_unit->hp, winner_end_hp);
    anim->battle.winner_hp_end = winner_end_hp;
    anim->battle.steps = MAX(losing_unit->hp,
                             anim->battle.winner_hp_start - winner_end_hp);
    animation_add(anim);

    anim = fc_malloc(sizeof(struct animation));
    anim->type = ANIM_EXPL;
    anim->id = winning_unit->id;
    anim->expl.tile = losing_unit->tile;
    anim->expl.sprites = get_unit_explode_animation(tileset);
    anim->expl.sprite_count = sprite_vector_size(anim->expl.sprites);
    anim->width = tileset_tile_width(tileset) * map_zoom;
    anim->height = tileset_tile_height(tileset) * map_zoom;
    animation_add(anim);
  } else {
    const struct sprite_vector *anim = get_unit_explode_animation(tileset);
    const int num_tiles_explode_unit = sprite_vector_size(anim);

    while (punit0->hp > hp0 || punit1->hp > hp1) {
      const int diff0 = punit0->hp - hp0, diff1 = punit1->hp - hp1;

      anim_timer = timer_renew(anim_timer, TIMER_USER, TIMER_ACTIVE);
      timer_start(anim_timer);

      if (fc_rand(diff0 + diff1) < diff0) {
        punit0->hp--;
        refresh_unit_mapcanvas(punit0, unit_tile(punit0), FALSE, FALSE);
      } else {
        punit1->hp--;
        refresh_unit_mapcanvas(punit1, unit_tile(punit1), FALSE, FALSE);
      }

      unqueue_mapview_updates(TRUE);
      gui_flush();

      timer_usleep_since_start(anim_timer,
                               gui_options.smooth_combat_step_msec * 1000ul);
    }

    if (num_tiles_explode_unit > 0
        && tile_to_canvas_pos(&canvas_x, &canvas_y,
                              unit_tile(losing_unit))) {
      refresh_unit_mapcanvas(losing_unit, unit_tile(losing_unit), FALSE, FALSE);
      unqueue_mapview_updates(FALSE);
      canvas_copy(mapview.tmp_store, mapview.store,
                  canvas_x, canvas_y, canvas_x, canvas_y,
                  tileset_tile_width(tileset) * map_zoom,
                  tileset_tile_height(tileset) * map_zoom);

      for (i = 0; i < num_tiles_explode_unit; i++) {
        int w, h;
        struct sprite *sprite = *sprite_vector_get(anim, i);

        get_sprite_dimensions(sprite, &w, &h);
        anim_timer = timer_renew(anim_timer, TIMER_USER, TIMER_ACTIVE);
        timer_start(anim_timer);

        /* We first draw the explosion onto the unit and draw draw the
         * complete thing onto the map canvas window. This avoids
         * flickering. */
        canvas_copy(mapview.store, mapview.tmp_store,
                    canvas_x, canvas_y, canvas_x, canvas_y,
                    tileset_tile_width(tileset) * map_zoom,
                    tileset_tile_height(tileset) * map_zoom);
        canvas_put_sprite_full(mapview.store,
                               canvas_x + tileset_tile_width(tileset) / 2 * map_zoom
                               - w / 2,
                               canvas_y + tileset_tile_height(tileset) / 2 * map_zoom
                               - h / 2,
                               sprite);
        dirty_rect(canvas_x, canvas_y, tileset_tile_width(tileset) * map_zoom,
                   tileset_tile_height(tileset) * map_zoom);

        flush_dirty();
        gui_flush();

        timer_usleep_since_start(anim_timer,
                                 gui_options.smooth_combat_step_msec * 2 * 1000ul);
      }
    }
  }

  set_units_in_combat(NULL, NULL);
  refresh_unit_mapcanvas(punit0, unit_tile(punit0), TRUE, FALSE);
  refresh_unit_mapcanvas(punit1, unit_tile(punit1), TRUE, FALSE);
}

/************************************************************************//**
  Animates punit's "smooth" move from (x0, y0) to (x0+dx, y0+dy).
  Note: Works only for adjacent-tile moves.
****************************************************************************/
void move_unit_map_canvas(struct unit *punit,
                          struct tile *src_tile, int dx, int dy)
{
  struct tile *dest_tile;
  int dest_x, dest_y, src_x, src_y;
  int prev_x = -1;
  int prev_y = -1;
  int tuw;
  int tuh;

  /* only works for adjacent-square moves */
  if (dx < -1 || dx > 1 || dy < -1 || dy > 1 || (dx == 0 && dy == 0)) {
    return;
  }

  index_to_map_pos(&src_x, &src_y, tile_index(src_tile));
  dest_x = src_x + dx;
  dest_y = src_y + dy;
  dest_tile = map_pos_to_tile(&(wld.map), dest_x, dest_y);
  if (!dest_tile) {
    return;
  }

  if (tile_visible_mapcanvas(src_tile)
      || tile_visible_mapcanvas(dest_tile)) {
    float start_x, start_y;
    float canvas_dx, canvas_dy;
    double timing_sec = (double)gui_options.smooth_move_unit_msec / 1000.0;
    double mytime;

    fc_assert(gui_options.smooth_move_unit_msec > 0);

    map_to_gui_vector(tileset, map_zoom, &canvas_dx, &canvas_dy, dx, dy);

    tile_to_canvas_pos(&start_x, &start_y, src_tile);
    if (tileset_is_isometric(tileset) && tileset_hex_height(tileset) == 0) {
      start_y -= tileset_tile_height(tileset) / 2 * map_zoom;
      start_y -= (tileset_unit_height(tileset) - tileset_full_tile_height(tileset)) * map_zoom;
    }

    /* Bring the backing store up to date, but don't flush. */
    unqueue_mapview_updates(FALSE);

    tuw = tileset_unit_width(tileset) * map_zoom;
    tuh = tileset_unit_height(tileset) * map_zoom;

    if (frame_by_frame_animation) {
      struct animation *anim = fc_malloc(sizeof(struct animation));

      anim->type = ANIM_MOVEMENT;
      anim->id = punit->id;
      punit->refcount++;
      anim->movement.mover = punit;
      anim->movement.src = src_tile;
      anim->movement.dest = dest_tile;
      anim->movement.canvas_dx = canvas_dx;
      anim->movement.canvas_dy = canvas_dy;
      anim->width = tuw;
      anim->height = tuh;
      animation_add(anim);
    } else {

      /* Start the timer (AFTER the unqueue above). */
      anim_timer = timer_renew(anim_timer, TIMER_USER, TIMER_ACTIVE);
      timer_start(anim_timer);

      do {
        int new_x, new_y;

        mytime = MIN(timer_read_seconds(anim_timer), timing_sec);

        new_x = start_x + canvas_dx * (mytime / timing_sec);
        new_y = start_y + canvas_dy * (mytime / timing_sec);

        if (new_x != prev_x || new_y != prev_y) {
          /* Backup the canvas store to the temp store. */
          canvas_copy(mapview.tmp_store, mapview.store,
                      new_x, new_y, new_x, new_y,
                      tuw, tuh);

          /* Draw */
          put_unit(punit, mapview.store, map_zoom, new_x, new_y);
          dirty_rect(new_x, new_y, tuw, tuh);

          /* Flush. */
          flush_dirty();
          gui_flush();

          /* Restore the backup.  It won't take effect until the next flush. */
          canvas_copy(mapview.store, mapview.tmp_store,
                      new_x, new_y, new_x, new_y,
                      tuw, tuh);
          dirty_rect(new_x, new_y, tuw, tuh);

          prev_x = new_x;
          prev_y = new_y;
        } else {
          fc_usleep(500);
        }
      } while (mytime < timing_sec);
    }
  }
}

/************************************************************************//**
  Find the "best" city/settlers to associate with the selected tile.
    a.  If a visible city is working the tile, return that city.
    b.  If another player's city is working the tile, return NULL.
    c.  If any selected cities are within range, return the closest one.
    d.  If any cities are within range, return the closest one.
    e.  If any active (with color) settler could work it if they founded a
        city, choose the closest one (only if punit != NULL).
    f.  If any settler could work it if they founded a city, choose the
        closest one (only if punit != NULL).
    g.  If nobody can work it, return NULL.
****************************************************************************/
struct city *find_city_or_settler_near_tile(const struct tile *ptile,
                                            struct unit **punit)
{
  struct city *closest_city;
  struct city *pcity;
  struct unit *closest_settler = NULL, *best_settler = NULL;
  int max_rad = rs_max_city_radius_sq();

  if (punit) {
    *punit = NULL;
  }

  /* Check if there is visible city working that tile */
  pcity = tile_worked(ptile);
  if (pcity && pcity->tile) {
    if (NULL == client.conn.playing
        || city_owner(pcity) == client.conn.playing) {
      /* rule a */
      return pcity;
    } else {
      /* rule b */
      return NULL;
    }
  }

  /* rule e */
  closest_city = NULL;

  /* check within maximum (squared) city radius */
  city_tile_iterate(max_rad, ptile, tile1) {
    pcity = tile_city(tile1);
    if (pcity
	&& (NULL == client.conn.playing
	    || city_owner(pcity) == client.conn.playing)
	&& client_city_can_work_tile(pcity, tile1)) {
      /*
       * Note, we must explicitly check if the tile is workable (with
       * city_can_work_tile() above), since it is possible that another
       * city (perhaps an UNSEEN city) may be working it!
       */
      
      if (mapdeco_is_highlight_set(city_tile(pcity))) {
	/* rule c */
	return pcity;
      }
      if (!closest_city) {
	closest_city = pcity;
      }
    }
  } city_tile_iterate_end;

  /* rule d */
  if (closest_city || !punit) {
    return closest_city;
  }

  if (!game.scenario.prevent_new_cities) {
    /* check within maximum (squared) city radius */
    city_tile_iterate(max_rad, ptile, tile1) {
      unit_list_iterate(tile1->units, psettler) {
        if ((NULL == client.conn.playing
             || unit_owner(psettler) == client.conn.playing)
            && unit_can_do_action(psettler, ACTION_FOUND_CITY)
            && city_can_be_built_here(unit_tile(psettler), psettler)) {
          if (!closest_settler) {
            closest_settler = psettler;
          }
          if (!best_settler && psettler->client.colored) {
            best_settler = psettler;
          }
        }
      } unit_list_iterate_end;
    } city_tile_iterate_end;

    if (best_settler) {
      /* Rule e */
      *punit = best_settler;
    } else if (closest_settler) {
      /* Rule f */
      *punit = closest_settler;
    }
  }

  /* rule g */
  return NULL;
}

/************************************************************************//**
  Find the nearest/best city that owns the tile.
****************************************************************************/
struct city *find_city_near_tile(const struct tile *ptile)
{
  return find_city_or_settler_near_tile(ptile, NULL);
}

/************************************************************************//**
  Append the buy cost of the current production of the given city to the
  already NULL-terminated buffer. Does nothing if draw_city_buycost is
  set to FALSE, or if it does not make sense to buy the current production
  (e.g. coinage).
****************************************************************************/
static void append_city_buycost_string(const struct city *pcity,
                                       char *buffer, int buffer_len)
{
  if (!pcity || !buffer || buffer_len < 1) {
    return;
  }

  if (!gui_options.draw_city_buycost || !city_can_buy(pcity)) {
    return;
  }

  cat_snprintf(buffer, buffer_len, "/%d", pcity->client.buy_cost);
}

/************************************************************************//**
  Find the mapview city production text for the given city, and place it
  into the buffer.
****************************************************************************/
void get_city_mapview_production(struct city *pcity,
                                 char *buffer, size_t buffer_len)
{
  int turns;

  universal_name_translation(&pcity->production, buffer, buffer_len);

  if (city_production_has_flag(pcity, IF_GOLD)) {
    return;
  }
  turns = city_production_turns_to_build(pcity, TRUE);

  if (999 < turns) {
    cat_snprintf(buffer, buffer_len, " -");
  } else {
    cat_snprintf(buffer, buffer_len, " %d", turns);
  }

  append_city_buycost_string(pcity, buffer, buffer_len);
}

/************************************************************************//**
  Find the mapview city trade routes text for the given city, and place it
  into the buffer. Sets 'pcolor' to the preferred color the text should
  be drawn in if it is non-NULL.
****************************************************************************/
void get_city_mapview_trade_routes(struct city *pcity,
                                   char *trade_routes_buffer,
                                   size_t trade_routes_buffer_len,
                                   enum color_std *pcolor)
{
  int num_trade_routes;
  int max_routes;

  if (!trade_routes_buffer || trade_routes_buffer_len <= 0) {
    return;
  }

  if (!pcity) {
    trade_routes_buffer[0] = '\0';
    if (pcolor) {
      *pcolor = COLOR_MAPVIEW_CITYTEXT;
    }
    return;
  }

  num_trade_routes = trade_route_list_size(pcity->routes);
  max_routes = max_trade_routes(pcity);

  fc_snprintf(trade_routes_buffer, trade_routes_buffer_len,
              "%d/%d", num_trade_routes, max_routes);

  if (pcolor) {
    if (num_trade_routes == max_routes) {
      *pcolor = COLOR_MAPVIEW_TRADE_ROUTES_ALL_BUILT;
    } else if (num_trade_routes == 0) {
      *pcolor = COLOR_MAPVIEW_TRADE_ROUTES_NO_BUILT;
    } else {
      *pcolor = COLOR_MAPVIEW_TRADE_ROUTES_SOME_BUILT;
    }
  }
}

/***************************************************************************/
static enum update_type needed_updates = UPDATE_NONE;
static bool callback_queued = FALSE;

/* These values hold the tiles that need city, unit, or tile updates.
 * These different types of updates just tell what area need to be updated,
 * not necessarily what's sitting on the tile.  A city update covers the
 * whole citymap area.  A unit update covers just the "full" unit tile
 * area.  A tile update covers the base tile plus half a tile in each
 * direction. */
struct tile_list *tile_updates[TILE_UPDATE_COUNT];

/************************************************************************//**
  This callback is called during an idle moment to unqueue any pending
  mapview updates.
****************************************************************************/
static void queue_callback(void *data)
{
  callback_queued = FALSE;
  unqueue_mapview_updates(TRUE);
}

/************************************************************************//**
  When a mapview update is queued this function should be called to prepare
  an idle-time callback to unqueue the updates.
****************************************************************************/
static void queue_add_callback(void)
{
  if (!callback_queued) {
    callback_queued = TRUE;
    add_idle_callback(queue_callback, NULL);
  }
}

/************************************************************************//**
  This function, along with unqueue_mapview_update(), helps in updating
  the mapview when a packet is received.  Previously, we just called
  update_map_canvas when (for instance) a city update was received.
  Not only would this often end up with a lot of duplicated work, but it
  would also draw over the city descriptions, which would then just
  "disappear" from the mapview.  The hack is to instead call
  queue_mapview_update in place of this update, and later (after all
  packets have been read) call unqueue_mapview_update.  The functions
  don't track which areas of the screen need updating, rather when the
  unqueue is done we just update the whole visible mapqueue, and redraw
  the city descriptions.

  Using these functions, updates are done correctly, and are probably
  faster too.  But it's a bit of a hack to insert this code into the
  packet-handling code.
****************************************************************************/
void queue_mapview_update(enum update_type update)
{
  if (can_client_change_view()) {
    needed_updates |= update;
    queue_add_callback();
  }
}

/************************************************************************//**
  Queue this tile to be refreshed.  The refresh will be done some time
  soon thereafter, and grouped with other needed refreshes.

  Note this should only be called for tiles.  For cities or units use
  queue_mapview_xxx_update instead.
****************************************************************************/
void queue_mapview_tile_update(struct tile *ptile,
                               enum tile_update_type type)
{
  if (can_client_change_view()) {
    if (!tile_updates[type]) {
      tile_updates[type] = tile_list_new();
    }
    tile_list_append(tile_updates[type], ptile);
    queue_add_callback();
  }
}

/************************************************************************//**
  See comment for queue_mapview_update().
****************************************************************************/
void unqueue_mapview_updates(bool write_to_screen)
{
  /* Calculate the area covered by each update type.  The area array gives
   * the offset from the tile origin as well as the width and height of the
   * area to be updated.  This is initialized each time when entering the
   * function from the existing tileset variables.
   *
   * A TILE update covers the base tile (W x H) plus a half-tile in each
   * direction (for edge/corner graphics), making its area 2W x 2H.
   *
   * A UNIT update covers a UW x UH area.  This is centered horizontally
   * over the tile but extends up above the tile (e.g., units in iso-view).
   *
   * A CITYMAP update covers the whole citymap of a tile.  This includes
   * the citymap area itself plus an extra half-tile in each direction (for
   * edge/corner graphics).
   */
  const float W = tileset_tile_width(tileset) * map_zoom;
  const float H = tileset_tile_height(tileset) * map_zoom;
  const float UW = tileset_unit_width(tileset) * map_zoom;
  const float UH = tileset_unit_height(tileset) * map_zoom;
  const float city_width = get_citydlg_canvas_width() * map_zoom + W;
  const float city_height = get_citydlg_canvas_height() * map_zoom + H;
  const struct {
    float dx, dy, w, h;
  } area[TILE_UPDATE_COUNT] = {
    {0, 0, W, H},
    {-W / 2, -H / 2, 2 * W, 2 * H},
    {(W - UW) / 2, H - UH, UW, UH},
    {-(max_desc_width - W) / 2, H, max_desc_width, max_desc_height},
    {-(city_width - W) / 2, -(city_height - H) / 2, city_width, city_height},
    {-(max_label_width - W) / 2, H, max_label_width, max_label_height}
  };
  struct tile_list *my_tile_updates[TILE_UPDATE_COUNT];

  int i;

  if (!can_client_change_view()) {
    /* Double sanity check: make sure we don't unqueue an invalid update
     * after we've already detached. */
    return;
  }

  log_debug("unqueue_mapview_update: needed_updates=%d",
            needed_updates);

  /* This code "pops" the lists of tile updates off of the static array and
   * stores them locally.  This allows further updates to be queued within
   * the function itself (namely, within update_map_canvas). */
  for (i = 0; i < TILE_UPDATE_COUNT; i++) {
    my_tile_updates[i] = tile_updates[i];
    tile_updates[i] = NULL;
  }

  if (!map_is_empty()) {
    if ((needed_updates & UPDATE_MAP_CANVAS_VISIBLE)
	|| (needed_updates & UPDATE_CITY_DESCRIPTIONS)
        || (needed_updates & UPDATE_TILE_LABELS)) {
      dirty_all();
      update_map_canvas(0, 0, mapview.store_width,
			mapview.store_height);
      /* Have to update the overview too, since some tiles may have changed. */
      refresh_overview_canvas();
    } else {
      int min_x = mapview.width, min_y = mapview.height;
      int max_x = 0, max_y = 0;

      for (i = 0; i < TILE_UPDATE_COUNT; i++) {
        if (my_tile_updates[i]) {
          tile_list_iterate(my_tile_updates[i], ptile) {
            float xl, yt;
            int xr, yb;

	    (void) tile_to_canvas_pos(&xl, &yt, ptile);

	    xl += area[i].dx;
	    yt += area[i].dy;
	    xr = xl + area[i].w;
	    yb = yt + area[i].h;

	    if (xr > 0 && xl < mapview.width
		&& yb > 0 && yt < mapview.height) {
	      min_x = MIN(min_x, xl);
	      min_y = MIN(min_y, yt);
	      max_x = MAX(max_x, xr);
	      max_y = MAX(max_y, yb);
	    }

	    /* FIXME: These overview updates should be batched as well.
	     * Right now they account for as much as 90% of the runtime of
	     * the unqueue. */
	    overview_update_tile(ptile);
	  } tile_list_iterate_end;
	}
      }

      if (min_x < max_x && min_y < max_y) {
	update_map_canvas(min_x, min_y, max_x - min_x, max_y - min_y);
      }
    }
  }

  for (i = 0; i < TILE_UPDATE_COUNT; i++) {
    if (my_tile_updates[i]) {
      tile_list_destroy(my_tile_updates[i]);
    }
  }
  needed_updates = UPDATE_NONE;

  if (write_to_screen) {
    flush_dirty();
    flush_dirty_overview();
  }
}

/************************************************************************//**
  Fill the two buffers which information about the city which is shown
  below it. It does not take draw_city_names/draw_city_growth into account.
****************************************************************************/
void get_city_mapview_name_and_growth(struct city *pcity,
                                      char *name_buffer,
                                      size_t name_buffer_len,
                                      char *growth_buffer,
                                      size_t growth_buffer_len,
                                      enum color_std *growth_color,
                                      enum color_std *production_color)
{
  fc_strlcpy(name_buffer, city_name_get(pcity), name_buffer_len);

  *production_color = COLOR_MAPVIEW_CITYTEXT;
  if (NULL == client.conn.playing
      || city_owner(pcity) == client.conn.playing) {
    int turns = city_turns_to_grow(pcity);

    if (turns == 0) {
      fc_snprintf(growth_buffer, growth_buffer_len, "X");
    } else if (turns == FC_INFINITY) {
      fc_snprintf(growth_buffer, growth_buffer_len, "-");
    } else {
      /* Negative turns means we're shrinking, but that's handled
         down below. */
      fc_snprintf(growth_buffer, growth_buffer_len, "%d", abs(turns));
    }

    if (turns <= 0) {
      /* A blocked or shrinking city has its growth status shown in red. */
      *growth_color = COLOR_MAPVIEW_CITYGROWTH_BLOCKED;
    } else {
      *growth_color = COLOR_MAPVIEW_CITYTEXT;
    }

    if (pcity->surplus[O_SHIELD] < 0) {
      *production_color = COLOR_MAPVIEW_CITYPROD_NEGATIVE;
    }
  } else {
    growth_buffer[0] = '\0';
    *growth_color = COLOR_MAPVIEW_CITYTEXT;
  }
}

/************************************************************************//**
  Returns TRUE if cached drawing is possible.  If the mapview is too large
  we have to turn it off.
****************************************************************************/
static bool can_do_cached_drawing(void)
{
  const int W = tileset_tile_width(tileset) * map_zoom;
  const int H = tileset_tile_height(tileset) * map_zoom;
  int w = mapview.store_width, h = mapview.store_height;

  /* If the mapview window is too large, cached drawing is not possible.
   *
   * BACKGROUND: cached drawing occurrs when the mapview is scrolled just
   * a short distance.  The majority of the mapview window can simply be
   * copied while the newly visible areas must be drawn from scratch.  This
   * speeds up drawing significantly, especially when using the scrollbars
   * or mapview sliding.
   *
   * When the mapview is larger than the map, however, some tiles may become
   * visible twice.  In this case one instance of the tile will be drawn
   * while all others are drawn black.  When this happens the cached drawing
   * system breaks since it assumes the mapview canvas is an "ideal" window
   * over the map.  So black tiles may be scrolled from the edges of the
   * mapview into the center, while drawn tiles may be scrolled from the
   * center of the mapview out to the edges.  The result is very bad.
   *
   * There are a few different ways this could be solved.  One way is simply
   * to turn off cached drawing, which is what we do now.  If the mapview
   * window gets to be too large, the caching is disabled.  Another would
   * be to prevent the window from getting too large in the first place -
   * but because the window boundaries aren't at an even tile this would
   * mean the entire map could never be shown.  Yet another way would be
   * to draw tiles more than once if they are visible in multiple locations
   * on the mapview.
   *
   * The logic below is complicated and determined in part by
   * trial-and-error. */
  if (!current_topo_has_flag(TF_WRAPX) && !current_topo_has_flag(TF_WRAPY)) {
    /* An unwrapping map: no limitation.  On an unwrapping map no tile can
     * be visible twice so there's no problem. */
    return TRUE;
  }
  if (XOR(current_topo_has_flag(TF_ISO) || current_topo_has_flag(TF_HEX),
	  tileset_is_isometric(tileset))) {
    /* Non-matching.  In this case the mapview does not line up with the
     * map's axis of wrapping.  This will give very bad results for the
     * player!
     * We can never show more than half of the map.
     *
     * We divide by 4 below because we have to divide by 2 twice.  The
     * first division by 2 is because the square must be half the size
     * of the (width+height).  The second division by two is because for
     * an iso-map, NATURAL_XXX has a scale of 2, whereas for iso-view
     * NORMAL_TILE_XXX has a scale of 2. */
    return (w <= (NATURAL_WIDTH + NATURAL_HEIGHT) * W / 4
	    && h <= (NATURAL_WIDTH + NATURAL_HEIGHT) * H / 4);
  } else {
    /* Matching. */
    const int isofactor = (tileset_is_isometric(tileset) ? 2 : 1);
    const int isodiff = (tileset_is_isometric(tileset) ? 6 : 2);

    /* Now we can use the full width and height, with the exception of a small
     * area on each side. */
    if (current_topo_has_flag(TF_WRAPX)
	&& w > (NATURAL_WIDTH - isodiff) * W / isofactor) {
      return FALSE;
    }
    if (current_topo_has_flag(TF_WRAPY)
	&& h > (NATURAL_HEIGHT - isodiff) * H / isofactor) {
      return FALSE;
    }
    return TRUE;
  }
}

/************************************************************************//**
  Called when we receive map dimensions.  It initialized the mapview
  decorations.
****************************************************************************/
void mapdeco_init(void)
{
  /* HACK: this must be called on a map_info packet. */
  mapview.can_do_cached_drawing = can_do_cached_drawing();

  mapdeco_free();
  mapdeco_highlight_table = tile_hash_new();
  mapdeco_crosshair_table = tile_hash_new();
  mapdeco_gotoline_table = gotoline_hash_new();
}

/************************************************************************//**
  Free all memory used for map decorations.
****************************************************************************/
void mapdeco_free(void)
{
  if (mapdeco_highlight_table) {
    tile_hash_destroy(mapdeco_highlight_table);
    mapdeco_highlight_table = NULL;
  }
  if (mapdeco_crosshair_table) {
    tile_hash_destroy(mapdeco_crosshair_table);
    mapdeco_crosshair_table = NULL;
  }
  if (mapdeco_gotoline_table) {
    gotoline_hash_destroy(mapdeco_gotoline_table);
    mapdeco_gotoline_table = NULL;
  }
}

/************************************************************************//**
  Set the given tile's map decoration as either highlighted or not,
  depending on the value of 'highlight'.
****************************************************************************/
void mapdeco_set_highlight(const struct tile *ptile, bool highlight)
{
  bool changed = FALSE;

  if (!ptile || !mapdeco_highlight_table) {
    return;
  }

  if (highlight) {
    changed = tile_hash_insert(mapdeco_highlight_table, ptile, NULL);
  } else {
    changed = tile_hash_remove(mapdeco_highlight_table, ptile);
  }

  if (changed) {
    /* FIXME: Remove the cast. */
    refresh_tile_mapcanvas((struct tile *) ptile, TRUE, FALSE);
  }
}

/************************************************************************//**
  Return TRUE if the given tile is highlighted.
****************************************************************************/
bool mapdeco_is_highlight_set(const struct tile *ptile)
{
  if (!ptile || !mapdeco_highlight_table) {
    return FALSE;
  }
  return tile_hash_lookup(mapdeco_highlight_table, ptile, NULL);
}

/************************************************************************//**
  Clears all highlighting. Marks the previously highlighted tiles as
  needing a mapview update.
****************************************************************************/
void mapdeco_clear_highlights(void)
{
  if (!mapdeco_highlight_table) {
    return;
  }

  tile_hash_iterate(mapdeco_highlight_table, ptile) {
    refresh_tile_mapcanvas(ptile, TRUE, FALSE);
  } tile_hash_iterate_end;

  tile_hash_clear(mapdeco_highlight_table);
}

/************************************************************************//**
  Marks the given tile as having a "crosshair" map decoration.
****************************************************************************/
void mapdeco_set_crosshair(const struct tile *ptile, bool crosshair)
{
  bool changed;

  if (!mapdeco_crosshair_table || !ptile) {
    return;
  }

  if (crosshair) {
    changed = tile_hash_insert(mapdeco_crosshair_table, ptile, NULL);
  } else {
    changed = tile_hash_remove(mapdeco_crosshair_table, ptile);
  }

  if (changed) {
    /* FIXME: Remove the cast. */
    refresh_tile_mapcanvas((struct tile *) ptile, FALSE, FALSE);
  }
}

/************************************************************************//**
  Returns TRUE if there is a "crosshair" decoration set at the given tile.
****************************************************************************/
bool mapdeco_is_crosshair_set(const struct tile *ptile)
{
  if (!mapdeco_crosshair_table || !ptile) {
    return FALSE;
  }
  return tile_hash_lookup(mapdeco_crosshair_table, ptile, NULL);
}

/************************************************************************//**
  Clears all previous set tile crosshair decorations. Marks the affected
  tiles as needing a mapview update.
****************************************************************************/
void mapdeco_clear_crosshairs(void)
{
  if (!mapdeco_crosshair_table) {
    return;
  }

  tile_hash_iterate(mapdeco_crosshair_table, ptile) {
    refresh_tile_mapcanvas(ptile, FALSE, FALSE);
  } tile_hash_iterate_end;

  tile_hash_clear(mapdeco_crosshair_table);
}

/************************************************************************//**
  Add a goto line from the given tile 'ptile' in the direction 'dir'. If
  there was no previously drawn line there, a mapview update is queued
  for the source and destination tiles.
****************************************************************************/
void mapdeco_add_gotoline(const struct tile *ptile, enum direction8 dir)
{
  struct gotoline_counter *pglc;
  const struct tile *ptile_dest;
  bool changed;

  if (!mapdeco_gotoline_table || !ptile
      || !(0 <= dir && dir <= direction8_max())) {
    return;
  }
  ptile_dest = mapstep(&(wld.map), ptile, dir);
  if (!ptile_dest) {
    return;
  }

  if (!gotoline_hash_lookup(mapdeco_gotoline_table, ptile, &pglc)) {
    pglc = gotoline_counter_new();
    gotoline_hash_insert(mapdeco_gotoline_table, ptile, pglc);
  }
  changed = (pglc->line_count[dir] < 1);
  pglc->line_count[dir]++;

  if (changed) {
    /* FIXME: Remove cast. */
    refresh_tile_mapcanvas((struct tile *) ptile, FALSE, FALSE);
    refresh_tile_mapcanvas((struct tile *) ptile_dest, FALSE, FALSE);
  }
}

/************************************************************************//**
  Removes a goto line from the given tile 'ptile' going in the direction
  'dir'. If this was the last line there, a mapview update is queued to
  erase the drawn line.
****************************************************************************/
void mapdeco_remove_gotoline(const struct tile *ptile,
                             enum direction8 dir)
{
  struct gotoline_counter *pglc;
  bool changed = FALSE;

  if (!mapdeco_gotoline_table || !ptile
      || !(0 <= dir && dir <= direction8_max())) {
    return;
  }

  if (!gotoline_hash_lookup(mapdeco_gotoline_table, ptile, &pglc)) {
    return;
  }

  pglc->line_count[dir]--;
  if (pglc->line_count[dir] <= 0) {
    pglc->line_count[dir] = 0;
    changed = TRUE;
  }

  if (changed) {
    /* FIXME: Remove the casts. */
    refresh_tile_mapcanvas((struct tile *) ptile, FALSE, FALSE);
    ptile = mapstep(&(wld.map), ptile, dir);
    if (ptile != NULL) {
      refresh_tile_mapcanvas((struct tile *) ptile, FALSE, FALSE);
    }
  }
}

/************************************************************************//**
  Set the map decorations for the given unit's goto route. A goto route
  consists of one or more goto lines, with each line being from the center
  of one tile to the center of another tile.
****************************************************************************/
void mapdeco_set_gotoroute(const struct unit *punit)
{
  const struct unit_order *porder;
  const struct tile *ptile;
  int i, ind;

  if (!punit || !unit_tile(punit) || !unit_has_orders(punit)
      || punit->orders.length < 1) {
    return;
  }

  ptile = unit_tile(punit);

  for (i = 0; ptile != NULL && i < punit->orders.length; i++) {
    if (punit->orders.index + i >= punit->orders.length
        && !punit->orders.repeat) {
      break;
    }

    ind = (punit->orders.index + i) % punit->orders.length;
    porder = &punit->orders.list[ind];
    if (porder->order != ORDER_MOVE) {
      /* FIXME: should display some indication of non-move orders here. */
      continue;
    }

    mapdeco_add_gotoline(ptile, porder->dir);
    ptile = mapstep(&(wld.map), ptile, porder->dir);
  }
}

/************************************************************************//**
  Returns TRUE if a goto line should be drawn from the given tile in the
  given direction.
****************************************************************************/
bool mapdeco_is_gotoline_set(const struct tile *ptile,
                             enum direction8 dir)
{
  struct gotoline_counter *pglc;

  if (!ptile || !(0 <= dir && dir <= direction8_max())
      || !mapdeco_gotoline_table) {
    return FALSE;
  }

  if (!gotoline_hash_lookup(mapdeco_gotoline_table, ptile, &pglc)) {
    return FALSE;
  }

  return pglc->line_count[dir] > 0;
}

/************************************************************************//**
  Clear all goto line map decorations and queues mapview updates for the
  affected tiles.
****************************************************************************/
void mapdeco_clear_gotoroutes(void)
{
  if (!mapdeco_gotoline_table) {
    return;
  }

  gotoline_hash_iterate(mapdeco_gotoline_table, ptile, pglc) {
    refresh_tile_mapcanvas(ptile, FALSE, FALSE);
    adjc_dir_iterate(&(wld.map), ptile, ptile_dest, dir) {
      if (pglc->line_count[dir] > 0) {
        refresh_tile_mapcanvas(ptile_dest, FALSE, FALSE);
      }
    } adjc_dir_iterate_end;
  } gotoline_hash_iterate_end;
  gotoline_hash_clear(mapdeco_gotoline_table);
}

/************************************************************************//**
  Called if the map in the GUI is resized.

  Returns TRUE iff the canvas was redrawn.
****************************************************************************/
bool map_canvas_resized(int width, int height)
{
  int old_tile_width = mapview.tile_width;
  int old_tile_height = mapview.tile_height;
  int old_width = mapview.width, old_height = mapview.height;
  int tile_width = (width + tileset_tile_width(tileset) * map_zoom - 1) /
    (tileset_tile_width(tileset) * map_zoom);
  int tile_height = (height + tileset_tile_height(tileset) * map_zoom - 1) /
    (tileset_tile_height(tileset) * map_zoom);
  int full_width = tile_width * tileset_tile_width(tileset) * map_zoom;
  int full_height = tile_height * tileset_tile_height(tileset) * map_zoom;
  bool tile_size_changed, size_changed, redrawn = FALSE;

  /* Resized */

  /* Since a resize is only triggered when the tile_*** changes, the canvas
   * width and height must include the entire backing store - otherwise
   * small resizings may lead to undrawn tiles. */
  mapview.tile_width = tile_width;
  mapview.tile_height = tile_height;
  mapview.width = width;
  mapview.height = height;
  mapview.store_width = full_width;
  mapview.store_height = full_height;

  /* Check for what's changed. */
  tile_size_changed = (tile_width != old_tile_width
 		       || tile_height != old_tile_height);
  size_changed = (width != old_width || height != old_height);

  /* If the tile size has changed, resize the canvas. */
  if (tile_size_changed) {
    if (mapview.store) {
      canvas_free(mapview.store);
      canvas_free(mapview.tmp_store);
    }
    mapview.store = canvas_create(full_width, full_height);
    canvas_set_zoom(mapview.store, map_zoom);
    canvas_put_rectangle(mapview.store,
                         get_color(tileset, COLOR_MAPVIEW_UNKNOWN),
                         0, 0, full_width / map_zoom, full_height / map_zoom);

    mapview.tmp_store = canvas_create(full_width, full_height);
    canvas_set_zoom(mapview.tmp_store, map_zoom);
  }

  if (!map_is_empty() && can_client_change_view()) {
    if (tile_size_changed) {
      if (center_tile != NULL) {
        int x_left, y_top;
        float gui_x, gui_y;

        index_to_map_pos(&x_left, &y_top, tile_index(center_tile));
        map_to_gui_pos(tileset, &gui_x, &gui_y, x_left, y_top);

        /* Put the center pixel of the tile at the exact center of the mapview. */
        gui_x -= (mapview.width - tileset_tile_width(tileset) * map_zoom) / 2;
        gui_y -= (mapview.height - tileset_tile_height(tileset) * map_zoom) / 2;

        calc_mapview_origin(&gui_x, &gui_y);
        mapview.gui_x0 = gui_x;
        mapview.gui_y0 = gui_y;
      }
      update_map_canvas_visible();
      center_tile_overviewcanvas();

      /* Do not draw to the screen here as that could cause problems
       * when we are only initially setting up the view and some widgets
       * are not yet ready. */
      unqueue_mapview_updates(FALSE);
      redrawn = TRUE;
    }

    /* If the width/height has changed, update the scrollbars even if
     * the backing store is not resized. */
    if (size_changed) {
      update_map_canvas_scrollbars_size();
      update_map_canvas_scrollbars();
    }
  }

  mapview.can_do_cached_drawing = can_do_cached_drawing();

  return redrawn;
}

/************************************************************************//**
  Sets up data for the mapview and overview.
****************************************************************************/
void init_mapcanvas_and_overview(void)
{
  /* Create a dummy map to make sure mapview.store is never NULL. */
  map_canvas_resized(1, 1);
}

/************************************************************************//**
  Frees resources allocated for mapview and overview
****************************************************************************/
void free_mapcanvas_and_overview(void)
{
  canvas_free(mapview.store);
  canvas_free(mapview.tmp_store);
}

/************************************************************************//**
  Return the desired width of the spaceship canvas.
****************************************************************************/
void get_spaceship_dimensions(int *width, int *height)
{
  struct sprite *sprite
    = get_spaceship_sprite(tileset, SPACESHIP_HABITATION);

  get_sprite_dimensions(sprite, width, height);
  *width *= 7;
  *height *= 7;
}

/************************************************************************//**
  Draw the spaceship onto the canvas.
****************************************************************************/
void put_spaceship(struct canvas *pcanvas, int canvas_x, int canvas_y,
                   const struct player *pplayer)
{
  int i, x, y;  
  const struct player_spaceship *ship = &pplayer->spaceship;
  int w, h;
  struct sprite *spr;
  struct tileset *t = tileset;

  spr = get_spaceship_sprite(t, SPACESHIP_HABITATION);
  get_sprite_dimensions(spr, &w, &h);

  canvas_put_rectangle(pcanvas,
                       get_color(tileset, COLOR_SPACESHIP_BACKGROUND),
                       0, 0, w * 7, h * 7);

  for (i = 0; i < NUM_SS_MODULES; i++) {
    const int j = i / 3;
    const int k = i % 3;

    if ((k == 0 && j >= ship->habitation)
	|| (k == 1 && j >= ship->life_support)
	|| (k == 2 && j >= ship->solar_panels)) {
      continue;
    }
    x = modules_info[i].x * w / 4 - w / 2;
    y = modules_info[i].y * h / 4 - h / 2;

    spr = (k == 0 ? get_spaceship_sprite(t, SPACESHIP_HABITATION)
           : k == 1 ? get_spaceship_sprite(t, SPACESHIP_LIFE_SUPPORT)
           : get_spaceship_sprite(t, SPACESHIP_SOLAR_PANEL));
    canvas_put_sprite_full(pcanvas, x, y, spr);
  }

  for (i = 0; i < NUM_SS_COMPONENTS; i++) {
    const int j = i / 2;
    const int k = i % 2;

    if ((k == 0 && j >= ship->fuel)
	|| (k == 1 && j >= ship->propulsion)) {
      continue;
    }
    x = components_info[i].x * w / 4 - w / 2;
    y = components_info[i].y * h / 4 - h / 2;

    spr = ((k == 0) ? get_spaceship_sprite(t, SPACESHIP_FUEL)
           : get_spaceship_sprite(t, SPACESHIP_PROPULSION));

    canvas_put_sprite_full(pcanvas, x, y, spr);

    if (k && ship->state == SSHIP_LAUNCHED) {
      spr = get_spaceship_sprite(t, SPACESHIP_EXHAUST);
      canvas_put_sprite_full(pcanvas, x + w, y, spr);
    }
  }

  for (i = 0; i < NUM_SS_STRUCTURALS; i++) {
    if (!BV_ISSET(ship->structure, i)) {
      continue;
    }
    x = structurals_info[i].x * w / 4 - w / 2;
    y = structurals_info[i].y * h / 4 - h / 2;

    spr = get_spaceship_sprite(t, SPACESHIP_STRUCTURAL);
    canvas_put_sprite_full(pcanvas, x, y, spr);
  }
}

/****************************************************************************
  Map link mark module: it makes link marks when a link is sent by chating,
  or restore a mark with clicking a link on the chatline.
****************************************************************************/
struct link_mark {
  enum text_link_type type;     /* The target type. */
  int id;                       /* The city or unit id, or tile index. */
  int turn_counter;             /* The turn counter before it disappears. */
};

#define SPECLIST_TAG link_mark
#define SPECLIST_TYPE struct link_mark
#include "speclist.h"
#define link_marks_iterate(pmark) \
  TYPED_LIST_ITERATE(struct link_mark, link_marks, pmark)
#define link_marks_iterate_end LIST_ITERATE_END

static struct link_mark_list *link_marks = NULL;

/************************************************************************//**
  Find a link mark in the list.
****************************************************************************/
static struct link_mark *link_mark_find(enum text_link_type type, int id)
{
  link_marks_iterate(pmark) {
    if (pmark->type == type && pmark->id == id) {
      return pmark;
    }
  } link_marks_iterate_end;

  return NULL;
}

/************************************************************************//**
  Create a new link mark.
****************************************************************************/
static struct link_mark *link_mark_new(enum text_link_type type,
                                       int id, int turns)
{
  struct link_mark *pmark = fc_malloc(sizeof(struct link_mark));

  pmark->type = type;
  pmark->id = id;
  pmark->turn_counter = turns;

  return pmark;
}

/************************************************************************//**
  Remove a link mark.
****************************************************************************/
static void link_mark_destroy(struct link_mark *pmark)
{
  free(pmark);
}

/************************************************************************//**
  Returns the location of the pointed mark.
****************************************************************************/
static struct tile *link_mark_tile(const struct link_mark *pmark)
{
  switch (pmark->type) {
  case TLT_CITY:
    {
      struct city *pcity = game_city_by_number(pmark->id);
      return pcity ? pcity->tile : NULL;
    }
  case TLT_TILE:
    return index_to_tile(&(wld.map), pmark->id);
  case TLT_UNIT:
    {
      struct unit *punit = game_unit_by_number(pmark->id);
      return punit ? unit_tile(punit) : NULL;
    }
  }
  return NULL;
}

/************************************************************************//**
  Returns the color of the pointed mark.
****************************************************************************/
static struct color *link_mark_color(const struct link_mark *pmark)
{
  switch (pmark->type) {
  case TLT_CITY:
    return get_color(tileset, COLOR_MAPVIEW_CITY_LINK);
  case TLT_TILE:
    return get_color(tileset, COLOR_MAPVIEW_TILE_LINK);
  case TLT_UNIT:
    return get_color(tileset, COLOR_MAPVIEW_UNIT_LINK);
  }
  return NULL;
}

/************************************************************************//**
  Print a link mark.
****************************************************************************/
static void link_mark_draw(const struct link_mark *pmark)
{
  int width = tileset_tile_width(tileset) * map_zoom;
  int height = tileset_tile_height(tileset) * map_zoom;
  int xd = width / 20, yd = height / 20;
  int xlen = width / 3, ylen = height / 3;
  float canvas_x, canvas_y;
  int x_left, x_right, y_top, y_bottom;
  struct tile *ptile = link_mark_tile(pmark);
  struct color *pcolor = link_mark_color(pmark);

  if (!ptile || !tile_to_canvas_pos(&canvas_x, &canvas_y, ptile)) {
    return;
  }

  x_left = canvas_x + xd;
  x_right = canvas_x + width - xd;
  y_top = canvas_y + yd;
  y_bottom = canvas_y + height - yd;

  /* XXX: canvas_put_line doesn't currently take map_zoom into account
   * itself, but it probably should? If so these will need adjusting */
  canvas_put_line(mapview.store, pcolor, LINE_TILE_FRAME, x_left, y_top, xlen, 0);
  canvas_put_line(mapview.store, pcolor, LINE_TILE_FRAME, x_left, y_top, 0, ylen);
  
  canvas_put_line(mapview.store, pcolor, LINE_TILE_FRAME, x_right, y_top, -xlen, 0);
  canvas_put_line(mapview.store, pcolor, LINE_TILE_FRAME, x_right, y_top, 0, ylen);
  
  canvas_put_line(mapview.store, pcolor, LINE_TILE_FRAME, x_left, y_bottom, xlen, 0);
  canvas_put_line(mapview.store, pcolor, LINE_TILE_FRAME, x_left, y_bottom, 0, -ylen);
  
  canvas_put_line(mapview.store, pcolor, LINE_TILE_FRAME, x_right, y_bottom, -xlen, 0);
  canvas_put_line(mapview.store, pcolor, LINE_TILE_FRAME, x_right, y_bottom, 0, -ylen);
}

/************************************************************************//**
  Initialize the link marks.
****************************************************************************/
void link_marks_init(void)
{
  if (link_marks) {
    link_marks_free();
  }

  link_marks = link_mark_list_new_full(link_mark_destroy);
}

/************************************************************************//**
  Free the link marks.
****************************************************************************/
void link_marks_free(void)
{
  if (!link_marks) {
    return;
  }

  link_mark_list_destroy(link_marks);
  link_marks = NULL;
}

/************************************************************************//**
  Draw all link marks.
****************************************************************************/
void link_marks_draw_all(void)
{
  link_marks_iterate(pmark) {
    link_mark_draw(pmark);
  } link_marks_iterate_end;
}

/************************************************************************//**
  Clear all visible links.
****************************************************************************/
void link_marks_clear_all(void)
{
  link_mark_list_clear(link_marks);
  update_map_canvas_visible();
}

/************************************************************************//**
  Clear all visible links.
****************************************************************************/
void link_marks_decrease_turn_counters(void)
{
  link_marks_iterate(pmark) {
    if (--pmark->turn_counter <= 0) {
      link_mark_list_remove(link_marks, pmark);
    }
  } link_marks_iterate_end;

  /* update_map_canvas_visible(); not needed here. */
}

/************************************************************************//**
  Add a visible link for 2 turns.
****************************************************************************/
void link_mark_add_new(enum text_link_type type, int id)
{
  struct link_mark *pmark = link_mark_find(type, id);
  struct tile *ptile;

  if (pmark) {
    /* Already displayed, but maybe increase the turn counter. */
    pmark->turn_counter = MAX(pmark->turn_counter, 2);
    return;
  }

  pmark = link_mark_new(type, id, 2);
  link_mark_list_append(link_marks, pmark);
  ptile = link_mark_tile(pmark);
  if (ptile && tile_visible_mapcanvas(ptile)) {
    refresh_tile_mapcanvas(ptile, FALSE, FALSE);
  }
}

/************************************************************************//**
  Add a visible link for 1 turn.
****************************************************************************/
void link_mark_restore(enum text_link_type type, int id)
{
  struct link_mark *pmark;
  struct tile *ptile;

  if (link_mark_find(type, id)) {
    return;
  }

  pmark = link_mark_new(type, id, 1);
  link_mark_list_append(link_marks, pmark);
  ptile = link_mark_tile(pmark);
  if (ptile && tile_visible_mapcanvas(ptile)) {
    refresh_tile_mapcanvas(ptile, FALSE, FALSE);
  }
}

/************************************************************************//**
  Are the topology and tileset compatible?
****************************************************************************/
enum topo_comp_lvl tileset_map_topo_compatible(int topology_id,
                                               struct tileset *tset)
{
  int tileset_topology;

  if (tileset_hex_width(tset) > 0) {
    fc_assert(tileset_is_isometric(tset));
    tileset_topology = TF_HEX | TF_ISO;
  } else if (tileset_hex_height(tset) > 0) {
    fc_assert(tileset_is_isometric(tset));
    tileset_topology = TF_HEX;
  } else if (tileset_is_isometric(tset)) {
    tileset_topology = TF_ISO;
  } else {
    tileset_topology = 0;
  }

  if (tileset_topology & TF_HEX) {
    if ((topology_id & (TF_HEX | TF_ISO)) == tileset_topology) {
      return TOPO_COMPATIBLE;
    }

    /* Hex topology must match for both hexness and iso/non-iso */
    return TOPO_INCOMP_HARD;
  }

  if (topology_id & TF_HEX) {
    return TOPO_INCOMP_HARD;
  }

  if ((topology_id & TF_ISO) != (tileset_topology & TF_ISO)) {
    /* Non-hex iso/non-iso incompatibility is a soft one */
    return TOPO_INCOMP_SOFT;
  }

  return TOPO_COMPATIBLE;
}

/************************************************************************//**
  Set frame by frame animation mode on.
****************************************************************************/
void set_frame_by_frame_animation(void)
{
  frame_by_frame_animation = TRUE;
}
