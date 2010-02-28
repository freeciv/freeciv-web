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

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xaw/Form.h>

#include "astring.h"
#include "fcintl.h"
#include "mem.h"

#include "gui_main.h"

#include "gui_stuff.h"

void put_line_8(char *psrc, char *pdst,  int dst_w, int xoffset_table[]);
void put_line_16(char *psrc, char *pdst,  int dst_w, int xoffset_table[]);
void put_line_24(char *psrc, char *pdst,  int dst_w, int xoffset_table[]);
void put_line_32(char *psrc, char *pdst,  int dst_w, int xoffset_table[]);

/**************************************************************************
...
**************************************************************************/
void xaw_set_relative_position(Widget ref, Widget w, int px, int py)
{
  Position x, y;
  Dimension width, height;
  
  XtVaGetValues(ref, XtNwidth, &width, XtNheight, &height, NULL);
  XtTranslateCoords(ref, (Position) px*width/100, (Position) py*height/100, &x, &y);
  XtVaSetValues(w, XtNx, x, XtNy, y, NULL);
}


/**************************************************************************
...
**************************************************************************/
void xaw_horiz_center(Widget w)
{
  Dimension width, width2;

  XtVaGetValues(XtParent(w), XtNwidth, &width, NULL);
  XtVaGetValues(w, XtNwidth, &width2, NULL);
  XtVaSetValues(w, XtNhorizDistance, (width-width2)/2, NULL);
}

/**************************************************************************
...
**************************************************************************/
void xaw_set_bitmap(Widget w, Pixmap pm)
{
  XtVaSetValues(w, XtNbitmap, (XtArgVal)pm, NULL);
  xaw_expose_now(w);
}


/**************************************************************************
...
**************************************************************************/
void xaw_set_label(Widget w, const char *text)
{
  String str;

  XtVaGetValues(w, XtNlabel, &str, NULL);
  if (strcmp(str, text) != 0) {
    XtVaSetValues(w, XtNlabel, (XtArgVal)text, NULL);
    xaw_expose_now(w);
  }
  
}

/**************************************************************************
...
**************************************************************************/
void xaw_expose_now(Widget w)
{
  Dimension width, height;
  XExposeEvent xeev;
  
  xeev.type = Expose;
  xeev.display = XtDisplay (w);
  xeev.window = XtWindow(w);
  xeev.x = 0;
  xeev.y = 0;
  XtVaGetValues(w, XtNwidth, &width, XtNheight, &height, NULL);
  (XtClass(w))->core_class.expose(w, (XEvent *)&xeev, NULL);
}

/**************************************************************************
...
**************************************************************************/
void x_simulate_button_click(Widget w)
{
  XButtonEvent ev;
  
  ev.display = XtDisplay(w);
  ev.window = XtWindow(w);
  ev.type=ButtonPress;
  ev.button=Button1;
  ev.x=10;
  ev.y=10;

  XtDispatchEvent((XEvent *)&ev);
  ev.type=ButtonRelease;
  XtDispatchEvent((XEvent *)&ev);
}

