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
#include "gui_iconv.h"
#include "gui_id.h"
#include "gui_tilespec.h"
#include "mapview.h"
#include "themespec.h"
#include "unistring.h"

#include "widget.h"
#include "widget_p.h"

struct UniChar {
  struct UniChar *next;
  struct UniChar *prev;
  Uint16 chr[2];
  SDL_Surface *pTsurf;
};

struct EDIT {
  struct UniChar *pBeginTextChain;
  struct UniChar *pEndTextChain;
  struct UniChar *pInputChain;
  SDL_Surface *pBg;
  struct widget *pWidget;
  int ChainLen;
  int Start_X;
  int Truelength;
  int InputChain_X;
};

static size_t chainlen(const struct UniChar *pChain);
static void del_chain(struct UniChar *pChain);
static struct UniChar * text2chain(const Uint16 *pInText);
static Uint16 * chain2text(const struct UniChar *pInChain, size_t len);

static int (*baseclass_redraw)(struct widget *pwidget);

/**************************************************************************
...
**************************************************************************/
static int redraw_edit_chain(struct EDIT *pEdt)
{
  struct UniChar *pInputChain_TMP;
  SDL_Rect Dest, Dest_Copy = {0, 0, 0, 0};
  int iStart_Mod_X;
  int ret;

  Dest_Copy.x = pEdt->pWidget->size.x;
  Dest_Copy.y = pEdt->pWidget->size.y;

  ret = (*baseclass_redraw)(pEdt->pWidget);
  if (ret != 0) {
    return ret;
  }

  /* blit theme */
  Dest = Dest_Copy;

  alphablit(pEdt->pBg, NULL, pEdt->pWidget->dst->surface, &Dest);

  /* set start parametrs */
  pInputChain_TMP = pEdt->pBeginTextChain;
  iStart_Mod_X = 0;

  Dest_Copy.y += (pEdt->pBg->h - pInputChain_TMP->pTsurf->h) / 2;
  Dest_Copy.x += pEdt->Start_X;

  /* draw loop */
  while (pInputChain_TMP) {
    Dest_Copy.x += iStart_Mod_X;
    /* chech if we draw inside of edit rect */
    if (Dest_Copy.x > pEdt->pWidget->size.x + pEdt->pBg->w - 4) {
      break;
    }

    if (Dest_Copy.x > pEdt->pWidget->size.x) {
      Dest = Dest_Copy;
      alphablit(pInputChain_TMP->pTsurf, NULL, pEdt->pWidget->dst->surface, &Dest);
    }

    iStart_Mod_X = pInputChain_TMP->pTsurf->w;

    /* draw cursor */
    if (pInputChain_TMP == pEdt->pInputChain) {
      Dest = Dest_Copy;

      putline(pEdt->pWidget->dst->surface, Dest.x - 1,
		  Dest.y + (pEdt->pBg->h / 8), Dest.x - 1,
		  Dest.y + pEdt->pBg->h - (pEdt->pBg->h / 4),
		  map_rgba(pEdt->pWidget->dst->surface->format,
                  *get_game_colorRGB(COLOR_THEME_EDITFIELD_CARET)));
      /* save active element position */
      pEdt->InputChain_X = Dest_Copy.x;
    }
	
    pInputChain_TMP = pInputChain_TMP->next;
  }	/* while - draw loop */

  widget_flush(pEdt->pWidget);
  
  return 0;
}

