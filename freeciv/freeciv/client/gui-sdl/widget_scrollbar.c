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
#include "gui_id.h"
#include "gui_tilespec.h"
#include "mapview.h"
#include "themespec.h"

#include "widget.h"
#include "widget_p.h"

struct UP_DOWN {
  struct widget *pBegin;
  struct widget *pEnd;
  struct widget *pBeginWidgetLIST;
  struct widget *pEndWidgetLIST;
  struct ScrollBar *pVscroll;
  float old_y;
  int step;
  int prev_x;
  int prev_y;
  int offset; /* number of pixels the mouse is away from the slider origin */
};

#define UpperAdd(pNew_Widget, pAdd_Dock)	\
do {						\
  pNew_Widget->prev = pAdd_Dock;		\
  pNew_Widget->next = pAdd_Dock->next;		\
  if(pAdd_Dock->next) {			\
    pAdd_Dock->next->prev = pNew_Widget;	\
  }						\
  pAdd_Dock->next = pNew_Widget;		\
} while(0)

static int (*baseclass_redraw)(struct widget *pwidget);

/* =================================================== */
/* ===================== VSCROOLBAR ================== */
/* =================================================== */

/**************************************************************************
  Create background image for vscrollbars
  then return pointer to this image.

  Graphic is taken from pVert_theme surface and blit to new created image.

  hight depend of 'High' parametr.

  Type of image depend of "state" parametr.
    state = 0 - normal
    state = 1 - selected
    state = 2 - pressed
    state = 3 - disabled
**************************************************************************/
static SDL_Surface *create_vertical_surface(SDL_Surface * pVert_theme,
					    Uint8 state, Uint16 High)
{
  SDL_Surface *pVerSurf = NULL;
  SDL_Rect src, des;

  Uint16 i;
  Uint16 start_y;

  Uint16 tile_count_midd;
  Uint8 tile_len_end;
  Uint8 tile_len_midd;


  tile_len_end = pVert_theme->h / 16;

  start_y = 0 + state * (pVert_theme->h / 4);

  /* tile_len_midd = pVert_theme->h/4  - tile_len_end*2; */
  tile_len_midd = pVert_theme->h / 4 - tile_len_end * 2;

  tile_count_midd = (High - tile_len_end * 2) / tile_len_midd;

  /* correction */
  if (tile_len_midd * tile_count_midd + tile_len_end * 2 < High)
    tile_count_midd++;

  if (!tile_count_midd) {
    pVerSurf = create_surf_alpha(pVert_theme->w, tile_len_end * 2, SDL_SWSURFACE);
  } else {
    pVerSurf = create_surf_alpha(pVert_theme->w, High, SDL_SWSURFACE);
  }

  src.x = 0;
  src.y = start_y;
  src.w = pVert_theme->w;
  src.h = tile_len_end;
  alphablit(pVert_theme, &src, pVerSurf, NULL);

  src.y = start_y + tile_len_end;
  src.h = tile_len_midd;

  des.x = 0;

  for (i = 0; i < tile_count_midd; i++) {
    des.y = tile_len_end + i * tile_len_midd;
    alphablit(pVert_theme, &src, pVerSurf, &des);
  }

  src.y = start_y + tile_len_end + tile_len_midd;
  src.h = tile_len_end;
  des.y = pVerSurf->h - tile_len_end;
  alphablit(pVert_theme, &src, pVerSurf, &des);

  return pVerSurf;
}

/**************************************************************************
  ...
**************************************************************************/
static int redraw_vert(struct widget *pVert)
{
  int ret;
  SDL_Rect dest = pVert->size;
  SDL_Surface *pVert_Surf;

  ret = (*baseclass_redraw)(pVert);
  if (ret != 0) {
    return ret;
  }
  
  pVert_Surf = create_vertical_surface(pVert->theme,
				       get_wstate(pVert),
				       pVert->size.h);
  ret =
      blit_entire_src(pVert_Surf, pVert->dst->surface, dest.x, dest.y);
  
  FREESURFACE(pVert_Surf);

  return ret;
}

/**************************************************************************
  Create ( malloc ) VSrcrollBar Widget structure.

  Theme graphic is taken from pVert_theme surface;

  This function determinate future size of VScrollBar
  ( width = 'pVert_theme->w' , high = 'high' ) and
  save this in: pWidget->size rectangle ( SDL_Rect )

  function return pointer to allocated Widget.
**************************************************************************/
struct widget * create_vertical(SDL_Surface *pVert_theme, struct gui_layer *pDest,
  			Uint16 high, Uint32 flags)
{
  struct widget *pVer = widget_new();

  pVer->theme = pVert_theme;
  pVer->size.w = pVert_theme->w;
  pVer->size.h = high;
  set_wflag(pVer, (WF_FREE_STRING | WF_FREE_GFX | flags));
  set_wstate(pVer, FC_WS_DISABLED);
  set_wtype(pVer, WT_VSCROLLBAR);
  pVer->mod = KMOD_NONE;
  pVer->dst = pDest;

  baseclass_redraw = pVer->redraw;  
  pVer->redraw = redraw_vert;

  return pVer;
}

/**************************************************************************
  ...
**************************************************************************/
int draw_vert(struct widget *pVert, Sint16 x, Sint16 y)
{
  pVert->size.x = x;
  pVert->size.y = y;
  pVert->gfx = crop_rect_from_surface(pVert->dst->surface, &pVert->size);
  return redraw_vert(pVert);
}

/* =================================================== */
/* ===================== HSCROOLBAR ================== */
/* =================================================== */

/**************************************************************************
  Create background image for hscrollbars
  then return	pointer to this image.

  Graphic is taken from pHoriz_theme surface and blit to new created image.

  hight depend of 'Width' parametr.

  Type of image depend of "state" parametr.
    state = 0 - normal
    state = 1 - selected
    state = 2 - pressed
    state = 3 - disabled
**************************************************************************/
static SDL_Surface *create_horizontal_surface(SDL_Surface * pHoriz_theme,
					      Uint8 state, Uint16 Width)
{
  SDL_Surface *pHorSurf = NULL;
  SDL_Rect src, des;

  Uint16 i;
  Uint16 start_x;

  Uint16 tile_count_midd;
  Uint8 tile_len_end;
  Uint8 tile_len_midd;

  tile_len_end = pHoriz_theme->w / 16;

  start_x = 0 + state * (pHoriz_theme->w / 4);

  tile_len_midd = pHoriz_theme->w / 4 - tile_len_end * 2;

  tile_count_midd = (Width - tile_len_end * 2) / tile_len_midd;

  /* correction */
  if (tile_len_midd * tile_count_midd + tile_len_end * 2 < Width) {
    tile_count_midd++;
  }

  if (!tile_count_midd) {
    pHorSurf = create_surf_alpha(tile_len_end * 2, pHoriz_theme->h, SDL_SWSURFACE);
  } else {
    pHorSurf = create_surf_alpha(Width, pHoriz_theme->h, SDL_SWSURFACE);
  }

  src.y = 0;
  src.x = start_x;
  src.h = pHoriz_theme->h;
  src.w = tile_len_end;
  alphablit(pHoriz_theme, &src, pHorSurf, NULL);

  src.x = start_x + tile_len_end;
  src.w = tile_len_midd;

  des.y = 0;

  for (i = 0; i < tile_count_midd; i++) {
    des.x = tile_len_end + i * tile_len_midd;
    alphablit(pHoriz_theme, &src, pHorSurf, &des);
  }

  src.x = start_x + tile_len_end + tile_len_midd;
  src.w = tile_len_end;
  des.x = pHorSurf->w - tile_len_end;
  alphablit(pHoriz_theme, &src, pHorSurf, &des);

  return pHorSurf;
}

