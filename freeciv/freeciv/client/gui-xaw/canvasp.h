/*

    CanvasP.h - private header file for the Canvas Widget
    -  a widget that allows programmer-specified refresh procedures.
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

#ifndef _CanvasP_h
#define _CanvasP_h

#include <X11/IntrinsicP.h>
#include <X11/CoreP.h>
#include <X11/Xaw/SimpleP.h>

#include "canvas.h"

typedef struct {
  char	dummy;	/* some stupid compilers barf on empty structures */
} CanvasClassPart;


typedef struct _PixlabelClassRec {
  CoreClassPart       core_class;
  SimpleClassPart     simple_class;
  CanvasClassPart     canvas_class;
} CanvasClassRec;

extern CanvasClassRec canvasClassRec;


typedef struct {
  XfwfCanvasExposeProc	redraw;
  XtPointer		redraw_data;
  XfwfCanvasResizeProc	resize;
  XtPointer		resize_data;
  Pixmap                pixmap;
  Visual		*visual;
  int                   is_visible;
} CanvasPart;

typedef struct _CanvasRec {
  CorePart    core;
  SimplePart  simple;
  CanvasPart  canvas;
} CanvasRec;

#endif /* _CanvasP_h */
