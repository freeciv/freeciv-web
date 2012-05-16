/* $XConsortium: MenuButtoP.h,v 1.8 94/04/17 20:12:18 converse Exp $
 *
Copyright (c) 1989  X Consortium

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
 */

/***********************************************************************
 *
 * MenuButton Widget
 *
 ***********************************************************************/

/*
 * MenuButtonP.h - Private Header file for MenuButton widget.
 *
 * This is the private header file for the Athena MenuButton widget.
 * It is intended to provide an easy method of activating pulldown menus.
 *
 * Date:    May 2, 1989
 *
 * By:      Chris D. Peterson
 *          MIT X Consortium 
 *          kit@expo.lcs.mit.edu
 */

#ifndef _PixcommP_h
#define _PixcommP_h

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <X11/IntrinsicP.h>

#if defined(HAVE_LIBXAW3D)
#include <X11/Xaw3d/CommandP.h>
#else
#include <X11/Xaw/CommandP.h>
#endif

#include "pixcomm.h"

/************************************
 *
 *  Class structure
 *
 ***********************************/


   /* New fields for the MenuButton widget class record */
typedef struct _PixcommClass 
{
  int makes_compiler_happy;  /* not used */
} PixcommClassPart;

   /* Full class record declaration */
typedef struct _PixcommandClassRec {
  CoreClassPart	            core_class;
  SimpleClassPart	    simple_class;
#if defined(HAVE_LIBXAW3D)
  ThreeDClassPart	    threeD_class;
#endif
  LabelClassPart	    label_class;
  CommandClassPart	    command_class;
  PixcommClassPart          pixcomm_class;
} PixcommClassRec;

extern PixcommClassRec pixcommClassRec;

/***************************************
 *
 *  Instance (widget) structure 
 *
 **************************************/

    /* New fields for the MenuButton widget record */
typedef struct {
  /* resources */
  Pixmap back_store;
} PixcommPart;

   /* Full widget declaration */
typedef struct _PixcommRec {
    CorePart         core;
    SimplePart	     simple;
#if defined(HAVE_LIBXAW3D)
    ThreeDPart	     threeD;
#endif
    LabelPart	     label;
    CommandPart	     command;
    PixcommPart      pixcomm;
} PixcommRec;

#endif
