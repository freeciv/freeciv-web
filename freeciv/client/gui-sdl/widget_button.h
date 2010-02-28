/**********************************************************************
 Freeciv - Copyright (C) 2006 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifndef FC__WIDGET_BUTTON_H
#define FC__WIDGET_BUTTON_H

#define create_icon_button_from_unichar(pIcon, pDest, pUniChar, pUniCharSize, iPtsize, flags) \
	create_icon_button(pIcon, pDest, create_string16(pUniChar, pUniCharSize, iPtsize), flags)

#define create_icon_button_from_chars(pIcon, pDest, pCharString, iPtsize, flags) \
	create_icon_button(pIcon, pDest,                                        \
			   create_str16_from_char(pCharString, iPtsize),  \
			   flags)

#define create_themeicon_button_from_unichar(pIcon_theme, pDest, pUniChar, pUniCharSize, iPtsize, flags) \
	create_themeicon_button(pIcon, pDest, create_string16(pUniChar, pUniCharSize, iPtsize), \
				flags)

#define create_themeicon_button_from_chars(pIcon_theme, pDest, pCharString, iPtsize, flags) \
	create_themeicon_button(pIcon_theme, pDest,                 \
				create_str16_from_char(pCharString, \
						       iPtsize),    \
				flags)

struct widget *create_icon_button(SDL_Surface *pIcon,
	  struct gui_layer *pDest, SDL_String16 *pString, Uint32 flags);

struct widget *create_themeicon_button(SDL_Surface *pIcon_theme,
		struct gui_layer *pDest, SDL_String16 *pString, Uint32 flags);

int draw_tibutton(struct widget *pButton, Sint16 start_x, Sint16 start_y);
int draw_ibutton(struct widget *pButton, Sint16 start_x, Sint16 start_y);

#endif /* FC__WIDGET_BUTTON_H */
