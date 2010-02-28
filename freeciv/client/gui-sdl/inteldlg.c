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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "SDL.h"

/* utility */
#include "fcintl.h"
#include "log.h"

/* common */
#include "government.h"

/* client */
#include "client_main.h"

/* gui-sdl */
#include "graphics.h"
#include "gui_id.h"
#include "gui_main.h"
#include "gui_tilespec.h"
#include "mapview.h"
#include "repodlgs.h"
#include "spaceshipdlg.h"
#include "sprite.h"
#include "widget.h"

#include "inteldlg.h"

struct intel_dialog {
  struct player *pplayer;
  struct ADVANCED_DLG *pdialog;
  int pos_x, pos_y;
};

#define SPECLIST_TAG dialog
#define SPECLIST_TYPE struct intel_dialog
#include "speclist.h"

#define dialog_list_iterate(dialoglist, pdialog) \
    TYPED_LIST_ITERATE(struct intel_dialog, dialoglist, pdialog)
#define dialog_list_iterate_end  LIST_ITERATE_END

static struct dialog_list *dialog_list;
static struct intel_dialog *create_intel_dialog(struct player *p);

/****************************************************************
...
*****************************************************************/
void intel_dialog_init()
{
  dialog_list = dialog_list_new();
}

/****************************************************************
...
*****************************************************************/
void intel_dialog_done()
{
  dialog_list_free(dialog_list);
}

/****************************************************************
...
*****************************************************************/
static struct intel_dialog *get_intel_dialog(struct player *pplayer)
{
  dialog_list_iterate(dialog_list, pdialog) {
    if (pdialog->pplayer == pplayer) {
      return pdialog;
    }
  } dialog_list_iterate_end;

  return NULL;
}

/****************************************************************
...
*****************************************************************/
static int intel_window_dlg_callback(struct widget *pWindow)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct intel_dialog *pSelectedDialog = get_intel_dialog(pWindow->data.player);
  
    move_window_group(pSelectedDialog->pdialog->pBeginWidgetList, pWindow);
  }
  return -1;
}

static int tech_callback(struct widget *pWidget)
{
  /* get tech help - PORT ME */
  return -1;
}

static int spaceship_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct player *pPlayer = pWidget->data.player;
    popdown_intel_dialog(pPlayer);
    popup_spaceship_dialog(pPlayer);
  }
  return -1;
}


static int exit_intel_dlg_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    popdown_intel_dialog(pWidget->data.player);
    flush_dirty();
  }
  return -1;
}

static struct intel_dialog *create_intel_dialog(struct player *pPlayer) {

  struct intel_dialog *pdialog = fc_calloc(1, sizeof(struct intel_dialog));

  pdialog->pplayer = pPlayer;
  
  pdialog->pdialog = fc_calloc(1, sizeof(struct ADVANCED_DLG));
      
  pdialog->pos_x = 0;
  pdialog->pos_y = 0;
    
  dialog_list_prepend(dialog_list, pdialog);  
  
  return pdialog;
}


/**************************************************************************
  Popup an intelligence dialog for the given player.
**************************************************************************/
void popup_intel_dialog(struct player *p)
{
  struct intel_dialog *pdialog;

  if (!(pdialog = get_intel_dialog(p))) {
    pdialog = create_intel_dialog(p);
  } else {
    /* bring existing dialog to front */
    sellect_window_group_dialog(pdialog->pdialog->pBeginWidgetList,
                                         pdialog->pdialog->pEndWidgetList);
  }

  update_intel_dialog(p);
}

/**************************************************************************
  Popdown an intelligence dialog for the given player.
**************************************************************************/
void popdown_intel_dialog(struct player *p)
{
  struct intel_dialog *pdialog = get_intel_dialog(p);
    
  if (pdialog) {
    popdown_window_group_dialog(pdialog->pdialog->pBeginWidgetList,
			                  pdialog->pdialog->pEndWidgetList);
      
    dialog_list_unlink(dialog_list, pdialog);
      
    FC_FREE(pdialog->pdialog->pScroll);
    FC_FREE(pdialog->pdialog);  
    FC_FREE(pdialog);
  }
}

/**************************************************************************
  Popdown all intelligence dialogs
**************************************************************************/
void popdown_intel_dialogs() {
  dialog_list_iterate(dialog_list, pdialog) {
    popdown_intel_dialog(pdialog->pplayer);
  } dialog_list_iterate_end;
}

