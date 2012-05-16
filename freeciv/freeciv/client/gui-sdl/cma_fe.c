/**********************************************************************
 Freeciv - Copyright (C) 2001 - R. Falke, M. Kaufman
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
#include "fcintl.h"

/* common */
#include "game.h"

/* client */
#include "client_main.h" /* can_client_issue_orders() */

/* gui-sdl */
#include "citydlg.h"
#include "cityrep.h"
#include "cma_fec.h"
#include "colors.h"
#include "graphics.h"
#include "gui_iconv.h"
#include "gui_id.h"
#include "gui_main.h"
#include "gui_tilespec.h"
#include "mapview.h"
#include "sprite.h"
#include "themespec.h"
#include "widget.h"

#include "cma_fe.h"

struct hmove {
  struct widget *pScrollBar;
  int min, max, base;
};

static struct cma_dialog {
  struct city *pCity;
  struct SMALL_DLG *pDlg;
  struct ADVANCED_DLG *pAdv;  
  struct cm_result *pResult;
  struct cm_parameter edited_cm_parm;
} *pCma = NULL;

enum specialist_type {
  SP_ELVIS, SP_SCIENTIST, SP_TAXMAN, SP_LAST   
};

static void set_cma_hscrollbars(void);

/* =================================================================== */

static int cma_dlg_callback(struct widget *pWindow)
{
  return -1;
}

static int exit_cma_dialog_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    popdown_city_cma_dialog();
    flush_dirty();
  }
  return -1;
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
static Uint16 scroll_mouse_motion_handler(SDL_MouseMotionEvent *pMotionEvent, void *pData)
{
  struct hmove *pMotion = (struct hmove *)pData;
  char cBuf[4];
  
  pMotionEvent->x -= pMotion->pScrollBar->dst->dest_rect.x;
  
  if (pMotion && pMotionEvent->xrel &&
    (pMotionEvent->x >= pMotion->min) && (pMotionEvent->x <= pMotion->max)) {

    /* draw bcgd */
    widget_undraw(pMotion->pScrollBar);
    widget_mark_dirty(pMotion->pScrollBar);
       
    if ((pMotion->pScrollBar->size.x + pMotionEvent->xrel) >
	 (pMotion->max - pMotion->pScrollBar->size.w)) {
      pMotion->pScrollBar->size.x = pMotion->max - pMotion->pScrollBar->size.w;
    } else {
      if ((pMotion->pScrollBar->size.x + pMotionEvent->xrel) < pMotion->min) {
	pMotion->pScrollBar->size.x = pMotion->min;
      } else {
	pMotion->pScrollBar->size.x += pMotionEvent->xrel;
      }
    }
    
    *(int *)pMotion->pScrollBar->data.ptr =
    		pMotion->base + (pMotion->pScrollBar->size.x - pMotion->min);
    
    my_snprintf(cBuf, sizeof(cBuf), "%d", *(int *)pMotion->pScrollBar->data.ptr);
    copy_chars_to_string16(pMotion->pScrollBar->next->string16, cBuf);
    
    /* redraw label */
    widget_redraw(pMotion->pScrollBar->next);
    widget_mark_dirty(pMotion->pScrollBar->next);
    
    /* redraw scroolbar */
    if (get_wflags(pMotion->pScrollBar) & WF_RESTORE_BACKGROUND) {
      refresh_widget_background(pMotion->pScrollBar);
    }
    widget_redraw(pMotion->pScrollBar);
    widget_mark_dirty(pMotion->pScrollBar);

    flush_dirty();
  }				/* if (count) */
  
  return ID_ERROR;
}

static int min_horiz_cma_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct hmove pMotion;
      
    pMotion.pScrollBar = pWidget;
    pMotion.min = pWidget->next->size.x + pWidget->next->size.w + 5;
    pMotion.max = pMotion.min + 70;
    pMotion.base = -20;
    
    MOVE_STEP_X = 2;
    MOVE_STEP_Y = 0;
    /* Filter mouse motion events */
    SDL_SetEventFilter(FilterMouseMotionEvents);
    gui_event_loop((void *)(&pMotion), NULL, NULL, NULL, NULL,
                    scroll_mouse_button_up, scroll_mouse_motion_handler);
    /* Turn off Filter mouse motion events */
    SDL_SetEventFilter(NULL);
    MOVE_STEP_X = DEFAULT_MOVE_STEP;
    MOVE_STEP_Y = DEFAULT_MOVE_STEP;
    
    pSellected_Widget = pWidget;
    set_wstate(pWidget, FC_WS_SELLECTED);
    /* save the change */
    cmafec_set_fe_parameter(pCma->pCity, &pCma->edited_cm_parm);
    /* refreshes the cma */
    if (cma_is_city_under_agent(pCma->pCity, NULL)) {
      cma_release_city(pCma->pCity);
      cma_put_city_under_agent(pCma->pCity, &pCma->edited_cm_parm);
    }
    update_city_cma_dialog();
  }
  return -1;
}

static int factor_horiz_cma_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct hmove pMotion;
      
    pMotion.pScrollBar = pWidget;
    pMotion.min = pWidget->next->size.x + pWidget->next->size.w + 5;
    pMotion.max = pMotion.min + 54;
    pMotion.base = 1;
  
    MOVE_STEP_X = 2;
    MOVE_STEP_Y = 0;
    /* Filter mouse motion events */
    SDL_SetEventFilter(FilterMouseMotionEvents);
    gui_event_loop((void *)(&pMotion), NULL, NULL, NULL, NULL,
                    scroll_mouse_button_up, scroll_mouse_motion_handler);
    /* Turn off Filter mouse motion events */
    SDL_SetEventFilter(NULL);
    MOVE_STEP_X = DEFAULT_MOVE_STEP;
    MOVE_STEP_Y = DEFAULT_MOVE_STEP;
    
    pSellected_Widget = pWidget;
    set_wstate(pWidget, FC_WS_SELLECTED);
    /* save the change */
    cmafec_set_fe_parameter(pCma->pCity, &pCma->edited_cm_parm);
    /* refreshes the cma */
    if (cma_is_city_under_agent(pCma->pCity, NULL)) {
      cma_release_city(pCma->pCity);
      cma_put_city_under_agent(pCma->pCity, &pCma->edited_cm_parm);
    }
    update_city_cma_dialog();
  }
  return -1;
}

