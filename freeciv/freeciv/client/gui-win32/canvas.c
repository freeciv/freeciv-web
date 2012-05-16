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
#include <windowsx.h>

/* utility */
#include "log.h"
#include "fcintl.h"

/* client */
#include "options.h"

/* gui-win32 */
#include "canvas.h"
#include "colors.h"
#include "graphics.h"
#include "gui_main.h"
#include "sprite.h"

static HFONT *fonts[FONT_COUNT] = {&main_font,
				   &city_descriptions_font};

/***************************************************************************
 ...
***************************************************************************/
static HDC canvas_get_hdc(struct canvas *pcanvas)
{
  if (!pcanvas->hdc) {
    switch(pcanvas->type) {
      case CANVAS_BITMAP:
	pcanvas->hdc = CreateCompatibleDC(NULL);
	pcanvas->tmp = SelectObject(pcanvas->hdc, pcanvas->bmp);
	break;
      case CANVAS_WINDOW:
	pcanvas->hdc = GetDC(pcanvas->wnd);
	pcanvas->tmp = (void *)1; /* non-null */
	break;
      default:
	break;
    }
  }

  return pcanvas->hdc;
}

/***************************************************************************
 ...
***************************************************************************/
static void canvas_release_hdc(struct canvas *pcanvas)
{
  if (pcanvas->tmp) {
    switch(pcanvas->type) {
      case CANVAS_BITMAP:
	SelectObject(pcanvas->hdc, pcanvas->tmp);
	DeleteDC(pcanvas->hdc);
	pcanvas->hdc = NULL;
	break;
      case CANVAS_WINDOW:
	ReleaseDC(pcanvas->wnd, pcanvas->hdc);
	pcanvas->hdc = NULL;
	break;
      default:
	break;
    }
    pcanvas->tmp = NULL;
  }
}

/***************************************************************************
  Create a canvas of the given size.
***************************************************************************/
struct canvas *canvas_create(int width, int height)
{
  struct canvas *result = fc_malloc(sizeof(*result));
  HDC hdc;
  hdc = GetDC(root_window);
  result->type = CANVAS_BITMAP;
  result->hdc = NULL;
  result->bmp = CreateCompatibleBitmap(hdc, width, height);
  result->wnd = NULL;
  result->tmp = NULL;
  ReleaseDC(root_window, hdc);
  return result;
}

/***************************************************************************
  Free any resources associated with this canvas and the canvas struct
  itself.
***************************************************************************/
void canvas_free(struct canvas *store)
{
  DeleteObject(store->bmp);
  free(store);
}

static bool overview_canvas_init = FALSE;
static bool mapview_canvas_init = FALSE;
static struct canvas overview_canvas;
static struct canvas mapview_canvas;

/****************************************************************************
  Return a canvas that is the overview window.
****************************************************************************/
struct canvas *get_overview_window(void)
{
  if (!overview_canvas_init) {
    overview_canvas.type = CANVAS_WINDOW;
    overview_canvas.hdc = NULL;
    overview_canvas.bmp = NULL;
    overview_canvas.wnd = root_window;
    overview_canvas.tmp = NULL;
    overview_canvas_init = TRUE;
  }
  return &overview_canvas;
}

/****************************************************************************
  Return a canvas that is the mapview window.
****************************************************************************/
struct canvas *get_mapview_window(void)
{
  if (!mapview_canvas_init) {
    mapview_canvas.type = CANVAS_WINDOW;
    mapview_canvas.hdc = NULL;
    mapview_canvas.bmp = NULL;
    mapview_canvas.wnd = map_window;
    mapview_canvas.tmp = NULL;
    mapview_canvas_init = TRUE;
  }
  return &mapview_canvas;
}

/***************************************************************************
  Copies an area from the source canvas to the destination canvas.
***************************************************************************/
void canvas_copy(struct canvas *dst, struct canvas *src,
		 int src_x, int src_y, int dst_x, int dst_y,
		 int width, int height)
{
  HDC hdcsrc = canvas_get_hdc(src);
  HDC hdcdst = canvas_get_hdc(dst);

  BitBlt(hdcdst, dst_x, dst_y, width, height, hdcsrc, src_x, src_y, SRCCOPY);

  canvas_release_hdc(src);
  canvas_release_hdc(dst);
}

/****************************************************************************
  Return the size of the given text in the given font.  This size should
  include the ascent and descent of the text.  Either of width or height
  may be NULL in which case those values simply shouldn't be filled out.
****************************************************************************/
void get_text_size(int *width, int *height,
		   enum client_font font, const char *text)
{
  RECT rc;
  HDC hdc;


  hdc = GetDC(root_window);

  DrawText(hdc, text, strlen(text), &rc, DT_CALCRECT);

  ReleaseDC(root_window, hdc);

  if (width) {
    *width = rc.right - rc.left + 1;
  }
  if (height) {
    *height = rc.bottom - rc.top + 1;
  }
}

