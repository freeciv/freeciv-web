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
#include "mem.h"

/* client/gui-sdl2 */
#include "colors.h"
#include "graphics.h"
#include "themespec.h"

#include "widget.h"
#include "widget_p.h"

static int (*baseclass_redraw)(struct widget *pwidget);
  
/**********************************************************************//**
  Blit themelabel2 gfx to surface its on.
**************************************************************************/
static inline int redraw_themelabel2(struct widget *pLabel)
{    
  SDL_Rect src = {0,0, pLabel->size.w, pLabel->size.h};
  SDL_Rect dst = {pLabel->size.x, pLabel->size.y, 0, 0};
/*
  if (!pLabel) {
    return -3;
  }
*/
  if (get_wstate(pLabel) == FC_WS_SELECTED) {
    src.y = pLabel->size.h;
  }

  return alphablit(pLabel->theme, &src, pLabel->dst->surface, &dst, 255);
}

/**********************************************************************//**
  Blit label gfx to surface its on.
**************************************************************************/
static int redraw_label(struct widget *pLabel)
{
  int ret;
  SDL_Rect area = pLabel->size;
  SDL_Color bar_color = *get_theme_color(COLOR_THEME_LABEL_BAR);
  SDL_Color backup_color = {0, 0, 0, 0};

  ret = (*baseclass_redraw)(pLabel);
  if (ret != 0) {
    return ret;
  }

  if (get_wtype(pLabel) == WT_T2_LABEL) {
    return redraw_themelabel2(pLabel);
  }

  /* redraw selected bar */
  if (get_wstate(pLabel) == FC_WS_SELECTED) {
    if (get_wflags(pLabel) & WF_SELECT_WITHOUT_BAR) {
      if (pLabel->string_utf8 != NULL) {
        backup_color = pLabel->string_utf8->fgcol;
        pLabel->string_utf8->fgcol = bar_color;
        if (pLabel->string_utf8->style & TTF_STYLE_BOLD) {
          pLabel->string_utf8->style |= TTF_STYLE_UNDERLINE;
        } else {
          pLabel->string_utf8->style |= TTF_STYLE_BOLD;
        }
      }
    } else {
      fill_rect_alpha(pLabel->dst->surface, &area, &bar_color);
    }
  }

  /* redraw icon label */
  ret = redraw_iconlabel(pLabel);

  if ((get_wstate(pLabel) == FC_WS_SELECTED) && (pLabel->string_utf8 != NULL)) {
    if (get_wflags(pLabel) & WF_SELECT_WITHOUT_BAR) {
      if (pLabel->string_utf8->style & TTF_STYLE_UNDERLINE) {
        pLabel->string_utf8->style &= ~TTF_STYLE_UNDERLINE;
      } else {
        pLabel->string_utf8->style &= ~TTF_STYLE_BOLD;
      }
      pLabel->string_utf8->fgcol = backup_color;
    } else {
      if (pLabel->string_utf8->render == 3) {
        pLabel->string_utf8->bgcol = backup_color;
      }
    }
  }

  return ret;
}

/**********************************************************************//**
  Calculate new size for a label.
**************************************************************************/
void remake_label_size(struct widget *pLabel)
{
  SDL_Surface *pIcon = pLabel->theme;
  utf8_str *text = pLabel->string_utf8;
  Uint32 flags = get_wflags(pLabel);
  SDL_Rect buf = { 0, 0, 0, 0 };
  Uint16 w = 0, h = 0, space;

  if (flags & WF_DRAW_TEXT_LABEL_WITH_SPACE) {
    space = adj_size(10);
  } else {
    space = 0;
  }

  if (text != NULL) {
    bool without_box = ((get_wflags(pLabel) & WF_SELECT_WITHOUT_BAR) == WF_SELECT_WITHOUT_BAR);
    bool bold = TRUE;

    if (without_box) {
      bold = ((text->style & TTF_STYLE_BOLD) == TTF_STYLE_BOLD);
      text->style |= TTF_STYLE_BOLD;
    }

    utf8_str_size(text, &buf);

    if (without_box && !bold) {
      text->style &= ~TTF_STYLE_BOLD;
    }

    w = MAX(w, buf.w + space);
    h = MAX(h, buf.h);
  }

  if (pIcon) {
    if (text != NULL) {
      if ((flags & WF_ICON_UNDER_TEXT) || (flags & WF_ICON_ABOVE_TEXT)) {
        w = MAX(w, pIcon->w + space);
        h = MAX(h, buf.h + pIcon->h + adj_size(3));
      } else {
        if (flags & WF_ICON_CENTER) {
          w = MAX(w, pIcon->w + space);
          h = MAX(h, pIcon->h);
        } else {
          w = MAX(w, buf.w + pIcon->w + adj_size(5) + space);
          h = MAX(h, pIcon->h);
        }
      }
      /* text */
    } else {
      w = MAX(w, pIcon->w + space);
      h = MAX(h, pIcon->h);
    }
  }

  /* pIcon */
  pLabel->size.w = w;
  pLabel->size.h = h;
}

