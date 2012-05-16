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
                          graphics.c  -  description
                             -------------------
    begin                : Mon Jul 1 2002
    copyright            : (C) 2000 by Michael Speck
			 : (C) 2002 by Rafał Bursig
    email                : Michael Speck <kulkanie@gmx.net>
			 : Rafał Bursig <bursig@poczta.fm>
 **********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include "SDL_image.h"
#include "SDL_syswm.h"

/* utility */
#include "fcintl.h"
#include "log.h"

/* client */
#include "tilespec.h"

/* gui-sdl */
#include "SDL_ttf.h"
#include "gui_tilespec.h"
#include "mapview.h"
#include "themebackgrounds.h"
#include "themespec.h"

#include "graphics.h"

#ifdef HAVE_MMX1
#include "mmx.h"
#endif

/* ------------------------------ */

struct main Main;

struct gui_layer *gui_layer_new(int x, int y, SDL_Surface *surface)
{
  struct gui_layer *result;
    
  result = fc_calloc(1, sizeof(struct gui_layer));
    
  result->dest_rect = (SDL_Rect){x, y, 0, 0};
  result->surface = surface;
  
  return result;
}

void gui_layer_destroy(struct gui_layer **gui_layer)
{
  FREESURFACE((*gui_layer)->surface);
  FC_FREE(*gui_layer);
}

struct gui_layer *get_gui_layer(SDL_Surface *surface)
{
  int i = 0;
  
  while ((i < Main.guis_count) && Main.guis[i]) {
    if(Main.guis[i]->surface == surface) {
      return Main.guis[i];
    }
    i++;
  }
  
  return NULL;
}

/**************************************************************************
  Buffer allocation function.
  This function is call by "create_window(...)" function and allocate 
  buffer layer for this function.

  Pointer for this buffer is put in buffer array on last position that 
  flush functions will draw this layer last.
**************************************************************************/
struct gui_layer *add_gui_layer(int width, int height)
{
  struct gui_layer *gui_layer = NULL;
  SDL_Surface *pBuffer;

  pBuffer = create_surf_alpha(/*Main.screen->w*/width, /*Main.screen->h*/height, SDL_SWSURFACE);
  gui_layer = gui_layer_new(0, 0, pBuffer);
  
  /* add to buffers array */
  if (Main.guis) {
    int i;
    /* find NULL element */
    for(i = 0; i < Main.guis_count; i++) {
      if(!Main.guis[i]) {
        Main.guis[i] = gui_layer;
	return gui_layer;
      }
    }
    Main.guis_count++;
    Main.guis = fc_realloc(Main.guis, Main.guis_count * sizeof(struct gui_layer *));
    Main.guis[Main.guis_count - 1] = gui_layer;
  } else {
    Main.guis = fc_calloc(1, sizeof(struct gui_layer *));
    Main.guis[0] = gui_layer;
    Main.guis_count = 1;
  }
  
  return gui_layer;
}

/**************************************************************************
  Free buffer layer ( call by popdown_window_group_dialog(...) funct )
  Funct. free buffer layer and cleare buffer array entry.
**************************************************************************/
void remove_gui_layer(struct gui_layer *gui_layer)
{
  int i;
  
  for(i = 0; i < Main.guis_count - 1; i++) {
    if(Main.guis[i] && (Main.guis[i]== gui_layer)) {
      gui_layer_destroy(&Main.guis[i]);
      Main.guis[i] = Main.guis[i + 1];
      Main.guis[i + 1] = NULL;
    } else {
      if(!Main.guis[i]) {
	Main.guis[i] = Main.guis[i + 1];
        Main.guis[i + 1] = NULL;
      }
    }
  }

  if (Main.guis[Main.guis_count - 1]) {
    gui_layer_destroy(&Main.guis[Main.guis_count - 1]);
  }
}

void screen_rect_to_layer_rect(struct gui_layer *gui_layer, SDL_Rect *dest_rect)
{
  if (gui_layer) {
    dest_rect->x = dest_rect->x - gui_layer->dest_rect.x;
    dest_rect->y = dest_rect->y - gui_layer->dest_rect.y;
  }
}

/* ============ FreeCiv sdl graphics function =========== */

int alphablit(SDL_Surface *src, SDL_Rect *srcrect, 
              SDL_Surface *dst, SDL_Rect *dstrect) {

  if (!(src && dst)) {
    return 0;
  }

  /* use for RGBA->RGBA blits only */  
  if (src->format->Amask && dst->format->Amask) {
    return pygame_AlphaBlit(src, srcrect, dst, dstrect);
  } else {
    return SDL_BlitSurface(src, srcrect, dst, dstrect);
  }   
}

/**************************************************************************
  Create new surface (pRect->w x pRect->h size) and copy pRect area of
  pSource.
  if pRect == NULL then create copy of entire pSource.
**************************************************************************/
SDL_Surface * crop_rect_from_surface(SDL_Surface *pSource,
				    SDL_Rect *pRect)
{
  SDL_Surface *pNew = create_surf_with_format(pSource->format,
					      pRect ? pRect->w : pSource->w,
					      pRect ? pRect->h : pSource->h,
					      SDL_SWSURFACE);
  if (alphablit(pSource, pRect, pNew, NULL) != 0) {
    FREESURFACE(pNew);
  }
  
  return pNew;
}

/**************************************************************************
  Reduce the alpha of the final surface proportional to the alpha of the mask.
  Thus if the mask has 50% alpha the final image will be reduced by 50% alpha.

  mask_offset_x, mask_offset_y is the offset of the mask relative to the
  origin of the source image.  The pixel at (mask_offset_x,mask_offset_y)
  in the mask image will be used to clip pixel (0,0) in the source image
  which is pixel (-x,-y) in the new image.
**************************************************************************/
SDL_Surface *mask_surface(SDL_Surface * pSrc, SDL_Surface * pMask,
                          int mask_offset_x, int mask_offset_y)
{
  SDL_Surface *pDest = NULL;
  
  int row, col;  
  bool free_pMask = FALSE;
  Uint32 *pSrc_Pixel = NULL;
  Uint32 *pDest_Pixel = NULL;
  Uint32 *pMask_Pixel = NULL;
  unsigned char src_alpha, mask_alpha;

  if (!pMask->format->Amask) {
    pMask = SDL_DisplayFormatAlpha(pMask);
    free_pMask = TRUE;
  }
  
  pSrc = SDL_DisplayFormatAlpha(pSrc);
  pDest = SDL_DisplayFormatAlpha(pSrc);
  
  lock_surf(pSrc);
  lock_surf(pMask);  
  lock_surf(pDest);
  
  pSrc_Pixel = (Uint32 *)pSrc->pixels;
  pDest_Pixel = (Uint32 *)pDest->pixels;

  for (row = 0; row < pSrc->h; row++) {
      
    pMask_Pixel = (Uint32 *)pMask->pixels
                  + pMask->w * (row + mask_offset_y)
                  + mask_offset_x;
    
    for (col = 0; col < pSrc->w; col++) {
      src_alpha = (*pSrc_Pixel & pSrc->format->Amask) >> pSrc->format->Ashift;
      mask_alpha = (*pMask_Pixel & pMask->format->Amask) >> pMask->format->Ashift;
      
      *pDest_Pixel = (*pSrc_Pixel & ~pSrc->format->Amask)
                   | (((src_alpha * mask_alpha) / 255) << pDest->format->Ashift);
      
      pSrc_Pixel++; pDest_Pixel++; pMask_Pixel++;
    }
  }

  unlock_surf(pDest);
  unlock_surf(pMask);    
  unlock_surf(pSrc);
  
  if (free_pMask) {
    FREESURFACE(pMask);
  }

  FREESURFACE(pSrc); /* result of SDL_DisplayFormatAlpha() */
  
  return pDest;
}

SDL_Surface *blend_surface(SDL_Surface *pSrc, unsigned char alpha)
{
  SDL_Surface *pMask = SDL_DisplayFormatAlpha(pSrc);
  SDL_Color c = {255, 255, 255, alpha}; 
  SDL_FillRect(pMask, NULL, map_rgba(pMask->format, c));
  return mask_surface(pSrc, pMask, 0, 0);   
}

/**************************************************************************
  Load a surface from file putting it in software mem.
**************************************************************************/
SDL_Surface *load_surf(const char *pFname)
{
  SDL_Surface *pBuf;
  
  if(!pFname) {
    return NULL;
  }
  
  if ((pBuf = IMG_Load(pFname)) == NULL) {
    freelog(LOG_ERROR, _("load_surf: Failed to load graphic file %s!"),
	    pFname);
    return NULL;
  }
  
  if(Main.screen) {
    SDL_Surface *pNew_sur;
    if ((pNew_sur = SDL_DisplayFormatAlpha(pBuf)) == NULL) {
      freelog(LOG_ERROR, _("load_surf: Unable to convert file %s "
			 "into screen's format!"), pFname);
    } else {
      FREESURFACE(pBuf);
      return pNew_sur;
    }
  }
  
  return pBuf;
}

/**************************************************************************
  Load a surface from file putting it in soft or hardware mem
  Warning: pFname must have full path to file
**************************************************************************/
SDL_Surface *load_surf_with_flags(const char *pFname, int iFlags)
{
  SDL_Surface *pBuf = NULL;
  SDL_Surface *pNew_sur = NULL;
  SDL_PixelFormat *pSpf = SDL_GetVideoSurface()->format;

  if ((pBuf = IMG_Load(pFname)) == NULL) {
    freelog(LOG_ERROR, _("load_surf_with_flags: "
                         "Unable to load file %s."), pFname);
    return NULL;
  }

  if ((pNew_sur = SDL_ConvertSurface(pBuf, pSpf, iFlags)) == NULL) {
    freelog(LOG_ERROR, _("Unable to convert image from file %s "
			 "into format %d."), pFname, iFlags);
    return pBuf;
  }

  FREESURFACE(pBuf);

  return pNew_sur;
}

/**************************************************************************
   create an surface with format
   MUST NOT BE USED IF NO SDLSCREEN IS SET
**************************************************************************/
SDL_Surface *create_surf_with_format(SDL_PixelFormat * pSpf,
				     int iWidth, int iHeight,
				     Uint32 iFlags)
{
  SDL_Surface *pSurf = SDL_CreateRGBSurface(iFlags, iWidth, iHeight,
					    pSpf->BitsPerPixel,
					    pSpf->Rmask,
					    pSpf->Gmask,
					    pSpf->Bmask, pSpf->Amask);

  if (!pSurf) {
    freelog(LOG_ERROR, _("Unable to create Sprite (Surface) of size "
			 "%d x %d %d Bits in format %d"), iWidth, 
	    			iHeight, pSpf->BitsPerPixel, iFlags);
    return NULL;
  }

  return pSurf;
}

/**************************************************************************
  create a surface with alpha channel
**************************************************************************/
SDL_Surface *create_surf_alpha(int iWidth, int iHeight, Uint32 iFlags) {
  SDL_Surface *pTmp = create_surf(iWidth, iHeight, iFlags);
  SDL_Surface *pNew = SDL_DisplayFormatAlpha(pTmp);
  FREESURFACE(pTmp);
  clear_surface(pNew, NULL);  
  
  return pNew;
}

/**************************************************************************
  create an surface with screen format and fill with color.
  if pColor == NULL surface is filled with transparent white A = 128
  MUST NOT BE USED IF NO SDLSCREEN IS SET
**************************************************************************/
SDL_Surface *create_filled_surface(Uint16 w, Uint16 h, Uint32 iFlags,
				   SDL_Color * pColor, bool add_alpha)
{
  SDL_Surface *pNew;

  if (add_alpha) {
    pNew = create_surf_alpha(w, h, iFlags);
  } else {
    pNew = create_surf(w, h, iFlags);
  }
  
  if (!pNew) {
    return NULL;
  }

  if (!pColor) {
    /* pColor->unused == ALPHA */
    SDL_Color color ={255, 255, 255, 128};
    pColor = &color;
  }

  SDL_FillRect(pNew, NULL,
	       SDL_MapRGBA(pNew->format, pColor->r, pColor->g, pColor->b,
			   pColor->unused));

  if (pColor->unused != 255) {
    SDL_SetAlpha(pNew, SDL_SRCALPHA, pColor->unused);
  }

  return pNew;
}

/**************************************************************************
  fill surface with (0, 0, 0, 0), so the next blitting operation can set
  the per pixel alpha
**************************************************************************/
int clear_surface(SDL_Surface *pSurf, SDL_Rect *dstrect) {
  /* SDL_FillRect might change the rectangle, so we create a copy */
  if (dstrect) {
    SDL_Rect _dstrect = *dstrect;
    return SDL_FillRect(pSurf, &_dstrect, SDL_MapRGBA(pSurf->format, 0, 0, 0, 0));
  } else {
    return SDL_FillRect(pSurf, NULL, SDL_MapRGBA(pSurf->format, 0, 0, 0, 0));
  }
}

/**************************************************************************
  blit entire src [SOURCE] surface to destination [DEST] surface
  on position : [iDest_x],[iDest_y] using it's actual alpha and
  color key settings.
**************************************************************************/
int blit_entire_src(SDL_Surface * pSrc, SDL_Surface * pDest,
		    Sint16 iDest_x, Sint16 iDest_y)
{
  SDL_Rect dest_rect = { iDest_x, iDest_y, 0, 0 };
  return alphablit(pSrc, NULL, pDest, &dest_rect);
}


/*
 * this is center main application window function
 * currently it work only for X but problem is that such 
 * functions will be needed by others enviroments.
 * ( for X it's make by settings "SDL_VIDEO_CENTERED" enviroment )
 */
int center_main_window_on_screen(void)
{
  SDL_SysWMinfo myinfo;
  SDL_VERSION(&myinfo.version);
  if (SDL_GetWMInfo(&myinfo) > 0)
  {
#ifdef WIN32_NATIVE
    
    /* Port ME - Write center window code with WinAPI instructions */
    
    return 0;
#else
    
#if 0 
    /* this code is for X and is only example what should be write to other 
       eviroments */
    Screen *defscr;
    Display *d = myinfo.info.x11.display;
    myinfo.info.x11.lock_func();
    defscr = DefaultScreenOfDisplay(d);
    XMoveWindow(d, myinfo.info.x11.wmwindow,
               (defscr->width - Main.screen->w) / 2,
               (defscr->height - Main.screen->h) / 2);
    myinfo.info.x11.unlock_func();
#endif    
    return 0;
#endif
  }
  return -1;
}



/**************************************************************************
  get pixel
  Return the pixel value at (x, y)
  NOTE: The surface must be locked before calling this!
**************************************************************************/
Uint32 getpixel(SDL_Surface * pSurface, Sint16 x, Sint16 y)
{
  if (!pSurface) return 0x0;
  switch (pSurface->format->BytesPerPixel) {
  case 1:
    return *(Uint8 *) ((Uint8 *) pSurface->pixels + y * pSurface->pitch + x);

  case 2:
    return *(Uint16 *) ((Uint8 *) pSurface->pixels + y * pSurface->pitch +
			(x << 1));

  case 3:
    {
      /* Here ptr is the address to the pixel we want to retrieve */
      Uint8 *ptr =
	  (Uint8 *) pSurface->pixels + y * pSurface->pitch + x * 3;
      if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
	return ptr[0] << 16 | ptr[1] << 8 | ptr[2];
      } else {
	return ptr[0] | ptr[1] << 8 | ptr[2] << 16;
      }
    }
  case 4:
    return *(Uint32 *) ((Uint8 *) pSurface->pixels + y * pSurface->pitch +
			(x << 2));

  default:
    return 0;			/* shouldn't happen, but avoids warnings */
  }
}

/**************************************************************************
  get first pixel
  Return the pixel value at (0, 0)
  NOTE: The surface must be locked before calling this!
**************************************************************************/
Uint32 get_first_pixel(SDL_Surface *pSurface)
{
  if (!pSurface) return 0;
  switch (pSurface->format->BytesPerPixel) {
  case 1:
    return *((Uint8 *)pSurface->pixels);

  case 2:
    return *((Uint16 *)pSurface->pixels);

  case 3:
    {
      if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
	return (((Uint8 *)pSurface->pixels)[0] << 16)|
		(((Uint8 *)pSurface->pixels)[1] << 8)|
			((Uint8 *)pSurface->pixels)[2];
      } else {
	return ((Uint8 *)pSurface->pixels)[0]|
		(((Uint8 *)pSurface->pixels)[1] << 8)|
			(((Uint8 *)pSurface->pixels)[2] << 16);
      }
    }
  case 4:
    return *((Uint32 *)pSurface->pixels);

  default:
    return 0;			/* shouldn't happen, but avoids warnings */
  }
}


/**************************************************************************
  Idea of "putline" code came from SDL_gfx lib. 
**************************************************************************/

void * my_memset8(void *dst_mem, Uint8 var, size_t lenght)
{
  return memset(dst_mem, var, lenght);
}

void * my_memset16(void *dst_mem, Uint16 var, size_t lenght)
{
  Uint16 *ptr = (Uint16 *)dst_mem;
  Uint32 color = (var << 16) | var;
#ifndef HAVE_MMX1  
  #ifndef ARM_WINCE
  DUFFS_LOOP_DOUBLE2(
  {
    *ptr++ = var;
  },{
    *(Uint32 *)ptr = color;  /* this statement causes an exception on my StrongARM-PDA */
    ptr += 2;
  }, lenght);
  #else
  DUFFS_LOOP_DOUBLE2(
  {
    *ptr++ = var;
  },{
    *ptr++ = var;
    *ptr++ = var;
  }, lenght);
  #endif
#else
  movd_m2r(color, mm0); /* color(0000CLCL) -> mm0 */
  punpckldq_r2r(mm0, mm0); /* CLCLCLCL -> mm0 */
  
  DUFFS_LOOP_QUATRO2(
  {
    *ptr++ = var;
  },{
    *(Uint32 *)ptr = color;
    ptr += 2;
  },{
    movq_r2m(mm0, *ptr); /* mm0 (CLCLCLCL) -> ptr */
    ptr += 4;
  }, lenght);
  emms();
#endif  
  return dst_mem;
}

void * my_memset24(void *dst_mem, Uint32 var, size_t lenght)
{
  Uint8 *ptr = (Uint8 *)dst_mem;
  
  var &= 0xFFFFFF;
  DUFFS_LOOP4(
  {
    memmove(ptr, &var, 3);
    ptr += 3;
  }, lenght);
  
  return dst_mem;
}

