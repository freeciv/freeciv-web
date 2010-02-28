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

#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>

#include "back_end.h"
#include "be_common_pixels.h"
#include "be_common_cairo_32.h"

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
static struct ct_size _screen_size;
static cairo_surface_t *screen_surface;
static int x11fd;
static int other_fd = -1;

/*************************************************************************
  Initialize video mode and window.
*************************************************************************/
void be_init(const struct ct_size *screen_size, bool fullscreen)
{
  _screen_size = *screen_size;
	
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

  screen_surface = cairo_xlib_surface_create(display, window,
                                  DefaultVisual(display, screen_number),
		  		  _screen_size.width, _screen_size.height);
  
  /* pop this window up on the screen */
  XMapRaised(display, window);
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

  ret = fc_select(highest + 1, &readfds, NULL, &exceptfds, timeout);
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

/*************************************************************************
  Create a cairo surface
*************************************************************************/
cairo_surface_t *be_cairo_surface_create(int width, int height) {
  cairo_t *cr;
  cairo_surface_t *surface;
  
  surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
  
  cr = cairo_create(surface);
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);
  cairo_destroy(cr);
  
  return surface;
}

/*************************************************************************
  ...
*************************************************************************/
void be_copy_osda_to_screen(struct osda *src, const struct ct_rect *rect)
{
  assert((src->image->width == _screen_size.width) &&
	     (src->image->height == _screen_size.height));

  cairo_t *cr;
  
  struct ct_rect rect2;
  
  if (rect) {
    rect2 = *rect;
  } else {
    rect2 = (struct ct_rect){0, 0, src->image->width, src->image->height};
  }
  
  cr = cairo_create(screen_surface);

  cairo_rectangle(cr, rect2.x, rect2.y, rect2.width, rect2.height);
  cairo_clip(cr);

  cairo_translate(cr, rect2.x, rect2.y);
  
  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);	
  cairo_set_source_surface(cr, src->image->data, -rect2.x, -rect2.y);
  cairo_paint(cr);
	
  cairo_destroy(cr);
  
  XFlush(display);
}

/*************************************************************************
  ...
*************************************************************************/
bool be_supports_fullscreen(void)
{
    return FALSE;
}
