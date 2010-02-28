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

/**********************************************************************
  Functions for handling the tilespec files which describe
  the files and contents of tilesets.
  original author: David Pfitzner <dwp@mso.anu.edu.au>
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>		/* exit */
#include <string.h>

/* utility */
#include "astring.h"
#include "capability.h"
#include "fcintl.h"
#include "hash.h"
#include "log.h"
#include "mem.h"
#include "rand.h"
#include "registry.h"
#include "shared.h"
#include "support.h"

/* common */
#include "base.h"
#include "game.h"		/* game.control.styles_count */
#include "government.h"
#include "map.h"
#include "movement.h"
#include "nation.h"
#include "player.h"
#include "specialist.h"
#include "unit.h"
#include "unitlist.h"

#include "dialogs_g.h"
#include "graphics_g.h"
#include "gui_main_g.h"
#include "mapview_g.h"		/* for update_map_canvas_visible */
#include "themes_g.h"

/* client */
#include "client_main.h"
#include "climap.h"		/* for client_tile_get_known() */
#include "control.h"		/* for fill_xxx */
#include "editor.h"
#include "goto.h"
#include "options.h"		/* for fill_xxx */
#include "themes_common.h"

#include "citydlg_common.h"	/* for generate_citydlg_dimensions() */
#include "tilespec.h"

#define TILESPEC_CAPSTR "+tilespec4+2007.Oct.26 duplicates_ok"
/*
 * Tilespec capabilities acceptable to this program:
 *
 * +tilespec4     -  basic format; required
 *
 * duplicates_ok  -  we can handle existence of duplicate tags
 *                   (lattermost tag which appears is used; tilesets which
 *		     have duplicates should specify "+duplicates_ok")
 */

#define SPEC_CAPSTR "+spec3"
/*
 * Individual spec file capabilities acceptable to this program:
 * +spec3          -  basic format, required
 */

#define TILESPEC_SUFFIX ".tilespec"
#define TILE_SECTION_PREFIX "tile_"

/* This the way directional indices are now encoded: */
#define MAX_INDEX_CARDINAL 		64
#define MAX_INDEX_HALF                  16
#define MAX_INDEX_VALID			256

#define NUM_TILES_HP_BAR 11
#define NUM_TILES_DIGITS 10
#define NUM_TILES_SELECT 4
#define MAX_NUM_CITIZEN_SPRITES 6

/* This could be moved to common/map.h if there's more use for it. */
enum direction4 {
  DIR4_NORTH = 0, DIR4_SOUTH, DIR4_EAST, DIR4_WEST
};
static const char direction4letters[4] = "udrl";

static const int DIR4_TO_DIR8[4] =
    { DIR8_NORTH, DIR8_SOUTH, DIR8_EAST, DIR8_WEST };

enum match_style {
  MATCH_NONE,
  MATCH_SAME,		/* "boolean" match */
  MATCH_PAIR,
  MATCH_FULL
};

enum sprite_type {
  CELL_WHOLE,		/* entire tile */
  CELL_CORNER		/* corner of tile */
};

struct drawing_data {
  char *name;
  char *mine_tag;

  int num_layers; /* 1 thru MAX_NUM_LAYERS. */
#define MAX_NUM_LAYERS 3

  struct drawing_layer {
    bool is_tall;
    int offset_x, offset_y;

    enum match_style match_style;
    int match_index[1 + MATCH_FULL];
    int match_indices; /* 0 = no match_type, 1 = no match_with */

    enum sprite_type sprite_type;

    struct sprite_vector base;
    struct sprite *match[MAX_INDEX_CARDINAL];
    struct sprite **cells;
  } layer[MAX_NUM_LAYERS];

  bool is_reversed;

  int blending; /* layer, 0 = none */
  struct sprite *blender;
  struct sprite *blend[4]; /* indexed by a direction4 */

  struct sprite *mine;
};

struct city_style_threshold {
  int city_size;
  struct sprite *sprite;
};

struct city_sprite {
  struct {
    int land_num_thresholds;
    struct city_style_threshold *land_thresholds;
    int oceanic_num_thresholds;
    struct city_style_threshold *oceanic_thresholds;
  } *styles;
  int num_styles;
};

struct named_sprites {
  struct sprite
    *indicator[INDICATOR_COUNT][NUM_TILES_PROGRESS],
    *treaty_thumb[2],     /* 0=disagree, 1=agree */
    *arrow[ARROW_LAST], /* 0=right arrow, 1=plus, 2=minus */

    *icon[ICON_COUNT],

    /* The panel sprites for showing tax % allocations. */
    *tax_luxury, *tax_science, *tax_gold,
    *dither_tile;     /* only used for isometric view */

  struct {
    struct sprite
      *tile,
      *worked_tile,
      *unworked_tile;
  } mask;

  struct sprite *tech[A_LAST];
  struct sprite *building[B_LAST];
  struct sprite *government[G_MAGIC];
  struct sprite *unittype[U_LAST];
  struct sprite *resource[MAX_NUM_RESOURCES];

  struct sprite_vector nation_flag;
  struct sprite_vector nation_shield;

  struct citizen_graphic {
    /* Each citizen type has up to MAX_NUM_CITIZEN_SPRITES different
     * sprites, as defined by the tileset. */
    int count;
    struct sprite *sprite[MAX_NUM_CITIZEN_SPRITES];
  } citizen[CITIZEN_LAST], specialist[SP_MAX];
  struct sprite *spaceship[SPACESHIP_COUNT];
  struct {
    int hot_x, hot_y;
    struct sprite *frame[NUM_CURSOR_FRAMES];
  } cursor[CURSOR_LAST];
  struct {
    struct sprite
      /* for roadstyle 0 */
      *dir[8],     /* all entries used */
      /* for roadstyle 1 */
      *even[MAX_INDEX_HALF],    /* first unused */
      *odd[MAX_INDEX_HALF],     /* first unused */
      /* for roadstyle 0 and 1 */
      *isolated,
      *corner[8], /* Indexed by direction; only non-cardinal dirs used. */
      *total[MAX_INDEX_VALID];     /* includes all possibilities */
  } road, rail;
  struct {
    struct sprite_vector unit;
    struct sprite *nuke;
  } explode;
  struct {
    struct sprite
      *hp_bar[NUM_TILES_HP_BAR],
      *vet_lev[MAX_VET_LEVELS],
      *select[NUM_TILES_SELECT],
      *auto_attack,
      *auto_settler,
      *auto_explore,
      *fallout,
      *fortified,
      *fortifying,
      *go_to,			/* goto is a C keyword :-) */
      *irrigate,
      *mine,
      *pillage,
      *pollution,
      *road,
      *sentry,
      *stack,
      *loaded,
      *transform,
      *connect,
      *patrol,
      *battlegroup[MAX_NUM_BATTLEGROUPS],
      *lowfuel,
      *tired;
  } unit;
  struct {
    struct sprite
      *unhappy[2],
      *output[O_LAST][2];
  } upkeep;
  struct {
    struct sprite
      *disorder,
      *size[NUM_TILES_DIGITS],
      *size_tens[NUM_TILES_DIGITS],		/* first unused */
      *tile_foodnum[NUM_TILES_DIGITS],
      *tile_shieldnum[NUM_TILES_DIGITS],
      *tile_tradenum[NUM_TILES_DIGITS];
    struct city_sprite
      *tile,
      *wall,
      *occupied;
    struct sprite_vector worked_tile_overlay;
    struct sprite_vector unworked_tile_overlay;
  } city;
  struct citybar_sprites citybar;
  struct editor_sprites editor;
  struct {
    struct sprite
      *turns[NUM_TILES_DIGITS],
      *turns_tens[NUM_TILES_DIGITS];
  } path;
  struct {
    struct sprite *attention;
  } user;
  struct {
    struct sprite
      *farmland[MAX_INDEX_CARDINAL],
      *irrigation[MAX_INDEX_CARDINAL],
      *pollution,
      *village,
      *fallout,
      *fog,
      **fullfog,
      *spec_river[MAX_INDEX_CARDINAL],
      *darkness[MAX_INDEX_CARDINAL],         /* first unused */
      *river_outlet[4];		/* indexed by enum direction4 */
  } tx;				/* terrain extra */
  struct {
    struct sprite
      *background,
      *middleground,
      *foreground,
      *activity;
  } bases[MAX_BASE_TYPES];
  struct {
    struct sprite
      *main[EDGE_COUNT],
      *city[EDGE_COUNT],
      *worked[EDGE_COUNT],
      *unavailable,
      *selected[EDGE_COUNT],
      *coastline[EDGE_COUNT],
      *borders[EDGE_COUNT][2],
      *player_borders[MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS][EDGE_COUNT][2];
  } grid;
  struct {
    struct sprite *player[MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS];
    struct sprite *background; /* Generic background */
  } backgrounds;
  struct {
    struct sprite_vector overlays;
    struct sprite *background; /* Generic background color */
    struct sprite *player[MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS];
  } colors;

  struct drawing_data *drawing[MAX_NUM_ITEMS];
};

/* Don't reorder this enum since tilesets depend on it. */
enum fog_style {
  FOG_AUTO, /* Fog is automatically appended by the code. */
  FOG_SPRITE, /* A single fog sprite is provided by the tileset (tx.fog). */
  FOG_NONE /* No fog. */
};

/* Darkness style.  Don't reorder this enum since tilesets depend on it. */
enum darkness_style {
  /* No darkness sprites are drawn. */
  DARKNESS_NONE = 0,

  /* 1 sprite that is split into 4 parts and treated as a darkness4.  Only
   * works in iso-view. */
  DARKNESS_ISORECT = 1,

  /* 4 sprites, one per direction.  More than one sprite per tile may be
   * drawn. */
  DARKNESS_CARD_SINGLE = 2,

  /* 15=2^4-1 sprites.  A single sprite is drawn, chosen based on whether
   * there's darkness in _each_ of the cardinal directions. */
  DARKNESS_CARD_FULL = 3,

  /* Corner darkness & fog.  3^4 = 81 sprites. */
  DARKNESS_CORNER = 4
};

struct specfile {
  struct sprite *big_sprite;
  char *file_name;
};

#define SPECLIST_TAG specfile
#define SPECLIST_TYPE struct specfile
#include "speclist.h"

#define specfile_list_iterate(list, pitem) \
    TYPED_LIST_ITERATE(struct specfile, list, pitem)
#define specfile_list_iterate_end  LIST_ITERATE_END

/* 
 * Information about an individual sprite. All fields except 'sprite' are
 * filled at the time of the scan of the specfile. 'Sprite' is
 * set/cleared on demand in load_sprite/unload_sprite.
 */
struct small_sprite {
  int ref_count;

  /* The sprite is in this file. */
  char *file;

  /* Or, the sprite is in this file at the location. */
  struct specfile *sf;
  int x, y, width, height;

  /* A little more (optional) data. */
  int hot_x, hot_y;

  struct sprite *sprite;
};

#define SPECLIST_TAG small_sprite
#define SPECLIST_TYPE struct small_sprite
#include "speclist.h"

#define small_sprite_list_iterate(list, pitem) \
    TYPED_LIST_ITERATE(struct small_sprite, list, pitem)
#define small_sprite_list_iterate_end  LIST_ITERATE_END

struct tileset {
  char name[512];
  int priority;

  bool is_isometric;
  int hex_width, hex_height;

  int normal_tile_width, normal_tile_height;
  int full_tile_width, full_tile_height;
  int unit_tile_width, unit_tile_height;
  int small_sprite_width, small_sprite_height;

  char *main_intro_filename;
  char *minimap_intro_filename;

  int city_names_font_size, city_productions_font_size;

  int roadstyle;
  enum fog_style fogstyle;
  enum darkness_style darkness_style;

  int unit_flag_offset_x, unit_flag_offset_y;
  int city_flag_offset_x, city_flag_offset_y;
  int unit_offset_x, unit_offset_y;

  int citybar_offset_y;

#define NUM_CORNER_DIRS 4
#define TILES_PER_CORNER 4
  int num_valid_tileset_dirs, num_cardinal_tileset_dirs;
  int num_index_valid, num_index_cardinal;
  enum direction8 valid_tileset_dirs[8], cardinal_tileset_dirs[8];

  struct tileset_layer {
    char **match_types;
    int match_count;
  } layers[MAX_NUM_LAYERS];

  struct specfile_list *specfiles;
  struct small_sprite_list *small_sprites;

  /*
   * This hash table maps tilespec tags to struct small_sprites.
   */
  struct hash_table *sprite_hash;

  /* This hash table maps terrain graphic strings to drawing data. */
  struct hash_table *tile_hash;

  struct named_sprites sprites;

  struct color_system *color_system;
  
  int num_prefered_themes;
  char** prefered_themes;
};

struct tileset *tileset;

int focus_unit_state = 0;

/****************************************************************************
  Hash callback for freeing key
****************************************************************************/
static void sprite_hash_free_key(void *key)
{
  free(key);
}

/****************************************************************************
  Return the name of the given tileset.
****************************************************************************/
const char *tileset_get_name(const struct tileset *t)
{
  return t->name;
}

/****************************************************************************
  Return whether the current tileset is isometric.
****************************************************************************/
bool tileset_is_isometric(const struct tileset *t)
{
  return t->is_isometric;
}

/****************************************************************************
  Return the hex_width of the current tileset.  For hex tilesets this value
  will be > 0 and is_isometric will be set.
****************************************************************************/
int tileset_hex_width(const struct tileset *t)
{
  return t->hex_width;
}

/****************************************************************************
  Return the hex_height of the current tileset.  For iso-hex tilesets this
  value will be > 0 and is_isometric will be set.
****************************************************************************/
int tileset_hex_height(const struct tileset *t)
{
  return t->hex_height;
}

/****************************************************************************
  Return the tile width of the current tileset.  This is the tesselation
  width of the tiled plane.  This means it's the width of the bounding box
  of the basic map tile.

  For best results:
    - The value should be even (or a multiple of 4 in iso-view).
    - In iso-view, the width should be twice the height (to give a
      perspective of 30 degrees above the horizon).
    - In non-iso-view, width and height should be equal (overhead
      perspective).
    - In hex or iso-hex view, remember this is the tesselation vector.
      hex_width and hex_height then give the size of the side of the
      hexagon.  Calculating the dimensions of a "regular" hexagon or
      iso-hexagon may be tricky.
  However these requirements are not absolute and callers should not
  depend on them (although some do).
****************************************************************************/
int tileset_tile_width(const struct tileset *t)
{
  return t->normal_tile_width;
}

/****************************************************************************
  Return the tile height of the current tileset.  This is the tesselation
  height of the tiled plane.  This means it's the height of the bounding box
  of the basic map tile.

  See also tileset_tile_width.
****************************************************************************/
int tileset_tile_height(const struct tileset *t)
{
  return t->normal_tile_height;
}

/****************************************************************************
  Return the full tile width of the current tileset.  This is the maximum
  width that any mapview sprite will have.

  Note: currently this is always equal to the tile width.
****************************************************************************/
int tileset_full_tile_width(const struct tileset *t)
{
  return t->full_tile_width;
}

/****************************************************************************
  Return the full tile height of the current tileset.  This is the maximum
  height that any mapview sprite will have.  This may be greater than the
  tile width in which case the extra area is above the "normal" tile.

  Some callers assume the full height is 50% larger than the height in
  iso-view, and equal in non-iso view.
****************************************************************************/
int tileset_full_tile_height(const struct tileset *t)
{
  return t->full_tile_height;
}

/****************************************************************************
  Return the unit tile width of the current tileset.
****************************************************************************/
int tileset_unit_width(const struct tileset *t)
{
  return t->unit_tile_width;
}

/****************************************************************************
  Return the unit tile height of the current tileset.
****************************************************************************/
int tileset_unit_height(const struct tileset *t)
{
  return t->unit_tile_height;
}

/****************************************************************************
  Return the small sprite width of the current tileset.  The small sprites
  are used for various theme graphics (e.g., citymap citizens/specialists
  as well as panel indicator icons).
****************************************************************************/
int tileset_small_sprite_width(const struct tileset *t)
{
  return t->small_sprite_width;
}

/****************************************************************************
  Return the offset from the origin of the city tile at which to place the
  citybar text.
****************************************************************************/
int tileset_citybar_offset_y(const struct tileset *t)
{
  return t->citybar_offset_y;
}

/****************************************************************************
  Return the small sprite height of the current tileset.  The small sprites
  are used for various theme graphics (e.g., citymap citizens/specialists
  as well as panel indicator icons).
****************************************************************************/
int tileset_small_sprite_height(const struct tileset *t)
{
  return t->small_sprite_height;
}

/****************************************************************************
  Return the path within the data directories where the main intro graphics
  file can be found.  (It is left up to the GUI code to load and unload this
  file.)
****************************************************************************/
const char *tileset_main_intro_filename(const struct tileset *t)
{
  return t->main_intro_filename;
}

/****************************************************************************
  Return the path within the data directories where the mini intro graphics
  file can be found.  (It is left up to the GUI code to load and unload this
  file.)
****************************************************************************/
const char *tileset_mini_intro_filename(const struct tileset *t)
{
  return t->minimap_intro_filename;
}

/****************************************************************************
  Return the number of possible colors for city overlays.
****************************************************************************/
int tileset_num_city_colors(const struct tileset *t)
{
  return t->sprites.city.worked_tile_overlay.size;
}

/**************************************************************************
  Initialize.
**************************************************************************/
static struct tileset *tileset_new(void)
{
  struct tileset *t = fc_calloc(1, sizeof(*t));

  t->specfiles = specfile_list_new();
  t->small_sprites = small_sprite_list_new();
  return t;
}

/**************************************************************************
  Return the tileset name of the direction.  This is similar to
  dir_get_name but you shouldn't change this or all tilesets will break.
**************************************************************************/
static const char *dir_get_tileset_name(enum direction8 dir)
{
  switch (dir) {
  case DIR8_NORTH:
    return "n";
  case DIR8_NORTHEAST:
    return "ne";
  case DIR8_EAST:
    return "e";
  case DIR8_SOUTHEAST:
    return "se";
  case DIR8_SOUTH:
    return "s";
  case DIR8_SOUTHWEST:
    return "sw";
  case DIR8_WEST:
    return "w";
  case DIR8_NORTHWEST:
    return "nw";
  }
  assert(0);
  return "";
}

/****************************************************************************
  Return TRUE iff the dir is valid in this tileset.
****************************************************************************/
static bool is_valid_tileset_dir(const struct tileset *t,
				 enum direction8 dir)
{
  if (t->hex_width > 0) {
    return dir != DIR8_NORTHEAST && dir != DIR8_SOUTHWEST;
  } else if (t->hex_height > 0) {
    return dir != DIR8_NORTHWEST && dir != DIR8_SOUTHEAST;
  } else {
    return TRUE;
  }
}

/****************************************************************************
  Return TRUE iff the dir is cardinal in this tileset.

  "Cardinal", in this sense, means that a tile will share a border with
  another tile in the direction rather than sharing just a single vertex.
****************************************************************************/
static bool is_cardinal_tileset_dir(const struct tileset *t,
				    enum direction8 dir)
{
  if (t->hex_width > 0 || t->hex_height > 0) {
    return is_valid_tileset_dir(t, dir);
  } else {
    return (dir == DIR8_NORTH || dir == DIR8_EAST
	    || dir == DIR8_SOUTH || dir == DIR8_WEST);
  }
}

/**********************************************************************
  Returns a static list of tilesets available on the system by
  searching all data directories for files matching TILESPEC_SUFFIX.
  The list is NULL-terminated.
***********************************************************************/
const char **get_tileset_list(void)
{
  static const char **tilesets = NULL;

  if (!tilesets) {
    /* Note: this means you must restart the client after installing a new
       tileset. */
    char **list = datafilelist(TILESPEC_SUFFIX);
    int i, count = 0;

    for (i = 0; list[i]; i++) {
      struct tileset *t = tileset_read_toplevel(list[i], FALSE);

      if (t) {
 	tilesets = fc_realloc(tilesets, (count + 1) * sizeof(*tilesets));
 	tilesets[count] = list[i];
 	count++;
 	tileset_free(t);
      } else {
	free(list[i]);
      }
    }
    free(list);

    tilesets = fc_realloc(tilesets, (count + 1) * sizeof(*tilesets));
    tilesets[count] = NULL;
  }

  return tilesets;
}

/**********************************************************************
  Gets full filename for tilespec file, based on input name.
  Returned data is allocated, and freed by user as required.
  Input name may be null, in which case uses default.
  Falls back to default if can't find specified name;
  dies if can't find default.
***********************************************************************/
static char *tilespec_fullname(const char *tileset_name)
{
  if (tileset_name) {
    char fname[strlen(tileset_name) + strlen(TILESPEC_SUFFIX) + 1], *dname;

    my_snprintf(fname, sizeof(fname),
		"%s%s", tileset_name, TILESPEC_SUFFIX);

    dname = datafilename(fname);

    if (dname) {
      return mystrdup(dname);
    }
  }

  return NULL;
}

/**********************************************************************
  Checks options in filename match what we require and support.
  Die if not.
  'which' should be "tilespec" or "spec".
***********************************************************************/
static bool check_tilespec_capabilities(struct section_file *file,
					const char *which,
					const char *us_capstr,
					const char *filename,
                                        bool verbose)
{
  int log_level = verbose ? LOG_ERROR : LOG_DEBUG;

  char *file_capstr = secfile_lookup_str(file, "%s.options", which);
  
  if (!has_capabilities(us_capstr, file_capstr)) {
    freelog(log_level, "\"%s\": %s file appears incompatible:",
	    filename, which);
    freelog(log_level, "  datafile options: %s", file_capstr);
    freelog(log_level, "  supported options: %s", us_capstr);
    return FALSE;
  }
  if (!has_capabilities(file_capstr, us_capstr)) {
    freelog(log_level, "\"%s\": %s file requires option(s)"
			 " that client doesn't support:",
	    filename, which);
    freelog(log_level, "  datafile options: %s", file_capstr);
    freelog(log_level, "  supported options: %s", us_capstr);
    return FALSE;
  }

  return TRUE;
}

