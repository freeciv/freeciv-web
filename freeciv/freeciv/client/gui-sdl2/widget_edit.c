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
#include "gui_iconv.h"
#include "gui_id.h"
#include "gui_tilespec.h"
#include "mapview.h"
#include "themespec.h"
#include "unistring.h"
#include "utf8string.h"
#include "widget.h"
#include "widget_p.h"

struct Utf8Char {
  struct Utf8Char *next;
  struct Utf8Char *prev;
  int bytes;
  char chr[7];
  SDL_Surface *pTsurf;
};

struct EDIT {
  struct Utf8Char *pBeginTextChain;
  struct Utf8Char *pEndTextChain;
  struct Utf8Char *pInputChain;
  SDL_Surface *pBg;
  struct widget *pWidget;
  int ChainLen;
  int Start_X;
  int Truelength;
  int InputChain_X;
};

static size_t chainlen(const struct Utf8Char *pChain);
static void del_chain(struct Utf8Char *pChain);
static struct Utf8Char *text2chain(const char *text_in);
static char *chain2text(const struct Utf8Char *pInChain, size_t len, size_t *size);

static int (*baseclass_redraw)(struct widget *pwidget);

/**********************************************************************//**
  Draw the text being edited.
**************************************************************************/
static int redraw_edit_chain(struct EDIT *pEdt)
{
  struct Utf8Char *pInputChain_TMP;
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

  alphablit(pEdt->pBg, NULL, pEdt->pWidget->dst->surface, &Dest, 255);

  /* set start parameters */
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
      alphablit(pInputChain_TMP->pTsurf, NULL, pEdt->pWidget->dst->surface, &Dest,
                255);
    }

    iStart_Mod_X = pInputChain_TMP->pTsurf->w;

    /* draw cursor */
    if (pInputChain_TMP == pEdt->pInputChain) {
      Dest = Dest_Copy;

      create_line(pEdt->pWidget->dst->surface,
                  Dest.x - 1, Dest.y + (pEdt->pBg->h / 8),
                  Dest.x - 1, Dest.y + pEdt->pBg->h - (pEdt->pBg->h / 4),
                  get_theme_color(COLOR_THEME_EDITFIELD_CARET));
      /* save active element position */
      pEdt->InputChain_X = Dest_Copy.x;
    }

    pInputChain_TMP = pInputChain_TMP->next;
  } /* while - draw loop */

  widget_flush(pEdt->pWidget);

  return 0;
}