/**************************************************************************
  ...
**************************************************************************/
static int redraw_horiz(struct widget *pHoriz)
{
  int ret;
  SDL_Rect dest = pHoriz->size;
  SDL_Surface *pHoriz_Surf;

  ret = (*baseclass_redraw)(pHoriz);
  if (ret != 0) {
    return ret;
  }
  
  pHoriz_Surf = create_horizontal_surface(pHoriz->theme,
					  get_wstate(pHoriz),
					  pHoriz->size.w);
  ret = blit_entire_src(pHoriz_Surf, pHoriz->dst->surface, dest.x, dest.y);
  
  FREESURFACE(pHoriz_Surf);

  return ret;
}

/**************************************************************************
  Create ( malloc ) VSrcrollBar Widget structure.

  Theme graphic is taken from pVert_theme surface;

  This function determinate future size of VScrollBar
  ( width = 'pVert_theme->w' , high = 'high' ) and
  save this in: pWidget->size rectangle ( SDL_Rect )

  function return pointer to allocated Widget.
**************************************************************************/
struct widget * create_horizontal(SDL_Surface *pHoriz_theme, struct gui_layer *pDest,
  		Uint16 width, Uint32 flags)
{
  struct widget *pHor = widget_new();

  pHor->theme = pHoriz_theme;
  pHor->size.w = width;
  pHor->size.h = pHoriz_theme->h;
  set_wflag(pHor, WF_FREE_STRING | flags);
  set_wstate(pHor, FC_WS_DISABLED);
  set_wtype(pHor, WT_HSCROLLBAR);
  pHor->mod = KMOD_NONE;
  pHor->dst = pDest;

  baseclass_redraw = pHor->redraw;
  pHor->redraw = redraw_horiz;
  
  return pHor;
}

/**************************************************************************
  ...
**************************************************************************/
int draw_horiz(struct widget *pHoriz, Sint16 x, Sint16 y)
{
  pHoriz->size.x = x;
  pHoriz->size.y = y;
  pHoriz->gfx = crop_rect_from_surface(pHoriz->dst->surface, &pHoriz->size);
  return redraw_horiz(pHoriz);
}

/* =================================================== */
/* =====================            ================== */
/* =================================================== */

/**************************************************************************
  ...
**************************************************************************/
static int get_step(struct ScrollBar *pScroll)
{
  float step = pScroll->max - pScroll->min;
  step *= (float) (1.0 - (float) (pScroll->active * pScroll->step) /
						  (float)pScroll->count);
  step /= (float)(pScroll->count - pScroll->active * pScroll->step);
  step *= (float)pScroll->step;
  step++;
  return (int)step;
}

/**************************************************************************
  ...
**************************************************************************/
static int get_position(struct ADVANCED_DLG *pDlg)
{
  struct widget *pBuf = pDlg->pActiveWidgetList;
  int count = pDlg->pScroll->active * pDlg->pScroll->step - 1;
  int step = get_step(pDlg->pScroll);
  
  /* find last seen widget */
  while(count) {
    if(pBuf == pDlg->pBeginActiveWidgetList) {
      break;
    }
    count--;
    pBuf = pBuf->prev;
  }
  
  count = 0;
  if(pBuf != pDlg->pBeginActiveWidgetList) {
    do {
      count++;
      pBuf = pBuf->prev;
    } while (pBuf != pDlg->pBeginActiveWidgetList);
  }
 
  if (pDlg->pScroll->pScrollBar) {
  return pDlg->pScroll->max - pDlg->pScroll->pScrollBar->size.h -
					count * (float)step / pDlg->pScroll->step;
  } else {
    return pDlg->pScroll->max - count * (float)step / pDlg->pScroll->step;
  }
}


/**************************************************************************
  			Vertical ScrollBar
**************************************************************************/

static struct widget *up_scroll_widget_list(struct ScrollBar *pVscroll,
				  struct widget *pBeginActiveWidgetLIST,
				  struct widget *pBeginWidgetLIST,
				  struct widget *pEndWidgetLIST);

static struct widget *down_scroll_widget_list(struct ScrollBar *pVscroll,
				    struct widget *pBeginActiveWidgetLIST,
				    struct widget *pBeginWidgetLIST,
				    struct widget *pEndWidgetLIST);

static struct widget *vertic_scroll_widget_list(struct ScrollBar *pVscroll,
				      struct widget *pBeginActiveWidgetLIST,
				      struct widget *pBeginWidgetLIST,
				      struct widget *pEndWidgetLIST);
                                        
/**************************************************************************
  ...
**************************************************************************/
static int std_up_advanced_dlg_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct ADVANCED_DLG *pDlg = pWidget->private_data.adv_dlg;
    struct widget *pBegin = up_scroll_widget_list(
                          pDlg->pScroll,
                          pDlg->pActiveWidgetList,
                          pDlg->pBeginActiveWidgetList,
                          pDlg->pEndActiveWidgetList);
  
    if (pBegin) {
      pDlg->pActiveWidgetList = pBegin;
    }
    
    unsellect_widget_action();
    pSellected_Widget = pWidget;
    set_wstate(pWidget, FC_WS_SELLECTED);
    widget_redraw(pWidget);
    widget_flush(pWidget);
  }
  return -1;
}

/**************************************************************************
  ...
**************************************************************************/
static int std_down_advanced_dlg_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct ADVANCED_DLG *pDlg = pWidget->private_data.adv_dlg;
    struct widget *pBegin = down_scroll_widget_list(
                              pDlg->pScroll,
                              pDlg->pActiveWidgetList,
                              pDlg->pBeginActiveWidgetList,
                              pDlg->pEndActiveWidgetList);
  
    if (pBegin) {
      pDlg->pActiveWidgetList = pBegin;
    }
  
    unsellect_widget_action();
    pSellected_Widget = pWidget;
    set_wstate(pWidget, FC_WS_SELLECTED);
    widget_redraw(pWidget);
    widget_flush(pWidget);
  }
  return -1;
}