/**********************************************************************
  Frees the tilespec toplevel data, in preparation for re-reading it.

  See tilespec_read_toplevel().
***********************************************************************/
static void tileset_free_toplevel(struct tileset *t)
{
  int i, j;

  if (t->main_intro_filename) {
    free(t->main_intro_filename);
    t->main_intro_filename = NULL;
  }
  if (t->minimap_intro_filename) {
    free(t->minimap_intro_filename);
    t->minimap_intro_filename = NULL;
  }
  
  if (t->prefered_themes) {
    for (i = 0; i < t->num_prefered_themes; i++) {
      free(t->prefered_themes[i]);
    }
    free(t->prefered_themes);
    t->prefered_themes = NULL;
  }
  t->num_prefered_themes = 0;

  if (t->tile_hash) {
    while (hash_num_entries(t->tile_hash) > 0) {
      struct drawing_data *draw
         = (void *)hash_value_by_number(t->tile_hash, 0);

      hash_delete_entry(t->tile_hash, draw->name);
      free(draw->name);
      if (draw->mine_tag) {
	free(draw->mine_tag);
      }
      for (i = 0; i < 4; i++) {
	if (draw->blend[i]) {
	  free_sprite(draw->blend[i]);
	}
      }
      for (i = 0; i < draw->num_layers; i++) {
	sprite_vector_free(&draw->layer[i].base);
      }
      free(draw);
    }
    hash_free(t->tile_hash);
    t->tile_hash = NULL; /* Helpful for sanity. */
  }

  for (i = 0; i < MAX_NUM_LAYERS; i++) {
    struct tileset_layer *tslp = &t->layers[i];

    if (tslp->match_types) {
      for (j = 0; j < tslp->match_count; j++) {
	free(tslp->match_types[j]);
      }
      free(tslp->match_types);
      tslp->match_types = NULL;
    }
  }

  if (t->color_system) {
    color_system_free(t->color_system);
    t->color_system = NULL;
  }
}

/**************************************************************************
  Clean up.
**************************************************************************/
void tileset_free(struct tileset *t)
{
  tileset_free_tiles(t);
  tileset_free_toplevel(t);
  specfile_list_free(t->specfiles);
  small_sprite_list_free(t->small_sprites);
  free(t);
}

/**********************************************************************
  Read a new tilespec in when first starting the game.

  Call this function with the (guessed) name of the tileset, when
  starting the client.
***********************************************************************/
void tilespec_try_read(const char *tileset_name, bool verbose)
{
  if (!(tileset = tileset_read_toplevel(tileset_name, verbose))) {
    char **list = datafilelist(TILESPEC_SUFFIX);
    int i;

    for (i = 0; list[i]; i++) {
      struct tileset *t = tileset_read_toplevel(list[i], FALSE);

      if (t) {
	if (!tileset || t->priority > tileset->priority) {
	  tileset = t;
	} else {
	  tileset_free(t);
	}
      }
      free(list[i]);
    }
    free(list);

    if (!tileset) {
      freelog(LOG_FATAL, _("No usable default tileset found, aborting!"));
      exit(EXIT_FAILURE);
    }

    freelog(LOG_VERBOSE, "Trying tileset \"%s\".", tileset->name);
  }
  sz_strlcpy(default_tileset_name, tileset_get_name(tileset));
}

/**********************************************************************
  Read a new tilespec in from scratch.

  Unlike the initial reading code, which reads pieces one at a time,
  this gets rid of the old data and reads in the new all at once.  If the
  new tileset fails to load the old tileset may be reloaded; otherwise the
  client will exit.  If a NULL name is given the current tileset will be
  reread.

  It will also call the necessary functions to redraw the graphics.
***********************************************************************/
void tilespec_reread(const char *new_tileset_name)
{
  int id;
  struct tile *center_tile;
  enum client_states state = client_state();
  const char *name = new_tileset_name ? new_tileset_name : tileset->name;
  char tileset_name[strlen(name) + 1], old_name[strlen(tileset->name) + 1];

  /* Make local copies since these values may be freed down below */
  sz_strlcpy(tileset_name, name);
  sz_strlcpy(old_name, tileset->name);

  freelog(LOG_NORMAL, _("Loading tileset \"%s\"."), tileset_name);

  /* Step 0:  Record old data.
   *
   * We record the current mapcanvas center, etc.
   */
  center_tile = get_center_tile_mapcanvas();

  /* Step 1:  Cleanup.
   *
   * We free all old data in preparation for re-reading it.
   */
  tileset_free_tiles(tileset);
  tileset_free_toplevel(tileset);

  /* Step 2:  Read.
   *
   * We read in the new tileset.  This should be pretty straightforward.
   */
  if (!(tileset = tileset_read_toplevel(tileset_name, FALSE))) {
    if (!(tileset = tileset_read_toplevel(old_name, FALSE))) {
      die("Failed to re-read the currently loaded tileset.");
    }
  }
  sz_strlcpy(default_tileset_name, tileset->name);
  tileset_load_tiles(tileset);
  tileset_use_prefered_theme(tileset);

  /* Step 3: Setup
   *
   * This is a seriously sticky problem.  On startup, we build a hash
   * from all the sprite data. Then, when we connect to a server, the
   * server sends us ruleset data a piece at a time and we use this data
   * to assemble the sprite structures.  But if we change while connected
   *  we have to reassemble all of these.  This should just involve
   * calling tilespec_setup_*** on everything.  But how do we tell what
   * "everything" is?
   *
   * The below code just does things straightforwardly, by setting up
   * each possible sprite again.  Hopefully it catches everything, and
   * doesn't mess up too badly if we change tilesets while not connected
   * to a server.
   */
  if (state < C_S_RUNNING) {
    /* The ruleset data is not sent until this point. */
    return;
  }
  terrain_type_iterate(pterrain) {
    tileset_setup_tile_type(tileset, pterrain);
  } terrain_type_iterate_end;
  resource_type_iterate(presource) {
    tileset_setup_resource(tileset, presource);
  } resource_type_iterate_end;
  unit_type_iterate(punittype) {
    tileset_setup_unit_type(tileset, punittype);
  } unit_type_iterate_end;
  government_iterate(gov) {
    tileset_setup_government(tileset, gov);
  } government_iterate_end;
  base_type_iterate(pbase) {
    tileset_setup_base(tileset, pbase);
  } base_type_iterate_end;
  nations_iterate(pnation) {
    tileset_setup_nation_flag(tileset, pnation);
  } nations_iterate_end;
  improvement_iterate(pimprove) {
    tileset_setup_impr_type(tileset, pimprove);
  } improvement_iterate_end;
  advance_iterate(A_FIRST, padvance) {
    tileset_setup_tech_type(tileset, padvance);
  } advance_iterate_end;
  specialist_type_iterate(sp) {
    tileset_setup_specialist_type(tileset, sp);
  } specialist_type_iterate_end;

  for (id = 0; id < game.control.styles_count; id++) {
    tileset_setup_city_tiles(tileset, id);
  }

  /* Step 4:  Draw.
   *
   * Do any necessary redraws.
   */
  popdown_all_game_dialogs();
  generate_citydlg_dimensions();
  tileset_changed();
  can_slide = FALSE;
  center_tile_mapcanvas(center_tile);
  /* update_map_canvas_visible forces a full redraw.  Otherwise with fast
   * drawing we might not get one.  Of course this is slower. */
  update_map_canvas_visible();
  can_slide = TRUE;
}

/**************************************************************************
  This is merely a wrapper for tilespec_reread (above) for use in
  options.c and the client local options dialog.
**************************************************************************/
void tilespec_reread_callback(struct client_option *option)
{
  assert(option->string.pvalue && *option->string.pvalue != '\0');
  tilespec_reread(option->string.pvalue);
}

/**************************************************************************
  Loads the given graphics file (found in the data path) into a newly
  allocated sprite.
**************************************************************************/
static struct sprite *load_gfx_file(const char *gfx_filename)
{
  const char **gfx_fileexts = gfx_fileextensions(), *gfx_fileext;
  struct sprite *s;

  /* Try out all supported file extensions to find one that works. */
  while ((gfx_fileext = *gfx_fileexts++)) {
    char *real_full_name;
    char full_name[strlen(gfx_filename) + strlen(gfx_fileext) + 2];

    sprintf(full_name, "%s.%s", gfx_filename, gfx_fileext);
    if ((real_full_name = datafilename(full_name))) {
      freelog(LOG_DEBUG, "trying to load gfx file \"%s\".", real_full_name);
      s = load_gfxfile(real_full_name);
      if (s) {
	return s;
      }
    }
  }

  freelog(LOG_ERROR, "Could not load gfx file \"%s\".", gfx_filename);
  return NULL;
}

/**************************************************************************
  Ensure that the big sprite of the given spec file is loaded.
**************************************************************************/
static void ensure_big_sprite(struct specfile *sf)
{
  struct section_file the_file, *file = &the_file;
  const char *gfx_filename;

  if (sf->big_sprite) {
    /* Looks like it's already loaded. */
    return;
  }

  /* Otherwise load it.  The big sprite will sometimes be freed and will have
   * to be reloaded, but most of the time it's just loaded once, the small
   * sprites are extracted, and then it's freed. */
  if (!section_file_load(file, sf->file_name)) {
    freelog(LOG_FATAL, _("Could not open \"%s\"."), sf->file_name);
    exit(EXIT_FAILURE);
  }

  if (!check_tilespec_capabilities(file, "spec",
				   SPEC_CAPSTR, sf->file_name, TRUE)) {
    exit(EXIT_FAILURE);
  }

  gfx_filename = secfile_lookup_str(file, "file.gfx");

  sf->big_sprite = load_gfx_file(gfx_filename);

  if (!sf->big_sprite) {
    freelog(LOG_FATAL, "Could not load gfx file for the spec file \"%s\".",
	    sf->file_name);
    exit(EXIT_FAILURE);
  }
  section_file_free(file);
}

/**************************************************************************
  Scan all sprites declared in the given specfile.  This means that the
  positions of the sprites in the big_sprite are saved in the
  small_sprite structs.
**************************************************************************/
static void scan_specfile(struct tileset *t, struct specfile *sf,
			  bool duplicates_ok)
{
  struct section_file the_file, *file = &the_file;
  char **gridnames;
  int num_grids, i;

  if (!section_file_load(file, sf->file_name)) {
    freelog(LOG_FATAL, _("Could not open \"%s\"."), sf->file_name);
    exit(EXIT_FAILURE);
  }
  if (!check_tilespec_capabilities(file, "spec",
				   SPEC_CAPSTR, sf->file_name, TRUE)) {
    exit(EXIT_FAILURE);
  }

  /* currently unused */
  (void) section_file_lookup(file, "info.artists");

  gridnames = secfile_get_secnames_prefix(file, "grid_", &num_grids);

  for (i = 0; i < num_grids; i++) {
    int j, k;
    int x_top_left, y_top_left, dx, dy;
    int pixel_border;

    pixel_border = secfile_lookup_int_default(file, 0, "%s.pixel_border",
					      gridnames[i]);

    x_top_left = secfile_lookup_int(file, "%s.x_top_left", gridnames[i]);
    y_top_left = secfile_lookup_int(file, "%s.y_top_left", gridnames[i]);
    dx = secfile_lookup_int(file, "%s.dx", gridnames[i]);
    dy = secfile_lookup_int(file, "%s.dy", gridnames[i]);

    j = -1;
    while (section_file_lookup(file, "%s.tiles%d.tag", gridnames[i], ++j)) {
      struct small_sprite *ss = fc_malloc(sizeof(*ss));
      int row, column;
      int x1, y1;
      char **tags;
      int num_tags;
      int hot_x, hot_y;

      row = secfile_lookup_int(file, "%s.tiles%d.row", gridnames[i], j);
      column = secfile_lookup_int(file, "%s.tiles%d.column", gridnames[i], j);
      tags = secfile_lookup_str_vec(file, &num_tags, "%s.tiles%d.tag",
				    gridnames[i], j);
      hot_x = secfile_lookup_int_default(file, 0, "%s.tiles%d.hot_x",
					 gridnames[i], j);
      hot_y = secfile_lookup_int_default(file, 0, "%s.tiles%d.hot_y",
					 gridnames[i], j);

      /* there must be at least 1 because of the while(): */
      assert(num_tags > 0);

      x1 = x_top_left + (dx + pixel_border) * column;
      y1 = y_top_left + (dy + pixel_border) * row;

      ss->ref_count = 0;
      ss->file = NULL;
      ss->x = x1;
      ss->y = y1;
      ss->width = dx;
      ss->height = dy;
      ss->sf = sf;
      ss->sprite = NULL;
      ss->hot_x = hot_x;
      ss->hot_y = hot_y;

      small_sprite_list_prepend(t->small_sprites, ss);

      if (!duplicates_ok) {
        for (k = 0; k < num_tags; k++) {
          if (!hash_insert(t->sprite_hash, mystrdup(tags[k]), ss)) {
	    freelog(LOG_ERROR, "warning: already have a sprite for \"%s\".", tags[k]);
          }
        }
      } else {
        for (k = 0; k < num_tags; k++) {
	  (void) hash_replace(t->sprite_hash, mystrdup(tags[k]), ss);
        }
      }

      free(tags);
      tags = NULL;
    }
  }
  free(gridnames);
  gridnames = NULL;

  /* Load "extra" sprites.  Each sprite is one file. */
  i = -1;
  while (secfile_lookup_str_default(file, NULL, "extra.sprites%d.tag", ++i)) {
    struct small_sprite *ss = fc_malloc(sizeof(*ss));
    char **tags;
    char *filename;
    int num_tags, k;
    int hot_x, hot_y;

    tags
      = secfile_lookup_str_vec(file, &num_tags, "extra.sprites%d.tag", i);
    filename = secfile_lookup_str(file, "extra.sprites%d.file", i);

    hot_x = secfile_lookup_int_default(file, 0, "extra.sprites%d.hot_x", i);
    hot_y = secfile_lookup_int_default(file, 0, "extra.sprites%d.hot_y", i);

    ss->ref_count = 0;
    ss->file = mystrdup(filename);
    ss->sf = NULL;
    ss->sprite = NULL;
    ss->hot_x = hot_x;
    ss->hot_y = hot_y;

    small_sprite_list_prepend(t->small_sprites, ss);

    if (!duplicates_ok) {
      for (k = 0; k < num_tags; k++) {
	if (!hash_insert(t->sprite_hash, mystrdup(tags[k]), ss)) {
	  freelog(LOG_ERROR, "warning: already have a sprite for \"%s\".", tags[k]);
	}
      }
    } else {
      for (k = 0; k < num_tags; k++) {
	(void) hash_replace(t->sprite_hash, mystrdup(tags[k]), ss);
      }
    }
    free(tags);
  }

  section_file_check_unused(file, sf->file_name);
  section_file_free(file);
}

/**********************************************************************
  Returns the correct name of the gfx file (with path and extension)
  Must be free'd when no longer used
***********************************************************************/
static char *tilespec_gfx_filename(const char *gfx_filename)
{
  const char  *gfx_current_fileext;
  const char **gfx_fileexts = gfx_fileextensions();

  while((gfx_current_fileext = *gfx_fileexts++))
  {
    char *full_name =
       fc_malloc(strlen(gfx_filename) + strlen(gfx_current_fileext) + 2);
    char *real_full_name;

    sprintf(full_name,"%s.%s",gfx_filename,gfx_current_fileext);

    real_full_name = datafilename(full_name);
    free(full_name);
    if (real_full_name) {
      return mystrdup(real_full_name);
    }
  }

  freelog(LOG_FATAL, "Couldn't find a supported gfx file extension for \"%s\".",
         gfx_filename);
  exit(EXIT_FAILURE);
  return NULL;
}

/**********************************************************************
  Determine the sprite_type string.
***********************************************************************/
static int check_sprite_type(const char *sprite_type, const char *tile_section)
{
  if (mystrcasecmp(sprite_type, "corner") == 0) {
    return CELL_CORNER;
  }
  if (mystrcasecmp(sprite_type, "single") == 0) {
    return CELL_WHOLE;
  }
  if (mystrcasecmp(sprite_type, "whole") == 0) {
    return CELL_WHOLE;
  }
  freelog(LOG_ERROR, "[%s] unknown sprite_type \"%s\".",
	  tile_section,
	  sprite_type);
  return CELL_WHOLE;
}

/**********************************************************************
  Finds and reads the toplevel tilespec file based on given name.
  Sets global variables, including tile sizes and full names for
  intro files.
***********************************************************************/
struct tileset *tileset_read_toplevel(const char *tileset_name, bool verbose)
{
  struct section_file the_file, *file = &the_file;
  char *fname, *c;
  int i;
  int num_spec_files, num_sections;
  char **spec_filenames, **sections;
  char *file_capstr;
  bool duplicates_ok, is_hex;
  enum direction8 dir;
  const int spl = strlen(TILE_SECTION_PREFIX);
  struct tileset *t = tileset_new();

  fname = tilespec_fullname(tileset_name);
  if (!fname) {
    if (verbose) {
      freelog(LOG_ERROR, "Can't find tileset \"%s\".", tileset_name); 
    }
    tileset_free(t);
    return NULL;
  }
  freelog(LOG_VERBOSE, "tilespec file is \"%s\".", fname);

  if (!section_file_load(file, fname)) {
    freelog(LOG_ERROR, "Could not open \"%s\".", fname);
    section_file_free(file);
    free(fname);
    tileset_free(t);
    return NULL;
  }

  if (!check_tilespec_capabilities(file, "tilespec",
				   TILESPEC_CAPSTR, fname, verbose)) {
    section_file_free(file);
    free(fname);
    tileset_free(t);
    return NULL;
  }

  file_capstr = secfile_lookup_str(file, "%s.options", "tilespec");
  duplicates_ok = has_capabilities("+duplicates_ok", file_capstr);

  (void) section_file_lookup(file, "tilespec.name"); /* currently unused */

  sz_strlcpy(t->name, tileset_name);
  t->priority = secfile_lookup_int(file, "tilespec.priority");

  t->is_isometric = secfile_lookup_bool(file, "tilespec.is_isometric");

  /* Read hex-tileset information. */
  is_hex = secfile_lookup_bool(file, "tilespec.is_hex");
  t->hex_width = t->hex_height = 0;
  if (is_hex) {
    int hex_side = secfile_lookup_int(file, "tilespec.hex_side");

    if (t->is_isometric) {
      t->hex_height = hex_side;
    } else {
      t->hex_width = hex_side;
    }
    /* Hex tilesets are drawn the same as isometric. */
    t->is_isometric = TRUE;
  }

  if (t->is_isometric && !isometric_view_supported()) {
    freelog(LOG_NORMAL, _("Client does not support isometric tilesets."));
    freelog(LOG_NORMAL, _("Using default tileset instead."));
    assert(tileset_name != NULL);
    section_file_free(file);
    free(fname);
    tileset_free(t);
    return NULL;
  }
  if (!t->is_isometric && !overhead_view_supported()) {
    freelog(LOG_NORMAL, _("Client does not support overhead view tilesets."));
    freelog(LOG_NORMAL, _("Using default tileset instead."));
    assert(tileset_name != NULL);
    section_file_free(file);
    free(fname);
    tileset_free(t);
    return NULL;
  }

  /* Create arrays of valid and cardinal tileset dirs.  These depend
   * entirely on the tileset, not the topology.  They are also in clockwise
   * rotational ordering. */
  t->num_valid_tileset_dirs = t->num_cardinal_tileset_dirs = 0;
  dir = DIR8_NORTH;
  do {
    if (is_valid_tileset_dir(t, dir)) {
      t->valid_tileset_dirs[t->num_valid_tileset_dirs] = dir;
      t->num_valid_tileset_dirs++;
    }
    if (is_cardinal_tileset_dir(t, dir)) {
      t->cardinal_tileset_dirs[t->num_cardinal_tileset_dirs] = dir;
      t->num_cardinal_tileset_dirs++;
    }

    dir = dir_cw(dir);
  } while (dir != DIR8_NORTH);
  assert(t->num_valid_tileset_dirs % 2 == 0); /* Assumed elsewhere. */
  t->num_index_valid = 1 << t->num_valid_tileset_dirs;
  t->num_index_cardinal = 1 << t->num_cardinal_tileset_dirs;

  t->normal_tile_width
    = secfile_lookup_int(file, "tilespec.normal_tile_width");
  t->normal_tile_height
    = secfile_lookup_int(file, "tilespec.normal_tile_height");
  if (t->is_isometric) {
    t->full_tile_width = t->normal_tile_width;
    t->full_tile_height = 3 * t->normal_tile_height / 2;
  } else {
    t->full_tile_width = t->normal_tile_width;
    t->full_tile_height = t->normal_tile_height;
  }
  t->unit_tile_width
    = secfile_lookup_int_default(file, t->full_tile_width, "tilespec.unit_width");
  t->unit_tile_height
    = secfile_lookup_int_default(file, t->full_tile_height, "tilespec.unit_height");
  t->small_sprite_width
    = secfile_lookup_int(file, "tilespec.small_tile_width");
  t->small_sprite_height
    = secfile_lookup_int(file, "tilespec.small_tile_height");
  freelog(LOG_VERBOSE, "tile sizes %dx%d, %d%d unit, %d%d small",
	  t->normal_tile_width, t->normal_tile_height,
	  t->full_tile_width, t->full_tile_height,
	  t->small_sprite_width, t->small_sprite_height);

  t->roadstyle = secfile_lookup_int(file, "tilespec.roadstyle");
  t->fogstyle = secfile_lookup_int(file, "tilespec.fogstyle");
  t->darkness_style = secfile_lookup_int(file, "tilespec.darkness_style");
  if (t->darkness_style < DARKNESS_NONE
      || t->darkness_style > DARKNESS_CORNER
      || (t->darkness_style == DARKNESS_ISORECT
	  && (!t->is_isometric || t->hex_width > 0 || t->hex_height > 0))) {
    freelog(LOG_FATAL, "Invalid darkness style set in tileset.");
    exit(EXIT_FAILURE);
  }
  t->unit_flag_offset_x
    = secfile_lookup_int(file, "tilespec.unit_flag_offset_x");
  t->unit_flag_offset_y
    = secfile_lookup_int(file, "tilespec.unit_flag_offset_y");
  t->city_flag_offset_x
    = secfile_lookup_int(file, "tilespec.city_flag_offset_x");
  t->city_flag_offset_y
    = secfile_lookup_int(file, "tilespec.city_flag_offset_y");
  t->unit_offset_x = secfile_lookup_int(file, "tilespec.unit_offset_x");
  t->unit_offset_y = secfile_lookup_int(file, "tilespec.unit_offset_y");

  t->citybar_offset_y
    = secfile_lookup_int(file, "tilespec.citybar_offset_y");

