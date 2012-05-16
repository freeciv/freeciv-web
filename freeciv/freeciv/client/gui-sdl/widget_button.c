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
#include "gui_tilespec.h"
#include "themespec.h"

#include "widget.h"
#include "widget_p.h"

static int (*baseclass_redraw)(struct widget *pwidget);

/**************************************************************************
  Create Icon Button image with text and Icon then blit to Dest(ination)
  on positon pIButton->size.x , pIButton->size.y.
  WARRING: pDest must exist.

  Text with atributes is taken from pIButton->string16 parameter.

  Graphic for button is taken from pIButton->theme surface 
  and blit to new created image.

  Graphic for Icon is taken from pIButton->theme2 surface and blit to new
  created image.

  function return (-1) if there are no Icon and Text.
  Else return 0.
**************************************************************************/
static int redraw_ibutton(struct widget *pIButton)
{
  SDL_Rect dest = { 0, 0, 0, 0 };
  SDL_String16 TMPString;
  SDL_Surface *pButton = NULL, *pText = NULL, *pIcon = pIButton->theme2;
  Uint16 Ix, Iy, x;
  Uint16 y = 0; /* FIXME: possibly uninitialized */
  int ret;
  
  ret = (*baseclass_redraw)(pIButton);
  if (ret != 0) {
    return ret;
  }

  if (pIButton->string16 && !(get_wflags(pIButton) & WF_WIDGET_HAS_INFO_LABEL)) {

    /* make copy of string16 */
    TMPString = *pIButton->string16;

    if (get_wstate(pIButton) == FC_WS_NORMAL) {
      TMPString.fgcol = *get_game_colorRGB(COLOR_THEME_WIDGET_NORMAL_TEXT);
    } else if (get_wstate(pIButton) == FC_WS_SELLECTED) {
      TMPString.fgcol = *get_game_colorRGB(COLOR_THEME_WIDGET_SELECTED_TEXT);
      TMPString.style |= TTF_STYLE_BOLD;
    } else if (get_wstate(pIButton) == FC_WS_PRESSED) {
      TMPString.fgcol = *get_game_colorRGB(COLOR_THEME_WIDGET_PRESSED_TEXT);
    } else if (get_wstate(pIButton) == FC_WS_DISABLED) {
      TMPString.fgcol = *get_game_colorRGB(COLOR_THEME_WIDGET_DISABLED_TEXT);
    }

    pText = create_text_surf_from_str16(&TMPString);
  }

  if (!pText && !pIcon) {
    return -1;
  }

  /* create Button graphic */
  pButton = create_bcgnd_surf(pIButton->theme, get_wstate(pIButton),
			      pIButton->size.w, pIButton->size.h);

  clear_surface(pIButton->dst->surface, &pIButton->size);
  alphablit(pButton, NULL, pIButton->dst->surface, &pIButton->size);
  FREESURFACE(pButton);

  if (pIcon) {			/* Icon */
    if (pText) {
      if (get_wflags(pIButton) & WF_ICON_CENTER_RIGHT) {
	Ix = pIButton->size.w - pIcon->w - 5;
      } else {
	if (get_wflags(pIButton) & WF_ICON_CENTER) {
	  Ix = (pIButton->size.w - pIcon->w) / 2;
	} else {
	  Ix = 5;
	}
      }

      if (get_wflags(pIButton) & WF_ICON_ABOVE_TEXT) {
	Iy = 3;
	y = 3 + pIcon->h + 3 + (pIButton->size.h -
				(pIcon->h + 6) - pText->h) / 2;
      } else {
	if (get_wflags(pIButton) & WF_ICON_UNDER_TEXT) {
	  y = 3 + (pIButton->size.h - (pIcon->h + 3) - pText->h) / 2;
	  Iy = y + pText->h + 3;
	} else {		/* center */
	  Iy = (pIButton->size.h - pIcon->h) / 2;
	  y = (pIButton->size.h - pText->h) / 2;
	}
      }
    } else {			/* no text */
      Iy = (pIButton->size.h - pIcon->h) / 2;
      Ix = (pIButton->size.w - pIcon->w) / 2;
    }

    if (get_wstate(pIButton) == FC_WS_PRESSED) {
      Ix += 1;
      Iy += 1;
    }


    dest.x = pIButton->size.x + Ix;
    dest.y = pIButton->size.y + Iy;

    ret = alphablit(pIcon, NULL, pIButton->dst->surface, &dest);
    if (ret) {
      FREESURFACE(pText);
      return ret - 10;
    }
  }

  if (pText) {

    if (pIcon) {
      if (!(get_wflags(pIButton) & WF_ICON_ABOVE_TEXT) &&
	  !(get_wflags(pIButton) & WF_ICON_UNDER_TEXT)) {
	if (get_wflags(pIButton) & WF_ICON_CENTER_RIGHT) {
	  if (pIButton->string16->style & SF_CENTER) {
	    x = (pIButton->size.w - (pIcon->w + 5) - pText->w) / 2;
	  } else {
	    if (pIButton->string16->style & SF_CENTER_RIGHT) {
	      x = pIButton->size.w - (pIcon->w + 7) - pText->w;
	    } else {
	      x = 5;
	    }
	  }
	} /* end WF_ICON_CENTER_RIGHT */
	else {
	  if (get_wflags(pIButton) & WF_ICON_CENTER) {
	    /* text is blit on icon */
	    goto Alone;
	  } /* end WF_ICON_CENTER */
	  else {		/* icon center left - default */
	    if (pIButton->string16->style & SF_CENTER) {
	      x = 5 + pIcon->w + ((pIButton->size.w -
				   (pIcon->w + 5) - pText->w) / 2);
	    } else {
	      if (pIButton->string16->style & SF_CENTER_RIGHT) {
		x = pIButton->size.w - pText->w - 5;
	      } else {		/* text center left */
		x = 5 + pIcon->w + 3;
	      }
	    }

	  }			/* end icon center left - default */

	}
	/* 888888888888888888 */
      } else {
	goto Alone;
      }
    } else {
      /* !pIcon */
      y = (pIButton->size.h - pText->h) / 2;
    Alone:
      if (pIButton->string16->style & SF_CENTER) {
	x = (pIButton->size.w - pText->w) / 2;
      } else {
	if (pIButton->string16->style & SF_CENTER_RIGHT) {
	  x = pIButton->size.w - pText->w - 5;
	} else {
	  x = 5;
	}
      }
    }

    if (get_wstate(pIButton) == FC_WS_PRESSED) {
      x += 1;
    } else {
      y -= 1;
    }

    dest.x = pIButton->size.x + x;
    dest.y = pIButton->size.y + y;

    ret = alphablit(pText, NULL, pIButton->dst->surface, &dest);
  }

  FREESURFACE(pText);

  return 0;
}