void * my_memset32(void *dst_mem, Uint32 var, size_t lenght)
{
  Uint32 *ptr = (Uint32 *)dst_mem;
#ifndef HAVE_MMX1  
  DUFFS_LOOP4({*ptr++ = var;}, lenght);
#else
  movd_m2r(var, mm0); /* color(0000CLCL) -> mm0 */
  punpckldq_r2r(mm0, mm0); /* CLCLCLCL -> mm0 */
  DUFFS_LOOP_DOUBLE2(
  {
    *ptr++ = var;
  },{
    movq_r2m(mm0, *ptr); /* mm0 (CLCLCLCL) -> ptr */
    ptr += 2;
  }, lenght);
  emms();
#endif  
  return dst_mem;
}

/**************************************************************************
  Draw Vertical line;
  NOTE: Befor call this func. check if 'x' is inside 'pDest' surface.
**************************************************************************/
static void put_vline(SDL_Surface * pDest, int x, Sint16 y0, Sint16 y1,
		      Uint32 color)
{
  register Uint8 *buf_ptr;
  int pitch;
  size_t lng;

  /* correct y0, y1 position ( must be inside 'pDest' surface ) */
  if (y0 < 0) {
    y0 = 0;
  } else {
    if (y0 >= pDest->h) {
      y0 = pDest->h - 1;
    }
  }

  if (y1 < 0) {
    y1 = 0;
  } else {
    if (y1 >= pDest->h) {
      y1 = pDest->h - 1;
    }
  }
  
  if (y1 - y0 < 0) {
    /* swap */
    pitch = y0;
    y0 = y1;
    y1 = pitch;
  }
  
  lng = y1 - y0;
  
  if (!lng) return;
    
  pitch = pDest->pitch;  
  buf_ptr = ((Uint8 *) pDest->pixels + (y0 * pitch));
  
  switch (pDest->format->BytesPerPixel) {
    case 1:
      buf_ptr += x;
      DUFFS_LOOP4(
      {
        *(Uint8 *) buf_ptr = color & 0xFF;
        buf_ptr += pitch;
      }, lng);
    return;
    case 2:
      buf_ptr += (x << 1);
      DUFFS_LOOP4(
      {
        *(Uint16 *) buf_ptr = color & 0xFFFF;
        buf_ptr += pitch;
      }, lng);
    return;
    case 3:
    {
      Uint8 c0, c1, c2;
      buf_ptr += (x << 1) + x;
    
      if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
        c0 = (color >> 16) & 0xff;
        c1 = (color >> 8) & 0xff;
        c2 = color & 0xff;
      } else {
        c0 = color & 0xff;
        c1 = (color >> 8) & 0xff;
        c2 = (color >> 16) & 0xff;
      }
      
      DUFFS_LOOP4(
      {
        buf_ptr[0] = c0;
        buf_ptr[1] = c1;
        buf_ptr[2] = c2;
        buf_ptr += pitch;
      }, lng);
    }
    return;
    case 4:
      buf_ptr += x << 2;
      DUFFS_LOOP4(
      {
        *(Uint32 *) buf_ptr = color;
        buf_ptr += pitch;
      }, lng);
    return;
    default:
      assert(0);
  }
}

/**************************************************************************
  Draw Horizontal line;
  NOTE: Befor call this func. check if 'y' is inside 'pDest' surface.
**************************************************************************/
static void put_hline(SDL_Surface * pDest, int y, Sint16 x0, Sint16 x1,
		      Uint32 color)
{
  Uint8 *buf_ptr;
  size_t lng;
  
  buf_ptr = ((Uint8 *) pDest->pixels + (y * pDest->pitch));
  
  /* correct x0, x1 position ( must be inside 'pDest' surface ) */
  if (x0 < 0) {
    x0 = 0;
  } else {
    if (x0 >= pDest->w) {
      x0 = pDest->w - 1;
    }
  }

  if (x1 < 0) {
    x1 = 0;
  } else {
    if (x1 >= pDest->w) {
      x1 = pDest->w - 1;
    }
  }

  if (x1 - x0 < 0) {
    /* swap */
    y = x0;
    x0 = x1;
    x1 = y;
  }
  
  lng = (x1 - x0) + 1;
  
  if (!lng) return;  
  
  switch (pDest->format->BytesPerPixel) {
    case 1:
      buf_ptr += x0;
      memset(buf_ptr, (color & 0xff), lng);
    return;
    case 2:
      buf_ptr += (x0 * 2);
      my_memset16(buf_ptr, (color & 0xffff), lng);
    return;
    case 3:
      buf_ptr += (x0 * 3);
      my_memset24(buf_ptr, (color & 0xffffff), lng);
    return;
    case 4:
      buf_ptr += (x0 * 4);
      my_memset32(buf_ptr, color, lng);
    return;
    default:
      assert(0);
  }
}

/**************************************************************************
  ...
**************************************************************************/
static void put_line(SDL_Surface * pDest, Sint16 x0, Sint16 y0, Sint16 x1,
		     Sint16 y1, Uint32 color)
{
  register int Bpp, pitch;
  register int x, y;
  int dx, dy;
  int sx, sy;
  int swaptmp;
  register Uint8 *pPixel;
  float m;
  int n;

  if (((x0 < 0) && (x1 < 0))
     || ((y0 < 0) && (y1 < 0))
     || ((x0 >= pDest->w) && (x1 >= pDest->w))
     || ((y0 >= pDest->h) && (y1 >= pDest->h))) {
    return;
  }
  
  /* correct x0, x1 position ( must be inside 'pDest' surface ) */
  if (x0 < 0) {
    m = (y1 - y0) / (x1 - x0);
    n = y1 - m * x1;
    x0 = 0;
    y0 = n;                        /* y0 = m * x0 + n; */
  } else if (x0 > pDest->w - 1) {
    m = (y1 - y0) / (x1 - x0);
    n = y1 - m * x1;
    x0 = pDest->w - 1;
    y0 = m * x0 + n;
  } else if (x1 < 0) {
    m = (y1 - y0) / (x1 - x0);
    n = y1 - m * x1;
    x1 = 0;
    y1 = n;                        /* y1 = m * x1 + n; */
  } else if (x1 > pDest->w - 1) {
    m = (y1 - y0) / (x1 - x0);
    n = y1 - m * x1;
    x1 = pDest->w - 1;
    y1 = m * x1 + n;
  }

  /* correct y0, y1 position ( must be inside 'pDest' surface ) */
  if (y0 < 0) {
    m = (x1 - x0) / (y1 - y0);
    n = x1 - m * y1;
    y0 = 0;
    x0 = n;                        /* x0 = m * y0 + n; */
  } else if (y0 > pDest->h - 1) {
    m = (x1 - x0) / (y1 - y0);
    n = x1 - m * y1;
    y0 = pDest->h - 1;
    x0 = m * y0 + n;    
  } else if (y1 < 0) {
    m = (x1 - x0) / (y1 - y0);
    n = x1 - m * y1;
    y1 = 0;
    x1 = n;                        /* x1 = m * y1 + n; */
  } else if (y1 > pDest->h - 1) {
    m = (x1 - x0) / (y1 - y0);
    n = x1 - m * y1;
    y1 = pDest->h - 1;
    x1 = m * y1 + n;
  }

  /* basic */
  dx = x1 - x0;
  dy = y1 - y0;
  sx = (dx >= 0) ? 1 : -1;
  sy = (dy >= 0) ? 1 : -1;

  /* advanced */
  dx = sx * dx + 1;
  dy = sy * dy + 1;
  Bpp = pDest->format->BytesPerPixel;
  pitch = pDest->pitch;

  pPixel = ((Uint8 *) pDest->pixels + (y0 * pitch) + (x0 * Bpp));

  Bpp *= sx;
  pitch *= sy;

  if (dx < dy) {
    swaptmp = dx;
    dx = dy;
    dy = swaptmp;

    swaptmp = Bpp;
    Bpp = pitch;
    pitch = swaptmp;
  }

  /* draw */
  x = 0;
  y = 0;
  switch (pDest->format->BytesPerPixel) {
  case 1:
    for (; x < dx; x++, pPixel += Bpp) {
      *pPixel = (color & 0xff);
      y += dy;
      if (y >= dx) {
	y -= dx;
	pPixel += pitch;
      }
    }
    return;
  case 2:
    for (; x < dx; x++, pPixel += Bpp) {
      *(Uint16 *) pPixel = (color & 0xffff);
      y += dy;
      if (y >= dx) {
	y -= dx;
	pPixel += pitch;
      }
    }
    return;
  case 3:
    for (; x < dx; x++, pPixel += Bpp) {
      if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
	pPixel[0] = (color >> 16) & 0xff;
	pPixel[1] = (color >> 8) & 0xff;
	pPixel[2] = color & 0xff;
      } else {
	pPixel[0] = (color & 0xff);
	pPixel[1] = (color >> 8) & 0xff;
	pPixel[2] = (color >> 16) & 0xff;
      }

      y += dy;
      if (y >= dx) {
	y -= dx;
	pPixel += pitch;
      }
    }
    return;
  case 4:
    for (; x < dx; x++, pPixel += Bpp) {
      *(Uint32 *) pPixel = color;
      y += dy;
      if (y >= dx) {
	y -= dx;
	pPixel += pitch;
      }
    }
    return;
  }

  return;
}

/**************************************************************************
  Draw line
**************************************************************************/
void putline(SDL_Surface * pDest, Sint16 x0, Sint16 y0,
	     Sint16 x1, Sint16 y1, Uint32 color)
{
  lock_surf(pDest);

  if (!(y1 - y0)) {		/* horizontal line */
    if ((y0 >= 0) && (y0 < pDest->h)) {
      put_hline(pDest, y0, x0, x1, color);
    }
    unlock_surf(pDest);
    return;
  }

  if ((x1 - x0)) {
    put_line(pDest, x0, y0, x1, y1, color);
  } else {			/* vertical line */
    if ((x0 >= 0) && (x0 < pDest->w)) {
      put_vline(pDest, x0, y0, y1, color);
    }
  }

  unlock_surf(pDest);
}

/**************************************************************************
  Draw frame line.
  x0,y0 - top left corrner.
  x1,y1 - botton right corrner.
**************************************************************************/
void putframe(SDL_Surface * pDest, Sint16 x0, Sint16 y0,
	      Sint16 x1, Sint16 y1, Uint32 color)
{
  lock_surf(pDest);

  if ((y0 >= 0) && (y0 < pDest->h)) {	/* top line */
    put_hline(pDest, y0, x0, x1, color);
  }

  if ((y1 >= 0) && (y1 < pDest->h)) {	/* botton line */
    put_hline(pDest, y1, x0, x1, color);
  }

  if ((x0 >= 0) && (x0 < pDest->w)) {
    put_vline(pDest, x0, y0, y1, color);	/* left line */
  }

  if ((x1 >= 0) && (x1 < pDest->w)) {
    put_vline(pDest, x1, y0, y1, color);	/* right line */
  }

  unlock_surf(pDest);
}

/* ===================================================================== */

/**************************************************************************
  initialize sdl with Flags
**************************************************************************/
void init_sdl(int iFlags)
{
  bool error;
  Main.screen = NULL;
  Main.guis = NULL;
  Main.gui = NULL;
  Main.map = NULL;
  Main.rects_count = 0;
  Main.guis_count = 0;

  if (SDL_WasInit(SDL_INIT_AUDIO)) {
    error = (SDL_InitSubSystem(iFlags) < 0);
  } else {
    error = (SDL_Init(iFlags) < 0);
  }
  if (error) {
    freelog(LOG_FATAL, _("Unable to initialize SDL library: %s"),
	    SDL_GetError());
    exit(1);
  }

  atexit(SDL_Quit);

  /* Initialize the TTF library */
  if (TTF_Init() < 0) {
    freelog(LOG_FATAL, _("Unable to initialize SDL_ttf library: %s"),
	    SDL_GetError());
    exit(2);
  }

  atexit(TTF_Quit);
}

/**************************************************************************
  free screen buffers
**************************************************************************/
void quit_sdl(void)
{
  FC_FREE(Main.guis);
  gui_layer_destroy(&Main.gui);
  FREESURFACE(Main.map);
}

/**************************************************************************
  Switch to passed video mode.
**************************************************************************/
int set_video_mode(int iWidth, int iHeight, int iFlags)
{
  /* find best bpp */
  int iDepth = SDL_GetVideoInfo()->vfmt->BitsPerPixel;

  /*  if screen does exist check if this is mayby
     exactly the same resolution then return 1 */
  if (Main.screen) {
    if (Main.screen->w == iWidth && Main.screen->h == iHeight)
      if ((Main.screen->flags & SDL_FULLSCREEN)
	  && (iFlags & SDL_FULLSCREEN))
	return 1;
  }

  /* Check to see if a particular video mode is supported */
  if ((iDepth = SDL_VideoModeOK(iWidth, iHeight, iDepth, iFlags)) == 0) {
    freelog(LOG_ERROR, _("No available mode for this resolution "
			 ": %d x %d %d bpp"), iWidth, iHeight, iDepth);

    freelog(LOG_DEBUG, _("Setting default resolution to : "
    					"640 x 480 16 bpp SW"));

    Main.screen = SDL_SetVideoMode(640, 480, 16, SDL_SWSURFACE);
  } else { /* set video mode */
    if ((Main.screen = SDL_SetVideoMode(iWidth, iHeight,
					iDepth, iFlags)) == NULL) {
    freelog(LOG_ERROR, _("Unable to set this resolution: "
			 "%d x %d %d bpp %s"),
	    		iWidth, iHeight, iDepth, SDL_GetError());

    exit(-30);
  }

  freelog(LOG_DEBUG, _("Setting resolution to: %d x %d %d bpp"),
	  					iWidth, iHeight, iDepth);
  }

  FREESURFACE(Main.map);
  Main.map = SDL_DisplayFormat(Main.screen);
  
  if (Main.gui) {
    FREESURFACE(Main.gui->surface);
    Main.gui->surface = create_surf_alpha(Main.screen->w, Main.screen->h, SDL_SWSURFACE);
  } else {
    Main.gui = add_gui_layer(Main.screen->w, Main.screen->h);
  }
  
  clear_surface(Main.gui->surface, NULL);
 
  return 0;
}

/**************************************************************************
                           Fill Rect with RGBA color
**************************************************************************/
#define MASK565	0xf7de
#define MASK555	0xfbde

/* 50% alpha (128) */
#define BLEND16_50( d, s , mask )	\
	(((( s & mask ) + ( d & mask )) >> 1) + ( s & d & ( ~mask & 0xffff)))

#define BLEND2x16_50( d, s , mask )	\
	(((( s & (mask | mask << 16)) + ( d & ( mask | mask << 16 ))) >> 1) + \
	( s & d & ( ~(mask | mask << 16))))

#ifdef HAVE_MMX1

