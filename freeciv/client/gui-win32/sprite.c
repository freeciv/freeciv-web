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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdio.h>

#include <windows.h>
#include <png.h>

#include "log.h"

#include "tilespec.h"

#include "graphics.h"
#include "sprite.h"

extern bool have_AlphaBlend;

extern BOOL (WINAPI * AlphaBlend)(HDC,int,int,int,int,HDC,int,int,int,int,BLENDFUNCTION);

static HBITMAP stipple;
static HBITMAP fogmask;

/**************************************************************************
  Return a NULL-terminated, permanently allocated array of possible
  graphics types extensions.  Extensions listed first will be checked
  first.
**************************************************************************/
const char **
gfx_fileextensions(void)
{
  static const char *ext[] =
  {
    "png",
    NULL
  };

  return ext;
}

/**************************************************************************
  Create a new sprite from the given bitmap.
**************************************************************************/
static struct sprite *sprite_new(BITMAP *bmp)
{
  struct sprite *mysprite;

  mysprite = fc_malloc(sizeof(struct sprite));

  mysprite->img            = bmp;
  mysprite->fog            = NULL;
  mysprite->mask           = NULL;
  mysprite->pmimg          = NULL;
  mysprite->img_cache_id   = -1;
  mysprite->fog_cache_id   = -1;
  mysprite->mask_cache_id  = -1;
  mysprite->pmimg_cache_id = -1;
  mysprite->width          = bmp->bmWidth;
  mysprite->height         = bmp->bmHeight;

  if (bmp_test_mask(bmp)) {
    mysprite->mask = bmp_generate_mask(bmp);
    if (bmp_test_alpha(bmp)) {
      mysprite->pmimg = bmp_premult_alpha(bmp);
    }
  }

  return mysprite;
}

/***************************************************************************
  Load the given graphics file into a sprite.  This function loads an
  entire image file, which may later be broken up into individual sprites
  with crop_sprite.
***************************************************************************/
struct sprite *load_gfxfile(const char *filename)
{
  return sprite_new(bmp_load_png(filename));
}

/****************************************************************************
  Create a new sprite by cropping and taking only the given portion of
  the image.

  source gives the sprite that is to be cropped.

  x,y, width, height gives the rectangle to be cropped.  The pixel at
  position of the source sprite will be at (0,0) in the new sprite, and
  the new sprite will have dimensions (width, height).

  mask gives an additional mask to be used for clipping the new sprite.

  mask_offset_x, mask_offset_y is the offset of the mask relative to the
  origin of the source image.  The pixel at (mask_offset_x,mask_offset_y)
  in the mask image will be used to clip pixel (0,0) in the source image
  which is pixel (-x,-y) in the new image.
****************************************************************************/
struct sprite *crop_sprite(struct sprite *sprsrc,
			   int x, int y, int width, int height,
			   struct sprite *sprmask,
			   int mask_offset_x, int mask_offset_y)
{
  BITMAP *crop_bmp, *dst_bmp;

  crop_bmp = bmp_crop(sprsrc->img, x, y, width, height);

  if (sprmask && sprmask->mask) {
    dst_bmp = bmp_blend_alpha(crop_bmp, sprmask->img,
			      x - mask_offset_x, y - mask_offset_y);
    bmp_free (crop_bmp);
  } else {
    dst_bmp = crop_bmp;
  }

  return sprite_new(dst_bmp);
}

/****************************************************************************
  Find the dimensions of the sprite.
****************************************************************************/
void get_sprite_dimensions(struct sprite *sprite, int *width, int *height)
{
  *width = sprite->width;
  *height = sprite->height;
}


/**************************************************************************
  Free a sprite and all associated image data.
**************************************************************************/
void free_sprite(struct sprite *s)
{
  if (s->mask) {
    bmp_free(s->mask);
  }

  if (s->fog) {
    bmp_free(s->fog);
  }

  if (s->pmimg) {
    bmp_free(s->pmimg);
  }

  bmp_free(s->img);

  free(s);
}

/**************************************************************************
  Initialize the stipple bitmap and the fog mask.
***************************************************************************/
void init_fog_bmp(void)
{
  int x,y;
  HBITMAP old;
  HDC hdc;
  hdc = CreateCompatibleDC(NULL);
  if (stipple) {
    DeleteObject(stipple);
  }
  if (fogmask) {
    DeleteObject(fogmask);
  }
  stipple = CreateCompatibleBitmap(hdc, tileset_tile_width(tileset), tileset_tile_height(tileset));
  fogmask = CreateCompatibleBitmap(hdc, tileset_tile_width(tileset), tileset_tile_height(tileset));
  old = SelectObject(hdc, stipple);
  BitBlt(hdc, 0, 0, tileset_tile_width(tileset), tileset_tile_height(tileset), NULL, 0, 0, BLACKNESS);
  for(x = 0; x < tileset_tile_width(tileset); x++) {
    for(y = 0; y < tileset_tile_height(tileset); y++) {
      if ((x + y) & 1) {
	SetPixel(hdc, x, y, RGB(255, 255, 255));
      }
    }
  }
  SelectObject(hdc, old);
  DeleteDC(hdc);  
}

/**************************************************************************
  Create a dimmed version of this sprite's image.
**************************************************************************/
void fog_sprite(struct sprite *sprite)
{
  sprite->fog = bmp_fog(sprite->img, 65);
}

