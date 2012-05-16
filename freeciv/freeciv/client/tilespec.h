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
  Reading and using the tilespec files, which describe
  the files and contents of tilesets.
***********************************************************************/
#ifndef FC__TILESPEC_H
#define FC__TILESPEC_H

#include "fc_types.h"

#include "city.h"		/* enum citizen_category */
#include "options.h"

struct sprite;			/* opaque; gui-dep */

struct base_type;
struct resource;

/* Create the sprite_vector type. */
#define SPECVEC_TAG sprite
#define SPECVEC_TYPE struct sprite *
#include "specvec.h"
#define sprite_vector_iterate(sprite_vec, psprite) \
  TYPED_VECTOR_ITERATE(struct sprite *, sprite_vec, psprite)
#define sprite_vector_iterate_end VECTOR_ITERATE_END

/* An edge is the border between two tiles.  This structure represents one
 * edge.  The tiles are given in the same order as the enumeration name. */
enum edge_type {
  EDGE_NS, /* North and south */
  EDGE_WE, /* West and east */
  EDGE_UD, /* Up and down (nw/se), for hex_width tilesets */
  EDGE_LR, /* Left and right (ne/sw), for hex_height tilesets */
  EDGE_COUNT
};
struct tile_edge {
  enum edge_type type;
#define NUM_EDGE_TILES 2
  const struct tile *tile[NUM_EDGE_TILES];
};

/* A corner is the endpoint of several edges.  At each corner 4 tiles will
 * meet (3 in hex view).  Tiles are in clockwise order NESW. */
struct tile_corner {
#define NUM_CORNER_TILES 4
  const struct tile *tile[NUM_CORNER_TILES];
};

struct drawn_sprite {
  bool foggable;	/* Set to FALSE for sprites that are never fogged. */
  struct sprite *sprite;
  int offset_x, offset_y;	/* offset from tile origin */
};

/* Items on the mapview are drawn in layers.  Each entry below represents
 * one layer.  The names are basically arbitrary and just correspond to
 * groups of elements in fill_sprite_array().  Callers of fill_sprite_array
 * must call it once for each layer. */
enum mapview_layer {
  LAYER_BACKGROUND,
  LAYER_TERRAIN1,
  LAYER_TERRAIN2,
  LAYER_TERRAIN3,
  LAYER_WATER,
  LAYER_ROADS,
  LAYER_SPECIAL1,
  LAYER_GRID1,
  LAYER_CITY1,
  LAYER_SPECIAL2,
  LAYER_UNIT,
  LAYER_SPECIAL3,
  LAYER_FOG,
  LAYER_CITY2,
  LAYER_GRID2,
  LAYER_OVERLAYS,
  LAYER_CITYBAR,
  LAYER_FOCUS_UNIT,
  LAYER_GOTO,
  LAYER_EDITOR,
  LAYER_COUNT
};

#define mapview_layer_iterate(layer)			                    \
{									    \
  enum mapview_layer layer;						    \
									    \
  for (layer = 0; layer < LAYER_COUNT; layer++) {			    \

#define mapview_layer_iterate_end		                            \
  }									    \
}

#define NUM_TILES_PROGRESS 8

enum arrow_type {
  ARROW_RIGHT,
  ARROW_PLUS,
  ARROW_MINUS,
  ARROW_LAST
};

struct tileset;

extern struct tileset *tileset;

const char **get_tileset_list(void);

struct tileset *tileset_read_toplevel(const char *tileset_name, bool verbose);
void tileset_init(struct tileset *t);
void tileset_free(struct tileset *tileset);
void tileset_load_tiles(struct tileset *t);
void tileset_free_tiles(struct tileset *t);

void tilespec_try_read(const char *tileset_name, bool verbose);
void tilespec_reread(const char *tileset_name);
void tilespec_reread_callback(struct client_option *option);

void tileset_setup_specialist_type(struct tileset *t, Specialist_type_id id);
void tileset_setup_unit_type(struct tileset *t, struct unit_type *punittype);
void tileset_setup_impr_type(struct tileset *t,
			     struct impr_type *pimprove);
void tileset_setup_tech_type(struct tileset *t,
			     struct advance *padvance);
void tileset_setup_tile_type(struct tileset *t,
			     const struct terrain *pterrain);
void tileset_setup_resource(struct tileset *t,
			    const struct resource *presource);
void tileset_setup_base(struct tileset *t,
                        const struct base_type *pbase);
void tileset_setup_government(struct tileset *t,
			      struct government *gov);
void tileset_setup_nation_flag(struct tileset *t, 
			       struct nation_type *nation);
void tileset_setup_city_tiles(struct tileset *t, int style);

/* Gfx support */

int fill_sprite_array(struct tileset *t,
		      struct drawn_sprite *sprs, enum mapview_layer layer,
		      const struct tile *ptile,
		      const struct tile_edge *pedge,
		      const struct tile_corner *pcorner,
		      const struct unit *punit, const struct city *pcity,
		      const struct city *citymode);
int fill_basic_terrain_layer_sprite_array(struct tileset *t,
                                          struct drawn_sprite *sprs,
                                          int layer,
                                          struct terrain *pterrain);
int fill_basic_base_sprite_array(const struct tileset *t,
                                 struct drawn_sprite *sprs,
                                 const struct base_type *pbase);

double get_focus_unit_toggle_timeout(const struct tileset *t);
void reset_focus_unit_state(struct tileset *t);
void focus_unit_in_combat(struct tileset *t);
void toggle_focus_unit_state(struct tileset *t);
struct unit *get_drawable_unit(const struct tileset *t,
			       struct tile *ptile,
			       const struct city *citymode);


