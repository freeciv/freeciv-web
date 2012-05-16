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
#ifndef FC__GUI_STUFF_H
#define FC__GUI_STUFF_H

#include <X11/Intrinsic.h>

void xaw_horiz_center(Widget w);
void xaw_set_label(Widget w, const char *text);
void xaw_set_bitmap(Widget w, Pixmap pm);
void x_simulate_button_click(Widget w);
void xaw_expose_now(Widget w);
void xaw_set_relative_position(Widget ref, Widget w, int px, int py);
Pixmap x_scale_pixmap(Pixmap src, int src_w, int src_h, int dst_w, int dst_h, 
		      Window root);

Widget xaw_intl_label(Widget w);
Widget xaw_intl_label_width(Widget w);
Widget xaw_intl_title(Widget w);
Widget xaw_intl_icon_name(Widget w);
Widget xaw_intl_string(Widget w);
Widget xaw_intl_string_width(Widget w);

#define I_L(w)  xaw_intl_label(w)
#define I_LW(w) xaw_intl_label_width(w)
#define I_T(w)  xaw_intl_title(w)
#define I_IN(w) xaw_intl_icon_name(w)
#define I_S(w)  xaw_intl_string(w)
#define I_SW(w) xaw_intl_string_width(w)

#define INT_TO_XTPOINTER(m_i)	((XtPointer)((long)(m_i)))
#define XTPOINTER_TO_INT(m_p)	((int)((long)(m_p)))

#endif  /* FC__GUI_STUFF_H */