/**************************************************************************
  ..
**************************************************************************/
static int __FillRectAlpha555(SDL_Surface * pSurface, SDL_Rect * pRect,
			      SDL_Color * pColor)
{
  register Uint32 D, S =
      SDL_MapRGB(pSurface->format, pColor->r, pColor->g, pColor->b);
  register Uint32 A = pColor->unused >> 3;
  Uint8 load[8];
  Uint32 y, end;
  Uint16 *start, *pixel;
  			
  S &= 0xFFFF;
  
  *(Uint64 *)load = pColor->unused;
  movq_m2r(*load, mm0); /* alpha -> mm0 */
  punpcklwd_r2r(mm0, mm0); /* 00000A0A -> mm0 */
  punpckldq_r2r(mm0, mm0); /* 0A0A0A0A -> mm0 */
  
  *(Uint64 *)load = S;
  movq_m2r(*load, mm2); /* src(000000CL) -> mm2 */
  punpcklwd_r2r(mm2, mm2); /* 0000CLCL -> mm2 */
  punpckldq_r2r(mm2, mm2); /* CLCLCLCL -> mm2 */
  
  /* Setup the color channel masks */
  *(Uint64 *)load = 0x7C007C007C007C00;
  movq_m2r(*load, mm1); /* MASKRED -> mm1 */
  *(Uint64 *)load = 0x03E003E003E003E0;
  movq_m2r(*load, mm4); /* MASKGREEN -> mm4 */
  *(Uint64 *)load = 0x001F001F001F001F;
  movq_m2r(*load, mm7); /* MASKBLUE -> mm7 */
  
  lock_surf(pSurface);
  
  if (pRect == NULL) {
    end = pSurface->w * pSurface->h;
    pixel = (Uint16 *) pSurface->pixels;
    if (A == 16) {		/* A == 128 >> 3 */
      S = S | S << 16;
      *(Uint64 *)load = MASK555;
      movq_m2r(*load, mm0); /* mask(000000MS) -> mm0 */
      punpcklwd_r2r(mm0, mm0); /* 0000MSMS -> mm0 */
      punpcklwd_r2r(mm0, mm0); /* MSMSMSMS -> mm0 */
      movq_r2r(mm0, mm1); /* mask -> mm1 */
      *(Uint64 *)load = 0xFFFFFFFFFFFFFFFF;
      movq_m2r(*load, mm7);
      pxor_r2r(mm7, mm1); /* !mask -> mm1 */
      DUFFS_LOOP_QUATRO2(
      {
	D = *pixel;
	*pixel++ = BLEND16_50(D, (S & 0xFFFF), MASK555);
      },{
	D = *(Uint32 *) pixel;
	*(Uint32 *) pixel = BLEND2x16_50(D, S, MASK555);
	pixel += 2;
      },{
	movq_m2r((*pixel), mm3);/* load 4 dst pixels -> mm2 */
	movq_r2r(mm3, mm5); /* dst -> mm5 */
	movq_r2r(mm2, mm4); /* src -> mm4 */
	
	pand_r2r(mm0, mm5); /* dst & mask -> mm1 */
	pand_r2r(mm0, mm4); /* src & mask -> mm4 */
	paddw_r2r(mm5, mm4); /* mm5 + mm4 -> mm4 */
	psrlq_i2r(1, mm4); /* mm4 >> 1 -> mm4 */
	
	pand_r2r(mm2, mm3); /* src & dst -> mm3 */
	pand_r2r(mm1, mm3); /* mm3 & !mask -> mm3 */
	paddw_r2r(mm4, mm3); /* mm3 + mm4 -> mm4 */
	movq_r2m(mm3, (*pixel));/* mm3 -> 4 dst pixels */
	pixel += 4;
      }, end);
      emms();
    } else {
      S = (S | S << 16) & 0x03e07c1f;
      DUFFS_LOOP_QUATRO2(
      {
	D = *pixel;
	D = (D | D << 16) & 0x03e07c1f;
	D += (S - D) * A >> 5;
	D &= 0x03e07c1f;
	*pixel++ = (D | (D >> 16)) & 0xFFFF;
      },{
	D = *pixel;
	D = (D | D << 16) & 0x03e07c1f;
	D += (S - D) * A >> 5;
	D &= 0x03e07c1f;
	*pixel++ = (D | (D >> 16)) & 0xFFFF;
	D = *pixel;
	D = (D | D << 16) & 0x03e07c1f;
	D += (S - D) * A >> 5;
	D &= 0x03e07c1f;
	*pixel++ = (D | (D >> 16)) & 0xFFFF;
      },{
	movq_m2r((*pixel), mm3);/* load 4 dst pixels -> mm2 */
	
	/* RED */
	movq_r2r(mm2, mm5); /* src -> mm5 */
	pand_r2r(mm1 , mm5); /* src & MASKRED -> mm5 */
	psrlq_i2r(10, mm5); /* mm5 >> 10 -> mm5 [000r 000r 000r 000r] */
	
	movq_r2r(mm3, mm6); /* dst -> mm6 */
	pand_r2r(mm1 , mm6); /* dst & MASKRED -> mm6 */
	psrlq_i2r(10, mm6); /* mm6 >> 10 -> mm6 [000r 000r 000r 000r] */
	
	/* blend */
	psubw_r2r(mm6, mm5);/* src - dst -> mm5 */
	pmullw_r2r(mm0, mm5); /* mm5 * alpha -> mm5 */
	psrlw_i2r(8, mm5); /* mm5 >> 8 -> mm5 */
	paddw_r2r(mm5, mm6); /* mm1 + mm2(dst) -> mm2 */
	psllq_i2r(10, mm6); /* mm6 << 10 -> mm6 */
	pand_r2r(mm1, mm6); /* mm6 & MASKRED -> mm6 */
	
	movq_r2r(mm4, mm5); /* MASKGREEN -> mm5 */
	por_r2r(mm7, mm5);  /* MASKBLUE | mm5 -> mm5 */
	pand_r2r(mm5, mm3); /* mm3 & mm5(!MASKRED) -> mm3 */
	por_r2r(mm6, mm3);  /* save new reds in dsts */
	
	/* green */
	movq_r2r(mm2, mm5); /* src -> mm5 */
	pand_r2r(mm4 , mm5); /* src & MASKGREEN -> mm5 */
	psrlq_i2r(5, mm5); /* mm5 >> 5 -> mm5 [000g 000g 000g 000g] */
	
	movq_r2r(mm3, mm6); /* dst -> mm6 */
	pand_r2r(mm4 , mm6); /* dst & MASKGREEN -> mm6 */
	psrlq_i2r(5, mm6); /* mm6 >> 5 -> mm6 [000g 000g 000g 000g] */
	
	/* blend */
	psubw_r2r(mm6, mm5);/* src - dst -> mm5 */
	pmullw_r2r(mm0, mm5); /* mm5 * alpha -> mm5 */
	psrlw_i2r(8, mm5); /* mm5 >> 8 -> mm5 */
	paddw_r2r(mm5, mm6); /* mm1 + mm2(dst) -> mm2 */
	psllq_i2r(5, mm6); /* mm6 << 5 -> mm6 */
	pand_r2r(mm4, mm6); /* mm6 & MASKGREEN -> mm6 */
	
	movq_r2r(mm1, mm5); /* MASKRED -> mm5 */
	por_r2r(mm7, mm5);  /* MASKBLUE | mm5 -> mm5 */
	pand_r2r(mm5, mm3); /* mm3 & mm5(!MASKGREEN) -> mm3 */
	por_r2r(mm6, mm3); /* save new greens in dsts */
	
	/* blue */
	movq_r2r(mm2, mm5); /* src -> mm5 */
	pand_r2r(mm7 , mm5); /* src & MASKRED -> mm5[000b 000b 000b 000b] */
		
	movq_r2r(mm3, mm6); /* dst -> mm6 */
	pand_r2r(mm7 , mm6); /* dst & MASKBLUE -> mm6[000b 000b 000b 000b] */
	
	/* blend */
	psubw_r2r(mm6, mm5);/* src - dst -> mm5 */
	pmullw_r2r(mm0, mm5); /* mm5 * alpha -> mm5 */
	psrlw_i2r(8, mm5); /* mm5 >> 8 -> mm5 */
	paddw_r2r(mm5, mm6); /* mm1 + mm2(dst) -> mm2 */
	pand_r2r(mm7, mm6); /* mm6 & MASKBLUE -> mm6 */
	
	movq_r2r(mm1, mm5); /* MASKRED -> mm5 */
	por_r2r(mm4, mm5);  /* MASKGREEN | mm5 -> mm5 */
	pand_r2r(mm5, mm3); /* mm3 & mm5(!MASKBLUE) -> mm3 */
	por_r2r(mm6, mm3); /* save new blues in dsts */
	
	movq_r2m(mm3, *pixel);/* mm2 -> 4 dst pixels */
	
	pixel += 4;
	
      }, end);
      emms();
    }
  } else {
    /* correct pRect size */
    if (pRect->x < 0) {
      pRect->w += pRect->x;
      pRect->x = 0;
    } else {
      if (pRect->x >= pSurface->w - pRect->w) {
	pRect->w = pSurface->w - pRect->x;
      }
    }

    if (pRect->y < 0) {
      pRect->h += pRect->y;
      pRect->y = 0;
    } else {
      if (pRect->y >= pSurface->h - pRect->h) {
	pRect->h = pSurface->h - pRect->y;
      }
    }

    start = pixel = (Uint16 *) pSurface->pixels +
	(pRect->y * (pSurface->pitch >> 1)) + pRect->x;

    if (A == 16) {		/* A == 128 >> 3 */
      S = S | S << 16;
      y = pRect->h;
      end = pRect->w;
      *(Uint64 *)load = MASK555;
      movq_m2r(*load, mm0); /* mask(000000MS) -> mm0 */
      punpcklwd_r2r(mm0, mm0); /* 0000MSMS -> mm0 */
      punpcklwd_r2r(mm0, mm0); /* MSMSMSMS -> mm0 */
      movq_r2r(mm0, mm1); /* mask -> mm1 */
      pandn_r2r(mm1, mm1); /* !mask -> mm1 */
      while(y--) {	
        DUFFS_LOOP_QUATRO2(
        {
	  D = *pixel;
 	  *pixel++ = BLEND16_50(D, (S & 0xFFFF), MASK555);
        },{
	  D = *(Uint32 *) pixel;
	  *(Uint32 *) pixel = BLEND2x16_50(D, S, MASK555);
	  pixel += 2;
        },{
	  movq_m2r((*pixel), mm3);/* load 4 dst pixels -> mm2 */
	  movq_r2r(mm3, mm5); /* dst -> mm5 */
	  movq_r2r(mm2, mm4); /* src -> mm4 */
	
	  pand_r2r(mm0, mm5); /* dst & mask -> mm1 */
	  pand_r2r(mm0, mm4); /* src & mask -> mm4 */
	  paddd_r2r(mm5, mm4); /* mm5 + mm4 -> mm4 */ /*w*/
	  psrld_i2r(1, mm4); /* mm4 >> 1 -> mm4 */ /*q*/
	
	  pand_r2r(mm2, mm3); /* src & dst -> mm3 */
	  pand_r2r(mm1, mm3); /* mm3 & !mask -> mm3 */
	  paddd_r2r(mm4, mm3); /* mm3 + mm4 -> mm4 */ /*w*/
	  movq_r2m(mm3, (*pixel));/* mm3 -> 4 dst pixels */
	  pixel += 4;
        }, end);
	pixel = start + (pSurface->pitch >> 1);
	start = pixel;
      }/* while */
      emms();
    } else {
      S = (S | S << 16) & 0x03e07c1f;
      y = pRect->h;
      end = pRect->w;
      
      while(y--) {
	DUFFS_LOOP_QUATRO2(
        {
	  D = *pixel;
	  D = (D | D << 16) & 0x03e07c1f;
	  D += (S - D) * A >> 5;
	  D &= 0x03e07c1f;
	  *pixel++ = (D | (D >> 16)) & 0xFFFF;
        },{
	  D = *pixel;
	  D = (D | D << 16) & 0x03e07c1f;
	  D += (S - D) * A >> 5;
	  D &= 0x03e07c1f;
	  *pixel++ = (D | (D >> 16)) & 0xFFFF;
	  D = *pixel;
	  D = (D | D << 16) & 0x03e07c1f;
	  D += (S - D) * A >> 5;
	  D &= 0x03e07c1f;
	  *pixel++ = (D | (D >> 16)) & 0xFFFF;
        },{
	  movq_m2r((*pixel), mm3);/* load 4 dst pixels -> mm2 */
	
	  /* RED */
	  movq_r2r(mm2, mm5); /* src -> mm5 */
	  pand_r2r(mm1 , mm5); /* src & MASKRED -> mm5 */
	  psrlq_i2r(10, mm5); /* mm5 >> 10 -> mm5 [000r 000r 000r 000r] */
	
	  movq_r2r(mm3, mm6); /* dst -> mm6 */
	  pand_r2r(mm1 , mm6); /* dst & MASKRED -> mm6 */
	  psrlq_i2r(10, mm6); /* mm6 >> 10 -> mm6 [000r 000r 000r 000r] */
	
	  /* blend */
	  psubw_r2r(mm6, mm5);/* src - dst -> mm5 */
	  pmullw_r2r(mm0, mm5); /* mm5 * alpha -> mm5 */
	  psrlw_i2r(8, mm5); /* mm5 >> 8 -> mm5 */
	  paddw_r2r(mm5, mm6); /* mm1 + mm2(dst) -> mm2 */
	  psllq_i2r(10, mm6); /* mm6 << 10 -> mm6 */
	  pand_r2r(mm1, mm6); /* mm6 & MASKRED -> mm6 */
	
	  movq_r2r(mm4, mm5); /* MASKGREEN -> mm5 */
	  por_r2r(mm7, mm5);  /* MASKBLUE | mm5 -> mm5 */
	  pand_r2r(mm5, mm3); /* mm3 & mm5(!MASKRED) -> mm3 */
	  por_r2r(mm6, mm3);  /* save new reds in dsts */
	
	  /* green */
	  movq_r2r(mm2, mm5); /* src -> mm5 */
	  pand_r2r(mm4 , mm5); /* src & MASKGREEN -> mm5 */
	  psrlq_i2r(5, mm5); /* mm5 >> 5 -> mm5 [000g 000g 000g 000g] */
	
	  movq_r2r(mm3, mm6); /* dst -> mm6 */
	  pand_r2r(mm4 , mm6); /* dst & MASKGREEN -> mm6 */
	  psrlq_i2r(5, mm6); /* mm6 >> 5 -> mm6 [000g 000g 000g 000g] */
	
	  /* blend */
	  psubw_r2r(mm6, mm5);/* src - dst -> mm5 */
	  pmullw_r2r(mm0, mm5); /* mm5 * alpha -> mm5 */
	  psrlw_i2r(8, mm5); /* mm5 >> 8 -> mm5 */
	  paddw_r2r(mm5, mm6); /* mm1 + mm2(dst) -> mm2 */
	  psllq_i2r(5, mm6); /* mm6 << 5 -> mm6 */
	  pand_r2r(mm4, mm6); /* mm6 & MASKGREEN -> mm6 */
	
	  movq_r2r(mm1, mm5); /* MASKRED -> mm5 */
	  por_r2r(mm7, mm5);  /* MASKBLUE | mm5 -> mm5 */
	  pand_r2r(mm5, mm3); /* mm3 & mm5(!MASKGREEN) -> mm3 */
	  por_r2r(mm6, mm3); /* save new greens in dsts */
	
	  /* blue */
	  movq_r2r(mm2, mm5); /* src -> mm5 */
	  pand_r2r(mm7 , mm5); /* src & MASKRED -> mm5[000b 000b 000b 000b] */
		
	  movq_r2r(mm3, mm6); /* dst -> mm6 */
	  pand_r2r(mm7 , mm6); /* dst & MASKBLUE -> mm6[000b 000b 000b 000b] */
	
	  /* blend */
	  psubw_r2r(mm6, mm5);/* src - dst -> mm5 */
	  pmullw_r2r(mm0, mm5); /* mm5 * alpha -> mm5 */
	  psrlw_i2r(8, mm5); /* mm5 >> 8 -> mm5 */
	  paddw_r2r(mm5, mm6); /* mm1 + mm2(dst) -> mm2 */
	  pand_r2r(mm7, mm6); /* mm6 & MASKBLUE -> mm6 */
	
	  movq_r2r(mm1, mm5); /* MASKRED -> mm5 */
	  por_r2r(mm4, mm5);  /* MASKGREEN | mm5 -> mm5 */
	  pand_r2r(mm5, mm3); /* mm3 & mm5(!MASKBLUE) -> mm3 */
	  por_r2r(mm6, mm3); /* save new blues in dsts */
	
	  movq_r2m(mm3, *pixel);/* mm2 -> 4 dst pixels */
	
	  pixel += 4;
	
        }, end);
      
        pixel = start + (pSurface->pitch >> 1);
        start = pixel;
      } /* while */
      emms();
    }

  }
  
  unlock_surf(pSurface);
  return 0;
}

