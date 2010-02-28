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

/***************************************************************************
                          gui_string.c  -  description
                             -------------------
    begin                : June 30 2002
    copyright            : (C) 2002 by Rafał Bursig
    email                : Rafał Bursig <bursig@poczta.fm>
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include "SDL.h"

/* utility */
#include "fcintl.h"
#include "log.h"

/* gui-sdl */
#include "colors.h"
#include "graphics.h"
#include "gui_iconv.h"
#include "gui_main.h"
#include "themespec.h"
#include "unistring.h"

#include "gui_string.h"

/* =================================================== */

static struct TTF_Font_Chain {
  struct TTF_Font_Chain *next;
  TTF_Font *font;
  Uint16 ptsize;		/* size of font */
  Uint16 count;			/* number of strings alliased with this font */
} *Font_TAB = NULL;

static unsigned int Sizeof_Font_TAB;
static char *pFont_with_FullPath = NULL;

static TTF_Font *load_font(Uint16 ptsize);

static SDL_Surface *create_str16_surf(SDL_String16 * pString);
static SDL_Surface *create_str16_multi_surf(SDL_String16 * pString);

/**************************************************************************
  ...
**************************************************************************/
#ifdef SMALL_SCREEN
int adj_font(int size) {
  switch(size) {
    case 24:
      return 12;
    case 20:
      return 12;
    case 16:
      return 10;      
    case 14:
      return 8;
    case 13:
      return 8;
    case 12:
      return 8;
    case 11:
      return 7;
    case 10:
      return 7;
    case 8:
      return 6;
    default:
      return size;    
  }
}
#endif

/**************************************************************************
  ...
**************************************************************************/
SDL_Rect str16size(SDL_String16 *pString16)
{
  SDL_Rect Ret = {0, 0, 0, 0};
  
  if (pString16 && pString16->text && pString16->text != '\0') {
    Uint16 *pStr16 = pString16->text;
    Uint16 c = *pStr16;
    bool new_line = FALSE;
    int w, h;
    
    /* find '\n' */
    while (c != '\0') {
      if (c == 10) {
	new_line = TRUE;
	break;
      }
      pStr16++;
      c = *pStr16;
    }
   
    if (!((pString16->style & 0x0F) & TTF_STYLE_NORMAL)) {
      TTF_SetFontStyle(pString16->font, (pString16->style & 0x0F));
    }
    
    if (new_line) {
      int ww, hh, count = 0;
      Uint16 **UniTexts = create_new_line_unistrings(pString16->text);
      
      w = 0;
      h = 0;
      while (UniTexts[count]) {
        if (TTF_SizeUNICODE(pString16->font, UniTexts[count], &ww, &hh) < 0) {
          do {
	    FC_FREE(UniTexts[count]);
            count++;
	  } while(UniTexts[count]);
	  die("TTF_SizeUNICODE return ERROR !");
        }
        w = MAX(w, ww);
        h += hh;
        FC_FREE(UniTexts[count]);
        count++;
      }
    } else {  
      if (TTF_SizeUNICODE(pString16->font, pString16->text, &w, &h) < 0) {
	die("TTF_SizeUNICODE return ERROR !");
      }
    }
   
    if (!((pString16->style & 0x0F) & TTF_STYLE_NORMAL)) {
      TTF_SetFontStyle(pString16->font, TTF_STYLE_NORMAL);
    }
    
    Ret.w = w;
    Ret.h = h;
  } else {
    Ret.h = (pString16 ? TTF_FontHeight(pString16->font) : 0);
  }
  
  return Ret;
}