static int toggle_cma_celebrating_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    pCma->edited_cm_parm.require_happy ^= TRUE;
    /* save the change */
    cmafec_set_fe_parameter(pCma->pCity, &pCma->edited_cm_parm);
    update_city_cma_dialog();
  }
  return -1;
}

/* ============================================================= */

static int save_cma_window_callback(struct widget *pWindow)
{
  return -1;
}

static int ok_save_cma_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if(pWidget && pCma && pCma->pAdv) {
      struct widget *pEdit = (struct widget *)pWidget->data.ptr;
      char *name = convert_to_chars(pEdit->string16->text);
   
      if(name) { 
        cmafec_preset_add(name, &pCma->edited_cm_parm);
        FC_FREE(name);
      } else {
        cmafec_preset_add(_("new preset"), &pCma->edited_cm_parm);
      }
          
      del_group_of_widgets_from_gui_list(pCma->pAdv->pBeginWidgetList,
                                                  pCma->pAdv->pEndWidgetList);
      FC_FREE(pCma->pAdv);
      
      update_city_cma_dialog();
    }
  }
  return -1;
}

/* -------------------------------------------------------------------- */
/*   Cancel : SAVE, LOAD, DELETE Dialogs				*/
/* -------------------------------------------------------------------- */
static int cancel_SLD_cma_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if(pCma && pCma->pAdv) {
      popdown_window_group_dialog(pCma->pAdv->pBeginWidgetList,
                                  pCma->pAdv->pEndWidgetList);
      FC_FREE(pCma->pAdv->pScroll);
      FC_FREE(pCma->pAdv);
      flush_dirty();
    }
  }
  return -1;
}

static int save_cma_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct widget *pBuf, *pWindow;
    SDL_String16 *pStr;
    SDL_Surface *pText;
    SDL_Rect dst;
    SDL_Rect area;
    
    if (pCma->pAdv) {
      return 1;
    }
    
    pCma->pAdv = fc_calloc(1, sizeof(struct ADVANCED_DLG));
        
    pStr = create_str16_from_char(_("Name new preset"), adj_font(12));
    pStr->style |= TTF_STYLE_BOLD;
  
    pWindow = create_window_skeleton(NULL, pStr, 0);
  
    pWindow->action = save_cma_window_callback;
    set_wstate(pWindow, FC_WS_NORMAL);
    pCma->pAdv->pEndWidgetList = pWindow;
  
    add_to_gui_list(ID_WINDOW, pWindow);
    
    area = pWindow->area;
    area.h = MAX(area.h, 1);
    
    /* ============================================================= */
    /* label */
    pStr = create_str16_from_char(_("What should we name the preset?"), adj_font(10));
    pStr->style |= (TTF_STYLE_BOLD|SF_CENTER);
    pStr->fgcol = *get_game_colorRGB(COLOR_THEME_CMA_TEXT);
    
    pText = create_text_surf_from_str16(pStr);
    FREESTRING16(pStr);
    area.w = MAX(area.w, pText->w);
    area.h += pText->h + adj_size(5);
    /* ============================================================= */
    
    pBuf = create_edit(NULL, pWindow->dst,
                  create_str16_from_char(_("new preset"), adj_font(12)), adj_size(100),
                          (WF_RESTORE_BACKGROUND|WF_FREE_STRING));
    set_wstate(pBuf, FC_WS_NORMAL);
    area.h += pBuf->size.h;
    area.w = MAX(area.w, pBuf->size.w);
    
    add_to_gui_list(ID_EDIT, pBuf);
    /* ============================================================= */
    
    pBuf = create_themeicon_button_from_chars(pTheme->OK_Icon, pWindow->dst,
                                                _("Yes"), adj_font(12), 0);
  
    pBuf->action = ok_save_cma_callback;
    set_wstate(pBuf, FC_WS_NORMAL);
    pBuf->key = SDLK_RETURN;
    add_to_gui_list(ID_BUTTON, pBuf);
    pBuf->data.ptr = (void *)pBuf->next;
    
    pBuf = create_themeicon_button_from_chars(pTheme->CANCEL_Icon,
                              pWindow->dst, _("No"), adj_font(12), 0);
    pBuf->action = cancel_SLD_cma_callback;
    set_wstate(pBuf, FC_WS_NORMAL);
    pBuf->key = SDLK_ESCAPE;
    
    add_to_gui_list(ID_BUTTON, pBuf);
    
    area.h += pBuf->size.h;
    pBuf->size.w = MAX(pBuf->next->size.w, pBuf->size.w);
    pBuf->next->size.w = pBuf->size.w;
    area.w = MAX(area.w, 2 * pBuf->size.w + adj_size(20));
      
    pCma->pAdv->pBeginWidgetList = pBuf;
    
    /* setup window size and start position */
    area.w += adj_size(20);
    area.h += adj_size(15);

    resize_window(pWindow, NULL, get_game_colorRGB(COLOR_THEME_BACKGROUND),
                  (pWindow->size.w - pWindow->area.w) + area.w,
                  (pWindow->size.h - pWindow->area.h) + area.h);

    area = pWindow->area;

    widget_set_position(pWindow,
                        pWidget->size.x - pWindow->size.w / 2,
                        pWidget->size.y - pWindow->size.h);
  
    /* setup rest of widgets */
    /* label */
    dst.x = area.x + (area.w - pText->w) / 2;
    dst.y = area.y + 1;
    alphablit(pText, NULL, pWindow->theme, &dst);
    dst.y += pText->h + adj_size(5);
    FREESURFACE(pText);
    
    /* edit */
    pBuf = pWindow->prev;
    pBuf->size.w = area.w - adj_size(10);
    pBuf->size.x = area.x + adj_size(5);
    pBuf->size.y = dst.y;
    dst.y += pBuf->size.h + adj_size(5);
    
    /* yes */
    pBuf = pBuf->prev;
    pBuf->size.x = area.x + (area.w - (2 * pBuf->size.w + adj_size(20))) / 2;
    pBuf->size.y = dst.y;
    
    /* no */
    pBuf = pBuf->prev;
    pBuf->size.x = pBuf->next->size.x + pBuf->next->size.w + adj_size(20);
    pBuf->size.y = pBuf->next->size.y;
    
    /* ================================================== */
    /* redraw */
    redraw_group(pCma->pAdv->pBeginWidgetList, pWindow, 0);
    widget_mark_dirty(pWindow);
    flush_dirty();
  }
  return -1;
}
/* ================================================== */