/**************************************************************************
  ..
**************************************************************************/
static int __FillRectAlpha565(SDL_Surface * pSurface, SDL_Rect * pRect,
			      SDL_Color * pColor)
{
  register Uint32 D, S =
      SDL_MapRGB(pSurface->format, pColor->r, pColor->g, pColor->b);
  register Uint32 A = pColor->unused >> 3;
  Uint8 load[8];
  Uint32 y, end;
  Uint16 *start, *pixel;
  			
  S &= 0xFFFF;
  
  *(Uint64 *)load = pColor->unused;
  movq_m2r(*load, mm0); /* alpha -> mm0 */
  punpcklwd_r2r(mm0, mm0); /* 00000A0A -> mm0 */
  punpckldq_r2r(mm0, mm0); /* 0A0A0A0A -> mm0 */
  
  *(Uint64 *)load = S;
  movq_m2r(*load, mm2); /* src(000000CL) -> mm2 */
  punpcklwd_r2r(mm2, mm2); /* 0000CLCL -> mm2 */
  punpckldq_r2r(mm2, mm2); /* CLCLCLCL -> mm2 */
  
  /* Setup the color channel masks */
  *(Uint64 *)load = 0xF800F800F800F800;
  movq_m2r(*load, mm1); /* MASKRED -> mm1 */
  *(Uint64 *)load = 0x07E007E007E007E0;
  movq_m2r(*load, mm4); /* MASKGREEN -> mm4 */
  *(Uint64 *)load = 0x001F001F001F001F;
  movq_m2r(*load, mm7); /* MASKBLUE -> mm7 */
  
  lock_surf(pSurface);
  
  if (pRect == NULL) {
    end = pSurface->w * pSurface->h;
    pixel = (Uint16 *) pSurface->pixels;
    if (A == 16) {		/* A == 128 >> 3 */
      S = S | S << 16;
      *(Uint64 *)load = MASK565;
      movq_m2r(*load, mm0); /* mask(000000MS) -> mm0 */
      punpcklwd_r2r(mm0, mm0); /* 0000MSMS -> mm0 */
      punpcklwd_r2r(mm0, mm0); /* MSMSMSMS -> mm0 */
      movq_r2r(mm0, mm1); /* mask -> mm1 */
      *(Uint64 *)load = 0xFFFFFFFFFFFFFFFF;
      movq_m2r(*load, mm7);
      pxor_r2r(mm7, mm1); /* !mask -> mm1 */
      DUFFS_LOOP_QUATRO2(
      {
	D = *pixel;
	*pixel++ = BLEND16_50(D, (S & 0xFFFF), MASK565);
      },{
	D = *(Uint32 *) pixel;
	*(Uint32 *) pixel = BLEND2x16_50(D, S, MASK565);
	pixel += 2;
      },{
	movq_m2r((*pixel), mm3);/* load 4 dst pixels -> mm2 */
	movq_r2r(mm3, mm5); /* dst -> mm5 */
	movq_r2r(mm2, mm4); /* src -> mm4 */
	
	pand_r2r(mm0, mm5); /* dst & mask -> mm1 */
	pand_r2r(mm0, mm4); /* src & mask -> mm4 */
	paddw_r2r(mm5, mm4); /* mm5 + mm4 -> mm4 */
	psrlq_i2r(1, mm4); /* mm4 >> 1 -> mm4 */
	
	pand_r2r(mm2, mm3); /* src & dst -> mm3 */
	pand_r2r(mm1, mm3); /* mm3 & !mask -> mm3 */
	paddw_r2r(mm4, mm3); /* mm3 + mm4 -> mm4 */
	movq_r2m(mm3, (*pixel));/* mm3 -> 4 dst pixels */
	pixel += 4;
      }, end);
      emms();
    } else {
      S = (S | S << 16) & 0x07e0f81f;
      DUFFS_LOOP_QUATRO2(
      {
	D = *pixel;
	D = (D | D << 16) & 0x07e0f81f;
	D += (S - D) * A >> 5;
	D &= 0x07e0f81f;
	*pixel++ = (D | (D >> 16)) & 0xFFFF;
      },{
	D = *pixel;
	D = (D | D << 16) & 0x07e0f81f;
	D += (S - D) * A >> 5;
	D &= 0x07e0f81f;
	*pixel++ = (D | (D >> 16)) & 0xFFFF;
	D = *pixel;
	D = (D | D << 16) & 0x07e0f81f;
	D += (S - D) * A >> 5;
	D &= 0x07e0f81f;
	*pixel++ = (D | (D >> 16)) & 0xFFFF;
      },{
	movq_m2r((*pixel), mm3);/* load 4 dst pixels -> mm2 */
	
	/* RED */
	movq_r2r(mm2, mm5); /* src -> mm5 */
	pand_r2r(mm1 , mm5); /* src & MASKRED -> mm5 */
	psrlq_i2r(11, mm5); /* mm5 >> 11 -> mm5 [000r 000r 000r 000r] */
	
	movq_r2r(mm3, mm6); /* dst -> mm6 */
	pand_r2r(mm1 , mm6); /* dst & MASKRED -> mm6 */
	psrlq_i2r(11, mm6); /* mm6 >> 11 -> mm6 [000r 000r 000r 000r] */
	
	/* blend */
	psubw_r2r(mm6, mm5);/* src - dst -> mm5 */
	pmullw_r2r(mm0, mm5); /* mm5 * alpha -> mm5 */
	psrlw_i2r(8, mm5); /* mm5 >> 8 -> mm5 */
	paddw_r2r(mm5, mm6); /* mm1 + mm2(dst) -> mm2 */
	psllq_i2r(11, mm6); /* mm6 << 11 -> mm6 */
	pand_r2r(mm1, mm6); /* mm6 & MASKRED -> mm6 */
	
	movq_r2r(mm4, mm5); /* MASKGREEN -> mm5 */
	por_r2r(mm7, mm5);  /* MASKBLUE | mm5 -> mm5 */
	pand_r2r(mm5, mm3); /* mm3 & mm5(!MASKRED) -> mm3 */
	por_r2r(mm6, mm3);  /* save new reds in dsts */
	
	/* green */
	movq_r2r(mm2, mm5); /* src -> mm5 */
	pand_r2r(mm4 , mm5); /* src & MASKGREEN -> mm5 */
	psrlq_i2r(5, mm5); /* mm5 >> 5 -> mm5 [000g 000g 000g 000g] */
	
	movq_r2r(mm3, mm6); /* dst -> mm6 */
	pand_r2r(mm4 , mm6); /* dst & MASKGREEN -> mm6 */
	psrlq_i2r(5, mm6); /* mm6 >> 5 -> mm6 [000g 000g 000g 000g] */
	
	/* blend */
	psubw_r2r(mm6, mm5);/* src - dst -> mm5 */
	pmullw_r2r(mm0, mm5); /* mm5 * alpha -> mm5 */
	psrlw_i2r(8, mm5); /* mm5 >> 8 -> mm5 */
	paddw_r2r(mm5, mm6); /* mm1 + mm2(dst) -> mm2 */
	psllq_i2r(5, mm6); /* mm6 << 5 -> mm6 */
	pand_r2r(mm4, mm6); /* mm6 & MASKGREEN -> mm6 */
	
	movq_r2r(mm1, mm5); /* MASKRED -> mm5 */
	por_r2r(mm7, mm5);  /* MASKBLUE | mm5 -> mm5 */
	pand_r2r(mm5, mm3); /* mm3 & mm5(!MASKGREEN) -> mm3 */
	por_r2r(mm6, mm3); /* save new greens in dsts */
	
	/* blue */
	movq_r2r(mm2, mm5); /* src -> mm5 */
	pand_r2r(mm7 , mm5); /* src & MASKRED -> mm5[000b 000b 000b 000b] */
		
	movq_r2r(mm3, mm6); /* dst -> mm6 */
	pand_r2r(mm7 , mm6); /* dst & MASKBLUE -> mm6[000b 000b 000b 000b] */
	
	/* blend */
	psubw_r2r(mm6, mm5);/* src - dst -> mm5 */
	pmullw_r2r(mm0, mm5); /* mm5 * alpha -> mm5 */
	psrlw_i2r(8, mm5); /* mm5 >> 8 -> mm5 */
	paddw_r2r(mm5, mm6); /* mm1 + mm2(dst) -> mm2 */
	pand_r2r(mm7, mm6); /* mm6 & MASKBLUE -> mm6 */
	
	movq_r2r(mm1, mm5); /* MASKRED -> mm5 */
	por_r2r(mm4, mm5);  /* MASKGREEN | mm5 -> mm5 */
	pand_r2r(mm5, mm3); /* mm3 & mm5(!MASKBLUE) -> mm3 */
	por_r2r(mm6, mm3); /* save new blues in dsts */
	
	movq_r2m(mm3, *pixel);/* mm2 -> 4 dst pixels */
	
	pixel += 4;
	
      }, end);
      emms();
    }
  } else {
    /* correct pRect size */
    if (pRect->x < 0) {
      pRect->w += pRect->x;
      pRect->x = 0;
    } else {
      if (pRect->x >= pSurface->w - pRect->w) {
	pRect->w = pSurface->w - pRect->x;
      }
    }

    if (pRect->y < 0) {
      pRect->h += pRect->y;
      pRect->y = 0;
    } else {
      if (pRect->y >= pSurface->h - pRect->h) {
	pRect->h = pSurface->h - pRect->y;
      }
    }

    start = pixel = (Uint16 *) pSurface->pixels +
	(pRect->y * (pSurface->pitch >> 1)) + pRect->x;

    if (A == 16) {		/* A == 128 >> 3 */
      S = S | S << 16;
      y = pRect->h;
      end = pRect->w;
      *(Uint64 *)load = MASK565;
      movq_m2r(*load, mm0); /* mask(000000MS) -> mm0 */
      punpcklwd_r2r(mm0, mm0); /* 0000MSMS -> mm0 */
      punpcklwd_r2r(mm0, mm0); /* MSMSMSMS -> mm0 */
      movq_r2r(mm0, mm1); /* mask -> mm1 */
      pandn_r2r(mm1, mm1); /* !mask -> mm1 */
      while(y--) {	
        DUFFS_LOOP_QUATRO2(
        {
	  D = *pixel;
 	  *pixel++ = BLEND16_50(D, (S & 0xFFFF), MASK565);
        },{
	  D = *(Uint32 *) pixel;
	  *(Uint32 *) pixel = BLEND2x16_50(D, S, MASK565);
	  pixel += 2;
        },{
	  movq_m2r((*pixel), mm3);/* load 4 dst pixels -> mm2 */
	  movq_r2r(mm3, mm5); /* dst -> mm5 */
	  movq_r2r(mm2, mm4); /* src -> mm4 */
	
	  pand_r2r(mm0, mm5); /* dst & mask -> mm1 */
	  pand_r2r(mm0, mm4); /* src & mask -> mm4 */
	  paddd_r2r(mm5, mm4); /* mm5 + mm4 -> mm4 */ /*w*/
	  psrld_i2r(1, mm4); /* mm4 >> 1 -> mm4 */ /*q*/
	
	  pand_r2r(mm2, mm3); /* src & dst -> mm3 */
	  pand_r2r(mm1, mm3); /* mm3 & !mask -> mm3 */
	  paddd_r2r(mm4, mm3); /* mm3 + mm4 -> mm4 */ /*w*/
	  movq_r2m(mm3, (*pixel));/* mm3 -> 4 dst pixels */
	  pixel += 4;
        }, end);
	pixel = start + (pSurface->pitch >> 1);
	start = pixel;
      }/* while */
      emms();
    } else {
      S = (S | S << 16) & 0x07e0f81f;
      y = pRect->h;
      end = pRect->w;
      
      while(y--) {
	DUFFS_LOOP_QUATRO2(
        {
	  D = *pixel;
	  D = (D | D << 16) & 0x07e0f81f;
	  D += (S - D) * A >> 5;
	  D &= 0x07e0f81f;
	  *pixel++ = (D | (D >> 16)) & 0xFFFF;
        },{
	  D = *pixel;
	  D = (D | D << 16) & 0x07e0f81f;
	  D += (S - D) * A >> 5;
	  D &= 0x07e0f81f;
	  *pixel++ = (D | (D >> 16)) & 0xFFFF;
	  D = *pixel;
	  D = (D | D << 16) & 0x07e0f81f;
	  D += (S - D) * A >> 5;
	  D &= 0x07e0f81f;
	  *pixel++ = (D | (D >> 16)) & 0xFFFF;
        },{
	  movq_m2r((*pixel), mm3);/* load 4 dst pixels -> mm2 */
	
	  /* RED */
	  movq_r2r(mm2, mm5); /* src -> mm5 */
	  pand_r2r(mm1 , mm5); /* src & MASKRED -> mm5 */
	  psrlq_i2r(11, mm5); /* mm5 >> 11 -> mm5 [000r 000r 000r 000r] */
	
	  movq_r2r(mm3, mm6); /* dst -> mm6 */
	  pand_r2r(mm1 , mm6); /* dst & MASKRED -> mm6 */
	  psrlq_i2r(11, mm6); /* mm6 >> 11 -> mm6 [000r 000r 000r 000r] */
	
	  /* blend */
	  psubw_r2r(mm6, mm5);/* src - dst -> mm5 */
	  pmullw_r2r(mm0, mm5); /* mm5 * alpha -> mm5 */
	  psrlw_i2r(8, mm5); /* mm5 >> 8 -> mm5 */
	  paddw_r2r(mm5, mm6); /* mm1 + mm2(dst) -> mm2 */
	  psllq_i2r(11, mm6); /* mm6 << 11 -> mm6 */
	  pand_r2r(mm1, mm6); /* mm6 & MASKRED -> mm6 */
	
	  movq_r2r(mm4, mm5); /* MASKGREEN -> mm5 */
	  por_r2r(mm7, mm5);  /* MASKBLUE | mm5 -> mm5 */
	  pand_r2r(mm5, mm3); /* mm3 & mm5(!MASKRED) -> mm3 */
	  por_r2r(mm6, mm3);  /* save new reds in dsts */
	
	  /* green */
	  movq_r2r(mm2, mm5); /* src -> mm5 */
	  pand_r2r(mm4 , mm5); /* src & MASKGREEN -> mm5 */
	  psrlq_i2r(5, mm5); /* mm5 >> 11 -> mm5 [000g 000g 000g 000g] */
	
	  movq_r2r(mm3, mm6); /* dst -> mm6 */
	  pand_r2r(mm4 , mm6); /* dst & MASKGREEN -> mm6 */
	  psrlq_i2r(5, mm6); /* mm6 >> 11 -> mm6 [000g 000g 000g 000g] */
	
	  /* blend */
	  psubw_r2r(mm6, mm5);/* src - dst -> mm5 */
	  pmullw_r2r(mm0, mm5); /* mm5 * alpha -> mm5 */
	  psrlw_i2r(8, mm5); /* mm5 >> 8 -> mm5 */
	  paddw_r2r(mm5, mm6); /* mm1 + mm2(dst) -> mm2 */
	  psllq_i2r(5, mm6); /* mm6 << 5 -> mm6 */
	  pand_r2r(mm4, mm6); /* mm6 & MASKGREEN -> mm6 */
	
	  movq_r2r(mm1, mm5); /* MASKRED -> mm5 */
	  por_r2r(mm7, mm5);  /* MASKBLUE | mm5 -> mm5 */
	  pand_r2r(mm5, mm3); /* mm3 & mm5(!MASKGREEN) -> mm3 */
	  por_r2r(mm6, mm3); /* save new greens in dsts */
	
	  /* blue */
	  movq_r2r(mm2, mm5); /* src -> mm5 */
	  pand_r2r(mm7 , mm5); /* src & MASKRED -> mm5[000b 000b 000b 000b] */
		
	  movq_r2r(mm3, mm6); /* dst -> mm6 */
	  pand_r2r(mm7 , mm6); /* dst & MASKBLUE -> mm6[000b 000b 000b 000b] */
	
	  /* blend */
	  psubw_r2r(mm6, mm5);/* src - dst -> mm5 */
	  pmullw_r2r(mm0, mm5); /* mm5 * alpha -> mm5 */
	  psrlw_i2r(8, mm5); /* mm5 >> 8 -> mm5 */
	  paddw_r2r(mm5, mm6); /* mm1 + mm2(dst) -> mm2 */
	  pand_r2r(mm7, mm6); /* mm6 & MASKBLUE -> mm6 */
	
	  movq_r2r(mm1, mm5); /* MASKRED -> mm5 */
	  por_r2r(mm4, mm5);  /* MASKGREEN | mm5 -> mm5 */
	  pand_r2r(mm5, mm3); /* mm3 & mm5(!MASKBLUE) -> mm3 */
	  por_r2r(mm6, mm3); /* save new blues in dsts */
	
	  movq_r2m(mm3, *pixel);/* mm2 -> 4 dst pixels */
	
	  pixel += 4;
	
        }, end);
      
        pixel = start + (pSurface->pitch >> 1);
        start = pixel;
      } /* while */
      emms();
    }

  }
  
  unlock_surf(pSurface);
  return 0;
}

