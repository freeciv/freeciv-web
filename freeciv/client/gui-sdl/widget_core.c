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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "SDL.h"

/* gui-sdl */
#include "colors.h"
#include "graphics.h"
#include "mapview.h"
#include "themespec.h"

#include "widget.h"
#include "widget_p.h"

/**************************************************************************
  ...
**************************************************************************/
void set_wstate(struct widget *pWidget, enum widget_state state)
{
  pWidget->state_types_flags &= ~STATE_MASK;
  pWidget->state_types_flags |= state;
}

/**************************************************************************
  ...
**************************************************************************/
void set_wtype(struct widget *pWidget, enum widget_type type)
{
  pWidget->state_types_flags &= ~TYPE_MASK;
  pWidget->state_types_flags |= type;
}

/**************************************************************************
  ...
**************************************************************************/
void set_wflag(struct widget *pWidget, enum widget_flag flag)
{
  (pWidget)->state_types_flags |= ((flag) & FLAG_MASK);
}

/**************************************************************************
  ...
**************************************************************************/
void clear_wflag(struct widget *pWidget, enum widget_flag flag)
{
  (pWidget)->state_types_flags &= ~((flag) & FLAG_MASK);
}

/**************************************************************************
  ...
**************************************************************************/
enum widget_state get_wstate(const struct widget *pWidget)
{
  return ((enum widget_state)(pWidget->state_types_flags & STATE_MASK));
}

/**************************************************************************
  ...
**************************************************************************/
enum widget_type get_wtype(const struct widget *pWidget)
{
  return ((enum widget_type)(pWidget->state_types_flags & TYPE_MASK));
}

/**************************************************************************
  ...
**************************************************************************/
enum widget_flag get_wflags(const struct widget *pWidget)
{
  return ((enum widget_flag)(pWidget->state_types_flags & FLAG_MASK));
}

/**************************************************************************
   ...
**************************************************************************/
void widget_free(struct widget **pWidget)
{
  struct widget *pGUI = *pWidget;
  
  if ((get_wflags(pGUI) & WF_FREE_STRING) == WF_FREE_STRING) {
    FREESTRING16(pGUI->string16);
  }
  if ((get_wflags(pGUI) & WF_FREE_GFX) == WF_FREE_GFX) {
    FREESURFACE(pGUI->gfx);
  }
  if ((get_wflags(pGUI) & WF_FREE_THEME) == WF_FREE_THEME) {
    if (get_wtype(pGUI) == WT_CHECKBOX) {
      FREESURFACE(pGUI->private_data.cbox->pTRUE_Theme);
      FREESURFACE(pGUI->private_data.cbox->pFALSE_Theme);
    } else {
      FREESURFACE(pGUI->theme);
    }
  }
  if ((get_wflags(pGUI) & WF_FREE_THEME2) == WF_FREE_THEME2) {
    FREESURFACE(pGUI->theme2);
  }
  if ((get_wflags(pGUI) & WF_FREE_DATA) == WF_FREE_DATA) {
    FC_FREE(pGUI->data.ptr);
  }
  if ((get_wflags(pGUI) & WF_FREE_PRIVATE_DATA) == WF_FREE_PRIVATE_DATA) {
    FC_FREE(pGUI->private_data.ptr);
  }
  
  FC_FREE(*pWidget);
}

/**************************************************************************
  ...
**************************************************************************/
static void widget_core_set_area(struct widget *pwidget, SDL_Rect area)
{
  pwidget->area = area;
}

/**************************************************************************
  ...
**************************************************************************/
static void widget_core_set_position(struct widget *pwidget, int x, int y)
{
  pwidget->size.x = x;
  pwidget->size.y = y;    
}

/**************************************************************************
  ...
**************************************************************************/
static void widget_core_resize(struct widget *pwidget, int w, int h)
{
  pwidget->size.w = w;
  pwidget->size.h = h;    
}

/**************************************************************************
  ...
**************************************************************************/
static int widget_core_redraw(struct widget *pwidget)
{
  if (!pwidget || (get_wflags(pwidget) & WF_HIDDEN)) {
    return -1;
  }
  
  if (pwidget->gfx) {
    widget_undraw(pwidget);
  }
  
  if (!pwidget->gfx && (get_wflags(pwidget) & WF_RESTORE_BACKGROUND)) {
    refresh_widget_background(pwidget);
  }
  
  return 0;
}

/**************************************************************************
  ...
**************************************************************************/
static void widget_core_draw_frame(struct widget *pwidget)
{
  draw_frame_inside_widget(pwidget);
}

/**************************************************************************
  ...
**************************************************************************/
static void widget_core_mark_dirty(struct widget *pwidget)
{
  SDL_Rect rect = {
    pwidget->dst->dest_rect.x + pwidget->size.x,
    pwidget->dst->dest_rect.y + pwidget->size.y,
    pwidget->size.w,
    pwidget->size.h
  };

  sdl_dirty_rect(rect);
}

/**************************************************************************
  ...
**************************************************************************/
static void widget_core_flush(struct widget *pwidget)
{
  SDL_Rect rect = {
    pwidget->dst->dest_rect.x + pwidget->size.x,
    pwidget->dst->dest_rect.y + pwidget->size.y,
    pwidget->size.w,
    pwidget->size.h
  };
  
  flush_rect(rect, FALSE);
}

/**************************************************************************
  ...
**************************************************************************/
static void widget_core_undraw(struct widget *pwidget)
{
  if (get_wflags(pwidget) & WF_RESTORE_BACKGROUND) {
    if (pwidget->gfx) {
      clear_surface(pwidget->dst->surface, &pwidget->size);
      blit_entire_src(pwidget->gfx, pwidget->dst->surface,
                      pwidget->size.x, pwidget->size.y);
    }
  } else {
    clear_surface(pwidget->dst->surface, &pwidget->size);
  }
}

/**************************************************************************
  ...
**************************************************************************/
static void widget_core_select(struct widget *pwidget)
{
  widget_redraw(pwidget);
  widget_flush(pwidget);
}

/**************************************************************************
  ...
**************************************************************************/
static void widget_core_unselect(struct widget *pwidget)
{
  widget_redraw(pwidget);
  widget_flush(pwidget);
}

/**************************************************************************
  ...
**************************************************************************/
struct widget *widget_new()
{
  struct widget *pWidget = fc_calloc(1, sizeof(struct widget));

  pWidget->set_area = widget_core_set_area;
  pWidget->set_position = widget_core_set_position;
  pWidget->resize = widget_core_resize;
  pWidget->redraw = widget_core_redraw;
  pWidget->draw_frame = widget_core_draw_frame;
  pWidget->mark_dirty = widget_core_mark_dirty;
  pWidget->flush = widget_core_flush;
  pWidget->undraw = widget_core_undraw;
  pWidget->select = widget_core_select;
  pWidget->unselect = widget_core_unselect;
  
  return pWidget;
}