/**************************************************************************
  Create Icon Button image with text and Icon then blit to Dest(ination)
  on positon pTIButton->size.x , pTIButton->size.y. WARRING: pDest must
  exist.

  Text with atributes is taken from pTIButton->string16 parameter.

  Graphic for button is taken from pTIButton->theme surface 
  and blit to new created image.

  Graphic for Icon Theme is taken from pTIButton->theme2 surface 
  and blit to new created image.

  function return (-1) if there are no Icon and Text.  Else return 0.
**************************************************************************/
static int redraw_tibutton(struct widget *pTIButton)
{
  int iRet = 0;
  SDL_Surface *pIcon;
  SDL_Surface *pCopy_Of_Icon_Theme;

  iRet = (*baseclass_redraw)(pTIButton);
  if (iRet != 0) {
    return iRet;
  }
  
  pIcon = create_icon_from_theme(pTIButton->theme2, get_wstate(pTIButton));
  pCopy_Of_Icon_Theme = pTIButton->theme2;

  pTIButton->theme2 = pIcon;

  iRet = redraw_ibutton(pTIButton);

  FREESURFACE(pTIButton->theme2);
  pTIButton->theme2 = pCopy_Of_Icon_Theme;

  return iRet;
}

/**************************************************************************
  Create ( malloc ) Icon (theme)Button Widget structure.

  Icon graphic is taken from 'pIcon' surface (don't change with button
  changes );  Button Theme graphic is taken from pTheme->Button surface;
  Text is taken from 'pString16'.

  This function determinate future size of Button ( width, high ) and
  save this in: pWidget->size rectangle ( SDL_Rect )

  function return pointer to allocated Button Widget.
**************************************************************************/
struct widget * create_icon_button(SDL_Surface *pIcon, struct gui_layer *pDest,
			  SDL_String16 *pStr, Uint32 flags)
{
  SDL_Rect buf = {0, 0, 0, 0};
  Uint16 w = 0, h = 0;
  struct widget *pButton;

  if (!pIcon && !pStr) {
    return NULL;
  }

  pButton = widget_new();

  pButton->theme = pTheme->Button;
  pButton->theme2 = pIcon;
  pButton->string16 = pStr;
  set_wflag(pButton, (WF_FREE_STRING | flags));
  set_wstate(pButton, FC_WS_DISABLED);
  set_wtype(pButton, WT_I_BUTTON);
  pButton->mod = KMOD_NONE;
  pButton->dst = pDest;

  baseclass_redraw = pButton->redraw;  
  pButton->redraw = redraw_ibutton;

  if (pStr && !(flags & WF_WIDGET_HAS_INFO_LABEL)) {
    pButton->string16->style |= SF_CENTER;
    /* if BOLD == true then longest wight */
    if (!(pStr->style & TTF_STYLE_BOLD)) {
      pStr->style |= TTF_STYLE_BOLD;
      buf = str16size(pStr);
      pStr->style &= ~TTF_STYLE_BOLD;
    } else {
      buf = str16size(pStr);
    }

    w = MAX(w, buf.w);
    h = MAX(h, buf.h);
  }

  if (pIcon) {
    if (pStr) {
      if ((flags & WF_ICON_UNDER_TEXT) || (flags & WF_ICON_ABOVE_TEXT)) {
	w = MAX(w, pIcon->w + adj_size(2));
	h = MAX(h, buf.h + pIcon->h + adj_size(4));
      } else {
	w = MAX(w, buf.w + pIcon->w + adj_size(20));
	h = MAX(h, pIcon->h + adj_size(2));
      }
    } else {
      w = MAX(w, pIcon->w + adj_size(2));
      h = MAX(h, pIcon->h + adj_size(2));
    }
  } else {
    w += adj_size(10);
    h += adj_size(2);
  }

  correct_size_bcgnd_surf(pTheme->Button, &w, &h);

  pButton->size.w = w;
  pButton->size.h = h;

  return pButton;
}