static int LD_cma_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    bool load = pWidget->data.ptr != NULL;
    int index = MAX_ID - pWidget->ID;
    
    popdown_window_group_dialog(pCma->pAdv->pBeginWidgetList,
                                pCma->pAdv->pEndWidgetList);
    FC_FREE(pCma->pAdv->pScroll);
    FC_FREE(pCma->pAdv);
    
    if(load) {
      cm_copy_parameter(&pCma->edited_cm_parm, cmafec_preset_get_parameter(index));
      set_cma_hscrollbars();
      /* save the change */
      cmafec_set_fe_parameter(pCma->pCity, &pCma->edited_cm_parm);
      /* stop the cma */
      if (cma_is_city_under_agent(pCma->pCity, NULL)) {
        cma_release_city(pCma->pCity);
      }
    } else {
      cmafec_preset_remove(index);
    }
      
    update_city_cma_dialog();
  }
  return -1;
}


static void popup_load_del_presets_dialog(bool load, struct widget *pButton)
{
  int hh, count, i;
  struct widget *pBuf, *pWindow;
  SDL_String16 *pStr;
  SDL_Rect area;
  
  if (pCma->pAdv) {
    return;
  }
  
  count = cmafec_preset_num();
  
  if(count == 1) {
    if(load) {
      cm_copy_parameter(&pCma->edited_cm_parm, cmafec_preset_get_parameter(0));
      set_cma_hscrollbars();
      /* save the change */
      cmafec_set_fe_parameter(pCma->pCity, &pCma->edited_cm_parm);
      /* stop the cma */
      if (cma_is_city_under_agent(pCma->pCity, NULL)) {
        cma_release_city(pCma->pCity);
      }
    } else {
      cmafec_preset_remove(0);
    }
    update_city_cma_dialog();
    return;
  }
  
  pCma->pAdv = fc_calloc(1, sizeof(struct ADVANCED_DLG));
      
  pStr = create_str16_from_char(_("Presets"), adj_font(12));
  pStr->style |= TTF_STYLE_BOLD;

  pWindow = create_window_skeleton(NULL, pStr, 0);

  pWindow->action = save_cma_window_callback;
  set_wstate(pWindow, FC_WS_NORMAL);
  pCma->pAdv->pEndWidgetList = pWindow;

  add_to_gui_list(ID_WINDOW, pWindow);

  area = pWindow->area;
  
  /* ---------- */
  /* create exit button */
  pBuf = create_themeicon(pTheme->Small_CANCEL_Icon, pWindow->dst,
	                  WF_WIDGET_HAS_INFO_LABEL | WF_RESTORE_BACKGROUND);
  pBuf->string16 = create_str16_from_char(_("Close Dialog (Esc)"), adj_font(12));
  pBuf->action = cancel_SLD_cma_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->key = SDLK_ESCAPE;
  area.w += (pBuf->size.w + adj_size(10));
  
  add_to_gui_list(ID_BUTTON, pBuf);
  /* ---------- */
  
  for(i = 0; i < count; i++) {
    pStr = create_str16_from_char(cmafec_preset_get_descr(i), adj_font(10));
    pStr->style |= TTF_STYLE_BOLD;
    pBuf = create_iconlabel(NULL, pWindow->dst, pStr,
    	     (WF_RESTORE_BACKGROUND|WF_DRAW_TEXT_LABEL_WITH_SPACE));
    pBuf->string16->bgcol = (SDL_Color) {0, 0, 0, 0};
    pBuf->action = LD_cma_callback;
    
    area.w = MAX(area.w, pBuf->size.w);
    area.h += pBuf->size.h;
    set_wstate(pBuf , FC_WS_NORMAL);
    
    if(load) {
      pBuf->data.ptr = (void *)1;
    } else {
      pBuf->data.ptr = NULL;
    }
    
    add_to_gui_list(MAX_ID - i, pBuf);
    
    if (i > 10)
    {
      set_wflag(pBuf, WF_HIDDEN);
    }
  }
  pCma->pAdv->pBeginWidgetList = pBuf;
  pCma->pAdv->pBeginActiveWidgetList = pCma->pAdv->pBeginWidgetList;
  pCma->pAdv->pEndActiveWidgetList = pWindow->prev->prev;
  pCma->pAdv->pActiveWidgetList = pCma->pAdv->pEndActiveWidgetList;
  
  area.w += adj_size(2);
  area.h += 1;
  
  if (count > 11)
  {
    create_vertical_scrollbar(pCma->pAdv, 1, 11, FALSE, TRUE);
        
    /* ------- window ------- */
    area.h = 11 * pWindow->prev->prev->size.h + adj_size(2) 
    		+ 2 * pCma->pAdv->pScroll->pUp_Left_Button->size.h;
    pCma->pAdv->pScroll->pUp_Left_Button->size.w = area.w;
    pCma->pAdv->pScroll->pDown_Right_Button->size.w = area.w;
  }
  
  /* ----------------------------------- */

  resize_window(pWindow, NULL, NULL, 
                (pWindow->size.w - pWindow->area.w) + area.w,
                (pWindow->size.h - pWindow->area.h) + area.h);
  
  area = pWindow->area;
  
  widget_set_position(pWindow,
                      pButton->size.x - (pWindow->size.w / 2),
                      pButton->size.y - pWindow->size.h);
  
  /* exit button */
  pBuf = pWindow->prev; 
  pBuf->size.x = area.x + area.w - pBuf->size.w - 1;
  pBuf->size.y = pWindow->size.y + adj_size(2);
  
  pBuf = pBuf->prev;
  hh = (pCma->pAdv->pScroll ? pCma->pAdv->pScroll->pUp_Left_Button->size.h + 1 : 0);
  setup_vertical_widgets_position(1, area.x + 1,
		  area.y + 1 + hh, area.w - 1, 0,
		  pCma->pAdv->pBeginActiveWidgetList, pBuf);
                  
  if(pCma->pAdv->pScroll) {
    pCma->pAdv->pScroll->pUp_Left_Button->size.x = area.x;
    pCma->pAdv->pScroll->pUp_Left_Button->size.y = area.y;
    pCma->pAdv->pScroll->pDown_Right_Button->size.x = area.x;
    pCma->pAdv->pScroll->pDown_Right_Button->size.y =
      area.y + area.h - pCma->pAdv->pScroll->pDown_Right_Button->size.h;
  }
    
  /* ==================================================== */
  /* redraw */
  redraw_group(pCma->pAdv->pBeginWidgetList, pWindow, 0);

  widget_flush(pWindow);
}