/**************************************************************************
  ...
**************************************************************************/
static int __FillRectAlpha888_32bit(SDL_Surface * pSurface, SDL_Rect * pRect,
			       SDL_Color * pColor)
{
  Uint8 load[8] = {pColor->unused, pColor->unused,
		    pColor->unused, pColor->unused,
    			pColor->unused, pColor->unused,
			pColor->unused, pColor->unused};
  Uint32 A = pColor->unused;
  register Uint32 sSIMD2 = SDL_MapRGB(pSurface->format,
					    pColor->r, pColor->g,
					    pColor->b);
  Uint32 y, end;
  Uint32 *start, *pixel;
  
			
  movq_m2r(*load, mm4); /* alpha -> mm4 */
			
  *(Uint64 *)load = 0x00FF00FF00FF00FF;
  movq_m2r(*load, mm3); /* mask -> mm2 */
            
  pand_r2r(mm3, mm4); /* mm4 & mask -> 0A0A0A0A -> mm4 */

  *(Uint64 *)load = sSIMD2;
  movq_m2r(*load, mm5); /* src(0000ARGB) -> mm5 */
  movq_r2r(mm5, mm1); /* src(0000ARGB) -> mm1 (fot alpha 128 blits) */
  punpcklbw_r2r(mm5, mm5); /* AARRGGBB -> mm5 */
  pand_r2r(mm3, mm5); /* 0A0R0G0B -> mm5 */
  
  *(Uint64 *)load = 0xFF000000FF000000;/* dst alpha mask */
  movq_m2r(*load, mm7); /* dst alpha mask -> mm7 */
  
  lock_surf(pSurface);
  
  if (pRect == NULL) {
    end = pSurface->w * pSurface->h;
    pixel = (Uint32 *) pSurface->pixels;
    if (A == 128) {		/* 50% A */
      *(Uint64 *)load = 0x00fefefe00fefefe;/* alpha128 mask */
      movq_m2r(*load, mm4); /* alpha128 mask -> mm4 */
      *(Uint64 *)load = 0x0001010100010101;/* alpha128 mask */
      movq_m2r(*load, mm3); /* !alpha128 mask -> mm3 */
      punpckldq_r2r(mm1, mm1); /* mm1(0000ARGB) -> mm1(ARGBARGB) */
      DUFFS_LOOP_DOUBLE2(
      {
	A = *pixel;
	*pixel++ = ((((sSIMD2 & 0x00fefefe) + (A & 0x00fefefe)) >> 1)
		    + (sSIMD2 & A & 0x00010101)) | 0xFF000000;
      },{
	movq_m2r((*pixel), mm2);/* dst(ARGBARGB) -> mm2 */
	movq_r2r(mm2, mm6); /* dst(ARGBARGB) -> mm6 */
	movq_r2r(mm1, mm5); /* src(ARGBARGB) -> mm5 */
		
	pand_r2r(mm4, mm6); /* dst & mask -> mm1 */
	pand_r2r(mm4, mm5); /* src & mask -> mm4 */
	paddw_r2r(mm6, mm5); /* mm5 + mm4 -> mm4 */
	psrlq_i2r(1, mm5); /* mm4 >> 1 -> mm4 */
	
	pand_r2r(mm1, mm2); /* src & dst -> mm2 */
	pand_r2r(mm3, mm2); /* mm2 & !mask -> mm2 */
	paddw_r2r(mm5, mm2); /* mm5 + mm2 -> mm2 */
	por_r2r(mm7, mm2); /* mm7(dst alpha mask) | mm2 -> mm2 */
	movq_r2m(mm2, (*pixel));/* mm2 -> 2 dst pixels */
	pixel += 2;
	
      }, end);
      emms();
    } else {
      DUFFS_LOOP_DOUBLE2(
      {
	movq_r2r(mm5, mm1); /* src(0A0R0G0B) -> mm1 */
	
	movd_m2r((*pixel), mm2);/* dst(ARGB) -> mm2 (0000ARGB)*/
        punpcklbw_r2r(mm2, mm2); /* AARRGGBB -> mm2 */
        pand_r2r(mm3, mm2); /* 0A0R0G0B -> mm2 */
		
        psubw_r2r(mm2, mm1);/* src - dst -> mm1 */
	pmullw_r2r(mm4, mm1); /* mm1 * alpha -> mm1 */
	psrlw_i2r(8, mm1); /* mm1 >> 8 -> mm1 */
	paddw_r2r(mm1, mm2); /* mm1 + mm2(dst) -> mm2 */
	pand_r2r(mm3, mm2); /* 0A0R0G0B -> mm2 */
	packuswb_r2r(mm2, mm2);  /* ARGBARGB -> mm2 */
        por_r2r(mm7, mm2); /* mm7(dst alpha mask) | mm2 -> mm2 */
	movd_r2m(mm2, *pixel);/* mm2 -> pixel */
	pixel++;
      }, {
	movq_r2r(mm5, mm1); /* src(0A0R0G0B) -> mm1 */
	movq_r2r(mm5, mm0); /* src(0A0R0G0B) -> mm0 */
	
	movq_m2r((*pixel), mm2);/* dst(ARGBARGB) -> mm2 */
	movq_r2r(mm2, mm6); /* dst(ARGBARGB) -> mm1 */
        punpcklbw_r2r(mm2, mm2); /* low - AARRGGBB -> mm2 */
	punpckhbw_r2r(mm6, mm6); /* high - AARRGGBB -> mm6 */
        pand_r2r(mm3, mm2); /* 0A0R0G0B -> mm2 */
	pand_r2r(mm3, mm6); /* 0A0R0G0B -> mm6 */
	
        psubw_r2r(mm2, mm1);/* src - dst1 -> mm1 */
	psubw_r2r(mm6, mm0);/* src - dst2 -> mm0 */
	
	pmullw_r2r(mm4, mm1); /* mm1 * alpha -> mm1 */
	pmullw_r2r(mm4, mm0); /* mm0 * alpha -> mm0 */
	psrlw_i2r(8, mm1); /* mm1 >> 8 -> mm1 */
	psrlw_i2r(8, mm0); /* mm0 >> 8 -> mm0 */
	paddw_r2r(mm1, mm2); /* mm1 + mm2(dst) -> mm2 */
	paddw_r2r(mm0, mm6); /* mm0 + mm6(dst) -> mm6 */
	pand_r2r(mm3, mm2); /* 0A0R0G0B -> mm2 */
	pand_r2r(mm3, mm6); /* 0A0R0G0B -> mm6 */
	packuswb_r2r(mm2, mm2);  /* ARGBARGB -> mm2 */
	packuswb_r2r(mm6, mm6);  /* ARGBARGB -> mm6 */
	psrlq_i2r(32, mm2); /* mm2 >> 32 -> mm2 */
	psllq_i2r(32, mm6); /* mm6 << 32 -> mm6 */
	por_r2r(mm6, mm2); /* mm6 | mm2 -> mm2 */
	por_r2r(mm7, mm2); /* mm7(dst alpha mask) | mm2 -> mm2 */
        movq_r2m(mm2, *pixel);/* mm2 -> pixel */
	pixel += 2;
      },end);
      emms();
    } /* A != 128 */
  } else {
    /* correct pRect size */
    if (pRect->x < 0) {
      pRect->w += pRect->x;
      pRect->x = 0;
    } else {
      if (pRect->x >= pSurface->w - pRect->w) {
	pRect->w = pSurface->w - pRect->x;
      }
    }

    if (pRect->y < 0) {
      pRect->h += pRect->y;
      pRect->y = 0;
    } else {
      if (pRect->y >= pSurface->h - pRect->h) {
	pRect->h = pSurface->h - pRect->y;
      }
    }

    start = pixel = (Uint32 *) pSurface->pixels +
	(pRect->y * (pSurface->pitch >> 2)) + pRect->x;

    if (A == 128) {		/* 50% A */
      *(Uint64 *)load = 0x00fefefe00fefefe;/* alpha128 mask */
      movq_m2r(*load, mm4); /* alpha128 mask -> mm4 */
      *(Uint64 *)load = 0x0001010100010101;/* alpha128 mask */
      movq_m2r(*load, mm3); /* !alpha128 mask -> mm3 */
      punpckldq_r2r(mm1, mm1); /* mm1(0000ARGB) -> mm1(ARGBARGB) */
      y = pRect->h;
      end = pRect->w;
      while(y--) {
        DUFFS_LOOP_DOUBLE2(
        {
	  A = *pixel;
	  *pixel++ = ((((sSIMD2 & 0x00fefefe) + (A & 0x00fefefe)) >> 1)
		    + (sSIMD2 & A & 0x00010101)) | 0xFF000000;
        },{
	  movq_m2r((*pixel), mm2);/* dst(ARGBARGB) -> mm2 */
	  movq_r2r(mm2, mm6); /* dst(ARGBARGB) -> mm6 */
	  movq_r2r(mm1, mm5); /* src(ARGBARGB) -> mm5 */
		
	  pand_r2r(mm4, mm6); /* dst & mask -> mm1 */
	  pand_r2r(mm4, mm5); /* src & mask -> mm4 */
	  paddd_r2r(mm6, mm5); /* mm5 + mm4 -> mm4 */
	  psrld_i2r(1, mm5); /* mm4 >> 1 -> mm4 */
	
	  pand_r2r(mm1, mm2); /* src & dst -> mm2 */
	  pand_r2r(mm3, mm2); /* mm2 & !mask -> mm2 */
	  paddd_r2r(mm5, mm2); /* mm5 + mm2 -> mm2 */
	  por_r2r(mm7, mm2); /* mm7(dst alpha mask) | mm2 -> mm2 */
	  movq_r2m(mm2, (*pixel));/* mm2 -> 2 dst pixels */
	  pixel += 2;
	
        }, end);
	pixel = start + (pSurface->pitch >> 2);
	start = pixel;
      }/* while */
      emms();
      
    } else {
      y = pRect->h;
      end = pRect->w;
      
      while(y--) {
	DUFFS_LOOP_DOUBLE2(
        {
          movq_r2r(mm5, mm1); /* src(0A0R0G0B) -> mm1 */
	
	  movd_m2r((*pixel), mm2);/* dst(ARGB) -> mm2 (0000ARGB)*/
          punpcklbw_r2r(mm2, mm2); /* AARRGGBB -> mm2 */
          pand_r2r(mm3, mm2); /* 0A0R0G0B -> mm2 */
		
          psubw_r2r(mm2, mm1);/* src - dst -> mm1 */
	  pmullw_r2r(mm4, mm1); /* mm1 * alpha -> mm1 */
	  psrlw_i2r(8, mm1); /* mm1 >> 8 -> mm1 */
	  paddw_r2r(mm1, mm2); /* mm1 + mm2(dst) -> mm2 */
	  pand_r2r(mm3, mm2); /* 0A0R0G0B -> mm2 */
	  packuswb_r2r(mm2, mm2);  /* ARGBARGB -> mm2 */
	  por_r2r(mm7, mm2); /* mm7(dst alpha mask) | mm2 -> mm2 */
	  movd_r2m(mm2, *pixel);/* mm2 -> pixel */
	  pixel++;
        }, {
	  movq_r2r(mm5, mm1); /* src(0A0R0G0B) -> mm1 */
	  movq_r2r(mm5, mm0); /* src(0A0R0G0B) -> mm0 */
	
	  movq_m2r((*pixel), mm2);/* dst(ARGBARGB) -> mm2 */
	  movq_r2r(mm2, mm6); /* dst(ARGBARGB) -> mm1 */
          punpcklbw_r2r(mm2, mm2); /* low - AARRGGBB -> mm2 */
	  punpckhbw_r2r(mm6, mm6); /* high - AARRGGBB -> mm6 */
          pand_r2r(mm3, mm2); /* 0A0R0G0B -> mm2 */
	  pand_r2r(mm3, mm6); /* 0A0R0G0B -> mm6 */
	
          psubw_r2r(mm2, mm1);/* src - dst1 -> mm1 */
	  psubw_r2r(mm6, mm0);/* src - dst2 -> mm0 */
	
	  pmullw_r2r(mm4, mm1); /* mm1 * alpha -> mm1 */
	  pmullw_r2r(mm4, mm0); /* mm0 * alpha -> mm0 */
	  psrlw_i2r(8, mm1); /* mm1 >> 8 -> mm1 */
	  psrlw_i2r(8, mm0); /* mm0 >> 8 -> mm0 */
	  paddw_r2r(mm1, mm2); /* mm1 + mm2(dst) -> mm2 */
	  paddw_r2r(mm0, mm6); /* mm0 + mm6(dst) -> mm6 */
	  pand_r2r(mm3, mm2); /* 0A0R0G0B -> mm2 */
	  pand_r2r(mm3, mm6); /* 0A0R0G0B -> mm6 */
	  packuswb_r2r(mm2, mm2);  /* ARGBARGB -> mm2 */
	  packuswb_r2r(mm6, mm6);  /* ARGBARGB -> mm6 */
	  psrlq_i2r(32, mm2); /* mm2 >> 32 -> mm2 */
	  psllq_i2r(32, mm6); /* mm6 << 32 -> mm6 */
	  por_r2r(mm6, mm2); /* mm6 | mm2 -> mm2 */
	  por_r2r(mm7, mm2); /* mm7(dst alpha mask) | mm2 -> mm2 */
          movq_r2m(mm2, *pixel);/* mm2 -> pixel */
	  pixel += 2;
	}, end);
      
	pixel = start + (pSurface->pitch >> 2);
	start = pixel;
      } /* while */
      emms();
    }   
  }
  
  unlock_surf(pSurface);
  return 0;
}

/**************************************************************************
  ...
**************************************************************************/
static int __FillRectAlpha8888_32bit(SDL_Surface * pSurface, SDL_Rect * pRect,
			       SDL_Color * pColor)
{
  Uint8 load[8] = {pColor->unused, pColor->unused,
		    pColor->unused, pColor->unused,
    			pColor->unused, pColor->unused,
			pColor->unused, pColor->unused};
  Uint32 A = pColor->unused;
  Uint32 sSIMD2 = SDL_MapRGB(pSurface->format,
					    pColor->r, pColor->g,
					    pColor->b);
  Uint32 y, end, A_Dst, A_Mask = pSurface->format->Amask;
  Uint32 *start, *pixel;
  
  lock_surf(pSurface);
			
  movq_m2r(*load, mm4); /* alpha -> mm4 */
			
  *(Uint64 *)load = 0x00FF00FF00FF00FF;
  movq_m2r(*load, mm3); /* mask -> mm2 */
            
  pand_r2r(mm3, mm4); /* mm4 & mask -> 0A0A0A0A -> mm4 */

  *(Uint64 *)load = sSIMD2;
  movq_m2r(*load, mm5); /* src(0000ARGB) -> mm5 */
  movq_r2r(mm5, mm1); /* src(0000ARGB) -> mm1 (fot alpha 128 blits) */
  punpcklbw_r2r(mm5, mm5); /* AARRGGBB -> mm5 */
  pand_r2r(mm3, mm5); /* 0A0R0G0B -> mm5 */
  
  *(Uint64 *)load = 0xFF000000FF000000;/* dst alpha mask */
  movq_m2r(*load, mm7); /* dst alpha mask -> mm7 */
   
  
  if (pRect == NULL) {
    end = pSurface->w * pSurface->h;
    pixel = (Uint32 *) pSurface->pixels;
    if (A == 128) {		/* 50% A */
      *(Uint64 *)load = 0x00fefefe00fefefe;/* alpha128 mask */
      movq_m2r(*load, mm4); /* alpha128 mask -> mm4 */
      *(Uint64 *)load = 0x0001010100010101;/* alpha128 mask */
      movq_m2r(*load, mm3); /* !alpha128 mask -> mm3 */
      punpckldq_r2r(mm1, mm1); /* mm1(0000ARGB) -> mm1(ARGBARGB) */
      DUFFS_LOOP_DOUBLE2(
      {
	A = *pixel;
	A_Dst = A & A_Mask;
	*pixel++ = ((((sSIMD2 & 0x00fefefe) + (A & 0x00fefefe)) >> 1)
		    + (sSIMD2 & A & 0x00010101)) | A_Dst;
      },{
	movq_m2r((*pixel), mm2);/* dst(ARGBARGB) -> mm2 */
	movq_r2r(mm2, mm6); /* dst(ARGBARGB) -> mm6 */
	movq_r2r(mm2, mm0); /* dst(ARGBARGB) -> mm0 */
	pand_r2r(mm7, mm0); /* dst & alpha mask -> mm0 */
	movq_r2r(mm1, mm5); /* src(ARGBARGB) -> mm5 */
		
	pand_r2r(mm4, mm6); /* dst & mask -> mm1 */
	pand_r2r(mm4, mm5); /* src & mask -> mm4 */
	paddw_r2r(mm6, mm5); /* mm5 + mm4 -> mm4 */
	psrlq_i2r(1, mm5); /* mm4 >> 1 -> mm4 */
	
	pand_r2r(mm1, mm2); /* src & dst -> mm2 */
	pand_r2r(mm3, mm2); /* mm2 & !mask -> mm2 */
	paddw_r2r(mm5, mm2); /* mm5 + mm2 -> mm2 */
	
	por_r2r(mm0, mm2); /* mm0(dst alpha) | mm2 -> mm2 */
	
	movq_r2m(mm2, (*pixel));/* mm2 -> 2 dst pixels */
	pixel += 2;
	
      }, end);
      emms();
    } else {
      DUFFS_LOOP_DOUBLE2(
      {
	movq_r2r(mm5, mm1); /* src(0A0R0G0B) -> mm1 */
	
	movd_m2r((*pixel), mm2);/* dst(ARGB) -> mm2 (0000ARGB)*/
	movq_r2r(mm2, mm6);/* dst(ARGB) -> mm6 (0000ARGB)*/
        punpcklbw_r2r(mm2, mm2); /* AARRGGBB -> mm2 */
        pand_r2r(mm3, mm2); /* 0A0R0G0B -> mm2 */
	pand_r2r(mm7, mm6); /* 0000A000 -> mm6 */
        psubw_r2r(mm2, mm1);/* src - dst -> mm1 */
	pmullw_r2r(mm4, mm1); /* mm1 * alpha -> mm1 */
	psrlw_i2r(8, mm1); /* mm1 >> 8 -> mm1 */
	paddw_r2r(mm1, mm2); /* mm1 + mm2(dst) -> mm2 */
	pand_r2r(mm3, mm2); /* 0A0R0G0B -> mm2 */
	packuswb_r2r(mm2, mm2);  /* ARGBARGB -> mm2 */
	pcmpeqd_r2r(mm5,mm5); /* set mm5 high "1" */
	pxor_r2r(mm7, mm5); /* make clear alpha mask */
	pand_r2r(mm5, mm2); /* 0RGB0RGB -> mm2 */
	por_r2r(mm6, mm2); /* mm6(dst alpha) | mm2 -> mm2 */
	movd_r2m(mm2, *pixel);/* mm2 -> pixel */
	pixel++;
      }, {
	movq_r2r(mm5, mm1); /* src(0A0R0G0B) -> mm1 */
	movq_r2r(mm5, mm0); /* src(0A0R0G0B) -> mm0 */
	
	movq_m2r((*pixel), mm2);/* 2 x dst -> mm2(ARGBARGB) */
	movq_r2r(mm2, mm6); /* 2 x dst -> mm1(ARGBARGB) */
	movq_r2r(mm2, mm5); /* 2 x dst -> mm1(ARGBARGB) */
	pand_r2r(mm7, mm5); /* save dst alpha -> mm5(A000A000) */
        punpcklbw_r2r(mm2, mm2); /* low - AARRGGBB -> mm2 */
	punpckhbw_r2r(mm6, mm6); /* high - AARRGGBB -> mm6 */
        pand_r2r(mm3, mm2); /* 0A0R0G0B -> mm2 */
	pand_r2r(mm3, mm6); /* 0A0R0G0B -> mm6 */
	
        psubw_r2r(mm2, mm1);/* src - dst1 -> mm1 */
	psubw_r2r(mm6, mm0);/* src - dst2 -> mm0 */
	
	pmullw_r2r(mm4, mm1); /* mm1 * alpha -> mm1 */
	pmullw_r2r(mm4, mm0); /* mm0 * alpha -> mm0 */
	psrlw_i2r(8, mm1); /* mm1 >> 8 -> mm1 */
	psrlw_i2r(8, mm0); /* mm0 >> 8 -> mm0 */
	paddw_r2r(mm1, mm2); /* mm1 + mm2(dst) -> mm2 */
	paddw_r2r(mm0, mm6); /* mm0 + mm6(dst) -> mm6 */
	pand_r2r(mm3, mm2); /* 0A0R0G0B -> mm2 */
	pand_r2r(mm3, mm6); /* 0A0R0G0B -> mm6 */
	packuswb_r2r(mm2, mm2);  /* ARGBARGB -> mm2 */
	packuswb_r2r(mm6, mm6);  /* ARGBARGB -> mm6 */
	psrlq_i2r(32, mm2); /* mm2 >> 32 -> mm2 */
	psllq_i2r(32, mm6); /* mm6 << 32 -> mm6 */
	por_r2r(mm6, mm2); /* mm6 | mm2 -> mm2 */
	pcmpeqd_r2r(mm1,mm1); /* set mm1 to "1" */
	pxor_r2r(mm7, mm1); /* make clear alpha mask */
	pand_r2r(mm1, mm2); /* 0RGB0RGB -> mm2 */
	por_r2r(mm5, mm2); /* mm5(dst alpha) | mm2 -> mm2 */
        movq_r2m(mm2, *pixel);/* mm2 -> pixel */
	pixel += 2;
      },end);
      emms();
    } /* A != 128 */
  } else {
    /* correct pRect size */
    if (pRect->x < 0) {
      pRect->w += pRect->x;
      pRect->x = 0;
    } else {
      if (pRect->x >= pSurface->w - pRect->w) {
	pRect->w = pSurface->w - pRect->x;
      }
    }

    if (pRect->y < 0) {
      pRect->h += pRect->y;
      pRect->y = 0;
    } else {
      if (pRect->y >= pSurface->h - pRect->h) {
	pRect->h = pSurface->h - pRect->y;
      }
    }

    start = pixel = (Uint32 *) pSurface->pixels +
	(pRect->y * (pSurface->pitch >> 2)) + pRect->x;

    if (A == 128) {		/* 50% A */
      *(Uint64 *)load = 0x00fefefe00fefefe;/* alpha128 mask */
      movq_m2r(*load, mm4); /* alpha128 mask -> mm4 */
      *(Uint64 *)load = 0x0001010100010101;/* alpha128 mask */
      movq_m2r(*load, mm3); /* !alpha128 mask -> mm3 */
      punpckldq_r2r(mm1, mm1); /* mm1(0000ARGB) -> mm1(ARGBARGB) */
      y = pRect->h;
      end = pRect->w;
      while(y--) {
        DUFFS_LOOP_DOUBLE2(
        {
	  A = *pixel;
	  A_Dst = A & A_Mask;
	  *pixel++ = ((((sSIMD2 & 0x00fefefe) + (A & 0x00fefefe)) >> 1)
		    + (sSIMD2 & A & 0x00010101)) | A_Dst;
        },{
	  movq_m2r((*pixel), mm2);/* dst(ARGBARGB) -> mm2 */
	  movq_r2r(mm2, mm6); /* dst(ARGBARGB) -> mm6 */
	  movq_r2r(mm2, mm0); /* dst(ARGBARGB) -> mm0 */
	  pand_r2r(mm7, mm0); /* dst & alpha mask -> mm0(A000A000) */
	  movq_r2r(mm1, mm5); /* src(ARGBARGB) -> mm5 */
		
	  pand_r2r(mm4, mm6); /* dst & mask -> mm1 */
	  pand_r2r(mm4, mm5); /* src & mask -> mm4 */
	  paddd_r2r(mm6, mm5); /* mm5 + mm4 -> mm4 */
	  psrld_i2r(1, mm5); /* mm4 >> 1 -> mm4 */
	
	  pand_r2r(mm1, mm2); /* src & dst -> mm2 */
	  pand_r2r(mm3, mm2); /* mm2 & !mask -> mm2 */
	  paddd_r2r(mm5, mm2); /* mm5 + mm2 -> mm2 */
	  por_r2r(mm0, mm2); /* mm0(dst alpha) | mm2 -> mm2 */
	  movq_r2m(mm2, (*pixel));/* mm2 -> 2 dst pixels */
	  pixel += 2;
	
        }, end);
	pixel = start + (pSurface->pitch >> 2);
	start = pixel;
      }/* while */
      emms();
      
    } else {
      y = pRect->h;
      end = pRect->w;
      
      while(y--) {
	DUFFS_LOOP_DOUBLE2(
        {
          movq_r2r(mm5, mm1); /* src(0A0R0G0B) -> mm1 */
	
	  movd_m2r((*pixel), mm2);/* dst(ARGB) -> mm2 (0000ARGB)*/
	  movq_r2r(mm2, mm6);/* dst(ARGB) -> mm6 (0000ARGB)*/
          punpcklbw_r2r(mm2, mm2); /* AARRGGBB -> mm2 */
          pand_r2r(mm3, mm2); /* 0A0R0G0B -> mm2 */
	  pand_r2r(mm7, mm6); /* 0000A000 -> mm6 */	
          psubw_r2r(mm2, mm1);/* src - dst -> mm1 */
	  pmullw_r2r(mm4, mm1); /* mm1 * alpha -> mm1 */
	  psrlw_i2r(8, mm1); /* mm1 >> 8 -> mm1 */
	  paddw_r2r(mm1, mm2); /* mm1 + mm2(dst) -> mm2 */
	  pand_r2r(mm3, mm2); /* 0A0R0G0B -> mm2 */
	  packuswb_r2r(mm2, mm2);  /* ARGBARGB -> mm2 */
	  pcmpeqd_r2r(mm5,mm5); /* set mm5 high "1" */
	  pxor_r2r(mm7, mm5); /* make clear alpha mask */
	  pand_r2r(mm5, mm2); /* 0RGB0RGB -> mm2 */
	  por_r2r(mm6, mm2); /* mm6(dst alpha) | mm2 -> mm2 */
	  movd_r2m(mm2, *pixel);/* mm2 -> pixel */
	  pixel++;
        }, {
	  movq_r2r(mm5, mm1); /* src(0A0R0G0B) -> mm1 */
	  movq_r2r(mm5, mm0); /* src(0A0R0G0B) -> mm0 */
	
	  movq_m2r((*pixel), mm2);/* dst(ARGBARGB) -> mm2 */
	  movq_r2r(mm2, mm6); /* dst(ARGBARGB) -> mm1 */
	  punpckhbw_r2r(mm6, mm6); /* high - AARRGGBB -> mm6 */
	  pand_r2r(mm3, mm6); /* 0A0R0G0B -> mm6 */
	  
	  psubw_r2r(mm6, mm0);/* src - dst2 -> mm0 */
	  pmullw_r2r(mm4, mm0); /* mm0 * alpha -> mm0 */  
	  psrlw_i2r(8, mm0); /* mm0 >> 8 -> mm0 */
	  paddw_r2r(mm0, mm6); /* mm0 + mm6(dst) -> mm6 */
	  packuswb_r2r(mm6, mm6);  /* ARGBARGB -> mm6 */
	  
	  movq_r2r(mm2, mm0); /* 2 x dst -> mm0(ARGBARGB) */
	  pand_r2r(mm7, mm0); /* save dst alpha -> mm0(A000A000) */
          punpcklbw_r2r(mm2, mm2); /* low - AARRGGBB -> mm2 */
	  pand_r2r(mm3, mm2); /* 0A0R0G0B -> mm2 */
	  	  	
          psubw_r2r(mm2, mm1);/* src - dst1 -> mm1 */
	  pmullw_r2r(mm4, mm1); /* mm1 * alpha -> mm1 */
	  	  
	  psrlw_i2r(8, mm1); /* mm1 >> 8 -> mm1 */
	  paddw_r2r(mm1, mm2); /* mm1 + mm2(dst) -> mm2 */
	  
	  pand_r2r(mm3, mm2); /* 0A0R0G0B -> mm2 */
	  packuswb_r2r(mm2, mm2);  /* ARGBARGB -> mm2 */
	  	  
	  psrlq_i2r(32, mm2); /* mm2 >> 32 -> mm2 */
	  pcmpeqd_r2r(mm1,mm1); /* set mm1 to "1" */
	  psllq_i2r(32, mm6); /* mm6 << 32 -> mm6 */
	  por_r2r(mm6, mm2); /* mm6 | mm2 -> mm2 */
	  pxor_r2r(mm7, mm1); /* make clear alpha mask */
	  pand_r2r(mm1, mm2); /* 0RGB0RGB -> mm2 */
	  por_r2r(mm0, mm2); /* mm5(dst alpha) | mm2 -> mm2 */
          movq_r2m(mm2, *pixel);/* mm2 -> pixel */
	  pixel += 2;
	}, end);
      
	pixel = start + (pSurface->pitch >> 2);
	start = pixel;
      } /* while */
      emms();
    }   
  }
  
  unlock_surf(pSurface);
  return 0;
}

