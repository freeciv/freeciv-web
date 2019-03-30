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

/* utility */
#include "log.h"

/* gui-sdl2 */
#include "colors.h"
#include "graphics.h"
#include "gui_tilespec.h"
#include "themespec.h"

#include "widget.h"
#include "widget_p.h"

static int (*checkbox_baseclass_redraw)(struct widget *pwidget);
static int (*textcheckbox_baseclass_redraw)(struct widget *pwidget);

/**********************************************************************//**
  Blit checkbox gfx to surface its on.
**************************************************************************/
static int redraw_checkbox(struct widget *pIcon)
{
  int ret;
  SDL_Rect src, area = pIcon->size;

  ret = (*checkbox_baseclass_redraw)(pIcon);
  if (ret != 0) {
    return ret;
  }

  if (!pIcon->theme) {
    return -3;
  }

  src.x = (pIcon->theme->w / 4) * (Uint8) (get_wstate(pIcon));
  src.y = 0;
  src.w = (pIcon->theme->w / 4);
  src.h = pIcon->theme->h;

  if (pIcon->size.w != src.w) {
    area.x += (pIcon->size.w - src.w) / 2;
  }

  if (pIcon->size.h != src.h) {
    area.y += (pIcon->size.h - src.h) / 2;
  }

  return alphablit(pIcon->theme, &src, pIcon->dst->surface, &area, 255);
}

/**********************************************************************//**
  Blit checkbox-with-text gfx to surface its on.
**************************************************************************/
static int redraw_textcheckbox(struct widget *pCBox)
{
  int ret;
  SDL_Surface *pTheme_Surface, *pIcon;

  ret = (*textcheckbox_baseclass_redraw)(pCBox);
  if (ret != 0) {
    return ret;
  }

  if (pCBox->string_utf8 == NULL) {
    return widget_redraw(pCBox);
  }

  pTheme_Surface = pCBox->theme;
  pIcon = create_icon_from_theme(pTheme_Surface, get_wstate(pCBox));

  if (!pIcon) {
    return -3;
  }

  pCBox->theme = pIcon;

  /* redraw icon label */
  ret = redraw_iconlabel(pCBox);

  FREESURFACE(pIcon);
  pCBox->theme = pTheme_Surface;

  return ret;
}

/**********************************************************************//**
  Create a new checkbox widget.
**************************************************************************/
struct widget *create_checkbox(struct gui_layer *pDest, bool state,
                               Uint32 flags)
{
  struct widget *pCBox = widget_new();
  struct CHECKBOX *pTmp = fc_calloc(1, sizeof(struct CHECKBOX));

  if (state) {
    pCBox->theme = current_theme->CBOX_Sell_Icon;
  } else {
    pCBox->theme = current_theme->CBOX_Unsell_Icon;
  }

  set_wflag(pCBox, (WF_FREE_STRING | WF_FREE_GFX | WF_FREE_PRIVATE_DATA | flags));
  set_wstate(pCBox, FC_WS_DISABLED);
  set_wtype(pCBox, WT_CHECKBOX);
  pCBox->mod = KMOD_NONE;
  pCBox->dst = pDest;
  pTmp->state = state;
  pTmp->pTRUE_Theme = current_theme->CBOX_Sell_Icon;
  pTmp->pFALSE_Theme = current_theme->CBOX_Unsell_Icon;
  pCBox->private_data.cbox = pTmp;

  checkbox_baseclass_redraw = pCBox->redraw;
  pCBox->redraw = redraw_checkbox;

  pCBox->size.w = pCBox->theme->w / 4;
  pCBox->size.h = pCBox->theme->h;

  return pCBox;
}

/**********************************************************************//**
  Create a new checkbox-with-text widget.
**************************************************************************/
struct widget *create_textcheckbox(struct gui_layer *pDest, bool state,
                                   utf8_str *pstr, Uint32 flags)
{
  struct widget *pCBox;
  struct CHECKBOX *pTmp;
  SDL_Surface *pSurf, *pIcon;
  struct widget *pTmpWidget;

  if (pstr == NULL) {
    return create_checkbox(pDest, state, flags);
  }

  pTmp = fc_calloc(1, sizeof(struct CHECKBOX));

  if (state) {
    pSurf = current_theme->CBOX_Sell_Icon;
  } else {
    pSurf = current_theme->CBOX_Unsell_Icon;
  }

  pIcon = create_icon_from_theme(pSurf, 0);
  pCBox = create_iconlabel(pIcon, pDest, pstr, (flags | WF_FREE_PRIVATE_DATA));

  pstr->style &= ~SF_CENTER;

  pCBox->theme = pSurf;
  FREESURFACE(pIcon);

  set_wtype(pCBox, WT_TCHECKBOX);
  pTmp->state = state;
  pTmp->pTRUE_Theme = current_theme->CBOX_Sell_Icon;
  pTmp->pFALSE_Theme = current_theme->CBOX_Unsell_Icon;
  pCBox->private_data.cbox = pTmp;

  pTmpWidget = widget_new();
  /* we can't use pCBox->redraw here, because it is of type iconlabel */
  textcheckbox_baseclass_redraw = pTmpWidget->redraw;
  FREEWIDGET(pTmpWidget);
  pCBox->redraw = redraw_textcheckbox;

  return pCBox;
}

/**********************************************************************//**
  Set theme surfaces for a checkbox.
**************************************************************************/
int set_new_checkbox_theme(struct widget *pCBox,
                           SDL_Surface *pTrue, SDL_Surface *pFalse)
{
  struct CHECKBOX *pTmp;

  if (!pCBox || (get_wtype(pCBox) != WT_CHECKBOX)) {
    return -1;
  }

  if (!pCBox->private_data.cbox) {
    pCBox->private_data.cbox = fc_calloc(1, sizeof(struct CHECKBOX));
    set_wflag(pCBox, WF_FREE_PRIVATE_DATA);
    pCBox->private_data.cbox->state = FALSE;
  }

  pTmp = pCBox->private_data.cbox;
  pTmp->pTRUE_Theme = pTrue;
  pTmp->pFALSE_Theme = pFalse;
  if (pTmp->state) {
    pCBox->theme = pTrue;
  } else {
    pCBox->theme = pFalse;
  }

  return 0;
}

/**********************************************************************//**
  Change the state of the checkbox.
**************************************************************************/
void toggle_checkbox(struct widget *pCBox)
{
  if (pCBox->private_data.cbox->state) {
    pCBox->theme = pCBox->private_data.cbox->pFALSE_Theme;
    pCBox->private_data.cbox->state = FALSE;
  } else {
    pCBox->theme = pCBox->private_data.cbox->pTRUE_Theme;
    pCBox->private_data.cbox->state = TRUE;
  }
}

/**********************************************************************//**
  Check state of the checkbox.
**************************************************************************/
bool get_checkbox_state(struct widget *pCBox)
{
  return pCBox->private_data.cbox->state;
}
