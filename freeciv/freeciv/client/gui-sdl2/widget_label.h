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

#ifndef FC__WIDGET_LABEL_H
#define FC__WIDGET_LABEL_H

#define create_iconlabel_from_chars(picon, pdest, chars, ptsize, flags) \
	create_iconlabel(picon, pdest, create_utf8_from_char(chars, ptsize), flags)

#define create_active_iconlabel(pBuf, pDest, pstr, pString, pCallback)   \
do { 									 \
  pstr = create_utf8_from_char(pString, 10);				 \
  pstr->style |= TTF_STYLE_BOLD;					 \
  pBuf = create_iconlabel(NULL, pDest, pstr, 				 \
    	     (WF_RESTORE_BACKGROUND|WF_DRAW_TEXT_LABEL_WITH_SPACE)); \
  pBuf->action = pCallback;						 \
} while (FALSE)

struct widget *create_themelabel(SDL_Surface *pBackground, struct gui_layer *pDest,
                                 utf8_str *pstr, Uint16 w, Uint16 h,
                                 Uint32 flags);
struct widget *create_themelabel2(SDL_Surface *pIcon, struct gui_layer *pDest,
                                  utf8_str *pstr, Uint16 w, Uint16 h, Uint32 flags);
struct widget *create_iconlabel(SDL_Surface *pIcon, struct gui_layer *pDest,
                                utf8_str *text, Uint32 flags);
struct widget *convert_iconlabel_to_themeiconlabel2(struct widget *pIconLabel);
int draw_label(struct widget *pLabel, Sint16 start_x, Sint16 start_y);

int redraw_iconlabel(struct widget *pLabel);
void remake_label_size(struct widget *pLabel);

#endif /* FC__WIDGET_LABEL_H */