/**************************************************************************
  FIXME : fix main funct : vertic_scroll_widget_list(...)
**************************************************************************/
static int std_vscroll_advanced_dlg_callback(struct widget *pScrollBar)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct ADVANCED_DLG *pDlg = pScrollBar->private_data.adv_dlg;
    struct widget *pBegin = vertic_scroll_widget_list(
                              pDlg->pScroll,
                              pDlg->pActiveWidgetList,
                              pDlg->pBeginActiveWidgetList,
                              pDlg->pEndActiveWidgetList);
  
    if (pBegin) {
      pDlg->pActiveWidgetList = pBegin;
    }
    unsellect_widget_action();
    set_wstate(pScrollBar, FC_WS_SELLECTED);
    pSellected_Widget = pScrollBar;
    redraw_vert(pScrollBar);
    widget_flush(pScrollBar);
  }
  return -1;
}


/**************************************************************************
  ...
**************************************************************************/
Uint32 create_vertical_scrollbar(struct ADVANCED_DLG *pDlg,
                                 Uint8 step, Uint8 active,
                                 bool create_scrollbar, bool create_buttons)
{
  Uint16 count = 0;
  struct widget *pBuf = NULL, *pWindow = NULL;
    
  assert(pDlg != NULL);
  
  pWindow = pDlg->pEndWidgetList;
  
  if (!pDlg->pScroll) {
    pDlg->pScroll = fc_calloc(1, sizeof(struct ScrollBar));
        
    pBuf = pDlg->pEndActiveWidgetList;
    while (pBuf && (pBuf != pDlg->pBeginActiveWidgetList->prev)) {
      pBuf = pBuf->prev;
      count++;
    }
  
    pDlg->pScroll->count = count;
  }
  
  pDlg->pScroll->active = active;
  pDlg->pScroll->step = step;
  
  if(create_buttons) {
    /* create up button */
    pBuf = create_themeicon_button(pTheme->UP_Icon, pWindow->dst, NULL, WF_RESTORE_BACKGROUND);
    
    pBuf->ID = ID_BUTTON;
    pBuf->private_data.adv_dlg = pDlg;
    pBuf->action = std_up_advanced_dlg_callback;
    set_wstate(pBuf, FC_WS_NORMAL);
    
    pDlg->pScroll->pUp_Left_Button = pBuf;
    DownAdd(pBuf, pDlg->pBeginWidgetList);
    pDlg->pBeginWidgetList = pBuf;
    
    count = pBuf->size.w;
    
    /* create down button */
    pBuf = create_themeicon_button(pTheme->DOWN_Icon, pWindow->dst, NULL, WF_RESTORE_BACKGROUND);
    
    pBuf->ID = ID_BUTTON;
    pBuf->private_data.adv_dlg = pDlg;
    pBuf->action = std_down_advanced_dlg_callback;
    set_wstate(pBuf, FC_WS_NORMAL);
    
    pDlg->pScroll->pDown_Right_Button = pBuf;
    DownAdd(pBuf, pDlg->pBeginWidgetList);
    pDlg->pBeginWidgetList = pBuf;
    
  }
  
  if(create_scrollbar) {
    /* create vsrollbar */
    pBuf = create_vertical(pTheme->Vertic, pWindow->dst,
				adj_size(10), WF_RESTORE_BACKGROUND);
    
    pBuf->ID = ID_SCROLLBAR;
    pBuf->private_data.adv_dlg = pDlg;
    pBuf->action = std_vscroll_advanced_dlg_callback;
    set_wstate(pBuf, FC_WS_NORMAL);
  
    pDlg->pScroll->pScrollBar = pBuf;
    DownAdd(pBuf, pDlg->pBeginWidgetList);
    pDlg->pBeginWidgetList = pBuf;
    
    if(!count) {
      count = pBuf->size.w;
    }
  }
  
  return count;
}

/**************************************************************************
  ...
**************************************************************************/
void setup_vertical_scrollbar_area(struct ScrollBar *pScroll,
	Sint16 start_x, Sint16 start_y, Uint16 hight, bool swap_start_x)
{
  bool buttons_exist;
  
  assert(pScroll != NULL);
  
  buttons_exist = (pScroll->pDown_Right_Button && pScroll->pUp_Left_Button);
  
  if(buttons_exist) {
    /* up */
    pScroll->pUp_Left_Button->size.y = start_y;
    if(swap_start_x) {
      pScroll->pUp_Left_Button->size.x = start_x - 
    					pScroll->pUp_Left_Button->size.w;
    } else {
      pScroll->pUp_Left_Button->size.x = start_x;
    }
    pScroll->min = start_y + pScroll->pUp_Left_Button->size.h;
    /* -------------------------- */
    /* down */
    pScroll->pDown_Right_Button->size.y = start_y + hight -
				  pScroll->pDown_Right_Button->size.h;
    if(swap_start_x) {
      pScroll->pDown_Right_Button->size.x = start_x -
				pScroll->pDown_Right_Button->size.w;
    } else {
      pScroll->pDown_Right_Button->size.x = start_x;
    }
    pScroll->max = pScroll->pDown_Right_Button->size.y;
  }
  /* --------------- */
  /* scrollbar */
  if (pScroll->pScrollBar) {
    
    if(swap_start_x) {
      pScroll->pScrollBar->size.x = start_x - pScroll->pScrollBar->size.w + 2;
    } else {
      pScroll->pScrollBar->size.x = start_x;
    } 

    if(buttons_exist) {
      pScroll->pScrollBar->size.y = start_y +
				      pScroll->pUp_Left_Button->size.h;
      if(pScroll->count > pScroll->active * pScroll->step) {
	pScroll->pScrollBar->size.h = scrollbar_size(pScroll);
      } else {
	pScroll->pScrollBar->size.h = pScroll->max - pScroll->min;
      }
    } else {
      pScroll->pScrollBar->size.y = start_y;
      pScroll->pScrollBar->size.h = hight;
      pScroll->min = start_y;
      pScroll->max = start_y + hight;
    }
  }
}

/* =================================================== */
/* ============ Vertical Scroll Group List =========== */
/* =================================================== */