/**************************************************************************
  ...
**************************************************************************/
static int __FillRectAlpha888_24bit(SDL_Surface * pSurface, SDL_Rect * pRect,
			       SDL_Color * pColor)
{
  Uint8 load[8] = {pColor->unused, pColor->unused,
		    pColor->unused, pColor->unused,
    			pColor->unused, pColor->unused,
			pColor->unused, pColor->unused};
  Uint32 A = pColor->unused;
  register Uint32 sSIMD2 = SDL_MapRGB(pSurface->format,
					    pColor->r, pColor->g,
					    pColor->b);
  Uint32 y, end;
  Uint8 *start, *pixel;
  
			
  movq_m2r(*load, mm4); /* alpha -> mm4 */
			
  *(Uint64 *)load = 0x00FF00FF00FF00FF;
  movq_m2r(*load, mm3); /* mask -> mm2 */
            
  pand_r2r(mm3, mm4); /* mm4 & mask -> 0A0A0A0A -> mm4 */

  *(Uint64 *)load = sSIMD2;
  movq_m2r(*load, mm5); /* src(0000ARGB) -> mm5 */
  movq_r2r(mm5, mm1); /* src(0000ARGB) -> mm1 (fot alpha 128 blits) */
  punpcklbw_r2r(mm5, mm5); /* AARRGGBB -> mm5 */
  pand_r2r(mm3, mm5); /* 0A0R0G0B -> mm5 */
    
  lock_surf(pSurface);
  
  if (pRect == NULL) {
    end = pSurface->w * pSurface->h;
    pixel = (Uint8 *) pSurface->pixels;
    if (A == 128) {		/* 50% A */
      *(Uint64 *)load = 0x0000fefefefefefe;/* alpha128 mask */
      movq_m2r(*load, mm4); /* alpha128 mask -> mm4 */
      *(Uint64 *)load = 0x0000010101010101;/* alpha128 mask */
      movq_m2r(*load, mm3); /* !alpha128 mask -> mm3 */
      movq_r2r(mm1, mm7); /* color -> mm7 */
      psllq_i2r(24, mm7); /* mm7 << 24 -> mm7 */
      por_r2r(mm7, mm1); /* (00RGBRGB) -> mm1 */
      DUFFS_LOOP_DOUBLE2(
      {
	A = *pixel;
	A = ((((sSIMD2 & 0x00fefefe) + (A & 0x00fefefe)) >> 1)
		    + (sSIMD2 & A & 0x00010101));
	pixel[0] = A & 0xff;
        pixel[1] = (A >> 8) & 0xff;
        pixel[2] = (A >> 16) & 0xff;
	pixel += 3;
      },{
	movq_m2r((*pixel), mm2);/* dst(gbRGBRGB) -> mm2 */
	movq_r2r(mm2, mm6); /* dst(gbRGBRGB) -> mm6 */
	movq_r2r(mm1, mm5); /* src(00RGBRGB) -> mm5 */
		
	pand_r2r(mm4, mm6); /* dst & mask -> mm1 */
	pand_r2r(mm4, mm5); /* src & mask -> mm4 */
	paddw_r2r(mm6, mm5); /* mm5 + mm4 -> mm4 */
	psrlq_i2r(1, mm5); /* mm4 >> 1 -> mm4 */
	
	pand_r2r(mm1, mm2); /* src & dst -> mm2 */
	pand_r2r(mm3, mm2); /* mm2 & !mask -> mm2 */
	paddw_r2r(mm5, mm2); /* mm5 + mm2 -> mm2 */
	movq_r2m(mm2, *load);/* mm2 -> 2 dst pixels */
	pixel[0] = load[0];
	pixel[1] = load[1];
	pixel[2] = load[2];
	pixel[3] = load[3];
	pixel[4] = load[4];
	pixel[5] = load[5];
	pixel += 6;
      }, end);
      emms();
    } else {
      DUFFS_LOOP_DOUBLE2(
      {
	movq_r2r(mm5, mm1); /* src(0A0R0G0B) -> mm1 */
	
	movd_m2r((*pixel), mm2);/* dst(ARGB) -> mm2 (0000bRGB)*/
        punpcklbw_r2r(mm2, mm2); /* bbRRGGBB -> mm2 */
        pand_r2r(mm3, mm2); /* 0b0R0G0B -> mm2 */
		
        psubw_r2r(mm2, mm1);/* src - dst -> mm1 */
	pmullw_r2r(mm4, mm1); /* mm1 * alpha -> mm1 */
	psrlw_i2r(8, mm1); /* mm1 >> 8 -> mm1 */
	paddw_r2r(mm1, mm2); /* mm1 + mm2(dst) -> mm2 */
	pand_r2r(mm3, mm2); /* 0b0R0G0B -> mm2 */
	packuswb_r2r(mm2, mm2);  /* bRGBbRGB -> mm2 */
	movd_r2m(mm2, *load);/* mm2 -> pixel */
	pixel[0] = load[0];
	pixel[1] = load[1];
	pixel[2] = load[2];
	pixel += 3;
      }, {
	movq_r2r(mm5, mm1); /* src(0A0R0G0B) -> mm1 */
	movq_r2r(mm5, mm0); /* src(0A0R0G0B) -> mm0 */
	
	movq_m2r((*pixel), mm2);/* dst(gbRGBRGB) -> mm2 */
	movq_r2r(mm2, mm6); /* dst(gbRGBRGB) -> mm6 */
	psllq_i2r(8, mm6); /* mm6 << 8 -> mm6(bRGBRGB0) */
        punpcklbw_r2r(mm2, mm2); /* low - BBRRGGBB -> mm2 */
	punpckhbw_r2r(mm6, mm6); /* high - bbRRGGBB -> mm6 */
        pand_r2r(mm3, mm2); /* 0b0R0G0B -> mm2 */
	pand_r2r(mm3, mm6); /* 0b0R0G0B -> mm6 */
	
        psubw_r2r(mm2, mm1);/* src - dst1 -> mm1 */
	psubw_r2r(mm6, mm0);/* src - dst2 -> mm0 */
	pmullw_r2r(mm4, mm1); /* mm1 * alpha -> mm1 */
	pmullw_r2r(mm4, mm0); /* mm0 * alpha -> mm0 */
	psrlw_i2r(8, mm1); /* mm1 >> 8 -> mm1 */
	psrlw_i2r(8, mm0); /* mm0 >> 8 -> mm0 */
	paddw_r2r(mm1, mm2); /* mm1 + mm2(dst) -> mm2 */
	paddw_r2r(mm0, mm6); /* mm0 + mm6(dst) -> mm6 */
	pand_r2r(mm3, mm2); /* 0A0R0G0B -> mm2 */
	pand_r2r(mm3, mm6); /* 0A0R0G0B -> mm6 */
	packuswb_r2r(mm2, mm2);  /* ARGBARGB -> mm2 */
	packuswb_r2r(mm6, mm6);  /* ARGBARGB -> mm6 */
	psrlq_i2r(32, mm2); /* mm2 >> 32 -> mm2 */
	psllq_i2r(32, mm6); /* mm6 << 32 -> mm6 */
	por_r2r(mm6, mm2); /* mm6 | mm2 -> mm2 */
        movq_r2m(mm2, *load);/* mm2 -> pixel */
	pixel[0] = load[0];
	pixel[1] = load[1];
	pixel[2] = load[2];
	pixel[4] = load[4];
	pixel[5] = load[5];
	pixel[6] = load[6];
	pixel += 6;
      },end);
      emms();
    } /* A != 128 */
  } else {
    /* correct pRect size */
    if (pRect->x < 0) {
      pRect->w += pRect->x;
      pRect->x = 0;
    } else {
      if (pRect->x >= pSurface->w - pRect->w) {
	pRect->w = pSurface->w - pRect->x;
      }
    }

    if (pRect->y < 0) {
      pRect->h += pRect->y;
      pRect->y = 0;
    } else {
      if (pRect->y >= pSurface->h - pRect->h) {
	pRect->h = pSurface->h - pRect->y;
      }
    }

    start = pixel = (Uint8 *) pSurface->pixels +
	(pRect->y * pSurface->pitch ) + pRect->x * 3;

    if (A == 128) {		/* 50% A */
      *(Uint64 *)load = 0x0000fefefefefefe;/* alpha128 mask */
      movq_m2r(*load, mm4); /* alpha128 mask -> mm4 */
      *(Uint64 *)load = 0x0000010101010101;/* alpha128 mask */
      movq_m2r(*load, mm3); /* !alpha128 mask -> mm3 */
      movq_r2r(mm1, mm7); /* color -> mm7 */
      psllq_i2r(24, mm7); /* mm7 << 24 -> mm7 */
      por_r2r(mm7, mm1);
      y = pRect->h;
      end = pRect->w;
      while(y--) {
        DUFFS_LOOP_DOUBLE2(
        {
	  A = *pixel;
	  A = ((((sSIMD2 & 0x00fefefe) + (A & 0x00fefefe)) >> 1)
		    + (sSIMD2 & A & 0x00010101));
	  pixel[0] = A & 0xff;
          pixel[1] = (A >> 8) & 0xff;
          pixel[2] = (A >> 16) & 0xff;
	  pixel += 3;
        },{
	  movq_m2r((*pixel), mm2);/* dst(gbRGBRGB) -> mm2 */
	  movq_r2r(mm2, mm6); /* dst(gbRGBRGB) -> mm6 */
	  movq_r2r(mm1, mm5); /* src(ARGBARGB) -> mm5 */
		
	  pand_r2r(mm4, mm6); /* dst & mask -> mm1 */
	  pand_r2r(mm4, mm5); /* src & mask -> mm4 */
	  paddd_r2r(mm6, mm5); /* mm5 + mm4 -> mm4 */
	  psrld_i2r(1, mm5); /* mm4 >> 1 -> mm4 */
	
	  pand_r2r(mm1, mm2); /* src & dst -> mm2 */
	  pand_r2r(mm3, mm2); /* mm2 & !mask -> mm2 */
	  paddd_r2r(mm5, mm2); /* mm5 + mm2 -> mm2 */
	  por_r2r(mm7, mm2); /* mm7(dst alpha mask) | mm2 -> mm2 */
	  movq_r2m(mm2, *load);/* mm2 -> 2 dst pixels */
	  pixel[0] = load[0];
	  pixel[1] = load[1];
	  pixel[2] = load[2];
	  pixel[3] = load[3];
	  pixel[4] = load[4];
	  pixel[5] = load[5];
	  pixel += 6;
        }, end);
	pixel = start + pSurface->pitch;
	start = pixel;
      }/* while */
      emms();
      
    } else {
      y = pRect->h;
      end = pRect->w;
      
      while(y--) {
	DUFFS_LOOP_DOUBLE2(
        {
          movq_r2r(mm5, mm1); /* src(0A0R0G0B) -> mm1 */
	
	  movd_m2r((*pixel), mm2);/* dst(bRGB) -> mm2 (0000bRGB)*/
          punpcklbw_r2r(mm2, mm2); /* bbRRGGBB -> mm2 */
          pand_r2r(mm3, mm2); /* 0b0R0G0B -> mm2 */
		
          psubw_r2r(mm2, mm1);/* src - dst -> mm1 */
	  pmullw_r2r(mm4, mm1); /* mm1 * alpha -> mm1 */
	  psrlw_i2r(8, mm1); /* mm1 >> 8 -> mm1 */
	  paddw_r2r(mm1, mm2); /* mm1 + mm2(dst) -> mm2 */
	  pand_r2r(mm3, mm2); /* 0A0R0G0B -> mm2 */
	  packuswb_r2r(mm2, mm2);  /* ARGBARGB -> mm2 */
	  por_r2r(mm7, mm2); /* mm7(dst alpha mask) | mm2 -> mm2 */
	  movd_r2m(mm2, *load);/* mm2 -> pixel */
	  pixel[0] = load[0];
	  pixel[1] = load[1];
	  pixel[2] = load[2];
	
	  pixel += 3;
        }, {
	  movq_r2r(mm5, mm1); /* src(0A0R0G0B) -> mm1 */
	  movq_r2r(mm5, mm0); /* src(0A0R0G0B) -> mm0 */
	
	  movq_m2r((*pixel), mm2);/* dst(gbRGBRGB) -> mm2 */
	  movq_r2r(mm2, mm6); /* dst(gbRGBRGB) -> mm1 */
	  psllq_i2r(8, mm6); /* mm6 << 8 -> mm6(bRGBRGB0) */
          punpcklbw_r2r(mm2, mm2); /* low - BBRRGGBB -> mm2 */
	  punpckhbw_r2r(mm6, mm6); /* high - bbRRGGBB -> mm6 */
          pand_r2r(mm3, mm2); /* 0B0R0G0B -> mm2 */
	  pand_r2r(mm3, mm6); /* 0b0R0G0B -> mm6 */
	
          psubw_r2r(mm2, mm1);/* src - dst1 -> mm1 */
	  psubw_r2r(mm6, mm0);/* src - dst2 -> mm0 */
	
	  pmullw_r2r(mm4, mm1); /* mm1 * alpha -> mm1 */
	  pmullw_r2r(mm4, mm0); /* mm0 * alpha -> mm0 */
	  psrlw_i2r(8, mm1); /* mm1 >> 8 -> mm1 */
	  psrlw_i2r(8, mm0); /* mm0 >> 8 -> mm0 */
	  paddw_r2r(mm1, mm2); /* mm1 + mm2(dst) -> mm2 */
	  paddw_r2r(mm0, mm6); /* mm0 + mm6(dst) -> mm6 */
	  pand_r2r(mm3, mm2); /* 0A0R0G0B -> mm2 */
	  pand_r2r(mm3, mm6); /* 0A0R0G0B -> mm6 */
	  packuswb_r2r(mm2, mm2);  /* ARGBARGB -> mm2 */
	  packuswb_r2r(mm6, mm6);  /* ARGBARGB -> mm6 */
	  psrlq_i2r(32, mm2); /* mm2 >> 32 -> mm2 */
	  psllq_i2r(32, mm6); /* mm6 << 32 -> mm6 */
	  por_r2r(mm6, mm2); /* mm6 | mm2 -> mm2 */
	  por_r2r(mm7, mm2); /* mm7(dst alpha mask) | mm2 -> mm2 */
          movq_r2m(mm2, *load);/* mm2 -> pixel */
	  pixel[0] = load[0];
	  pixel[1] = load[1];
	  pixel[2] = load[2];
	  pixel[3] = load[4];
	  pixel[4] = load[5];
	  pixel[5] = load[6];
	  pixel += 6;
	}, end);
      
	pixel = start + pSurface->pitch;
	start = pixel;
      } /* while */
      emms();
    }   
  }
  
  unlock_surf(pSurface);
  return 0;
}