/**************************************************************************
  Create Edit Field surface ( with Text) and blit them to Main.screen,
  on position 'pEdit_Widget->size.x , pEdit_Widget->size.y'

  Graphic is taken from 'pEdit_Widget->theme'
  Text is taken from	'pEdit_Widget->sting16'

  if flag 'FW_DRAW_THEME_TRANSPARENT' is set theme will be blit
  transparent ( Alpha = 128 )

  function return Hight of created surfaces or (-1) if theme surface can't
  be created.
**************************************************************************/
static int redraw_edit(struct widget *pEdit_Widget)
{
  int ret;
  
  if (get_wstate(pEdit_Widget) == FC_WS_PRESSED) {
    return redraw_edit_chain((struct EDIT *)pEdit_Widget->data.ptr);
  } else {
    int iRet = 0;
    SDL_Rect rDest = {pEdit_Widget->size.x, pEdit_Widget->size.y, 0, 0};
    SDL_Surface *pEdit = NULL;
    SDL_Surface *pText;

    ret = (*baseclass_redraw)(pEdit_Widget);
    if (ret != 0) {
      return ret;
    }
    
    if (pEdit_Widget->string16->text &&
    	get_wflags(pEdit_Widget) & WF_PASSWD_EDIT) {
      Uint16 *backup = pEdit_Widget->string16->text;
      size_t len = unistrlen(backup) + 1;
      char *cBuf = fc_calloc(1, len);
    
      memset(cBuf, '*', len - 1);
      cBuf[len - 1] = '\0';
      pEdit_Widget->string16->text = convert_to_utf16(cBuf);
      pText = create_text_surf_from_str16(pEdit_Widget->string16);
      FC_FREE(pEdit_Widget->string16->text);
      FC_FREE(cBuf);
      pEdit_Widget->string16->text = backup;
    } else {
      pText = create_text_surf_from_str16(pEdit_Widget->string16);
    }
  
    pEdit = create_bcgnd_surf(pEdit_Widget->theme, get_wstate(pEdit_Widget),
                              pEdit_Widget->size.w, pEdit_Widget->size.h);

    if (!pEdit) {
      return -1;
    }
    
    /* blit theme */
    alphablit(pEdit, NULL, pEdit_Widget->dst->surface, &rDest);

    /* set position and blit text */
    if (pText) {
      rDest.y += (pEdit->h - pText->h) / 2;
      /* blit centred text to botton */
      if (pEdit_Widget->string16->style & SF_CENTER) {
        rDest.x += (pEdit->w - pText->w) / 2;
      } else {
        if (pEdit_Widget->string16->style & SF_CENTER_RIGHT) {
	  rDest.x += pEdit->w - pText->w - adj_size(5);
        } else {
	  rDest.x += adj_size(5);		/* cennter left */
        }
      }

      alphablit(pText, NULL, pEdit_Widget->dst->surface, &rDest);
    }
    /* pText */
    iRet = pEdit->h;

    /* Free memory */
    FREESURFACE(pText);
    FREESURFACE(pEdit);
    return iRet;
  }
  return 0;
}

/**************************************************************************
  Return length of UniChar chain.
  WARRNING: if struct UniChar has 1 member and UniChar->chr == 0 then
  this function return 1 ( not 0 like in strlen )
**************************************************************************/
static size_t chainlen(const struct UniChar *pChain)
{
  size_t length = 0;

  if (pChain) {
    while (1) {
      length++;
      if (pChain->next == NULL) {
	break;
      }
      pChain = pChain->next;
    }
  }

  return length;
}

/**************************************************************************
  Delete UniChar structure.
**************************************************************************/
static void del_chain(struct UniChar *pChain)
{
  int i, len = 0;

  if (!pChain) {
    return;
  }

  len = chainlen(pChain);

  if (len > 1) {
    pChain = pChain->next;
    for (i = 0; i < len - 1; i++) {
      FREESURFACE(pChain->prev->pTsurf);
      FC_FREE(pChain->prev);
      pChain = pChain->next;
    }
  }

  FC_FREE(pChain);
}

/**************************************************************************
  Convert Unistring ( Uint16[] ) to UniChar structure.
  Memmory alocation -> after all use need call del_chain(...) !
**************************************************************************/
static struct UniChar *text2chain(const Uint16 * pInText)
{
  int i, len;
  struct UniChar *pOutChain = NULL;
  struct UniChar *chr_tmp = NULL;

  len = unistrlen(pInText);

  if (len == 0) {
    return pOutChain;
  }

  pOutChain = fc_calloc(1, sizeof(struct UniChar));
  pOutChain->chr[0] = pInText[0];
  pOutChain->chr[1] = 0;
  chr_tmp = pOutChain;

  for (i = 1; i < len; i++) {
    chr_tmp->next = fc_calloc(1, sizeof(struct UniChar));
    chr_tmp->next->chr[0] = pInText[i];
    chr_tmp->next->chr[1] = 0;
    chr_tmp->next->prev = chr_tmp;
    chr_tmp = chr_tmp->next;
  }

  return pOutChain;
}