  t->city_names_font_size
    = secfile_lookup_int(file, "tilespec.city_names_font_size");

  t->city_productions_font_size
    = secfile_lookup_int(file, "tilespec.city_productions_font_size");
  set_city_names_font_sizes(t->city_names_font_size,
			    t->city_productions_font_size);

  c = secfile_lookup_str(file, "tilespec.main_intro_file");
  t->main_intro_filename = tilespec_gfx_filename(c);
  freelog(LOG_DEBUG, "intro file %s", t->main_intro_filename);
  
  c = secfile_lookup_str(file, "tilespec.minimap_intro_file");
  t->minimap_intro_filename = tilespec_gfx_filename(c);
  freelog(LOG_DEBUG, "radar file %s", t->minimap_intro_filename);

  /* Terrain layer info. */
  for (i = 0; i < MAX_NUM_LAYERS; i++) {
    struct tileset_layer *tslp = &t->layers[i];
    int j, k;

    tslp->match_types
      = secfile_lookup_str_vec(file, &tslp->match_count,
			       "layer%d.match_types", i);
    for (j = 0; j < tslp->match_count; j++) {
      tslp->match_types[j] = mystrdup(tslp->match_types[j]);

      for (k = 0; k < j; k++) {
        if (tslp->match_types[k][0] == tslp->match_types[j][0]) {
          freelog(LOG_FATAL, "[layer%d] match_types: \"%s\" initial"
                             " ('%c') is not unique.",
                             i,
                             tslp->match_types[j],
                             tslp->match_types[j][0]);
          exit(EXIT_FAILURE);
        }
      }
    }
  }

  /* Tile drawing info. */
  sections = secfile_get_secnames_prefix(file, TILE_SECTION_PREFIX, &num_sections);
  if (num_sections == 0) {
    freelog(LOG_ERROR, "No [%s] sections supported by tileset \"%s\".",
				TILE_SECTION_PREFIX, fname);
    section_file_free(file);
    free(fname);
    tileset_free(t);
    return NULL;
  }

  assert(t->tile_hash == NULL);
  t->tile_hash = hash_new(hash_fval_string, hash_fcmp_string);

  for (i = 0; i < num_sections; i++) {
    struct drawing_data *draw = fc_calloc(1, sizeof(*draw));
    char *sprite_type;
    int l;

    draw->name = mystrdup(sections[i] + spl);
    draw->blending = secfile_lookup_int_default(file, 0,
						"%s.is_blended",
						sections[i]);
    draw->blending = CLIP(0, draw->blending, MAX_NUM_LAYERS);

    draw->is_reversed = secfile_lookup_bool_default(file, FALSE,
						    "%s.is_reversed",
						    sections[i]);
    draw->num_layers = secfile_lookup_int(file, "%s.num_layers",
					  sections[i]);
    draw->num_layers = CLIP(1, draw->num_layers, MAX_NUM_LAYERS);

    for (l = 0; l < draw->num_layers; l++) {
      struct drawing_layer *dlp = &draw->layer[l];
      struct tileset_layer *tslp = &t->layers[l];
      char *match_type;
      char **match_with;
      int count;

      dlp->is_tall
	= secfile_lookup_bool_default(file, FALSE, "%s.layer%d_is_tall",
				      sections[i], l);
      dlp->offset_x
	= secfile_lookup_int_default(file, 0, "%s.layer%d_offset_x",
				     sections[i], l);
      dlp->offset_y
	= secfile_lookup_int_default(file, 0, "%s.layer%d_offset_y",
				     sections[i], l);

      match_type = secfile_lookup_str_default(file, NULL,
					      "%s.layer%d_match_type",
					      sections[i], l);
      if (match_type) {
        int j;

	/* Determine our match_type. */
	for (j = 0; j < tslp->match_count; j++) {
	  if (mystrcasecmp(tslp->match_types[j], match_type) == 0) {
	    break;
	  }
	}
	if (j >= tslp->match_count) {
	  freelog(LOG_ERROR, "[%s] invalid match_type \"%s\".",
	                     sections[i],
	                     match_type);
	} else {
	  dlp->match_index[dlp->match_indices++] = j;
	}
      }

      match_with = secfile_lookup_str_vec(file, &count,
					  "%s.layer%d_match_with",
					  sections[i], l);
      if (match_with) {
        int j, k;

        if (count > MATCH_FULL) {
          freelog(LOG_ERROR, "[%s] match_with has too many types (%d, max %d)",
                             sections[i],
                             count,
                             MATCH_FULL);
          count = MATCH_FULL;
        }

        if (1 < dlp->match_indices) {
          freelog(LOG_ERROR, "[%s] previous match_with ignored.",
                             sections[i]);
          dlp->match_indices = 1;
        } else if (1 > dlp->match_indices) {
          freelog(LOG_ERROR, "[%s] missing match_type, using \"%s\".",
                             sections[i],
                             tslp->match_types[0]);
          dlp->match_index[0] = 0;
          dlp->match_indices = 1;
        }

        for (k = 0; k < count; k++) {
          for (j = 0; j < tslp->match_count; j++) {
            if (mystrcasecmp(tslp->match_types[j], match_with[k]) == 0) {
              break;
            }
          }
          if (j >= tslp->match_count) {
            freelog(LOG_ERROR, "[%s] layer%d_match_with: invalid  \"%s\".",
                               sections[i],
                               l,
                               match_with[k]);
          } else if (1 < count) {
            int m;

            for (m = 0; m < dlp->match_indices; m++) {
              if (dlp->match_index[m] == j) {
                freelog(LOG_ERROR, "[%s] layer%d_match_with: duplicate \"%s\".",
                                   sections[i],
                                   l,
                                   match_with[k]);
                break;
              }
            }
            if (m >= dlp->match_indices) {
              dlp->match_index[dlp->match_indices++] = j;
            }
          } else {
            dlp->match_index[dlp->match_indices++] = j;
          }
        }
        free(match_with);
        match_with = NULL;
      }

      /* Check match_indices */
      switch (dlp->match_indices) {
      case 0:
      case 1:
        dlp->match_style = MATCH_NONE;
        break;
      case 2:
        if (dlp->match_index[0] == dlp->match_index[1] ) {
          dlp->match_style = MATCH_SAME;
        } else {
          dlp->match_style = MATCH_PAIR;
        }
        break;
      default:
        dlp->match_style = MATCH_FULL;
        break;
      };

      sprite_type
	= secfile_lookup_str_default(file, "whole", "%s.layer%d_sprite_type",
				     sections[i], l);
      dlp->sprite_type = check_sprite_type(sprite_type, sections[i]);

      switch (dlp->sprite_type) {
      case CELL_WHOLE:
	/* OK, no problem */
	break;
      case CELL_CORNER:
	if (dlp->is_tall
	    || dlp->offset_x > 0
	    || dlp->offset_y > 0) {
	  freelog(LOG_ERROR,
		  "[%s] layer %d: you cannot have tall terrain or\n"
		  "a sprite offset with a cell-based drawing method.",
		  sections[i], l);
	  dlp->is_tall = FALSE;
	  dlp->offset_x = dlp->offset_y = 0;
	}
	break;
      };
    }

    draw->mine_tag = secfile_lookup_str_default(file, NULL, "%s.mine_sprite",
						sections[i]);
    if (draw->mine_tag) {
      draw->mine_tag = mystrdup(draw->mine_tag);
    }

    if (!hash_insert(t->tile_hash, draw->name, draw)) {
      freelog(LOG_ERROR, "warning: duplicate tilespec entry [%s].",
	      sections[i]);
      section_file_free(file);
      free(fname);
      tileset_free(t);
      return NULL;
    }
  }
  free(sections);

  spec_filenames = secfile_lookup_str_vec(file, &num_spec_files,
					  "tilespec.files");
  if (num_spec_files == 0) {
    freelog(LOG_ERROR, "No tile graphics files specified in \"%s\"", fname);
    section_file_free(file);
    free(fname);
    tileset_free(t);
    return NULL;
  }

  assert(t->sprite_hash == NULL);
  t->sprite_hash = hash_new_full(hash_fval_string, hash_fcmp_string,
                                 sprite_hash_free_key, NULL);
  for (i = 0; i < num_spec_files; i++) {
    struct specfile *sf = fc_malloc(sizeof(*sf));
    char *dname;

    freelog(LOG_DEBUG, "spec file %s", spec_filenames[i]);
    
    sf->big_sprite = NULL;
    dname = datafilename(spec_filenames[i]);
    if (!dname) {
      if (verbose) {
        freelog(LOG_ERROR, "Can't find spec file \"%s\".", spec_filenames[i]);
      }
      section_file_free(file);
      free(fname);
      tileset_free(t);
      return NULL;
    }
    sf->file_name = mystrdup(dname);
    scan_specfile(t, sf, duplicates_ok);

    specfile_list_prepend(t->specfiles, sf);
  }
  free(spec_filenames);

  t->color_system = color_system_read(file);

  section_file_check_unused(file, fname);
  
  t->prefered_themes = secfile_lookup_str_vec(file, &(t->num_prefered_themes),
                                              "tilespec.prefered_themes");
  for (i = 0; i < t->num_prefered_themes; i++) {
    t->prefered_themes[i] = mystrdup(t->prefered_themes[i]);
  }
  
  section_file_free(file);
  freelog(LOG_VERBOSE, "finished reading \"%s\".", fname);
  free(fname);

  return t;
}

/**********************************************************************
  Returns a text name for the citizen, as used in the tileset.
***********************************************************************/
static const char *citizen_rule_name(enum citizen_category citizen)
{
  /* These strings are used in reading the tileset.  Do not
   * translate. */
  switch (citizen) {
  case CITIZEN_HAPPY:
    return "happy";
  case CITIZEN_CONTENT:
    return "content";
  case CITIZEN_UNHAPPY:
    return "unhappy";
  case CITIZEN_ANGRY:
    return "angry";
  default:
    break;
  }
  die("unknown citizen type %d", (int) citizen);
  return NULL;
}

/****************************************************************************
  Return a directional string for the cardinal directions.  Normally the
  binary value 1000 will be converted into "n1e0s0w0".  This is in a
  clockwise ordering.
****************************************************************************/
static const char *cardinal_index_str(const struct tileset *t, int idx)
{
  static char c[64];
  int i;

  c[0] = '\0';
  for (i = 0; i < t->num_cardinal_tileset_dirs; i++) {
    int value = (idx >> i) & 1;

    cat_snprintf(c, sizeof(c), "%s%d",
		 dir_get_tileset_name(t->cardinal_tileset_dirs[i]), value);
  }

  return c;
}

/****************************************************************************
  Do the same thing as cardinal_str, except including all valid directions.
  The returned string is a pointer to static memory.
****************************************************************************/
static char *valid_index_str(const struct tileset *t, int index)
{
  static char c[64];
  int i;

  c[0] = '\0';
  for (i = 0; i < t->num_valid_tileset_dirs; i++) {
    int value = (index >> i) & 1;

    cat_snprintf(c, sizeof(c), "%s%d",
		 dir_get_tileset_name(t->valid_tileset_dirs[i]), value);
  }

  return c;
}

/**************************************************************************
  Loads the sprite. If the sprite is already loaded a reference
  counter is increased. Can return NULL if the sprite couldn't be
  loaded.
**************************************************************************/
static struct sprite *load_sprite(struct tileset *t, const char *tag_name)
{
  /* Lookup information about where the sprite is found. */
  struct small_sprite *ss = hash_lookup_data(t->sprite_hash, tag_name);

  freelog(LOG_DEBUG, "load_sprite(tag='%s')", tag_name);
  if (!ss) {
    return NULL;
  }

  assert(ss->ref_count >= 0);

  if (!ss->sprite) {
    /* If the sprite hasn't been loaded already, then load it. */
    assert(ss->ref_count == 0);
    if (ss->file) {
      ss->sprite = load_gfx_file(ss->file);
      if (!ss->sprite) {
	freelog(LOG_FATAL, "Couldn't load gfx file \"%s\" for sprite '%s'.",
		ss->file, tag_name);
	exit(EXIT_FAILURE);
      }
    } else {
      int sf_w, sf_h;

      ensure_big_sprite(ss->sf);
      get_sprite_dimensions(ss->sf->big_sprite, &sf_w, &sf_h);
      if (ss->x < 0 || ss->x + ss->width > sf_w
	  || ss->y < 0 || ss->y + ss->height > sf_h) {
	freelog(LOG_ERROR,
		"Sprite '%s' in file \"%s\" isn't within the image!",
		tag_name, ss->sf->file_name);
	return NULL;
      }
      ss->sprite =
	crop_sprite(ss->sf->big_sprite, ss->x, ss->y, ss->width, ss->height,
		    NULL, -1, -1);
    }
  }

  /* Track the reference count so we know when to free the sprite. */
  ss->ref_count++;

  return ss->sprite;
}

/**************************************************************************
  Unloads the sprite. Decrease the reference counter. If the last
  reference is removed the sprite is freed.
**************************************************************************/
static void unload_sprite(struct tileset *t, const char *tag_name)
{
  struct small_sprite *ss = hash_lookup_data(t->sprite_hash, tag_name);

  assert(ss);
  assert(ss->ref_count >= 1);
  assert(ss->sprite);

  ss->ref_count--;

  if (ss->ref_count == 0) {
    /* Nobody's using the sprite anymore, so we should free it.  We know
     * where to find it if we need it again. */
    freelog(LOG_DEBUG, "freeing sprite '%s'.", tag_name);
    free_sprite(ss->sprite);
    ss->sprite = NULL;
  }
}

/****************************************************************************
  Insert a generated sprite into the existing sprite hash with a given
  name.

  This means the sprite can now be accessed via the tag, and will be
  automatically freed along with the tileset.  In other words, you should
  only do this with sprites you've just allocated, and only on tags that
  are unused!
****************************************************************************/
static void insert_sprite(struct tileset *t, const char *tag_name,
			  struct sprite *sprite)
{
  struct small_sprite *ss = fc_calloc(sizeof(*ss), 1);

  assert(load_sprite(t, tag_name) == 0);
  ss->ref_count = 1;
  ss->sprite = sprite;
  small_sprite_list_prepend(t->small_sprites, ss);
  if (!hash_insert(t->sprite_hash, mystrdup(tag_name), ss)) {
    freelog(LOG_ERROR, "warning: already have a sprite for '%s'.", tag_name);
  }
}

/**************************************************************************
  Return TRUE iff the specified sprite exists in the tileset (whether
  or not it is currently loaded).
**************************************************************************/
static bool sprite_exists(const struct tileset *t, const char *tag_name)
{
  /* Lookup information about where the sprite is found. */
  struct small_sprite *ss = hash_lookup_data(t->sprite_hash, tag_name);

  return (ss != NULL);
}

/* Not very safe, but convenient: */
#define SET_SPRITE(field, tag)					  \
  do {								  \
    t->sprites.field = load_sprite(t, tag);			  \
    if (!t->sprites.field) {					  \
      die("Sprite tag '%s' missing.", tag);			  \
    }								  \
  } while(FALSE)

/* Sets sprites.field to tag or (if tag isn't available) to alt */
#define SET_SPRITE_ALT(field, tag, alt)					    \
  do {									    \
    t->sprites.field = load_sprite(t, tag);				    \
    if (!t->sprites.field) {						    \
      t->sprites.field = load_sprite(t, alt);				    \
    }									    \
    if (!t->sprites.field) {						    \
      die("Sprite tag '%s' and alternate '%s' are both missing.", tag, alt);\
    }									    \
  } while(FALSE)

/* Sets sprites.field to tag, or NULL if not available */
#define SET_SPRITE_OPT(field, tag) \
  t->sprites.field = load_sprite(t, tag)

