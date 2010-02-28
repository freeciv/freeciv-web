/*

    Canvas.c - a widget that allows programmer-specified refresh procedures.
    Copyright (C) 1990,93,94 Robert H. Forsman Jr.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/XawInit.h>

#include "gui_main.h"
#include "gui_stuff.h"
#include "canvasp.h"

#define offset(field) XtOffset(CanvasWidget, canvas.field)

static XtResource resources[] = {
  {XtNexposeProc, XtCExposeProc, XtRFunction, sizeof(XfwfCanvasExposeProc),
     offset(redraw),	XtRFunction, NULL},
  {XtNexposeProcData, XtCExposeProcData, XtRPointer, sizeof(XtPointer),
     offset(redraw_data), XtRFunction, NULL},
  {XtNresizeProc, XtCResizeProc, XtRFunction, sizeof(XfwfCanvasResizeProc),
     offset(resize),	XtRFunction, NULL},
  {XtNresizeProcData, XtCResizeProcData, XtRPointer, sizeof(XtPointer),
     offset(resize_data), XtRFunction, NULL},
  {XtNvisual, XtCVisual, XtRVisual, sizeof(Visual*),
      offset(visual), XtRImmediate, CopyFromParent},
  {XtNBackPixmap, XtCBackPixmap, XtRBitmap, sizeof(Pixmap),
     offset(pixmap), XtRBitmap, NULL}
};


static void CanvasRealize(Widget widget, XtValueMask *value_mask, 
			  XSetWindowAttributes *attributes);
static void Redisplay(Widget w, XEvent *event, Region region);
static void ClassInitialize(void);
static void Resize(Widget w);
static void Destroy(Widget w);
static Boolean SetValues(Widget current,
			 Widget request,
			 Widget New,
			 ArgList args, Cardinal *nargs);

CanvasClassRec canvasClassRec = {
    {
    /* core_class fields	 */
    /* superclass	  	 */ (WidgetClass) &simpleClassRec,
    /* class_name	  	 */ "Canvas",
    /* widget_size	  	 */ sizeof(CanvasRec),
    /* class_initialize   	 */ ClassInitialize,
    /* class_part_initialize	 */ NULL,
    /* class_inited       	 */ False,
    /* initialize	  	 */ NULL,
    /* initialize_hook		 */ NULL,
    /* realize		  	 */ CanvasRealize,
    /* actions		  	 */ NULL,
    /* num_actions	  	 */ 0,
    /* resources	  	 */ resources,
    /* num_resources	  	 */ XtNumber(resources),
    /* xrm_class	  	 */ NULLQUARK,
    /* compress_motion	  	 */ True,
    /* compress_exposure  	 */ XtExposeCompressMultiple,
    /* compress_enterleave	 */ True,
    /* visible_interest	  	 */ True,
    /* destroy		  	 */ Destroy,
    /* resize		  	 */ Resize,
    /* expose		  	 */ Redisplay,
    /* set_values	  	 */ SetValues,
    /* set_values_hook		 */ NULL,
    /* set_values_almost	 */ XtInheritSetValuesAlmost,
    /* get_values_hook		 */ NULL,
    /* accept_focus	 	 */ NULL,
    /* version			 */ XtVersion,
    /* callback_private   	 */ NULL,
    /* tm_table		   	 */ NULL,
    /* query_geometry		 */ NULL,
    /* display_accelerator       */ XtInheritDisplayAccelerator,
    /* extension                 */ NULL
    },
    {
      XtInheritChangeSensitive            /* change_sensitive       */ 
    },  /* SimpleClass fields initialization */
    {
      0 /* some stupid compilers barf on empty structures */
    },
};

WidgetClass xfwfcanvasWidgetClass = (WidgetClass) & canvasClassRec;


static void CanvasRealize(Widget widget, XtValueMask *value_mask, 
			  XSetWindowAttributes *attributes)
{
  CanvasWidget	cw = (CanvasWidget)widget;
  cw->canvas.is_visible=0;
  
  XtCreateWindow(widget, (unsigned int) InputOutput,
	(Visual *) cw->canvas.visual, *value_mask, attributes);

  cw->canvas.pixmap=XCreatePixmap(display, root_window, 
				  cw->core.width, cw->core.height, 
				  display_depth);  

} /* CoreRealize */

static void Redisplay(Widget w, XEvent *event, Region region)
{
  XExposeEvent *exposeEvent = (XExposeEvent *)event;
  CanvasWidget	cw = (CanvasWidget)w;

  if (!XtIsRealized(w))
    return;

  if(cw->canvas.redraw) {
    (cw->canvas.redraw)((Widget)cw,exposeEvent,region,cw->canvas.redraw_data);
  }
  else {
    if(cw->canvas.is_visible)
      XCopyArea(display, cw->canvas.pixmap, XtWindow(w),
		civ_gc,
		0, 0,
		cw->core.width, cw->core.height,
		0, 0);
    else {
      XSetForeground(display, fill_bg_gc, cw->core.background_pixel);
      XFillRectangle(display, XtWindow(w), fill_bg_gc, 
		     0, 0, cw->core.width, cw->core.height);
    }
  }
}

static Boolean SetValues(Widget current,
			 Widget request,
			 Widget New,
			 ArgList args, Cardinal *nargs)
{
  int	i;
  for(i=0; i<*nargs; i++) {
    if (strcmp(XtNexposeProc,args[i].name)==0 ||
	strcmp(XtNexposeProcData,args[i].name)==0)
      return True;
  }
  return False;
}


static void Resize(Widget w)
{
  CanvasWidget cw = (CanvasWidget)w;
  if (cw->canvas.resize)
    (cw->canvas.resize)((Widget)cw, cw->canvas.resize_data);

  XFreePixmap(display, cw->canvas.pixmap);
  
  cw->canvas.pixmap=XCreatePixmap(display, root_window, 
				  cw->core.width, cw->core.height, 
				  display_depth);
}

static void Destroy(Widget w)
{
  CanvasWidget	cw = (CanvasWidget)w;
  XFreePixmap(display, cw->canvas.pixmap);
}

static void ClassInitialize(void)
{
    XawInitializeWidgetSet();
}


Pixmap canvas_get_backpixmap(Widget w)
{
  CanvasWidget	cw = (CanvasWidget)w;
  return XtIsRealized(w) ? cw->canvas.pixmap : 0;
}

void canvas_copy_to_backpixmap(Widget w, Pixmap src)
{
  CanvasWidget cw = (CanvasWidget)w;
 
  XCopyArea(display, src, cw->canvas.pixmap, 
	    civ_gc,
	    0, 0,
	    cw->core.width, cw->core.height,
	    0, 0);
  cw->canvas.is_visible=1;
  xaw_expose_now(w);
}


void canvas_show(Widget w)
{
  CanvasWidget	cw = (CanvasWidget)w;
  cw->canvas.is_visible=1;
  xaw_expose_now(w);
}

void canvas_hide(Widget w)
{
  CanvasWidget	cw = (CanvasWidget)w;
  cw->canvas.is_visible=0;
  xaw_expose_now(w);
}