/**************************************************************************
  scroll pointers on list.
  dir == directions: up == -1, down == 1.
**************************************************************************/
static struct widget *vertical_scroll_widget_list(struct widget *pActiveWidgetLIST,
				      struct widget *pBeginWidgetLIST,
				      struct widget *pEndWidgetLIST,
				      int active, int step, int dir)
{
  struct widget *pBegin = pActiveWidgetLIST;
  struct widget *pBuf = pActiveWidgetLIST;
  struct widget *pTmp = NULL;  
  int count = active; /* row */
  int count_step = step; /* col */
    
  if (dir < 0) {
    /* up */
    bool real = TRUE;
    if (pBuf != pEndWidgetLIST) {
      /*
       move pointers to positions and unhidde scrolled widgets
       B = pBuf - new top
       T = pTmp - current top == pActiveWidgetLIST
       [B] [ ] [ ]
       -----------
       [T] [ ] [ ]
       [ ] [ ] [ ]
       -----------
       [ ] [ ] [ ]
    */
      pTmp = pBuf; /* now pBuf == pActiveWidgetLIST == current Top */
      while (count_step > 0) {
      	pBuf = pBuf->next;
	clear_wflag(pBuf, WF_HIDDEN);
	count_step--;
      }
      count_step = step;
      
      /* setup new ActiveWidget pointer */
      pBegin = pBuf;
      
      /*
       scroll pointers up
       B = pBuf
       T = pTmp
       [B0] [B1] [B2]
       -----------
       [T0] [T1] [T2]   => B position = T position
       [T3] [T4] [T5]
       -----------
       [  ] [  ] [  ]
      
       start from B0 and go downd list
       B0 = T0, B1 = T1, B2 = T2
       T0 = T3, T1 = T4, T2 = T5
       etc...
    */

      /* pBuf == pBegin == new top widget */

      while (count > 0) {
	if(real) {
	  pBuf->size.x = pTmp->size.x;
	  pBuf->size.y = pTmp->size.y;
          pBuf->gfx = pTmp->gfx;
          
          if ((pBuf->size.w != pTmp->size.w) || (pBuf->size.h != pTmp->size.h)) {
            widget_undraw(pTmp);
            widget_mark_dirty(pTmp);
            if (get_wflags(pBuf) & WF_RESTORE_BACKGROUND) {
              refresh_widget_background(pBuf);
            }
          }
          
	  pTmp->gfx = NULL;
          
	  if(count == 1) {
	    set_wflag(pTmp, WF_HIDDEN);
	  }
	  if(pTmp == pBeginWidgetLIST) {
	    real = FALSE;
	  }
	  pTmp = pTmp->prev;
	} else {
	  /*
	     unsymetric list support.
	     This is big problem becouse we can't take position from no exist
	     list memeber. We must put here some hypotetical positions
	  
	     [B0] [B1] [B2]
             --------------
             [T0] [T1]
	  
	  */
	  if (active > 1) {
	    /* this work good if active > 1 but is buggy when active == 1 */
	    pBuf->size.y += pBuf->size.h;
	  } else {
	    /* this work good if active == 1 but may be broken if "next"
	       element have another "y" position */
	    pBuf->size.y = pBuf->next->size.y;
	  }
	  pBuf->gfx = NULL;
	}
	
	pBuf = pBuf->prev;
	count_step--;
	if(!count_step) {
	  count_step = step;
	  count--;
	}
      }
      
    }
  } else {
    /* down */
    count = active * step; /* row * col */
    
    /*
       find end
       B = pBuf
       A - start (pBuf == pActiveWidgetLIST)
       [ ] [ ] [ ]
       -----------
       [A] [ ] [ ]
       [ ] [ ] [ ]
       -----------
       [B] [ ] [ ]
    */
    while (count && pBuf != pBeginWidgetLIST->prev) {
      pBuf = pBuf->prev;
      count--;
    }

    if (!count && pBuf != pBeginWidgetLIST->prev) {
      /*
       move pointers to positions and unhidde scrolled widgets
       B = pBuf
       T = pTmp
       A - start (pActiveWidgetLIST)
       [ ] [ ] [ ]
       -----------
       [A] [ ] [ ]
       [ ] [ ] [T]
       -----------
       [ ] [ ] [B]
    */
      pTmp = pBuf->next;
      count_step = step - 1;
      while(count_step && pBuf != pBeginWidgetLIST) {
	clear_wflag(pBuf, WF_HIDDEN);
	pBuf = pBuf->prev;
	count_step--;
      }
      clear_wflag(pBuf, WF_HIDDEN);

      /*
       Unsymetric list support.
       correct pTmp and undraw empty fields
       B = pBuf
       T = pTmp
       A - start (pActiveWidgetLIST)
       [ ] [ ] [ ]
       -----------
       [A] [ ] [ ]
       [ ] [T] [U]  <- undraw U
       -----------
       [ ] [B]
    */
      count = count_step;
      while(count) {
	/* hack - clear area under no exist list members */
        widget_undraw(pTmp);
	widget_mark_dirty(pTmp);
	FREESURFACE(pTmp->gfx);
	if (active == 1) {
	  set_wflag(pTmp, WF_HIDDEN);
	}
	pTmp = pTmp->next;
	count--;
      }
      
      /* reset counters */
      count = active;
      if(count_step) {
        count_step = step - count_step;
      } else {
	count_step = step;
      }
      
      /*
       scroll pointers down
       B = pBuf
       T = pTmp
       [  ] [  ] [  ]
       -----------
       [  ] [  ] [  ]
       [T2] [T1] [T0]   => B position = T position
       -----------
       [B2] [B1] [B0]
    */
      while (count) {
	pBuf->size.x = pTmp->size.x;
	pBuf->size.y = pTmp->size.y;
	pBuf->gfx = pTmp->gfx;
        
        if ((pBuf->size.w != pTmp->size.w) || (pBuf->size.h != pTmp->size.h)) {
          widget_undraw(pTmp);
          widget_mark_dirty(pTmp);
          if (get_wflags(pBuf) & WF_RESTORE_BACKGROUND) {
            refresh_widget_background(pBuf);
          }
        }
        
        pTmp->gfx = NULL;
        
	if(count == 1) {
	  set_wflag(pTmp, WF_HIDDEN);
	}
        
	pTmp = pTmp->next;
	pBuf = pBuf->next;
	count_step--;
	if(!count_step) {
	  count_step = step;
	  count--;
	}
      }
      /* setup new ActiveWidget pointer */
      pBegin = pBuf->prev;
    }
  }

  return pBegin;
}

/**************************************************************************
  ...
**************************************************************************/
static void inside_scroll_down_loop(void *pData)
{
  struct UP_DOWN *pDown = (struct UP_DOWN *)pData;
  
  if (pDown->pEnd != pDown->pBeginWidgetLIST) {
      if (pDown->pVscroll->pScrollBar
	&& pDown->pVscroll->pScrollBar->size.y <=
	  pDown->pVscroll->max - pDown->pVscroll->pScrollBar->size.h) {
            
	/* draw bcgd */
        widget_undraw(pDown->pVscroll->pScrollBar);
	widget_mark_dirty(pDown->pVscroll->pScrollBar);

	if (pDown->pVscroll->pScrollBar->size.y + pDown->step >
	    pDown->pVscroll->max - pDown->pVscroll->pScrollBar->size.h) {
	  pDown->pVscroll->pScrollBar->size.y =
	      pDown->pVscroll->max - pDown->pVscroll->pScrollBar->size.h;
	} else {
	  pDown->pVscroll->pScrollBar->size.y += pDown->step;
	}
      }

      pDown->pBegin = vertical_scroll_widget_list(pDown->pBegin,
			  pDown->pBeginWidgetLIST, pDown->pEndWidgetLIST,
			  pDown->pVscroll->active, pDown->pVscroll->step, 1);

      pDown->pEnd = pDown->pEnd->prev;

      redraw_group(pDown->pBeginWidgetLIST, pDown->pEndWidgetLIST, TRUE);

      if (pDown->pVscroll->pScrollBar) {
	/* redraw scrollbar */
        if (get_wflags(pDown->pVscroll->pScrollBar) & WF_RESTORE_BACKGROUND) {
	  refresh_widget_background(pDown->pVscroll->pScrollBar);
        }
	redraw_vert(pDown->pVscroll->pScrollBar);

	widget_mark_dirty(pDown->pVscroll->pScrollBar);
      }

      flush_dirty();
    }
}

