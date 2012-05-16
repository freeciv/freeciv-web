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

/* utility */
#include "log.h"

/* gui-sdl */
#include "colors.h"
#include "graphics.h"
#include "themespec.h"

#include "widget.h"
#include "widget_p.h"

static int (*baseclass_redraw)(struct widget *pwidget);
  
/**************************************************************************
  ...
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
  if(get_wstate(pLabel) == FC_WS_SELLECTED) {
    src.y = pLabel->size.h;
  }

  return alphablit(pLabel->theme, &src, pLabel->dst->surface, &dst);
}

/**************************************************************************
  ...
**************************************************************************/
static int redraw_label(struct widget *pLabel)
{
  int ret;
  SDL_Rect area = pLabel->size;
  SDL_Color bar_color = *get_game_colorRGB(COLOR_THEME_LABEL_BAR);
  SDL_Color backup_color = {0, 0, 0, 0};

  ret = (*baseclass_redraw)(pLabel);
  if (ret != 0) {
    return ret;
  }
  
  if(get_wtype(pLabel) == WT_T2_LABEL) {
    return redraw_themelabel2(pLabel);
  }

  /* redraw sellect bar */
  if (get_wstate(pLabel) == FC_WS_SELLECTED) {
    if(get_wflags(pLabel) & WF_SELLECT_WITHOUT_BAR) {
      if (pLabel->string16) {
        backup_color = pLabel->string16->fgcol;
        pLabel->string16->fgcol = bar_color;
	if(pLabel->string16->style & TTF_STYLE_BOLD) {
	  pLabel->string16->style |= TTF_STYLE_UNDERLINE;
	} else {
	  pLabel->string16->style |= TTF_STYLE_BOLD;
	}
      }
    } else {
      SDL_FillRectAlpha(pLabel->dst->surface, &area, &bar_color);
    }
  }

  /* redraw icon label */
  ret = redraw_iconlabel(pLabel);
  
  if ((get_wstate(pLabel) == FC_WS_SELLECTED) && (pLabel->string16)) {
    if(get_wflags(pLabel) & WF_SELLECT_WITHOUT_BAR) {
      if (pLabel->string16->style & TTF_STYLE_UNDERLINE) {
	pLabel->string16->style &= ~TTF_STYLE_UNDERLINE;
      } else {
	pLabel->string16->style &= ~TTF_STYLE_BOLD;
      }
      pLabel->string16->fgcol = backup_color;
    } else {
      if(pLabel->string16->render == 3) {
	pLabel->string16->bgcol = backup_color;
      }
    } 
  }
  
  return ret;
}

/**************************************************************************
  ...
**************************************************************************/
void remake_label_size(struct widget *pLabel)
{
  SDL_Surface *pIcon = pLabel->theme;
  SDL_String16 *pText = pLabel->string16;
  Uint32 flags = get_wflags(pLabel);
  SDL_Rect buf = { 0, 0, 0, 0 };
  Uint16 w = 0, h = 0, space;

  if (flags & WF_DRAW_TEXT_LABEL_WITH_SPACE) {
    space = adj_size(10);
  } else {
    space = 0;
  }

  if (pText) {
    bool without_box = ((get_wflags(pLabel) & WF_SELLECT_WITHOUT_BAR) == WF_SELLECT_WITHOUT_BAR);
    bool bold = TRUE;
    
    if (without_box)
    {
      bold = ((pText->style & TTF_STYLE_BOLD) == TTF_STYLE_BOLD);
      pText->style |= TTF_STYLE_BOLD;
    }
    
    buf = str16size(pText);

    if (without_box && !bold)
    {
      pText->style &= ~TTF_STYLE_BOLD;
    }
    
    w = MAX(w, buf.w + space);
    h = MAX(h, buf.h);
  }

  if (pIcon) {
    if (pText) {
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
    } /* pText */
    else {
      w = MAX(w, pIcon->w + space);
      h = MAX(h, pIcon->h);
    }
  }

  /* pIcon */
  pLabel->size.w = w;
  pLabel->size.h = h;
}

