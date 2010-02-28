/**********************************************************************
 Freeciv - Copyright (C) 2004 - The Freeciv Project
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
#include <errno.h>
#include <stdio.h>

#include "SDL.h"

#include "back_end.h"
#include "be_common_pixels.h"
#include "be_common_32.h"

#include "shared.h"

#include "fcintl.h"
#include "log.h"
#include "mem.h"

static SDL_Surface *screen;

/* SDL interprets each pixel as a 32-bit number, so our masks must depend
 * on the endianness (byte order) of the machine */
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
static Uint32 rmask = 0xff000000;
static Uint32 gmask = 0x00ff0000;
static Uint32 bmask = 0x0000ff00;
static Uint32 amask = 0x000000ff;
#else
static Uint32 rmask = 0x000000ff;
static Uint32 gmask = 0x0000ff00;
static Uint32 bmask = 0x00ff0000;
static Uint32 amask = 0xff000000;
#endif

/*************************************************************************
  Initialize video mode and SDL.
*************************************************************************/
void be_init(const struct ct_size *screen_size, bool fullscreen)
{
  Uint32 flags = SDL_SWSURFACE | (fullscreen ? SDL_FULLSCREEN : 0)
                 | SDL_ANYFORMAT;

  char device[20];

  /* auto center new windows in X enviroment */
  putenv((char *) "SDL_VIDEO_CENTERED=yes");

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) < 0) {
    freelog(LOG_FATAL, _("Unable to initialize SDL library: %s"),
	    SDL_GetError());
    exit(EXIT_FAILURE);
  }
  atexit(SDL_Quit);

  freelog(LOG_NORMAL, _("Using Video Output: %s"),
	  SDL_VideoDriverName(device, sizeof(device)));
  {
    const SDL_VideoInfo *info = SDL_GetVideoInfo();
    freelog(LOG_VERBOSE, "Video memory of driver: %dkb", info->video_mem);
    freelog(LOG_VERBOSE, "Preferred depth: %d bits per pixel",
	    info->vfmt->BitsPerPixel);
  }

#if 0
  {
    SDL_Rect **modes;
    int i;

    modes = SDL_ListModes(NULL, flags);
    if (modes == (SDL_Rect **) 0) {
      printf("No modes available!\n");
      exit(-1);
    }
    if (modes == (SDL_Rect **) - 1) {
      printf("All resolutions available.\n");
    } else {
      /* Print valid modes */
      printf("Available Modes\n");
      for (i = 0; modes[i]; ++i)
	printf("  %d x %d\n", modes[i]->w, modes[i]->h);
    }
  }
#endif

  screen =
      SDL_SetVideoMode(screen_size->width, screen_size->height, 32, flags);
  if (screen == NULL) {
    freelog(LOG_FATAL, _("Can't set video mode: %s"), SDL_GetError());
    exit(1);
  }

  freelog(LOG_VERBOSE, "Got a screen with size (%dx%d) and %d bits per pixel",
	  screen->w, screen->h, screen->format->BitsPerPixel);
  freelog(LOG_VERBOSE, "  format: red=0x%x green=0x%x blue=0x%x mask=0x%x",
	  screen->format->Rmask, screen->format->Gmask,
	  screen->format->Bmask, screen->format->Amask);
  freelog(LOG_VERBOSE, "  format: bits-per-pixel=%d bytes-per-pixel=%d",
	  screen->format->BitsPerPixel, screen->format->BytesPerPixel);
  SDL_EnableUNICODE(1);
  SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
#if 0
  SDL_EventState(SDL_KEYUP, SDL_IGNORE);
  SDL_EventState(SDL_ACTIVEEVENT, SDL_IGNORE);
#endif
}

/*************************************************************************
  Copy our osda to the screen.  No alpha-blending here.
*************************************************************************/
void be_copy_osda_to_screen(struct osda *src, const struct ct_rect *rect)
{
  SDL_Surface *buf;
  SDL_Rect rect2;
  
  if (rect) {
    rect2 = (SDL_Rect){rect->x, rect->y, rect->width, rect->height};
  } else {
    rect2 = (SDL_Rect){0, 0, src->image->width, src->image->height};
  }

  buf = SDL_CreateRGBSurfaceFrom(src->image->data, src->image->width,
                                 src->image->height, 32, src->image->pitch,
                                 rmask, gmask, bmask, amask);
  SDL_BlitSurface(buf, &rect2, screen, &rect2);
  SDL_FreeSurface(buf);
  
  SDL_UpdateRect(screen, rect2.x, rect2.y, rect2.w, rect2.h);;
}

/*************************************************************************
  ...
*************************************************************************/
void be_screen_get_size(struct ct_size *size)
{
  size->width = screen->w;
  size->height = screen->h;
}
