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
#ifndef FC__CANVAS_G_H
#define FC__CANVAS_G_H

#include "shared.h"		/* bool type */

struct color;
struct sprite;

struct canvas;			/* opaque type, real type is gui-dep */

enum line_type {
  LINE_NORMAL, LINE_BORDER, LINE_TILE_FRAME, LINE_GOTO
};

/* Creator and destructor */
struct canvas *canvas_create(int width, int height);
void canvas_free(struct canvas *store);

/* Drawing functions */
void canvas_copy(struct canvas *dest, struct canvas *src,
		 int src_x, int src_y, int dest_x, int dest_y,
		 int width, int height);
void canvas_put_sprite(struct canvas *pcanvas,
		       int canvas_x, int canvas_y, struct sprite *sprite,
		       int offset_x, int offset_y, int width, int height);
void canvas_put_sprite_full(struct canvas *pcanvas, 
			    int canvas_x, int canvas_y,
			    struct sprite *sprite);
void canvas_put_sprite_fogged(struct canvas *pcanvas,
			      int canvas_x, int canvas_y,
			      struct sprite *psprite,
			      bool fog, int fog_x, int fog_y);
void canvas_put_rectangle(struct canvas *pcanvas,
			  struct color *pcolor,
			  int canvas_x, int canvas_y, int width, int height);
void canvas_fill_sprite_area(struct canvas *pcanvas,
			     struct sprite *psprite,
			     struct color *pcolor,
			     int canvas_x, int canvas_y);
void canvas_fog_sprite_area(struct canvas *pcanvas, struct sprite *psprite,
			    int canvas_x, int canvas_y);
void canvas_put_line(struct canvas *pcanvas,
		     struct color *pcolor,
		     enum line_type ltype, int start_x, int start_y,
		     int dx, int dy);
void canvas_put_curved_line(struct canvas *pcanvas,
                            struct color *pcolor,
                            enum line_type ltype, int start_x, int start_y,
                            int dx, int dy);

/* Text drawing functions */
enum client_font {
  FONT_CITY_NAME,
  FONT_CITY_PROD,
  FONT_REQTREE_TEXT,
  FONT_COUNT
};
void get_text_size(int *width, int *height,
		   enum client_font font, const char *text);
void canvas_put_text(struct canvas *pcanvas, int canvas_x, int canvas_y,
		     enum client_font font,
		     struct color *pcolor,
		     const char *text);

#endif  /* FC__CANVAS_G_H */