#else
/**************************************************************************
  ..
**************************************************************************/
static int __FillRectAlpha565(SDL_Surface * pSurface, SDL_Rect * pRect,
			      SDL_Color * pColor)
{
  Uint32 y, end;

  Uint16 *start, *pixel;

  register Uint32 D, S =
      SDL_MapRGB(pSurface->format, pColor->r, pColor->g, pColor->b);
  register Uint32 A = pColor->unused >> 3;

  S &= 0xFFFF;
  
  lock_surf(pSurface);
  if (pRect == NULL) {
    end = pSurface->w * pSurface->h;
    pixel = (Uint16 *) pSurface->pixels;
    if (A == 16) {		/* A == 128 >> 3 */
      /* this code don't work (A == 128) */
      if (end & 0x1) {		/* end % 2 */
	D = *pixel;
	*pixel++ = BLEND16_50(D, S, MASK565);
	end--;
      }

      S = S | S << 16;
      for (y = 0; y < end; y += 2) {
	D = *(Uint32 *) pixel;
	*(Uint32 *) pixel = BLEND2x16_50(D, S, MASK565);
	pixel += 2;
      }
    } else {
      S = (S | S << 16) & 0x07e0f81f;
      DUFFS_LOOP8(
      {
	D = *pixel;
	D = (D | D << 16) & 0x07e0f81f;
	D += (S - D) * A >> 5;
	D &= 0x07e0f81f;
	*pixel++ = (D | (D >> 16)) & 0xFFFF;
      }, end);
    }
  } else {
    /* correct pRect size */
    if (pRect->x < 0) {
      pRect->w += pRect->x;
      pRect->x = 0;
    } else {
      if (pRect->x >= pSurface->w - pRect->w) {
	pRect->w = pSurface->w - pRect->x;
      }
    }

    if (pRect->y < 0) {
      pRect->h += pRect->y;
      pRect->y = 0;
    } else {
      if (pRect->y >= pSurface->h - pRect->h) {
	pRect->h = pSurface->h - pRect->y;
      }
    }

    start = pixel = (Uint16 *) pSurface->pixels +
	(pRect->y * (pSurface->pitch >> 1)) + pRect->x;

    if (A == 16) {		/* A == 128 >> 3 */
      /* this code don't work (A == 128) */
      S = S | S << 16;
      for (y = 0; y < pRect->h; y++) {
	end = 0;

	if (pRect->w & 0x1) {
	  D = *pixel;
	  *pixel++ = BLEND16_50(D, (S & 0xFFFF), MASK565);
	  end++;
	}

	for (; end < pRect->w; end += 2) {
	  D = *(Uint32 *) pixel;
	  *(Uint32 *) pixel = BLEND2x16_50(D, S, MASK565);
	  pixel += 2;
	}

	pixel = start + (pSurface->pitch >> 1);
	start = pixel;
      }
    } else {
      y = 0;
      S = (S | S << 16) & 0x07e0f81f;
      y = pRect->h;
      end = pRect->w;
      
      while(y--) {
	DUFFS_LOOP8(
        {
          D = *pixel;
	  D = (D | D << 16) & 0x07e0f81f;
	  D += (S - D) * A >> 5;
	  D &= 0x07e0f81f;
	  *pixel++ = (D | (D >> 16)) & 0xFFFF;
        }, end);
      
	pixel = start + (pSurface->pitch >> 1);
	start = pixel;
      } /* while */
    }

  }
  
  unlock_surf(pSurface);
  return 0;
}

/**************************************************************************
  ...
**************************************************************************/
static int __FillRectAlpha555(SDL_Surface * pSurface, SDL_Rect * pRect,
			      SDL_Color * pColor)
{
  Uint32 y, end;

  Uint16 *start, *pixel;

  register Uint32 D, S =
      SDL_MapRGB(pSurface->format, pColor->r, pColor->g, pColor->b);
  register Uint32 A = pColor->unused >> 3;

  S &= 0xFFFF;
  
  lock_surf(pSurface);
  
  if (pRect == NULL) {
    end = pSurface->w * pSurface->h;
    pixel = (Uint16 *) pSurface->pixels;
    if (A == 16) {		/* A == 128 >> 3 */
      if (end & 0x1) {
	D = *pixel;
	*pixel++ = BLEND16_50(D, S, MASK555);
	end--;
      }

      S = S | S << 16;
      for (y = 0; y < end; y += 2) {
	D = *(Uint32 *) pixel;
	*(Uint32 *) pixel = BLEND2x16_50(D, S, MASK555);
	pixel += 2;
      }
    } else {
      S = (S | S << 16) & 0x03e07c1f;
      DUFFS_LOOP8(
      {
	D = *pixel;
	D = (D | D << 16) & 0x03e07c1f;
	D += (S - D) * A >> 5;
	D &= 0x03e07c1f;
	*pixel++ = (D | (D >> 16)) & 0xFFFF;
      }, end);
    }
  } else {
    /* correct pRect size */
    if (pRect->x < 0) {
      pRect->w += pRect->x;
      pRect->x = 0;
    } else {
      if (pRect->x >= pSurface->w - pRect->w) {
	pRect->w = pSurface->w - pRect->x;
      }
    }

    if (pRect->y < 0) {
      pRect->h += pRect->y;
      pRect->y = 0;
    } else {
      if (pRect->y >= pSurface->h - pRect->h) {
	pRect->h = pSurface->h - pRect->y;
      }
    }

    start = pixel = (Uint16 *) pSurface->pixels +
	(pRect->y * (pSurface->pitch >> 1)) + pRect->x;

    if (A == 16) {		/* A == 128 >> 3 */
      S = S | S << 16;
      for (y = 0; y < pRect->h; y++) {
	end = 0;

	if (pRect->w & 0x1) {
	  D = *pixel;
	  *pixel++ = BLEND16_50(D, (S & 0xFFFF), MASK555);
	  end++;
	}

	for (; end < pRect->w; end += 2) {
	  D = *(Uint32 *) pixel;
	  *(Uint32 *) pixel = BLEND2x16_50(D, S, MASK555);
	  pixel += 2;
	}

	pixel = start + (pSurface->pitch >> 1);
	start = pixel;
      }
    } else {
      
      S = (S | S << 16) & 0x03e07c1f;
      y = pRect->h;
      end = pRect->w;
      
      while(y--) {
	DUFFS_LOOP8(
        {
          D = *pixel;
	  D = (D | D << 16) & 0x03e07c1f;
	  D += (S - D) * A >> 5;
	  D &= 0x03e07c1f;
	  *pixel++ = (D | (D >> 16)) & 0xFFFF;
        }, end);
      
	pixel = start + (pSurface->pitch >> 1);
	start = pixel;
      } /* while */
    }
  }
  
  unlock_surf(pSurface);
  return 0;
}

/**************************************************************************
  ...
**************************************************************************/
static int __FillRectAlpha8888_32bit(SDL_Surface * pSurface, SDL_Rect * pRect,
			       SDL_Color * pColor)
{
  register Uint32 A = pColor->unused;
  register Uint32 dSIMD1, dSIMD2;
  register Uint32 sSIMD1, sSIMD2 = SDL_MapRGB(pSurface->format,
					    pColor->r, pColor->g,
					    pColor->b);
  Uint32 y, end, A_Dst, A_Mask = pSurface->format->Amask;
  Uint32 *start, *pixel;

  sSIMD1 = sSIMD2 & 0x00FF00FF;
  
  lock_surf(pSurface);
  
  if (pRect == NULL) {
    end = pSurface->w * pSurface->h;
    pixel = (Uint32 *) pSurface->pixels;
    if (A == 128) {		/* 50% A */
      DUFFS_LOOP8(
      {
	dSIMD2 = *pixel;
	A_Dst = dSIMD2 & A_Mask;
	*pixel++ = ((((sSIMD2 & 0x00fefefe) + (dSIMD2 & 0x00fefefe)) >> 1)
		      + (sSIMD2 & dSIMD2 & 0x00010101)) | A_Dst;
      }, end);
    } else {
      sSIMD2 &= 0xFF00;
      sSIMD2 = sSIMD2 >> 8 | sSIMD2 << 8;
      DUFFS_LOOP_DOUBLE2(
        {
	  dSIMD2 = *pixel;
	  A_Dst = dSIMD2 & A_Mask;
	  dSIMD1 = dSIMD2 & 0x00FF00FF;
	  dSIMD1 += (sSIMD1 - dSIMD1) * A >> 8;
	  dSIMD1 &= 0x00FF00FF;
	  dSIMD2 &= 0xFF00;
	  dSIMD2 += (((sSIMD2 << 8) & 0xFF00) - dSIMD2) * A >> 8;
	  dSIMD2 &= 0xFF00;
	  *pixel++ = dSIMD1 | dSIMD2 | A_Dst;
      },{
	  dSIMD1 = *pixel;
	  A_Dst = dSIMD1 & A_Mask;
	  dSIMD1 &= 0x00FF00FF;
	  dSIMD1 += (sSIMD1 - dSIMD1) * A >> 8;
	  dSIMD1 &= 0x00FF00FF;

	  dSIMD2 = ((*pixel & 0xFF00) >> 8)| ((pixel[1] & 0xFF00) << 8);
	  dSIMD2 += (sSIMD2 - dSIMD2) * A >> 8;
	  dSIMD2 &= 0x00FF00FF;

	  *pixel++ = dSIMD1 | ((dSIMD2 << 8) & 0xFF00) | A_Dst;
	
	  dSIMD1 = *pixel;
	  A_Dst = dSIMD1 & A_Mask;
	  dSIMD1 &= 0x00FF00FF;
	  dSIMD1 += (sSIMD1 - dSIMD1) * A >> 8;
	  dSIMD1 &= 0x00FF00FF;
	
	  *pixel++ = dSIMD1 | ((dSIMD2 >> 8) & 0xFF00) | A_Dst;
      }, end);
    }
  } else {
    /* correct pRect size */
    if (pRect->x < 0) {
      pRect->w += pRect->x;
      pRect->x = 0;
    } else {
      if (pRect->x >= pSurface->w - pRect->w) {
	pRect->w = pSurface->w - pRect->x;
      }
    }

    if (pRect->y < 0) {
      pRect->h += pRect->y;
      pRect->y = 0;
    } else {
      if (pRect->y >= pSurface->h - pRect->h) {
	pRect->h = pSurface->h - pRect->y;
      }
    }

    start = pixel = (Uint32 *) pSurface->pixels +
	(pRect->y * (pSurface->pitch >> 2)) + pRect->x;

    if (A == 128) {		/* 50% A */
      y = pRect->h;
      end = pRect->w;
      while(y--) {
        DUFFS_LOOP4(
	{
	  dSIMD2 = *pixel;
	  A_Dst = dSIMD2 & A_Mask;
	  *pixel++ = ((((sSIMD2 & 0x00fefefe) + (dSIMD2 & 0x00fefefe)) >> 1)
		      + (sSIMD2 & dSIMD2 & 0x00010101)) | A_Dst;
	}, end);
	pixel = start + (pSurface->pitch >> 2);
	start = pixel;
      }
    } else {
      y = pRect->h;
      end = pRect->w;
      
      sSIMD2 &= 0xFF00;
      sSIMD2 = sSIMD2 >> 8 | sSIMD2 << 8;
      
      while(y--) {
	DUFFS_LOOP_DOUBLE2(
        {
	  dSIMD2 = *pixel;
	  A_Dst = dSIMD2 & A_Mask;
	  dSIMD1 = dSIMD2 & 0x00FF00FF;
	  dSIMD1 += (sSIMD1 - dSIMD1) * A >> 8;
	  dSIMD1 &= 0x00FF00FF;
	  dSIMD2 &= 0xFF00;
	  dSIMD2 += (((sSIMD2 << 8) & 0xFF00) - dSIMD2) * A >> 8;
	  dSIMD2 &= 0xFF00;
	  *pixel++ = dSIMD1 | dSIMD2 | A_Dst;
        },{
	  dSIMD1 = *pixel;
	  A_Dst = dSIMD1 & A_Mask;
	  dSIMD1 &= 0x00FF00FF;
	  dSIMD1 += (sSIMD1 - dSIMD1) * A >> 8;
	  dSIMD1 &= 0x00FF00FF;

	  dSIMD2 = ((*pixel & 0xFF00) >> 8)| ((pixel[1] & 0xFF00) << 8);
	  dSIMD2 += (sSIMD2 - dSIMD2) * A >> 8;
	  dSIMD2 &= 0x00FF00FF;

	  *pixel++ = dSIMD1 | ((dSIMD2 << 8) & 0xFF00) | A_Dst;
	
	  dSIMD1 = *pixel;
	  A_Dst = dSIMD1 & A_Mask;
	  dSIMD1 &= 0x00FF00FF;
	  dSIMD1 += (sSIMD1 - dSIMD1) * A >> 8;
	  dSIMD1 &= 0x00FF00FF;
	
	  *pixel++ = dSIMD1 | ((dSIMD2 >> 8) & 0xFF00) | A_Dst;
        }, end);
	
	pixel = start + (pSurface->pitch >> 2);
	start = pixel;
	
      } /* while */
    }   
  }
  
  unlock_surf(pSurface);
  return 0;
}

/**************************************************************************
  ...
**************************************************************************/
static int __FillRectAlpha888_32bit(SDL_Surface * pSurface, SDL_Rect * pRect,
			       SDL_Color * pColor)
{
  register Uint32 A = pColor->unused;
  register Uint32 dSIMD1, dSIMD2;
  register Uint32 sSIMD1, sSIMD2 = SDL_MapRGB(pSurface->format,
					    pColor->r, pColor->g,
					    pColor->b);
  Uint32 y, end;
  Uint32 *start, *pixel;

  sSIMD1 = sSIMD2 & 0x00FF00FF;
  
  lock_surf(pSurface);
  
  if (pRect == NULL) {
    end = pSurface->w * pSurface->h;
    pixel = (Uint32 *) pSurface->pixels;
    if (A == 128) {		/* 50% A */
      for (y = 0; y < end; y++) {
	dSIMD2 = *pixel;
	*pixel++ = ((((sSIMD2 & 0x00fefefe) + (dSIMD2 & 0x00fefefe)) >> 1)
		    + (sSIMD2 & dSIMD2 & 0x00010101)) | 0xFF000000;
      }
    } else {
      sSIMD2 &= 0xFF00;
      sSIMD2 = sSIMD2 >> 8 | sSIMD2 << 8;
      DUFFS_LOOP_DOUBLE2(
        {
	  dSIMD2 = *pixel;
	  dSIMD1 = dSIMD2 & 0x00FF00FF;
	  dSIMD1 += (sSIMD1 - dSIMD1) * A >> 8;
	  dSIMD1 &= 0x00FF00FF;
	  dSIMD2 &= 0xFF00;
	  dSIMD2 += (((sSIMD2 << 8) & 0xFF00) - dSIMD2) * A >> 8;
	  dSIMD2 &= 0xFF00;
	  *pixel++ = dSIMD1 | dSIMD2 | 0xFF000000;
      },{
	  dSIMD1 = *pixel & 0x00FF00FF;
	  dSIMD1 += (sSIMD1 - dSIMD1) * A >> 8;
	  dSIMD1 &= 0x00FF00FF;

	  dSIMD2 = ((*pixel & 0xFF00) >> 8)| ((pixel[1] & 0xFF00) << 8);
	  dSIMD2 += (sSIMD2 - dSIMD2) * A >> 8;
	  dSIMD2 &= 0x00FF00FF;

	  *pixel++ = dSIMD1 | ((dSIMD2 << 8) & 0xFF00) | 0xFF000000;
	
	  dSIMD1 = *pixel & 0x00FF00FF;
	  dSIMD1 += (sSIMD1 - dSIMD1) * A >> 8;
	  dSIMD1 &= 0x00FF00FF;
	
	  *pixel++ = dSIMD1 | ((dSIMD2 >> 8) & 0xFF00) | 0xFF000000;
      }, end);
    }
  } else {
    /* correct pRect size */
    if (pRect->x < 0) {
      pRect->w += pRect->x;
      pRect->x = 0;
    } else {
      if (pRect->x >= pSurface->w - pRect->w) {
	pRect->w = pSurface->w - pRect->x;
      }
    }

    if (pRect->y < 0) {
      pRect->h += pRect->y;
      pRect->y = 0;
    } else {
      if (pRect->y >= pSurface->h - pRect->h) {
	pRect->h = pSurface->h - pRect->y;
      }
    }

    start = pixel = (Uint32 *) pSurface->pixels +
	(pRect->y * (pSurface->pitch >> 2)) + pRect->x;

    if (A == 128) {		/* 50% A */

      for (y = 0; y < pRect->h; y++) {

	for (end = 0; end < pRect->w; end++) {
	  dSIMD2 = *pixel;
	  *pixel++ = ((((sSIMD2 & 0x00fefefe) + (dSIMD2 & 0x00fefefe)) >> 1)
		      + (sSIMD2 & dSIMD2 & 0x00010101)) | 0xFF000000;
	}

	pixel = start + (pSurface->pitch >> 2);
	start = pixel;
      }
    } else {
      y = pRect->h;
      end = pRect->w;
      
      sSIMD2 &= 0xFF00;
      sSIMD2 = sSIMD2 >> 8 | sSIMD2 << 8;
      
      while(y--) {
	DUFFS_LOOP_DOUBLE2(
        {
	  dSIMD2 = *pixel;
	  dSIMD1 = dSIMD2 & 0x00FF00FF;
	  dSIMD1 += (sSIMD1 - dSIMD1) * A >> 8;
	  dSIMD1 &= 0x00FF00FF;
	  dSIMD2 &= 0xFF00;
	  dSIMD2 += (((sSIMD2 << 8) & 0xFF00) - dSIMD2) * A >> 8;
	  dSIMD2 &= 0xFF00;
	  *pixel++ = dSIMD1 | dSIMD2 | 0xFF000000;
        },{
	  dSIMD1 = *pixel & 0x00FF00FF;
	  dSIMD1 += (sSIMD1 - dSIMD1) * A >> 8;
	  dSIMD1 &= 0x00FF00FF;

	  dSIMD2 = ((*pixel & 0xFF00) >> 8)| ((pixel[1] & 0xFF00) << 8);
	  dSIMD2 += (sSIMD2 - dSIMD2) * A >> 8;
	  dSIMD2 &= 0x00FF00FF;

	  *pixel++ = dSIMD1 | ((dSIMD2 << 8) & 0xFF00) | 0xFF000000;
	
	  dSIMD1 = *pixel & 0x00FF00FF;
	  dSIMD1 += (sSIMD1 - dSIMD1) * A >> 8;
	  dSIMD1 &= 0x00FF00FF;
	
	  *pixel++ = dSIMD1 | ((dSIMD2 >> 8) & 0xFF00) | 0xFF000000;
        }, end);
	
	pixel = start + (pSurface->pitch >> 2);
	start = pixel;
	
      } /* while */
    }   
  }
  
  unlock_surf(pSurface);
  return 0;
}


