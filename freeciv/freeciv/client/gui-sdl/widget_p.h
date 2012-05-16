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

#ifndef FC__WIDGET_P_H
#define FC__WIDGET_P_H

#define STATE_MASK		0x03       /* 0..0000000000000011 */
#define TYPE_MASK		0x03FC     /* 0..0000001111111100 */
#define FLAG_MASK		0xFFFFFC00 /* 1..1111110000000000 */

struct widget *widget_new(void);

void correct_size_bcgnd_surf(SDL_Surface *pTheme,
				    Uint16 *pWidth, Uint16 *pHigh);
SDL_Surface *get_buffer_layer(int width, int height);

int redraw_iconlabel(struct widget *pLabel);
  
#endif /* FC__WIDGET_P_H */