/**************************************************************************
  Create ( malloc ) Theme Icon (theme)Button Widget structure.

  Icon Theme graphic is taken from 'pIcon_theme' surface ( change with
  button changes ); Button Theme graphic is taken from pTheme->Button
  surface; Text is taken from 'pString16'.

  This function determinate future size of Button ( width, high ) and
  save this in: pWidget->size rectangle ( SDL_Rect )

  function return pointer to allocated Button Widget.
**************************************************************************/
struct widget * create_themeicon_button(SDL_Surface *pIcon_theme,
		struct gui_layer *pDest, SDL_String16 *pString16, Uint32 flags)
{
  /* extract a single icon */
  SDL_Surface *pIcon = create_icon_from_theme(pIcon_theme, 1);
  struct widget *pButton = create_icon_button(pIcon, pDest, pString16, flags);

  FREESURFACE(pButton->theme2);
  pButton->theme2 = pIcon_theme;
  set_wtype(pButton, WT_TI_BUTTON);

  pButton->redraw = redraw_tibutton;

  return pButton;
}

/**************************************************************************
  Steate Button image with text and Icon.  Then blit to Main.screen on
  positon start_x , start_y.

  Text with atributes is taken from pButton->string16 parameter.

  Graphic for button is taken from pButton->theme surface and blit to new
  created image.

  Graphic for Icon theme is taken from pButton->theme2 surface and blit to
  new created image.

  function return (-1) if there are no Icon and Text.
  Else return 0.
**************************************************************************/
int draw_tibutton(struct widget *pButton, Sint16 start_x, Sint16 start_y)
{
  pButton->size.x = start_x;
  pButton->size.y = start_y;
  return redraw_tibutton(pButton);
}

/**************************************************************************
  Create Button image with text and Icon.
  Then blit to Main.screen on positon start_x , start_y.

   Text with atributes is taken from pButton->string16 parameter.

   Graphic for button is taken from pButton->theme surface 
   and blit to new created image.

  Graphic for Icon is taken from pButton->theme2 surface 
  and blit to new created image.

  function return (-1) if there are no Icon and Text.
  Else return 0.
**************************************************************************/
int draw_ibutton(struct widget *pButton, Sint16 start_x, Sint16 start_y)
{
  pButton->size.x = start_x;
  pButton->size.y = start_y;
  return redraw_ibutton(pButton);
}