/**************************************************************************
  Convert UniChar structure to Unistring ( Uint16[] ).
  WARRING: Do not free UniChar structure but allocate new Unistring.   
**************************************************************************/
static Uint16 *chain2text(const struct UniChar *pInChain, size_t len)
{
  int i;
  Uint16 *pOutText = NULL;

  if (!(len && pInChain)) {
    return pOutText;
  }

  pOutText = fc_calloc(len + 1, sizeof(Uint16));
  for (i = 0; i < len; i++) {
    pOutText[i] = pInChain->chr[0];
    pInChain = pInChain->next;
  }

  return pOutText;
}

/* =================================================== */

/**************************************************************************
  Create ( malloc ) Edit Widget structure.

  Edit Theme graphic is taken from pTheme->Edit surface;
  Text is taken from 'pString16'.

  'length' parametr determinate width of Edit rect.

  This function determinate future size of Edit ( width, high ) and
  save this in: pWidget->size rectangle ( SDL_Rect )

  function return pointer to allocated Edit Widget.
**************************************************************************/
struct widget * create_edit(SDL_Surface *pBackground, struct gui_layer *pDest,
		SDL_String16 *pString16, Uint16 length, Uint32 flags)
{
  SDL_Rect buf = {0, 0, 0, 0};

  struct widget *pEdit = widget_new();

  pEdit->theme = pTheme->Edit;
  pEdit->theme2 = pBackground; /* FIXME: make somewhere use of it */
  pEdit->string16 = pString16;
  set_wflag(pEdit, (WF_FREE_STRING | WF_FREE_GFX | flags));
  set_wstate(pEdit, FC_WS_DISABLED);
  set_wtype(pEdit, WT_EDIT);
  pEdit->mod = KMOD_NONE;
  
  baseclass_redraw = pEdit->redraw;
  pEdit->redraw = redraw_edit;
  
  if (pString16) {
    pEdit->string16->style |= SF_CENTER;
    buf = str16size(pString16);
    buf.h += adj_size(4);
  }

  length = MAX(length, buf.w + adj_size(10));

  correct_size_bcgnd_surf(pTheme->Edit, &length, &buf.h);

  pEdit->size.w = length;
  pEdit->size.h = buf.h;

  if(pDest) {
    pEdit->dst = pDest;
  } else {
    pEdit->dst = add_gui_layer(pEdit->size.w, pEdit->size.h);
  }

  return pEdit;
}

/**************************************************************************
  set new x, y position and redraw edit.
**************************************************************************/
int draw_edit(struct widget *pEdit, Sint16 start_x, Sint16 start_y)
{
  pEdit->size.x = start_x;
  pEdit->size.y = start_y;

  if (get_wflags(pEdit) & WF_RESTORE_BACKGROUND) {
    refresh_widget_background(pEdit);
  }

  return redraw_edit(pEdit);
}