/**************************************************************************
  ...
**************************************************************************/
static void inside_scroll_up_loop(void *pData)
{
  struct UP_DOWN *pUp = (struct UP_DOWN *)pData;
  
  if (pUp && pUp->pBegin != pUp->pEndWidgetLIST) {

    if (pUp->pVscroll->pScrollBar
      && (pUp->pVscroll->pScrollBar->size.y >= pUp->pVscroll->min)) {

      /* draw bcgd */
      widget_undraw(pUp->pVscroll->pScrollBar);
      widget_mark_dirty(pUp->pVscroll->pScrollBar);

      if (((pUp->pVscroll->pScrollBar->size.y - pUp->step) < pUp->pVscroll->min)) {
	pUp->pVscroll->pScrollBar->size.y = pUp->pVscroll->min;
      } else {
	pUp->pVscroll->pScrollBar->size.y -= pUp->step;
      }
    }

    pUp->pBegin = vertical_scroll_widget_list(pUp->pBegin,
			pUp->pBeginWidgetLIST, pUp->pEndWidgetLIST,
			pUp->pVscroll->active, pUp->pVscroll->step, -1);

    redraw_group(pUp->pBeginWidgetLIST, pUp->pEndWidgetLIST, TRUE);

    if (pUp->pVscroll->pScrollBar) {
      /* redraw scroolbar */
      if (get_wflags(pUp->pVscroll->pScrollBar) & WF_RESTORE_BACKGROUND) {
        refresh_widget_background(pUp->pVscroll->pScrollBar);
      }
      redraw_vert(pUp->pVscroll->pScrollBar);
      widget_mark_dirty(pUp->pVscroll->pScrollBar);
    }

    flush_dirty();
  }
}

/**************************************************************************
  FIXME
**************************************************************************/
static Uint16 scroll_mouse_motion_handler(SDL_MouseMotionEvent *pMotionEvent, void *pData)
{
  struct UP_DOWN *pMotion = (struct UP_DOWN *)pData;
  int xrel, yrel;
  int x, y;
  int normalized_y;
  int net_slider_area;
  int net_count;
  float scroll_step;
  
  xrel = pMotionEvent->x - pMotion->prev_x;
  yrel = pMotionEvent->y - pMotion->prev_y;
  pMotion->prev_x = pMotionEvent->x;
  pMotion->prev_y = pMotionEvent->y;
  
  x = pMotionEvent->x - pMotion->pVscroll->pScrollBar->dst->dest_rect.x;
  y = pMotionEvent->y - pMotion->pVscroll->pScrollBar->dst->dest_rect.y;

  normalized_y = (y - pMotion->offset);
  
  net_slider_area = (pMotion->pVscroll->max - pMotion->pVscroll->min - pMotion->pVscroll->pScrollBar->size.h);
  net_count = round((float)pMotion->pVscroll->count / pMotion->pVscroll->step) - pMotion->pVscroll->active + 1;
  scroll_step = (float)net_slider_area / net_count;
  
  if ((yrel != 0) &&
     ((normalized_y >= pMotion->pVscroll->min) || 
      ((normalized_y < pMotion->pVscroll->min) && (pMotion->pVscroll->pScrollBar->size.y > pMotion->pVscroll->min))) &&
     ((normalized_y <= pMotion->pVscroll->max - pMotion->pVscroll->pScrollBar->size.h) ||
      ((normalized_y > pMotion->pVscroll->max) && (pMotion->pVscroll->pScrollBar->size.y < (pMotion->pVscroll->max - pMotion->pVscroll->pScrollBar->size.h)))) ) {
  
    int count;
      
    /* draw bcgd */
    widget_undraw(pMotion->pVscroll->pScrollBar);
    widget_mark_dirty(pMotion->pVscroll->pScrollBar);

    if ((pMotion->pVscroll->pScrollBar->size.y + yrel) >
	 (pMotion->pVscroll->max - pMotion->pVscroll->pScrollBar->size.h)) {
           
      pMotion->pVscroll->pScrollBar->size.y =
        (pMotion->pVscroll->max - pMotion->pVscroll->pScrollBar->size.h);
           
    } else if ((pMotion->pVscroll->pScrollBar->size.y + yrel) < pMotion->pVscroll->min) {
      
	pMotion->pVscroll->pScrollBar->size.y = pMotion->pVscroll->min;
      
    } else {
      
	pMotion->pVscroll->pScrollBar->size.y += yrel;
      
    }
    
    count = round((pMotion->pVscroll->pScrollBar->size.y - pMotion->old_y) / scroll_step);

    if (count != 0) {
      
      int i = count;
      while (i != 0) {
        
	pMotion->pBegin = vertical_scroll_widget_list(pMotion->pBegin,
			pMotion->pBeginWidgetLIST, pMotion->pEndWidgetLIST,
				pMotion->pVscroll->active,
				pMotion->pVscroll->step, i);
	if (i > 0) {
	  i--;
	} else {
	  i++;
	}

      }	/* while (i != 0) */

      pMotion->old_y = pMotion->pVscroll->min + 
        ((round((pMotion->old_y - pMotion->pVscroll->min) / scroll_step) + count) * scroll_step);

      redraw_group(pMotion->pBeginWidgetLIST, pMotion->pEndWidgetLIST, TRUE);
    }

    /* redraw slider */
    if (get_wflags(pMotion->pVscroll->pScrollBar) & WF_RESTORE_BACKGROUND) {
      refresh_widget_background(pMotion->pVscroll->pScrollBar);
    }
    redraw_vert(pMotion->pVscroll->pScrollBar);
    widget_mark_dirty(pMotion->pVscroll->pScrollBar);

    flush_dirty();
  }

  return ID_ERROR;
}

/**************************************************************************
  ...
**************************************************************************/
static Uint16 scroll_mouse_button_up(SDL_MouseButtonEvent *pButtonEvent, void *pData)
{
  return (Uint16)ID_SCROLLBAR;
}

/**************************************************************************
  ...
**************************************************************************/
static struct widget *down_scroll_widget_list(struct ScrollBar *pVscroll,
				    struct widget *pBeginActiveWidgetLIST,
				    struct widget *pBeginWidgetLIST,
				    struct widget *pEndWidgetLIST)
{
  struct UP_DOWN pDown;
  struct widget *pBegin = pBeginActiveWidgetLIST;
  int step = pVscroll->active * pVscroll->step - 1;

  while (step--) {
    pBegin = pBegin->prev;
  }

  pDown.step = get_step(pVscroll);
  pDown.pBegin = pBeginActiveWidgetLIST;
  pDown.pEnd = pBegin;
  pDown.pBeginWidgetLIST = pBeginWidgetLIST;
  pDown.pEndWidgetLIST = pEndWidgetLIST;
  pDown.pVscroll = pVscroll;
  
