/***********************************************************************
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

/***********************************************************************
                          graphics.c  -  description
                             -------------------
    begin                : Mon Jul 1 2002
    copyright            : (C) 2000 by Michael Speck
                         : (C) 2002 by Rafał Bursig
    email                : Michael Speck <kulkanie@gmx.net>
                         : Rafał Bursig <bursig@poczta.fm>
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

/* SDL2 */
#ifdef SDL2_PLAIN_INCLUDE
#include <SDL_image.h>
#include <SDL_syswm.h>
#include <SDL_ttf.h>
#else  /* SDL2_PLAIN_INCLUDE */
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_ttf.h>
#endif /* SDL2_PLAIN_INCLUDE */

/* utility */
#include "fcintl.h"
#include "log.h"

/* client */
#include "tilespec.h"

/* gui-sdl2 */
#include "colors.h"
#include "gui_tilespec.h"
#include "mapview.h"
#include "themebackgrounds.h"
#include "themespec.h"

#include "graphics.h"

/* ------------------------------ */

struct main Main;

static SDL_Surface *main_surface;

static bool render_dirty = TRUE;

/**********************************************************************//**
  Allocate new gui_layer.
**************************************************************************/
struct gui_layer *gui_layer_new(int x, int y, SDL_Surface *surface)
{
  struct gui_layer *result;

  result = fc_calloc(1, sizeof(struct gui_layer));

  result->dest_rect = (SDL_Rect){x, y, 0, 0};
  result->surface = surface;

  return result;
}

/**********************************************************************//**
  Free resources associated with gui_layer.
**************************************************************************/
void gui_layer_destroy(struct gui_layer **gui_layer)
{
  FREESURFACE((*gui_layer)->surface);
  FC_FREE(*gui_layer);
}

/**********************************************************************//**
  Get surface gui_layer.
**************************************************************************/
struct gui_layer *get_gui_layer(SDL_Surface *surface)
{
  int i = 0;

  while ((i < Main.guis_count) && Main.guis[i]) {
    if (Main.guis[i]->surface == surface) {
      return Main.guis[i];
    }
    i++;
  }

  return NULL;
}

