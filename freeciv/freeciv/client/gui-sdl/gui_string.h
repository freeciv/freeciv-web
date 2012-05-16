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
                          gui_string.h  -  description
                             -------------------
    begin                : Thu May 30 2002
    copyright            : (C) 2002 by Rafał Bursig
    email                : Rafał Bursig <bursig@poczta.fm>
 **********************************************************************/

#ifndef FC__GUISTRING_H
#define FC__GUISTRING_H

#include "SDL.h"

#include "fc_types.h"

#include "SDL_ttf.h"
#include "gui_main.h"

#define SF_CENTER	8
#define SF_CENTER_RIGHT	16

/* styles:
	TTF_STYLE_NORMAL	0
	TTF_STYLE_BOLD		1
	TTF_STYLE_ITALIC	2
	TTF_STYLE_UNDERLINE	4
	SF_CENTER	 	8	- use with multi string
	SF_CENTER_RIGHT		16	- use with multi string
*/

typedef struct SDL_String16 {
  Uint8 style;
  Uint8 render;
  Uint16 ptsize;
  size_t n_alloc;		/* total allocated text memory */
  SDL_Color fgcol;
  SDL_Color bgcol;
  TTF_Font *font;
  Uint16 *text;
} SDL_String16;

SDL_String16 * create_string16(Uint16 *pInTextString,
					size_t n_alloc, Uint16 ptsize);
SDL_String16 * copy_chars_to_string16(SDL_String16 *pString,
					const char *pCharString);
bool convert_string_to_const_surface_width(SDL_String16 *pString,
								int width);
int write_text16(SDL_Surface * pDest, Sint16 x, Sint16 y,
		 SDL_String16 * pString);
SDL_Surface * create_text_surf_from_str16(SDL_String16 *pString);
SDL_Surface * create_text_surf_smaller_that_w(SDL_String16 *pString, int w);
SDL_Rect str16size(SDL_String16 *pString16);
void change_ptsize16(SDL_String16 *pString, Uint16 new_ptsize);

void unload_font(Uint16 ptsize);
void free_font_system(void);

/* adjust font sizes on 320x240 screen */
#ifdef SMALL_SCREEN
  int adj_font(int size);
#else
  #define adj_font(size) size
#endif

#define str16len(pString16) str16size(pString16).w
#define str16height(pString16) str16size(pString16).h

/*
 *	here we use ordinary free( ... ) becouse check is made 
 *	on start.
 */
#define FREESTRING16( pString16 )		\
do {						\
	if (pString16) {			\
		FC_FREE(pString16->text);		\
		unload_font(pString16->ptsize);	\
		free(pString16); 		\
		pString16 = NULL;		\
	}					\
} while(0)

#define create_str16_from_char(pInCharString, iPtsize) \
        copy_chars_to_string16(create_string16(NULL, 0,iPtsize), pInCharString)

#endif /* FC__GUISTRING_H */
