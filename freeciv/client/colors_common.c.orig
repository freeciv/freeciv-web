/********************************************************************** 
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
#include <config.h>
#endif

#include <assert.h>

#include "hash.h"
#include "log.h"
#include "shared.h"

#include "player.h"

#include "colors_g.h"

#include "colors_common.h"
#include "tilespec.h"

/* An RGBcolor contains the R,G,B bitvalues for a color.  The color itself
 * holds the color structure for this color but may be NULL (it's allocated
 * on demand at runtime). */
struct rgbcolor {
  int r, g, b;
  struct color *color;
};

struct color_system {
  struct rgbcolor colors[COLOR_LAST];

  int num_player_colors;
  struct rgbcolor *player_colors;

  /* Terrain colors: we have one color per terrain.  These are stored in a
   * larger-than-necessary array.  There's also a hash that is used to store
   * all colors; this is created when the tileset toplevel is read and later
   * used when the rulesets are received. */
  struct hash_table *terrain_hash;
  struct rgbcolor terrain_colors[MAX_NUM_TERRAINS];
};

char *color_names[] = {
  /* Mapview */
  "mapview_unknown",
  "mapview_citytext",
  "mapview_cityblocked",
  "mapview_goto",
  "mapview_selection",
  "mapview_traderoute_line",
  "mapview_traderoutes_all_built",
  "mapview_traderoutes_some_built",
  "mapview_traderoutes_no_built",
  "mapview_city_link",
  "mapview_tile_link",
  "mapview_unit_link",

  /* Spaceship */
  "spaceship_background",

  /* Overview */
  "overview_unknown",
  "overview_mycity",
  "overview_alliedcity",
  "overview_enemycity",
  "overview_myunit",
  "overview_alliedunit",
  "overview_enemyunit",
  "overview_ocean",
  "overview_ground",
  "overview_viewrect",

  /* Reqtree */
  "reqtree_researching",
  "reqtree_known",
  "reqtree_goal_prereqs_known",
  "reqtree_goal_unknown",
  "reqtree_prereqs_known",
  "reqtree_unknown",
  "reqtree_unreachable",
  "reqtree_background",
  "reqtree_text",
  "reqtree_edge",

  /* Player dialog */
  "playerdlg_background"
};

/****************************************************************************
  Called when the client first starts to allocate the default colors.

  Currently this must be called in ui_main, generally after UI
  initialization.
****************************************************************************/
struct color_system *color_system_read(struct section_file *file)
{
  int i;
  struct color_system *colors = fc_malloc(sizeof(*colors));

  assert(ARRAY_SIZE(color_names) == COLOR_LAST);
  for (i = 0; i < COLOR_LAST; i++) {
    colors->colors[i].r
      = secfile_lookup_int(file, "colors.%s0.r", color_names[i]);
    colors->colors[i].g
      = secfile_lookup_int(file, "colors.%s0.g", color_names[i]);
    colors->colors[i].b
      = secfile_lookup_int(file, "colors.%s0.b", color_names[i]);
    colors->colors[i].color = NULL;
  }

  for (i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
    if (!section_file_lookup(file, "colors.player%d.r", i)) {
      break;
    }
  }
  colors->num_player_colors = MAX(i, 1);
  colors->player_colors = fc_malloc(colors->num_player_colors
				    * sizeof(*colors->player_colors));
  if (i == 0) {
    /* Use a simple fallback. */
    freelog(LOG_ERROR,
	    "Missing colors.player.  See misc/colors.tilespec.");
    colors->player_colors[0].r = 128;
    colors->player_colors[0].g = 0;
    colors->player_colors[0].b = 0;
    colors->player_colors[0].color = NULL;
  } else {
    for (i = 0; i < colors->num_player_colors; i++) {
      struct rgbcolor *rgb = &colors->player_colors[i];

      rgb->r = secfile_lookup_int(file, "colors.player%d.r", i);
      rgb->g = secfile_lookup_int(file, "colors.player%d.g", i);
      rgb->b = secfile_lookup_int(file, "colors.player%d.b", i);
      rgb->color = NULL;
    }
  }

