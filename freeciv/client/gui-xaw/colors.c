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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "fcintl.h"
#include "log.h"
#include "mem.h"

#include "gui_main.h"

#include "colors.h"

Colormap cmap;

/* This is just so we can print the visual class intelligibly */
/*static char *visual_class[] = {
   "StaticGray",
   "GrayScale",
   "StaticColor",
   "PseudoColor",
   "TrueColor",
   "DirectColor"
};
*/

void alloc_color_for_colormap(XColor *color);

/****************************************************************************
  Allocate a color (adjusting it for our colormap if necessary on paletted
  systems) and return a pointer to it.
****************************************************************************/
struct color *color_alloc(int r, int g, int b)
{
  XColor mycolor;
  struct color *color = fc_malloc(sizeof(*color));

  mycolor.red = r << 8;
  mycolor.green = g << 8;
  mycolor.blue = b << 8;

  alloc_color_for_colormap(&mycolor);

  color->color.pixel = mycolor.pixel;
  return color;
}

/****************************************************************************
  Free a previously allocated color.  See color_alloc.
****************************************************************************/
void color_free(struct color *color)
{
  free(color);
}


#if 0
/*************************************************************
...
*************************************************************/
static void alloc_standard_colors(void)
{
  XColor mycolors[COLOR_STD_LAST];
  int i;

  for(i=0; i<COLOR_STD_LAST; i++) {
    mycolors[i].red = colors_standard_rgb[i].r << 8;
    mycolors[i].green = colors_standard_rgb[i].g << 8;
    mycolors[i].blue =  colors_standard_rgb[i].b << 8;
  }

  alloc_colors(mycolors, COLOR_STD_LAST);  

  for (i = 0; i < COLOR_STD_LAST; i++) {
    colors_standard[i] = mycolors[i].pixel;
  }
}
#endif

/*************************************************************
...
*************************************************************/
enum Display_color_type get_visual(void)
{
  int i, default_depth;
  Visual *default_visual;
  XVisualInfo visual_info; 

  /* Try to allocate colors for PseudoColor, TrueColor,
   * DirectColor, and StaticColor; use black and white
   * for StaticGray and GrayScale */
  default_depth = DefaultDepth(display, screen_number);
  default_visual = DefaultVisual(display, screen_number);

  if (default_depth == 1) { 
    /* Must be StaticGray, use black and white */
    freelog(LOG_VERBOSE, "found B/W display.");
    return BW_DISPLAY;
  }

  i=5;
  
  while(!XMatchVisualInfo(display, screen_number, 
			  default_depth, i--, &visual_info));
  

/*
  freelog(LOG_VERBOSE, "Found a %s class visual at default depth.",
      visual_class[++i]);
*/
   
  if(i < StaticColor) { /* Color visual classes are 2 to 5 */
    /* No color visual available at default depth;
     * some applications might call XMatchVisualInfo
     * here to try for a GrayScale visual if they
     * can use gray to advantage, before giving up
     * and using black and white */
    freelog(LOG_VERBOSE, "found grayscale(?) display.");
    return GRAYSCALE_DISPLAY;
  }

   /* Otherwise, got a color visual at default depth */
   /* The visual we found is not necessarily the default
    * visual, and therefore it is not necessarily the one
    * we used to create our window; however, we now know
    * for sure that color is supported, so the following
    * code will work (or fail in a controlled way) */
   /* Let's check just out of curiosity: */
  if (visual_info.visual != default_visual) {
/*    freelog(LOG_VERBOSE, "Found: %s class visual at default depth",
	visual_class[i]); */
  }

  freelog(LOG_VERBOSE, "color system booted ok.");

  return COLOR_DISPLAY;
}


#if 0
/*************************************************************
...
*************************************************************/
void init_color_system(void)
{
  cmap=DefaultColormap(display, screen_number);

  alloc_standard_colors();
}
#endif

/*************************************************************
  Allocate all needed colors given in the array.
*************************************************************/
void alloc_colors(XColor *colors, int ncols)
{
  int i;

  if (!cmap) {
    cmap = DefaultColormap(display, screen_number);
  }

  for (i = 0; i < ncols; i++) {
    if (!XAllocColor(display, cmap, &colors[i])) {
      /* We're out of colors.  For the rest of the palette, just
       * find the closest match and use it.  We could instead try
       * to use a private colormap, but this is ugly, takes extra
       * code, and has no guarantee of getting better performance
       * unless we do a lot of work to optimize the colormap. */
      XColor *cells;
      int ncells, j;

      ncells = DisplayCells(display, screen_number);
      cells = fc_malloc(sizeof(XColor) * ncells);

      for (j = 0; j < ncells; j++) {
        cells[j].pixel = j;
      }

      /* We need to lock the server so that the colors don't change
       * while we're searching through them. */
      XGrabServer(display);

      XQueryColors(display, cmap, cells, ncells);

      for (; i < ncols; i++) {
        int best = INT_MAX;
        unsigned long pixel = 0;

	/* Find the best match among all available colors. */
        for (j = 0; j < ncells; j++) {
          int rd, gd, bd, dist;
          
          rd = (cells[j].red - colors[i].red) >> 8;
          gd = (cells[j].green - colors[i].green) >> 8;
          bd = (cells[j].blue - colors[i].blue) >> 8;
          dist = rd * rd + gd * gd + bd * bd;
          
          if (dist < best) {
            best = dist;
            pixel = j;
          }
        }

  	XAllocColor(display, cmap, &cells[pixel]);
        colors[i].pixel = pixel;
      }

      /* Unlock the server, since we're done querying it. */
      XUngrabServer(display);

      free(cells);
      break;
    }
  }
}

/*************************************************************
  Alloc given color for our colormap.
*************************************************************/
void alloc_color_for_colormap(XColor *color)
{
  if (!cmap) {
    cmap = DefaultColormap(display, screen_number);
  }

  if (!XAllocColor(display, cmap, color)) {
    /* We're out of colors.  For the rest of the palette, just
     * find the closest match and use it.  We could instead try
     * to use a private colormap, but this is ugly, takes extra
     * code, and has no guarantee of getting better performance
     * unless we do a lot of work to optimize the colormap. */
    XColor *cells;
    int ncells, j;
    int best = INT_MAX;
    unsigned long pixel = 0;

    ncells = DisplayCells(display, screen_number);
    cells = fc_malloc(sizeof(XColor) * ncells);

    for (j = 0; j < ncells; j++) {
      cells[j].pixel = j;
    }

    /* We need to lock the server so that the colors don't change
     * while we're searching through them. */
    XGrabServer(display);

    XQueryColors(display, cmap, cells, ncells);

    /* Find the best match among all available colors. */
    for (j = 0; j < ncells; j++) {
      int rd, gd, bd, dist;

      rd = (cells[j].red - color->red) >> 8;
      gd = (cells[j].green - color->green) >> 8;
      bd = (cells[j].blue - color->blue) >> 8;
      dist = rd * rd + gd * gd + bd * bd;

      if (dist < best) {
	best = dist;
	pixel = j;
      }
    }

    XAllocColor(display, cmap, &cells[pixel]);
    color->pixel = pixel;

    /* Unlock the server, since we're done querying it. */
    XUngrabServer(display);

    free(cells);
  }
}



/*************************************************************
...
*************************************************************/
void free_colors(unsigned long *pixels, int ncols)
{
#if 0
  XFreeColors(display, cmap, pixels, ncols, 0);
#endif
}