/**************************************************************************
  Draw a sprite to the specified device context.
**************************************************************************/
static void real_draw_sprite(struct sprite *sprite, HDC hdc, int x, int y,
			     int w, int h, int src_x, int src_y, bool fog)
{
  HDC src_hdc;
  HGDIOBJ tmp;
  struct crect *bmpe;
  bool blend;

  if (!sprite) return;

  if (src_x < 0) {
    x -= src_x;
    src_x = 0;
  }

  if (src_y < 0) {
    y -= src_y;
    src_y = 0;
  }

  w = MIN(w, sprite->width  - src_x);
  h = MIN(h, sprite->height - src_y);

  blend = gui_win32_enable_alpha && sprite->pmimg;

  if (!fog && blend && !have_AlphaBlend) {
    blend_bmp_to_hdc(hdc, x, y, w, h, sprite->pmimg, src_x, src_y);
    return;
  }

  if (fog) {
    bmpe = getcachehbitmap(sprite->fog,   &(sprite->fog_cache_id));
  } else if (blend) {
    bmpe = getcachehbitmap(sprite->pmimg, &(sprite->pmimg_cache_id));
  } else {
    bmpe = getcachehbitmap(sprite->img,   &(sprite->img_cache_id));
  }

  if (!fog && blend) {
    BLENDFUNCTION bf;

    src_hdc = CreateCompatibleDC(hdc);
    tmp = SelectObject(src_hdc, bmpe->hbmp);

    bf.BlendOp             = AC_SRC_OVER;
    bf.BlendFlags          = 0;
    bf.SourceConstantAlpha = 255;
    bf.AlphaFormat         = AC_SRC_ALPHA;
    AlphaBlend(hdc, x, y, w, h, src_hdc,
	       src_x + bmpe->x, src_y + bmpe->y, w, h, bf);
  } else if (sprite->mask) {
    HDC mask_hdc;
    HGDIOBJ tmp2;
    HBITMAP mask = BITMAP2HBITMAP(sprite->mask);

    src_hdc = CreateCompatibleDC(hdc);
    tmp = SelectObject(src_hdc, bmpe->hbmp);

    mask_hdc = CreateCompatibleDC(hdc);
    tmp2 = SelectObject(mask_hdc, mask);

    BitBlt(hdc, x, y, w, h, src_hdc,  src_x + bmpe->x, src_y + bmpe->y, SRCINVERT);
    BitBlt(hdc, x, y, w, h, mask_hdc, src_x, src_y, SRCAND);
    BitBlt(hdc, x, y, w, h, src_hdc,  src_x + bmpe->x, src_y + bmpe->y, SRCINVERT);

    SelectObject(mask_hdc, tmp2);
    DeleteDC(mask_hdc);
    DeleteObject(mask);
  } else {
    src_hdc = CreateCompatibleDC(hdc);
    tmp = SelectObject(src_hdc, bmpe->hbmp);
    BitBlt(hdc, x, y, w, h, src_hdc, src_x + bmpe->x, src_y + bmpe->y, SRCCOPY);
  }

  SelectObject(src_hdc, tmp);
  DeleteDC(src_hdc);
}

/**************************************************************************
  Wrapper for real_draw_sprite(), for fogged sprites.
**************************************************************************/
void draw_sprite_fog(struct sprite *sprite, HDC hdc, int x, int y)
{
  real_draw_sprite(sprite, hdc, x, y, sprite->width, sprite->height, 0, 0,
		   TRUE);
}

/**************************************************************************
  Wrapper for real_draw_sprite(), for unfogged sprites.
**************************************************************************/
void draw_sprite(struct sprite *sprite, HDC hdc, int x, int y)
{
  real_draw_sprite(sprite, hdc, x, y, sprite->width, sprite->height, 0, 0,
		   FALSE);
}

/**************************************************************************
  Wrapper for real_draw_sprite(), for partial sprites.
**************************************************************************/
void draw_sprite_part(struct sprite *sprite, HDC hdc, int x, int y, int w,
		      int h, int offset_x, int offset_y)
{
  real_draw_sprite(sprite, hdc, x, y, w, h, offset_x, offset_y, FALSE);
}

/**************************************************************************
  Draw stippled fog, using the mask of the specified sprite.
**************************************************************************/
void draw_fog(struct sprite *sprmask, HDC hdc, int x, int y)
{
#if 0
  HDC hdcmask;
  HDC hdcsrc;

  HBITMAP tempmask;
  HBITMAP tempsrc;

  HBITMAP bmpmask;

  int w, h;

  w = sprmask->width;
  h = sprmask->height;

  bmpmask = getcachehbitmap(sprmask->mask, &(sprmask->mask_cache_id));

  hdcsrc = CreateCompatibleDC(hdc);
  tempsrc = SelectObject(hdcsrc, bmpmask);

  hdcmask = CreateCompatibleDC(hdc);
  tempmask = SelectObject(hdcmask, fogmask); 

  BitBlt(hdcmask, 0, 0, w, h, hdcsrc, 0, 0, SRCCOPY);
  
  SelectObject(hdcsrc, stipple);

  BitBlt(hdcmask, 0, 0, w, h, hdcsrc, 0, 0, SRCPAINT);

  SelectObject(hdcsrc, tempsrc);
  DeleteDC(hdcsrc);

  BitBlt(hdc, x, y, w, h, hdcmask, 0, 0, SRCAND);

  SelectObject(hdcmask, tempmask);
  DeleteDC(hdcmask);
#endif
}

