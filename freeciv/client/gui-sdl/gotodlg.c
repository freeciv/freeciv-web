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

#include <stdlib.h>

#include "SDL.h"

/* utility */
#include "fcintl.h"
#include "log.h"

/* common */
#include "game.h"
#include "unitlist.h"

/* client */
#include "client_main.h"
#include "control.h"
#include "goto.h"

/* gui-sdl */
#include "colors.h"
#include "graphics.h"
#include "gui_id.h"
#include "gui_main.h"
#include "gui_tilespec.h"
#include "mapview.h"
#include "sprite.h"
#include "widget.h"

#include "gotodlg.h"

static struct ADVANCED_DLG *pGotoDlg = NULL;
static Uint32 all_players = 0;
static bool GOTO = TRUE;

static void update_goto_dialog(void);

static int goto_dialog_window_callback(struct widget *pWindow)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    move_window_group(pGotoDlg->pBeginWidgetList, pWindow);
  }
  return -1;
}

static int exit_goto_dialog_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    popdown_goto_airlift_dialog();
    flush_dirty();
  }
  return -1;
}

static int toggle_goto_nations_cities_dialog_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    all_players ^= (1u << player_index(player_by_number(MAX_ID - pWidget->ID)));
    update_goto_dialog();
  }
  return -1;
}

static int goto_city_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct city *pDestcity = game_find_city_by_number(MAX_ID - pWidget->ID);
  
    if (pDestcity) {
      struct unit *pUnit = head_of_units_in_focus();
      if (pUnit) {
        if(GOTO) {
          send_goto_tile(pUnit, pDestcity->tile);
        } else {
          request_unit_airlift(pUnit, pDestcity);
        }
      }
    }
    
    popdown_goto_airlift_dialog();
    flush_dirty();
  }
  return -1;
}

/**************************************************************************
...
**************************************************************************/
static void update_goto_dialog(void)
{
  struct widget *pBuf = NULL, *pAdd_Dock, *pLast;
  SDL_Surface *pLogo = NULL;
  SDL_String16 *pStr;
  char cBuf[128]; 
  int n = 0;  
  struct player *owner = NULL;
  
  if(pGotoDlg->pEndActiveWidgetList) {
    pAdd_Dock = pGotoDlg->pEndActiveWidgetList->next;
    pGotoDlg->pBeginWidgetList = pAdd_Dock;
    del_group(pGotoDlg->pBeginActiveWidgetList, pGotoDlg->pEndActiveWidgetList);
    pGotoDlg->pActiveWidgetList = NULL;
  } else {
    pAdd_Dock = pGotoDlg->pBeginWidgetList;
  }
  
  pLast = pAdd_Dock;
  
  players_iterate(pPlayer) {
    
    if (!TEST_BIT(all_players, player_index(pPlayer))) {
      continue;
    }

    city_list_iterate(pPlayer->cities, pCity) {
      
      /* FIXME: should use unit_can_airlift_to(). */
      if (!GOTO && !pCity->airlift) {
	continue;
      }
      
      my_snprintf(cBuf, sizeof(cBuf), "%s (%d)", city_name(pCity), pCity->size);
      
      pStr = create_str16_from_char(cBuf, adj_font(12));
      pStr->style |= TTF_STYLE_BOLD;
   
      if(!player_owns_city(owner, pCity)) {
        pLogo = get_nation_flag_surface(nation_of_player(city_owner(pCity)));
        pLogo = crop_visible_part_from_surface(pLogo);
      }
      
      pBuf = create_iconlabel(pLogo, pGotoDlg->pEndWidgetList->dst, pStr, 
    	(WF_RESTORE_BACKGROUND|WF_DRAW_TEXT_LABEL_WITH_SPACE));
    
      if(!player_owns_city(owner, pCity)) {
        set_wflag(pBuf, WF_FREE_THEME);
        owner = city_owner(pCity);
      }
      
      pBuf->string16->fgcol =
	    *(get_player_color(tileset, city_owner(pCity))->color);	  	  
      pBuf->action = goto_city_callback;
      
      if(GOTO || pCity->airlift) {
        set_wstate(pBuf, FC_WS_NORMAL);
      }
      
      assert((MAX_ID - pCity->id) > 0);
      pBuf->ID = MAX_ID - pCity->id;
      
      DownAdd(pBuf, pAdd_Dock);
      pAdd_Dock = pBuf;
      
      if (n > (pGotoDlg->pScroll->active - 1))
      {
        set_wflag(pBuf, WF_HIDDEN);
      }
      
      n++;  
    } city_list_iterate_end;
  } players_iterate_end;

  if (n > 0) {
    pGotoDlg->pBeginWidgetList = pBuf;
    
    pGotoDlg->pBeginActiveWidgetList = pGotoDlg->pBeginWidgetList;
    pGotoDlg->pEndActiveWidgetList = pLast->prev;
    pGotoDlg->pActiveWidgetList = pGotoDlg->pEndActiveWidgetList;
    pGotoDlg->pScroll->count = n;
    
    if (n > pGotoDlg->pScroll->active)
    {
      show_scrollbar(pGotoDlg->pScroll);
      pGotoDlg->pScroll->pScrollBar->size.y = pGotoDlg->pEndWidgetList->area.y +
        pGotoDlg->pScroll->pUp_Left_Button->size.h;
      pGotoDlg->pScroll->pScrollBar->size.h = scrollbar_size(pGotoDlg->pScroll);
    } else {
      hide_scrollbar(pGotoDlg->pScroll);
    }
  
    setup_vertical_widgets_position(1,
	pGotoDlg->pEndWidgetList->area.x,
        pGotoDlg->pEndWidgetList->area.y,
        pGotoDlg->pScroll->pUp_Left_Button->size.x -
          pGotoDlg->pEndWidgetList->area.x - adj_size(2),
        0, pGotoDlg->pBeginActiveWidgetList, pGotoDlg->pEndActiveWidgetList);
        
  } else {
    hide_scrollbar(pGotoDlg->pScroll);
  }
  
  /* redraw */
  redraw_group(pGotoDlg->pBeginWidgetList, pGotoDlg->pEndWidgetList, 0);
  widget_flush(pGotoDlg->pEndWidgetList);
  
}


