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

#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/StringDefs.h>
#include <X11/keysym.h>
#include <X11/Shell.h>

#include "back_end.h"
#include "be_common_pixels.h"
#include "be_common_32.h"

#include "shared.h"

#include "mem.h"
#include "netintf.h"

// fixme
#include "timing.h"
#include "widget.h"

static Display *display;
static Window window;
static int screen_number;
static int display_depth;
static int x11fd;
static int other_fd = -1;
static XImage *root_image;
static GC gc_plain;

/*************************************************************************
  Initialize video mode and window.
*************************************************************************/
void be_init(const struct ct_size *screen_size, bool fullscreen)
{
  XGCValues values;

  _Xdebug = 0;
  display = XOpenDisplay(NULL);
  assert(display);

  screen_number = DefaultScreen(display);
  display_depth = DefaultDepth(display, screen_number);
  x11fd = ConnectionNumber(display);

  window =
      XCreateSimpleWindow(display, XDefaultRootWindow(display), 200, 200,
			  screen_size->width, screen_size->height, 0, 0,
			  WhitePixel(display, screen_number));
  XSelectInput(display, window,
	       ExposureMask | PointerMotionMask | ButtonPressMask |
	       ButtonReleaseMask | KeyPressMask);

  {
    Pixmap dummy_mask = XCreatePixmap(display, window, 1, 1, 1);

    values.graphics_exposures = False;
    values.foreground = BlackPixel(display, screen_number);

    values.graphics_exposures = False;
    gc_plain = XCreateGC(display, window, GCGraphicsExposures, &values);

    XFreePixmap(display, dummy_mask);
  }

  /* pop this window up on the screen */
  XMapRaised(display, window);

  root_image =
      XCreateImage(display, DefaultVisual(display, screen_number),
		   display_depth, ZPixmap, 0, NULL, screen_size->width,
		   screen_size->height, 32, 0);
  root_image->data =
      fc_malloc(root_image->bytes_per_line * root_image->height);
}

/*************************************************************************
  ...
*************************************************************************/
static bool copy_event(struct be_event *event, XEvent * xevent)
{
  switch (xevent->type) {
  case NoExpose:
  case GraphicsExpose:
    return FALSE;
  case Expose:
    {
      XExposeEvent *xev = &xevent->xexpose;

      if (xev->count > 0) {
	return FALSE;
      }
      event->type = BE_EXPOSE;
    }
    break;
  case MotionNotify:
    {
      XMotionEvent *xev = &xevent->xmotion;

      event->type = BE_MOUSE_MOTION;
      event->position.x = xev->x;
      event->position.y = xev->y;
    }
    break;
  case ButtonPress:
  case ButtonRelease:
    {
      XButtonEvent *xev = &xevent->xbutton;

#if 0
      printf("pos=(%d,%d) button=0x%x state=0x%x\n", xev->x, xev->y,
	     xev->button, xev->state);
#endif
      event->type =
	  xevent->type == ButtonPress ? BE_MOUSE_PRESSED : BE_MOUSE_RELEASED;
      event->position.x = xev->x;
      event->position.y = xev->y;
      switch (xev->button) {
      case 1:
	event->button = BE_MB_LEFT;
	break;
      case 2:
	event->button = BE_MB_MIDDLE;
	break;
      case 3:
	event->button = BE_MB_RIGHT;
	break;
      default:
	assert(0);
      }
    }
    break;
  case KeyPress:
    {
      XKeyEvent *xev = &xevent->xkey;
      XKeyEvent copy_xev = *xev;
      char string[10];
      KeySym key;
      struct be_key *k = &event->key;
      int chars;

      /* Mod2Mask is NumLock */
      copy_xev.state = copy_xev.state & (ShiftMask | Mod2Mask);
      chars = XLookupString(&copy_xev, string, sizeof(string), &key, NULL);

      if (0)
	printf("chars=%d string='%s' key=%ld cursor=%d\n", chars, string,
	       key, IsCursorKey(key));
      k->shift = (xev->state & ShiftMask);
      k->control = (xev->state & ControlMask);
      k->alt = (xev->state & Mod1Mask);

#define T(x,b)			\
    } else if (key == x) {	\
      k->type = b
	  
      if (FALSE) {
	T(XK_BackSpace, BE_KEY_BACKSPACE);
	T(XK_Return, BE_KEY_RETURN);
	T(XK_KP_Enter, BE_KEY_ENTER);
	T(XK_Delete, BE_KEY_DELETE);
	T(XK_Left, BE_KEY_LEFT);
	T(XK_Right, BE_KEY_RIGHT);
	T(XK_Up, BE_KEY_UP);
	T(XK_Down, BE_KEY_DOWN);
	T(XK_Escape, BE_KEY_ESCAPE);
	T(XK_Print, BE_KEY_PRINT);
	T(XK_space, BE_KEY_SPACE);
	T(XK_KP_0,BE_KEY_KP_0);
	T(XK_KP_1,BE_KEY_KP_1);
	T(XK_KP_2,BE_KEY_KP_2);
	T(XK_KP_3,BE_KEY_KP_3);
	T(XK_KP_4,BE_KEY_KP_4);
	T(XK_KP_5,BE_KEY_KP_5);
	T(XK_KP_6,BE_KEY_KP_6);
	T(XK_KP_7,BE_KEY_KP_7);
	T(XK_KP_8,BE_KEY_KP_8);
	T(XK_KP_9,BE_KEY_KP_9);
      } else if (chars == 1 && string[0] > ' ' && string[0] <= '~') {
	k->type = BE_KEY_NORMAL;
	k->key = string[0];
	k->shift = FALSE;
      } else {
	printf
	    ("WARNING: BE-X11: unconverted KeyPress: chars=%d string='%s' key=0x%lx\n",
	     chars, string, key);
	return FALSE;
      }
      event->type = BE_KEY_PRESSED;
      event->position.x = xev->x;
      event->position.y = xev->y;
    }
    break;
  default:
    printf("got event %d\n", xevent->type);
    assert(0);
  }
  return TRUE;
}

