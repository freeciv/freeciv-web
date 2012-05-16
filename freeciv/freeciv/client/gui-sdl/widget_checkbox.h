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

#ifndef FC__WIDGET_CHECKBOX_H
#define FC__WIDGET_CHECKBOX_H

struct CHECKBOX {
  SDL_Surface *pTRUE_Theme;
  SDL_Surface *pFALSE_Theme;
  bool state;
};

struct widget *create_textcheckbox(struct gui_layer *pDest, bool state,
                                   SDL_String16 *pStr, Uint32 flags);
struct widget *create_checkbox(struct gui_layer *pDest, bool state, Uint32 flags);
void togle_checkbox(struct widget *pCBox);
bool get_checkbox_state(struct widget *pCBox);
int set_new_checkbox_theme(struct widget *pCBox ,
                           SDL_Surface *pTrue, SDL_Surface *pFalse);

#endif /* FC__WIDGET_CHECKBOX_H */