/**************************************************************************
  Create string16 struct with ptsize font.
  Font will be loaded or aliased with existing font of that size.
  pInTextString must be allocated in memory (MALLOC/fc_calloc)
**************************************************************************/
SDL_String16 * create_string16(Uint16 *pInTextString,
					size_t n_alloc, Uint16 ptsize)
{
  SDL_String16 *str = fc_calloc(1, sizeof(SDL_String16));

  if (!ptsize) {
    str->ptsize = theme_default_font_size(theme);
  } else {
    str->ptsize = ptsize;
  }
  
  if ((str->font = load_font(str->ptsize)) == NULL) {
    freelog(LOG_ERROR, "create_string16: load_font failed");
    FC_FREE(str);
    return str;
  }

  str->style = TTF_STYLE_NORMAL;
  str->bgcol = (SDL_Color) {0, 0, 0, 0};
  str->fgcol = *get_game_colorRGB(COLOR_THEME_TEXT);
  str->render = 2;

  /* pInTextString must be allocated in memory (MALLOC/fc_calloc) */
  str->text = pInTextString;
  str->n_alloc = n_alloc;
  
  return str;
}

/**************************************************************************
  ...
**************************************************************************/
SDL_String16 * copy_chars_to_string16(SDL_String16 *pString,
						const char *pCharString)
{
  size_t n;
  
  assert(pString != NULL);
  assert(pCharString != NULL);
  
  n = (strlen(pCharString) + 1) * 2;
  
  if (n > pString->n_alloc) {
    /* allocated more if this is only a small increase on before: */
    size_t n1 = (3 * pString->n_alloc) / 2;
    pString->n_alloc = (n > n1) ? n : n1;
    pString->text = fc_realloc(pString->text, pString->n_alloc);
  }
  
  convertcopy_to_utf16(pString->text, pString->n_alloc, pCharString);
  
  return pString;
}

/**************************************************************************
  ...
**************************************************************************/
int write_text16(SDL_Surface * pDest, Sint16 x, Sint16 y,
		 SDL_String16 * pString)
{
  SDL_Rect dst_rect = { x, y, 0, 0 };
  SDL_Surface *pText = create_text_surf_from_str16(pString);

  if (alphablit(pText, NULL, pDest, &dst_rect) < 0) {
    freelog(LOG_ERROR, "write_text16: couldn't blit text to display: %s",
	    SDL_GetError());
    FREESURFACE(pText);
    return -1;
  }

  FREESURFACE(pText);
  return 0;
}

/**************************************************************************
  Create Text Surface from SDL_String16
**************************************************************************/
static SDL_Surface *create_str16_surf(SDL_String16 * pString)
{
  SDL_Surface *pText = NULL; /* FIXME: possibly uninitialized */

  if (!pString) {
    return NULL;
  }

  if (!((pString->style & 0x0F) & TTF_STYLE_NORMAL)) {
    TTF_SetFontStyle(pString->font, (pString->style & 0x0F));
  }

  switch (pString->render) {
  case 0:
    pText = TTF_RenderUNICODE_Shaded(pString->font,
				     pString->text, pString->fgcol,
				     pString->bgcol);
    break;
  case 1:
  {
    SDL_Surface *pTmp = TTF_RenderUNICODE_Solid(pString->font,
				    pString->text, pString->fgcol);

    if ((pText = SDL_DisplayFormat(pTmp)) == NULL) {
      freelog(LOG_ERROR,
	      "SDL_create_str16_surf: couldn't convert text "
	      "to display format: %s", SDL_GetError());
      pText = pTmp;
    } else {
      FREESURFACE( pTmp );
    }
    
  }
  break;
  case 2:
    pText = TTF_RenderUNICODE_Blended(pString->font,
				      pString->text, pString->fgcol);
    break;
  }

  if (pText != NULL) {
    freelog(LOG_DEBUG,
            "SDL_create_str16_surf: Font is generally %d big, and "
              "string is %hd big", TTF_FontHeight(pString->font), pText->h);
    freelog(LOG_DEBUG, "SDL_create_str16_surf: String is %d length",
            pText->w);
  } else {
    freelog(LOG_DEBUG, "SDL_create_str16_surf: pText NULL");
    pText = create_surf_alpha(0, 0, SDL_SWSURFACE);
  }

  if (!((pString->style & 0x0F) & TTF_STYLE_NORMAL)) {
    TTF_SetFontStyle(pString->font, TTF_STYLE_NORMAL);
  }

  return pText;
}