/**********************************************************************//**
  Create Edit Field surface ( with Text) and blit them to Main.screen,
  on position 'pEdit_Widget->size.x , pEdit_Widget->size.y'

  Graphic is taken from 'pEdit_Widget->theme'
  Text is taken from	'pEdit_Widget->string_utf8'

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

    if (pEdit_Widget->string_utf8->text != NULL
        && get_wflags(pEdit_Widget) & WF_PASSWD_EDIT) {
      char *backup = pEdit_Widget->string_utf8->text;
      size_t len = strlen(backup) + 1;
      char *cbuf = fc_calloc(1, len);

      memset(cbuf, '*', len - 1);
      cbuf[len - 1] = '\0';
      pEdit_Widget->string_utf8->text = cbuf;
      pText = create_text_surf_from_utf8(pEdit_Widget->string_utf8);
      FC_FREE(cbuf);
      pEdit_Widget->string_utf8->text = backup;
    } else {
      pText = create_text_surf_from_utf8(pEdit_Widget->string_utf8);
    }

    pEdit = create_bcgnd_surf(pEdit_Widget->theme, get_wstate(pEdit_Widget),
                              pEdit_Widget->size.w, pEdit_Widget->size.h);

    if (!pEdit) {
      return -1;
    }

    /* blit theme */
    alphablit(pEdit, NULL, pEdit_Widget->dst->surface, &rDest, 255);

    /* set position and blit text */
    if (pText) {
      rDest.y += (pEdit->h - pText->h) / 2;
      /* blit centred text to botton */
      if (pEdit_Widget->string_utf8->style & SF_CENTER) {
        rDest.x += (pEdit->w - pText->w) / 2;
      } else {
        if (pEdit_Widget->string_utf8->style & SF_CENTER_RIGHT) {
          rDest.x += pEdit->w - pText->w - adj_size(5);
        } else {
          rDest.x += adj_size(5); /* center left */
        }
      }

      alphablit(pText, NULL, pEdit_Widget->dst->surface, &rDest, 255);
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

/**********************************************************************//**
  Return length of Utf8Char chain.
  WARRNING: if struct Utf8Char has 1 member and Utf8Char->chr == 0 then
  this function return 1 ( not 0 like in strlen )
**************************************************************************/
static size_t chainlen(const struct Utf8Char *pChain)
{
  size_t length = 0;

  if (pChain) {
    while (TRUE) {
      length++;
      if (pChain->next == NULL) {
	break;
      }
      pChain = pChain->next;
    }
  }

  return length;
}

/**********************************************************************//**
  Delete Utf8Char structure.
**************************************************************************/
static void del_chain(struct Utf8Char *pChain)
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

/**********************************************************************//**
  Convert utf8 string to Utf8Char structure.
  Memory alocation -> after all use need call del_chain(...) !
**************************************************************************/
static struct Utf8Char *text2chain(const char *text_in)
{
  int i, len;
  struct Utf8Char *out_chain = NULL;
  struct Utf8Char *chr_tmp = NULL;
  int j;

  if (text_in == NULL) {
    return NULL;
  }

  len = strlen(text_in);

  if (len == 0) {
    return NULL;
  }

  out_chain = fc_calloc(1, sizeof(struct Utf8Char));
  out_chain->chr[0] = text_in[0];
  for (j = 1; (text_in[j] & (128 + 64)) == 128; j++) {
    out_chain->chr[j] = text_in[j];
  }
  out_chain->bytes = j;
  chr_tmp = out_chain;

  for (i = 1; i < len; i += j) {
    chr_tmp->next = fc_calloc(1, sizeof(struct Utf8Char));
    chr_tmp->next->chr[0] = text_in[i];
    for (j = 1; (text_in[i + j] & (128 + 64)) == 128; j++) {
      chr_tmp->next->chr[j] = text_in[i + j];
    }
    chr_tmp->next->bytes  = j;
    chr_tmp->next->prev = chr_tmp;
    chr_tmp = chr_tmp->next;
  }

  return out_chain;
}

/**********************************************************************//**
  Convert Utf8Char structure to chars
  WARNING: Do not free Utf8Char structure but allocates new char array.
**************************************************************************/
static char *chain2text(const struct Utf8Char *pInChain, size_t len,
                        size_t *size)
{
  int i;
  char *pOutText = NULL;
  int oi = 0;
  int total_size = 0;

  if (!(len && pInChain)) {
    return pOutText;
  }

  pOutText = fc_calloc(8, len + 1);
  for (i = 0; i < len; i++) {
    int j;

    for (j = 0; j < pInChain->bytes && i < len; j++) {
      pOutText[oi++] = pInChain->chr[j];
    }

    total_size += pInChain->bytes;
    pInChain = pInChain->next;
  }

  *size = total_size;

  return pOutText;
}

/* =================================================== */

/**********************************************************************//**
  Create ( malloc ) Edit Widget structure.

  Edit Theme graphic is taken from current_theme->Edit surface;
  Text is taken from 'pstr'.

  'length' parameter determinate width of Edit rect.

  This function determinate future size of Edit ( width, height ) and
  save this in: pWidget->size rectangle ( SDL_Rect )

  function return pointer to allocated Edit Widget.
**************************************************************************/
struct widget *create_edit(SDL_Surface *pBackground, struct gui_layer *pDest,
                           utf8_str *pstr, int length, Uint32 flags)
{
  SDL_Rect buf = {0, 0, 0, 0};
  struct widget *pEdit = widget_new();

  pEdit->theme = current_theme->Edit;
  pEdit->theme2 = pBackground; /* FIXME: make somewhere use of it */
  pEdit->string_utf8 = pstr;
  set_wflag(pEdit, (WF_FREE_STRING | WF_FREE_GFX | flags));
  set_wstate(pEdit, FC_WS_DISABLED);
  set_wtype(pEdit, WT_EDIT);
  pEdit->mod = KMOD_NONE;

  baseclass_redraw = pEdit->redraw;
  pEdit->redraw = redraw_edit;

  if (pstr != NULL) {
    pEdit->string_utf8->style |= SF_CENTER;
    utf8_str_size(pstr, &buf);
    buf.h += adj_size(4);
  }

  length = MAX(length, buf.w + adj_size(10));

  correct_size_bcgnd_surf(current_theme->Edit, &length, &buf.h);

  pEdit->size.w = length;
  pEdit->size.h = buf.h;

  if (pDest) {
    pEdit->dst = pDest;
  } else {
    pEdit->dst = add_gui_layer(pEdit->size.w, pEdit->size.h);
  }

  return pEdit;
}

/**********************************************************************//**
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

/**********************************************************************//**
  This functions are pure madness :)
  Create Edit Field surface ( with Text) and blit them to Main.screen,
  on position 'pEdit_Widget->size.x , pEdit_Widget->size.y'

  Main role of this functions are been text input to GUI.
  This code allow you to add, del unichar from unistring.

  Graphic is taken from 'pEdit_Widget->theme'
  OldText is taken from	'pEdit_Widget->string_utf8'

  NewText is returned to 'pEdit_Widget->string_utf8' ( after free OldText )

  if flag 'FW_DRAW_THEME_TRANSPARENT' is set theme will be blit
  transparent ( Alpha = 128 )

  NOTE: This functions can return NULL in 'pEdit_Widget->string_utf8->text' but
        never free 'pEdit_Widget->string_utf8' struct.
**************************************************************************/
static Uint16 edit_key_down(SDL_Keysym key, void *pData)
{
  struct EDIT *pEdt = (struct EDIT *)pData;
  struct Utf8Char *pInputChain_TMP;
  bool Redraw = FALSE;

  /* find which key is pressed */
  switch (key.sym) {
    case SDLK_ESCAPE:
      /* exit from loop without changes */
      return ED_ESC;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
      /* exit from loop */
      return ED_RETURN;
      /*
    case SDLK_KP6:
      if (key.mod & KMOD_NUM) {
	goto INPUT;
      }
      */
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
    /*
    case SDLK_KP4:
      if (key.mod & KMOD_NUM) {
        goto INPUT;
      }
    */
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
    /*
    case SDLK_KP7:
      if (key.mod & KMOD_NUM) {
	goto INPUT;
      }
    */
    case SDLK_HOME:
    {
      /* move cursor to begin of chain (and edit field) */
      pEdt->pInputChain = pEdt->pBeginTextChain;
      Redraw = TRUE;
      pEdt->Start_X = adj_size(5);
    }
    break;
    /*
    case SDLK_KP1:
      if (key.mod & KMOD_NUM) {
	goto INPUT;
      }
    */
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
    /*
    case SDLK_KP_PERIOD:
      if (key.mod & KMOD_NUM) {
	goto INPUT;
      }
    */
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
    break;
  } /* key pressed switch */

  if (Redraw) {
    redraw_edit_chain(pEdt);
  }

  return ID_ERROR;
}

/**********************************************************************//**
  Handle textinput strings coming to the edit widget
**************************************************************************/
static Uint16 edit_textinput(char *text, void *pData)
{
  struct EDIT *pEdt = (struct EDIT *)pData;
  struct Utf8Char *pInputChain_TMP;
  int i;

  for (i = 0; text[i] != '\0';) {
    int charlen = 1;
    unsigned char leading = text[i++];
    int sum = 128 + 64;
    int addition = 32;

    /* add new element of chain (and move cursor right) */
    if (pEdt->pInputChain != pEdt->pBeginTextChain) {
      pInputChain_TMP = pEdt->pInputChain->prev;
      pEdt->pInputChain->prev = fc_calloc(1, sizeof(struct Utf8Char));
      pEdt->pInputChain->prev->next = pEdt->pInputChain;
      pEdt->pInputChain->prev->prev = pInputChain_TMP;
      pInputChain_TMP->next = pEdt->pInputChain->prev;
    } else {
      pEdt->pInputChain->prev = fc_calloc(1, sizeof(struct Utf8Char));
      pEdt->pInputChain->prev->next = pEdt->pInputChain;
      pEdt->pBeginTextChain = pEdt->pInputChain->prev;
    }

    pEdt->pInputChain->prev->chr[0] = leading;
    /* UTF-8 multibyte handling */
    while (leading >= sum) {
      pEdt->pInputChain->prev->chr[charlen++] = text[i++];
      sum += addition;
      addition /= 2;
    }
    pEdt->pInputChain->prev->chr[charlen] = '\0';
    pEdt->pInputChain->prev->bytes = charlen;

    if (get_wflags(pEdt->pWidget) & WF_PASSWD_EDIT) {
      char passwd_chr[2] = {'*', '\0'};

      pEdt->pInputChain->prev->pTsurf =
        TTF_RenderUTF8_Blended(pEdt->pWidget->string_utf8->font,
                               passwd_chr,
                               pEdt->pWidget->string_utf8->fgcol);
    } else {
      pEdt->pInputChain->prev->pTsurf =
        TTF_RenderUTF8_Blended(pEdt->pWidget->string_utf8->font,
                               pEdt->pInputChain->prev->chr,
                               pEdt->pWidget->string_utf8->fgcol);
    }
    pEdt->Truelength += pEdt->pInputChain->prev->pTsurf->w;

    if (pEdt->InputChain_X >= pEdt->pWidget->size.x + pEdt->pBg->w - adj_size(10)) {
      if (pEdt->pInputChain == pEdt->pEndTextChain) {
        pEdt->Start_X = pEdt->pBg->w - adj_size(5) - pEdt->Truelength;
      } else {
        pEdt->Start_X -= pEdt->pInputChain->prev->pTsurf->w -
          (pEdt->pWidget->size.x + pEdt->pBg->w - adj_size(5) - pEdt->InputChain_X);
      }
    }

    pEdt->ChainLen++;
  }

  redraw_edit_chain(pEdt);

  return ID_ERROR;
}

/**********************************************************************//**
  Handle mouse down events on edit widget.
**************************************************************************/
static Uint16 edit_mouse_button_down(SDL_MouseButtonEvent *pButtonEvent,
                                     void *pData)
{
  struct EDIT *pEdt = (struct EDIT *)pData;

  if (pButtonEvent->button == SDL_BUTTON_LEFT) {
    if (!(pButtonEvent->x >= pEdt->pWidget->size.x
          && pButtonEvent->x < pEdt->pWidget->size.x + pEdt->pBg->w
          && pButtonEvent->y >= pEdt->pWidget->size.y
          && pButtonEvent->y < pEdt->pWidget->size.y + pEdt->pBg->h)) {
      /* exit from loop */
      return (Uint16)ED_MOUSE;
    }
  }

  return (Uint16)ID_ERROR;
}

/**********************************************************************//**
  Handle active edit. Returns what should happen to the edit
  next.
**************************************************************************/
enum Edit_Return_Codes edit_field(struct widget *pEdit_Widget)
{
  struct EDIT pEdt;
  struct Utf8Char ___last;
  struct Utf8Char *pInputChain_TMP = NULL;
  enum Edit_Return_Codes ret;
  void *backup = pEdit_Widget->data.ptr;

  pEdt.pWidget = pEdit_Widget;
  pEdt.ChainLen = 0;
  pEdt.Truelength = 0;
  pEdt.Start_X = adj_size(5);
  pEdt.InputChain_X = 0;

  pEdit_Widget->data.ptr = (void *)&pEdt;

#if 0
  SDL_EnableUNICODE(1);
  SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
#endif /* 0 */

  pEdt.pBg = create_bcgnd_surf(pEdit_Widget->theme, 2,
                               pEdit_Widget->size.w, pEdit_Widget->size.h);

  /* Creating Chain */
  pEdt.pBeginTextChain = text2chain(pEdit_Widget->string_utf8->text);

  /* Creating Empty (Last) pice of Chain */
  pEdt.pInputChain = &___last;
  pEdt.pEndTextChain = pEdt.pInputChain;
  pEdt.pEndTextChain->chr[0] = 32; /* spacebar */
  pEdt.pEndTextChain->chr[1] = 0;  /* spacebar */
  pEdt.pEndTextChain->next = NULL;
  pEdt.pEndTextChain->prev = NULL;

  /* set font style (if any ) */
  if (!((pEdit_Widget->string_utf8->style & 0x0F) & TTF_STYLE_NORMAL)) {
    TTF_SetFontStyle(pEdit_Widget->string_utf8->font,
                     (pEdit_Widget->string_utf8->style & 0x0F));
  }

  pEdt.pEndTextChain->pTsurf =
      TTF_RenderUTF8_Blended(pEdit_Widget->string_utf8->font,
                             pEdt.pEndTextChain->chr,
                             pEdit_Widget->string_utf8->fgcol);

  /* create surface for each font in chain and find chain length */
  if (pEdt.pBeginTextChain) {
    pInputChain_TMP = pEdt.pBeginTextChain;

    while (TRUE) {
      pEdt.ChainLen++;

      if (get_wflags(pEdit_Widget) & WF_PASSWD_EDIT) {
        const char passwd_chr[2] = {'*', '\0'};

        pInputChain_TMP->pTsurf =
          TTF_RenderUTF8_Blended(pEdit_Widget->string_utf8->font,
                                 passwd_chr,
                                 pEdit_Widget->string_utf8->fgcol);
      } else {
        pInputChain_TMP->pTsurf =
          TTF_RenderUTF8_Blended(pEdit_Widget->string_utf8->font,
                                 pInputChain_TMP->chr,
                                 pEdit_Widget->string_utf8->fgcol);
      }

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
                                 edit_key_down, NULL, edit_textinput,
                                 edit_mouse_button_down, NULL, NULL);

    if (pEdt.pBeginTextChain == pEdt.pEndTextChain) {
      pEdt.pBeginTextChain = NULL;
    }

    if (rety == MAX_ID) {
      ret = ED_FORCE_EXIT;
    } else {
      ret = (enum Edit_Return_Codes) rety;

      /* this is here because we have no knowledge that pEdit_Widget exist
         or nor in force exit mode from gui loop */

      /* reset font settings */
      if (!((pEdit_Widget->string_utf8->style & 0x0F) & TTF_STYLE_NORMAL)) {
        TTF_SetFontStyle(pEdit_Widget->string_utf8->font, TTF_STYLE_NORMAL);
      }

      if (ret != ED_ESC) {
        size_t len = 0;

        FC_FREE(pEdit_Widget->string_utf8->text);
        pEdit_Widget->string_utf8->text =
          chain2text(pEdt.pBeginTextChain, pEdt.ChainLen, &len);
        pEdit_Widget->string_utf8->n_alloc = len + 1;
      }

      pEdit_Widget->data.ptr = backup;
      set_wstate(pEdit_Widget, FC_WS_NORMAL);
    }
  }

  FREESURFACE(pEdt.pEndTextChain->pTsurf);

  del_chain(pEdt.pBeginTextChain);

  FREESURFACE(pEdt.pBg);

  /* disable repeat key */

#if 0
  SDL_EnableKeyRepeat(0, SDL_DEFAULT_REPEAT_INTERVAL);

  /* disable Unicode */
  SDL_EnableUNICODE(0);
#endif /* 0 */

  return ret;
}