/****************************************************************************
  Update the intelligence dialog for the given player.  This is called by
  the core client code when that player's information changes.
****************************************************************************/
void update_intel_dialog(struct player *p)
{
  struct intel_dialog *pdialog = get_intel_dialog(p);
      
  struct widget *pWindow = NULL, *pBuf = NULL, *pLast;
  SDL_Surface *pLogo = NULL, *pTmpSurf = NULL;
  SDL_Surface *pText1, *pInfo, *pText2 = NULL;
  SDL_String16 *pStr;
  SDL_Rect dst;
  char cBuf[256];
  int n = 0, count = 0, col;
  struct city *pCapital;
  SDL_Rect area;
  struct player_research* research;
      
  if (pdialog) {

    /* save window position and delete old content */
    if (pdialog->pdialog->pEndWidgetList) {
      pdialog->pos_x = pdialog->pdialog->pEndWidgetList->size.x;
      pdialog->pos_y = pdialog->pdialog->pEndWidgetList->size.y;
        
      popdown_window_group_dialog(pdialog->pdialog->pBeginWidgetList,
                                            pdialog->pdialog->pEndWidgetList);
    }
        
    pStr = create_str16_from_char(_("Foreign Intelligence Report") , adj_font(12));
    pStr->style |= TTF_STYLE_BOLD;
    
    pWindow = create_window_skeleton(NULL, pStr, 0);
      
    pWindow->action = intel_window_dlg_callback;
    set_wstate(pWindow , FC_WS_NORMAL);
    pWindow->data.player = p;
    
    add_to_gui_list(ID_WINDOW, pWindow);
    pdialog->pdialog->pEndWidgetList = pWindow;
    
    area = pWindow->area;
    
    /* ---------- */
    /* exit button */
    pBuf = create_themeicon(pTheme->Small_CANCEL_Icon, pWindow->dst,
                            WF_WIDGET_HAS_INFO_LABEL | WF_RESTORE_BACKGROUND);
    pBuf->string16 = create_str16_from_char(_("Close Dialog (Esc)"), adj_font(12));
    area.w = MAX(area.w, pBuf->size.w + adj_size(10));
    pBuf->action = exit_intel_dlg_callback;
    set_wstate(pBuf, FC_WS_NORMAL);
    pBuf->data.player = p;  
    pBuf->key = SDLK_ESCAPE;
    
    add_to_gui_list(ID_BUTTON, pBuf);
    /* ---------- */
    
    pLogo = get_nation_flag_surface(nation_of_player(p));
    pText1 = zoomSurface(pLogo, DEFAULT_ZOOM * 4.0 , DEFAULT_ZOOM * 4.0, 1);
    pLogo = pText1;
          
    pBuf = create_icon2(pLogo, pWindow->dst,
          (WF_RESTORE_BACKGROUND|WF_WIDGET_HAS_INFO_LABEL|
                                          WF_FREE_STRING|WF_FREE_THEME));
    pBuf->action = spaceship_callback;
    set_wstate(pBuf, FC_WS_NORMAL);
    pBuf->data.player = p;
    my_snprintf(cBuf, sizeof(cBuf),
                _("Intelligence Information about the %s Spaceship"), 
                nation_adjective_for_player(p));
    pBuf->string16 = create_str16_from_char(cBuf, adj_font(12));
          
    add_to_gui_list(ID_ICON, pBuf);
          
    /* ---------- */
    my_snprintf(cBuf, sizeof(cBuf),
                _("Intelligence Information for the %s Empire"), 
                nation_adjective_for_player(p));
    
    pStr = create_str16_from_char(cBuf, adj_font(14));
    pStr->style |= TTF_STYLE_BOLD;
    pStr->bgcol = (SDL_Color) {0, 0, 0, 0};
    
    pText1 = create_text_surf_from_str16(pStr);
    area.w = MAX(area.w, pText1->w + adj_size(20));
    area.h += pText1->h + adj_size(20);
      
    /* ---------- */
    
    pCapital = find_palace(p);
    research = get_player_research(p);
    change_ptsize16(pStr, adj_font(10));
    pStr->style &= ~TTF_STYLE_BOLD;

    /* FIXME: these should use common gui code, and avoid duplication! */
    switch (research->researching) {
    case A_UNKNOWN:
    case A_UNSET:
      my_snprintf(cBuf, sizeof(cBuf),
        _("Ruler: %s %s  Government: %s\nCapital: %s  Gold: %d\nTax: %d%%"
          " Science: %d%% Luxury: %d%%\nResearching: unknown"),
        ruler_title_translation(p),
        player_name(p),
        government_name_for_player(p),
        /* TRANS: "unknown" location */
        (!pCapital) ? _("(unknown)") : city_name(pCapital),
        p->economic.gold,
        p->economic.tax, p->economic.science, p->economic.luxury);
      break;
    default:
      my_snprintf(cBuf, sizeof(cBuf),
        _("Ruler: %s %s  Government: %s\nCapital: %s  Gold: %d\nTax: %d%%"
          " Science: %d%% Luxury: %d%%\nResearching: %s(%d/%d)"),
        ruler_title_translation(p),
        player_name(p),
        government_name_for_player(p),
        /* TRANS: "unknown" location */
        (!pCapital) ? _("(unknown)") : city_name(pCapital),
        p->economic.gold,
        p->economic.tax, p->economic.science, p->economic.luxury,
        advance_name_researching(p),
        research->bulbs_researched, total_bulbs_required(p));
      break;
    };
    
    copy_chars_to_string16(pStr, cBuf);
    pInfo = create_text_surf_from_str16(pStr);
    area.w = MAX(area.w, pLogo->w + adj_size(10) + pInfo->w + adj_size(20));
    area.h += MAX(pLogo->h + adj_size(20), pInfo->h + adj_size(20));
      
    /* ---------- */
    pTmpSurf = get_tech_icon(A_FIRST);
    col = area.w / (pTmpSurf->w + adj_size(4));
    FREESURFACE(pTmpSurf);
    n = 0;
    pLast = pBuf;
    advance_index_iterate(A_FIRST, i) {
      if (TECH_KNOWN == player_invention_state(p, i)
       && player_invention_reachable(client.conn.playing, i)
       && TECH_KNOWN != player_invention_state(client.conn.playing, i)) {

        pBuf = create_icon2(get_tech_icon(i), pWindow->dst,
          (WF_RESTORE_BACKGROUND|WF_WIDGET_HAS_INFO_LABEL|WF_FREE_STRING | WF_FREE_THEME));
        pBuf->action = tech_callback;
        set_wstate(pBuf, FC_WS_NORMAL);
  
        pBuf->string16 = create_str16_from_char(
                           advance_name_translation(advance_by_number(i)),
                           adj_font(12));
          
        add_to_gui_list(ID_ICON, pBuf);
          
        if(n > ((2 * col) - 1)) {
          set_wflag(pBuf, WF_HIDDEN);
        }
        
        n++;	
      }
    }  advance_index_iterate_end;
    
    pdialog->pdialog->pBeginWidgetList = pBuf;
    
    if (n > 0) {
      pdialog->pdialog->pEndActiveWidgetList = pLast->prev;
      pdialog->pdialog->pBeginActiveWidgetList = pdialog->pdialog->pBeginWidgetList;
      if(n > 2 * col) {
        pdialog->pdialog->pActiveWidgetList = pdialog->pdialog->pEndActiveWidgetList;
        count = create_vertical_scrollbar(pdialog->pdialog, col, 2, TRUE, TRUE);
        area.h += (2 * pBuf->size.h + adj_size(10));
      } else {
        count = 0;
        if(n > col) {
          area.h += pBuf->size.h;
        }
        area.h += (adj_size(10) + pBuf->size.h);
      }
      
      area.w = MAX(area.w, col * pBuf->size.w + count);
      
      my_snprintf(cBuf, sizeof(cBuf), _("Their techs that we don't have :"));
      copy_chars_to_string16(pStr, cBuf);
      pStr->style |= TTF_STYLE_BOLD;
      pText2 = create_text_surf_from_str16(pStr);
    }
    
    FREESTRING16(pStr);

    resize_window(pWindow, NULL, NULL,
                  (pWindow->size.w - pWindow->area.w) + area.w,
                  (pWindow->size.h - pWindow->area.h) + area.h);
    
    area = pWindow->area;
    
    /* ------------------------ */  
    widget_set_position(pWindow,
      (pdialog->pos_x) ? (pdialog->pos_x) : ((Main.screen->w - pWindow->size.w) / 2),
      (pdialog->pos_y) ? (pdialog->pos_y) : ((Main.screen->h - pWindow->size.h) / 2));
    
    /* exit button */
    pBuf = pWindow->prev; 
    pBuf->size.x = area.x + area.w - pBuf->size.w - 1;
    pBuf->size.y = pWindow->size.y + adj_size(2);
    
    dst.x = area.x + (area.w - pText1->w) / 2;
    dst.y = area.y + adj_size(8);
    
    alphablit(pText1, NULL, pWindow->theme, &dst);
    dst.y += pText1->h + adj_size(10);
    FREESURFACE(pText1);
    
    /* space ship button */
    pBuf = pBuf->prev;
    dst.x = area.x + (area.w - (pBuf->size.w + adj_size(10) + pInfo->w)) / 2;
    pBuf->size.x = dst.x;
    pBuf->size.y = dst.y;
    
    dst.x += pBuf->size.w + adj_size(10);  
    alphablit(pInfo, NULL, pWindow->theme, &dst);
    dst.y += pInfo->h + adj_size(10);
    FREESURFACE(pInfo);
        
    /* --------------------- */
      
    if(n) {
      
      dst.x = area.x + adj_size(5);
      alphablit(pText2, NULL, pWindow->theme, &dst);
      dst.y += pText2->h + adj_size(2);
      FREESURFACE(pText2);
      
      setup_vertical_widgets_position(col,
          area.x, dst.y,
            0, 0, pdialog->pdialog->pBeginActiveWidgetList,
                            pdialog->pdialog->pEndActiveWidgetList);
      
      if(pdialog->pdialog->pScroll) {
        setup_vertical_scrollbar_area(pdialog->pdialog->pScroll,
          area.x + area.w, dst.y,
          area.h - (dst.y + 1), TRUE);
      }
    }

    redraw_group(pdialog->pdialog->pBeginWidgetList, pdialog->pdialog->pEndWidgetList, 0);
    widget_mark_dirty(pWindow);
  
    flush_dirty();
    
  }
}