/**************************************************************************
  ...
**************************************************************************/
static SDL_Surface *create_str16_multi_surf(SDL_String16 * pString)
{
  SDL_Rect des = {0, 0, 0, 0};
  SDL_Surface *pText = NULL, **pTmp = NULL;
  Uint16 i, w = 0, count = 0;
  Uint32 color;
  Uint16 *pBuf = pString->text;
  Uint16 **UniTexts = create_new_line_unistrings(pString->text);

  while (UniTexts[count]) {
    count++;
  }

  pTmp = fc_calloc(count, sizeof(SDL_Surface *));

  for (i = 0; i < count; i++) {
    pString->text = UniTexts[i];
    pTmp[i] = create_str16_surf(pString);
  }

  pString->text = pBuf;

  /* find max len */
  for (i = 0; i < count; i++) {
    if (pTmp[i]->w > w) {
      w = pTmp[i]->w;
    }
  }

  /* create and fill surface */
  
  color = pTmp[0]->format->colorkey;
  
  switch (pString->render) {
  case 1:
    pText = create_surf(w, count * pTmp[0]->h, SDL_SWSURFACE);
    SDL_FillRect(pText, NULL, color);
    SDL_SetColorKey(pText, SDL_SRCCOLORKEY, color);
    break;
  case 2:
      pText = create_surf_with_format(pTmp[0]->format,
				     w, count * pTmp[0]->h, pTmp[0]->flags);
      SDL_FillRect(pText, NULL, color);
    break;
  default:
    pText = create_surf(w, count * pTmp[0]->h, SDL_SWSURFACE);
    SDL_FillRect(pText, NULL, color);
    break;
  }

  /* blit (default: center left) */
  for (i = 0; i < count; i++) {
    if (pString->style & SF_CENTER) {
      des.x = (w - pTmp[i]->w) / 2;
    } else {
      if (pString->style & SF_CENTER_RIGHT) {
	des.x = w - pTmp[i]->w;
      } else {
	des.x = 0;
      }
    }

    alphablit(pTmp[i], NULL, pText, &des);
    des.y += pTmp[i]->h;
  }


  /* Free Memmory */
  for (i = 0; i < count; i++) {
    FC_FREE(UniTexts[i]);
    FREESURFACE(pTmp[i]);
  }
  
  FC_FREE(pTmp);

  return pText;
}

/**************************************************************************
  ...
**************************************************************************/
SDL_Surface * create_text_surf_from_str16(SDL_String16 *pString)
{
  if (pString && pString->text) {
    Uint16 *pStr16 = pString->text;
    Uint16 c = *pStr16;
    
    /* find '\n' */
    while (c != '\0') {
      if (c == 10) {
        return create_str16_multi_surf(pString);
      }
      pStr16++;
      c = *pStr16;
    }

    return create_str16_surf(pString);
  }
  
  return NULL;
}

#if 0
/**************************************************************************
  ...
**************************************************************************/
SDL_Surface * create_text_surf_smaller_that_w(SDL_String16 *pString, int w)
{
  assert(pString != NULL);

  SDL_Surface *pText = create_text_surf_from_str16(pString);
  
  if(pText && pText->w > w - 4) {
    /* cut string length to w length by replacing space " " with new line "\n" */
    int ptsize = pString->ptsize;
    Uint16 pNew_Line[2], pSpace[2];
    Uint16 *ptr = pString->text;
    
    convertcopy_to_utf16(pNew_Line, sizeof(pNew_Line), "\n");
    convertcopy_to_utf16(pSpace, sizeof(pSpace), " ");
   
    do {
      if (*ptr) {
	if(*ptr == *pSpace) {
          *ptr = *pNew_Line;/* "\n" */
	  FREESURFACE(pText);
          pText = create_text_surf_from_str16(pString);
	}
	ptr++;
      } else {
	FREESURFACE(pText);
        if (pString->ptsize > 8) {
	  change_ptsize16(pString, pString->ptsize - 1);
	  pText = create_text_surf_from_str16(pString);
	} else {
	  /* die */
          assert(pText != NULL);
	}
      }
    } while (pText->w > w - 4);    
  
    if(pString->ptsize != ptsize) {
      change_ptsize16(pString, ptsize);
    }    
  }
    
  return pText;
}
#endif