/**************************************************************************
  This functions are pure madness :)
  Create Edit Field surface ( with Text) and blit them to Main.screen,
  on position 'pEdit_Widget->size.x , pEdit_Widget->size.y'

  Main role of this functions are been text input to GUI.
  This code allow you to add, del unichar from unistring.

  Graphic is taken from 'pEdit_Widget->theme'
  OldText is taken from	'pEdit_Widget->sting16'

  NewText is returned to 'pEdit_Widget->sting16' ( after free OldText )

  if flag 'FW_DRAW_THEME_TRANSPARENT' is set theme will be blit
  transparent ( Alpha = 128 )

  NOTE: This functions can return NULL in 'pEdit_Widget->sting16->text' but
        never free 'pEdit_Widget->sting16' struct.
**************************************************************************/
static Uint16 edit_key_down(SDL_keysym Key, void *pData)
{
  struct EDIT *pEdt = (struct EDIT *)pData;
  struct UniChar *pInputChain_TMP;
  bool Redraw = FALSE;
      
  /* find which key is pressed */
  switch (Key.sym) {
    case SDLK_ESCAPE:
      /* exit from loop without changes */
      return ED_ESC;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
      /* exit from loop */
      return ED_RETURN;
    case SDLK_KP6:
      if(Key.mod & KMOD_NUM) {
	goto INPUT;
      }
    case SDLK_RIGHT:
    {
      /* move cursor right */
      if (pEdt->pInputChain->next) {
	
       if (pEdt->InputChain_X >= (pEdt->pWidget->size.x + pEdt->pBg->w - adj_size(10))) {
	pEdt->Start_X -= pEdt->pInputChain->pTsurf->w -
		(pEdt->pWidget->size.x + pEdt->pBg->w - adj_size(5) - pEdt->InputChain_X);
       }

	pEdt->pInputChain = pEdt->pInputChain->next;
	Redraw = TRUE;
      }
    }
    break;
    case SDLK_KP4:
      if(Key.mod & KMOD_NUM) {
	goto INPUT;
      }
    case SDLK_LEFT:
    {
      /* move cursor left */
      if (pEdt->pInputChain->prev) {
        pEdt->pInputChain = pEdt->pInputChain->prev;
	if ((pEdt->InputChain_X <=
	       (pEdt->pWidget->size.x + adj_size(9))) && (pEdt->Start_X != adj_size(5))) {
	  if (pEdt->InputChain_X != (pEdt->pWidget->size.x + adj_size(5))) {
	      pEdt->Start_X += (pEdt->pWidget->size.x - pEdt->InputChain_X + adj_size(5));
	  }

	  pEdt->Start_X += (pEdt->pInputChain->pTsurf->w);
	}
	Redraw = TRUE;
      }
    }
    break;
    case SDLK_KP7:
      if(Key.mod & KMOD_NUM) {
	goto INPUT;
      }  
    case SDLK_HOME:
    {
      /* move cursor to begin of chain (and edit field) */
      pEdt->pInputChain = pEdt->pBeginTextChain;
      Redraw = TRUE;
      pEdt->Start_X = adj_size(5);
    }
    break;
    case SDLK_KP1:
      if(Key.mod & KMOD_NUM) {
	goto INPUT;
      }
    case SDLK_END:
    {
	/* move cursor to end of chain (and edit field) */
      pEdt->pInputChain = pEdt->pEndTextChain;
      Redraw = TRUE;

      if (pEdt->pWidget->size.w - pEdt->Truelength < 0) {
	  pEdt->Start_X = pEdt->pWidget->size.w - pEdt->Truelength - adj_size(5);
      }
    }
    break;
    case SDLK_BACKSPACE:
    {
	/* del element of chain (and move cursor left) */
      if (pEdt->pInputChain->prev) {

	if ((pEdt->InputChain_X <=
	       (pEdt->pWidget->size.x + adj_size(9))) && (pEdt->Start_X != adj_size(5))) {
	  if (pEdt->InputChain_X != (pEdt->pWidget->size.x + adj_size(5))) {
	      pEdt->Start_X += (pEdt->pWidget->size.x - pEdt->InputChain_X + adj_size(5));
	  }
	  pEdt->Start_X += (pEdt->pInputChain->prev->pTsurf->w);
	}

	if (pEdt->pInputChain->prev->prev) {
	  pEdt->pInputChain->prev->prev->next = pEdt->pInputChain;
	  pInputChain_TMP = pEdt->pInputChain->prev->prev;
	  pEdt->Truelength -= pEdt->pInputChain->prev->pTsurf->w;
	  FREESURFACE(pEdt->pInputChain->prev->pTsurf);
	  FC_FREE(pEdt->pInputChain->prev);
	  pEdt->pInputChain->prev = pInputChain_TMP;
	} else {
	  pEdt->Truelength -= pEdt->pInputChain->prev->pTsurf->w;
	  FREESURFACE(pEdt->pInputChain->prev->pTsurf);
	  FC_FREE(pEdt->pInputChain->prev);
	  pEdt->pBeginTextChain = pEdt->pInputChain;
	}
	
	pEdt->ChainLen--;
	Redraw = TRUE;
      }
    }
    break;
    case SDLK_KP_PERIOD:
      if(Key.mod & KMOD_NUM) {
	goto INPUT;
      }  
    case SDLK_DELETE:
    {
	/* del element of chain */
      if (pEdt->pInputChain->next && pEdt->pInputChain->prev) {
	pEdt->pInputChain->prev->next = pEdt->pInputChain->next;
	pEdt->pInputChain->next->prev = pEdt->pInputChain->prev;
	pInputChain_TMP = pEdt->pInputChain->next;
	pEdt->Truelength -= pEdt->pInputChain->pTsurf->w;
	FREESURFACE(pEdt->pInputChain->pTsurf);
	FC_FREE(pEdt->pInputChain);
	pEdt->pInputChain = pInputChain_TMP;
	pEdt->ChainLen--;
	Redraw = TRUE;
      }

      if (pEdt->pInputChain->next && !pEdt->pInputChain->prev) {
	pEdt->pInputChain = pEdt->pInputChain->next;
	pEdt->Truelength -= pEdt->pInputChain->prev->pTsurf->w;
	FREESURFACE(pEdt->pInputChain->prev->pTsurf);
	FC_FREE(pEdt->pInputChain->prev);
	pEdt->pBeginTextChain = pEdt->pInputChain;
	pEdt->ChainLen--;
	Redraw = TRUE;
      }
    }
    break;
    default:
    {
INPUT:/* add new element of chain (and move cursor right) */
      if (Key.unicode) {
	if (pEdt->pInputChain != pEdt->pBeginTextChain) {
	  pInputChain_TMP = pEdt->pInputChain->prev;
	  pEdt->pInputChain->prev = fc_calloc(1, sizeof(struct UniChar));
	  pEdt->pInputChain->prev->next = pEdt->pInputChain;
	  pEdt->pInputChain->prev->prev = pInputChain_TMP;
	  pInputChain_TMP->next = pEdt->pInputChain->prev;
	} else {
	  pEdt->pInputChain->prev = fc_calloc(1, sizeof(struct UniChar));
	  pEdt->pInputChain->prev->next = pEdt->pInputChain;
	  pEdt->pBeginTextChain = pEdt->pInputChain->prev;
	}
        
        pEdt->pInputChain->prev->chr[0] = Key.unicode;        
	pEdt->pInputChain->prev->chr[1] = '\0';

	if (pEdt->pInputChain->prev->chr) {
	  if (get_wflags(pEdt->pWidget) & WF_PASSWD_EDIT) {
	    Uint16 passwd_chr[2] = {'*', '\0'};
	    
	    pEdt->pInputChain->prev->pTsurf =
	      TTF_RenderUNICODE_Blended(pEdt->pWidget->string16->font,
					  passwd_chr,
					  pEdt->pWidget->string16->fgcol);
	  } else {
	    pEdt->pInputChain->prev->pTsurf =
	      TTF_RenderUNICODE_Blended(pEdt->pWidget->string16->font,
					  pEdt->pInputChain->prev->chr,
					  pEdt->pWidget->string16->fgcol);
	  }
	  pEdt->Truelength += pEdt->pInputChain->prev->pTsurf->w;
	}

	if (pEdt->InputChain_X >= pEdt->pWidget->size.x + pEdt->pBg->w - adj_size(10)) {
	  if (pEdt->pInputChain == pEdt->pEndTextChain) {
	    pEdt->Start_X = pEdt->pBg->w - adj_size(5) - pEdt->Truelength;
	  } else {
	    pEdt->Start_X -= pEdt->pInputChain->prev->pTsurf->w -
		  (pEdt->pWidget->size.x + pEdt->pBg->w - adj_size(5) - pEdt->InputChain_X);
	  }
	}
	
	pEdt->ChainLen++;
	Redraw = TRUE;
      }
    }
    break;
  }				/* key pressed switch */
    
  if (Redraw) {
    redraw_edit_chain(pEdt);
  }
    
  return ID_ERROR;
}

