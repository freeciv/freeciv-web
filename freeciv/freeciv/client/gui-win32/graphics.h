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
#ifndef FC__GRAPHICS_H
#define FC__GRAPHICS_H

#include "graphics_g.h"

struct crect
{
  BITMAP *src;
  HBITMAP hbmp;
  int x;
  int y;
  int w;
  int h;
};

extern struct sprite *intro_gfx_sprite;
extern struct sprite *radar_gfx_sprite;

BITMAP *bmp_new(int width, int height);
void bmp_free(BITMAP *bmp);

HBITMAP BITMAP2HBITMAP(BITMAP *bmp);
BITMAP *HBITMAP2BITMAP(HBITMAP hbmp);
struct crect *getcachehbitmap(BITMAP *bmp, int *cache_id);

bool bmp_test_mask(BITMAP *bmp);
bool bmp_test_alpha(BITMAP *bmp);

BITMAP *bmp_premult_alpha(BITMAP *bmp);
BITMAP *bmp_generate_mask(BITMAP *bmp);
BITMAP *bmp_crop(BITMAP *bmp, int src_x, int src_y, int width, int height);
BITMAP *bmp_blend_alpha(BITMAP *bmp, BITMAP *mask,
			int offset_x, int offset_y);
BITMAP *bmp_fog(BITMAP *bmp, int brightness);

BITMAP *bmp_load_png(const char *filename);

void blend_bmp_to_hdc(HDC hdc, int dst_x, int dst_y, int w, int h,
		      BITMAP *bmp, int src_x, int src_y);

#endif  /* FC__GRAPHICS_H */