static int load_cma_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    popup_load_del_presets_dialog(TRUE, pWidget);
  }
  return -1;
}

static int del_cma_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    popup_load_del_presets_dialog(FALSE, pWidget);
  }
  return -1;
}

/* ================================================== */

/**************************************************************************
 changes the workers of the city to the cma parameters and puts the
 city under agent control
**************************************************************************/
static int run_cma_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    cma_put_city_under_agent(pCma->pCity, &pCma->edited_cm_parm);
    update_city_cma_dialog();
  }
  return -1;
}

/**************************************************************************
 changes the workers of the city to the cma parameters
**************************************************************************/
static int run_cma_once_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct cm_result result;
  
    update_city_cma_dialog();  
    /* fill in result label */
    cm_query_result(pCma->pCity, &pCma->edited_cm_parm, &result);
    cma_apply_result(pCma->pCity, &result);
  }
  return -1;
}

static int stop_cma_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    cma_release_city(pCma->pCity);
    update_city_cma_dialog();
  }
  return -1;
}

/* ===================================================================== */

static void set_cma_hscrollbars(void)
{
  struct widget *pBuf;
  char cBuf[4];
  
  if (!pCma) {
    return;
  }
  
  /* exit button */
  pBuf = pCma->pDlg->pEndWidgetList->prev;
  output_type_iterate(i) {
    /* min label */
    pBuf = pBuf->prev;
    my_snprintf(cBuf, sizeof(cBuf), "%d", *(int *)pBuf->prev->data.ptr);
    copy_chars_to_string16(pBuf->string16, cBuf);
        
    /* min scrollbar */
    pBuf = pBuf->prev;
    pBuf->size.x = pBuf->next->size.x
    	+ pBuf->next->size.w + adj_size(5) + adj_size(20) + *(int *)pBuf->data.ptr;
    
    /* factor label */
    pBuf = pBuf->prev;
    my_snprintf(cBuf, sizeof(cBuf), "%d", *(int *)pBuf->prev->data.ptr);
    copy_chars_to_string16(pBuf->string16, cBuf);
    
    /* factor scrollbar*/
    pBuf = pBuf->prev;
    pBuf->size.x = pBuf->next->size.x 
    		+ pBuf->next->size.w + adj_size(5) + *(int *)pBuf->data.ptr - 1;
    
  } output_type_iterate_end;
  
  /* happy factor label */
  pBuf = pBuf->prev;
  my_snprintf(cBuf, sizeof(cBuf), "%d", *(int *)pBuf->prev->data.ptr);
  copy_chars_to_string16(pBuf->string16, cBuf);
  
  /* happy factor scrollbar */
  pBuf = pBuf->prev;
  pBuf->size.x = pBuf->next->size.x 
    		+ pBuf->next->size.w + adj_size(5) + *(int *)pBuf->data.ptr - 1;
    
}

#if 0
static bool is_worker(const struct city *pCity, int x, int y)
{
  return pCma && pCma->pResult && pCma->pResult->worker_positions_used[x][y];
}
#endif