enum cursor_type {
  CURSOR_GOTO,
  CURSOR_PATROL,
  CURSOR_PARADROP,
  CURSOR_NUKE,
  CURSOR_SELECT,
  CURSOR_INVALID,
  CURSOR_ATTACK,
  CURSOR_EDIT_PAINT,
  CURSOR_EDIT_ADD,
  CURSOR_WAIT,
  CURSOR_LAST,
  CURSOR_DEFAULT,
};

#define NUM_CURSOR_FRAMES 6

enum indicator_type {
  INDICATOR_BULB,
  INDICATOR_WARMING,
  INDICATOR_COOLING,
  INDICATOR_COUNT
};

enum icon_type {
  ICON_FREECIV,
  ICON_CITYDLG,
  ICON_COUNT
};

enum spaceship_part {
  SPACESHIP_SOLAR_PANEL,
  SPACESHIP_LIFE_SUPPORT,
  SPACESHIP_HABITATION,
  SPACESHIP_STRUCTURAL,
  SPACESHIP_FUEL,
  SPACESHIP_PROPULSION,
  SPACESHIP_EXHAUST,
  SPACESHIP_COUNT
};

struct citybar_sprites {
  struct sprite
    *shields,
    *food,
    *trade,
    *occupied,
    *background;
  struct sprite_vector occupancy;
};

struct editor_sprites {
  struct sprite
    *erase,
    *brush,
    *copy,
    *paste,
    *copypaste,
    *startpos,
    *terrain,
    *terrain_resource,
    *terrain_special,
    *unit,
    *city,
    *vision,
    *territory,
    *properties,
    *military_base;
};

struct sprite *get_spaceship_sprite(const struct tileset *t,
				    enum spaceship_part part);
struct sprite *get_citizen_sprite(const struct tileset *t,
				  enum citizen_category type,
				  int citizen_index,
				  const struct city *pcity);
struct sprite *get_city_flag_sprite(const struct tileset *t,
				    const struct city *pcity);
struct sprite *get_nation_flag_sprite(const struct tileset *t,
				      const struct nation_type *nation);
struct sprite *get_tech_sprite(const struct tileset *t, Tech_type_id tech);
struct sprite *get_building_sprite(const struct tileset *t,
				   struct impr_type *pimprove);
struct sprite *get_government_sprite(const struct tileset *t,
				     const struct government *gov);
struct sprite *get_unittype_sprite(const struct tileset *t,
				   const struct unit_type *punittype);
struct sprite *get_sample_city_sprite(const struct tileset *t,
				      int city_style);
struct sprite *get_arrow_sprite(const struct tileset *t,
				enum arrow_type arrow);
struct sprite *get_tax_sprite(const struct tileset *t, Output_type_id otype);
struct sprite *get_treaty_thumb_sprite(const struct tileset *t, bool on_off);
const struct sprite_vector *get_unit_explode_animation(const struct
						       tileset *t);
struct sprite *get_nuke_explode_sprite(const struct tileset *t);
struct sprite *get_cursor_sprite(const struct tileset *t,
				 enum cursor_type cursor,
				 int *hot_x, int *hot_y, int frame);
const struct citybar_sprites *get_citybar_sprites(const struct tileset *t);
const struct editor_sprites *get_editor_sprites(const struct tileset *t);
struct sprite *get_icon_sprite(const struct tileset *t, enum icon_type icon);
struct sprite *get_attention_crosshair_sprite(const struct tileset *t);
struct sprite *get_indicator_sprite(const struct tileset *t,
				    enum indicator_type indicator,
				    int index);
struct sprite *get_unit_unhappy_sprite(const struct tileset *t,
				       const struct unit *punit,
				       int happy_cost);
struct sprite *get_unit_upkeep_sprite(const struct tileset *t,
				      Output_type_id otype,
				      const struct unit *punit,
				      const int *upkeep_cost);
struct sprite *get_basic_fog_sprite(const struct tileset *t);
struct sprite *get_resource_sprite(const struct tileset *t,
                                   const struct resource *presouce);
struct sprite *get_basic_special_sprite(const struct tileset *t,
                                        enum tile_special_type special);
struct sprite *get_basic_mine_sprite(const struct tileset *t);

struct sprite* tiles_lookup_sprite_tag_alt(struct tileset *t, int loglevel,
					   const char *tag, const char *alt,
					   const char *what, const char *name);

struct color_system;
struct color_system *get_color_system(const struct tileset *t);

/* Tileset accessor functions. */
const char *tileset_get_name(const struct tileset *t);
bool tileset_is_isometric(const struct tileset *t);
int tileset_hex_width(const struct tileset *t);
int tileset_hex_height(const struct tileset *t);
int tileset_tile_width(const struct tileset *t);
int tileset_tile_height(const struct tileset *t);
int tileset_full_tile_width(const struct tileset *t);
int tileset_full_tile_height(const struct tileset *t);
int tileset_unit_width(const struct tileset *t);
int tileset_unit_height(const struct tileset *t);
int tileset_small_sprite_width(const struct tileset *t);
int tileset_small_sprite_height(const struct tileset *t);
int tileset_citybar_offset_y(const struct tileset *t);
const char *tileset_main_intro_filename(const struct tileset *t);
const char *tileset_mini_intro_filename(const struct tileset *t);
int tileset_num_city_colors(const struct tileset *t);
void tileset_use_prefered_theme(const struct tileset *t);

#endif  /* FC__TILESPEC_H */