static Uint16 edit_mouse_button_down(SDL_MouseButtonEvent *pButtonEvent, void *pData)
{
  struct EDIT *pEdt = (struct EDIT *)pData;
    
  if (pButtonEvent->button == SDL_BUTTON_LEFT) {
    if (!(pButtonEvent->x >= pEdt->pWidget->size.x &&
              pButtonEvent->x < pEdt->pWidget->size.x + pEdt->pBg->w &&
              pButtonEvent->y >= pEdt->pWidget->size.y &&
              pButtonEvent->y < pEdt->pWidget->size.y + pEdt->pBg->h)) {
          /* exit from loop */
          return (Uint16)ED_MOUSE;
    }
  }
  return (Uint16)ID_ERROR;
}

enum Edit_Return_Codes edit_field(struct widget *pEdit_Widget)
{
  struct EDIT pEdt;
  struct UniChar ___last;
  struct UniChar *pInputChain_TMP = NULL;
  enum Edit_Return_Codes ret;
  void *backup = pEdit_Widget->data.ptr;
  
  pEdt.pWidget = pEdit_Widget;
  pEdt.ChainLen = 0;
  pEdt.Truelength = 0;
  pEdt.Start_X = adj_size(5);
  pEdt.InputChain_X = 0;
  
  pEdit_Widget->data.ptr = (void *)&pEdt;
  