/**********************************************************************//**
  ThemeLabel is utf8_str with Background ( pIcon ).
**************************************************************************/
struct widget *create_themelabel(SDL_Surface *pIcon, struct gui_layer *pDest,
                                 utf8_str *pstr, Uint16 w, Uint16 h,
                                 Uint32 flags)
{
  struct widget *pLabel = NULL;

  if (pIcon == NULL && pstr == NULL) {
    return NULL;
  }

  pLabel = widget_new();
  pLabel->theme = pIcon;
  pLabel->string_utf8 = pstr;
  set_wflag(pLabel,
	    (WF_ICON_CENTER | WF_FREE_STRING | WF_FREE_GFX |
	     WF_RESTORE_BACKGROUND | flags));
  set_wstate(pLabel, FC_WS_DISABLED);
  set_wtype(pLabel, WT_T_LABEL);
  pLabel->mod = KMOD_NONE;
  pLabel->dst = pDest;

  baseclass_redraw = pLabel->redraw;
  pLabel->redraw = redraw_label;

  remake_label_size(pLabel);

  pLabel->size.w = MAX(pLabel->size.w, w);
  pLabel->size.h = MAX(pLabel->size.h, h);

  return pLabel;
}

/**********************************************************************//**
  This Label is UTF8 string with Icon.
**************************************************************************/
struct widget *create_iconlabel(SDL_Surface *pIcon, struct gui_layer *pDest,
                                utf8_str *pstr, Uint32 flags)
{
  struct widget *pILabel = NULL;

  pILabel = widget_new();

  pILabel->theme = pIcon;
  pILabel->string_utf8 = pstr;
  set_wflag(pILabel, WF_FREE_STRING | WF_FREE_GFX | flags);
  set_wstate(pILabel, FC_WS_DISABLED);
  set_wtype(pILabel, WT_I_LABEL);
  pILabel->mod = KMOD_NONE;
  pILabel->dst = pDest;

  baseclass_redraw = pILabel->redraw;
  pILabel->redraw = redraw_label;

  remake_label_size(pILabel);

  return pILabel;
}

/**********************************************************************//**
  ThemeLabel is UTF8 string with Background ( pIcon ).
**************************************************************************/
struct widget *create_themelabel2(SDL_Surface *pIcon, struct gui_layer *pDest,
                                  utf8_str *pstr, Uint16 w, Uint16 h,
                                  Uint32 flags)
{
  struct widget *pLabel = NULL;
  SDL_Surface *ptheme = NULL;
  SDL_Rect area;
  SDL_Color store = {0, 0, 0, 0};
  SDL_Color bg_color = *get_theme_color(COLOR_THEME_THEMELABEL2_BG);
  Uint32 colorkey;

  if (pIcon == NULL && pstr == NULL) {
    return NULL;
  }

  pLabel = widget_new();
  pLabel->theme = pIcon;
  pLabel->string_utf8 = pstr;
  set_wflag(pLabel, (WF_FREE_THEME | WF_FREE_STRING | WF_FREE_GFX | flags));
  set_wstate(pLabel, FC_WS_DISABLED);
  set_wtype(pLabel, WT_T2_LABEL);
  pLabel->mod = KMOD_NONE;
  baseclass_redraw = pLabel->redraw;
  pLabel->redraw = redraw_label;

  remake_label_size(pLabel);

  pLabel->size.w = MAX(pLabel->size.w, w);
  pLabel->size.h = MAX(pLabel->size.h, h);

  ptheme = create_surf(pLabel->size.w, pLabel->size.h * 2, SDL_SWSURFACE);

  colorkey = SDL_MapRGBA(ptheme->format, pstr->bgcol.r,
                         pstr->bgcol.g, pstr->bgcol.b, pstr->bgcol.a);
  SDL_FillRect(ptheme, NULL, colorkey);

  pLabel->size.x = 0;
  pLabel->size.y = 0;
  area = pLabel->size;
  pLabel->dst = gui_layer_new(0, 0, ptheme);

  /* normal */
  redraw_iconlabel(pLabel);

  /* selected */
  area.x = 0;
  area.y = pLabel->size.h;

  if (flags & WF_RESTORE_BACKGROUND) {
    SDL_FillRect(ptheme, &area, map_rgba(ptheme->format, bg_color));
    store = pstr->bgcol;
    SDL_GetRGBA(getpixel(ptheme, area.x , area.y), ptheme->format,
                &pstr->bgcol.r, &pstr->bgcol.g,
                &pstr->bgcol.b, &pstr->bgcol.a);
  } else {
    fill_rect_alpha(ptheme, &area, &bg_color);
  }

  pLabel->size.y = pLabel->size.h;
  redraw_iconlabel(pLabel);

  if (flags & WF_RESTORE_BACKGROUND) {
    pstr->bgcol = store;
  }

  pLabel->size.x = 0;
  pLabel->size.y = 0;
  if (flags & WF_FREE_THEME) {
    FREESURFACE(pLabel->theme);
  }
  pLabel->theme = ptheme;
  FC_FREE(pLabel->dst);
  pLabel->dst = pDest;

  return pLabel;
}