/**************************************************************************
...
**************************************************************************/
Pixmap x_scale_pixmap(Pixmap src, int src_w, int src_h, int dst_w, int dst_h, 
		      Window root)
{
  Pixmap dst;
  XImage *xi_src, *xi_dst;
  int xoffset_table[4096];
  int x, xoffset, xadd, xremsum, xremadd;
  int y, yoffset, yadd, yremsum, yremadd;
  char *pdst_data;
  char *psrc_data;

  xi_src=XGetImage(display, src, 0, 0, src_w, src_h, AllPlanes, ZPixmap);
  xi_dst=XCreateImage(display, DefaultVisual(display, screen_number),
		       xi_src->depth, ZPixmap,
		       0, NULL,
		       dst_w, dst_h,
		       xi_src->bitmap_pad, 0);

  xi_dst->data=fc_calloc(xi_dst->bytes_per_line * xi_dst->height, 1);

  /* for each pixel in dst, calculate pixel offset in src */
  xadd=src_w/dst_w;
  xremadd=src_w%dst_w;
  xoffset=0;
  xremsum=dst_w/2;

  for (x = 0; x < dst_w; x++) {
    xoffset_table[x]=xoffset;
    xoffset+=xadd;
    xremsum+=xremadd;
    if(xremsum>=dst_w) {
      xremsum-=dst_w;
      xoffset++;
    }
  }

  yadd=src_h/dst_h;
  yremadd=src_h%dst_h;
  yoffset=0;
  yremsum=dst_h/2; 

  for (y = 0; y < dst_h; y++) {
    psrc_data=xi_src->data + (yoffset * xi_src->bytes_per_line);
    pdst_data=xi_dst->data + (y * xi_dst->bytes_per_line);

    switch(xi_src->bits_per_pixel) {
     case 8:
      put_line_8(psrc_data, pdst_data, dst_w, xoffset_table);
      break;
     case 16:
      put_line_16(psrc_data, pdst_data, dst_w, xoffset_table);
      break;
     case 24:
      put_line_24(psrc_data, pdst_data, dst_w, xoffset_table);
      break;
     case 32:
      put_line_32(psrc_data, pdst_data, dst_w, xoffset_table);
      break;
     default:
      memcpy(pdst_data, psrc_data, (src_w<dst_w) ? src_w : dst_w);
      break;
    }

    yoffset+=yadd;
    yremsum+=yremadd;
    if(yremsum>=dst_h) {
      yremsum-=dst_h;
      yoffset++;
    }
  }

  dst=XCreatePixmap(display, root, dst_w, dst_h, display_depth);
  XPutImage(display, dst, civ_gc, xi_dst, 0, 0, 0, 0, dst_w, dst_h);

  xi_src->f.destroy_image(xi_src);
  xi_dst->f.destroy_image(xi_dst);

  return dst;
}

void put_line_8(char *psrc, char *pdst,  int dst_w, int xoffset_table[])
{
  int x;
  for (x = 0; x < dst_w; x++)
    *pdst++=*(psrc+xoffset_table[x]+0);
}

void put_line_16(char *psrc, char *pdst,  int dst_w, int xoffset_table[])
{
  int x;
  for (x = 0; x < dst_w; x++) {
    *pdst++=*(psrc+2*xoffset_table[x]+0);
    *pdst++=*(psrc+2*xoffset_table[x]+1);
  }
}

void put_line_24(char *psrc, char *pdst,  int dst_w, int xoffset_table[])
{
  int x;
  for (x = 0; x < dst_w; x++) {
    *pdst++=*(psrc+3*xoffset_table[x]+0);
    *pdst++=*(psrc+3*xoffset_table[x]+1);
    *pdst++=*(psrc+3*xoffset_table[x]+2);
  }
}

void put_line_32(char *psrc, char *pdst,  int dst_w, int xoffset_table[])
{
  int x;
  for (x = 0; x < dst_w; x++) {
    *pdst++=*(psrc+4*xoffset_table[x]+0);
    *pdst++=*(psrc+4*xoffset_table[x]+1);
    *pdst++=*(psrc+4*xoffset_table[x]+2);
    *pdst++=*(psrc+4*xoffset_table[x]+3);
  }
}


/**************************************************************************
  Returns 1 if string has gettext markings _inside_ string
  (marking must be strictly at ends of string).
**************************************************************************/
static int has_intl_marking(const char *str)
{
  int len;
  
  if (!(str[0] == '_' && str[1] == '(' && str[2] == '\"'))
    return 0;
  
  len = strlen(str);

  return (len >= 5 && str[len-2] == '\"' && str[len-1] == ')');
}

/**************************************************************************
  For a string which _has_ intl marking (caller should check beforehand)
  returns gettext result of string inside marking.
  Below buf is an internal buffer which is never free'd.
**************************************************************************/
static const char *convert_intl_marking(const char *str)
{
  static struct astring buf = ASTRING_INIT;
  int len = strlen(str);

  assert(len>=5);
  astr_minsize(&buf, len-2);	/* +1 nul, -3 start */
  strcpy(buf.str, str+3);
  buf.str[len-5] = '\0';

  return _(buf.str);
}