  gui_event_loop((void *)&pDown, inside_scroll_down_loop,
	NULL, NULL, NULL, scroll_mouse_button_up, NULL);
  
  return pDown.pBegin;
}

/**************************************************************************
  ...
**************************************************************************/
static struct widget *up_scroll_widget_list(struct ScrollBar *pVscroll,
				  struct widget *pBeginActiveWidgetLIST,
				  struct widget *pBeginWidgetLIST,
				  struct widget *pEndWidgetLIST)
{
  struct UP_DOWN pUp;
      
  pUp.step = get_step(pVscroll);
  pUp.pBegin = pBeginActiveWidgetLIST;
  pUp.pBeginWidgetLIST = pBeginWidgetLIST;
  pUp.pEndWidgetLIST = pEndWidgetLIST;
  pUp.pVscroll = pVscroll;
  
  gui_event_loop((void *)&pUp, inside_scroll_up_loop,
	NULL, NULL, NULL, scroll_mouse_button_up, NULL);
  
  return pUp.pBegin;
}

/**************************************************************************
  FIXME
**************************************************************************/
static struct widget *vertic_scroll_widget_list(struct ScrollBar *pVscroll,
				      struct widget *pBeginActiveWidgetLIST,
				      struct widget *pBeginWidgetLIST,
				      struct widget *pEndWidgetLIST)
{
  struct UP_DOWN pMotion;
      
  pMotion.step = get_step(pVscroll);
  pMotion.pBegin = pBeginActiveWidgetLIST;
  pMotion.pBeginWidgetLIST = pBeginWidgetLIST;
  pMotion.pEndWidgetLIST = pEndWidgetLIST;
  pMotion.pVscroll = pVscroll;
  pMotion.old_y = pVscroll->pScrollBar->size.y;
  SDL_GetMouseState(&pMotion.prev_x, &pMotion.prev_y);
  pMotion.offset = pMotion.prev_y - pVscroll->pScrollBar->dst->dest_rect.y - pVscroll->pScrollBar->size.y;
  
  MOVE_STEP_X = 0;
  MOVE_STEP_Y = 3;
  /* Filter mouse motion events */
  SDL_SetEventFilter(FilterMouseMotionEvents);
  gui_event_loop((void *)&pMotion, NULL, NULL, NULL, NULL,
		  scroll_mouse_button_up, scroll_mouse_motion_handler);
  /* Turn off Filter mouse motion events */
  SDL_SetEventFilter(NULL);
  MOVE_STEP_X = DEFAULT_MOVE_STEP;
  MOVE_STEP_Y = DEFAULT_MOVE_STEP;
  
  return pMotion.pBegin;
}

/* ==================================================================== */

/**************************************************************************
  Add new widget to srolled list and set draw position of all changed widgets.
  dir :
    TRUE - upper add => pAdd_Dock->next = pNew_Widget.
    FALSE - down add => pAdd_Dock->prev = pNew_Widget.
  start_x, start_y - positions of first seen widget (pActiveWidgetList).
  pDlg->pScroll ( scrollbar ) must exist.
  It isn't full secure to multi widget list.
**************************************************************************/
bool add_widget_to_vertical_scroll_widget_list(struct ADVANCED_DLG *pDlg,
                                               struct widget *pNew_Widget,
                                               struct widget *pAdd_Dock,
                                               bool dir,
                                               Sint16 start_x, Sint16 start_y)
{
  struct widget *pBuf = NULL;
  struct widget *pEnd = NULL, *pOld_End = NULL;
  int count = 0;
  bool last = FALSE, seen = TRUE;
  
  assert(pNew_Widget != NULL);
  assert(pDlg != NULL);
  assert(pDlg->pScroll != NULL);
  
  if (!pAdd_Dock) {
    pAdd_Dock = pDlg->pBeginWidgetList; /* last item */
  }
  
  pDlg->pScroll->count++;
  
  if (pDlg->pScroll->count > (pDlg->pScroll->active * pDlg->pScroll->step)) {
    /* -> scrollbar needed */
    
    if (pDlg->pActiveWidgetList) {
      /* -> scrollbar is already visible */
      
      int i = 0;
      /* find last active widget */
      pOld_End = pAdd_Dock;
      while(pOld_End != pDlg->pActiveWidgetList) {
        pOld_End = pOld_End->next;
	i++;
	if (pOld_End == pDlg->pEndActiveWidgetList) {
          /* implies (pOld_End == pDlg->pActiveWidgetList)? */
	  seen = FALSE;
	  break;
	}
      }
      
      if (seen) {
        count = (pDlg->pScroll->active * pDlg->pScroll->step) - 1;
        if (i > count) {
	  seen = FALSE;
        } else {
          while (count > 0) {
	    pOld_End = pOld_End->prev;
	    count--;
          }
          if (pOld_End == pAdd_Dock) {
	    last = TRUE;
          }
	}
      }
      
    } else {
      last = TRUE;
      pDlg->pActiveWidgetList = pDlg->pEndActiveWidgetList;
      show_scrollbar(pDlg->pScroll);
    }
  }

  count = 0;
  
  /* add Pointer to list */
  if (dir) {
    /* upper add */
    UpperAdd(pNew_Widget, pAdd_Dock);
    
    if(pAdd_Dock == pDlg->pEndWidgetList) {
      pDlg->pEndWidgetList = pNew_Widget;
    }
    if(pAdd_Dock == pDlg->pEndActiveWidgetList) {
      pDlg->pEndActiveWidgetList = pNew_Widget;
    }
    if(pAdd_Dock == pDlg->pActiveWidgetList) {
      pDlg->pActiveWidgetList = pNew_Widget;
    }
  } else {
    /* down add */
    DownAdd(pNew_Widget, pAdd_Dock);
        
    if(pAdd_Dock == pDlg->pBeginWidgetList) {
      pDlg->pBeginWidgetList = pNew_Widget;
    }
    
    if(pAdd_Dock == pDlg->pBeginActiveWidgetList) {
      pDlg->pBeginActiveWidgetList = pNew_Widget;
    }
  }
  
  /* setup draw positions */
  if (seen) {
    if(!pDlg->pBeginActiveWidgetList) {
      /* first element ( active list empty ) */
      if(dir) {
	die("Forbided List Operation");
      }
      pNew_Widget->size.x = start_x;
      pNew_Widget->size.y = start_y;
      pDlg->pBeginActiveWidgetList = pNew_Widget;
      pDlg->pEndActiveWidgetList = pNew_Widget;
      if(!pDlg->pBeginWidgetList) {
        pDlg->pBeginWidgetList = pNew_Widget;
        pDlg->pEndWidgetList = pNew_Widget;
      }
    } else { /* there are some elements on local active list */
      if(last) {
        /* We add to last seen position */
        if(dir) {
	  /* only swap pAdd_Dock with pNew_Widget on last seen positions */
	  pNew_Widget->size.x = pAdd_Dock->size.x;
	  pNew_Widget->size.y = pAdd_Dock->size.y;
	  pNew_Widget->gfx = pAdd_Dock->gfx;
	  pAdd_Dock->gfx = NULL;
	  set_wflag(pAdd_Dock, WF_HIDDEN);
        } else {
	  /* repositon all widgets */
	  pBuf = pNew_Widget;
          do {
	    pBuf->size.x = pBuf->next->size.x;
	    pBuf->size.y = pBuf->next->size.y;
	    pBuf->gfx = pBuf->next->gfx;
	    pBuf->next->gfx = NULL;
	    pBuf = pBuf->next;
          } while(pBuf != pDlg->pActiveWidgetList);
          pBuf->gfx = NULL;
          set_wflag(pBuf, WF_HIDDEN);
	  pDlg->pActiveWidgetList = pDlg->pActiveWidgetList->prev;
        }
      } else { /* !last */
        pBuf = pNew_Widget;
        /* find last seen widget */
        if(pDlg->pActiveWidgetList) {
	  pEnd = pDlg->pActiveWidgetList;
	  count = pDlg->pScroll->active * pDlg->pScroll->step - 1;
          while(count && pEnd != pDlg->pBeginActiveWidgetList) {
	    pEnd = pEnd->prev;
	    count--;
          }
        }
        while(pBuf) {
          if(pBuf == pDlg->pBeginActiveWidgetList) {
	    struct widget *pTmp = pBuf;
	    count = pDlg->pScroll->step;
	    while(count) {
	      pTmp = pTmp->next;
	      count--;
	    }
	    pBuf->size.x = pTmp->size.x;
	    pBuf->size.y = pTmp->size.y + pTmp->size.h;
	    /* break when last active widget or last seen widget */
	    break;
          } else {
	    pBuf->size.x = pBuf->prev->size.x;
	    pBuf->size.y = pBuf->prev->size.y;
	    pBuf->gfx = pBuf->prev->gfx;
	    pBuf->prev->gfx = NULL;
	    if(pBuf == pEnd) {
	      break;
	    } 
          }
          pBuf = pBuf->prev;
        }
        if(pOld_End && pBuf->prev == pOld_End) {
          set_wflag(pOld_End, WF_HIDDEN);
        }
      }/* !last */
    } /* pDlg->pBeginActiveWidgetList */
  } else {/* !seen */
    set_wflag(pNew_Widget, WF_HIDDEN);
  }
  
  if(pDlg->pActiveWidgetList && pDlg->pScroll->pScrollBar) {
    widget_undraw(pDlg->pScroll->pScrollBar);
    widget_mark_dirty(pDlg->pScroll->pScrollBar);    
    
    pDlg->pScroll->pScrollBar->size.h = scrollbar_size(pDlg->pScroll);
    if(last) {
      pDlg->pScroll->pScrollBar->size.y = get_position(pDlg);
    }
    if (get_wflags(pDlg->pScroll->pScrollBar) & WF_RESTORE_BACKGROUND) {
      refresh_widget_background(pDlg->pScroll->pScrollBar);
    }
    if (!seen) {
      redraw_vert(pDlg->pScroll->pScrollBar);
    }
  }

  return last;  
}