/**********************************************************************//**
  Make themeiconlabel2 widget out of iconlabel widget.
**************************************************************************/
struct widget *convert_iconlabel_to_themeiconlabel2(struct widget *pIconLabel)
{
  SDL_Rect start, area;
  SDL_Color store = {0, 0, 0, 0};
  SDL_Color bg_color = *get_theme_color(COLOR_THEME_THEMELABEL2_BG);
  Uint32 colorkey, flags = get_wflags(pIconLabel);
  SDL_Surface *pDest;
  SDL_Surface *ptheme = create_surf(pIconLabel->size.w,
                                    pIconLabel->size.h * 2, SDL_SWSURFACE);

  colorkey = SDL_MapRGBA(ptheme->format,
                         pIconLabel->string_utf8->bgcol.r,
                         pIconLabel->string_utf8->bgcol.g,
                         pIconLabel->string_utf8->bgcol.b,
                         pIconLabel->string_utf8->bgcol.a);
  SDL_FillRect(ptheme, NULL, colorkey);

  start = pIconLabel->size;
  pIconLabel->size.x = 0;
  pIconLabel->size.y = 0;
  area = start;
  pDest = pIconLabel->dst->surface;
  pIconLabel->dst->surface = ptheme;

  /* normal */
  redraw_iconlabel(pIconLabel);

  /* selected */
  area.x = 0;
  area.y = pIconLabel->size.h;

  if (flags & WF_RESTORE_BACKGROUND) {
    SDL_FillRect(ptheme, &area, map_rgba(ptheme->format, bg_color));
    store = pIconLabel->string_utf8->bgcol;
    SDL_GetRGBA(getpixel(ptheme, area.x , area.y), ptheme->format,
                &pIconLabel->string_utf8->bgcol.r,
                &pIconLabel->string_utf8->bgcol.g,
      		&pIconLabel->string_utf8->bgcol.b,
                &pIconLabel->string_utf8->bgcol.a);
  } else {
    fill_rect_alpha(ptheme, &area, &bg_color);
  }

  pIconLabel->size.y = pIconLabel->size.h;
  redraw_iconlabel(pIconLabel);

  if (flags & WF_RESTORE_BACKGROUND) {
    pIconLabel->string_utf8->bgcol = store;
  }

  pIconLabel->size = start;
  if (flags & WF_FREE_THEME) {
    FREESURFACE(pIconLabel->theme);
  }
  pIconLabel->theme = ptheme;
  if (flags & WF_FREE_STRING) {
    FREEUTF8STR(pIconLabel->string_utf8);
  }
  pIconLabel->dst->surface = pDest;
  set_wtype(pIconLabel, WT_T2_LABEL);

  pIconLabel->redraw = redraw_label;

  return pIconLabel;
}

#if 0
/**********************************************************************//**
  Blit themelabel gfx to surface its on.
**************************************************************************/
static int redraw_themelabel(struct widget *pLabel)
{
  int ret;
  Sint16 x, y;
  SDL_Surface *pText = NULL;

  if (!pLabel) {
    return -3;
  }

  if ((pText = create_text_surf_from_utf8(pLabel->string_utf8)) == NULL) {
    return (-4);
  }

  if (pLabel->string_utf8->style & SF_CENTER) {
    x = (pLabel->size.w - pText->w) / 2;
  } else {
    if (pLabel->string_utf8->style & SF_CENTER_RIGHT) {
      x = pLabel->size.w - pText->w - adj_size(5);
    } else {
      x = adj_size(5);
    }
  }

  y = (pLabel->size.h - pText->h) / 2;

  /* redraw theme */
  if (pLabel->theme) {
    ret = blit_entire_src(pLabel->theme, pLabel->dst->surface, pLabel->size.x, pLabel->size.y);
    if (ret) {
      return ret;
    }
  }

  ret = blit_entire_src(pText, pLabel->dst->surface, pLabel->size.x + x, pLabel->size.y + y);

  FREESURFACE(pText);

  return ret;
}
#endif /* 0 */

