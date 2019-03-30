/***********************************************************************
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
#include <fc_config.h>
#endif

/* SDL2 */
#ifdef SDL2_PLAIN_INCLUDE
#include <SDL.h>
#else  /* SDL2_PLAIN_INCLUDE */
#include <SDL2/SDL.h>
#endif /* SDL2_PLAIN_INCLUDE */

/* gui-sdl2 */
#include "colors.h"
#include "graphics.h"
#include "mapview.h"
#include "themespec.h"
#include "widget.h"
#include "widget_p.h"

/**********************************************************************//**
  Set state of the widget.
**************************************************************************/
void set_wstate(struct widget *pWidget, enum widget_state state)
{
  pWidget->state_types_flags &= ~STATE_MASK;
  pWidget->state_types_flags |= state;
}

/**********************************************************************//**
  Set type of the widget.
**************************************************************************/
void set_wtype(struct widget *pWidget, enum widget_type type)
{
  pWidget->state_types_flags &= ~TYPE_MASK;
  pWidget->state_types_flags |= type;
}

/**********************************************************************//**
  Set flags of the widget.
**************************************************************************/
void set_wflag(struct widget *pWidget, enum widget_flag flag)
{
  (pWidget)->state_types_flags |= ((flag) & FLAG_MASK);
}

/**********************************************************************//**
  Clear flag from the widget.
**************************************************************************/
void clear_wflag(struct widget *pWidget, enum widget_flag flag)
{
  (pWidget)->state_types_flags &= ~((flag) & FLAG_MASK);
}

/**********************************************************************//**
  Get state of the widget.
**************************************************************************/
enum widget_state get_wstate(const struct widget *pWidget)
{
  return ((enum widget_state)(pWidget->state_types_flags & STATE_MASK));
}

/**********************************************************************//**
  Get type of the widget.
**************************************************************************/
enum widget_type get_wtype(const struct widget *pWidget)
{
  return ((enum widget_type)(pWidget->state_types_flags & TYPE_MASK));
}

/**********************************************************************//**
  Get all flags of the widget.
**************************************************************************/
enum widget_flag get_wflags(const struct widget *pWidget)
{
  return ((enum widget_flag)(pWidget->state_types_flags & FLAG_MASK));
}

/**********************************************************************//**
   Free resources allocated for the widget.
**************************************************************************/
void widget_free(struct widget **pWidget)
{
  struct widget *pGUI = *pWidget;

  if (get_wflags(pGUI) & WF_FREE_STRING) {
    FREEUTF8STR(pGUI->string_utf8);
  }
  if (get_wflags(pGUI) & WF_WIDGET_HAS_INFO_LABEL) {
    FREEUTF8STR(pGUI->info_label);
  }
  if (get_wflags(pGUI) & WF_FREE_GFX) {
    FREESURFACE(pGUI->gfx);
  }
  if (get_wflags(pGUI) & WF_FREE_THEME) {
    if (get_wtype(pGUI) == WT_CHECKBOX) {
      FREESURFACE(pGUI->private_data.cbox->pTRUE_Theme);
      FREESURFACE(pGUI->private_data.cbox->pFALSE_Theme);
    } else {
      FREESURFACE(pGUI->theme);
    }
  }
  if (get_wflags(pGUI) & WF_FREE_THEME2) {
    FREESURFACE(pGUI->theme2);
  }
  if (get_wflags(pGUI) & WF_FREE_DATA) {
    FC_FREE(pGUI->data.ptr);
  }
  if (get_wflags(pGUI) & WF_FREE_PRIVATE_DATA) {
    FC_FREE(pGUI->private_data.ptr);
  }
  if (NULL != pGUI->destroy) {
    pGUI->destroy(pGUI);
  }

  FC_FREE(*pWidget);
}

/**********************************************************************//**
  Set widget area.
**************************************************************************/
static void widget_core_set_area(struct widget *pwidget, SDL_Rect area)
{
  pwidget->area = area;
}

/**********************************************************************//**
  Set widget position.
**************************************************************************/
static void widget_core_set_position(struct widget *pwidget, int x, int y)
{
  pwidget->size.x = x;
  pwidget->size.y = y;
}

/**********************************************************************//**
  Set widget size.
**************************************************************************/
static void widget_core_resize(struct widget *pwidget, int w, int h)
{
  pwidget->size.w = w;
  pwidget->size.h = h;
}

/**********************************************************************//**
  Draw widget to the surface its on, if it's visible.
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

/**********************************************************************//**
  Draw frame of the widget.
**************************************************************************/
static void widget_core_draw_frame(struct widget *pwidget)
{
  draw_frame_inside_widget(pwidget);
}

/**********************************************************************//**
  Mark part of the display covered by the widget dirty.
**************************************************************************/
static void widget_core_mark_dirty(struct widget *pwidget)
{
  SDL_Rect rect = {
    pwidget->dst->dest_rect.x + pwidget->size.x,
    pwidget->dst->dest_rect.y + pwidget->size.y,
    pwidget->size.w,
    pwidget->size.h
  };

  dirty_sdl_rect(&rect);
}

/**********************************************************************//**
  Flush part of the display covered by the widget.
**************************************************************************/
static void widget_core_flush(struct widget *pwidget)
{
  SDL_Rect rect = {
    pwidget->dst->dest_rect.x + pwidget->size.x,
    pwidget->dst->dest_rect.y + pwidget->size.y,
    pwidget->size.w,
    pwidget->size.h
  };

  flush_rect(&rect, FALSE);
}

/**********************************************************************//**
  Clear widget from the display.
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

/**********************************************************************//**
  Callback for when widget gets selected.
**************************************************************************/
static void widget_core_select(struct widget *pwidget)
{
  widget_redraw(pwidget);
  widget_flush(pwidget);
}

/**********************************************************************//**
  Callback for when widget gets unselected.
**************************************************************************/
static void widget_core_unselect(struct widget *pwidget)
{
  widget_redraw(pwidget);
  widget_flush(pwidget);
}

/**********************************************************************//**
  Create a new widget.
**************************************************************************/
struct widget *widget_new(void)
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