/**************************************************************************
  Check if the current widget label has i18n/gettext markings
  _inside_ the string (for full string only).  If so, then
  strip off those markings, pass to gettext, and re-set label
  to gettext-returned string.  Note this is intended for resources
  which are specified in data/Freeciv; if the label is set via
  code, it should be gettext'd via code before/when set.
  Note all this is necessary even if not using gettext, to
  remove the gettext markings!
  Returns the same widget (eg, to nest calls of this and following funcs).
**************************************************************************/
Widget xaw_intl_label(Widget w)
{
  String str;
  Boolean rszbl;

  XtVaGetValues(w, XtNlabel, &str, XtNresizable, &rszbl, NULL);

  if (has_intl_marking(str)) {
    if (!rszbl)
      XtVaSetValues(w, XtNresizable, (XtArgVal)True, NULL);
    XtVaSetValues(w, XtNlabel, (XtArgVal)convert_intl_marking(str), NULL);
    if (!rszbl)
      XtVaSetValues(w, XtNresizable, (XtArgVal)False, NULL);
  }

  return w;
}

/**************************************************************************
  As above, but also making sure to preserve the widget width.
  Should probably use this version rather than above
  when data/Freeciv specifies the width.
**************************************************************************/
Widget xaw_intl_label_width(Widget w)
{
  String str;
  Boolean rszbl;
  Dimension width;

  XtVaGetValues(w, XtNlabel, &str, XtNresizable, &rszbl,
		XtNwidth, &width, NULL);

  if (has_intl_marking(str)) {
    if (!rszbl)
      XtVaSetValues(w, XtNresizable, (XtArgVal)True, NULL);
    XtVaSetValues(w, XtNlabel, (XtArgVal)convert_intl_marking(str),
		  XtNwidth, width, NULL);
    if (!rszbl)
      XtVaSetValues(w, XtNresizable, (XtArgVal)False, NULL);
  }

  return w;
}

/**************************************************************************
  As above, for widget title.
**************************************************************************/
Widget xaw_intl_title(Widget w)
{
  String str;

  XtVaGetValues(w, XtNtitle, &str, NULL);

  if (has_intl_marking(str))
    XtVaSetValues(w, XtNtitle, (XtArgVal)convert_intl_marking(str), NULL);
  return w;
}

/**************************************************************************
  As above, for widget icon name.
**************************************************************************/
Widget xaw_intl_icon_name(Widget w)
{
  String str;

  XtVaGetValues(w, XtNiconName, &str, NULL);

  if (has_intl_marking(str))
    XtVaSetValues(w, XtNiconName, (XtArgVal)convert_intl_marking(str), NULL);
  return w;
}

/**************************************************************************
  As above, for widget String contents.
**************************************************************************/
Widget xaw_intl_string(Widget w)
{
  String str;
  Boolean rszbl;

  XtVaGetValues(w, XtNstring, &str, XtNresizable, &rszbl, NULL);

  if (has_intl_marking(str)) {
    if (!rszbl)
      XtVaSetValues(w, XtNresizable, (XtArgVal)True, NULL);
    XtVaSetValues(w, XtNstring, (XtArgVal)convert_intl_marking(str), NULL);
    if (!rszbl)
      XtVaSetValues(w, XtNresizable, (XtArgVal)False, NULL);
  }

  return w;
}

/**************************************************************************
  As label_width above, for widget String contents.
**************************************************************************/
Widget xaw_intl_string_width(Widget w)
{
  String str;
  Boolean rszbl;
  Dimension width;

  XtVaGetValues(w, XtNstring, &str, XtNresizable, &rszbl,
		XtNwidth, &width, NULL);

  if (has_intl_marking(str)) {
    if (!rszbl)
      XtVaSetValues(w, XtNresizable, (XtArgVal)True, NULL);
    XtVaSetValues(w, XtNstring, (XtArgVal)convert_intl_marking(str),
		  XtNwidth, width, NULL);
    if (!rszbl)
      XtVaSetValues(w, XtNresizable, (XtArgVal)False, NULL);
  }

  return w;
}