/* ===================================================================== */
void update_city_cma_dialog(void)
{
  SDL_Color bg_color = {255, 255, 255, 136};
  
  int count, step, i;
  struct widget *pBuf = pCma->pDlg->pEndWidgetList; /* pWindow */
  SDL_Surface *pText;
  SDL_String16 *pStr;
  SDL_Rect dst;
  bool cma_presets_exist = cmafec_preset_num() > 0;
  bool client_under_control = can_client_issue_orders();
  bool controlled = cma_is_city_under_agent(pCma->pCity, NULL);
  struct cm_result result;
    
  /* redraw window background and exit button */
  redraw_group(pBuf->prev, pBuf, 0);
  		  
  /* fill in result label */
  cm_result_from_main_map(&result, pCma->pCity, TRUE);
  
  if(result.found_a_valid) {
    /* redraw resources */
    pCma->pResult = &result;
#if 0    
    refresh_city_resource_map(pBuf->dst->surface, pBuf->area.x + adj_size(22),
	  pBuf->area.y + adj_size(31), pCma->pCity, is_worker);
#endif    
    pCma->pResult = NULL;
  
    /* redraw Citizens */
    count = pCma->pCity->size;
    
    pText = get_tax_surface(O_LUXURY);
    step = (pBuf->size.w - adj_size(20)) / pText->w;
    if (count > step) {
      step = (pBuf->size.w - adj_size(20) - pText->w) / (count - 1);
    } else {
      step = pText->w;
    }

    dst.y = pBuf->area.y + adj_size(4);
    dst.x = pBuf->area.x + adj_size(7);

    for (i = 0;
      i < count - (result.specialists[SP_ELVIS]
		   + result.specialists[SP_SCIENTIST]
		   + result.specialists[SP_TAXMAN]); i++) {
      pText = adj_surf(get_citizen_surface(CITIZEN_CONTENT, i));
      alphablit(pText, NULL, pBuf->dst->surface, &dst);
      dst.x += step;
    }
    
    pText = get_tax_surface(O_LUXURY);
    for (i = 0; i < result.specialists[SP_ELVIS]; i++) {
      alphablit(pText, NULL, pBuf->dst->surface, &dst);
      dst.x += step;
    }

    pText = get_tax_surface(O_GOLD);
    for (i = 0; i < result.specialists[SP_TAXMAN]; i++) {
      alphablit(pText, NULL, pBuf->dst->surface, &dst);
      dst.x += step;
    }

    pText = get_tax_surface(O_SCIENCE);
    for (i = 0; i < result.specialists[SP_SCIENTIST]; i++) {
      alphablit(pText, NULL, pBuf->dst->surface, &dst);
      dst.x += step;
    }
  }
  
  /* create result text surface */
  pStr = create_str16_from_char(
  	cmafec_get_result_descr(pCma->pCity, &result, &pCma->edited_cm_parm), adj_font(12));
  
  pText = create_text_surf_from_str16(pStr);
  FREESTRING16(pStr);
  
  /* fill result text background */  
  dst.x = pBuf->area.x + adj_size(7);
  dst.y = pBuf->area.y + adj_size(186);
  dst.w = pText->w + adj_size(10);
  dst.h = pText->h + adj_size(10);
  SDL_FillRectAlpha(pBuf->dst->surface, &dst, &bg_color);
  putframe(pBuf->dst->surface, dst.x, dst.y, dst.x + dst.w - 1, dst.y + dst.h - 1,
           map_rgba(pBuf->dst->surface->format, *get_game_colorRGB(COLOR_THEME_CMA_FRAME)));
  
  dst.x += adj_size(5);
  dst.y += adj_size(5);
  alphablit(pText, NULL, pBuf->dst->surface, &dst);
  FREESURFACE(pText);
  
  /* happy factor scrollbar */
  pBuf = pCma->pDlg->pBeginWidgetList->next->next->next->next->next->next->next;
  if(client_under_control && get_checkbox_state(pBuf->prev)) {
    set_wstate(pBuf, FC_WS_NORMAL);
  } else {
    set_wstate(pBuf, FC_WS_DISABLED);
  }
  
  /* save as ... */
  pBuf = pBuf->prev->prev;
  if(client_under_control) {
    set_wstate(pBuf, FC_WS_NORMAL);
  } else {
    set_wstate(pBuf, FC_WS_DISABLED);
  }
  
  /* load */
  pBuf = pBuf->prev;
  if(cma_presets_exist && client_under_control) {
    set_wstate(pBuf, FC_WS_NORMAL);
  } else {
    set_wstate(pBuf, FC_WS_DISABLED);
  }
  
  /* del */
  pBuf = pBuf->prev;
  if(cma_presets_exist && client_under_control) {
    set_wstate(pBuf, FC_WS_NORMAL);
  } else {
    set_wstate(pBuf, FC_WS_DISABLED);
  }
  
  /* Run */
  pBuf = pBuf->prev;
  if(client_under_control && result.found_a_valid && !controlled) {
    set_wstate(pBuf, FC_WS_NORMAL);
  } else {
    set_wstate(pBuf, FC_WS_DISABLED);
  }
  
  /* Run once */
  pBuf = pBuf->prev;
  if(client_under_control && result.found_a_valid && !controlled) {
    set_wstate(pBuf, FC_WS_NORMAL);
  } else {
    set_wstate(pBuf, FC_WS_DISABLED);
  }
  
  /* stop */
  pBuf = pBuf->prev;
  if(client_under_control && controlled) {
    set_wstate(pBuf, FC_WS_NORMAL);
  } else {
    set_wstate(pBuf, FC_WS_DISABLED);
  }
  
  /* redraw rest widgets */
  redraw_group(pCma->pDlg->pBeginWidgetList,
  		pCma->pDlg->pEndWidgetList->prev->prev, 0);
  
  widget_flush(pCma->pDlg->pEndWidgetList);
}

