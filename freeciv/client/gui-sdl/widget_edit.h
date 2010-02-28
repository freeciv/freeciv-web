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

#ifndef FC__WIDGET_EDIT_H
#define FC__WIDGET_EDIT_H

enum Edit_Return_Codes {
  ED_RETURN = 1,
  ED_ESC = 2,
  ED_MOUSE = 3,
  ED_FORCE_EXIT = 4
};

#define create_edit_from_chars(pBackground, pDest, pCharString, iPtsize, length, flags)                                                                 \
	create_edit(pBackground, pDest,                                 \
		    create_str16_from_char(pCharString, iPtsize), \
		    length, flags)

#define create_edit_from_unichars(pBackground, pDest, pUniChar, pUniCharSize, iPtsize, length, flags) \
	create_edit(pBackground, pDest, create_string16(pUniChar, pUniCharSize, iPtsize), length, flags )

#define edit(pEdit) edit_field(pEdit)

struct widget *create_edit(SDL_Surface *pBackground, struct gui_layer *pDest,
			SDL_String16 *pString16, Uint16 lenght,
			Uint32 flags);
enum Edit_Return_Codes edit_field(struct widget *pEdit_Widget);
int draw_edit(struct widget *pEdit, Sint16 start_x, Sint16 start_y);

#endif /* FC__WIDGET_EDIT_H */