/**************************************************************************
  ThemeLabel is String16 with Background ( pIcon ).
**************************************************************************/
struct widget * create_themelabel(SDL_Surface *pIcon, struct gui_layer *pDest,
  		SDL_String16 *pText, Uint16 w, Uint16 h, Uint32 flags)
{
  struct widget *pLabel = NULL;
  
  if (!pIcon && !pText) {
    return NULL;
  }

  pLabel = widget_new();
  pLabel->theme = pIcon;
  pLabel->string16 = pText;
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

/**************************************************************************
  this Label is String16 with Icon.
**************************************************************************/
struct widget * create_iconlabel(SDL_Surface *pIcon, struct gui_layer *pDest,
  		SDL_String16 *pText, Uint32 flags)
{
  struct widget *pILabel = NULL;

  pILabel = widget_new();

  pILabel->theme = pIcon;
  pILabel->string16 = pText;
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

/**************************************************************************
  ThemeLabel is String16 with Background ( pIcon ).
**************************************************************************/
struct widget * create_themelabel2(SDL_Surface *pIcon, struct gui_layer *pDest,
  		SDL_String16 *pText, Uint16 w, Uint16 h, Uint32 flags)
{
  struct widget *pLabel = NULL;
  SDL_Surface *pBuf = NULL, *pTheme = NULL;
  SDL_Rect area;
  SDL_Color store = {0, 0, 0, 0};
  SDL_Color bg_color = *get_game_colorRGB(COLOR_THEME_THEMELABEL2_BG);
  Uint32 colorkey;
  
  if (!pIcon && !pText) {
    return NULL;
  }

  pLabel = widget_new();
  pLabel->theme = pIcon;
  pLabel->string16 = pText;
  set_wflag(pLabel, (WF_FREE_THEME | WF_FREE_STRING | WF_FREE_GFX | flags));
  set_wstate(pLabel, FC_WS_DISABLED);
  set_wtype(pLabel, WT_T2_LABEL);
  pLabel->mod = KMOD_NONE;
  baseclass_redraw = pLabel->redraw;
  pLabel->redraw = redraw_label;
  
  remake_label_size(pLabel);

  pLabel->size.w = MAX(pLabel->size.w, w);
  pLabel->size.h = MAX(pLabel->size.h, h);
  
  pBuf = create_surf_alpha(pLabel->size.w, pLabel->size.h * 2, SDL_SWSURFACE);
    
  if(flags & WF_RESTORE_BACKGROUND) {
    pTheme = SDL_DisplayFormatAlpha(pBuf);
    FREESURFACE(pBuf);
  } else {
    pTheme = pBuf;
  }
  
  colorkey = SDL_MapRGBA(pTheme->format, pText->bgcol.r,
  		pText->bgcol.g, pText->bgcol.b, pText->bgcol.unused);
  SDL_FillRect(pTheme, NULL, colorkey);
    
  pLabel->size.x = 0;
  pLabel->size.y = 0;
  area = pLabel->size;
  pLabel->dst = gui_layer_new(0, 0, pTheme);
  
  /* normal */
  redraw_iconlabel(pLabel);

  /* sellected */
  area.x = 0;
  area.y = pLabel->size.h;
    
  if(flags & WF_RESTORE_BACKGROUND) {
    SDL_FillRect(pTheme, &area, map_rgba(pTheme->format, bg_color));
    store = pText->bgcol;
    SDL_GetRGBA(getpixel(pTheme, area.x , area.y), pTheme->format,
	      &pText->bgcol.r, &pText->bgcol.g,
      		&pText->bgcol.b, &pText->bgcol.unused);
  } else {
    SDL_FillRectAlpha(pTheme, &area, &bg_color);
  }
  
  pLabel->size.y = pLabel->size.h;
  redraw_iconlabel(pLabel);
  
  if(flags & WF_RESTORE_BACKGROUND) {
    pText->bgcol = store;
  }
  
  pLabel->size.x = 0;
  pLabel->size.y = 0;
  if(flags & WF_FREE_THEME) {
    FREESURFACE(pLabel->theme);
  }
  pLabel->theme = pTheme;
  FC_FREE(pLabel->dst);
  pLabel->dst = pDest;
  return pLabel;
}

struct widget * convert_iconlabel_to_themeiconlabel2(struct widget *pIconLabel)
{
  SDL_Rect start, area;
  SDL_Color store = {0, 0, 0, 0};
  SDL_Color bg_color = *get_game_colorRGB(COLOR_THEME_THEMELABEL2_BG);
  Uint32 colorkey, flags = get_wflags(pIconLabel);
  SDL_Surface *pDest, *pTheme, *pBuf = create_surf_alpha(pIconLabel->size.w,
				  pIconLabel->size.h * 2, SDL_SWSURFACE);
  
  if(flags & WF_RESTORE_BACKGROUND) {
    pTheme = SDL_DisplayFormatAlpha(pBuf);
    FREESURFACE(pBuf);
  } else {
    pTheme = pBuf;
  }
  
  colorkey = SDL_MapRGBA(pTheme->format, pIconLabel->string16->bgcol.r,
  		pIconLabel->string16->bgcol.g,
		pIconLabel->string16->bgcol.b,
		pIconLabel->string16->bgcol.unused);
  SDL_FillRect(pTheme, NULL, colorkey);
  
  start = pIconLabel->size;
  pIconLabel->size.x = 0;
  pIconLabel->size.y = 0;
  area = start;
  pDest = pIconLabel->dst->surface;
  pIconLabel->dst->surface = pTheme;
  
  /* normal */
  redraw_iconlabel(pIconLabel);
  
  /* sellected */
  area.x = 0;
  area.y = pIconLabel->size.h;
    
  if(flags & WF_RESTORE_BACKGROUND) {
    SDL_FillRect(pTheme, &area, map_rgba(pTheme->format, bg_color));
    store = pIconLabel->string16->bgcol;
    SDL_GetRGBA(getpixel(pTheme, area.x , area.y), pTheme->format,
	      &pIconLabel->string16->bgcol.r, &pIconLabel->string16->bgcol.g,
      		&pIconLabel->string16->bgcol.b,
			&pIconLabel->string16->bgcol.unused);
  } else {
    SDL_FillRectAlpha(pTheme, &area, &bg_color);
  }
  
  pIconLabel->size.y = pIconLabel->size.h;
  redraw_iconlabel(pIconLabel);
  
  if(flags & WF_RESTORE_BACKGROUND) {
    pIconLabel->string16->bgcol = store;
  }
  
  pIconLabel->size = start;
  if(flags & WF_FREE_THEME) {
    FREESURFACE(pIconLabel->theme);
  }
  pIconLabel->theme = pTheme;
  if(flags & WF_FREE_STRING) {
    FREESTRING16(pIconLabel->string16);
  }
  pIconLabel->dst->surface = pDest;
  set_wtype(pIconLabel, WT_T2_LABEL);
  
  pIconLabel->redraw = redraw_label;
  
  return pIconLabel;
}

#if 0
/**************************************************************************
  ...
**************************************************************************/
static int redraw_themelabel(struct widget *pLabel)
{
  int ret;
  Sint16 x, y;
  SDL_Surface *pText = NULL;

  if (!pLabel) {
    return -3;
  }

  if ((pText = create_text_surf_from_str16(pLabel->string16)) == NULL) {
    return (-4);
  }

  if (pLabel->string16->style & SF_CENTER) {
    x = (pLabel->size.w - pText->w) / 2;
  } else {
    if (pLabel->string16->style & SF_CENTER_RIGHT) {
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
#endif

/**************************************************************************
  ...
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

  pText = create_text_surf_from_str16(pLabel->string16);
  
  if (pLabel->theme) {		/* Icon */
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
    } /* pText */
    else {
#if 0
      yI = (pLabel->size.h - pLabel->theme->h) / 2;
      xI = (pLabel->size.w - pLabel->theme->w) / 2;
#endif
      yI = 0;
      xI = space;
    }

    dst.x = pLabel->size.x + xI;
    dst.y = pLabel->size.y + yI;

    ret = alphablit(pLabel->theme, NULL, pLabel->dst->surface, &dst);
    
    if (ret) {
      return ret - 10;
    }
  }

  if (pText) {
    if (pLabel->theme) { /* Icon */
      if (!(flags & WF_ICON_ABOVE_TEXT) && !(flags & WF_ICON_UNDER_TEXT)) {
	if (flags & WF_ICON_CENTER_RIGHT) {
	  if (pLabel->string16->style & SF_CENTER) {
	    x = (pLabel->size.w - (pLabel->theme->w + 5 + space) -
		 pText->w) / 2;
	  } else {
	    if (pLabel->string16->style & SF_CENTER_RIGHT) {
	      x = pLabel->size.w - (pLabel->theme->w + 5 + space) - pText->w;
	    } else {
	      x = space;
	    }
	  }
	} /* WF_ICON_CENTER_RIGHT */
	else {
	  if (flags & WF_ICON_CENTER) {
	    /* text is blit on icon */
	    goto Alone;
	  } else {		/* WF_ICON_CENTER_LEFT */
	    if (pLabel->string16->style & SF_CENTER) {
	      x = space + pLabel->theme->w + adj_size(5) + ((pLabel->size.w -
						   (space +
						    pLabel->theme->w + adj_size(5)) -
						   pText->w) / 2);
	    } else {
	      if (pLabel->string16->style & SF_CENTER_RIGHT) {
		x = pLabel->size.w - pText->w - space;
	      } else {
		x = space + pLabel->theme->w + adj_size(5);
	      }
	    }
	  }			/* WF_ICON_CENTER_LEFT */
	}
      } /* !WF_ICON_ABOVE_TEXT && !WF_ICON_UNDER_TEXT */
      else {
	goto Alone;
      }
    } /* pLabel->theme == Icon */
    else {
      y = (pLabel->size.h - pText->h) / 2;
    Alone:
      if (pLabel->string16->style & SF_CENTER) {
	x = (pLabel->size.w - pText->w) / 2;
      } else {
	if (pLabel->string16->style & SF_CENTER_RIGHT) {
	  x = pLabel->size.w - pText->w - space;
	} else {
	  x = space;
	}
      }
    }

    dst.x = pLabel->size.x + x;
    dst.y = pLabel->size.y + y;

    ret = alphablit(pText, NULL, pLabel->dst->surface, &dst);
    FREESURFACE(pText);

  }

  SDL_SetClipRect(pLabel->dst->surface, NULL);
  return ret;
}

/**************************************************************************
  ...
**************************************************************************/
int draw_label(struct widget *pLabel, Sint16 start_x, Sint16 start_y)
{
  pLabel->size.x = start_x;
  pLabel->size.y = start_y;
  return redraw_label(pLabel);
}