/**************************************************************************
  Popup a dialog to have the focus unit goto to a city.
**************************************************************************/
static void popup_goto_airlift_dialog(void)
{
  SDL_Color bg_color = {0, 0, 0, 96};
  
  struct widget *pBuf, *pWindow;
  SDL_String16 *pStr;
  SDL_Surface *pFlag, *pEnabled, *pDisabled;
  SDL_Rect dst;
  int i, col, block_x, x, y;
  SDL_Rect area;
  
  if (pGotoDlg) {
    return;
  }
  
  pGotoDlg = fc_calloc(1, sizeof(struct ADVANCED_DLG));
    
  pStr = create_str16_from_char(_("Select destination"), adj_font(12));
  pStr->style |= TTF_STYLE_BOLD;
  
  pWindow = create_window_skeleton(NULL, pStr, 0);
  
  pWindow->action = goto_dialog_window_callback;
  set_wstate(pWindow, FC_WS_NORMAL);
  
  add_to_gui_list(ID_WINDOW, pWindow);
  pGotoDlg->pEndWidgetList = pWindow;

  area = pWindow->area;
  
  /* ---------- */
  /* create exit button */
  pBuf = create_themeicon(pTheme->Small_CANCEL_Icon, pWindow->dst,
  			  WF_WIDGET_HAS_INFO_LABEL | WF_RESTORE_BACKGROUND);
  pBuf->string16 = create_str16_from_char(_("Close Dialog (Esc)"), adj_font(12));
  pBuf->action = exit_goto_dialog_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->key = SDLK_ESCAPE;
  area.w = MAX(area.w, pBuf->size.w) + adj_size(10);
  
  add_to_gui_list(ID_BUTTON, pBuf);
  
  col = 0;
  /* --------------------------------------------- */
  players_iterate(pPlayer) {
    if (pPlayer != client.conn.playing
      && DS_NO_CONTACT == pplayer_get_diplstate(client.conn.playing, pPlayer)->type) {
      continue;
    }
    
    pFlag = ResizeSurfaceBox(get_nation_flag_surface(pPlayer->nation),
                             adj_size(30), adj_size(30), 1, TRUE, FALSE);
  
    pEnabled = create_icon_theme_surf(pFlag);
    SDL_FillRectAlpha(pFlag, NULL, &bg_color);
    pDisabled = create_icon_theme_surf(pFlag);
    FREESURFACE(pFlag);
    
    pBuf = create_checkbox(pWindow->dst,
      TEST_BIT(all_players, player_index(pPlayer)),
    	(WF_FREE_STRING|WF_FREE_THEME|WF_RESTORE_BACKGROUND|WF_WIDGET_HAS_INFO_LABEL));
    set_new_checkbox_theme(pBuf, pEnabled, pDisabled);
    
    pBuf->string16 = create_str16_from_char(
    			nation_adjective_for_player(pPlayer),
    			adj_font(12));
    pBuf->string16->style &= ~SF_CENTER;
    set_wstate(pBuf, FC_WS_NORMAL);
    
    pBuf->action = toggle_goto_nations_cities_dialog_callback;
    add_to_gui_list(MAX_ID - player_number(pPlayer), pBuf);
    col++;  
  } players_iterate_end;
    
  pGotoDlg->pBeginWidgetList = pBuf;

  create_vertical_scrollbar(pGotoDlg, 1, 16, TRUE, TRUE);
  hide_scrollbar(pGotoDlg->pScroll);
  
  area.w = MAX(area.w, adj_size(300));
  area.h = adj_size(320);

  resize_window(pWindow, NULL, NULL,
                (pWindow->size.w - pWindow->area.w) + area.w,
                (pWindow->size.h - pWindow->area.h) + area.h);

  /* background */
  col = (col + 15) / 16; /* number of flag columns */
  
  pFlag = ResizeSurface(pTheme->Block,
    (col * pBuf->size.w + (col - 1) * adj_size(5) + adj_size(10)), area.h, 1);
  
  block_x = dst.x = area.x + area.w - pFlag->w;
  dst.y = area.y;
  alphablit(pFlag, NULL, pWindow->theme, &dst);
  FREESURFACE(pFlag);

  widget_set_position(pWindow,
                      (Main.screen->w - pWindow->size.w) / 2,
                      (Main.screen->h - pWindow->size.h) / 2);

  /* exit button */
  pBuf = pWindow->prev;
  pBuf->size.x = area.x + area.w - pBuf->size.w - 1;
  pBuf->size.y = pWindow->size.y + adj_size(2);

  /* nations buttons */
  pBuf = pBuf->prev;
  i = 0;
  x = block_x + adj_size(5);
  y = area.y + adj_size(1);
  while(pBuf) {
    pBuf->size.x = x;
    pBuf->size.y = y;
       
    if(!((i + 1) % col)) {
      x = block_x + adj_size(5);
      y += pBuf->size.h + adj_size(1);
    } else {
      x += pBuf->size.w + adj_size(5);
    }
    
    if(pBuf == pGotoDlg->pBeginWidgetList) {
      break;
    }
    
    i++;
    pBuf = pBuf->prev;
  }

  setup_vertical_scrollbar_area(pGotoDlg->pScroll,
	                        block_x, area.y,
  	                        area.h, TRUE);
    
  update_goto_dialog();
}


/**************************************************************************
  Popup a dialog to have the focus unit goto to a city.
**************************************************************************/
void popup_goto_dialog(void)
{
  if (!can_client_issue_orders() || 0 == get_num_units_in_focus()) {
    return;
  }
  all_players = (1u << (player_index(client.conn.playing)));
  popup_goto_airlift_dialog();
}

/**************************************************************************
  Popup a dialog to have the focus unit airlift to a city.
**************************************************************************/
void popup_airlift_dialog(void)
{
  if (!can_client_issue_orders() || 0 == get_num_units_in_focus()) {
    return;
  }
  all_players = (1u << (player_index(client.conn.playing)));
  GOTO = FALSE;
  popup_goto_airlift_dialog();
}

/**************************************************************************
  Popdown goto/airlift to a city dialog.
**************************************************************************/
void popdown_goto_airlift_dialog(void)
{
 if(pGotoDlg) {
    popdown_window_group_dialog(pGotoDlg->pBeginWidgetList,
					    pGotoDlg->pEndWidgetList);
    FC_FREE(pGotoDlg->pScroll);
    FC_FREE(pGotoDlg);
  }
  GOTO = TRUE;
}