/**************************************************************************
  ...
**************************************************************************/
bool convert_string_to_const_surface_width(SDL_String16 *pString,
								int width)
{  
  int w;
  bool converted = FALSE;
  
  assert(pString != NULL);
  assert(pString->text != NULL);
  
  w = str16size(pString).w;
  if(w > width) {
    /* cut string length to w length by replacing space " " with new line "\n" */
    bool resize = FALSE;
    int len = 0, adv;
    Uint16 New_Line, Space;
    Uint16 *ptr_rev, *ptr = pString->text;
        
    {
      Uint16 pBuf[2];
      convertcopy_to_utf16(pBuf, sizeof(pBuf), "\n");
      New_Line = pBuf[0];
      convertcopy_to_utf16(pBuf, sizeof(pBuf), " ");
      Space = pBuf[0];
    }
    
    converted = TRUE;
    
    do {
      if (!resize) {
	
	if (*ptr == '\0') {
	  resize = TRUE;
	  continue;
	}
	
	if (*ptr == New_Line) {
	  len = 0;
	  ptr++;
	  continue;
	}
		
	if (!((pString->style & 0x0F) & TTF_STYLE_NORMAL)) {
    	  TTF_SetFontStyle(pString->font, (pString->style & 0x0F));
	}
	TTF_GlyphMetrics(pString->font, *ptr, NULL, NULL, NULL, NULL, &adv);
	if (!((pString->style & 0x0F) & TTF_STYLE_NORMAL)) {
    	  TTF_SetFontStyle(pString->font, TTF_STYLE_NORMAL);
	}
	
	len += adv;
	
	if (len > width) {
	  ptr_rev = ptr;
	  while(ptr_rev != pString->text) {
	    if(*ptr_rev == Space) {
              *ptr_rev = New_Line;/* "\n" */
              w = str16size(pString).w;
	      len = 0;
	      break;
	    }
	    if(*ptr_rev == New_Line) {
	      resize = TRUE;
	      break;
	    }
	    ptr_rev--;
	  }
	  if (ptr_rev == pString->text) {
	    resize = TRUE;
	  }
	}
	
	ptr++;
      } else {
        if (pString->ptsize > 8) {
	  change_ptsize16(pString, pString->ptsize - 1);
	  w = str16size(pString).w;
	} else {
	  freelog(LOG_ERROR, "Can't convert string to const width");
          break;
	}
      }  
      
    } while (w > width);
  }
    
  return converted;
}

/**************************************************************************
  ...
**************************************************************************/
SDL_Surface * create_text_surf_smaller_that_w(SDL_String16 *pString, int w)
{
  int ptsize;
  SDL_Surface *pText;
  
  assert(pString != NULL);
  
  ptsize = pString->ptsize;
  convert_string_to_const_surface_width(pString, w);
  pText = create_text_surf_from_str16(pString);
  if(pString->ptsize != ptsize) {
    change_ptsize16(pString, ptsize);
  }
  
  return pText;
}

/**************************************************************************
  ...
**************************************************************************/
void change_ptsize16(SDL_String16 *pString, Uint16 new_ptsize)
{
  TTF_Font *pBuf;

  if (pString->ptsize == new_ptsize) {
    return;
  }
  
  if ((pBuf = load_font(new_ptsize)) == NULL) {
    freelog(LOG_ERROR, "change_ptsize: load_font failed");
    return;
  }

  unload_font(pString->ptsize);
  pString->ptsize = new_ptsize;
  pString->font = pBuf;
}

/* =================================================== */

