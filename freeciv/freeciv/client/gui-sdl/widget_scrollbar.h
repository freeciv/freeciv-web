/**********************************************************************
 Freeciv - Copyright (C) 2006 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifndef FC__WIDGET_SCROLLBAR_H
#define FC__WIDGET_SCROLLBAR_H

#include "widget.h"

struct ScrollBar {
  struct widget *pUp_Left_Button;
  struct widget *pScrollBar;
  struct widget *pDown_Right_Button;  
  Uint8 active;		/* used by scroll: numbers of displayed rows */
  Uint8 step;		/* used by scroll: numbers of displayed columns */
  /* total dispalyed widget = active * step */
  Uint16 count;		/* total size of scroll list */
  Sint16 min;		/* used by scroll: min pixel position */
  Sint16 max;		/* used by scroll: max pixel position */
};

#define scrollbar_size(pScroll)				\
        ((float)((float)(pScroll->active * pScroll->step) /	\
        (float)pScroll->count) * (pScroll->max - pScroll->min))

#define hide_scrollbar(scrollbar)				\
do {								\
  if (scrollbar->pUp_Left_Button) {				\
    if (!(get_wflags(scrollbar->pUp_Left_Button) & WF_HIDDEN)) {\
      widget_undraw(scrollbar->pUp_Left_Button);                \
      widget_mark_dirty(scrollbar->pUp_Left_Button);            \
      set_wflag(scrollbar->pUp_Left_Button, WF_HIDDEN);		\
    }                                                           \
    if (!(get_wflags(scrollbar->pDown_Right_Button) & WF_HIDDEN)) {\
      widget_undraw(scrollbar->pDown_Right_Button);             \
      widget_mark_dirty(scrollbar->pDown_Right_Button);         \
      set_wflag(scrollbar->pDown_Right_Button, WF_HIDDEN);	\
    }                                                           \
  }								\
  if (scrollbar->pScrollBar) {					\
    if (!(get_wflags(scrollbar->pScrollBar) & WF_HIDDEN)) {     \
      widget_undraw(scrollbar->pScrollBar);                     \
      widget_mark_dirty(scrollbar->pScrollBar);                 \
      set_wflag(scrollbar->pScrollBar, WF_HIDDEN);	        \
    }                                                           \
  }								\
} while(0)

#define show_scrollbar(scrollbar)				\
do {								\
  if (scrollbar->pUp_Left_Button) {				\
    clear_wflag(scrollbar->pUp_Left_Button, WF_HIDDEN);		\
    clear_wflag(scrollbar->pDown_Right_Button, WF_HIDDEN);	\
  }								\
  if (scrollbar->pScrollBar) {					\
    clear_wflag(scrollbar->pScrollBar, WF_HIDDEN);		\
  }								\
} while(0)

/* VERTICAL */
struct widget *create_vertical(SDL_Surface *pVert_theme, struct gui_layer *pDest,
  				Uint16 high, Uint32 flags);
int draw_vert(struct widget *pVert, Sint16 x, Sint16 y);

Uint32 create_vertical_scrollbar(struct ADVANCED_DLG *pDlg,
	Uint8 step, Uint8 active, bool create_scrollbar, bool create_buttons);

void setup_vertical_scrollbar_area(struct ScrollBar *pScroll,
	Sint16 start_x, Sint16 start_y, Uint16 hight, bool swap_start_x);

void setup_vertical_scrollbar_default_callbacks(struct ScrollBar *pScroll);

/* HORIZONTAL */
struct widget *create_horizontal(SDL_Surface *pHoriz_theme, struct gui_layer *pDest,
  			Uint16 width, Uint32 flags);
int draw_horiz(struct widget *pHoriz, Sint16 x, Sint16 y);

Uint32 create_horizontal_scrollbar(struct ADVANCED_DLG *pDlg,
	  Sint16 start_x, Sint16 start_y, Uint16 width, Uint16 active,
	  bool create_scrollbar, bool create_buttons, bool swap_start_y);

#endif /* FC__WIDGET_SCROLLBAR_H */
