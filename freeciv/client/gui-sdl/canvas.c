/********************************************************************** 
 Freeciv - Copyright (C) 2005 The Freeciv Team
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

#include "SDL.h"

/* utility */
#include "log.h"

/* gui-sdl */
#include "SDL_ttf.h"
#include "colors.h"
#include "graphics.h"
#include "gui_main.h"
#include "gui_string.h"
#include "sprite.h"

#include "canvas.h"

static int *fonts[FONT_COUNT] = {
  &city_names_font_size,
  &city_productions_font_size,
  &city_productions_font_size
};

/**************************************************************************
  Create a canvas of the given size.
**************************************************************************/
struct canvas *canvas_create(int width, int height)
{
  struct canvas *result = fc_malloc(sizeof(*result));	

  result->surf = create_surf(width, height, SDL_SWSURFACE);

  return result;
}

/**************************************************************************
  Create a canvas of the given size (with alpha channel).
**************************************************************************/
struct canvas *canvas_create_with_alpha(int width, int height)
{
  struct canvas *result = fc_malloc(sizeof(*result));	

  result->surf = create_surf_alpha(width, height, SDL_SWSURFACE);

  return result;
}

/**************************************************************************
  Free any resources associated with this canvas and the canvas struct
  itself.
**************************************************************************/
void canvas_free(struct canvas *store)
{
  FREESURFACE(store->surf);
  free(store);
}

/**************************************************************************
  Copies an area from the source canvas to the destination canvas.
**************************************************************************/
void canvas_copy(struct canvas *dest, struct canvas *src,
		     int src_x, int src_y, int dest_x, int dest_y, int width,
		     int height)
{
  SDL_Rect src_rect = {src_x, src_y, width, height};
  SDL_Rect dest_rect = {dest_x, dest_y, width, height};

  alphablit(src->surf, &src_rect, dest->surf, &dest_rect);
}

/****************************************************************************
  Draw some or all of a sprite onto the canvas.
****************************************************************************/
void canvas_put_sprite(struct canvas *pcanvas,
		    int canvas_x, int canvas_y,
		    struct sprite *sprite,
		    int offset_x, int offset_y, int width, int height)
{
  SDL_Rect src = {offset_x, offset_y, width, height};
  SDL_Rect dst = {canvas_x + offset_x, canvas_y + offset_y, 0, 0};
  alphablit(GET_SURF(sprite), &src, pcanvas->surf, &dst);
}

/****************************************************************************
  Draw a full sprite onto the canvas.
****************************************************************************/
void canvas_put_sprite_full(struct canvas *pcanvas,
			 int canvas_x, int canvas_y,
			 struct sprite *sprite)
{
  SDL_Rect dst = {canvas_x, canvas_y, 0, 0};
  alphablit(GET_SURF(sprite), NULL, pcanvas->surf, &dst);
}

/****************************************************************************
  Draw a full sprite onto the canvas.  If "fog" is specified draw it with
  fog.
****************************************************************************/
void canvas_put_sprite_fogged(struct canvas *pcanvas,
			      int canvas_x, int canvas_y,
			      struct sprite *psprite,
			      bool fog, int fog_x, int fog_y)
{
  SDL_Rect dst = {canvas_x, canvas_y, 0, 0};

  if (fog) {
    SDL_Surface *tmp_surf = blend_surface(GET_SURF(psprite), 160);
    alphablit(tmp_surf, NULL, pcanvas->surf, &dst);
    FREESURFACE(tmp_surf);
  } else {
    canvas_put_sprite_full(pcanvas, canvas_x, canvas_y, psprite);
  }

}

/****************************************************************************
  Draw a filled-in colored rectangle onto canvas.
****************************************************************************/
void canvas_put_rectangle(struct canvas *pcanvas, struct color *pcolor,
		          int canvas_x, int canvas_y, int width, int height)
{
  SDL_Rect dst = {canvas_x, canvas_y, width, height};
  
  SDL_FillRect(pcanvas->surf, &dst, SDL_MapRGBA(pcanvas->surf->format,
                                                pcolor->color->r,
                                                pcolor->color->g,
		                                 pcolor->color->b,
                                                pcolor->color->unused));
}

