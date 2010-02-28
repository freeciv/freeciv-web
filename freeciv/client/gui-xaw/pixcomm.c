/* $XConsortium: MenuButton.c,v 1.21 95/06/26 20:35:12 kaleb Exp $ */

/*
Copyright (c) 1989, 1994  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.
 *
 */


/***********************************************************************
 *
 * Pixcomm Widget
 *
 ***********************************************************************/

/*
 * Pixcomm.c - Source code for PixCommand widget.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xmu/Misc.h>
#include <X11/Xaw/XawInit.h>
#include <X11/Xmu/Converters.h>
#include <X11/extensions/shape.h>

#include "pixcomm.h"
#include "pixcommp.h"
#include "gui_main.h"
#include "gui_stuff.h"

static void Notify(Widget w, XEvent *event,
		   String *params, Cardinal *num_params);
static void ClassInitialize(void);

static void Realize(Widget w, Mask *valueMask,
		    XSetWindowAttributes *attributes);
static void Resize(Widget w);
static void Destroy(Widget w);

static XtActionsRec actionsList[] = {
  {"notify",		Notify}
};

#define Superclass ((CommandWidgetClass)&commandClassRec)
 
/****************************************************************
 *
 * Full class record constant
 *
 ****************************************************************/

/* Private Data */

PixcommClassRec pixcommClassRec = {
  {
    (WidgetClass) Superclass,		/* superclass		  */	
    "Pixcomm",			        /* class_name		  */
    sizeof(PixcommRec),       	        /* size			  */
    ClassInitialize,			/* class_initialize	  */
    NULL,				/* class_part_initialize  */
    FALSE,				/* class_inited		  */
    NULL,				/* initialize		  */
    NULL,				/* initialize_hook	  */
    Realize,			        /* realize		  */
    actionsList,	                /* actions		  */
    XtNumber(actionsList),              /* num_actions		  */
    NULL,			        /* resources		  */
    0,		                        /* resource_count	  */
    NULLQUARK,				/* xrm_class		  */
    FALSE,				/* compress_motion	  */
    TRUE,				/* compress_exposure	  */
    TRUE,				/* compress_enterleave    */
    FALSE,				/* visible_interest	  */
    Destroy,				/* destroy		  */
    Resize,			        /* resize		  */
    XtInheritExpose,			/* expose		  */
    NULL,				/* set_values		  */
    NULL,				/* set_values_hook	  */
    XtInheritSetValuesAlmost,		/* set_values_almost	  */
    NULL,				/* get_values_hook	  */
    NULL,				/* accept_focus		  */
    XtVersion,				/* version		  */
    NULL,				/* callback_private	  */
    XtInheritTranslations,              /* tm_table		  */
    XtInheritQueryGeometry,		/* query_geometry	  */
    XtInheritDisplayAccelerator,	/* display_accelerator	  */
    NULL				/* extension		  */
  },  /* CoreClass fields initialization */
  {
    XtInheritChangeSensitive		/* change_sensitive	  */ 
  },  /* SimpleClass fields initialization */
#if defined(HAVE_LIBXAW3D)
  {
    XtInheritXaw3dShadowDraw,           /* shadowdraw           */
  },  /* ThreeD Class fields initialization */
#endif
  {
    0,                                     /* field not used    */
  },  /* LabelClass fields initialization */
  {
    0,                                     /* field not used    */
  },  /* CommandClass fields initialization */
  {
    0,                                     /* field not used    */
  }  /* PixcommClass fields initialization */
};

  /* for public consumption */
WidgetClass pixcommWidgetClass = (WidgetClass) &pixcommClassRec;

/* ARGSUSED */
static void 
Notify(w,event,params,num_params)
Widget w;
XEvent *event;
String *params;         /* unused */
Cardinal *num_params;   /* unused */
{
  PixcommWidget pw = (PixcommWidget)w; 

  /* check to be sure state is still Set so that user can cancel
     the action (e.g. by moving outside the window, in the default
     bindings.
  */
  if (pw->command.set)
    XtCallCallbackList(w, pw->command.callbacks, (XtPointer) event);
}

static void Realize(Widget w, Mask *valueMask,
		    XSetWindowAttributes *attributes)
{
  PixcommWidget pw = (PixcommWidget) w;

  (*pixcommWidgetClass->core_class.superclass->core_class.realize)
	(w, valueMask, attributes);

  pw->pixcomm.back_store=XCreatePixmap(display, root_window, 
				       pw->core.width, pw->core.height, 
				       display_depth);
  XSetForeground(display, fill_bg_gc, pw->core.background_pixel);
  XFillRectangle(display, pw->pixcomm.back_store, fill_bg_gc, 
		 0, 0, pw->core.width, pw->core.height);
  XtVaSetValues(w, XtNbitmap, pw->pixcomm.back_store, NULL);
}



static void Destroy(Widget w)
{
  PixcommWidget pw=(PixcommWidget)w;

  if(!XtIsRealized(w))
    return;
  if(pw->pixcomm.back_store)
    XFreePixmap(display, pw->pixcomm.back_store);
}


static void Resize(Widget w)
{
  PixcommWidget pw=(PixcommWidget)w;

  if(!XtIsRealized(w))
    return;
  if(pw->pixcomm.back_store)
    XFreePixmap(display, pw->pixcomm.back_store);

  pw->pixcomm.back_store=XCreatePixmap(display, root_window, 
				       pw->core.width, pw->core.height, 
				       display_depth);
  
  XSetForeground(display, fill_bg_gc, pw->core.background_pixel);
  XFillRectangle(display, pw->pixcomm.back_store, fill_bg_gc, 
		 0, 0, pw->core.width, pw->core.height);
  XtVaSetValues(w, XtNbitmap, pw->pixcomm.back_store, NULL);
  (*pixcommWidgetClass->core_class.superclass->core_class.resize)(w);
}



/****************************************************************
 *
 * Private Procedures
 *
 ****************************************************************/
static void ClassInitialize(void)
{
    XawInitializeWidgetSet();
}

Pixmap XawPixcommPixmap(Widget w)
{
  PixcommWidget	pw = (PixcommWidget)w;
  return XtIsRealized(w) ? pw->pixcomm.back_store : 0;
}

void XawPixcommCopyTo(Widget w, Pixmap src)
{
  PixcommWidget pw = (PixcommWidget)w;
 
  XCopyArea(display, src, pw->pixcomm.back_store, 
	    civ_gc,
	    0, 0,
	    pw->core.width, pw->core.height,
	    0, 0);
  xaw_expose_now(w);
}

void XawPixcommClear(Widget w)
{
  PixcommWidget pw = (PixcommWidget)w;

  XSetForeground(display, fill_bg_gc, pw->core.background_pixel);
  XFillRectangle(display, pw->pixcomm.back_store, fill_bg_gc, 
		 0, 0, pw->core.width, pw->core.height);
  xaw_expose_now(w);
}