/**************************************************************************
  Del widget from srolled list and set draw position of all changed widgets
  Don't free pDlg and pDlg->pScroll (if exist)
  It is full secure for multi widget list case.
**************************************************************************/
bool del_widget_from_vertical_scroll_widget_list(struct ADVANCED_DLG *pDlg, 
  						struct widget *pWidget)
{
  int count = 0;
  struct widget *pBuf = pWidget;
  assert(pWidget != NULL);
  assert(pDlg != NULL);
  
  /* if begin == end -> size = 1 */
  if (pDlg->pBeginActiveWidgetList == pDlg->pEndActiveWidgetList) {
    
    if(pDlg->pScroll) {
      pDlg->pScroll->count = 0;
    }
    
    if(pDlg->pBeginActiveWidgetList == pDlg->pBeginWidgetList) {
      pDlg->pBeginWidgetList = pDlg->pBeginWidgetList->next;
    }
    
    if(pDlg->pEndActiveWidgetList == pDlg->pEndWidgetList) {
      pDlg->pEndWidgetList = pDlg->pEndWidgetList->prev;
    }
    
    pDlg->pBeginActiveWidgetList = NULL;
    pDlg->pActiveWidgetList = NULL;
    pDlg->pEndActiveWidgetList = NULL;
      
    widget_undraw(pWidget);
    widget_mark_dirty(pWidget);
    del_widget_from_gui_list(pWidget);
    return FALSE;
  }
  
  if (pDlg->pScroll && pDlg->pActiveWidgetList) {
    /* scrollbar exist and active, start mod. from last seen label */
    
    struct widget *pLast;
    bool widget_found = FALSE;
      
    /* this is always true becouse no-scrolbar case (active*step < count)
       will be suported in other part of code (see "else" part) */
    count = pDlg->pScroll->active * pDlg->pScroll->step;
        
    /* find last */
    pBuf = pDlg->pActiveWidgetList;
    while (count > 0) {
      pBuf = pBuf->prev;
      count--;
    }
    if(!pBuf) {
      pLast = pDlg->pBeginActiveWidgetList;
    } else {
      pLast = pBuf->next;
    }
    
    if(pLast == pDlg->pBeginActiveWidgetList) {
      
      if(pDlg->pScroll->step == 1) {
        pDlg->pActiveWidgetList = pDlg->pActiveWidgetList->next;
        clear_wflag(pDlg->pActiveWidgetList, WF_HIDDEN);
        
        /* look for the widget in the non-visible part */
        pBuf = pDlg->pEndActiveWidgetList;
        while (pBuf != pDlg->pActiveWidgetList) {
          if (pBuf == pWidget) {
            widget_found = TRUE;
            pBuf = pDlg->pActiveWidgetList;
            break;
          }
          pBuf = pBuf->prev;
        }
        
        /* if we haven't found it yet, look in the visible part and update the
           positions of the other widgets */
        if (!widget_found) {
          while (pBuf != pWidget) {
            pBuf->gfx = pBuf->prev->gfx;
            pBuf->prev->gfx = NULL;
            pBuf->size.x = pBuf->prev->size.x;
            pBuf->size.y = pBuf->prev->size.y;
            pBuf = pBuf->prev;
          }
        }
      } else {
	pBuf = pLast;
	/* undraw last widget */
        widget_undraw(pBuf);
        widget_mark_dirty(pBuf);
        FREESURFACE(pBuf->gfx);
	goto STD;
      }  
    } else {
      clear_wflag(pBuf, WF_HIDDEN);
STD:  while (pBuf != pWidget) {
        pBuf->gfx = pBuf->next->gfx;
        pBuf->next->gfx = NULL;
        pBuf->size.x = pBuf->next->size.x;
        pBuf->size.y = pBuf->next->size.y;
        pBuf = pBuf->next;
      }
    }
    
    if ((pDlg->pScroll->count - 1) <= (pDlg->pScroll->active * pDlg->pScroll->step)) {
      /* scrollbar not needed anymore */
      hide_scrollbar(pDlg->pScroll);
      pDlg->pActiveWidgetList = NULL;
    }
    pDlg->pScroll->count--;
    
    if(pDlg->pActiveWidgetList) {
      if (pDlg->pScroll->pScrollBar) {
        widget_undraw(pDlg->pScroll->pScrollBar);
        pDlg->pScroll->pScrollBar->size.h = scrollbar_size(pDlg->pScroll);
        refresh_widget_background(pDlg->pScroll->pScrollBar);
      }
    }
    
  } else { /* no scrollbar */
    pBuf = pDlg->pBeginActiveWidgetList;
    
    /* undraw last widget */
    widget_undraw(pBuf);
    widget_mark_dirty(pBuf);
    FREESURFACE(pBuf->gfx);

    while (pBuf != pWidget) {
      pBuf->gfx = pBuf->next->gfx;
      pBuf->next->gfx = NULL;
      pBuf->size.x = pBuf->next->size.x;
      pBuf->size.y = pBuf->next->size.y;
      pBuf = pBuf->next;
    }

    if (pDlg->pScroll) {
      pDlg->pScroll->count--;
    }
        
  }