/****************************************************************************
  Fill the area covered by the sprite with the given color.
****************************************************************************/
void canvas_fill_sprite_area(struct canvas *pcanvas,
			     struct sprite *psprite, struct color *pcolor,
 			     int canvas_x, int canvas_y)
{
  SDL_Rect dst = {canvas_x, canvas_y, GET_SURF(psprite)->w,
                                      GET_SURF(psprite)->h};
                  
  SDL_FillRect(pcanvas->surf, &dst, SDL_MapRGBA(pcanvas->surf->format,
                                                pcolor->color->r,
                                                pcolor->color->g,
		                                pcolor->color->b,
                                                pcolor->color->unused));
}

/****************************************************************************
  Fill the area covered by the sprite with the given color.
****************************************************************************/
void canvas_fog_sprite_area(struct canvas *pcanvas, struct sprite *psprite,
			    int canvas_x, int canvas_y)
{
  SDL_Rect dst = {canvas_x, canvas_y, GET_SURF(psprite)->w,
                                      GET_SURF(psprite)->h};
                                      
  SDL_Surface *tmp_surf = create_surf_alpha(GET_SURF(psprite)->w, 
                                            GET_SURF(psprite)->h,
                                            SDL_SWSURFACE);
  SDL_FillRect(tmp_surf, NULL, SDL_MapRGBA(tmp_surf->format, 0, 0, 0, 64));
  alphablit(tmp_surf, NULL, pcanvas->surf, &dst);
  FREESURFACE(tmp_surf);
}

/****************************************************************************
  Draw a 1-pixel-width colored line onto the canvas.
****************************************************************************/
void canvas_put_line(struct canvas *pcanvas, struct color *pcolor,
		  enum line_type ltype, int start_x, int start_y,
		  int dx, int dy)
{
  putline(pcanvas->surf, start_x, start_y, start_x + dx, start_y + dy,
                                    SDL_MapRGBA(pcanvas->surf->format,
                                                pcolor->color->r,
                                                pcolor->color->g,
		                                pcolor->color->b,
                                                pcolor->color->unused));
}

/****************************************************************************
  Draw a 1-pixel-width colored curved line onto the canvas.
****************************************************************************/
void canvas_put_curved_line(struct canvas *pcanvas, struct color *pcolor,
                            enum line_type ltype, int start_x, int start_y,
                            int dx, int dy)
{
  /* FIXME: Implement curved line drawing. */
  canvas_put_line(pcanvas, pcolor, ltype, start_x, start_y, dx, dy);
}

/****************************************************************************
  Return the size of the given text in the given font.  This size should
  include the ascent and descent of the text.  Either of width or height
  may be NULL in which case those values simply shouldn't be filled out.
****************************************************************************/
void get_text_size(int *width, int *height,
		   enum client_font font, const char *text)
{
  SDL_String16 *pText = create_string16(NULL, 0, *fonts[font]);
  copy_chars_to_string16(pText, text);

  if (width) {
    *width = str16size(pText).w;
  }
  if (height) {
    *height = str16size(pText).h;
  }
  
  FREESTRING16(pText);
}

/****************************************************************************
  Draw the text onto the canvas in the given color and font.  The canvas
  position does not account for the ascent of the text; this function must
  take care of this manually.  The text will not be NULL but may be empty.
****************************************************************************/
void canvas_put_text(struct canvas *pcanvas, int canvas_x, int canvas_y,
		     enum client_font font, struct color *pcolor,
		     const char *text)
{
  SDL_Surface *pTmp;
  SDL_String16 *pText = create_string16(NULL, 0, *fonts[font]);
  copy_chars_to_string16(pText, text);
    
  pText->fgcol = *pcolor->color;
  pText->bgcol = (SDL_Color) {0, 0, 0, 0};
 
  pTmp = create_text_surf_from_str16(pText);
  
  blit_entire_src(pTmp, pcanvas->surf, canvas_x, canvas_y);

  FREESTRING16(pText);  
  FREESURFACE(pTmp);
}