/*************************************************************************
  ...
*************************************************************************/
void be_next_non_blocking_event(struct be_event *event)
{
  XEvent xevent;

restart:
  event->type = BE_NO_EVENT;

  if (XCheckMaskEvent(display, -1 /*all events */ , &xevent)) {
    if (copy_event(event, &xevent)) {
      return;
    } else {
      /* discard event */
      goto restart;
    }
  }
}

/*************************************************************************
  ...
*************************************************************************/
void be_next_blocking_event(struct be_event *event, struct timeval *timeout)
{
  fd_set readfds, exceptfds;
  int ret, highest = x11fd;

restart:
  event->type = BE_NO_EVENT;

  /* No event available: block on input socket until one is */
  FD_ZERO(&readfds);
  FD_SET(x11fd, &readfds);

  FD_ZERO(&exceptfds);

  if (other_fd != -1) {
    FD_SET(other_fd, &readfds);
    FD_SET(other_fd, &exceptfds);
    if (other_fd > highest) {
      highest = other_fd;
    }
  }

  ret = my_select(highest + 1, &readfds, NULL, &exceptfds, timeout);
  if (ret == 0) {
    // timed out
    event->type = BE_TIMEOUT;
  } else if (ret > 0) {
    if (other_fd != -1 && (FD_ISSET(other_fd, &readfds) ||
			   FD_ISSET(other_fd, &exceptfds))) {
      event->type = BE_DATA_OTHER_FD;
      event->socket = other_fd;
    }
    /* 
     * New data on the x11 fd. return with BE_NO_EVENT and let the
     * caller handle it. 
     */
  } else if (errno == EINTR) {
    goto restart;
  } else {
    assert(0);
  }
}

/*************************************************************************
  ...
*************************************************************************/
void be_add_net_input(int sock)
{
  other_fd = sock;
}

/*************************************************************************
  ...
*************************************************************************/
void be_remove_net_input(void)
{
  other_fd = -1;
}

/*************************************************************************
  ...
*************************************************************************/
void be_screen_get_size(struct ct_size *size)
{
  XWindowAttributes window_attributes;

  XGetWindowAttributes(display, window, &window_attributes);

  size->width = window_attributes.width;
  size->height = window_attributes.height;
}

#define COMP_565_RED(x)		((((x)>>3)&0x1f)<<11)
#define COMP_565_GREEN(x)	((((x)>>2)&0x3f)<< 5)
#define COMP_565_BLUE(x)	((((x)>>3)&0x1f)<< 0)

/*************************************************************************
  ...
*************************************************************************/
static void fill_ximage_from_image_565(XImage * ximage,
				       const struct image *image,
                                        int start_x, int start_y,
                                        int width, int height)
{
  int x, y;
  unsigned short *pdest = (unsigned short *) (ximage->data + 
                            start_y * (ximage->bytes_per_line / 2) +
                            start_x);

  int extra_per_line = ximage->bytes_per_line / 2 - width;

  for (y = start_y; y < height; y++) {
    for (x = start_x; x < width; x++) {
      unsigned char *psrc = IMAGE_GET_ADDRESS(image, x, y);
      unsigned short new_value =
	  (COMP_565_RED(psrc[0]) | COMP_565_GREEN(psrc[1]) |
	   COMP_565_BLUE(psrc[2]));
      *pdest = new_value;
      pdest++;
    }
    pdest += extra_per_line;
  }
}

