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

#ifndef FC__WIDGET_ICON_H
#define FC__WIDGET_ICON_H

/* ICON */
void set_new_icon_theme(struct widget *pIcon_Widget,
			SDL_Surface *pNew_Theme);
SDL_Surface *create_icon_theme_surf(SDL_Surface *pIcon);
struct widget *create_themeicon(SDL_Surface *pIcon_theme,
			  struct gui_layer *pDest ,Uint32 flags);
SDL_Surface *create_icon_from_theme(SDL_Surface *pIcon_theme,
				    Uint8 state);
int draw_icon_from_theme(SDL_Surface *pIcon_theme, Uint8 state,
			 struct gui_layer *pDest, Sint16 start_x,
			 Sint16 start_y);
int draw_icon(struct widget *pIcon, Sint16 start_x, Sint16 start_y);

/* ICON2 */
void set_new_icon2_theme(struct widget *pIcon_Widget, SDL_Surface *pNew_Theme,
			  bool free_old_theme);
struct widget *create_icon2(SDL_Surface *pIcon, struct gui_layer *pDest, Uint32 flags);


#endif /* FC__WIDGET_ICON_H */