  for (i = 0; i < ARRAY_SIZE(colors->terrain_colors); i++) {
    struct rgbcolor *rgb = &colors->terrain_colors[i];

    rgb->r = rgb->g = rgb->b = 0;
    rgb->color = NULL;
  }
  colors->terrain_hash = hash_new(hash_fval_string, hash_fcmp_string);
  for (i = 0; ; i++) {
    struct rgbcolor *rgb;
    char *key;

    if (!section_file_lookup(file, "colors.tiles%d.r", i)) {
      break;
    }
    rgb = fc_malloc(sizeof(*rgb));
    rgb->r = secfile_lookup_int(file, "colors.tiles%d.r", i);
    rgb->g = secfile_lookup_int(file, "colors.tiles%d.g", i);
    rgb->b = secfile_lookup_int(file, "colors.tiles%d.b", i);
    rgb->color = NULL;
    key = secfile_lookup_str(file, "colors.tiles%d.tag", i);

    if (!hash_insert(colors->terrain_hash, mystrdup(key), rgb)) {
      freelog(LOG_ERROR, "warning: already have a color for %s", key);
    }
  }

  return colors;
}

/****************************************************************************
  Called when terrain info is received from the server.
****************************************************************************/
void color_system_setup_terrain(struct color_system *colors,
				const struct terrain *pterrain,
				const char *tag)
{
  struct rgbcolor *rgb
    = hash_lookup_data(colors->terrain_hash, tag);

  if (rgb) {
    colors->terrain_colors[terrain_index(pterrain)] = *rgb;
  } else {
    freelog(LOG_ERROR, "[colors] missing [tile_%s] for \"%s\".",
            tag, terrain_rule_name(pterrain));
    /* Fallback: the color remains black. */
  }
}

/****************************************************************************
  Called when the client first starts to free any allocated colors.
****************************************************************************/
void color_system_free(struct color_system *colors)
{
  int i;

  for (i = 0; i < COLOR_LAST; i++) {
    if (colors->colors[i].color) {
      color_free(colors->colors[i].color);
    }
  }
  for (i = 0; i < colors->num_player_colors; i++) {
    if (colors->player_colors[i].color) {
      color_free(colors->player_colors[i].color);
    }
  }
  free(colors->player_colors);
  for (i = 0; i < ARRAY_SIZE(colors->terrain_colors); i++) {
    if (colors->terrain_colors[i].color) {
      color_free(colors->terrain_colors[i].color);
    }
  }
  while (hash_num_entries(colors->terrain_hash) > 0) {
    const char *key = hash_key_by_number(colors->terrain_hash, 0);
    const void *rgb = hash_value_by_number(colors->terrain_hash, 0);

    hash_delete_entry(colors->terrain_hash, key);
    free((void *)key);
    free((void *)rgb);
  }
  hash_free(colors->terrain_hash);
  free(colors);
}

/****************************************************************************
  Return the RGB color, allocating it if necessary.
****************************************************************************/
static struct color *ensure_color(struct rgbcolor *rgb)
{
  if (!rgb->color) {
    rgb->color = color_alloc(rgb->r, rgb->g, rgb->b);
  }
  return rgb->color;
}

/****************************************************************************
  Return a pointer to the given "standard" color.
****************************************************************************/
struct color *get_color(const struct tileset *t, enum color_std color)
{
  return ensure_color(&get_color_system(t)->colors[color]);
}

/**********************************************************************
  Not sure which module to put this in...
  It used to be that each nation had a color, when there was
  fixed number of nations.  Now base on player number instead,
  since still limited to less than 14.  Could possibly improve
  to allow players to choose their preferred color etc.
  A hack added to avoid returning more that COLOR_STD_RACE13.
  But really there should be more colors available -- jk.
***********************************************************************/
struct color *get_player_color(const struct tileset *t,
			       const struct player *pplayer)
{
  if (pplayer) {
    struct color_system *colors = get_color_system(t);
    int index = player_index(pplayer);

    assert(index >= 0 && colors->num_player_colors > 0);
    index %= colors->num_player_colors;
    return ensure_color(&colors->player_colors[index]);
  } else {
    assert(0);
    return NULL;
  }
}

/****************************************************************************
  Return a pointer to the given "terrain" color.

  Each terrain has a color associated.  This is usually used to draw the
  overview.
****************************************************************************/
struct color *get_terrain_color(const struct tileset *t,
				const struct terrain *pterrain)
{
  if (pterrain) {
    struct color_system *colors = get_color_system(t);

    return ensure_color(&colors->terrain_colors[terrain_index(pterrain)]);
  } else {
    assert(0);
    return NULL;
  }
}
