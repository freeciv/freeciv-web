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
#ifndef FC__SPRITE_H
#define FC__SPRITE_H

#include "sprite_g.h"

/**********************************************************************
Some notes about sprite management in this client:
Some windows versions (e.g: Win95, Win98, WinME) have some strange 
limitiations on gdi objects (like bitmaps). The number of bitmaps 
(I really mean number and not size) is limited. And the limit is too low
for the sprites freeciv uses.
That means HBITMAPs cannot be stored in a sprite struct.
The bitmaps are stored using GetObject/GetBitmapBits. 
**********************************************************************/

struct sprite
{
  BITMAP *img;
  BITMAP *fog;
  BITMAP *mask;
  BITMAP *pmimg;
  int img_cache_id;
  int fog_cache_id;
  int mask_cache_id;
  int pmimg_cache_id;
  int width;
  int height;
};

void fog_sprite(struct sprite *sprite);
void draw_sprite(struct sprite *sprite, HDC hdc, int x, int y);
void draw_sprite_fog(struct sprite *sprite, HDC hdc, int x, int y);
void draw_sprite_part(struct sprite *sprite, HDC hdc, int x, int y, int w,
		      int h, int offset_x, int offset_y);
void init_fog_bmp(void);
void draw_fog(struct sprite *sprite, HDC hdc, int x, int y);

#endif  /* FC__SPRITE_H */