/*************************************************************************
  ...
*************************************************************************/
#define COMP_555_RED(x)		((((x)>>3)&0x1f)<<10)
#define COMP_555_GREEN(x)	((((x)>>3)&0x1f)<< 5)
#define COMP_555_BLUE(x)	((((x)>>3)&0x1f)<< 0)
static void fill_ximage_from_image_555(XImage * ximage,
				       const struct image *image,
                                        int start_x, int start_y,
                                        int width, int height)
{
  int x, y;
  unsigned short *pdest = (unsigned short *) (ximage->data + 
                            start_y * (ximage->bytes_per_line / 2) +
                            start_x);
  int extra_per_line = ximage->bytes_per_line / 2 - width;

  for (y = start_y; y < height; y++) {
    for (x = start_x; x < width; x++) {
      unsigned char *psrc = IMAGE_GET_ADDRESS(image, x, y);
      unsigned short new_value =
	  (COMP_555_RED(psrc[0]) | COMP_555_GREEN(psrc[1]) |
	   COMP_555_BLUE(psrc[2]));
      *pdest = new_value;
      pdest++;
    }
    pdest += extra_per_line;
  }
}

/*************************************************************************
  ...
*************************************************************************/
static void fill_ximage_from_image_8888(XImage * ximage,
                                        const struct image *image,
                                        int start_x, int start_y,
                                        int width, int height)
{
  int x, y;
  unsigned char *pdest = (unsigned char *) (ximage->data + 
                           start_y * ximage->bytes_per_line +
                           start_x * 4);
  int extra_per_line = ximage->bytes_per_line - width*4;

  for (y = start_y; y < height; y++) {
    for (x = start_x; x < width; x++) {
      unsigned char *psrc = IMAGE_GET_ADDRESS(image, x, y);

      pdest[0] = psrc[2];
      pdest[1] = psrc[1];
      pdest[2] = psrc[0];
      pdest += 4;
    }
    pdest += extra_per_line;
  }
}

/*************************************************************************
  ...
*************************************************************************/
void be_copy_osda_to_screen(struct osda *src, const struct ct_rect *rect)
{
  assert(root_image->width == src->image->width &&
	 root_image->height == src->image->height);

  struct ct_rect rect2;
  
  if (rect) {
    rect2 = *rect;
  } else {
    rect2 = (struct ct_rect){0, 0, src->image->width, src->image->height};
  }

  if (root_image->red_mask == 0xf800 &&
      root_image->green_mask == 0x7e0 && root_image->blue_mask == 0x1f &&
      root_image->depth == 16 && root_image->bits_per_pixel == 16
      && root_image->byte_order == LSBFirst) {
    fill_ximage_from_image_565(root_image, src->image, rect2.x, rect2.y,
                               rect2.width, rect2.height);
  } else if (root_image->red_mask == 0x7c00 &&
	     root_image->green_mask == 0x3e0 && root_image->blue_mask == 0x1f
	     && root_image->depth == 15 && root_image->bits_per_pixel == 16
	     && root_image->byte_order == LSBFirst) {
    fill_ximage_from_image_555(root_image, src->image, rect2.x, rect2.y,
                               rect2.width, rect2.height);
  } else if (root_image->red_mask == 0xff0000 &&
	     root_image->green_mask == 0xff00
	     && root_image->blue_mask == 0xff && root_image->depth == 24
	     && root_image->bits_per_pixel == 32
	     && root_image->byte_order == LSBFirst) {
    fill_ximage_from_image_8888(root_image, src->image, rect2.x, rect2.y,
                                rect2.width, rect2.height);
  } else {
    fprintf(stderr, "ERROR: unknown screen format: red=0x%lx, "
	    "green=0x%lx, blue=0x%lx depth=%d bpp=%d "
	    "byte_order=%d (LSB=%d MSB=%d)\n",
	    root_image->red_mask, root_image->green_mask,
	    root_image->blue_mask, root_image->depth,
	    root_image->bits_per_pixel, root_image->byte_order, LSBFirst,
	    MSBFirst);
    assert(0);
  }

  XPutImage(display, window, gc_plain, root_image, rect2.x, rect2.y,
            rect2.x, rect2.y, rect2.width, rect2.height);
  XFlush(display);
}

/*************************************************************************
  ...
*************************************************************************/
bool be_supports_fullscreen(void)
{
    return FALSE;
}