  SDL_EnableUNICODE(1);

  SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

  pEdt.pBg = create_bcgnd_surf(pEdit_Widget->theme, 2,
			       pEdit_Widget->size.w, pEdit_Widget->size.h);

  /* Creating Chain */
  pEdt.pBeginTextChain = text2chain(pEdit_Widget->string16->text);


  /* Creating Empty (Last) pice of Chain */
  pEdt.pInputChain = &___last;
  pEdt.pEndTextChain = pEdt.pInputChain;
  pEdt.pEndTextChain->chr[0] = 32;	/*spacebar */
  pEdt.pEndTextChain->chr[1] = 0;	/*spacebar */
  pEdt.pEndTextChain->next = NULL;
  pEdt.pEndTextChain->prev = NULL;
  
  /* set font style (if any ) */
  if (!((pEdit_Widget->string16->style & 0x0F) & TTF_STYLE_NORMAL)) {
    TTF_SetFontStyle(pEdit_Widget->string16->font,
		     (pEdit_Widget->string16->style & 0x0F));
  }


  pEdt.pEndTextChain->pTsurf =
      TTF_RenderUNICODE_Blended(pEdit_Widget->string16->font,
			      pEdt.pEndTextChain->chr,
			      pEdit_Widget->string16->fgcol);
  
  /* create surface for each font in chain and find chain length */
  if (pEdt.pBeginTextChain) {
    pInputChain_TMP = pEdt.pBeginTextChain;
    while (TRUE) {
      pEdt.ChainLen++;

      pInputChain_TMP->pTsurf =
	  TTF_RenderUNICODE_Blended(pEdit_Widget->string16->font,
				    pInputChain_TMP->chr,
				    pEdit_Widget->string16->fgcol);

      pEdt.Truelength += pInputChain_TMP->pTsurf->w;

      if (pInputChain_TMP->next == NULL) {
	break;
      }

      pInputChain_TMP = pInputChain_TMP->next;
    }
    /* set terminator of list */
    pInputChain_TMP->next = pEdt.pInputChain;
    pEdt.pInputChain->prev = pInputChain_TMP;
    pInputChain_TMP = NULL;
  } else {
    pEdt.pBeginTextChain = pEdt.pInputChain;
  }

  redraw_edit_chain(&pEdt);
  
  set_wstate(pEdit_Widget, FC_WS_PRESSED);
  {
    /* local loop */  
    Uint16 rety = gui_event_loop((void *)&pEdt, NULL,
  	edit_key_down, NULL, edit_mouse_button_down, NULL, NULL);
    
    if (pEdt.pBeginTextChain == pEdt.pEndTextChain) {
      pEdt.pBeginTextChain = NULL;
    }
    
    if (rety == MAX_ID) {
      ret = ED_FORCE_EXIT;
    } else {
      ret = (enum Edit_Return_Codes) rety;
      
      /* this is here becouse we have no knowladge that pEdit_Widget exist
         or nor in force exit mode from gui loop */
  
      /* reset font settings */
      if (!((pEdit_Widget->string16->style & 0x0F) & TTF_STYLE_NORMAL)) {
        TTF_SetFontStyle(pEdit_Widget->string16->font, TTF_STYLE_NORMAL);
      }
      
      if(ret != ED_ESC) {
        FC_FREE(pEdit_Widget->string16->text);
        pEdit_Widget->string16->text =
  	    chain2text(pEdt.pBeginTextChain, pEdt.ChainLen);
        pEdit_Widget->string16->n_alloc = (pEdt.ChainLen + 1) * sizeof(Uint16);
      }
      
      pEdit_Widget->data.ptr = backup;
      set_wstate(pEdit_Widget, FC_WS_NORMAL);    
    }
  }
  
  FREESURFACE(pEdt.pEndTextChain->pTsurf);
   
  del_chain(pEdt.pBeginTextChain);
  
  FREESURFACE(pEdt.pBg);
    
  /* disable repeate key */
  SDL_EnableKeyRepeat(0, SDL_DEFAULT_REPEAT_INTERVAL);

  /* disable Unicode */
  SDL_EnableUNICODE(0);
    
  return ret;
}