/****************************************************************************
  Draw the text onto the canvas in the given color and font.  The canvas
  position does not account for the ascent of the text; this function must
  take care of this manually.  The text will not be NULL but may be empty.
****************************************************************************/
void canvas_put_text(struct canvas *pcanvas, int canvas_x, int canvas_y,
		     enum client_font font, struct color *pcolor,
		     const char *text)
{
  RECT rc;
  HDC hdc;
  HGDIOBJ temp;

  hdc = canvas_get_hdc(pcanvas);

  temp = SelectObject(hdc, *fonts[font]);

  SetBkMode(hdc, TRANSPARENT);

  rc.left = canvas_x;
  rc.right = canvas_x + 20;
  rc.top = canvas_y + 1;
  rc.bottom = canvas_y + 21;

  SetTextColor(hdc, RGB(0, 0, 0));
  DrawText(hdc, text, strlen(text), &rc, DT_NOCLIP);

  rc.left++;
  rc.right++;
  rc.top--;
  rc.bottom--;
  SetTextColor(hdc, pcolor->rgb);
  DrawText(hdc, text, strlen(text), &rc, DT_NOCLIP);

  SelectObject(hdc, temp);

  canvas_release_hdc(pcanvas);
}

/**************************************************************************
  Draw some or all of a sprite onto the canvas.
**************************************************************************/
void canvas_put_sprite(struct canvas *pcanvas,
		       int canvas_x, int canvas_y,
		       struct sprite *sprite,
		       int offset_x, int offset_y, int width, int height)
{
  HDC hdc = canvas_get_hdc(pcanvas);

  draw_sprite_part(sprite, hdc, canvas_x, canvas_y, width, height, offset_x,
		   offset_y);

  canvas_release_hdc(pcanvas);
}

/**************************************************************************
  Draw a full sprite onto the canvas.
**************************************************************************/
void canvas_put_sprite_full(struct canvas *pcanvas,
			    int canvas_x, int canvas_y,
			    struct sprite *sprite)
{
  HDC hdc = canvas_get_hdc(pcanvas);

  draw_sprite(sprite, hdc, canvas_x, canvas_y);

  canvas_release_hdc(pcanvas);
}

/****************************************************************************
  Draw a full sprite onto the canvas.  If "fog" is specified draw it with
  fog.
****************************************************************************/
void canvas_put_sprite_fogged(struct canvas *pcanvas,
			      int canvas_x, int canvas_y,
			      struct sprite *psprite,
			      bool fog, int fog_x, int fog_y)
{
  HDC hdc;

  if (!psprite)
    return;

  if (fog && gui_win32_better_fog && !psprite->fog) {
    fog_sprite(psprite);
    if (!psprite->fog) {
      freelog(LOG_NORMAL,
	      _("Better fog will only work in truecolor.  Disabling it"));
      gui_win32_better_fog = FALSE;
    }
  }

  hdc = canvas_get_hdc(pcanvas);

  if (fog && gui_win32_better_fog) {
    draw_sprite_fog(psprite, hdc, canvas_x, canvas_y);
  } else {
    draw_sprite(psprite, hdc, canvas_x, canvas_y);
    if (fog) {
      draw_fog(psprite, hdc, canvas_x, canvas_y);
    }
  }

  canvas_release_hdc(pcanvas);
}

/**************************************************************************
  Draw a filled-in colored rectangle onto the canvas.
**************************************************************************/
void canvas_put_rectangle(struct canvas *pcanvas, struct color *pcolor,
			  int canvas_x, int canvas_y, int width, int height)
{
  HDC hdc = canvas_get_hdc(pcanvas);
  RECT rect;
  HBRUSH brush;

  /* FillRect doesn't fill bottom and right edges, however canvas_x + width
   * and canvas_y + height are each 1 larger than necessary. */
  SetRect(&rect, canvas_x, canvas_y, canvas_x + width,
		 canvas_y + height);

  brush = brush_alloc(pcolor);

  FillRect(hdc, &rect, brush);

  brush_free(brush);

  canvas_release_hdc(pcanvas);
}

/****************************************************************************
  Fill the area covered by the sprite with the given color.
****************************************************************************/
void canvas_fog_sprite_area(struct canvas *pcanvas, struct sprite *psprite,
			    int canvas_x, int canvas_y)
{
  /* PORTME */
}

/**************************************************************************
  Draw a 1-pixel-width colored line onto the canvas.
**************************************************************************/
void canvas_put_line(struct canvas *pcanvas, struct color *pcolor,
		     enum line_type ltype, int start_x, int start_y,
		     int dx, int dy)
{
  HDC hdc = canvas_get_hdc(pcanvas);
  HPEN old_pen, pen;

  /* FIXME: set line type (size). */
  pen = pen_alloc(pcolor);
  old_pen = SelectObject(hdc, pen);
  MoveToEx(hdc, start_x, start_y, NULL);
  LineTo(hdc, start_x + dx, start_y + dy);
  SelectObject(hdc, old_pen);
  pen_free(pen);

  canvas_release_hdc(pcanvas);
}

/**************************************************************************
  Draw a 1-pixel-width colored curved line onto the canvas.
**************************************************************************/
void canvas_put_curved_line(struct canvas *pcanvas, struct color *pcolor,
                            enum line_type ltype, int start_x, int start_y,
                            int dx, int dy)
{
  /* FIXME: Implement curved line drawing. */
  canvas_put_line(pcanvas, pcolor, ltype, start_x, start_y, dx, dy);
}