/**************************************************************************
  ...
**************************************************************************/
static TTF_Font * load_font(Uint16 ptsize)
{
  struct TTF_Font_Chain *Font_TAB_TMP = Font_TAB;
  TTF_Font *font_tmp = NULL;

  /* find existing font and return pointer to him */
  if (Sizeof_Font_TAB) {
    while (Font_TAB_TMP) {
      if (Font_TAB_TMP->ptsize == ptsize) {
	Font_TAB_TMP->count++;
	return Font_TAB_TMP->font;
      }
      Font_TAB_TMP = Font_TAB_TMP->next;
    }
  }

  if(!pFont_with_FullPath) {
    const char *path = theme_font_filename(theme);
    pFont_with_FullPath = mystrdup(path);
    assert(pFont_with_FullPath != NULL);
  }

  /* Load Font */
  if ((font_tmp = TTF_OpenFont(pFont_with_FullPath, ptsize)) == NULL) {
    freelog(LOG_ERROR,
	    "load_font: Couldn't load %d pt font from %s: %s",
	    ptsize, pFont_with_FullPath, SDL_GetError());
    return font_tmp;
  }
  
  /* add new font to list */
  if (Sizeof_Font_TAB == 0) {
    Sizeof_Font_TAB++;
    Font_TAB_TMP = fc_calloc(1, sizeof(struct TTF_Font_Chain));
    Font_TAB = Font_TAB_TMP;
  } else {
    /* Go to end of chain */
    Font_TAB_TMP = Font_TAB;
    while (Font_TAB_TMP->next)
      Font_TAB_TMP = Font_TAB_TMP->next;

    Sizeof_Font_TAB++;
    Font_TAB_TMP->next = fc_calloc(1, sizeof(struct TTF_Font_Chain));
    Font_TAB_TMP = Font_TAB_TMP->next;
  }

  Font_TAB_TMP->ptsize = ptsize;
  Font_TAB_TMP->count = 1;
  Font_TAB_TMP->font = font_tmp;
  Font_TAB_TMP->next = NULL;

  return font_tmp;
}

/**************************************************************************
  ...
**************************************************************************/
void unload_font(Uint16 ptsize)
{
  int index;

  struct TTF_Font_Chain *Font_TAB_PREV = NULL;
  struct TTF_Font_Chain *Font_TAB_TMP = Font_TAB;

  if (Sizeof_Font_TAB == 0) {
    freelog(LOG_ERROR,
	    "unload_font: Trying unload from empty Font ARRAY");
    return;
  }

  for (index = 0; index < Sizeof_Font_TAB; index++) {
    if (Font_TAB_TMP->ptsize == ptsize) {
      break;
    }
    Font_TAB_PREV = Font_TAB_TMP;
    Font_TAB_TMP = Font_TAB_TMP->next;
  }

  if (index == Sizeof_Font_TAB) {
    freelog(LOG_ERROR,
	    "unload_font: Trying unload Font which is "
	    "not included in Font ARRAY");
    return;
  }

  Font_TAB_TMP->count--;

  if (Font_TAB_TMP->count) {
    return;
  }

  TTF_CloseFont(Font_TAB_TMP->font);
  Font_TAB_TMP->font = NULL;
  if (Font_TAB_TMP == Font_TAB) {
    Font_TAB = Font_TAB_TMP->next;
  } else {
    Font_TAB_PREV->next = Font_TAB_TMP->next;
  }
  Font_TAB_TMP->next = NULL;
  Sizeof_Font_TAB--;
  FC_FREE(Font_TAB_TMP);
}

void free_font_system(void)
{
  struct TTF_Font_Chain *Font_TAB_TMP;
    
  FC_FREE(pFont_with_FullPath);
  while(Font_TAB) {
    if (Font_TAB->next) {
      Font_TAB_TMP = Font_TAB;
      Font_TAB = Font_TAB->next;
      if(Font_TAB_TMP->font) {
	TTF_CloseFont(Font_TAB_TMP->font);
      }
      FC_FREE(Font_TAB_TMP);
    } else {
      if(Font_TAB->font) {
	TTF_CloseFont(Font_TAB->font);
      }
      FC_FREE(Font_TAB);
    }
  }
  
}