  if (pWidget == pDlg->pBeginWidgetList) {
    pDlg->pBeginWidgetList = pWidget->next;
  }

  if (pWidget == pDlg->pBeginActiveWidgetList) {
    pDlg->pBeginActiveWidgetList = pWidget->next;
  }

  if (pWidget == pDlg->pEndActiveWidgetList) {

    if (pWidget == pDlg->pEndWidgetList) {
      pDlg->pEndWidgetList = pWidget->prev;
    }

    if (pWidget == pDlg->pActiveWidgetList) {
      pDlg->pActiveWidgetList = pWidget->prev;
    }

    pDlg->pEndActiveWidgetList = pWidget->prev;

  }

  if (pDlg->pActiveWidgetList && (pDlg->pActiveWidgetList == pWidget)) {
    pDlg->pActiveWidgetList = pWidget->prev;
  }

  del_widget_from_gui_list(pWidget);
  
  if(pDlg->pScroll && pDlg->pScroll->pScrollBar && pDlg->pActiveWidgetList) {
    widget_undraw(pDlg->pScroll->pScrollBar);
    pDlg->pScroll->pScrollBar->size.y = get_position(pDlg);
    refresh_widget_background(pDlg->pScroll->pScrollBar);
  }
  
  return TRUE;
}

/**************************************************************************
  ...
**************************************************************************/
void setup_vertical_scrollbar_default_callbacks(struct ScrollBar *pScroll)
{
  assert(pScroll != NULL);
  if(pScroll->pUp_Left_Button) {
    pScroll->pUp_Left_Button->action = std_up_advanced_dlg_callback;
  }
  if(pScroll->pDown_Right_Button) {
    pScroll->pDown_Right_Button->action = std_down_advanced_dlg_callback;
  }
  if(pScroll->pScrollBar) {
    pScroll->pScrollBar->action = std_vscroll_advanced_dlg_callback;
  }
}

/**************************************************************************
  			Horizontal Scrollbar
**************************************************************************/


/**************************************************************************
  ...
**************************************************************************/
Uint32 create_horizontal_scrollbar(struct ADVANCED_DLG *pDlg,
		  Sint16 start_x, Sint16 start_y, Uint16 width, Uint16 active,
		  bool create_scrollbar, bool create_buttons, bool swap_start_y)
{
  Uint16 count = 0;
  struct widget *pBuf = NULL, *pWindow = NULL;
    
  assert(pDlg != NULL);
  
  pWindow = pDlg->pEndWidgetList;
  
  if(!pDlg->pScroll) {
    pDlg->pScroll = fc_calloc(1, sizeof(struct ScrollBar));
    
    pDlg->pScroll->active = active;
    
    pBuf = pDlg->pEndActiveWidgetList;
    while(pBuf && pBuf != pDlg->pBeginActiveWidgetList->prev) {
      pBuf = pBuf->prev;
      count++;
    }
  
    pDlg->pScroll->count = count;
  }
  
  if(create_buttons) {
    /* create up button */
    pBuf = create_themeicon_button(pTheme->LEFT_Icon, pWindow->dst, NULL, 0);
    
    pBuf->ID = ID_BUTTON;
    pBuf->data.ptr = (void *)pDlg;
    set_wstate(pBuf, FC_WS_NORMAL);
    
    pBuf->size.x = start_x;
    if(swap_start_y) {
      pBuf->size.y = start_y - pBuf->size.h;
    } else {
      pBuf->size.y = start_y;
    }
    
    pDlg->pScroll->min = start_x + pBuf->size.w;
    pDlg->pScroll->pUp_Left_Button = pBuf;
    DownAdd(pBuf, pDlg->pBeginWidgetList);
    pDlg->pBeginWidgetList = pBuf;
    
    
    count = pBuf->size.h;
    
    /* create down button */
    pBuf = create_themeicon_button(pTheme->RIGHT_Icon, pWindow->dst, NULL, 0);
    
    pBuf->ID = ID_BUTTON;
    pBuf->data.ptr = (void *)pDlg;
    set_wstate(pBuf, FC_WS_NORMAL);
    
    pBuf->size.x = start_x + width - pBuf->size.w;
    if(swap_start_y) {
      pBuf->size.y = start_y - pBuf->size.h;
    } else {
      pBuf->size.y = start_y;
    }
    
    pDlg->pScroll->max = pBuf->size.x;
    pDlg->pScroll->pDown_Right_Button = pBuf;
    DownAdd(pBuf, pDlg->pBeginWidgetList);
    pDlg->pBeginWidgetList = pBuf;
    
  }
  
  if(create_scrollbar) {
    /* create vsrollbar */
    pBuf = create_horizontal(pTheme->Horiz, pWindow->dst,
				width, WF_RESTORE_BACKGROUND);
    
    pBuf->ID = ID_SCROLLBAR;
    pBuf->data.ptr = (void *)pDlg;
    set_wstate(pBuf, FC_WS_NORMAL);
     
    if(swap_start_y) {
      pBuf->size.y = start_y - pBuf->size.h;
    } else {
      pBuf->size.y = start_y;
    } 

    if(create_buttons) {
      pBuf->size.x = start_x + pDlg->pScroll->pUp_Left_Button->size.w;
      if(pDlg->pScroll->count > pDlg->pScroll->active) {
	pBuf->size.w = scrollbar_size(pDlg->pScroll);
      } else {
	pBuf->size.w = pDlg->pScroll->max - pDlg->pScroll->min;
      }
    } else {
      pBuf->size.x = start_x;
      pDlg->pScroll->min = start_x;
      pDlg->pScroll->max = start_x + width;
    }
    
    pDlg->pScroll->pScrollBar = pBuf;
    DownAdd(pBuf, pDlg->pBeginWidgetList);
    pDlg->pBeginWidgetList = pBuf;
    
    if(!count) {
      count = pBuf->size.h;
    }
    
  }
  
  return count;
}