void popup_city_cma_dialog(struct city *pCity)
{
  SDL_Color bg_color = {255, 255, 255, 136};
  
  struct widget *pWindow, *pBuf;
  SDL_Surface *pLogo, *pText[O_LAST + 1], *pMinimal, *pFactor;
  SDL_Surface *pCity_Map;
  SDL_String16 *pStr;
  char cBuf[128];
  int w, text_w, x, cs;
  SDL_Rect dst, area;
  
  if (pCma) {
    return;
  }
  
  pCma = fc_calloc(1, sizeof(struct cma_dialog));
  pCma->pCity = pCity;
  pCma->pDlg = fc_calloc(1, sizeof(struct SMALL_DLG));
  pCma->pAdv = NULL;
  pCma->pResult = NULL;  
  pCity_Map = get_scaled_city_map(pCity);  
  
  cmafec_get_fe_parameter(pCity, &pCma->edited_cm_parm);
  
  /* --------------------------- */
    
  my_snprintf(cBuf, sizeof(cBuf),
	      _("City of %s (Population %s citizens) : %s"),
	      city_name(pCity),
	      population_to_text(city_population(pCity)),
	      _("Citizen Governor"));
  
  pStr = create_str16_from_char(cBuf, adj_font(12));
  pStr->style |= TTF_STYLE_BOLD;

  pWindow = create_window_skeleton(NULL, pStr, 0);
  
  pWindow->action = cma_dlg_callback;
  set_wstate(pWindow, FC_WS_NORMAL);
  add_to_gui_list(ID_WINDOW, pWindow);
  pCma->pDlg->pEndWidgetList = pWindow;
  
  area = pWindow->area;
  
  /* ---------- */
  /* create exit button */
  pBuf = create_themeicon(pTheme->Small_CANCEL_Icon, pWindow->dst,
  	                  WF_WIDGET_HAS_INFO_LABEL | WF_RESTORE_BACKGROUND);
  pBuf->string16 = create_str16_from_char(_("Close Dialog (Esc)"), adj_font(12));
  pBuf->action = exit_cma_dialog_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->key = SDLK_ESCAPE;
  area.w += (pBuf->size.w + adj_size(10));
  
  add_to_gui_list(ID_BUTTON, pBuf);

  pStr = create_string16(NULL, 0, adj_font(12));
  text_w = 0;
  
  copy_chars_to_string16(pStr, _("Minimal Surplus"));
  pMinimal = create_text_surf_from_str16(pStr);
    
  copy_chars_to_string16(pStr, _("Factor"));
  pFactor = create_text_surf_from_str16(pStr);
    
  /* ---------- */
  output_type_iterate(i) {
    copy_chars_to_string16(pStr, get_output_name(i));
    pText[i] = create_text_surf_from_str16(pStr);
    text_w = MAX(text_w, pText[i]->w);
    
    /* minimal label */
    pBuf = create_iconlabel(NULL, pWindow->dst,
    		create_str16_from_char("999", adj_font(10)),
			(WF_FREE_STRING | WF_RESTORE_BACKGROUND));
    

    add_to_gui_list(ID_LABEL, pBuf);
    
    /* minimal scrollbar */
    pBuf = create_horizontal(pTheme->Horiz, pWindow->dst, adj_size(30),
			(WF_RESTORE_BACKGROUND));

    pBuf->action = min_horiz_cma_callback;
    pBuf->data.ptr = &pCma->edited_cm_parm.minimal_surplus[i];
  
    set_wstate(pBuf, FC_WS_NORMAL);

    add_to_gui_list(ID_SCROLLBAR, pBuf);
    
    /* factor label */
    pBuf = create_iconlabel(NULL, pWindow->dst,
    		create_str16_from_char("999", adj_font(10)),
			(WF_FREE_STRING | WF_RESTORE_BACKGROUND));
    
    add_to_gui_list(ID_LABEL, pBuf);
    
    /* factor scrollbar */
    pBuf = create_horizontal(pTheme->Horiz, pWindow->dst, adj_size(30),
			(WF_RESTORE_BACKGROUND));

    pBuf->action = factor_horiz_cma_callback;
    pBuf->data.ptr = &pCma->edited_cm_parm.factor[i];
  
    set_wstate(pBuf, FC_WS_NORMAL);

    add_to_gui_list(ID_SCROLLBAR, pBuf);
  } output_type_iterate_end;
  
  copy_chars_to_string16(pStr, _("Celebrate"));
  pText[O_LAST] = create_text_surf_from_str16(pStr);
  FREESTRING16(pStr);
  
  /* happy factor label */
  pBuf = create_iconlabel(NULL, pWindow->dst,
    		create_str16_from_char("999", adj_font(10)),
			(WF_FREE_STRING | WF_RESTORE_BACKGROUND));
  
  add_to_gui_list(ID_LABEL, pBuf);
  
  /* happy factor scrollbar */
  pBuf = create_horizontal(pTheme->Horiz, pWindow->dst, adj_size(30),
			(WF_RESTORE_BACKGROUND));

  pBuf->action = factor_horiz_cma_callback;
  pBuf->data.ptr = &pCma->edited_cm_parm.happy_factor;
  
  set_wstate(pBuf, FC_WS_NORMAL);

  add_to_gui_list(ID_SCROLLBAR, pBuf);
  
  /* celebrating */
  pBuf = create_checkbox(pWindow->dst,
  		pCma->edited_cm_parm.require_happy, WF_RESTORE_BACKGROUND);
  
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->action = toggle_cma_celebrating_callback;
  add_to_gui_list(ID_CHECKBOX, pBuf);
    
  /* save as ... */
  pBuf = create_themeicon(pTheme->SAVE_Icon, pWindow->dst,
	(WF_RESTORE_BACKGROUND|WF_WIDGET_HAS_INFO_LABEL|WF_FREE_STRING));
  
  pBuf->action = save_cma_callback;
  pBuf->string16 = create_str16_from_char(_("Save settings as..."), adj_font(10));
  
  add_to_gui_list(ID_ICON, pBuf);
  
  /* load settings */
  pBuf = create_themeicon(pTheme->LOAD_Icon, pWindow->dst,
  	(WF_RESTORE_BACKGROUND|WF_WIDGET_HAS_INFO_LABEL|WF_FREE_STRING));
  
  pBuf->action = load_cma_callback;
  pBuf->string16 = create_str16_from_char(_("Load settings"), adj_font(10));
  
  add_to_gui_list(ID_ICON, pBuf);
  
  /* del settings */
  pBuf = create_themeicon(pTheme->DELETE_Icon, pWindow->dst,
  	(WF_RESTORE_BACKGROUND|WF_WIDGET_HAS_INFO_LABEL|WF_FREE_STRING));
  
  pBuf->action = del_cma_callback;
  pBuf->string16 = create_str16_from_char(_("Delete settings"), adj_font(10));
  
  add_to_gui_list(ID_ICON, pBuf);
    
  /* run cma */
  pBuf = create_themeicon(pTheme->QPROD_Icon, pWindow->dst,
  	(WF_RESTORE_BACKGROUND|WF_WIDGET_HAS_INFO_LABEL|WF_FREE_STRING));
  
  pBuf->action = run_cma_callback;
  pBuf->string16 = create_str16_from_char(_("Control city"), adj_font(10));
  
  add_to_gui_list(ID_ICON, pBuf);
  
  /* run cma onece */
  pBuf = create_themeicon(pTheme->FindCity_Icon, pWindow->dst,
  	(WF_RESTORE_BACKGROUND|WF_WIDGET_HAS_INFO_LABEL|WF_FREE_STRING));
  
  pBuf->action = run_cma_once_callback;
  pBuf->string16 = create_str16_from_char(_("Apply once"), adj_font(10));
  
  add_to_gui_list(ID_ICON, pBuf);
  
  /* del settings */
  pBuf = create_themeicon(pTheme->Support_Icon,	pWindow->dst,
  	(WF_RESTORE_BACKGROUND|WF_WIDGET_HAS_INFO_LABEL|WF_FREE_STRING));
  
  pBuf->action = stop_cma_callback;
  pBuf->string16 = create_str16_from_char(_("Release city"), adj_font(10));
  
  add_to_gui_list(ID_ICON, pBuf);
  
  /* -------------------------------- */
  pCma->pDlg->pBeginWidgetList = pBuf;

#ifdef SMALL_SCREEN
  area.w = MAX(pCity_Map->w + adj_size(220) + text_w + adj_size(10) +
	  (pWindow->prev->prev->size.w + adj_size(5 + 70 + 5) +
			  pWindow->prev->prev->size.w + adj_size(5 + 55 + 15)), w);
  area.h = adj_size(390) - (pWindow->size.w - pWindow->area.w);
#else
  area.w = MAX(pCity_Map->w + adj_size(150) + text_w + adj_size(10) +
	  (pWindow->prev->prev->size.w + adj_size(5 + 70 + 5) +
			  pWindow->prev->prev->size.w + adj_size(5 + 55 + 15)), area.w);
  area.h = adj_size(360) - (pWindow->size.w - pWindow->area.w);
#endif

  pLogo = theme_get_background(theme, BACKGROUND_CITYGOVDLG);
  if(resize_window(pWindow, pLogo, NULL,
                    (pWindow->size.w - pWindow->area.w) + area.w,
                    (pWindow->size.w - pWindow->area.w) + area.h)) {
    FREESURFACE(pLogo);
  }
  
  pLogo = SDL_DisplayFormat(pWindow->theme);
  FREESURFACE(pWindow->theme);
  pWindow->theme = pLogo;

  area = pWindow->area;
  
  widget_set_position(pWindow,
                      (Main.screen->w - pWindow->size.w) / 2,
                      (Main.screen->h - pWindow->size.h) / 2);
  
  /* exit button */
  pBuf = pWindow->prev;
  pBuf->size.x = area.x + area.w - pBuf->size.w - 1;
  pBuf->size.y = pWindow->size.y + adj_size(2);
  
  /* ---------- */
  dst.x = pCity_Map->w + adj_size(80) +
	  (pWindow->size.w - (pCity_Map->w + adj_size(40)) -
  	    (text_w + adj_size(10) + pWindow->prev->prev->size.w + adj_size(5 + 70 + 5) +
		pWindow->prev->prev->size.w + adj_size(5 + 55))) / 2;
  
#ifdef SMALL_SCREEN
  dst.x += 22;  
#endif
  
  dst.y =  adj_size(75);
  
  x = area.x = dst.x - adj_size(10);
  area.y = dst.y - adj_size(20);
  w = area.w = adj_size(10) + text_w + adj_size(10) + pWindow->prev->prev->size.w + adj_size(5 + 70 + 5) +
		pWindow->prev->prev->size.w + adj_size(5 + 55 + 10);
  area.h = (O_LAST + 1) * (pText[0]->h + adj_size(6)) + adj_size(20);
  SDL_FillRectAlpha(pWindow->theme, &area, &bg_color);
  putframe(pWindow->theme, area.x, area.y, area.x + area.w - 1, area.y + area.h - 1,
           map_rgba(pWindow->theme->format, *get_game_colorRGB(COLOR_THEME_CMA_FRAME)));
  
  area.x = dst.x + text_w + adj_size(10);
  alphablit(pMinimal, NULL, pWindow->theme, &area);
  area.x += pMinimal->w + adj_size(10);
  FREESURFACE(pMinimal);
  
  alphablit(pFactor, NULL, pWindow->theme, &area);
  FREESURFACE(pFactor);
  
  area.x = pWindow->area.x + adj_size(22);
  area.y = pWindow->area.y + adj_size(31);
  alphablit(pCity_Map, NULL, pWindow->theme, &area);
  FREESURFACE(pCity_Map);
  
  output_type_iterate(i) {
    /* min label */
    pBuf = pBuf->prev;
    pBuf->size.x = pWindow->size.x + dst.x + text_w + adj_size(10);
    pBuf->size.y = pWindow->size.y + dst.y + (pText[i]->h - pBuf->size.h) / 2;
    
    /* min sb */
    pBuf = pBuf->prev;
    pBuf->size.x = pBuf->next->size.x + pBuf->next->size.w + adj_size(5);
    pBuf->size.y = pWindow->size.y + dst.y + (pText[i]->h - pBuf->size.h) / 2;
  
    area.x = pBuf->size.x - pWindow->size.x - adj_size(2);
    area.y = pBuf->size.y - pWindow->size.y;
    area.w = adj_size(74);
    area.h = pBuf->size.h;
    SDL_FillRectAlpha(pWindow->theme, &area, &bg_color);
    putframe(pWindow->theme, area.x, area.y, area.x + area.w - 1, area.y + area.h - 1,
             map_rgba(pWindow->theme->format, *get_game_colorRGB(COLOR_THEME_CMA_FRAME)));
    
    /* factor label */
    pBuf = pBuf->prev;
    pBuf->size.x = pBuf->next->size.x + adj_size(75);
    pBuf->size.y = pWindow->size.y + dst.y + (pText[i]->h - pBuf->size.h) / 2;
    
    /* factor sb */
    pBuf = pBuf->prev;
    pBuf->size.x = pBuf->next->size.x + pBuf->next->size.w + adj_size(5);
    pBuf->size.y = pWindow->size.y + dst.y + (pText[i]->h - pBuf->size.h) / 2;
    
    area.x = pBuf->size.x - pWindow->size.x - adj_size(2);
    area.y = pBuf->size.y - pWindow->size.y;
    area.w = adj_size(58);
    area.h = pBuf->size.h;
    SDL_FillRectAlpha(pWindow->theme, &area, &bg_color);
    putframe(pWindow->theme, area.x, area.y, area.x + area.w - 1, area.y + area.h - 1,
             map_rgba(pWindow->theme->format, *get_game_colorRGB(COLOR_THEME_CMA_FRAME)));
        
    alphablit(pText[i], NULL, pWindow->theme, &dst);
    dst.y += pText[i]->h + adj_size(6);
    FREESURFACE(pText[i]);
    
  } output_type_iterate_end;
  
  /* happy factor label */
  pBuf = pBuf->prev;
  pBuf->size.x = pBuf->next->next->size.x;
  pBuf->size.y = pWindow->size.y + dst.y + (pText[O_LAST]->h - pBuf->size.h) / 2;
  
  /* happy factor sb */
  pBuf = pBuf->prev;
  pBuf->size.x = pBuf->next->size.x + pBuf->next->size.w + adj_size(5);
  pBuf->size.y = pWindow->size.y + dst.y + (pText[O_LAST]->h - pBuf->size.h) / 2;
  
  area.x = pBuf->size.x - pWindow->size.x - adj_size(2);
  area.y = pBuf->size.y - pWindow->size.y;
  area.w = adj_size(58);
  area.h = pBuf->size.h;
  SDL_FillRectAlpha(pWindow->theme, &area, &bg_color);
  putframe(pWindow->theme, area.x, area.y, area.x + area.w - 1, area.y + area.h - 1,
           map_rgba(pWindow->theme->format, *get_game_colorRGB(COLOR_THEME_CMA_FRAME)));
  
  /* celebrate cbox */
  pBuf = pBuf->prev;
  pBuf->size.x = pWindow->size.x + dst.x + adj_size(10);
  pBuf->size.y = pWindow->size.y + dst.y;
  
  /* celebrate static text */
  dst.x += (adj_size(10) + pBuf->size.w + adj_size(5));
  dst.y += (pBuf->size.h - pText[O_LAST]->h) / 2;
  alphablit(pText[O_LAST], NULL, pWindow->theme, &dst);
  FREESURFACE(pText[O_LAST]);
  /* ------------------------ */
  
  /* save as */
  pBuf = pBuf->prev;
  pBuf->size.x = pWindow->size.x + x + (w - (pBuf->size.w + adj_size(6)) * 6) / 2;
  pBuf->size.y = pWindow->size.y + pWindow->size.h - pBuf->size.h * 2 - adj_size(10);
    
  area.x = x;
  area.y = pBuf->size.y - pWindow->size.y - adj_size(5);
  area.w = w;
  area.h = pBuf->size.h + adj_size(10);
  SDL_FillRectAlpha(pWindow->theme, &area, &bg_color);
  putframe(pWindow->theme, area.x, area.y, area.x + area.w - 1, area.y + area.h - 1,
           map_rgba(pWindow->theme->format, *get_game_colorRGB(COLOR_THEME_CMA_FRAME)));
  
  /* load */
  pBuf = pBuf->prev;
  pBuf->size.x = pBuf->next->size.x + pBuf->next->size.w + adj_size(4);
  pBuf->size.y = pBuf->next->size.y;
  
  /* del */
  pBuf = pBuf->prev;
  pBuf->size.x = pBuf->next->size.x + pBuf->next->size.w + adj_size(4);
  pBuf->size.y = pBuf->next->size.y;
  
  /* run */
  pBuf = pBuf->prev;
  pBuf->size.x = pBuf->next->size.x + pBuf->next->size.w + adj_size(4);
  pBuf->size.y = pBuf->next->size.y;
  
  /* run one time */
  pBuf = pBuf->prev;
  pBuf->size.x = pBuf->next->size.x + pBuf->next->size.w + adj_size(4);
  pBuf->size.y = pBuf->next->size.y;
  
  /* del */
  pBuf = pBuf->prev;
  pBuf->size.x = pBuf->next->size.x + pBuf->next->size.w + adj_size(4);
  pBuf->size.y = pBuf->next->size.y;
  
  /* ------------------------ */
  /* check if Citizen Icons style was loaded */
  cs = style_of_city(pCma->pCity);

  if (cs != pIcons->style) {
    reload_citizens_icons(cs);
  }
  
  set_cma_hscrollbars();
  update_city_cma_dialog();
}

void popdown_city_cma_dialog(void)
{
  if(pCma) {
    popdown_window_group_dialog(pCma->pDlg->pBeginWidgetList,
				pCma->pDlg->pEndWidgetList);
    FC_FREE(pCma->pDlg);
    if(pCma->pAdv) {
      del_group_of_widgets_from_gui_list(pCma->pAdv->pBeginWidgetList,
						pCma->pAdv->pEndWidgetList);
      FC_FREE(pCma->pAdv->pScroll);
      FC_FREE(pCma->pAdv);
    }
    if(city_dialog_is_open(pCma->pCity)) {
      /* enable city dlg */
      enable_city_dlg_widgets();
      refresh_city_dialog(pCma->pCity);
    }
    city_report_dialog_update();
    FC_FREE(pCma);
  }
}