/**********************************************************************//**
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

  pBuffer = create_surf(width, height, SDL_SWSURFACE);
  gui_layer = gui_layer_new(0, 0, pBuffer);

  /* add to buffers array */
  if (Main.guis) {
    int i;

    /* find NULL element */
    for (i = 0; i < Main.guis_count; i++) {
      if (!Main.guis[i]) {
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

/**********************************************************************//**
  Free buffer layer ( call by popdown_window_group_dialog(...) func )
  Func. Free buffer layer and clear buffer array entry.
**************************************************************************/
void remove_gui_layer(struct gui_layer *gui_layer)
{
  int i;

  for (i = 0; i < Main.guis_count - 1; i++) {
    if (Main.guis[i] && (Main.guis[i] == gui_layer)) {
      gui_layer_destroy(&Main.guis[i]);
      Main.guis[i] = Main.guis[i + 1];
      Main.guis[i + 1] = NULL;
    } else {
      if (!Main.guis[i]) {
        Main.guis[i] = Main.guis[i + 1];
        Main.guis[i + 1] = NULL;
      }
    }
  }

  if (Main.guis[Main.guis_count - 1]) {
    gui_layer_destroy(&Main.guis[Main.guis_count - 1]);
  }
}

/**********************************************************************//**
  Adjust dest_rect according to gui_layer.
**************************************************************************/
void screen_rect_to_layer_rect(struct gui_layer *gui_layer,
                               SDL_Rect *dest_rect)
{
  if (gui_layer) {
    dest_rect->x = dest_rect->x - gui_layer->dest_rect.x;
    dest_rect->y = dest_rect->y - gui_layer->dest_rect.y;
  }
}

/* ============ Freeciv sdl graphics function =========== */

/**********************************************************************//**
  Execute alphablit.
**************************************************************************/
int alphablit(SDL_Surface *src, SDL_Rect *srcrect,
              SDL_Surface *dst, SDL_Rect *dstrect,
              unsigned char alpha_mod)
{
  int ret;

  if (src == NULL || dst == NULL) {
    return 1;
  }

  SDL_SetSurfaceAlphaMod(src, alpha_mod);

  ret = SDL_BlitSurface(src, srcrect, dst, dstrect);

  if (ret) {
    log_error("SDL_BlitSurface() fails: %s", SDL_GetError());
  }

  return ret;
}

/**********************************************************************//**
  Execute alphablit to the main surface
**************************************************************************/
int screen_blit(SDL_Surface *src, SDL_Rect *srcrect, SDL_Rect *dstrect,
                unsigned char alpha_mod)
{
  render_dirty = TRUE;
  return alphablit(src, srcrect, main_surface, dstrect, alpha_mod);
}

/**********************************************************************//**
  Create new surface (pRect->w x pRect->h size) and copy pRect area of
  pSource.
  if pRect == NULL then create copy of entire pSource.
**************************************************************************/
SDL_Surface *crop_rect_from_surface(SDL_Surface *pSource,
                                    SDL_Rect *pRect)
{
  SDL_Surface *pNew = create_surf_with_format(pSource->format,
                                              pRect ? pRect->w : pSource->w,
                                              pRect ? pRect->h : pSource->h,
                                              SDL_SWSURFACE);

  if (alphablit(pSource, pRect, pNew, NULL, 255) != 0) {
    FREESURFACE(pNew);

    return NULL;
  }

  return pNew;
}

/**********************************************************************//**
  Reduce the alpha of the final surface proportional to the alpha of the mask.
  Thus if the mask has 50% alpha the final image will be reduced by 50% alpha.

  mask_offset_x, mask_offset_y is the offset of the mask relative to the
  origin of the source image.  The pixel at (mask_offset_x,mask_offset_y)
  in the mask image will be used to clip pixel (0,0) in the source image
  which is pixel (-x,-y) in the new image.
**************************************************************************/
SDL_Surface *mask_surface(SDL_Surface *pSrc, SDL_Surface *pMask,
                          int mask_offset_x, int mask_offset_y)
{
  SDL_Surface *pDest = NULL;
  int row, col;
  Uint32 *pSrc_Pixel = NULL;
  Uint32 *pDest_Pixel = NULL;
  Uint32 *pMask_Pixel = NULL;
  unsigned char src_alpha, mask_alpha;

  pDest = copy_surface(pSrc);

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

  return pDest;
}

/**********************************************************************//**
  Load a surface from file putting it in software mem.
**************************************************************************/
SDL_Surface *load_surf(const char *pFname)
{
  SDL_Surface *pBuf;

  if (!pFname) {
    return NULL;
  }

  if ((pBuf = IMG_Load(pFname)) == NULL) {
    log_error(_("load_surf: Failed to load graphic file %s!"), pFname);

    return NULL;
  }

  return pBuf;
}

/**********************************************************************//**
  Create an surface with format
  MUST NOT BE USED IF NO SDLSCREEN IS SET
**************************************************************************/
SDL_Surface *create_surf_with_format(SDL_PixelFormat *pf,
                                     int width, int height,
                                     Uint32 flags)
{
  SDL_Surface *surf = SDL_CreateRGBSurface(flags, width, height,
                                           pf->BitsPerPixel,
                                           pf->Rmask,
                                           pf->Gmask,
                                           pf->Bmask, pf->Amask);

  if (surf == NULL) {
    log_error(_("Unable to create Sprite (Surface) of size "
                "%d x %d %d Bits in format %d"),
              width, height, pf->BitsPerPixel, flags);
    return NULL;
  }

  return surf;
}

/**********************************************************************//**
  Create surface with the same format as main window
**************************************************************************/
SDL_Surface *create_surf(int width, int height, Uint32 flags)
{
  return create_surf_with_format(main_surface->format, width, height, flags);
}

/**********************************************************************//**
  Convert surface to the main window format.
**************************************************************************/
SDL_Surface *convert_surf(SDL_Surface *surf_in)
{
  return SDL_ConvertSurface(surf_in, main_surface->format, 0);
}

/**********************************************************************//**
  Create an surface with screen format and fill with color.
  If pColor == NULL surface is filled with transparent white A = 128
**************************************************************************/
SDL_Surface *create_filled_surface(Uint16 w, Uint16 h, Uint32 iFlags,
                                   SDL_Color *pColor)
{
  SDL_Surface *pNew;
  SDL_Color color = {255, 255, 255, 128};

  pNew = create_surf(w, h, iFlags);

  if (!pNew) {
    return NULL;
  }

  if (!pColor) {
    /* pColor->unused == ALPHA */
    pColor = &color;
  }

  SDL_FillRect(pNew, NULL,
               SDL_MapRGBA(pNew->format, pColor->r, pColor->g, pColor->b,
                           pColor->a));

  if (pColor->a != 255) {
    SDL_SetSurfaceAlphaMod(pNew, pColor->a);
  }

  return pNew;
}

/**********************************************************************//**
  Fill surface with (0, 0, 0, 0), so the next blitting operation can set
  the per pixel alpha
**************************************************************************/
int clear_surface(SDL_Surface *pSurf, SDL_Rect *dstrect)
{
  /* SDL_FillRect might change the rectangle, so we create a copy */
  if (dstrect) {
    SDL_Rect _dstrect = *dstrect;

    return SDL_FillRect(pSurf, &_dstrect, SDL_MapRGBA(pSurf->format, 0, 0, 0, 0));
  } else {
    return SDL_FillRect(pSurf, NULL, SDL_MapRGBA(pSurf->format, 0, 0, 0, 0));
  }
}

/**********************************************************************//**
  Blit entire src [SOURCE] surface to destination [DEST] surface
  on position : [iDest_x],[iDest_y] using it's actual alpha and
  color key settings.
**************************************************************************/
int blit_entire_src(SDL_Surface *pSrc, SDL_Surface *pDest,
                    Sint16 iDest_x, Sint16 iDest_y)
{
  SDL_Rect dest_rect = { iDest_x, iDest_y, 0, 0 };

  return alphablit(pSrc, NULL, pDest, &dest_rect, 255);
}

/**********************************************************************//**
  Get pixel
  Return the pixel value at (x, y)
  NOTE: The surface must be locked before calling this!
**************************************************************************/
Uint32 getpixel(SDL_Surface *pSurface, Sint16 x, Sint16 y)
{
  if (!pSurface) {
    return 0x0;
  }

  switch (pSurface->format->BytesPerPixel) {
  case 1:
    return *(Uint8 *) ((Uint8 *) pSurface->pixels + y * pSurface->pitch + x);

  case 2:
    return *((Uint16 *)pSurface->pixels + y * pSurface->pitch / sizeof(Uint16) + x);

  case 3:
    {
      /* Here ptr is the address to the pixel we want to retrieve */
      Uint8 *ptr =
        (Uint8 *) pSurface->pixels + y * pSurface->pitch + x * 3;

      if (is_bigendian()) {
        return ptr[0] << 16 | ptr[1] << 8 | ptr[2];
      } else {
        return ptr[0] | ptr[1] << 8 | ptr[2] << 16;
      }
    }
  case 4:
    return *((Uint32 *)pSurface->pixels + y * pSurface->pitch / sizeof(Uint32) + x);

  default:
    return 0; /* shouldn't happen, but avoids warnings */
  }
}

/**********************************************************************//**
  Get first pixel
  Return the pixel value at (0, 0)
  NOTE: The surface must be locked before calling this!
**************************************************************************/
Uint32 get_first_pixel(SDL_Surface *pSurface)
{
  if (!pSurface) {
    return 0;
  }

  switch (pSurface->format->BytesPerPixel) {
  case 1:
    return *((Uint8 *)pSurface->pixels);

  case 2:
    return *((Uint16 *)pSurface->pixels);

  case 3:
    {
      if (is_bigendian()) {
        return (((Uint8 *)pSurface->pixels)[0] << 16)
          | (((Uint8 *)pSurface->pixels)[1] << 8)
          | ((Uint8 *)pSurface->pixels)[2];
      } else {
        return ((Uint8 *)pSurface->pixels)[0]
          | (((Uint8 *)pSurface->pixels)[1] << 8)
          | (((Uint8 *)pSurface->pixels)[2] << 16);
      }
    }
  case 4:
    return *((Uint32 *)pSurface->pixels);

  default:
    return 0; /* shouldn't happen, but avoids warnings */
  }
}

/* ===================================================================== */

/**********************************************************************//**
  Initialize sdl with Flags
**************************************************************************/
void init_sdl(int iFlags)
{
  bool error;

  Main.screen = NULL;
  Main.guis = NULL;
  Main.gui = NULL;
  Main.map = NULL;
  Main.dummy = NULL; /* can't create yet -- hope we don't need it */
  Main.rects_count = 0;
  Main.guis_count = 0;

  if (SDL_WasInit(SDL_INIT_AUDIO)) {
    error = (SDL_InitSubSystem(iFlags) < 0);
  } else {
    error = (SDL_Init(iFlags) < 0);
  }
  if (error) {
    log_fatal(_("Unable to initialize SDL2 library: %s"), SDL_GetError());
    exit(EXIT_FAILURE);
  }

  atexit(SDL_Quit);

  /* Initialize the TTF library */
  if (TTF_Init() < 0) {
    log_fatal(_("Unable to initialize SDL2_ttf library: %s"), SDL_GetError());
    exit(EXIT_FAILURE);
  }

  atexit(TTF_Quit);
}

/**********************************************************************//**
  Free screen buffers
**************************************************************************/
void quit_sdl(void)
{
  FC_FREE(Main.guis);
  gui_layer_destroy(&Main.gui);
  FREESURFACE(Main.map);
  FREESURFACE(Main.dummy);
}

/**********************************************************************//**
  Switch to passed video mode.
**************************************************************************/
int set_video_mode(int iWidth, int iHeight, int iFlags)
{
  unsigned int flags;

  Main.screen = SDL_CreateWindow(_("SDL2 Client for Freeciv"),
                                 SDL_WINDOWPOS_UNDEFINED,
                                 SDL_WINDOWPOS_UNDEFINED,
                                 iWidth, iHeight,
                                 0);

  if (iFlags & SDL_WINDOW_FULLSCREEN) {
    SDL_DisplayMode mode;

    /* Use SDL_WINDOW_FULLSCREEN_DESKTOP instead of real SDL_WINDOW_FULLSCREEN */
    SDL_SetWindowFullscreen(Main.screen, SDL_WINDOW_FULLSCREEN_DESKTOP);
    SDL_GetWindowDisplayMode(Main.screen, &mode);
    iWidth = mode.w;
    iHeight = mode.h;
  }

  if (is_bigendian()) {
      main_surface = SDL_CreateRGBSurface(0, iWidth, iHeight, 32,
                                          0x0000FF00, 0x00FF0000,
                                          0xFF000000, 0x000000FF);
  } else {
    main_surface = SDL_CreateRGBSurface(0, iWidth, iHeight, 32,
                                        0x00FF0000, 0x0000FF00,
                                        0x000000FF, 0xFF000000);
  }

  if (is_bigendian()) {
    Main.map = SDL_CreateRGBSurface(0, iWidth, iHeight, 32,
                                    0x0000FF00, 0x00FF0000,
                                    0xFF000000, 0x000000FF);
  } else {
    Main.map = SDL_CreateRGBSurface(0, iWidth, iHeight, 32,
                                    0x00FF0000, 0x0000FF00,
                                    0x000000FF, 0xFF000000);
  }

  if (gui_options.gui_sdl2_swrenderer) {
    flags = SDL_RENDERER_SOFTWARE;
  } else {
    flags = 0;
  }

  Main.renderer = SDL_CreateRenderer(Main.screen, -1, flags);

  Main.maintext = SDL_CreateTexture(Main.renderer,
                                    SDL_PIXELFORMAT_ARGB8888,
                                    SDL_TEXTUREACCESS_STREAMING,
                                    iWidth, iHeight);

  if (Main.gui) {
    FREESURFACE(Main.gui->surface);
    Main.gui->surface = create_surf(iWidth, iHeight, SDL_SWSURFACE);
  } else {
    Main.gui = add_gui_layer(iWidth, iHeight);
  }

  clear_surface(Main.gui->surface, NULL);

  return 0;
}

/**********************************************************************//**
  Render from main surface with screen renderer.
**************************************************************************/
void update_main_screen(void)
{
  if (render_dirty) {
    SDL_UpdateTexture(Main.maintext, NULL,
                      main_surface->pixels, main_surface->pitch);
    SDL_RenderClear(Main.renderer);
    SDL_RenderCopy(Main.renderer, Main.maintext, NULL, NULL);
    SDL_RenderPresent(Main.renderer);

    render_dirty = FALSE;
  }
}

/**********************************************************************//**
  Return width of the main window
**************************************************************************/
int main_window_width(void)
{
  return main_surface->w;
}

/**********************************************************************//**
  Return height of the main window
**************************************************************************/
int main_window_height(void)
{
  return main_surface->h;
}

/**************************************************************************
                           Fill Rect with RGBA color
**************************************************************************/
#define MASK565 0xf7de
#define MASK555 0xfbde

/* 50% alpha (128) */
#define BLEND16_50( d, s , mask )       \
        (((( s & mask ) + ( d & mask )) >> 1) + ( s & d & ( ~mask & 0xffff)))

#define BLEND2x16_50( d, s , mask )     \
        (((( s & (mask | mask << 16)) + ( d & ( mask | mask << 16 ))) >> 1) + \
        ( s & d & ( ~(mask | mask << 16))))

/**********************************************************************//**
  Fill rectangle for "565" format surface
**************************************************************************/
static int __FillRectAlpha565(SDL_Surface *pSurface, SDL_Rect *pRect,
                              SDL_Color *pColor)
{
  Uint32 y, end;
  Uint32 *start;
  Uint32 *pixel;
  register Uint32 D, S =
      SDL_MapRGB(pSurface->format, pColor->r, pColor->g, pColor->b);
  register Uint32 A = pColor->a >> 3;

  S &= 0xFFFF;

  lock_surf(pSurface);
  if (pRect == NULL) {
    end = pSurface->w * pSurface->h;
    pixel = pSurface->pixels;
    if (A == 16) { /* A == 128 >> 3 */
      /* this code don't work (A == 128) */
      if (end & 0x1) { /* end % 2 */
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

    start = pixel = (Uint32 *) pSurface->pixels +
      (pRect->y * pSurface->pitch) + pRect->x / 2;

    if (A == 16) { /* A == 128 >> 3 */
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

        pixel = start + pSurface->pitch;
        start = pixel;
      }
    } else {
      y = 0;
      S = (S | S << 16) & 0x07e0f81f;
      y = pRect->h;
      end = pRect->w;

      while (y--) {
        DUFFS_LOOP8(
        {
          D = *pixel;
          D = (D | D << 16) & 0x07e0f81f;
          D += (S - D) * A >> 5;
          D &= 0x07e0f81f;
          *pixel++ = (D | (D >> 16)) & 0xFFFF;
        }, end);

        pixel = start + pSurface->pitch;
        start = pixel;
      } /* while */
    }

  }
  
  unlock_surf(pSurface);
  return 0;
}

/**********************************************************************//**
  Fill rectangle for "555" format surface
**************************************************************************/
static int __FillRectAlpha555(SDL_Surface *pSurface, SDL_Rect *pRect,
                              SDL_Color *pColor)
{
  Uint32 y, end;
  Uint32 *start, *pixel;
  register Uint32 D, S =
      SDL_MapRGB(pSurface->format, pColor->r, pColor->g, pColor->b);
  register Uint32 A = pColor->a >> 3;

  S &= 0xFFFF;

  lock_surf(pSurface);

  if (pRect == NULL) {
    end = pSurface->w * pSurface->h;
    pixel = pSurface->pixels;
    if (A == 16) { /* A == 128 >> 3 */
      if (end & 0x1) {
        D = *pixel;
        *pixel++ = BLEND16_50(D, S, MASK555);
        end--;
      }

      S = S | S << 16;
      for (y = 0; y < end; y += 2) {
        D = *pixel;
        *pixel = BLEND2x16_50(D, S, MASK555);
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

    start = pixel = (Uint32 *) pSurface->pixels +
      (pRect->y * pSurface->pitch) + pRect->x / 2;

    if (A == 16) { /* A == 128 >> 3 */
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

        pixel = start + pSurface->pitch;
        start = pixel;
      }
    } else {
      
      S = (S | S << 16) & 0x03e07c1f;
      y = pRect->h;
      end = pRect->w;

      while (y--) {
        DUFFS_LOOP8(
        {
          D = *pixel;
          D = (D | D << 16) & 0x03e07c1f;
          D += (S - D) * A >> 5;
          D &= 0x03e07c1f;
          *pixel++ = (D | (D >> 16)) & 0xFFFF;
        }, end);

        pixel = start + pSurface->pitch;
        start = pixel;
      } /* while */
    }
  }
  
  unlock_surf(pSurface);
  return 0;
}

/**********************************************************************//**
  Fill rectangle for 32bit "8888" format surface
**************************************************************************/
static int __FillRectAlpha8888_32bit(SDL_Surface *pSurface, SDL_Rect *pRect,
                                     SDL_Color *pColor)
{
  register Uint32 A = pColor->a;
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
    if (A == 128) { /* 50% A */
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

    if (A == 128) { /* 50% A */
      y = pRect->h;
      end = pRect->w;
      while (y--) {
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

      while (y--) {
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

/**********************************************************************//**
  Fill rectangle for 32bit "888" format surface
**************************************************************************/
static int __FillRectAlpha888_32bit(SDL_Surface *pSurface, SDL_Rect *pRect,
                                    SDL_Color *pColor)
{
  register Uint32 A = pColor->a;
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
    if (A == 128) { /* 50% A */
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

    if (A == 128) { /* 50% A */

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

      while (y--) {
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

/**********************************************************************//**
  Fill rectangle for 24bit "888" format surface
**************************************************************************/
static int __FillRectAlpha888_24bit(SDL_Surface *pSurface, SDL_Rect *pRect,
                                    SDL_Color *pColor)
{
  Uint32 y, end;
  Uint8 *start, *pixel;
  register Uint32 P, D, S1, S2 = SDL_MapRGB(pSurface->format,
                                            pColor->r, pColor->g,
                                            pColor->b);
  register Uint32 A = pColor->a;

  S1 = S2 & 0x00FF00FF;

  S2 &= 0xFF00;

  lock_surf(pSurface);

  if (pRect == NULL) {
    end = pSurface->w * pSurface->h;
    pixel = (Uint8 *) pSurface->pixels;

    for (y = 0; y < end; y++) {
      D = (pixel[0] << 16) + (pixel[1] << 8) + pixel[2];

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
      D = (pixel[0] << 16) + (pixel[1] << 8) + pixel[2];

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

/**********************************************************************//**
  Fill rectangle with color with alpha channel.
**************************************************************************/
int fill_rect_alpha(SDL_Surface *pSurface, SDL_Rect *pRect,
                    SDL_Color *pColor)
{
  if (pRect && (pRect->x < - pRect->w || pRect->x >= pSurface->w
                || pRect->y < - pRect->h || pRect->y >= pSurface->h)) {
    return -2;
  }

  if (pColor->a == 255) {
    return SDL_FillRect(pSurface, pRect,
                        SDL_MapRGB(pSurface->format, pColor->r, pColor->g, pColor->b));
  }

  if (!pColor->a) {
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

/**********************************************************************//**
  Make rectangle region sane. Return TRUE if result is sane.
**************************************************************************/
bool correct_rect_region(SDL_Rect *pRect)
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

  if (pRect->x + ww > main_window_width()) {
    ww = main_window_width() - pRect->x;
  }

  if (pRect->y + hh > main_window_height()) {
    hh = main_window_height() - pRect->y;
  }

  /* End Correction */

  if (ww <= 0 || hh <= 0) {
    return FALSE; /* suprise :) */
  } else {
    pRect->w = ww;
    pRect->h = hh;
  }

  return TRUE;
}

/**********************************************************************//**
  Return whether coordinates are in rectangle.
**************************************************************************/
bool is_in_rect_area(int x, int y, SDL_Rect rect)
{
  return ((x >= rect.x) && (x < rect.x + rect.w)
          && (y >= rect.y) && (y < rect.y + rect.h));
}

/* ===================================================================== */

/**********************************************************************//**
  Get visible rectangle from surface.
**************************************************************************/
SDL_Rect get_smaller_surface_rect(SDL_Surface *pSurface)
{
  SDL_Rect src;
  int w, h, x, y;
  Uint16 minX, maxX, minY, maxY;
  Uint32 colorkey;

  fc_assert(pSurface != NULL);

  minX = pSurface->w;
  maxX = 0;
  minY = pSurface->h;
  maxY = 0;
  SDL_GetColorKey(pSurface, &colorkey);

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
      while (h--) {
        do {
          if (*pixel != colorkey) {
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
        } while (--w > 0);
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
      while (h--) {
        do {
          if (*pixel != colorkey) {
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
        } while (--w > 0);
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
      while (h--) {
        do {
          if (*pixel != colorkey) {
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
        } while (--w > 0);
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
      pixel = ((Uint16 *)pSurface->pixels + (y * pSurface->pitch / 2) + x);
      start = pixel;
      while (h--) {
        do {
          if (*pixel != colorkey) {
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
        } while (--w > 0);
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
      while (h--) {
        do {
          if (is_bigendian()) {
            color = (pixel[0] << 16 | pixel[1] << 8 | pixel[2]);
          } else {
            color = (pixel[0] | pixel[1] << 8 | pixel[2] << 16);
          }
          if (color != colorkey) {
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
        } while (--w > 0);
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
      while (h--) {
        do {
          if (is_bigendian()) {
            color = (pixel[0] << 16 | pixel[1] << 8 | pixel[2]);
          } else {
            color = (pixel[0] | pixel[1] << 8 | pixel[2] << 16);
          }
          if (color != colorkey) {
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
        } while (--w > 0);
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
      while (h--) {
        do {
          if (*pixel != colorkey) {
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
        } while (--w > 0);
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
      pixel = ((Uint32 *)pSurface->pixels + (y * pSurface->pitch / 4) + x);
      start = pixel;
      while (h--) {
        do {
          if (*pixel != colorkey) {
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
        } while (--w > 0);
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

/**********************************************************************//**
  Create new surface that is just visible part of source surface.
**************************************************************************/
SDL_Surface *crop_visible_part_from_surface(SDL_Surface *pSrc)
{
  SDL_Rect src = get_smaller_surface_rect(pSrc);

  return crop_rect_from_surface(pSrc, &src);
}

/**********************************************************************//**
  Scale surface.
**************************************************************************/
SDL_Surface *ResizeSurface(const SDL_Surface *pSrc, Uint16 new_width,
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

/**********************************************************************//**
  Resize a surface to fit into a box with the dimensions 'new_width' and a
  'new_height'. If 'scale_up' is FALSE, a surface that already fits into
  the box will not be scaled up to the boundaries of the box.
  If 'absolute_dimensions' is TRUE, the function returns a surface with the
  dimensions of the box and the scaled/original surface centered in it. 
**************************************************************************/
SDL_Surface *ResizeSurfaceBox(const SDL_Surface *pSrc,
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

    result = create_surf(new_width, new_height, SDL_SWSURFACE);
    alphablit(tmpSurface, NULL, result, &area, 255);
    FREESURFACE(tmpSurface);
  } else {
    result = tmpSurface;
  }

  return result;
}

/**********************************************************************//**
  Return copy of the surface
**************************************************************************/
SDL_Surface *copy_surface(SDL_Surface *src)
{
  SDL_Surface *dst;

  dst = SDL_CreateRGBSurface(0, src->w, src->h, src->format->BitsPerPixel,
                             src->format->Rmask, src->format->Gmask,
                             src->format->Bmask, src->format->Amask);

  SDL_BlitSurface(src, NULL, dst, NULL);

  return dst;
}

/* ============ Freeciv game graphics function =========== */

/**********************************************************************//**
  Return whether the client supports given view type
**************************************************************************/
bool is_view_supported(enum ts_type type)
{
  switch (type) {
  case TS_ISOMETRIC:
  case TS_OVERHEAD:
    return TRUE;
  case TS_3D:
    return FALSE;
  }

  return FALSE;
}

/**********************************************************************//**
  Loading tileset of the specified type
**************************************************************************/
void tileset_type_set(enum ts_type type)
{
}

/**********************************************************************//**
  Load intro sprites. Not used in SDL-client.
**************************************************************************/
void load_intro_gfx(void)
{
  /* nothing */
}

/**********************************************************************//**
  Frees the introductory sprites.
**************************************************************************/
void free_intro_radar_sprites(void)
{
  /* nothing */
}

/**********************************************************************//**
  Create colored frame
**************************************************************************/
void create_frame(SDL_Surface *dest, Sint16 left, Sint16 top,
                  Sint16 width, Sint16 height,
                  SDL_Color *pcolor)
{
  struct color gsdl2_color = { .color = pcolor };
  struct sprite *vertical = create_sprite(1, height, &gsdl2_color);
  struct sprite *horizontal = create_sprite(width, 1, &gsdl2_color);
  SDL_Rect tmp,dst = { left, top, 0, 0 };

  tmp = dst;
  alphablit(vertical->psurface, NULL, dest, &tmp, 255);

  dst.x += width - 1;
  tmp = dst;
  alphablit(vertical->psurface, NULL, dest, &tmp, 255);

  dst.x = left;
  tmp = dst;
  alphablit(horizontal->psurface, NULL, dest, &tmp, 255);

  dst.y += height - 1;
  tmp = dst;
  alphablit(horizontal->psurface, NULL, dest, &tmp, 255);

  free_sprite(horizontal);
  free_sprite(vertical);
}

/**********************************************************************//**
  Create single colored line
**************************************************************************/
void create_line(SDL_Surface *dest, Sint16 x0, Sint16 y0, Sint16 x1, Sint16 y1,
                 SDL_Color *pcolor)
{
  int xl = x0 < x1 ? x0 : x1;
  int xr = x0 < x1 ? x1 : x0;
  int yt = y0 < y1 ? y0 : y1;
  int yb = y0 < y1 ? y1 : y0;
  int w = (xr - xl) + 1;
  int h = (yb - yt) + 1;
  struct sprite *spr;
  SDL_Rect dst = { xl, yt, w, h };
  SDL_Color *pcol;
  struct color gsdl2_color;
  int l = MAX((xr - xl) + 1, (yb - yt) + 1);
  int i;

  pcol = fc_malloc(sizeof(pcol));
  pcol->r = pcolor->r;
  pcol->g = pcolor->g;
  pcol->b = pcolor->b;
  pcol->a = 0; /* Fill with transparency */

  gsdl2_color.color = pcol;
  spr = create_sprite(w, h, &gsdl2_color);

  lock_surf(spr->psurface);

  /* Set off transparency from pixels belonging to the line */
  if ((x0 <= x1 && y0 <= y1)
      || (x1 <= x0 && y1 <= y0)) {
    for (i = 0; i < l; i++) {
      int cx = (xr - xl) * i / l;
      int cy = (yb - yt) * i / l;

      *((Uint32 *)spr->psurface->pixels + spr->psurface->w * cy + cx)
        |= (pcolor->a << spr->psurface->format->Ashift);
    }
  } else {
    for (i = 0; i < l; i++) {
      int cx = (xr - xl) * i / l;
      int cy = yb - (yb - yt) * i / l;

      *((Uint32 *)spr->psurface->pixels + spr->psurface->w * cy + cx)
        |= (pcolor->a << spr->psurface->format->Ashift);
    }
  }

  unlock_surf(spr->psurface);

  alphablit(spr->psurface, NULL, dest, &dst, 255);

  free_sprite(spr);
  free(pcol);
}