/**************************************************************************
  ...
**************************************************************************/
static int __FillRectAlpha888_24bit(SDL_Surface * pSurface, SDL_Rect * pRect,
			      SDL_Color * pColor)
{
  Uint32 y, end;

  Uint8 *start, *pixel;

  register Uint32 P, D, S1, S2 = SDL_MapRGB(pSurface->format,
					    pColor->r, pColor->g,
					    pColor->b);

  register Uint32 A = pColor->unused;

  S1 = S2 & 0x00FF00FF;

  S2 &= 0xFF00;

  lock_surf(pSurface);
  
  if (pRect == NULL) {
    end = pSurface->w * pSurface->h;
    pixel = (Uint8 *) pSurface->pixels;

    for (y = 0; y < end; y++) {
      D = *(Uint32 *) pixel;

      P = D & 0x00FF00FF;
      P += (S1 - P) * A >> 8;
      P &= 0x00ff00ff;

      D = (D & 0xFF00);
      D += (S2 - D) * A >> 8;
      D &= 0xFF00;

      P = P | D;

      /* Fix me to little - big EDIAN */

      pixel[0] = P & 0xff;
      pixel[1] = (P >> 8) & 0xff;
      pixel[2] = (P >> 16) & 0xff;

      pixel += 3;
    }

  } else {
    /* correct pRect size */
    if (pRect->x < 0) {
      pRect->w += pRect->x;
      pRect->x = 0;
    } else {
      if (pRect->x >= pSurface->w - pRect->w) {
	pRect->w = pSurface->w - pRect->x;
      }
    }

    if (pRect->y < 0) {
      pRect->h += pRect->y;
      pRect->y = 0;
    } else {
      if (pRect->y >= pSurface->h - pRect->h) {
	pRect->h = pSurface->h - pRect->y;
      }
    }

    end = pRect->w * pRect->h;
    start = pixel = (Uint8 *) pSurface->pixels +
	(pRect->y * pSurface->pitch) + pRect->x * 3;

    y = 0;
    while (y != pRect->h) {
      D = *(Uint32 *) pixel;

      P = D & 0x00FF00FF;
      P += (S1 - P) * A >> 8;
      P &= 0x00ff00ff;

      D = (D & 0xFF00);
      D += (S2 - D) * A >> 8;
      D &= 0xFF00;

      P = P | D;

      /* Fix me to little - big EDIAN */

      pixel[0] = P & 0xff;
      pixel[1] = (P >> 8) & 0xff;
      pixel[2] = (P >> 16) & 0xff;

      if ((pixel - start) == (pRect->w * 3)) {
	pixel = start + pSurface->pitch;
	start = pixel;
	y++;
      } else {
	pixel += 3;
      }

    }

  }

  unlock_surf(pSurface);
  return 0;
}
#endif

/**************************************************************************
  ...
**************************************************************************/
int SDL_FillRectAlpha(SDL_Surface * pSurface, SDL_Rect * pRect,
		      SDL_Color * pColor)
{
	
  if (pRect && ( pRect->x < - pRect->w || pRect->x >= pSurface->w ||
	         pRect->y < - pRect->h || pRect->y >= pSurface->h ))
  {
     return -2;
  }
  
  if (pColor->unused == 255 )
  {
    return SDL_FillRect(pSurface, pRect,
	SDL_MapRGB(pSurface->format, pColor->r, pColor->g, pColor->b));
  }

  if (!pColor->unused)
  {
    return -3;
  }
  
  switch (pSurface->format->BytesPerPixel) {
  case 1:
    /* PORT ME */
    return -1;

  case 2:
    if (pSurface->format->Gmask == 0x7E0) {
      return __FillRectAlpha565(pSurface, pRect, pColor);
    } else {
      if (pSurface->format->Gmask == 0x3E0) {
	return __FillRectAlpha555(pSurface, pRect, pColor);
      } else {
	return -1;
      }
    }
    break;

  case 3:
    return __FillRectAlpha888_24bit(pSurface, pRect, pColor);

  case 4:
    if (pSurface->format->Amask) {
      return __FillRectAlpha8888_32bit(pSurface, pRect, pColor);
    } else {
      return __FillRectAlpha888_32bit(pSurface, pRect, pColor);
    }
  }

  return -1;
}

/**************************************************************************
  ...
**************************************************************************/
bool correct_rect_region(SDL_Rect * pRect)
{
  int ww = pRect->w, hh = pRect->h;

  if (pRect->x < 0) {
    ww += pRect->x;
    pRect->x = 0;
  }

  if (pRect->y < 0) {
    hh += pRect->y;
    pRect->y = 0;
  }

  if (pRect->x + ww > Main.screen->w) {
    ww = Main.screen->w - pRect->x;
  }

  if (pRect->y + hh > Main.screen->h) {
    hh = Main.screen->h - pRect->y;
  }

  /* End Correction */

  if (ww <= 0 || hh <= 0) {
    return FALSE;			/* suprice :) */
  } else {
    pRect->w = ww;
    pRect->h = hh;
  }

  return TRUE;
}

/**************************************************************************
  ...
**************************************************************************/
bool is_in_rect_area(int x, int y, SDL_Rect rect)
{
  return ((x >= rect.x) && (x < rect.x + rect.w)
	  && (y >= rect.y) && (y < rect.y + rect.h));
}

/**************************************************************************
  Most black color is coded like {0,0,0,255} but in sdl if alpha is turned 
  off and colorkey is set to 0 this black color is trasparent.
  To fix this we change all black {0, 0, 0, 255} to newblack {4, 4, 4, 255}
  (first collor != 0 in 16 bit coding).
**************************************************************************/
bool correct_black(SDL_Surface * pSrc)
{
  bool ret = 0;
  register int x;
  if (pSrc->format->BitsPerPixel == 32 && pSrc->format->Amask) {
    
    register Uint32 alpha, *pPixels = (Uint32 *) pSrc->pixels;
    Uint32 Amask = pSrc->format->Amask;
    
    Uint32 black = SDL_MapRGBA(pSrc->format, 0, 0, 0, 255);
    Uint32 new_black = SDL_MapRGBA(pSrc->format, 4, 4, 4, 255);

    int end = pSrc->w * pSrc->h;
    
    /* for 32 bit color coding */

    lock_surf(pSrc);

    for (x = 0; x < end; x++, pPixels++) {
      if (*pPixels == black) {
	*pPixels = new_black;
      } else {
	if (!ret) {
	  alpha = *pPixels & Amask;
	  if (alpha && (alpha != Amask)) {
	    ret = 1;
	  }
	}
      }
    }

    unlock_surf(pSrc);
  } else {
    if (pSrc->format->BitsPerPixel == 8 && pSrc->format->palette) {
      for(x = 0; x < pSrc->format->palette->ncolors; x++) {
	if (x != pSrc->format->colorkey &&
	  pSrc->format->palette->colors[x].r < 4 &&
	  pSrc->format->palette->colors[x].g < 4 &&
	  pSrc->format->palette->colors[x].b < 4) {
	    pSrc->format->palette->colors[x].r = 4;
	    pSrc->format->palette->colors[x].g = 4;
	    pSrc->format->palette->colors[x].b = 4;
	  }
      }
    }
  }

  return ret;
}

/* ===================================================================== */

/**************************************************************************
  ...
**************************************************************************/
SDL_Rect get_smaller_surface_rect(SDL_Surface * pSurface)
{
  int w, h, x, y;
  Uint16 minX, maxX, minY, maxY;
  Uint32 colorkey;
  SDL_Rect src;
  assert(pSurface != NULL);
  
  minX = pSurface->w;
  maxX = 0;
  minY = pSurface->h;
  maxY = 0;
  colorkey = pSurface->format->colorkey;
    
  lock_surf(pSurface);
      
  switch(pSurface->format->BytesPerPixel) {
    case 1:
    {
      Uint8 *pixel = (Uint8 *)pSurface->pixels;
      Uint8 *start = pixel;
      x = 0;
      y = 0;
      w = pSurface->w;
      h = pSurface->h;
      while(h--) {
        do {
	  if(*pixel != colorkey) {
	    if (minY > y) {
	      minY = y;
	    }

	    if (minX > x) {
	      minX = x;
	    }
            break;
	  }
	  pixel++;
	  x++;
        } while( --w > 0 );
	w = pSurface->w;
	x = 0;
	y++;
	pixel = start + pSurface->pitch;
	start = pixel;
      }
            
      w = pSurface->w;
      h = pSurface->h;
      x = w - 1;
      y = h - 1;
      pixel = (Uint8 *)((Uint8 *)pSurface->pixels + (y * pSurface->pitch) + x);
      start = pixel;
      while(h--) {
        do {
	  if(*pixel != colorkey) {
	    if (maxY < y) {
	      maxY = y;
	    }

	    if (maxX < x) {
	      maxX = x;
	    }
            break;
	  }
	  pixel--;
	  x--;
        } while( --w > 0 );
	w = pSurface->w;
	x = w - 1;
	y--;
	pixel = start - pSurface->pitch;
	start = pixel;
      }
    }
    break;
    case 2:
    {
      Uint16 *pixel = (Uint16 *)pSurface->pixels;
      Uint16 *start = pixel;
      x = 0;
      y = 0;
      w = pSurface->w;
      h = pSurface->h;
      while(h--) {
        do {
	  if(*pixel != colorkey) {
	    if (minY > y) {
	      minY = y;
	    }

	    if (minX > x) {
	      minX = x;
	    }
            break;
	  }
	  pixel++;
	  x++;
        } while( --w > 0 );
	w = pSurface->w;
	x = 0;
	y++;
	pixel = start + pSurface->pitch / 2;
	start = pixel;
      }
            
      w = pSurface->w;
      h = pSurface->h;
      x = w - 1;
      y = h - 1;
      pixel = (Uint16 *)((Uint8 *)pSurface->pixels + (y * pSurface->pitch) + x * 2);
      start = pixel;
      while(h--) {
        do {
	  if(*pixel != colorkey) {
	    if (maxY < y) {
	      maxY = y;
	    }

	    if (maxX < x) {
	      maxX = x;
	    }
            break;
	  }
	  pixel--;
	  x--;
        } while( --w > 0 );
	w = pSurface->w;
	x = w - 1;
	y--;
	pixel = start - pSurface->pitch / 2;
	start = pixel;
      }
    }
    break;
    case 3:
    {
      Uint8 *pixel = (Uint8 *)pSurface->pixels;
      Uint8 *start = pixel;
      Uint32 color;
      x = 0;
      y = 0;
      w = pSurface->w;
      h = pSurface->h;
      while(h--) {
        do {
	  if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
	    color = (pixel[0] << 16 | pixel[1] << 8 | pixel[2]);
          } else {
	    color = (pixel[0] | pixel[1] << 8 | pixel[2] << 16);
          }
	  if(color != colorkey) {
	    if (minY > y) {
	      minY = y;
	    }

	    if (minX > x) {
	      minX = x;
	    }
            break;
	  }
	  pixel += 3;
	  x++;
        } while( --w > 0 );
	w = pSurface->w;
	x = 0;
	y++;
	pixel = start + pSurface->pitch / 3;
	start = pixel;
      }
            
      w = pSurface->w;
      h = pSurface->h;
      x = w - 1;
      y = h - 1;
      pixel = (Uint8 *)((Uint8 *)pSurface->pixels + (y * pSurface->pitch) + x * 3);
      start = pixel;
      while(h--) {
        do {
	  if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
	    color = (pixel[0] << 16 | pixel[1] << 8 | pixel[2]);
          } else {
	    color = (pixel[0] | pixel[1] << 8 | pixel[2] << 16);
          }
	  if(color != colorkey) {
	    if (maxY < y) {
	      maxY = y;
	    }

	    if (maxX < x) {
	      maxX = x;
	    }
            break;
	  }
	  pixel -= 3;
	  x--;
        } while( --w > 0 );
	w = pSurface->w;
	x = w - 1;
	y--;
	pixel = start - pSurface->pitch / 3;
	start = pixel;
      }
    }
    break;
    case 4:
    {
      Uint32 *pixel = (Uint32 *)pSurface->pixels;
      Uint32 *start = pixel;
      x = 0;
      y = 0;
      w = pSurface->w;
      h = pSurface->h;
      while(h--) {
        do {
	  if(*pixel != colorkey) {
	    if (minY > y) {
	      minY = y;
	    }

	    if (minX > x) {
	      minX = x;
	    }
            break;
	  }
	  pixel++;
	  x++;
        } while( --w > 0 );
	w = pSurface->w;
	x = 0;
	y++;
	pixel = start + pSurface->pitch / 4;
	start = pixel;
      }
            
      w = pSurface->w;
      h = pSurface->h;
      x = w - 1;
      y = h - 1;
      pixel = (Uint32 *)((Uint8 *)pSurface->pixels + (y * pSurface->pitch) + x * 4);
      start = pixel;
      while(h--) {
        do {
	  if(*pixel != colorkey) {
	    if (maxY < y) {
	      maxY = y;
	    }

	    if (maxX < x) {
	      maxX = x;
	    }
            break;
	  }
	  pixel--;
	  x--;
        } while( --w > 0 );
	w = pSurface->w;
	x = w - 1;
	y--;
	pixel = start - pSurface->pitch / 4;
	start = pixel;
      }
    }
    break;
  }
  
  unlock_surf(pSurface);
  src.x = minX;
  src.y = minY;
  src.w = maxX - minX + 1;
  src.h = maxY - minY + 1;
  
  return src;
}

/**************************************************************************
  ... 
**************************************************************************/
SDL_Surface *crop_visible_part_from_surface(SDL_Surface * pSrc)
{
  SDL_Rect src = get_smaller_surface_rect(pSrc);
  return crop_rect_from_surface(pSrc, &src);
}

/**************************************************************************
  ...
**************************************************************************/
SDL_Surface *ResizeSurface(const SDL_Surface * pSrc, Uint16 new_width,
			   Uint16 new_height, int smooth)
{
  if (pSrc == NULL) {
    return NULL;
  }
  
  return zoomSurface((SDL_Surface*)pSrc,
                     (double)new_width / pSrc->w,
                     (double)new_height / pSrc->h,
                     smooth);
}

/**************************************************************************
  Resize a surface to fit into a box with the dimensions 'new_width' and a
  'new_height'. If 'scale_up' is FALSE, a surface that already fits into
  the box will not be scaled up to the boundaries of the box.
  If 'absolute_dimensions' is TRUE, the function returns a surface with the
  dimensions of the box and the scaled/original surface centered in it. 
**************************************************************************/
SDL_Surface *ResizeSurfaceBox(const SDL_Surface * pSrc,
                              Uint16 new_width, Uint16 new_height, int smooth,
                              bool scale_up, bool absolute_dimensions)
{
  SDL_Surface *tmpSurface, *result;

  if (pSrc == NULL) {
    return NULL;
  }

  if (!((scale_up == FALSE) && ((new_width >= pSrc->w) && (new_height >= pSrc->h)))) {
    if ((new_width - pSrc->w) <= (new_height - pSrc->h)) {
      /* horizontal limit */
      tmpSurface = zoomSurface((SDL_Surface*)pSrc,
                               (double)new_width / pSrc->w,
                               (double)new_width / pSrc->w,
                               smooth);
    } else {
      /* vertical limit */
      tmpSurface = zoomSurface((SDL_Surface*)pSrc,
                               (double)new_height / pSrc->h,
                               (double)new_height / pSrc->h,
                               smooth);
    }
  } else {
    tmpSurface = zoomSurface((SDL_Surface*)pSrc,
                             1.0,
                             1.0,
                             smooth);
  }
  
  if (absolute_dimensions) {
    SDL_Rect area = {
      (new_width - tmpSurface->w) / 2,
      (new_height - tmpSurface->h) / 2,
      0, 0
    };
    result = create_surf_alpha(new_width, new_height, SDL_SWSURFACE);
    alphablit(tmpSurface, NULL, result, &area);
    FREESURFACE(tmpSurface);
  } else {
    result = tmpSurface;
  }

  return result;  
}

/* ============ FreeCiv game graphics function =========== */

/**************************************************************************
  Return whether the client supports isometric view (isometric tilesets).
**************************************************************************/
bool isometric_view_supported(void)
{
  return TRUE;
}

/**************************************************************************
  Return whether the client supports "overhead" (non-isometric) view.
**************************************************************************/
bool overhead_view_supported(void)
{
  return TRUE;
}

/**************************************************************************
  ...
 **************************************************************************/
void load_intro_gfx(void)
{
  /* nothing */
}

/**************************************************************************
  Frees the introductory sprites.
**************************************************************************/
void free_intro_radar_sprites(void)
{
  /* nothing */
}