#define SET_SPRITE_ALT_OPT(field, tag, alt)				    \
  do {									    \
    t->sprites.field = tiles_lookup_sprite_tag_alt(t, LOG_VERBOSE, tag, alt,\
						   "sprite", #field);	    \
  } while (FALSE)

/****************************************************************************
  Setup the graphics for specialist types.
****************************************************************************/
void tileset_setup_specialist_type(struct tileset *t, Specialist_type_id id)
{
  /* Load the specialist sprite graphics. */
  char buffer[512];
  int j;
  const char *name = specialist_rule_name(specialist_by_number(id));

  for (j = 0; j < MAX_NUM_CITIZEN_SPRITES; j++) {
    my_snprintf(buffer, sizeof(buffer), "specialist.%s_%d", name, j);
    t->sprites.specialist[id].sprite[j] = load_sprite(t, buffer);
    if (!t->sprites.specialist[id].sprite[j]) {
      break;
    }
  }
  t->sprites.specialist[id].count = j;
  if (j == 0) {
    freelog(LOG_FATAL, "No graphics for specialist \"%s\".", name);
    exit(EXIT_FAILURE);
  }
}

/****************************************************************************
  Setup the graphics for (non-specialist) citizen types.
****************************************************************************/
static void tileset_setup_citizen_types(struct tileset *t)
{
  int i, j;
  char buffer[512];

  /* Load the citizen sprite graphics, no specialist. */
  for (i = 0; i < CITIZEN_LAST; i++) {
    const char *name = citizen_rule_name(i);

    for (j = 0; j < MAX_NUM_CITIZEN_SPRITES; j++) {
      my_snprintf(buffer, sizeof(buffer), "citizen.%s_%d", name, j);
      t->sprites.citizen[i].sprite[j] = load_sprite(t, buffer);
      if (!t->sprites.citizen[i].sprite[j]) {
	break;
      }
    }
    t->sprites.citizen[i].count = j;
    if (j == 0) {
      freelog(LOG_FATAL, "No graphics for citizen \"%s\".", name);
      exit(EXIT_FAILURE);
    }
  }
}

/****************************************************************************
  Return the sprite in the city_sprite listing that corresponds to this
  city - based on city style and size.

  See also load_city_sprite, free_city_sprite.
****************************************************************************/
static struct sprite *get_city_sprite(const struct city_sprite *city_sprite,
				      const struct city *pcity)
{
  /* get style and match the best tile based on city size */
  int style = style_of_city(pcity);
  int num_thresholds;
  struct city_style_threshold *thresholds;
  int t;

  assert(style < city_sprite->num_styles);

  if (is_ocean_tile(pcity->tile)
      && city_sprite->styles[style].oceanic_num_thresholds != 0) {
    num_thresholds = city_sprite->styles[style].oceanic_num_thresholds;
    thresholds = city_sprite->styles[style].oceanic_thresholds;
  } else {
    num_thresholds = city_sprite->styles[style].land_num_thresholds;
    thresholds = city_sprite->styles[style].land_thresholds;
  }

  if (num_thresholds == 0) {
    return NULL;
  }

  /* We find the sprite with the largest threshold value that's no bigger
   * than this city size. */
  for (t = 0; t < num_thresholds; t++) {
    if (pcity->size < thresholds[t].city_size) {
      break;
    }
  }

  return thresholds[MAX(t - 1, 0)].sprite;
}

/****************************************************************************
  Allocates one threshold set for city sprite
****************************************************************************/
static int load_city_thresholds_sprites(struct tileset *t, const char *tag,
                                        char *graphic, char *graphic_alt,
                                        struct city_style_threshold **thresholds)
{
  char buffer[128];
  char *gfx_in_use = graphic;
  int num_thresholds = 0;
  struct sprite *sprite;
  int size;

  *thresholds = NULL;

  for (size = 0; size < MAX_CITY_SIZE; size++) {
    my_snprintf(buffer, sizeof(buffer), "%s_%s_%d",
                gfx_in_use, tag, size);
    if ((sprite = load_sprite(t, buffer))) {
      num_thresholds++;
      *thresholds = fc_realloc(*thresholds, num_thresholds * sizeof(**thresholds));
      (*thresholds)[num_thresholds - 1].city_size = size;
      (*thresholds)[num_thresholds - 1].sprite = sprite;
    } else if (size == 0) {
      if (gfx_in_use == graphic) {
        /* Try again with graphic_alt. */
        size--;
        gfx_in_use = graphic_alt;
      } else {
        /* Don't load any others if the 0 element isn't there. */
        break;
      }
    }
  }

  return num_thresholds;
}

/****************************************************************************
  Allocates and loads a new city sprite from the given sprite tags.

  tag may be NULL.

  See also get_city_sprite, free_city_sprite.
****************************************************************************/
static struct city_sprite *load_city_sprite(struct tileset *t,
					    const char *tag)
{
  struct city_sprite *city_sprite = fc_malloc(sizeof(*city_sprite));
  int style;

  /* Store number of styles we have allocated memory for.
   * game.control.styles_count might change if client disconnects from
   * server and connects new one. */
  city_sprite->num_styles = game.control.styles_count;
  city_sprite->styles = fc_malloc(city_sprite->num_styles
				  * sizeof(*city_sprite->styles));

  for (style = 0; style < city_sprite->num_styles; style++) {
    city_sprite->styles[style].land_num_thresholds =
      load_city_thresholds_sprites(t, tag, city_styles[style].graphic,
                                   city_styles[style].graphic_alt,
                                   &city_sprite->styles[style].land_thresholds);
    city_sprite->styles[style].oceanic_num_thresholds =
      load_city_thresholds_sprites(t, tag, city_styles[style].oceanic_graphic,
                                   city_styles[style].oceanic_graphic_alt,
                                   &city_sprite->styles[style].oceanic_thresholds);
  }

  return city_sprite;
}

/****************************************************************************
  Frees a city sprite.

  See also get_city_sprite, load_city_sprite.
****************************************************************************/
static void free_city_sprite(struct city_sprite *city_sprite)
{
  int style;

  if (!city_sprite) {
    return;
  }
  for (style = 0; style < city_sprite->num_styles; style++) {
    if (city_sprite->styles[style].land_thresholds) {
      free(city_sprite->styles[style].land_thresholds);
    }
    if (city_sprite->styles[style].oceanic_thresholds) {
      free(city_sprite->styles[style].oceanic_thresholds);
    }
  }
  free(city_sprite->styles);
  free(city_sprite);
}

/**********************************************************************
  Initialize 'sprites' structure based on hardwired tags which
  freeciv always requires. 
***********************************************************************/
static void tileset_lookup_sprite_tags(struct tileset *t)
{
  char buffer[512], buffer2[512];
  const char dir_char[] = "nsew";
  const int W = t->normal_tile_width, H = t->normal_tile_height;
  int i, j, f;
  
  assert(t->sprite_hash != NULL);

  SET_SPRITE(treaty_thumb[0], "treaty.disagree_thumb_down");
  SET_SPRITE(treaty_thumb[1], "treaty.agree_thumb_up");

  for (j = 0; j < INDICATOR_COUNT; j++) {
    const char *names[] = {"science_bulb", "warming_sun", "cooling_flake"};

    for (i = 0; i < NUM_TILES_PROGRESS; i++) {
      my_snprintf(buffer, sizeof(buffer), "s.%s_%d", names[j], i);
      SET_SPRITE(indicator[j][i], buffer);
    }
  }

  SET_SPRITE(arrow[ARROW_RIGHT], "s.right_arrow");
  SET_SPRITE(arrow[ARROW_PLUS], "s.plus");
  SET_SPRITE(arrow[ARROW_MINUS], "s.minus");
  if (t->is_isometric) {
    SET_SPRITE(dither_tile, "t.dither_tile");
  }

  SET_SPRITE(mask.tile, "mask.tile");
  SET_SPRITE(mask.worked_tile, "mask.worked_tile");
  SET_SPRITE(mask.unworked_tile, "mask.unworked_tile");

  SET_SPRITE(tax_luxury, "s.tax_luxury");
  SET_SPRITE(tax_science, "s.tax_science");
  SET_SPRITE(tax_gold, "s.tax_gold");

  tileset_setup_citizen_types(t);

  for (i = 0; i < SPACESHIP_COUNT; i++) {
    const char *names[SPACESHIP_COUNT]
      = {"solar_panels", "life_support", "habitation",
	 "structural", "fuel", "propulsion", "exhaust"};

    my_snprintf(buffer, sizeof(buffer), "spaceship.%s", names[i]);
    SET_SPRITE(spaceship[i], buffer);
  }

  for (i = 0; i < CURSOR_LAST; i++) {
    for (f = 0; f < NUM_CURSOR_FRAMES; f++) {
      const char *names[CURSOR_LAST] =
               {"goto", "patrol", "paradrop", "nuke", "select", 
		"invalid", "attack", "edit_paint", "edit_add", "wait"};
      struct small_sprite *ss;

      assert(ARRAY_SIZE(names) == CURSOR_LAST);
      my_snprintf(buffer, sizeof(buffer), "cursor.%s%d", names[i], f);
      SET_SPRITE(cursor[i].frame[f], buffer);
      ss = hash_lookup_data(t->sprite_hash, buffer);
      t->sprites.cursor[i].hot_x = ss->hot_x;
      t->sprites.cursor[i].hot_y = ss->hot_y;
    }
  }

  for (i = 0; i < ICON_COUNT; i++) {
    const char *names[ICON_COUNT] = {"freeciv", "citydlg"};

    my_snprintf(buffer, sizeof(buffer), "icon.%s", names[i]);
    SET_SPRITE(icon[i], buffer);
  }

  /* Isolated road graphics are used by roadstyle 0 and 1*/
  if (t->roadstyle == 0 || t->roadstyle == 1) {
    SET_SPRITE(road.isolated, "r.road_isolated");
    SET_SPRITE(rail.isolated, "r.rail_isolated");
  }
  
  if (t->roadstyle == 0) {
    /* Roadstyle 0 has just 8 additional sprites for both road and rail:
     * one for the road/rail going off in each direction. */
    for (i = 0; i < t->num_valid_tileset_dirs; i++) {
      enum direction8 dir = t->valid_tileset_dirs[i];
      const char *dir_name = dir_get_tileset_name(dir);

      my_snprintf(buffer, sizeof(buffer), "r.road_%s", dir_name);
      SET_SPRITE(road.dir[i], buffer);
      my_snprintf(buffer, sizeof(buffer), "r.rail_%s", dir_name);
      SET_SPRITE(rail.dir[i], buffer);
    }
  } else if (t->roadstyle == 1) {
    int num_index = 1 << (t->num_valid_tileset_dirs / 2), j;

    /* Roadstyle 1 has 32 additional sprites for both road and rail:
     * 16 each for cardinal and diagonal directions.  Each set
     * of 16 provides a NSEW-indexed sprite to provide connectors for
     * all rails in the cardinal/diagonal directions.  The 0 entry is
     * unused (the "isolated" sprite is used instead). */

    for (i = 1; i < num_index; i++) {
      char c[64] = "", d[64] = "";

      for (j = 0; j < t->num_valid_tileset_dirs / 2; j++) {
	int value = (i >> j) & 1;

	cat_snprintf(c, sizeof(c), "%s%d",
		     dir_get_tileset_name(t->valid_tileset_dirs[2 * j]),
		     value);
	cat_snprintf(d, sizeof(d), "%s%d",
		     dir_get_tileset_name(t->valid_tileset_dirs[2 * j + 1]),
		     value);
      }

      my_snprintf(buffer, sizeof(buffer), "r.c_road_%s", c);
      SET_SPRITE(road.even[i], buffer);

      my_snprintf(buffer, sizeof(buffer), "r.d_road_%s", d);
      SET_SPRITE(road.odd[i], buffer);

      my_snprintf(buffer, sizeof(buffer), "r.c_rail_%s", c);
      SET_SPRITE(rail.even[i], buffer);

      my_snprintf(buffer, sizeof(buffer), "r.d_rail_%s", d);
      SET_SPRITE(rail.odd[i], buffer);
    }
  } else {
    /* Roadstyle 2 includes 256 sprites, one for every possibility.
     * Just go around clockwise, with all combinations. */
    for (i = 0; i < t->num_index_valid; i++) {
      my_snprintf(buffer, sizeof(buffer), "r.road_%s", valid_index_str(t, i));
      SET_SPRITE(road.total[i], buffer);

      my_snprintf(buffer, sizeof(buffer), "r.rail_%s", valid_index_str(t, i));
      SET_SPRITE(rail.total[i], buffer);
    }
  }

  /* Corner road/rail graphics are used by roadstyle 0 and 1. */
  if (t->roadstyle == 0 || t->roadstyle == 1) {
    for (i = 0; i < t->num_valid_tileset_dirs; i++) {
      enum direction8 dir = t->valid_tileset_dirs[i];

      if (!is_cardinal_tileset_dir(t, dir)) {
	my_snprintf(buffer, sizeof(buffer), "r.c_road_%s",
		    dir_get_tileset_name(dir));
	SET_SPRITE_OPT(road.corner[dir], buffer);

	my_snprintf(buffer, sizeof(buffer), "r.c_rail_%s",
		    dir_get_tileset_name(dir));
	SET_SPRITE_OPT(rail.corner[dir], buffer);
      }
    }
  }

  SET_SPRITE(explode.nuke, "explode.nuke");

  sprite_vector_init(&t->sprites.explode.unit);
  for (i = 0; ; i++) {
    struct sprite *sprite;

    my_snprintf(buffer, sizeof(buffer), "explode.unit_%d", i);
    sprite = load_sprite(t, buffer);
    if (!sprite) {
      break;
    }
    sprite_vector_append(&t->sprites.explode.unit, &sprite);
  }

  SET_SPRITE(unit.auto_attack,  "unit.auto_attack");
  SET_SPRITE(unit.auto_settler, "unit.auto_settler");
  SET_SPRITE(unit.auto_explore, "unit.auto_explore");
  SET_SPRITE(unit.fallout,	"unit.fallout");
  SET_SPRITE(unit.fortified,	"unit.fortified");     
  SET_SPRITE(unit.fortifying,	"unit.fortifying");     
  SET_SPRITE(unit.go_to,	"unit.goto");     
  SET_SPRITE(unit.irrigate,     "unit.irrigate");
  SET_SPRITE(unit.mine,	        "unit.mine");
  SET_SPRITE(unit.pillage,	"unit.pillage");
  SET_SPRITE(unit.pollution,    "unit.pollution");
  SET_SPRITE(unit.road,	        "unit.road");
  SET_SPRITE(unit.sentry,	"unit.sentry");      
  SET_SPRITE(unit.stack,	"unit.stack");
  SET_SPRITE(unit.loaded, "unit.loaded");
  SET_SPRITE(unit.transform,    "unit.transform");
  SET_SPRITE(unit.connect,      "unit.connect");
  SET_SPRITE(unit.patrol,       "unit.patrol");
  for (i = 0; i < MAX_NUM_BATTLEGROUPS; i++) {
    my_snprintf(buffer, sizeof(buffer), "unit.battlegroup_%d", i);
    my_snprintf(buffer2, sizeof(buffer2), "city.size_%d", i + 1);
    assert(MAX_NUM_BATTLEGROUPS < NUM_TILES_DIGITS);
    SET_SPRITE_ALT(unit.battlegroup[i], buffer, buffer2);
  }
  SET_SPRITE(unit.lowfuel, "unit.lowfuel");
  SET_SPRITE(unit.tired, "unit.tired");

  for(i=0; i<NUM_TILES_HP_BAR; i++) {
    my_snprintf(buffer, sizeof(buffer), "unit.hp_%d", i*10);
    SET_SPRITE(unit.hp_bar[i], buffer);
  }

  for (i = 0; i < MAX_VET_LEVELS; i++) {
    /* Veteran level sprites are optional.  For instance "green" units
     * usually have no special graphic. */
    my_snprintf(buffer, sizeof(buffer), "unit.vet_%d", i);
    t->sprites.unit.vet_lev[i] = load_sprite(t, buffer);
  }

  t->sprites.unit.select[0] = NULL;
  if (sprite_exists(t, "unit.select0")) {
    for (i = 0; i < NUM_TILES_SELECT; i++) {
      my_snprintf(buffer, sizeof(buffer), "unit.select%d", i);
      SET_SPRITE(unit.select[i], buffer);
    }
  }

  SET_SPRITE(citybar.shields, "citybar.shields");
  SET_SPRITE(citybar.food, "citybar.food");
  SET_SPRITE(citybar.trade, "citybar.trade");
  SET_SPRITE(citybar.occupied, "citybar.occupied");
  SET_SPRITE(citybar.background, "citybar.background");
  sprite_vector_init(&t->sprites.citybar.occupancy);
  for (i = 0; ; i++) {
    struct sprite *sprite;

    my_snprintf(buffer, sizeof(buffer), "citybar.occupancy_%d", i);
    sprite = load_sprite(t, buffer);
    if (!sprite) {
      break;
    }
    sprite_vector_append(&t->sprites.citybar.occupancy, &sprite);
  }
  if (t->sprites.citybar.occupancy.size < 2) {
    freelog(LOG_FATAL, "Missing necessary citybar.occupancy_N sprites.");
    exit(EXIT_FAILURE);
  }

#define SET_EDITOR_SPRITE(x) SET_SPRITE(editor.x, "editor." #x)
  SET_EDITOR_SPRITE(erase);
  SET_EDITOR_SPRITE(brush);
  SET_EDITOR_SPRITE(copy);
  SET_EDITOR_SPRITE(paste);
  SET_EDITOR_SPRITE(copypaste);
  SET_EDITOR_SPRITE(startpos);
  SET_EDITOR_SPRITE(terrain);
  SET_EDITOR_SPRITE(terrain_resource);
  SET_EDITOR_SPRITE(terrain_special);
  SET_EDITOR_SPRITE(unit);
  SET_EDITOR_SPRITE(city);
  SET_EDITOR_SPRITE(vision);
  SET_EDITOR_SPRITE(territory);
  SET_EDITOR_SPRITE(properties);
  SET_EDITOR_SPRITE(military_base);
#undef SET_EDITOR_SPRITE

  SET_SPRITE(city.disorder, "city.disorder");

  for(i=0; i<NUM_TILES_DIGITS; i++) {
    my_snprintf(buffer, sizeof(buffer), "city.size_%d", i);
    SET_SPRITE(city.size[i], buffer);
    my_snprintf(buffer2, sizeof(buffer2), "path.turns_%d", i);
    SET_SPRITE_ALT_OPT(path.turns[i], buffer2, buffer);

    if(i!=0) {
      my_snprintf(buffer, sizeof(buffer), "city.size_%d", i*10);
      SET_SPRITE(city.size_tens[i], buffer);
      my_snprintf(buffer2, sizeof(buffer2), "path.turns_%d", i * 10);
      SET_SPRITE_ALT_OPT(path.turns_tens[i], buffer2, buffer);
    }
    my_snprintf(buffer, sizeof(buffer), "city.t_food_%d", i);
    SET_SPRITE(city.tile_foodnum[i], buffer);
    my_snprintf(buffer, sizeof(buffer), "city.t_shields_%d", i);
    SET_SPRITE(city.tile_shieldnum[i], buffer);
    my_snprintf(buffer, sizeof(buffer), "city.t_trade_%d", i);
    SET_SPRITE(city.tile_tradenum[i], buffer);
  }

  SET_SPRITE(upkeep.unhappy[0], "upkeep.unhappy");
  SET_SPRITE(upkeep.unhappy[1], "upkeep.unhappy2");
  output_type_iterate(o) {
    my_snprintf(buffer, sizeof(buffer),
		"upkeep.%s", get_output_identifier(o));
    t->sprites.upkeep.output[o][0] = load_sprite(t, buffer);
    my_snprintf(buffer, sizeof(buffer),
		"upkeep.%s2", get_output_identifier(o));
    t->sprites.upkeep.output[o][1] = load_sprite(t, buffer);
  } output_type_iterate_end;
  
  SET_SPRITE(user.attention, "user.attention");

  SET_SPRITE(tx.fallout,    "tx.fallout");
  SET_SPRITE(tx.pollution,  "tx.pollution");
  SET_SPRITE(tx.village,    "tx.village");
  SET_SPRITE(tx.fog,        "tx.fog");

  /* Load color sprites. */
  for (i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
    my_snprintf(buffer, sizeof(buffer), "colors.player%d", i);
    SET_SPRITE(colors.player[i], buffer);
  }
  SET_SPRITE(colors.background, "colors.background");
  sprite_vector_init(&t->sprites.colors.overlays);
  for (i = 0; ; i++) {
    struct sprite *sprite;

    my_snprintf(buffer, sizeof(buffer), "colors.overlay_%d", i);
    sprite = load_sprite(t, buffer);
    if (!sprite) {
      break;
    }
    sprite_vector_append(&t->sprites.colors.overlays, &sprite);
  }
  if (i == 0) {
    freelog(LOG_FATAL, "Missing overlay-color sprite colors.overlay_0.");
    exit(EXIT_FAILURE);
  }

  /* Chop up and build the overlay graphics. */
  sprite_vector_reserve(&t->sprites.city.worked_tile_overlay,
			sprite_vector_size(&t->sprites.colors.overlays));
  sprite_vector_reserve(&t->sprites.city.unworked_tile_overlay,
			sprite_vector_size(&t->sprites.colors.overlays));
  for (i = 0; i < sprite_vector_size(&t->sprites.colors.overlays); i++) {
    struct sprite *color, *color_mask;
    struct sprite *worked, *unworked;

    color = *sprite_vector_get(&t->sprites.colors.overlays, i);
    color_mask = crop_sprite(color, 0, 0, W, H, t->sprites.mask.tile, 0, 0);
    worked = crop_sprite(color_mask, 0, 0, W, H,
			 t->sprites.mask.worked_tile, 0, 0);
    unworked = crop_sprite(color_mask, 0, 0, W, H,
			   t->sprites.mask.unworked_tile, 0, 0);
    free_sprite(color_mask);
    t->sprites.city.worked_tile_overlay.p[i] =  worked;
    t->sprites.city.unworked_tile_overlay.p[i] = unworked;
  }

  /* Chop up and build the background graphics. */
  t->sprites.backgrounds.background
    = crop_sprite(t->sprites.colors.background, 0, 0, W, H,
		  t->sprites.mask.tile, 0, 0);
  for (i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
    t->sprites.backgrounds.player[i]
      = crop_sprite(t->sprites.colors.player[i], 0, 0, W, H,
		    t->sprites.mask.tile, 0, 0);
  }

  {
    SET_SPRITE(grid.unavailable, "grid.unavailable");

    for (i = 0; i < EDGE_COUNT; i++) {
      char *name[EDGE_COUNT] = {"ns", "we", "ud", "lr"};
      int j, p;

      if (i == EDGE_UD && t->hex_width == 0) {
	continue;
      } else if (i == EDGE_LR && t->hex_height == 0) {
	continue;
      }

      my_snprintf(buffer, sizeof(buffer), "grid.main.%s", name[i]);
      SET_SPRITE(grid.main[i], buffer);

      my_snprintf(buffer, sizeof(buffer), "grid.city.%s", name[i]);
      SET_SPRITE(grid.city[i], buffer);

      my_snprintf(buffer, sizeof(buffer), "grid.worked.%s", name[i]);
      SET_SPRITE(grid.worked[i], buffer);

      my_snprintf(buffer, sizeof(buffer), "grid.selected.%s", name[i]);
      SET_SPRITE(grid.selected[i], buffer);

      my_snprintf(buffer, sizeof(buffer), "grid.coastline.%s", name[i]);
      SET_SPRITE(grid.coastline[i], buffer);

      for (j = 0; j < 2; j++) {
	struct sprite *s;

	my_snprintf(buffer, sizeof(buffer), "grid.borders.%c", name[i][j]);
	SET_SPRITE(grid.borders[i][j], buffer);

	for (p = 0; p < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; p++) {
	  my_snprintf(buffer, sizeof(buffer), "grid.borders.%c.%d",
		      name[i][j], p);
	  s = load_sprite(t, buffer);

	  if (!s) {
	    if (t->sprites.colors.player[p] && t->sprites.grid.borders[i][j]) {
	      s = crop_sprite(t->sprites.colors.player[p],
			      0, 0,
			      t->normal_tile_width, t->normal_tile_height,
			      t->sprites.grid.borders[i][j], 0, 0);
	      insert_sprite(t, buffer, s);
	    } else {
	      s = t->sprites.grid.borders[i][j];
	    }
	  }
	  t->sprites.grid.player_borders[p][i][j] = s;
	}
      }
    }
  }

  for (i = 0; i < t->num_index_cardinal; i++) {
    my_snprintf(buffer, sizeof(buffer), "tx.s_river_%s",
		cardinal_index_str(t, i));
    SET_SPRITE(tx.spec_river[i], buffer);
  }

  /* We use direction-specific irrigation and farmland graphics, if they
   * are available.  If not, we just fall back to the basic irrigation
   * graphics. */
  for (i = 0; i < t->num_index_cardinal; i++) {
    my_snprintf(buffer, sizeof(buffer), "tx.s_irrigation_%s",
		cardinal_index_str(t, i));
    SET_SPRITE_ALT(tx.irrigation[i], buffer, "tx.irrigation");
  }
  for (i = 0; i < t->num_index_cardinal; i++) {
    my_snprintf(buffer, sizeof(buffer), "tx.s_farmland_%s",
		cardinal_index_str(t, i));
    SET_SPRITE_ALT(tx.farmland[i], buffer, "tx.farmland");
  }

  switch (t->darkness_style) {
  case DARKNESS_NONE:
    /* Nothing. */
    break;
  case DARKNESS_ISORECT:
    {
      /* Isometric: take a single tx.darkness tile and split it into 4. */
      struct sprite *darkness = load_sprite(t, "tx.darkness");
      const int W = t->normal_tile_width, H = t->normal_tile_height;
      int offsets[4][2] = {{W / 2, 0}, {0, H / 2}, {W / 2, H / 2}, {0, 0}};

      if (!darkness) {
	freelog(LOG_FATAL, "Sprite tx.darkness missing.");
	exit(EXIT_FAILURE);
      }
      for (i = 0; i < 4; i++) {
	t->sprites.tx.darkness[i] = crop_sprite(darkness, offsets[i][0],
						offsets[i][1], W / 2, H / 2,
						NULL, 0, 0);
      }
    }
    break;
  case DARKNESS_CARD_SINGLE:
    for (i = 0; i < t->num_cardinal_tileset_dirs; i++) {
      enum direction8 dir = t->cardinal_tileset_dirs[i];

      my_snprintf(buffer, sizeof(buffer), "tx.darkness_%s",
		  dir_get_tileset_name(dir));
      SET_SPRITE(tx.darkness[i], buffer);
    }
    break;
  case DARKNESS_CARD_FULL:
    for(i = 1; i < t->num_index_cardinal; i++) {
      my_snprintf(buffer, sizeof(buffer), "tx.darkness_%s",
		  cardinal_index_str(t, i));
      SET_SPRITE(tx.darkness[i], buffer);
    }
    break;
  case DARKNESS_CORNER:
    t->sprites.tx.fullfog = fc_realloc(t->sprites.tx.fullfog,
				       81 * sizeof(*t->sprites.tx.fullfog));
    for (i = 0; i < 81; i++) {
      /* Unknown, fog, known. */
      char ids[] = {'u', 'f', 'k'};
      char buf[512] = "t.fog";
      int values[4], j, k = i;

      for (j = 0; j < 4; j++) {
	values[j] = k % 3;
	k /= 3;

	cat_snprintf(buf, sizeof(buf), "_%c", ids[values[j]]);
      }
      assert(k == 0);

      t->sprites.tx.fullfog[i] = load_sprite(t, buf);
    }
    break;
  };

  for(i=0; i<4; i++) {
    my_snprintf(buffer, sizeof(buffer), "tx.river_outlet_%c", dir_char[i]);
    SET_SPRITE(tx.river_outlet[i], buffer);
  }

  /* no other place to initialize these variables */
  sprite_vector_init(&t->sprites.nation_flag);
  sprite_vector_init(&t->sprites.nation_shield);
}

/**************************************************************************
  Frees any internal buffers which are created by load_sprite. Should
  be called after the last (for a given period of time) load_sprite
  call.  This saves a fair amount of memory, but it will take extra time
  the next time we start loading sprites again.
**************************************************************************/
static void finish_loading_sprites(struct tileset *t)
{
  specfile_list_iterate(t->specfiles, sf) {
    if (sf->big_sprite) {
      free_sprite(sf->big_sprite);
      sf->big_sprite = NULL;
    }
  } specfile_list_iterate_end;
}
/**********************************************************************
  Load the tiles; requires tilespec_read_toplevel() called previously.
  Leads to tile_sprites being allocated and filled with pointers
  to sprites.   Also sets up and populates sprite_hash, and calls func
  to initialize 'sprites' structure.
***********************************************************************/
void tileset_load_tiles(struct tileset *t)
{
  tileset_lookup_sprite_tags(t);
  finish_loading_sprites(t);
}

/**********************************************************************
  Lookup sprite to match tag, or else to match alt if don't find,
  or else return NULL, and emit log message.
***********************************************************************/
struct sprite* tiles_lookup_sprite_tag_alt(struct tileset *t, int loglevel,
					   const char *tag, const char *alt,
					   const char *what, const char *name)
{
  struct sprite *sp;
  
  /* (should get sprite_hash before connection) */
  if (!t->sprite_hash) {
    die("attempt to lookup for %s \"%s\" before sprite_hash setup", what, name);
  }

  sp = load_sprite(t, tag);
  if (sp) return sp;

  sp = load_sprite(t, alt);
  if (sp) {
    freelog(LOG_VERBOSE,
	    "Using alternate graphic \"%s\" (instead of \"%s\") for %s \"%s\".",
	    alt, tag, what, name);
    return sp;
  }

  freelog(loglevel,
	  "Don't have graphics tags \"%s\" or \"%s\" for %s \"%s\".",
	  tag, alt, what, name);
  if (LOG_FATAL >= loglevel) {
    exit(EXIT_FAILURE);
  }
  return NULL;
}

/**********************************************************************
  Set unit_type sprite value; should only happen after
  tilespec_load_tiles().
***********************************************************************/
void tileset_setup_unit_type(struct tileset *t, struct unit_type *ut)
{
  t->sprites.unittype[utype_index(ut)] =
    tiles_lookup_sprite_tag_alt(t, LOG_FATAL, ut->graphic_str,
				ut->graphic_alt, "unit_type",
				utype_rule_name(ut));

  /* should maybe do something if NULL, eg generic default? */
}

/**********************************************************************
  Set improvement_type sprite value; should only happen after
  tilespec_load_tiles().
***********************************************************************/
void tileset_setup_impr_type(struct tileset *t,
			     struct impr_type *pimprove)
{
  t->sprites.building[improvement_index(pimprove)] =
    tiles_lookup_sprite_tag_alt(t, LOG_VERBOSE, pimprove->graphic_str,
				pimprove->graphic_alt, "improvement",
				improvement_rule_name(pimprove));

  /* should maybe do something if NULL, eg generic default? */
}

/**********************************************************************
  Set tech_type sprite value; should only happen after
  tilespec_load_tiles().
***********************************************************************/
void tileset_setup_tech_type(struct tileset *t,
			     struct advance *padvance)
{
  if (valid_advance(padvance)) {
    t->sprites.tech[advance_index(padvance)] =
      tiles_lookup_sprite_tag_alt(t, LOG_VERBOSE, padvance->graphic_str,
				  padvance->graphic_alt, "technology",
				  advance_rule_name(padvance));

    /* should maybe do something if NULL, eg generic default? */
  } else {
    t->sprites.tech[advance_index(padvance)] = NULL;
  }
}

/****************************************************************************
  Set resource sprite values; should only happen after
  tilespec_load_tiles().
****************************************************************************/
void tileset_setup_resource(struct tileset *t,
			    const struct resource *presource)
{
  assert(presource);
  t->sprites.resource[resource_index(presource)] =
    tiles_lookup_sprite_tag_alt(t, LOG_VERBOSE, presource->graphic_str,
				presource->graphic_alt, "resource",
				resource_rule_name(presource));
}

/****************************************************************************
  Set base sprite values; should only happen after
  tilespec_load_tiles().
****************************************************************************/
void tileset_setup_base(struct tileset *t,
                        const struct base_type *pbase)
{
  char full_tag_name[MAX_LEN_NAME + strlen("_fg")];
  const int id = base_index(pbase);

  assert(id >= 0 && id < base_count());

  sz_strlcpy(full_tag_name, pbase->graphic_str);
  strcat(full_tag_name, "_bg");
  t->sprites.bases[id].background = load_sprite(t, full_tag_name);

  sz_strlcpy(full_tag_name, pbase->graphic_str);
  strcat(full_tag_name, "_mg");
  t->sprites.bases[id].middleground = load_sprite(t, full_tag_name);

  sz_strlcpy(full_tag_name, pbase->graphic_str);
  strcat(full_tag_name, "_fg");
  t->sprites.bases[id].foreground = load_sprite(t, full_tag_name);

  if (t->sprites.bases[id].background == NULL
      && t->sprites.bases[id].middleground == NULL
      && t->sprites.bases[id].foreground == NULL) {
    /* No primary graphics at all. Try alternative */
    freelog(LOG_VERBOSE,
	    "Using alternate graphic \"%s\" (instead of \"%s\") for base \"%s\".",
            pbase->graphic_alt,
            pbase->graphic_str,
            base_rule_name(pbase));

    sz_strlcpy(full_tag_name, pbase->graphic_alt);
    strcat(full_tag_name, "_bg");
    t->sprites.bases[id].background = load_sprite(t, full_tag_name);

    sz_strlcpy(full_tag_name, pbase->graphic_alt);
    strcat(full_tag_name, "_mg");
    t->sprites.bases[id].middleground = load_sprite(t, full_tag_name);

    sz_strlcpy(full_tag_name, pbase->graphic_alt);
    strcat(full_tag_name, "_fg");
    t->sprites.bases[id].foreground = load_sprite(t, full_tag_name);

    if (t->sprites.bases[id].background == NULL
        && t->sprites.bases[id].middleground == NULL
        && t->sprites.bases[id].foreground == NULL) {
      /* Cannot find alternative graphics either */
      freelog(LOG_FATAL, "No graphics for base \"%s\" at all!",
              base_rule_name(pbase));
      exit(EXIT_FAILURE);
    }
  }

  t->sprites.bases[id].activity = load_sprite(t, pbase->activity_gfx);
  if (t->sprites.bases[id].activity == NULL) {
    freelog(LOG_FATAL, "Missing %s building activity tag \"%s\".",
            base_rule_name(pbase),
            pbase->activity_gfx);
    exit(EXIT_FAILURE);
  }
}


/**********************************************************************
  Set tile_type sprite values; should only happen after
  tilespec_load_tiles().
***********************************************************************/
void tileset_setup_tile_type(struct tileset *t,
			     const struct terrain *pterrain)
{
  struct drawing_data *draw;
  struct sprite *sprite;
  char buffer[MAX_LEN_NAME + 20];
  int i, l;
  
  if (0 == strlen(terrain_rule_name(pterrain))) {
    return;
  }

  draw = hash_lookup_data(t->tile_hash, pterrain->graphic_str);
  if (!draw) {
    draw = hash_lookup_data(t->tile_hash, pterrain->graphic_alt);
    if (!draw) {
      freelog(LOG_FATAL, "Terrain \"%s\": no graphic tile \"%s\" or \"%s\".",
	      terrain_rule_name(pterrain),
	      pterrain->graphic_str,
	      pterrain->graphic_alt);
      exit(EXIT_FAILURE);
    }
  }

  /* Set up each layer of the drawing. */
  for (l = 0; l < draw->num_layers; l++) {
    struct drawing_layer *dlp = &draw->layer[l];
    struct tileset_layer *tslp = &t->layers[l];
    sprite_vector_init(&dlp->base);

    switch (dlp->sprite_type) {
    case CELL_WHOLE:
      switch (dlp->match_style) {
      case MATCH_NONE:
	/* Load whole sprites for this tile. */
	for (i = 0; ; i++) {
	  my_snprintf(buffer, sizeof(buffer), "t.l%d.%s%d",
		      l,
		      draw->name,
		      i + 1);
	  sprite = load_sprite(t, buffer);
	  if (!sprite) {
	    break;
	  }
	  sprite_vector_reserve(&dlp->base, i + 1);
	  dlp->base.p[i] = sprite;
	}
	/* check for base sprite, allowing missing sprites above base */
	if (0 == i  &&  0 == l) {
	  freelog(LOG_FATAL, "Missing base sprite tag \"%s\".",
		  buffer);
	  exit(EXIT_FAILURE);
	}
	break;
      case MATCH_SAME:
	/* Load 16 cardinally-matched sprites. */
	for (i = 0; i < t->num_index_cardinal; i++) {
	  my_snprintf(buffer, sizeof(buffer), "t.l%d.%s_%s",
		      l,
		      draw->name,
		      cardinal_index_str(t, i));
	  dlp->match[i] =
	    tiles_lookup_sprite_tag_alt(t, LOG_FATAL, buffer, "",
					"matched terrain",
					terrain_rule_name(pterrain));
	}
	break;
      case MATCH_PAIR:
      case MATCH_FULL:
	assert(0); /* not yet defined */
	break;
      };
      break;
    case CELL_CORNER:
      {
	const int count = dlp->match_indices;
	int number = NUM_CORNER_DIRS;

	switch (dlp->match_style) {
	case MATCH_NONE:
	  /* do nothing */
	  break;
	case MATCH_PAIR:
	case MATCH_SAME:
	  /* N directions (NSEW) * 3 dimensions of matching */
	  assert(count == 2);
	  number = NUM_CORNER_DIRS * 2 * 2 * 2;
	  break;
	case MATCH_FULL:
	default:
	  /* N directions (NSEW) * 3 dimensions of matching */
	  /* could use exp() or expi() here? */
	  number = NUM_CORNER_DIRS * count * count * count;
	  break;
	};

	dlp->cells
	  = fc_calloc(number, sizeof(*dlp->cells));

	for (i = 0; i < number; i++) {
	  enum direction4 dir = i % NUM_CORNER_DIRS;
	  int value = i / NUM_CORNER_DIRS;

	  switch (dlp->match_style) {
	  case MATCH_NONE:
	    my_snprintf(buffer, sizeof(buffer), "t.l%d.%s_cell_%c",
			l,
			draw->name,
			direction4letters[dir]);
	    dlp->cells[i] =
	      tiles_lookup_sprite_tag_alt(t, LOG_FATAL, buffer, "",
					  "cell terrain",
					  terrain_rule_name(pterrain));
	    break;
	  case MATCH_SAME:
	    my_snprintf(buffer, sizeof(buffer), "t.l%d.%s_cell_%c%d%d%d",
			l,
			draw->name,
			direction4letters[dir],
			(value) & 1,
			(value >> 1) & 1,
			(value >> 2) & 1);
	    dlp->cells[i] =
	      tiles_lookup_sprite_tag_alt(t, LOG_FATAL, buffer, "",
					  "same cell terrain",
					  terrain_rule_name(pterrain));
	    break;
	  case MATCH_PAIR:
	    my_snprintf(buffer, sizeof(buffer), "t.l%d.%s_cell_%c_%c_%c_%c",
			l,
			draw->name,
			direction4letters[dir],
			tslp->match_types[dlp->match_index[(value) & 1]][0],
			tslp->match_types[dlp->match_index[(value >> 1) & 1]][0],
			tslp->match_types[dlp->match_index[(value >> 2) & 1]][0]);
	    dlp->cells[i] =
	      tiles_lookup_sprite_tag_alt(t, LOG_FATAL, buffer, "",
					  "cell pair terrain",
					  terrain_rule_name(pterrain));
	    break;
	  case MATCH_FULL:
	    {
	      int this = dlp->match_index[0];
	      int n, s, e, w;
	      int v1, v2, v3;

	      v1 = dlp->match_index[value % count];
	      value /= count;
	      v2 = dlp->match_index[value % count];
	      value /= count;
	      v3 = dlp->match_index[value % count];

	      assert(v1 < count && v2 < count && v3 < count);

	      /* Assume merged cells.  This should be a separate option. */
	      switch (dir) {
	      case DIR4_NORTH:
		s = this;
		w = v1;
		n = v2;
		e = v3;
		break;
	      case DIR4_EAST:
		w = this;
		n = v1;
		e = v2;
		s = v3;
		break;
	      case DIR4_SOUTH:
		n = this;
		e = v1;
		s = v2;
		w = v3;
		break;
	      case DIR4_WEST:
	      default:		/* avoid warnings */
		e = this;
		s = v1;
		w = v2;
		n = v3;
		break;
	      };

	      /* Use first character of match_types,
	       * already checked for uniqueness. */
	      my_snprintf(buffer, sizeof(buffer),
			  "t.l%d.cellgroup_%c_%c_%c_%c",
			  l,
			  tslp->match_types[n][0],
			  tslp->match_types[e][0],
			  tslp->match_types[s][0],
			  tslp->match_types[w][0]);
	      sprite = load_sprite(t, buffer);

	      if (sprite) {
		/* Crop the sprite to separate this cell. */
		const int W = t->normal_tile_width;
		const int H = t->normal_tile_height;
		int x[4] = {W / 4, W / 4, 0, W / 2};
		int y[4] = {H / 2, 0, H / 4, H / 4};
		int xo[4] = {0, 0, -W / 2, W / 2};
		int yo[4] = {H / 2, -H / 2, 0, 0};

		sprite = crop_sprite(sprite,
				     x[dir], y[dir], W / 2, H / 2,
				     t->sprites.mask.tile,
				     xo[dir], yo[dir]);
	      } else {
		freelog(LOG_ERROR, "Terrain graphics tag \"%s\" missing.",
			buffer);
	      }

	      dlp->cells[i] = sprite;
	    }
	    break;
	  };
	}
      }
      break;
    };
  }

  /* try an optional special name */
  my_snprintf(buffer, sizeof(buffer), "t.blend.%s", draw->name);
  draw->blender =
    tiles_lookup_sprite_tag_alt(t, LOG_VERBOSE, buffer, "",
				"blend terrain",
				terrain_rule_name(pterrain));

  if (draw->blending > 0) {
    const int l = draw->blending - 1;

    if (NULL == draw->blender) {
      int i = 0;
      /* try an already loaded base */
      while (NULL == draw->blender
        &&  i < draw->blending
        &&  0 < draw->layer[i].base.size) {
        draw->blender = draw->layer[i++].base.p[0];
      }
    }

    if (NULL == draw->blender) {
      /* try an unloaded base name */
      my_snprintf(buffer, sizeof(buffer), "t.l%d.%s1", l, draw->name);
      draw->blender =
	tiles_lookup_sprite_tag_alt(t, LOG_FATAL, buffer, "",
				    "base (blend) terrain",
				    terrain_rule_name(pterrain));
    }
  }

  if (NULL != draw->blender) {
    /* Set up blending sprites. This only works in iso-view! */
    const int W = t->normal_tile_width;
    const int H = t->normal_tile_height;
    const int offsets[4][2] = {
      {W / 2, 0}, {0, H / 2}, {W / 2, H / 2}, {0, 0}
    };
    enum direction4 dir = 0;

    for (; dir < 4; dir++) {
      draw->blend[dir] = crop_sprite(draw->blender,
				     offsets[dir][0], offsets[dir][1],
				     W / 2, H / 2,
				     t->sprites.dither_tile, 0, 0);
    }
  }

  if (draw->mine_tag) {
    draw->mine = load_sprite(t, draw->mine_tag);
  } else {
    draw->mine = NULL;
  }

  t->sprites.drawing[terrain_index(pterrain)] = draw;

  color_system_setup_terrain(t->color_system, pterrain, draw->name);
}

/**********************************************************************
  Set government sprite value; should only happen after
  tilespec_load_tiles().
***********************************************************************/
void tileset_setup_government(struct tileset *t,
			      struct government *gov)
{
  t->sprites.government[government_index(gov)] =
    tiles_lookup_sprite_tag_alt(t, LOG_FATAL, gov->graphic_str,
				gov->graphic_alt, "government",
				government_rule_name(gov));
  
  /* should probably do something if NULL, eg generic default? */
}

/**********************************************************************
  Set nation flag sprite value; should only happen after
  tilespec_load_tiles().
***********************************************************************/
void tileset_setup_nation_flag(struct tileset *t, 
			       struct nation_type *nation)
{
  char *tags[] = {nation->flag_graphic_str,
		  nation->flag_graphic_alt,
		  "unknown", NULL};
  int i;
  struct sprite *flag = NULL, *shield = NULL;
  char buf[1024];

  for (i = 0; tags[i] && !flag; i++) {
    my_snprintf(buf, sizeof(buf), "f.%s", tags[i]);
    flag = load_sprite(t, buf);
  }
  for (i = 0; tags[i] && !shield; i++) {
    my_snprintf(buf, sizeof(buf), "f.shield.%s", tags[i]);
    shield = load_sprite(t, buf);
  }
  if (!flag || !shield) {
    /* Should never get here because of the f.unknown fallback. */
    freelog(LOG_FATAL, "Nation %s: no national flag.",
            nation_rule_name(nation));
    exit(EXIT_FAILURE);
  }

  sprite_vector_reserve(&t->sprites.nation_flag, nation_count());
  t->sprites.nation_flag.p[nation_index(nation)] = flag;

  sprite_vector_reserve(&t->sprites.nation_shield, nation_count());
  t->sprites.nation_shield.p[nation_index(nation)] = shield;
}

/**********************************************************************
  Return the flag graphic to be used by the city.
***********************************************************************/
struct sprite *get_city_flag_sprite(const struct tileset *t,
				    const struct city *pcity)
{
  return get_nation_flag_sprite(t, nation_of_city(pcity));
}

/**********************************************************************
  Return a sprite for the national flag for this unit.
***********************************************************************/
static struct sprite *get_unit_nation_flag_sprite(const struct tileset *t,
						  const struct unit *punit)
{
  struct nation_type *pnation = nation_of_unit(punit);

  if (draw_unit_shields) {
    return t->sprites.nation_shield.p[nation_index(pnation)];
  } else {
    return t->sprites.nation_flag.p[nation_index(pnation)];
  }
}

#define FULL_TILE_X_OFFSET ((t->normal_tile_width - t->full_tile_width) / 2)
#define FULL_TILE_Y_OFFSET (t->normal_tile_height - t->full_tile_height)

#define ADD_SPRITE(s, draw_fog, x_offset, y_offset)			    \
  (assert(s != NULL),							    \
   sprs->sprite = s,							    \
   sprs->foggable = (draw_fog && t->fogstyle == FOG_AUTO),		    \
   sprs->offset_x = x_offset,						    \
   sprs->offset_y = y_offset,						    \
   sprs++)
#define ADD_SPRITE_SIMPLE(s) ADD_SPRITE(s, TRUE, 0, 0)
#define ADD_SPRITE_FULL(s)						    \
  ADD_SPRITE(s, TRUE, FULL_TILE_X_OFFSET, FULL_TILE_Y_OFFSET)

/**************************************************************************
  Assemble some data that is used in building the tile sprite arrays.
    (map_x, map_y) : the (normalized) map position
  The values we fill in:
    tterrain_near  : terrain types of all adjacent terrain
    tspecial_near  : specials of all adjacent terrain
**************************************************************************/
static void build_tile_data(const struct tile *ptile,
			    struct terrain *pterrain,
			    struct terrain **tterrain_near,
			    bv_special *tspecial_near)
{
  enum direction8 dir;

  /* Loop over all adjacent tiles.  We should have an iterator for this. */
  for (dir = 0; dir < 8; dir++) {
    struct tile *tile1 = mapstep(ptile, dir);

    if (tile1 && client_tile_get_known(tile1) != TILE_UNKNOWN) {
      struct terrain *terrain1 = tile_terrain(tile1);

      if (NULL != terrain1) {
        tterrain_near[dir] = terrain1;
        tspecial_near[dir] = tile_specials(tile1);
        continue;
      }
      freelog(LOG_ERROR, "build_tile_data() tile (%d,%d) has no terrain!",
              TILE_XY(tile1));
    }
    /* At the edges of the (known) map, pretend the same terrain continued
     * past the edge of the map. */
    tterrain_near[dir] = pterrain;
    BV_CLR_ALL(tspecial_near[dir]);
  }
}

/**********************************************************************
  Fill in the sprite array for the unit
***********************************************************************/
static int fill_unit_sprite_array(const struct tileset *t,
				  struct drawn_sprite *sprs,
				  const struct unit *punit,
				  bool stack, bool backdrop)
{
  struct drawn_sprite *save_sprs = sprs;
  int ihp;

  if (backdrop) {
    if (!solid_color_behind_units) {
      ADD_SPRITE(get_unit_nation_flag_sprite(t, punit), TRUE,
		 FULL_TILE_X_OFFSET + t->unit_flag_offset_x,
		 FULL_TILE_Y_OFFSET + t->unit_flag_offset_y);
    } else {
      /* Taken care of in the LAYER_BACKGROUND. */
    }
  }

  ADD_SPRITE(t->sprites.unittype[utype_index(unit_type(punit))], TRUE,
	     FULL_TILE_X_OFFSET + t->unit_offset_x,
	     FULL_TILE_Y_OFFSET + t->unit_offset_y);

  if (t->sprites.unit.loaded && punit->transported_by != -1) {
    ADD_SPRITE_FULL(t->sprites.unit.loaded);
  }

  if(punit->activity!=ACTIVITY_IDLE) {
    struct sprite *s = NULL;
    switch(punit->activity) {
    case ACTIVITY_MINE:
      s = t->sprites.unit.mine;
      break;
    case ACTIVITY_POLLUTION:
      s = t->sprites.unit.pollution;
      break;
    case ACTIVITY_FALLOUT:
      s = t->sprites.unit.fallout;
      break;
    case ACTIVITY_PILLAGE:
      s = t->sprites.unit.pillage;
      break;
    case ACTIVITY_ROAD:
    case ACTIVITY_RAILROAD:
      s = t->sprites.unit.road;
      break;
    case ACTIVITY_IRRIGATE:
      s = t->sprites.unit.irrigate;
      break;
    case ACTIVITY_EXPLORE:
      s = t->sprites.unit.auto_explore;
      break;
    case ACTIVITY_FORTIFIED:
      s = t->sprites.unit.fortified;
      break;
    case ACTIVITY_FORTIFYING:
      s = t->sprites.unit.fortifying;
      break;
    case ACTIVITY_SENTRY:
      s = t->sprites.unit.sentry;
      break;
    case ACTIVITY_GOTO:
      s = t->sprites.unit.go_to;
      break;
    case ACTIVITY_TRANSFORM:
      s = t->sprites.unit.transform;
      break;
    case ACTIVITY_BASE:
      s = t->sprites.bases[punit->activity_base].activity;
      break;
    default:
      break;
    }

    ADD_SPRITE_FULL(s);
  }

  if (punit->ai.control && punit->activity != ACTIVITY_EXPLORE) {
    if (is_military_unit(punit)) {
      ADD_SPRITE_FULL(t->sprites.unit.auto_attack);
    } else {
      ADD_SPRITE_FULL(t->sprites.unit.auto_settler);
    }
  }

  if (unit_has_orders(punit)) {
    if (punit->orders.repeat) {
      ADD_SPRITE_FULL(t->sprites.unit.patrol);
    } else if (punit->activity != ACTIVITY_IDLE) {
      ADD_SPRITE_SIMPLE(t->sprites.unit.connect);
    } else {
      ADD_SPRITE_FULL(t->sprites.unit.go_to);
    }
  }

  if (punit->battlegroup != BATTLEGROUP_NONE) {
    ADD_SPRITE_FULL(t->sprites.unit.battlegroup[punit->battlegroup]);
  }

  if (t->sprites.unit.lowfuel
      && utype_fuel(unit_type(punit))
      && punit->fuel == 1
      && punit->moves_left <= 2 * SINGLE_MOVE) {
    /* Show a low-fuel graphic if the plane has 2 or fewer moves left. */
    ADD_SPRITE_FULL(t->sprites.unit.lowfuel);
  }
  if (t->sprites.unit.tired
      && punit->moves_left < SINGLE_MOVE) {
    /* Show a "tired" graphic if the unit has fewer than one move
     * remaining. */
    ADD_SPRITE_FULL(t->sprites.unit.tired);
  }

  if (stack || punit->occupy) {
    ADD_SPRITE_FULL(t->sprites.unit.stack);
  }

  if (t->sprites.unit.vet_lev[punit->veteran]) {
    ADD_SPRITE_FULL(t->sprites.unit.vet_lev[punit->veteran]);
  }

  ihp = ((NUM_TILES_HP_BAR-1)*punit->hp) / unit_type(punit)->hp;
  ihp = CLIP(0, ihp, NUM_TILES_HP_BAR-1);
  ADD_SPRITE_FULL(t->sprites.unit.hp_bar[ihp]);

  return sprs - save_sprs;
}

/**************************************************************************
  Add any corner road sprites to the sprite array.
**************************************************************************/
static int fill_road_corner_sprites(const struct tileset *t,
				    struct drawn_sprite *sprs,
				    bool road, bool *road_near,
				    bool rail, bool *rail_near)
{
  struct drawn_sprite *saved_sprs = sprs;
  int i;

  assert(draw_roads_rails);

  /* Roads going diagonally adjacent to this tile need to be
   * partly drawn on this tile. */

  /* Draw the corner sprite if:
   *   - There is a diagonal road (not rail!) between two adjacent tiles.
   *   - There is no diagonal road (not rail!) that intersects this road.
   * The logic is simple: roads are drawn underneath railrods, but are
   * not always covered by them (even in the corners!).  But if a railroad
   * connects two tiles, only the railroad (no road) is drawn between
   * those tiles.
   */
  for (i = 0; i < t->num_valid_tileset_dirs; i++) {
    enum direction8 dir = t->valid_tileset_dirs[i];

    if (!is_cardinal_tileset_dir(t, dir)) {
      /* Draw corner sprites for this non-cardinal direction. */
      int cw = (i + 1) % t->num_valid_tileset_dirs;
      int ccw
	= (i + t->num_valid_tileset_dirs - 1) % t->num_valid_tileset_dirs;
      enum direction8 dir_cw = t->valid_tileset_dirs[cw];
      enum direction8 dir_ccw = t->valid_tileset_dirs[ccw];

      if (t->sprites.road.corner[dir]
	  && (road_near[dir_cw] && road_near[dir_ccw]
	      && !(rail_near[dir_cw] && rail_near[dir_ccw]))
	  && !(road && road_near[dir] && !(rail && rail_near[dir]))) {
	ADD_SPRITE_SIMPLE(t->sprites.road.corner[dir]);
      }
    }
  }

  return sprs - saved_sprs;
}

/**************************************************************************
  Add any corner rail sprites to the sprite array.
**************************************************************************/
static int fill_rail_corner_sprites(const struct tileset *t,
				    struct drawn_sprite *sprs,
				    bool rail, bool *rail_near)
{
  struct drawn_sprite *saved_sprs = sprs;
  int i;

  assert(draw_roads_rails);

  /* Rails going diagonally adjacent to this tile need to be
   * partly drawn on this tile. */

  for (i = 0; i < t->num_valid_tileset_dirs; i++) {
    enum direction8 dir = t->valid_tileset_dirs[i];

    if (!is_cardinal_tileset_dir(t, dir)) {
      /* Draw corner sprites for this non-cardinal direction. */
      int cw = (i + 1) % t->num_valid_tileset_dirs;
      int ccw
	= (i + t->num_valid_tileset_dirs - 1) % t->num_valid_tileset_dirs;
      enum direction8 dir_cw = t->valid_tileset_dirs[cw];
      enum direction8 dir_ccw = t->valid_tileset_dirs[ccw];

      if (t->sprites.rail.corner[dir]
	  && rail_near[dir_cw] && rail_near[dir_ccw]
	  && !(rail && rail_near[dir])) {
	ADD_SPRITE_SIMPLE(t->sprites.rail.corner[dir]);
      }
    }
  }

  return sprs - saved_sprs;
}

/**************************************************************************
  Fill all road and rail sprites into the sprite array.
**************************************************************************/
static int fill_road_rail_sprite_array(const struct tileset *t,
				       struct drawn_sprite *sprs,
				       bv_special tspecial,
				       bv_special *tspecial_near,
				       const struct city *pcity)
{
  struct drawn_sprite *saved_sprs = sprs;
  bool road, road_near[8], rail, rail_near[8];
  bool draw_road[8], draw_single_road, draw_rail[8], draw_single_rail;
  enum direction8 dir;

  if (!draw_roads_rails) {
    /* Don't draw anything. */
    return 0;
  }

  /* Fill some data arrays. rail_near and road_near store whether road/rail
   * is present in the given direction.  draw_rail and draw_road store
   * whether road/rail is to be drawn in that direction.  draw_single_road
   * and draw_single_rail store whether we need an isolated road/rail to be
   * drawn. */
  road = contains_special(tspecial, S_ROAD);
  rail = contains_special(tspecial, S_RAILROAD);
  draw_single_road = road && (!pcity || !draw_cities) && !rail;
  draw_single_rail = rail && (!pcity || !draw_cities);
  for (dir = 0; dir < 8; dir++) {
    /* Check if there is adjacent road/rail. */
    road_near[dir] = contains_special(tspecial_near[dir], S_ROAD);
    rail_near[dir] = contains_special(tspecial_near[dir], S_RAILROAD);

    /* Draw rail/road if there is a connection from this tile to the
     * adjacent tile.  But don't draw road if there is also a rail
     * connection. */
    draw_rail[dir] = rail && rail_near[dir];
    draw_road[dir] = road && road_near[dir] && !draw_rail[dir];

    /* Don't draw an isolated road/rail if there's any connection. */
    draw_single_rail &= !draw_rail[dir];
    draw_single_road &= !draw_rail[dir] && !draw_road[dir];
  }

  /* Draw road corners underneath rails (styles 0 and 1). */
  sprs
    += fill_road_corner_sprites(t, sprs, road, road_near, rail, rail_near);

  if (t->roadstyle == 0) {
    /* With roadstyle 0, we simply draw one road/rail for every connection.
     * This means we only need a few sprites, but a lot of drawing is
     * necessary and it generally doesn't look very good. */
    int i;

    /* First raw roads under rails. */
    if (road) {
      for (i = 0; i < t->num_valid_tileset_dirs; i++) {
	if (draw_road[t->valid_tileset_dirs[i]]) {
	  ADD_SPRITE_SIMPLE(t->sprites.road.dir[i]);
	}
      }
    }

    /* Then draw rails over roads. */
    if (rail) {
      for (i = 0; i < t->num_valid_tileset_dirs; i++) {
	if (draw_rail[t->valid_tileset_dirs[i]]) {
	  ADD_SPRITE_SIMPLE(t->sprites.rail.dir[i]);
	}
      }
    }
  } else if (t->roadstyle == 1) {
    /* With roadstyle 1, we draw one sprite for cardinal road connections,
     * one sprite for diagonal road connections, and the same for rail.
     * This means we need about 4x more sprites than in style 0, but up to
     * 4x less drawing is needed.  The drawing quality may also be
     * improved. */

    /* First draw roads under rails. */
    if (road) {
      int road_even_tileno = 0, road_odd_tileno = 0, i;

      for (i = 0; i < t->num_valid_tileset_dirs / 2; i++) {
	enum direction8 even = t->valid_tileset_dirs[2 * i];
	enum direction8 odd = t->valid_tileset_dirs[2 * i + 1];

	if (draw_road[even]) {
	  road_even_tileno |= 1 << i;
	}
	if (draw_road[odd]) {
	  road_odd_tileno |= 1 << i;
	}
      }

      /* Draw the cardinal/even roads first. */
      if (road_even_tileno != 0) {
	ADD_SPRITE_SIMPLE(t->sprites.road.even[road_even_tileno]);
      }
      if (road_odd_tileno != 0) {
	ADD_SPRITE_SIMPLE(t->sprites.road.odd[road_odd_tileno]);
      }
    }

    /* Then draw rails over roads. */
    if (rail) {
      int rail_even_tileno = 0, rail_odd_tileno = 0, i;

      for (i = 0; i < t->num_valid_tileset_dirs / 2; i++) {
	enum direction8 even = t->valid_tileset_dirs[2 * i];
	enum direction8 odd = t->valid_tileset_dirs[2 * i + 1];

	if (draw_rail[even]) {
	  rail_even_tileno |= 1 << i;
	}
	if (draw_rail[odd]) {
	  rail_odd_tileno |= 1 << i;
	}
      }

      /* Draw the cardinal/even rails first. */
      if (rail_even_tileno != 0) {
	ADD_SPRITE_SIMPLE(t->sprites.rail.even[rail_even_tileno]);
      }
      if (rail_odd_tileno != 0) {
	ADD_SPRITE_SIMPLE(t->sprites.rail.odd[rail_odd_tileno]);
      }
    }
  } else {
    /* Roadstyle 2 is a very simple method that lets us simply retrieve 
     * entire finished tiles, with a bitwise index of the presence of
     * roads in each direction. */

    /* Draw roads first */
    if (road) {
      int road_tileno = 0, i;

      for (i = 0; i < t->num_valid_tileset_dirs; i++) {
	enum direction8 dir = t->valid_tileset_dirs[i];

	if (draw_road[dir]) {
	  road_tileno |= 1 << i;
	}
      }

      if (road_tileno != 0 || draw_single_road) {
        ADD_SPRITE_SIMPLE(t->sprites.road.total[road_tileno]);
      }
    }

    /* Then draw rails over roads. */
    if (rail) {
      int rail_tileno = 0, i;

      for (i = 0; i < t->num_valid_tileset_dirs; i++) {
	enum direction8 dir = t->valid_tileset_dirs[i];

	if (draw_rail[dir]) {
	  rail_tileno |= 1 << i;
	}
      }

      if (rail_tileno != 0 || draw_single_rail) {
        ADD_SPRITE_SIMPLE(t->sprites.rail.total[rail_tileno]);
      }
    }
  }

  /* Draw isolated rail/road separately (styles 0 and 1 only). */
  if (t->roadstyle == 0 || t->roadstyle == 1) { 
    if (draw_single_rail) {
      ADD_SPRITE_SIMPLE(t->sprites.rail.isolated);
    } else if (draw_single_road) {
      ADD_SPRITE_SIMPLE(t->sprites.road.isolated);
    }
  }

  /* Draw rail corners over roads (styles 0 and 1). */
  sprs += fill_rail_corner_sprites(t, sprs, rail, rail_near);

  return sprs - saved_sprs;
}

/**************************************************************************
  Return the index of the sprite to be used for irrigation or farmland in
  this tile.

  We assume that the current tile has farmland or irrigation.  We then
  choose a sprite (index) based upon which cardinally adjacent tiles have
  either farmland or irrigation (the two are considered interchangable for
  this).
**************************************************************************/
static int get_irrigation_index(const struct tileset *t,
				bv_special *tspecial_near)
{
  int tileno = 0, i;

  for (i = 0; i < t->num_cardinal_tileset_dirs; i++) {
    enum direction8 dir = t->cardinal_tileset_dirs[i];

    /* A tile with S_FARMLAND will also have S_IRRIGATION set. */
    if (contains_special(tspecial_near[dir], S_IRRIGATION)) {
      tileno |= 1 << i;
    }
  }

  return tileno;
}

/**************************************************************************
  Fill in the farmland/irrigation sprite for the tile.
**************************************************************************/
static int fill_irrigation_sprite_array(const struct tileset *t,
					struct drawn_sprite *sprs,
					bv_special tspecial,
					bv_special *tspecial_near,
					const struct city *pcity)
{
  struct drawn_sprite *saved_sprs = sprs;

  /* Tiles with S_FARMLAND also have S_IRRIGATION set. */
  assert(!contains_special(tspecial, S_FARMLAND)
	 || contains_special(tspecial, S_IRRIGATION));

  /* We don't draw the irrigation if there's a city (it just gets overdrawn
   * anyway, and ends up looking bad). */
  if (draw_irrigation
      && contains_special(tspecial, S_IRRIGATION)
      && !(pcity && draw_cities)) {
    int index = get_irrigation_index(t, tspecial_near);

    if (contains_special(tspecial, S_FARMLAND)) {
      ADD_SPRITE_SIMPLE(t->sprites.tx.farmland[index]);
    } else {
      ADD_SPRITE_SIMPLE(t->sprites.tx.irrigation[index]);
    }
  }

  return sprs - saved_sprs;
}

/**************************************************************************
  Fill in the city overlays for the tile.  This includes the citymap
  overlays on the mapview as well as the tile output sprites.
**************************************************************************/
static int fill_city_overlays_sprite_array(const struct tileset *t,
					   struct drawn_sprite *sprs,
					   const struct tile *ptile,
					   const struct city *citymode)
{
  const struct city *pcity;
  const struct city *pwork;
  struct unit *psettler;
  struct drawn_sprite *saved_sprs = sprs;
  int city_x, city_y;
  const int NUM_CITY_COLORS = t->sprites.city.worked_tile_overlay.size;

  if (NULL == ptile || TILE_UNKNOWN == client_tile_get_known(ptile)) {
    return 0;
  }
  pwork = tile_worked(ptile);

  if (citymode) {
    pcity = citymode;
  } else {
    pcity = find_city_or_settler_near_tile(ptile, &psettler);
  }

  /* Below code does not work if pcity is invisible.
   * Make sure it is not. */
  assert(pcity == NULL || pcity->tile != NULL);
  if (pcity && !pcity->tile) {
    pcity = NULL;
  }

  if (pcity && city_base_to_city_map(&city_x, &city_y, pcity, ptile)) {
    /* FIXME: check elsewhere for valid tile (instead of above) */

    if (!citymode && pcity->client.colored) {
      /* Add citymap overlay for a city. */
      int index = pcity->client.color_index % NUM_CITY_COLORS;

      if (NULL != pwork && pwork == pcity) {
        ADD_SPRITE_SIMPLE(t->sprites.city.worked_tile_overlay.p[index]);
      } else if (city_can_work_tile(pcity, ptile)) {
        ADD_SPRITE_SIMPLE(t->sprites.city.unworked_tile_overlay.p[index]);
      }
    } else if (NULL != pwork && pwork == pcity
               && (citymode || draw_city_output)) {
      /* Add on the tile output sprites. */
      int food = city_tile_output_now(pcity, ptile, O_FOOD);
      int shields = city_tile_output_now(pcity, ptile, O_SHIELD);
      int trade = city_tile_output_now(pcity, ptile, O_TRADE);
      const int ox = t->is_isometric ? t->normal_tile_width / 3 : 0;
      const int oy = t->is_isometric ? -t->normal_tile_height / 3 : 0;

      food = CLIP(0, food, NUM_TILES_DIGITS - 1);
      shields = CLIP(0, shields, NUM_TILES_DIGITS - 1);
      trade = CLIP(0, trade, NUM_TILES_DIGITS - 1);

      ADD_SPRITE(t->sprites.city.tile_foodnum[food], TRUE, ox, oy);
      ADD_SPRITE(t->sprites.city.tile_shieldnum[shields], TRUE, ox, oy);
      ADD_SPRITE(t->sprites.city.tile_tradenum[trade], TRUE, ox, oy);
    }
  } else if (psettler && psettler->client.colored) {
    /* Add citymap overlay for a unit. */
    int index = psettler->client.color_index % NUM_CITY_COLORS;

    ADD_SPRITE_SIMPLE(t->sprites.city.unworked_tile_overlay.p[index]);
  }

  return sprs - saved_sprs;
}

/****************************************************************************
  Helper function for fill_terrain_sprite_layer.
  Fill in the sprite array for blended terrain.
****************************************************************************/
static int fill_terrain_sprite_blending(const struct tileset *t,
					struct drawn_sprite *sprs,
					const struct tile *ptile,
					const struct terrain *pterrain,
					struct terrain **tterrain_near)
{
  struct drawn_sprite *saved_sprs = sprs;
  const int W = t->normal_tile_width, H = t->normal_tile_height;
  const int offsets[4][2] = {
    {W/2, 0}, {0, H / 2}, {W / 2, H / 2}, {0, 0}
  };
  enum direction4 dir = 0;

  /*
   * We want to mark unknown tiles so that an unreal tile will be
   * given the same marking as our current tile - that way we won't
   * get the "unknown" dither along the edge of the map.
   */
  for (; dir < 4; dir++) {
    struct tile *tile1 = mapstep(ptile, DIR4_TO_DIR8[dir]);
    struct terrain *other;

    if (!tile1
	|| client_tile_get_known(tile1) == TILE_UNKNOWN
	|| pterrain == (other = tterrain_near[DIR4_TO_DIR8[dir]])
	|| (0 == t->sprites.drawing[terrain_index(other)]->blending
	   &&  NULL == t->sprites.drawing[terrain_index(other)]->blender)) {
      continue;
    }

    ADD_SPRITE(t->sprites.drawing[terrain_index(other)]->blend[dir], TRUE,
	       offsets[dir][0], offsets[dir][1]);
  }

  return sprs - saved_sprs;
}

/****************************************************************************
  Add sprites for fog (and some forms of darkness).
****************************************************************************/
static int fill_fog_sprite_array(const struct tileset *t,
				 struct drawn_sprite *sprs,
				 const struct tile *ptile,
				 const struct tile_edge *pedge,
				 const struct tile_corner *pcorner)
{
  struct drawn_sprite *saved_sprs = sprs;

  if (t->fogstyle == FOG_SPRITE && draw_fog_of_war
      && NULL != ptile
      && TILE_KNOWN_UNSEEN == client_tile_get_known(ptile)) {
    /* With FOG_AUTO, fog is done this way. */
    ADD_SPRITE_SIMPLE(t->sprites.tx.fog);
  }

  if (t->darkness_style == DARKNESS_CORNER && pcorner && draw_fog_of_war) {
    int i, tileno = 0;

    for (i = 3; i >= 0; i--) {
      const int unknown = 0, fogged = 1, known = 2;
      int value = -1;

      if (!pcorner->tile[i]) {
	value = fogged;
      } else {
	switch (client_tile_get_known(pcorner->tile[i])) {
	case TILE_KNOWN_SEEN:
	  value = known;
	  break;
	case TILE_KNOWN_UNSEEN:
	  value = fogged;
	  break;
	case TILE_UNKNOWN:
	  value = unknown;
	  break;
	}
      }
      assert(value >= 0 && value < 3);

      tileno = tileno * 3 + value;
    }

    if (t->sprites.tx.fullfog[tileno]) {
      ADD_SPRITE_SIMPLE(t->sprites.tx.fullfog[tileno]);
    }
  }

  return sprs - saved_sprs;
}

/****************************************************************************
  Helper function for fill_terrain_sprite_layer.
****************************************************************************/
static int fill_terrain_sprite_array(struct tileset *t,
				     struct drawn_sprite *sprs,
				     int l, /* layer_num */
				     const struct tile *ptile,
				     const struct terrain *pterrain,
				     struct terrain **tterrain_near,
				     struct drawing_data *draw)
{
  struct drawn_sprite *saved_sprs = sprs;
  struct drawing_layer *dlp = &draw->layer[l];
  int this = dlp->match_index[0];
  int that = dlp->match_index[1];
  int ox = dlp->offset_x;
  int oy = dlp->offset_y;
  int i;

#define MATCH(dir)							    \
    (t->sprites.drawing[terrain_index(tterrain_near[(dir)])]->num_layers > l	    \
     ? t->sprites.drawing[terrain_index(tterrain_near[(dir)])]->layer[l].match_index[0] \
     : -1)

  switch (dlp->sprite_type) {
  case CELL_WHOLE:
    {
      switch (dlp->match_style) {
      case MATCH_NONE:
	{
	  int count = sprite_vector_size(&dlp->base);

	  if (count > 0) {
	    /* Pseudo-random reproducable algorithm to pick a sprite. */
	    count = myrandomly(tile_index(ptile), count);

	    if (dlp->is_tall) {
	      ox += FULL_TILE_X_OFFSET;
	      oy += FULL_TILE_Y_OFFSET;
	    }
	    ADD_SPRITE(dlp->base.p[count], TRUE, ox, oy);
	  }
	  break;
	}
      case MATCH_SAME:
	{
	  int tileno = 0;

	  for (i = 0; i < t->num_cardinal_tileset_dirs; i++) {
	    enum direction8 dir = t->cardinal_tileset_dirs[i];

	    if (MATCH(dir) == this) {
	      tileno |= 1 << i;
	    }
	  }

	  if (dlp->is_tall) {
	    ox += FULL_TILE_X_OFFSET;
	    oy += FULL_TILE_Y_OFFSET;
	  }
	  ADD_SPRITE(dlp->match[tileno], TRUE, ox, oy);
	  break;
	}
      case MATCH_PAIR:
      case MATCH_FULL:
	assert(0); /* not yet defined */
	break;
      };
      break;
    }
  case CELL_CORNER:
    {
      /* Divide the tile up into four rectangular cells.  Each of these
       * cells covers one corner, and each is adjacent to 3 different
       * tiles.  For each cell we pick a sprite based upon the adjacent
       * terrains at each of those tiles.  Thus, we have 8 different sprites
       * for each of the 4 cells (32 sprites total).
       *
       * These arrays correspond to the direction4 ordering. */
      const int W = t->normal_tile_width;
      const int H = t->normal_tile_height;
      const int iso_offsets[4][2] = {
	{W / 4, 0}, {W / 4, H / 2}, {W / 2, H / 4}, {0, H / 4}
      };
      const int noniso_offsets[4][2] = {
	{0, 0}, {W / 2, H / 2}, {W / 2, 0}, {0, H / 2}
      };

      /* put corner cells */
      for (i = 0; i < NUM_CORNER_DIRS; i++) {
	const int count = dlp->match_indices;
	int array_index = 0;
	enum direction8 dir = dir_ccw(DIR4_TO_DIR8[i]);
	int x = (t->is_isometric ? iso_offsets[i][0] : noniso_offsets[i][0]);
	int y = (t->is_isometric ? iso_offsets[i][1] : noniso_offsets[i][1]);
	int m[3] = {MATCH(dir_ccw(dir)), MATCH(dir), MATCH(dir_cw(dir))};
	struct sprite *s;

	/* synthesize 4 dimensional array? */
	switch (dlp->match_style) {
	case MATCH_NONE:
	  /* We have no need for matching, just plug the piece in place. */
	  break;
	case MATCH_SAME:
	  array_index = array_index * 2 + (m[2] != this);
	  array_index = array_index * 2 + (m[1] != this);
	  array_index = array_index * 2 + (m[0] != this);
	  break;
	case MATCH_PAIR:
	  array_index = array_index * 2 + (m[2] == that);
	  array_index = array_index * 2 + (m[1] == that);
	  array_index = array_index * 2 + (m[0] == that);
	  break;
	case MATCH_FULL:
	default:
	  {
	    int n[3];
	    int j = 0;
	    for (; j < 3; j++) {
	      int k = 0;
	      for (; k < count; k++) {
		n[j] = k; /* default to last entry */
		if (m[j] == dlp->match_index[k])
		{
		  break;
		}
	      }
	    }
	    array_index = array_index * count + n[2];
	    array_index = array_index * count + n[1];
	    array_index = array_index * count + n[0];
	  }
	  break;
	};
	array_index = array_index * NUM_CORNER_DIRS + i;

	s = dlp->cells[array_index];
	if (s) {
	  ADD_SPRITE(s, TRUE, x, y);
	}
      }
      break;
    }
  };
#undef MATCH

  return sprs - saved_sprs;
}

/****************************************************************************
  Helper function for fill_terrain_sprite_layer.
  Fill in the sprite array of darkness.
****************************************************************************/
static int fill_terrain_sprite_darkness(struct tileset *t,
				     struct drawn_sprite *sprs,
				     const struct tile *ptile,
				     struct terrain **tterrain_near)
{
  struct drawn_sprite *saved_sprs = sprs;
  int i, tileno;
  struct tile *adjc_tile;

#define UNKNOWN(dir)                                        \
    ((adjc_tile = mapstep(ptile, (dir)))		    \
     && client_tile_get_known(adjc_tile) == TILE_UNKNOWN)

  switch (t->darkness_style) {
  case DARKNESS_NONE:
    break;
  case DARKNESS_ISORECT:
    for (i = 0; i < 4; i++) {
      const int W = t->normal_tile_width, H = t->normal_tile_height;
      int offsets[4][2] = {{W / 2, 0}, {0, H / 2}, {W / 2, H / 2}, {0, 0}};

      if (UNKNOWN(DIR4_TO_DIR8[i])) {
	ADD_SPRITE(t->sprites.tx.darkness[i], TRUE,
		   offsets[i][0], offsets[i][1]);
      }
    }
    break;
  case DARKNESS_CARD_SINGLE:
    for (i = 0; i < t->num_cardinal_tileset_dirs; i++) {
      if (UNKNOWN(t->cardinal_tileset_dirs[i])) {
	ADD_SPRITE_SIMPLE(t->sprites.tx.darkness[i]);
      }
    }
    break;
  case DARKNESS_CARD_FULL:
    /* We're looking to find the INDEX_NSEW for the directions that
     * are unknown.  We want to mark unknown tiles so that an unreal
     * tile will be given the same marking as our current tile - that
     * way we won't get the "unknown" dither along the edge of the
     * map. */
    tileno = 0;
    for (i = 0; i < t->num_cardinal_tileset_dirs; i++) {
      if (UNKNOWN(t->cardinal_tileset_dirs[i])) {
	tileno |= 1 << i;
      }
    }

    if (tileno != 0) {
      ADD_SPRITE_SIMPLE(t->sprites.tx.darkness[tileno]);
    }
    break;
  case DARKNESS_CORNER:
    /* Handled separately. */
    break;
  };
#undef UNKNOWN

  return sprs - saved_sprs;
}

/****************************************************************************
  Add sprites for the base tile to the sprite list.  This doesn't
  include specials or rivers.
****************************************************************************/
static int fill_terrain_sprite_layer(struct tileset *t,
				     struct drawn_sprite *sprs,
				     int layer_num,
				     const struct tile *ptile,
				     const struct terrain *pterrain,
				     struct terrain **tterrain_near)
{
  struct sprite *sprite;
  struct drawn_sprite *saved_sprs = sprs;
  struct drawing_data *draw = t->sprites.drawing[terrain_index(pterrain)];
  const int l = (draw->is_reversed
		 ? (draw->num_layers - layer_num - 1) : layer_num);

  /* Skip the normal drawing process. */
  /* FIXME: this should avoid calling load_sprite since it's slow and
   * increases the refcount without limit. */
  if (ptile->spec_sprite && (sprite = load_sprite(t, ptile->spec_sprite))) {
    if (l == 0) {
      ADD_SPRITE_SIMPLE(sprite);
      return 1;
    } else {
      return 0;
    }
  }

  if (l < draw->num_layers) {
    sprs += fill_terrain_sprite_array(t, sprs, l, ptile, pterrain, tterrain_near, draw);

    if ((l + 1) == draw->blending) {
      sprs += fill_terrain_sprite_blending(t, sprs, ptile, pterrain, tterrain_near);
    }
  }

  /* Add darkness on top of the first layer.  Note that darkness is always
   * drawn, even in citymode, etc. */
  if (l == 0) {
    sprs += fill_terrain_sprite_darkness(t, sprs, ptile, tterrain_near);
  }

  return sprs - saved_sprs;
}


/****************************************************************************
  Fill in the grid sprites for the given tile, city, and unit.
****************************************************************************/
static int fill_grid_sprite_array(const struct tileset *t,
				  struct drawn_sprite *sprs,
				  const struct tile *ptile,
				  const struct tile_edge *pedge,
				  const struct tile_corner *pcorner,
				  const struct unit *punit,
				  const struct city *pcity,
				  const struct city *citymode)
{
  struct drawn_sprite *saved_sprs = sprs;

  if (pedge) {
    bool known[NUM_EDGE_TILES], city[NUM_EDGE_TILES];
    bool unit[NUM_EDGE_TILES], worked[NUM_EDGE_TILES];
    int i;
    struct unit_list *pfocus_units = get_units_in_focus();

    for (i = 0; i < NUM_EDGE_TILES; i++) {
      int dummy_x, dummy_y;
      const struct tile *tile = pedge->tile[i];
      struct player *powner = tile ? tile_owner(tile) : NULL;

      known[i] = tile && client_tile_get_known(tile) != TILE_UNKNOWN;
      unit[i] = FALSE;
      if (tile) {
	unit_list_iterate(pfocus_units, pfocus_unit) {
	  if (unit_has_type_flag(pfocus_unit, F_CITIES)
	      && city_can_be_built_here(pfocus_unit->tile, pfocus_unit)
	      && city_tile_to_city_map(&dummy_x, &dummy_y,
				      pfocus_unit->tile, tile)) {
	    unit[i] = TRUE;
	    break;
	  }
	} unit_list_iterate_end;
      }
      worked[i] = FALSE;

      city[i] = (tile
		 && (NULL == powner || NULL == client.conn.playing
		     || powner == client.conn.playing)
		 && (NULL == client.conn.playing
		     || player_in_city_radius(client.conn.playing, tile)));
      if (city[i]) {
	if (citymode) {
	  /* In citymode, we only draw worked tiles for this city - other
	   * tiles may be marked as unavailable. */
	  worked[i] = (tile_worked(tile) == citymode);
	} else {
	  worked[i] = (NULL != tile_worked(tile));
	}
      }
    }

    if (mapdeco_is_highlight_set(pedge->tile[0])
        || mapdeco_is_highlight_set(pedge->tile[1])) {
      ADD_SPRITE_SIMPLE(t->sprites.grid.selected[pedge->type]);
    } else if (!draw_terrain && draw_coastline
	       && pedge->tile[0] && pedge->tile[1]
	       && known[0] && known[1]
	       && (is_ocean_tile(pedge->tile[0])
		   ^ is_ocean_tile(pedge->tile[1]))) {
      ADD_SPRITE_SIMPLE(t->sprites.grid.coastline[pedge->type]);
    } else if (draw_map_grid) {
      if (worked[0] || worked[1]) {
	ADD_SPRITE_SIMPLE(t->sprites.grid.worked[pedge->type]);
      } else if (city[0] || city[1]) {
	ADD_SPRITE_SIMPLE(t->sprites.grid.city[pedge->type]);
      } else if (known[0] || known[1]) {
	ADD_SPRITE_SIMPLE(t->sprites.grid.main[pedge->type]);
      }
    } else if (draw_city_outlines) {
      if (XOR(city[0], city[1])) {
	ADD_SPRITE_SIMPLE(t->sprites.grid.city[pedge->type]);
      }
      if (!citymode && XOR(unit[0], unit[1])) {
	ADD_SPRITE_SIMPLE(t->sprites.grid.worked[pedge->type]);
      }
    }

    if (draw_borders && game.info.borders > 0 && known[0] && known[1]) {
      struct player *owner0 = tile_owner(pedge->tile[0]);
      struct player *owner1 = tile_owner(pedge->tile[1]);

      if (owner0 != owner1) {
	if (owner0) {
	  ADD_SPRITE_SIMPLE(t->sprites.grid.player_borders
			    [player_index(owner0)][pedge->type][0]);
	}
	if (owner1) {
	  ADD_SPRITE_SIMPLE(t->sprites.grid.player_borders
			    [player_index(owner1)][pedge->type][1]);
	}
      }
    }
  } else if (NULL != ptile && TILE_UNKNOWN != client_tile_get_known(ptile)) {
    int cx, cy;

    if (citymode
        /* test to ensure valid coordinates? */
     && city_base_to_city_map(&cx, &cy, citymode, ptile)
     && !city_can_work_tile(citymode, ptile)) {
      ADD_SPRITE_SIMPLE(t->sprites.grid.unavailable);
    }
  }

  return sprs - saved_sprs;
}

/****************************************************************************
  Fill in the given sprite array with any needed goto sprites.
****************************************************************************/
static int fill_goto_sprite_array(const struct tileset *t,
				  struct drawn_sprite *sprs,
				  const struct tile *ptile,
				  const struct tile_edge *pedge,
				  const struct tile_corner *pcorner)
{
  struct drawn_sprite *saved_sprs = sprs;

  if (is_valid_goto_destination(ptile)) {
    int length, units, tens;

    goto_get_turns(NULL, &length);
    if (length < 0 || length >= 100) {
      static bool reported = FALSE;

      if (!reported) {
	freelog(LOG_ERROR,
		_("Paths longer than 99 turns are not supported."));
	freelog(LOG_ERROR,
		/* TRANS: No full stop after the URL, could cause confusion. */
		_("Please report this message at %s"),
		BUG_URL);
	reported = TRUE;
      }
      tens = units = 9;
    } else {
      tens = (length / 10) % NUM_TILES_DIGITS;
      units = length % NUM_TILES_DIGITS;
    }

    ADD_SPRITE_SIMPLE(t->sprites.path.turns[units]);
    if (tens > 0) {
      ADD_SPRITE_SIMPLE(t->sprites.path.turns_tens[tens]);
    }
  }

  return sprs - saved_sprs;
}

/****************************************************************************
  Fill in the sprite array for the given tile, city, and unit.

  ptile, if specified, gives the tile.  If specified the terrain and specials
  will be drawn for this tile.  In this case (map_x,map_y) should give the
  location of the tile.

  punit, if specified, gives the unit.  For tile drawing this should
  generally be get_drawable_unit(); otherwise it can be any unit.

  pcity, if specified, gives the city.  For tile drawing this should
  generally be tile_city(ptile); otherwise it can be any city.

  citymode specifies whether this is part of a citydlg.  If so some drawing
  is done differently.
****************************************************************************/
int fill_sprite_array(struct tileset *t,
		      struct drawn_sprite *sprs, enum mapview_layer layer,
		      const struct tile *ptile,
		      const struct tile_edge *pedge,
		      const struct tile_corner *pcorner,
		      const struct unit *punit, const struct city *pcity,
		      const struct city *citymode)
{
  int tileno, dir;
  bv_special tspecial_near[8];
  bv_special tspecial;
  struct terrain *tterrain_near[8];
  struct terrain *pterrain = NULL;
  struct drawn_sprite *save_sprs = sprs;
  struct player *owner = NULL;
  /* Unit drawing is disabled when the view options are turned off,
   * but only where we're drawing on the mapview. */
  bool do_draw_unit = (punit && (draw_units || !ptile
				 || (draw_focus_unit
				     && unit_is_in_focus(punit))));
  bool solid_bg = (solid_color_behind_units
		   && (do_draw_unit
		       || (pcity && draw_cities)
		       || (ptile && !draw_terrain)));

  if (citymode) {
    int count = 0, i, cx, cy;
    const struct tile *const *tiles = NULL;
    bool valid = FALSE;

    if (ptile) {
      tiles = &ptile;
      count = 1;
    } else if (pcorner) {
      tiles = pcorner->tile;
      count = NUM_CORNER_TILES;
    } else if (pedge) {
      tiles = pedge->tile;
      count = NUM_EDGE_TILES;
    }

    for (i = 0; i < count; i++) {
      if (tiles[i] && city_base_to_city_map(&cx, &cy, citymode, tiles[i])) {
	valid = TRUE;
	break;
      }
    }
    if (!valid) {
      return 0;
    }
  }

  if (ptile && client_tile_get_known(ptile) != TILE_UNKNOWN) {
    tspecial = tile_specials(ptile);
    pterrain = tile_terrain(ptile);

    if (NULL != pterrain) {
      build_tile_data(ptile, pterrain, tterrain_near, tspecial_near);
    } else {
      freelog(LOG_ERROR, "fill_sprite_array() tile (%d,%d) has no terrain!",
              TILE_XY(ptile));
    }
  }

  switch (layer) {
  case LAYER_BACKGROUND:
    /* Set up background color. */
    if (solid_color_behind_units) {
      if (do_draw_unit) {
	owner = unit_owner(punit);
      } else if (pcity && draw_cities) {
	owner = city_owner(pcity);
      }
    }
    if (owner) {
      ADD_SPRITE_SIMPLE(t->sprites.backgrounds.player[player_index(owner)]);
    } else if (ptile && !draw_terrain) {
      ADD_SPRITE_SIMPLE(t->sprites.backgrounds.background);
    }
    break;

  case LAYER_TERRAIN1:
    if (NULL != pterrain && draw_terrain && !solid_bg) {
      sprs += fill_terrain_sprite_layer(t, sprs, 0, ptile, pterrain, tterrain_near);
    }
    break;

  case LAYER_TERRAIN2:
    if (NULL != pterrain && draw_terrain && !solid_bg) {
      sprs += fill_terrain_sprite_layer(t, sprs, 1, ptile, pterrain, tterrain_near);
    }
    break;

  case LAYER_TERRAIN3:
    if (NULL != pterrain && draw_terrain && !solid_bg) {
      assert(MAX_NUM_LAYERS == 3);
      sprs += fill_terrain_sprite_layer(t, sprs, 2, ptile, pterrain, tterrain_near);
    }
    break;

  case LAYER_WATER:
    if (NULL != pterrain) {
      if (draw_terrain && !solid_bg
       && terrain_has_flag(pterrain, TER_OCEANIC)) {
	for (dir = 0; dir < 4; dir++) {
	  if (contains_special(tspecial_near[DIR4_TO_DIR8[dir]], S_RIVER)) {
	    ADD_SPRITE_SIMPLE(t->sprites.tx.river_outlet[dir]);
	  }
	}
      }

      sprs += fill_irrigation_sprite_array(t, sprs, tspecial, tspecial_near,
					   pcity);

      if (draw_terrain && !solid_bg && contains_special(tspecial, S_RIVER)) {
	int i;

	/* Draw rivers on top of irrigation. */
	tileno = 0;
	for (i = 0; i < t->num_cardinal_tileset_dirs; i++) {
	  enum direction8 dir = t->cardinal_tileset_dirs[i];

	  if (contains_special(tspecial_near[dir], S_RIVER)
	      || terrain_has_flag(tterrain_near[dir], TER_OCEANIC)) {
	    tileno |= 1 << i;
	  }
	}
	ADD_SPRITE_SIMPLE(t->sprites.tx.spec_river[tileno]);
      }
    }
    break;

  case LAYER_ROADS:
    if (NULL != pterrain) {
      sprs += fill_road_rail_sprite_array(t, sprs,
					  tspecial, tspecial_near, pcity);
    }
    break;

  case LAYER_SPECIAL1:
    if (NULL != pterrain) {
      if (draw_specials) {
	if (tile_resource_is_valid(ptile)) {
	  ADD_SPRITE_SIMPLE(t->sprites.resource[resource_index(tile_resource(ptile))]);
	}
      }

      if (ptile && draw_fortress_airbase) {
        base_type_iterate(pbase) {
          if (tile_has_base(ptile, pbase)
              && t->sprites.bases[base_index(pbase)].background) {
            ADD_SPRITE_FULL(t->sprites.bases[base_index(pbase)].background);
          }
        } base_type_iterate_end;
      }

      if (draw_mines && contains_special(tspecial, S_MINE)) {
        struct drawing_data *draw = t->sprites.drawing[terrain_index(pterrain)];

        if (NULL != draw->mine) {
	  ADD_SPRITE_SIMPLE(draw->mine);
	}
      }

      if (draw_specials && contains_special(tspecial, S_HUT)) {
	ADD_SPRITE_SIMPLE(t->sprites.tx.village);
      }
    }
    break;

  case LAYER_GRID1:
    if (t->is_isometric) {
      sprs += fill_grid_sprite_array(t, sprs, ptile, pedge, pcorner,
				     punit, pcity, citymode);
    }
    break;

  case LAYER_CITY1:
    /* City.  Some city sprites are drawn later. */
    if (pcity && draw_cities) {
      if (!draw_full_citybar && !solid_color_behind_units) {
	ADD_SPRITE(get_city_flag_sprite(t, pcity), TRUE,
		   FULL_TILE_X_OFFSET + t->city_flag_offset_x,
		   FULL_TILE_Y_OFFSET + t->city_flag_offset_y);
      }
      /* For iso-view the city.wall graphics include the full city, whereas
       * for non-iso view they are an overlay on top of the base city
       * graphic. */
      if (!t->is_isometric || !pcity->client.walls) {
	ADD_SPRITE_FULL(get_city_sprite(t->sprites.city.tile, pcity));
      }
      if (t->is_isometric && pcity->client.walls) {
	ADD_SPRITE_FULL(get_city_sprite(t->sprites.city.wall, pcity));
      }
      if (!draw_full_citybar && pcity->client.occupied) {
	ADD_SPRITE_FULL(get_city_sprite(t->sprites.city.occupied, pcity));
      }
      if (!t->is_isometric && pcity->client.walls) {
	ADD_SPRITE_FULL(get_city_sprite(t->sprites.city.wall, pcity));
      }
      if (pcity->client.unhappy) {
	ADD_SPRITE_FULL(t->sprites.city.disorder);
      }
    }
    break;

  case LAYER_SPECIAL2:
    if (NULL != pterrain) {
      if (ptile && draw_fortress_airbase) {
        base_type_iterate(pbase) {
          if (tile_has_base(ptile, pbase)
              && t->sprites.bases[base_index(pbase)].middleground) {
            ADD_SPRITE_FULL(t->sprites.bases[base_index(pbase)].middleground);
          }
        } base_type_iterate_end;
      }

      if (draw_pollution && contains_special(tspecial, S_POLLUTION)) {
	ADD_SPRITE_SIMPLE(t->sprites.tx.pollution);
      }
      if (draw_pollution && contains_special(tspecial, S_FALLOUT)) {
	ADD_SPRITE_SIMPLE(t->sprites.tx.fallout);
      }
    }
    break;

  case LAYER_UNIT:
  case LAYER_FOCUS_UNIT:
    if (do_draw_unit && XOR(layer == LAYER_UNIT, unit_is_in_focus(punit))) {
      bool stacked = ptile && (unit_list_size(ptile->units) > 1);
      bool backdrop = !pcity;

      if (ptile && unit_is_in_focus(punit)
	  && t->sprites.unit.select[0]) {
	/* Special case for drawing the selection rectangle.  The blinking
	 * unit is handled separately, inside get_drawable_unit(). */
	ADD_SPRITE_SIMPLE(t->sprites.unit.select[focus_unit_state]);
      }

      sprs += fill_unit_sprite_array(t, sprs, punit, stacked, backdrop);
    }
    break;

  case LAYER_SPECIAL3:
    if (NULL != pterrain) {
      if (ptile && draw_fortress_airbase) {
        base_type_iterate(pbase) {
          if (tile_has_base(ptile, pbase)
              && t->sprites.bases[base_index(pbase)].foreground) {
            /* Draw fortress front in iso-view (non-iso view only has a fortress
             * back). */
            ADD_SPRITE_FULL(t->sprites.bases[base_index(pbase)].foreground);
          }
        } base_type_iterate_end;
      }
    }
    break;

  case LAYER_FOG:
    sprs += fill_fog_sprite_array(t, sprs, ptile, pedge, pcorner);
    break;

  case LAYER_CITY2:
    /* City size.  Drawing this under fog makes it hard to read. */
    if (pcity && draw_cities && !draw_full_citybar) {
      if (pcity->size >= 10) {
	ADD_SPRITE(t->sprites.city.size_tens[pcity->size / 10],
		   FALSE, FULL_TILE_X_OFFSET, FULL_TILE_Y_OFFSET);
      }
      ADD_SPRITE(t->sprites.city.size[pcity->size % 10],
		 FALSE, FULL_TILE_X_OFFSET, FULL_TILE_Y_OFFSET);
    }
    break;

  case LAYER_GRID2:
    if (!t->is_isometric) {
      sprs += fill_grid_sprite_array(t, sprs, ptile, pedge, pcorner,
				     punit, pcity, citymode);
    }
    break;

  case LAYER_OVERLAYS:
    sprs += fill_city_overlays_sprite_array(t, sprs, ptile, citymode);
    if (mapdeco_is_crosshair_set(ptile)) {
      ADD_SPRITE_SIMPLE(t->sprites.user.attention);
    }
    break;

  case LAYER_CITYBAR:
    /* Nothing.  This is just a placeholder. */
    break;

  case LAYER_GOTO:
    if (ptile && goto_is_active()) {
      sprs += fill_goto_sprite_array(t, sprs, ptile, pedge, pcorner);
    }
    break;

  case LAYER_EDITOR:
    if (ptile && editor_is_active()) {
      const struct nation_type *pnation;

      if (editor_tile_is_selected(ptile)) {
        int color = 2 % tileset_num_city_colors(tileset);
        ADD_SPRITE_SIMPLE(t->sprites.city.unworked_tile_overlay.p[color]);
      }

      pnation = map_get_startpos(ptile);
      if (pnation != NULL) {
        ADD_SPRITE_SIMPLE(t->sprites.user.attention);
      }
    }
    break;

  case LAYER_COUNT:
    assert(0);
    break;
  }

  return sprs - save_sprs;
}

/**********************************************************************
  Set city tiles sprite values; should only happen after
  tilespec_load_tiles().
***********************************************************************/
void tileset_setup_city_tiles(struct tileset *t, int style)
{
  if (style == game.control.styles_count - 1) {

    /* Free old sprites */
    free_city_sprite(t->sprites.city.tile);
    free_city_sprite(t->sprites.city.wall);
    free_city_sprite(t->sprites.city.occupied);

    t->sprites.city.tile = load_city_sprite(t, "city");
    t->sprites.city.wall = load_city_sprite(t, "wall");
    t->sprites.city.occupied = load_city_sprite(t, "occupied");

    for (style = 0; style < game.control.styles_count; style++) {
      if (t->sprites.city.tile->styles[style].land_num_thresholds == 0) {
	freelog(LOG_FATAL, "City style \"%s\": no city graphics.",
		city_style_rule_name(style));
	exit(EXIT_FAILURE);
      }
      if (t->sprites.city.wall->styles[style].land_num_thresholds == 0) {
	freelog(LOG_FATAL, "City style \"%s\": no wall graphics.",
		city_style_rule_name(style));
	exit(EXIT_FAILURE);
      }
      if (t->sprites.city.occupied->styles[style].land_num_thresholds == 0) {
	freelog(LOG_FATAL, "City style \"%s\": no occupied graphics.",
		city_style_rule_name(style));
	exit(EXIT_FAILURE);
      }
    }
  }
}

/****************************************************************************
  Return the amount of time between calls to toggle_focus_unit_state.
  The main loop needs to call toggle_focus_unit_state about this often
  to do the active-unit animation.
****************************************************************************/
double get_focus_unit_toggle_timeout(const struct tileset *t)
{
  if (t->sprites.unit.select[0]) {
    return 0.1;
  } else {
    return 0.5;
  }
}

/****************************************************************************
  Reset the focus unit state.  This should be called when changing
  focus units.
****************************************************************************/
void reset_focus_unit_state(struct tileset *t)
{
  focus_unit_state = 0;
}

void focus_unit_in_combat(struct tileset *t)
{
  if (!t->sprites.unit.select[0]) {
    reset_focus_unit_state(t);
  }
}

/****************************************************************************
  Toggle/increment the focus unit state.  This should be called once
  every get_focus_unit_toggle_timeout() seconds.
****************************************************************************/
void toggle_focus_unit_state(struct tileset *t)
{
  focus_unit_state++;
  if (t->sprites.unit.select[0]) {
    focus_unit_state %= NUM_TILES_SELECT;
  } else {
    focus_unit_state %= 2;
  }
}

/**********************************************************************
...
***********************************************************************/
struct unit *get_drawable_unit(const struct tileset *t,
			       struct tile *ptile,
			       const struct city *citymode)
{
  struct unit *punit = find_visible_unit(ptile);

  if (!punit)
    return NULL;

  if (citymode && unit_owner(punit) == city_owner(citymode))
    return NULL;

  if (!unit_is_in_focus(punit)
      || t->sprites.unit.select[0] || focus_unit_state == 0)
    return punit;
  else
    return NULL;
}

/****************************************************************************
  This patch unloads all sprites from the sprite hash (the hash itself
  is left intact).
****************************************************************************/
static void unload_all_sprites(struct tileset *t)
{
  if (t->sprite_hash) {
    int i, entries = hash_num_entries(t->sprite_hash);

    for (i = 0; i < entries; i++) {
      const char *tag_name = hash_key_by_number(t->sprite_hash, i);
      struct small_sprite *ss = hash_lookup_data(t->sprite_hash, tag_name);

      while (ss->ref_count > 0) {
	unload_sprite(t, tag_name);
      }
    }
  }
}

/**********************************************************************
...
***********************************************************************/
void tileset_free_tiles(struct tileset *t)
{
  int i;

  freelog(LOG_DEBUG, "tileset_free_tiles()");

  unload_all_sprites(t);

  free_city_sprite(t->sprites.city.tile);
  t->sprites.city.tile = NULL;
  free_city_sprite(t->sprites.city.wall);
  t->sprites.city.wall = NULL;
  free_city_sprite(t->sprites.city.occupied);
  t->sprites.city.occupied = NULL;

  if (t->sprite_hash) {
    const int entries = hash_num_entries(t->sprite_hash);

    for (i = 0; i < entries; i++) {
      const char *key = hash_key_by_number(t->sprite_hash, 0);

      hash_delete_entry(t->sprite_hash, key);
      /* now freed by callback */
    }

    hash_free(t->sprite_hash);
    t->sprite_hash = NULL;
  }

  small_sprite_list_iterate(t->small_sprites, ss) {
    small_sprite_list_unlink(t->small_sprites, ss);
    if (ss->file) {
      free(ss->file);
    }
    assert(ss->sprite == NULL);
    free(ss);
  } small_sprite_list_iterate_end;

  specfile_list_iterate(t->specfiles, sf) {
    specfile_list_unlink(t->specfiles, sf);
    free(sf->file_name);
    if (sf->big_sprite) {
      free_sprite(sf->big_sprite);
      sf->big_sprite = NULL;
    }
    free(sf);
  } specfile_list_iterate_end;

  sprite_vector_iterate(&t->sprites.city.worked_tile_overlay, psprite) {
    free_sprite(*psprite);
  } sprite_vector_iterate_end;
  sprite_vector_free(&t->sprites.city.worked_tile_overlay);

  sprite_vector_iterate(&t->sprites.city.unworked_tile_overlay, psprite) {
    free_sprite(*psprite);
  } sprite_vector_iterate_end;
  sprite_vector_free(&t->sprites.city.unworked_tile_overlay);

  for (i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
    if (t->sprites.backgrounds.player[i]) {
      free_sprite(t->sprites.backgrounds.player[i]);
      t->sprites.backgrounds.player[i] = NULL;
    }
  }

  if (t->sprites.tx.fullfog) {
    free(t->sprites.tx.fullfog);
    t->sprites.tx.fullfog = NULL;
  }
  if (t->sprites.backgrounds.background) {
    free_sprite(t->sprites.backgrounds.background);
    t->sprites.backgrounds.background = NULL;
  }

  sprite_vector_free(&t->sprites.colors.overlays);
  sprite_vector_free(&t->sprites.explode.unit);
  sprite_vector_free(&t->sprites.nation_flag);
  sprite_vector_free(&t->sprites.nation_shield);
  sprite_vector_free(&t->sprites.citybar.occupancy);
}

/**************************************************************************
  Return the sprite for drawing the given spaceship part.
**************************************************************************/
struct sprite *get_spaceship_sprite(const struct tileset *t,
				    enum spaceship_part part)
{
  return t->sprites.spaceship[part];
}

/**************************************************************************
  Return a sprite for the given citizen.  The citizen's type is given,
  as well as their index (in the range [0..pcity->size)).  The
  citizen's city can be used to determine which sprite to use (a NULL
  value indicates there is no city; i.e., the sprite is just being
  used as a picture).
**************************************************************************/
struct sprite *get_citizen_sprite(const struct tileset *t,
				  enum citizen_category type,
				  int citizen_index,
				  const struct city *pcity)
{
  const struct citizen_graphic *graphic;

  if (type < CITIZEN_SPECIALIST) {
    assert(type >= 0);
    graphic = &t->sprites.citizen[type];
  } else {
    assert(type < (CITIZEN_SPECIALIST + SP_MAX));
    graphic = &t->sprites.specialist[type - CITIZEN_SPECIALIST];
  }

  return graphic->sprite[citizen_index % graphic->count];
}

/**************************************************************************
  Return the sprite for the nation.
**************************************************************************/
struct sprite *get_nation_flag_sprite(const struct tileset *t,
				      const struct nation_type *pnation)
{
  return t->sprites.nation_flag.p[nation_index(pnation)];
}

/**************************************************************************
  Return the sprite for the technology/advance.
**************************************************************************/
struct sprite *get_tech_sprite(const struct tileset *t, Tech_type_id tech)
{
  if (tech < 0 || tech >= advance_count()) {
    assert(0);
    return NULL;
  }
  return t->sprites.tech[tech];
}

/**************************************************************************
  Return the sprite for the building/improvement.
**************************************************************************/
struct sprite *get_building_sprite(const struct tileset *t,
				   struct impr_type *pimprove)
{
  if (!pimprove) {
    assert(0);
    return NULL;
  }
  return t->sprites.building[improvement_index(pimprove)];
}

/****************************************************************************
  Return the sprite for the government.
****************************************************************************/
struct sprite *get_government_sprite(const struct tileset *t,
				     const struct government *gov)
{
  if (!gov) {
    assert(0);
    return NULL;
  }
  return t->sprites.government[government_index(gov)];
}

/****************************************************************************
  Return the sprite for the unit type (the base "unit" sprite).
****************************************************************************/
struct sprite *get_unittype_sprite(const struct tileset *t,
				   const struct unit_type *punittype)
{
  if (!punittype) {
    assert(0);
    return NULL;
  }
  return t->sprites.unittype[utype_index(punittype)];
}

/**************************************************************************
  Return a "sample" sprite for this city style.
**************************************************************************/
struct sprite *get_sample_city_sprite(const struct tileset *t,
				      int city_style)
{
  int num_thresholds =
    t->sprites.city.tile->styles[city_style].land_num_thresholds;

  if (num_thresholds == 0) {
    return NULL;
  } else {
    return (t->sprites.city.tile->styles[city_style]
	    .land_thresholds[num_thresholds - 1].sprite);
  }
}

/**************************************************************************
  Return a sprite with an "arrow" theme graphic.
**************************************************************************/
struct sprite *get_arrow_sprite(const struct tileset *t,
				enum arrow_type arrow)
{
  assert(arrow >= 0 && arrow < ARROW_LAST);

  return t->sprites.arrow[arrow];
}

/**************************************************************************
  Return a tax sprite for the given output type (usually gold/lux/sci).
**************************************************************************/
struct sprite *get_tax_sprite(const struct tileset *t, Output_type_id otype)
{
  switch (otype) {
  case O_SCIENCE:
    return t->sprites.tax_science;
  case O_GOLD:
    return t->sprites.tax_gold;
  case O_LUXURY:
    return t->sprites.tax_luxury;
  case O_TRADE:
  case O_FOOD:
  case O_SHIELD:
  case O_LAST:
    break;
  }
  return NULL;
}

/**************************************************************************
  Return a thumbs-up/thumbs-down sprite to show treaty approval or
  disapproval.
**************************************************************************/
struct sprite *get_treaty_thumb_sprite(const struct tileset *t, bool on_off)
{
  return t->sprites.treaty_thumb[on_off ? 1 : 0];
}

/**************************************************************************
  Return a sprite_vector containing the animation sprites for a unit
  explosion.
**************************************************************************/
const struct sprite_vector *get_unit_explode_animation(const struct
						       tileset *t)
{
  return &t->sprites.explode.unit;
}

/****************************************************************************
  Return a sprite contining the single nuke graphic.

  TODO: This should be an animation like the unit explode animation.
****************************************************************************/
struct sprite *get_nuke_explode_sprite(const struct tileset *t)
{
  return t->sprites.explode.nuke;
}

/**************************************************************************
  Return all the sprites used for citybar drawing.
**************************************************************************/
const struct citybar_sprites *get_citybar_sprites(const struct tileset *t)
{
  if (draw_full_citybar) {
    return &t->sprites.citybar;
  } else {
    return NULL;
  }
}

/**************************************************************************
  Return all the sprites used for editor icons, images, etc.
**************************************************************************/
const struct editor_sprites *get_editor_sprites(const struct tileset *t)
{
  return &t->sprites.editor;
}

/**************************************************************************
  Returns a sprite for the given cursor.  The "hot" coordinates (the
  active coordinates of the mouse relative to the sprite) are placed int
  (*hot_x, *hot_y). 
  A cursor can consist of several frames to be used for animation.
**************************************************************************/
struct sprite *get_cursor_sprite(const struct tileset *t,
				 enum cursor_type cursor,
				 int *hot_x, int *hot_y, int frame)
{
  *hot_x = t->sprites.cursor[cursor].hot_x;
  *hot_y = t->sprites.cursor[cursor].hot_y;
  return t->sprites.cursor[cursor].frame[frame];
}

/****************************************************************************
  Return a sprite for the given icon.  Icons are used by the operating
  system/window manager.  Usually freeciv has to tell the OS what icon to
  use.

  Note that this function will return NULL before the sprites are loaded.
  The GUI code must be sure to call tileset_load_tiles before setting the
  top-level icon.
****************************************************************************/
struct sprite *get_icon_sprite(const struct tileset *t, enum icon_type icon)
{
  return t->sprites.icon[icon];
}

/****************************************************************************
  Returns a sprite with the "user-attention" crosshair graphic.

  FIXME: This function shouldn't be needed if the attention graphics are
  drawn natively by the tileset code.
****************************************************************************/
struct sprite *get_attention_crosshair_sprite(const struct tileset *t)
{
  return t->sprites.user.attention;
}

/****************************************************************************
  Returns a sprite for the given indicator with the given index.  The
  index should be in [0, NUM_TILES_PROGRESS).
****************************************************************************/
struct sprite *get_indicator_sprite(const struct tileset *t,
				    enum indicator_type indicator,
				    int index)
{
  index = CLIP(0, index, NUM_TILES_PROGRESS - 1);
  assert(indicator >= 0 && indicator < INDICATOR_COUNT);
  return t->sprites.indicator[indicator][index];
}

/****************************************************************************
  Return a sprite for the unhappiness of the unit - to be shown as an
  overlay on the unit in the city support dialog, for instance.

  May return NULL if there's no unhappiness.
****************************************************************************/
struct sprite *get_unit_unhappy_sprite(const struct tileset *t,
				       const struct unit *punit,
				       int happy_cost)
{
  const int unhappy = CLIP(0, happy_cost, 2);

  if (unhappy > 0) {
    return t->sprites.upkeep.unhappy[unhappy - 1];
  } else {
    return NULL;
  }
}

/****************************************************************************
  Return a sprite for the upkeep of the unit - to be shown as an overlay
  on the unit in the city support dialog, for instance.

  May return NULL if there's no unhappiness.
****************************************************************************/
struct sprite *get_unit_upkeep_sprite(const struct tileset *t,
				      Output_type_id otype,
				      const struct unit *punit,
				      const int *upkeep_cost)
{
  const int upkeep = CLIP(0, upkeep_cost[otype], 2);

  if (upkeep > 0) {
    return t->sprites.upkeep.output[otype][upkeep - 1];
  } else {
    return NULL;
  }
}

/****************************************************************************
  Return a rectangular sprite containing a fog "color".  This can be used
  for drawing fog onto arbitrary areas (like the overview).
****************************************************************************/
struct sprite *get_basic_fog_sprite(const struct tileset *t)
{
  return t->sprites.tx.fog;
}

/****************************************************************************
  Return the tileset's color system.
****************************************************************************/
struct color_system *get_color_system(const struct tileset *t)
{
  return t->color_system;
}

/****************************************************************************
  Loads prefered theme if there's any.
****************************************************************************/
void tileset_use_prefered_theme(const struct tileset *t)
{
  int i;
    
  for (i = 0; i < t->num_prefered_themes; i++) {
    if (strcmp(t->prefered_themes[i], default_theme_name)) {
      if (popup_theme_suggestion_dialog(t->prefered_themes[i])) {
        freelog(LOG_DEBUG, "trying theme \"%s\".", t->prefered_themes[i]);
        if (load_theme(t->prefered_themes[i])) {
          sz_strlcpy(default_theme_name, t->prefered_themes[i]);
          return;
        }
      }
    }
  }
  freelog(LOG_VERBOSE, "The tileset doesn't specify prefered themes or "
                       "none of prefered themes can be used. Using system "
		       "default");
  gui_clear_theme();
}

/****************************************************************************
  Initialize tileset structure
****************************************************************************/
void tileset_init(struct tileset *t)
{
  /* We currently have no city sprites loaded. */
  t->sprites.city.tile     = NULL;
  t->sprites.city.wall     = NULL;
  t->sprites.city.occupied = NULL;
}

/****************************************************************************
  Fill the sprite array with sprites that together make a representative
  image of the given terrain type. Suitable for use as an icon and in list
  views.

  NB: The 'layer' argument is NOT a LAYER_* value, but rather one of 0, 1, 2.
  Using other values for 'layer' here will result in undefined behaviour. ;)
****************************************************************************/
int fill_basic_terrain_layer_sprite_array(struct tileset *t,
                                          struct drawn_sprite *sprs,
                                          int layer,
                                          struct terrain *pterrain)
{
  struct drawn_sprite *save_sprs = sprs;
  struct drawing_data *draw = t->sprites.drawing[terrain_index(pterrain)];

  struct terrain *tterrain_near[8];
  bv_special tspecial_near[8];

  struct tile dummy_tile; /* :( */

  int i;


  memset(&dummy_tile, 0, sizeof(struct tile));
  
  for (i = 0; i < 8; i++) {
    tterrain_near[i] = pterrain;
    BV_CLR_ALL(tspecial_near[i]);
  }

  i = draw->is_reversed ? draw->num_layers - layer - 1 : layer;
  sprs += fill_terrain_sprite_array(t, sprs, i, &dummy_tile,
                                    pterrain, tterrain_near, draw);

  return sprs - save_sprs;
}

/****************************************************************************
  Return the sprite for the given resource type.
****************************************************************************/
struct sprite *get_resource_sprite(const struct tileset *t,
                                   const struct resource *presource)
{
  if (presource == NULL) {
    return NULL;
  }

  return t->sprites.resource[resource_index(presource)];
}

/****************************************************************************
  Return a representative sprite for the mine special type (S_MINE).
****************************************************************************/
struct sprite *get_basic_mine_sprite(const struct tileset *t)
{
  struct drawing_data *draw;
  struct terrain *tm;

  tm = find_terrain_by_rule_name("mountains");
  if (tm != NULL) {
    draw = t->sprites.drawing[terrain_index(tm)];
    if (draw->mine != NULL) {
      return draw->mine;
    }
  }

  terrain_type_iterate(pterrain) {
    draw = t->sprites.drawing[terrain_index(pterrain)];
    if (draw->mine != NULL) {
      return draw->mine;
    }
  } terrain_type_iterate_end;

  return NULL;
}

/****************************************************************************
  Return a representative sprite for the given special type.

  NB: This does not include generic base specials (S_FORTRESS and
  S_AIRBASE). Use fill_basic_base_sprite_array and the base type for that.
****************************************************************************/
struct sprite *get_basic_special_sprite(const struct tileset *t,
                                        enum tile_special_type special)
{
  int i;

  switch (special) {
  case S_ROAD:
    for (i = 0; i < t->num_valid_tileset_dirs; i++) {
      if (!t->valid_tileset_dirs[i]) {
        continue;
      }
      if (t->roadstyle == 0) {
        return t->sprites.road.dir[i];
      } else if (t->roadstyle == 1) {
        return t->sprites.road.even[1 << i];
      } else if (t->roadstyle == 2) {
        return t->sprites.road.total[1 << i];
      }
    }
    return NULL;
    break;
  case S_IRRIGATION:
    return t->sprites.tx.irrigation[0];
    break;
  case S_RAILROAD:
    for (i = 0; i < t->num_valid_tileset_dirs; i++) {
      if (!t->valid_tileset_dirs[i]) {
        continue;
      }
      if (t->roadstyle == 0) {
        return t->sprites.rail.dir[i];
      } else if (t->roadstyle == 1) {
        return t->sprites.rail.even[1 << i];
      } else if (t->roadstyle == 2) {
        return t->sprites.rail.total[1 << i];
      }
    }
    return NULL;
    break;
  case S_MINE:
    return get_basic_mine_sprite(t);
    break;
  case S_POLLUTION:
    return t->sprites.tx.pollution;
    break;
  case S_HUT:
    return t->sprites.tx.village;
    break;
  case S_RIVER:
    return t->sprites.tx.spec_river[0];
    break;
  case S_FARMLAND:
    return t->sprites.tx.farmland[0];
    break;
  case S_FALLOUT:
    return t->sprites.tx.fallout;
    break;
  default:
    break;
  }

  return NULL;
}

/****************************************************************************
  Fills the sprite array with sprites that together make a representative
  image of the given base type. The image is suitable for use as an icon
  for the base type, for example.
****************************************************************************/
int fill_basic_base_sprite_array(const struct tileset *t,
                                 struct drawn_sprite *sprs,
                                 const struct base_type *pbase)
{
  struct drawn_sprite *saved_sprs = sprs;
  int index;

  if (!t || !sprs || !pbase) {
    return 0;
  }

  index = base_index(pbase);

  if (!(0 <= index && index < game.control.num_base_types)) {
    return 0;
  }

#define ADD_SPRITE_IF_NOT_NULL(x) do {\
  if ((x) != NULL) {\
    ADD_SPRITE_FULL(x);\
  }\
} while (0)

  /* Corresponds to LAYER_SPECIAL{1,2,3} order. */
  ADD_SPRITE_IF_NOT_NULL(t->sprites.bases[index].background);
  ADD_SPRITE_IF_NOT_NULL(t->sprites.bases[index].middleground);
  ADD_SPRITE_IF_NOT_NULL(t->sprites.bases[index].foreground);

#undef ADD_SPRITE_IF_NOT_NULL

  return sprs - saved_sprs;
}