/**********************************************************************//**
  Blit iconlabel gfx to surface its on.
**************************************************************************/
int redraw_iconlabel(struct widget *pLabel)
{
  int space, ret = 0; /* FIXME: possibly uninitialized */
  Sint16 x, xI, yI;
  Sint16 y = 0; /* FIXME: possibly uninitialized */
  SDL_Surface *pText;
  SDL_Rect dst;
  Uint32 flags;

  if (!pLabel) {
    return -3;
  }

  SDL_SetClipRect(pLabel->dst->surface, &pLabel->size);

  flags = get_wflags(pLabel);

  if (flags & WF_DRAW_TEXT_LABEL_WITH_SPACE) {
    space = adj_size(5);
  } else {
    space = 0;
  }

  pText = create_text_surf_from_utf8(pLabel->string_utf8);
  
  if (pLabel->theme) { /* Icon */
    if (pText) {
      if (flags & WF_ICON_CENTER_RIGHT) {
        xI = pLabel->size.w - pLabel->theme->w - space;
      } else {
        if (flags & WF_ICON_CENTER) {
          xI = (pLabel->size.w - pLabel->theme->w) / 2;
        } else {
          xI = space;
        }
      }

      if (flags & WF_ICON_ABOVE_TEXT) {
        yI = 0;
        y = pLabel->theme->h + adj_size(3)
          + (pLabel->size.h - (pLabel->theme->h + adj_size(3)) - pText->h) / 2;
      } else {
        if (flags & WF_ICON_UNDER_TEXT) {
          y = (pLabel->size.h - (pLabel->theme->h + adj_size(3)) - pText->h) / 2;
          yI = y + pText->h + adj_size(3);
        } else {
          yI = (pLabel->size.h - pLabel->theme->h) / 2;
          y = (pLabel->size.h - pText->h) / 2;
        }
      }
      /* pText */
    } else {
#if 0
      yI = (pLabel->size.h - pLabel->theme->h) / 2;
      xI = (pLabel->size.w - pLabel->theme->w) / 2;
#endif /* 0 */
      yI = 0;
      xI = space;
    }

    dst.x = pLabel->size.x + xI;
    dst.y = pLabel->size.y + yI;

    ret = alphablit(pLabel->theme, NULL, pLabel->dst->surface, &dst, 255);

    if (ret) {
      return ret - 10;
    }
  }

  if (pText) {
    if (pLabel->theme) { /* Icon */
      if (!(flags & WF_ICON_ABOVE_TEXT) && !(flags & WF_ICON_UNDER_TEXT)) {
        if (flags & WF_ICON_CENTER_RIGHT) {
          if (pLabel->string_utf8->style & SF_CENTER) {
            x = (pLabel->size.w - (pLabel->theme->w + 5 + space) -
                 pText->w) / 2;
          } else {
            if (pLabel->string_utf8->style & SF_CENTER_RIGHT) {
              x = pLabel->size.w - (pLabel->theme->w + 5 + space) - pText->w;
            } else {
              x = space;
            }
          }
          /* WF_ICON_CENTER_RIGHT */
        } else {
          if (flags & WF_ICON_CENTER) {
            /* text is blit on icon */
            goto Alone;
          } else { /* WF_ICON_CENTER_LEFT */
            if (pLabel->string_utf8->style & SF_CENTER) {
              x = space + pLabel->theme->w + adj_size(5) + ((pLabel->size.w -
                                                             (space +
                                                              pLabel->theme->w + adj_size(5)) -
                                                             pText->w) / 2);
            } else {
              if (pLabel->string_utf8->style & SF_CENTER_RIGHT) {
                x = pLabel->size.w - pText->w - space;
              } else {
                x = space + pLabel->theme->w + adj_size(5);
              }
            }
          } /* WF_ICON_CENTER_LEFT */
        }
        /* !WF_ICON_ABOVE_TEXT && !WF_ICON_UNDER_TEXT */
      } else {
        goto Alone;
      }
      /* pLabel->theme == Icon */
    } else {
      y = (pLabel->size.h - pText->h) / 2;
    Alone:
      if (pLabel->string_utf8->style & SF_CENTER) {
        x = (pLabel->size.w - pText->w) / 2;
      } else {
        if (pLabel->string_utf8->style & SF_CENTER_RIGHT) {
          x = pLabel->size.w - pText->w - space;
        } else {
          x = space;
        }
      }
    }

    dst.x = pLabel->size.x + x;
    dst.y = pLabel->size.y + y;

    ret = alphablit(pText, NULL, pLabel->dst->surface, &dst, 255);
    FREESURFACE(pText);
  }

  SDL_SetClipRect(pLabel->dst->surface, NULL);

  return ret;
}

/**********************************************************************//**
  Draw the label widget.
**************************************************************************/
int draw_label(struct widget *pLabel, Sint16 start_x, Sint16 start_y)
{
  pLabel->size.x = start_x;
  pLabel->size.y = start_y;

  return redraw_label(pLabel);
}
