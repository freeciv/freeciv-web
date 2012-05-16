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

/* gui-sdl */
#include "colors.h"
#include "graphics.h"
#include "gui_id.h"
#include "gui_main.h"
#include "gui_tilespec.h"
#include "mapctrl.h"
#include "mapview.h"
#include "sprite.h"
#include "widget.h"

#include "finddlg.h"

/* ====================================================================== */
/* ============================= FIND CITY MENU ========================= */
/* ====================================================================== */
static struct ADVANCED_DLG  *pFind_City_Dlg = NULL;

static int find_city_window_dlg_callback(struct widget *pWindow)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    move_window_group(pFind_City_Dlg->pBeginWidgetList, pWindow);
  }
  return -1;
}

static int exit_find_city_dlg_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    int orginal_x = pWidget->data.cont->id0;
    int orginal_y = pWidget->data.cont->id1;
        
    popdown_find_dialog();
    
    center_tile_mapcanvas(map_pos_to_tile(orginal_x, orginal_y));
    
    flush_dirty();
  }
  return -1;
}

static int find_city_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct city *pCity = pWidget->data.city;
    if(pCity) {
      center_tile_mapcanvas(pCity->tile);
      if(Main.event.button.button == SDL_BUTTON_RIGHT) {
        popdown_find_dialog();
      }
      flush_dirty();
    }
  }
  return -1;
}

/**************************************************************************
  Popdown a dialog to ask for a city to find.
**************************************************************************/
void popdown_find_dialog(void)
{
  if (pFind_City_Dlg) {
    popdown_window_group_dialog(pFind_City_Dlg->pBeginWidgetList,
			pFind_City_Dlg->pEndWidgetList);
    FC_FREE(pFind_City_Dlg->pScroll);
    FC_FREE(pFind_City_Dlg);
    enable_and_redraw_find_city_button();
  }
}

/**************************************************************************
  Popup a dialog to ask for a city to find.
**************************************************************************/
void popup_find_dialog(void)
{
  struct widget *pWindow = NULL, *pBuf = NULL;
  SDL_Surface *pLogo = NULL;
  SDL_String16 *pStr;
  char cBuf[128]; 
  int h = 0, n = 0, w = 0, units_h = 0;
  struct player *owner = NULL;
  struct tile *original;
  int window_x = 0, window_y = 0;
  bool mouse = (Main.event.type == SDL_MOUSEBUTTONDOWN);
  SDL_Rect area;
  
  /* check that there are any cities to find */
  players_iterate(pplayer) {
    h = city_list_size(pplayer->cities);
    if (h > 0) {
      break;
    }
  } players_iterate_end;
  
  if (pFind_City_Dlg && !h) {
    return;
  }
     
  original = canvas_pos_to_tile(Main.map->w/2, Main.map->h/2);
  
  pFind_City_Dlg = fc_calloc(1, sizeof(struct ADVANCED_DLG));
  
  pStr = create_str16_from_char(_("Find City") , adj_font(12));
  pStr->style |= TTF_STYLE_BOLD;
  
  pWindow = create_window_skeleton(NULL, pStr, 0);
    
  pWindow->action = find_city_window_dlg_callback;
  set_wstate(pWindow , FC_WS_NORMAL);
  
  add_to_gui_list(ID_TERRAIN_ADV_DLG_WINDOW, pWindow);
  pFind_City_Dlg->pEndWidgetList = pWindow;
  
  area = pWindow->area;
  
  /* ---------- */
  /* exit button */
  pBuf = create_themeicon(pTheme->Small_CANCEL_Icon, pWindow->dst,
  			  (WF_WIDGET_HAS_INFO_LABEL | WF_RESTORE_BACKGROUND | WF_FREE_DATA));
  pBuf->string16 = create_str16_from_char(_("Close Dialog (Esc)"), adj_font(12));
  area.w = MAX(area.w, pBuf->size.w + adj_size(10));
  pBuf->action = exit_find_city_dlg_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->key = SDLK_ESCAPE;
  pBuf->data.cont = fc_calloc(1, sizeof(struct CONTAINER));
  pBuf->data.cont->id0 = original->x;
  pBuf->data.cont->id1 = original->y;

  add_to_gui_list(ID_TERRAIN_ADV_DLG_EXIT_BUTTON, pBuf);
  /* ---------- */
  
  players_iterate(pPlayer) {
    city_list_iterate(pPlayer->cities, pCity) {
    
      my_snprintf(cBuf , sizeof(cBuf), "%s (%d)", city_name(pCity), pCity->size);
      
      pStr = create_str16_from_char(cBuf , adj_font(10));
      pStr->style |= (TTF_STYLE_BOLD|SF_CENTER);
   
      if(!player_owns_city(owner, pCity)) {
        pLogo = get_nation_flag_surface(nation_of_player(city_owner(pCity)));
        pLogo = crop_visible_part_from_surface(pLogo);
      }
      
      pBuf = create_iconlabel(pLogo, pWindow->dst, pStr, 
    	(WF_RESTORE_BACKGROUND|WF_DRAW_TEXT_LABEL_WITH_SPACE));
      
      if(!player_owns_city(owner, pCity)) {
        set_wflag(pBuf, WF_FREE_THEME);
        owner = city_owner(pCity);
      }
      
      pBuf->string16->style &= ~SF_CENTER;
      pBuf->string16->fgcol =
	    *(get_player_color(tileset, city_owner(pCity))->color);	  
      pBuf->string16->bgcol = (SDL_Color) {0, 0, 0, 0};
    
      pBuf->data.city = pCity;
  
      pBuf->action = find_city_callback;
      set_wstate(pBuf, FC_WS_NORMAL);
  
      add_to_gui_list(ID_LABEL , pBuf);
    
      area.w = MAX(area.w , pBuf->size.w);
      area.h += pBuf->size.h;
    
      if (n > 19)
      {
        set_wflag(pBuf , WF_HIDDEN);
      }
      
      n++;  
    } city_list_iterate_end;
  } players_iterate_end;
  pFind_City_Dlg->pBeginWidgetList = pBuf;
  pFind_City_Dlg->pBeginActiveWidgetList = pFind_City_Dlg->pBeginWidgetList;
  pFind_City_Dlg->pEndActiveWidgetList = pWindow->prev->prev;
  pFind_City_Dlg->pActiveWidgetList = pFind_City_Dlg->pEndActiveWidgetList;
  
  
  /* ---------- */
  if (n > 20)
  {
     
    units_h = create_vertical_scrollbar(pFind_City_Dlg, 1, 20, TRUE, TRUE);
    pFind_City_Dlg->pScroll->count = n;
    
    n = units_h;
    area.w += n;
    
    units_h = 20 * pBuf->size.h + adj_size(2);
    
  } else {
    units_h = area.h;
  }
        
  /* ---------- */
  
  area.h = units_h;

  resize_window(pWindow , NULL, NULL,
                (pWindow->size.w - pWindow->area.w) + area.w,
                (pWindow->size.h - pWindow->area.h) + area.h);
  
  area = pWindow->area;
  
  if(!mouse) {  
    window_x = adj_size(10);
    window_y = (Main.screen->h - pWindow->size.h) / 2;
  } else {
    window_x = (Main.event.motion.x + pWindow->size.w + adj_size(10) < Main.screen->w) ?
                (Main.event.motion.x + adj_size(10)) :
                (Main.screen->w - pWindow->size.w - adj_size(10));
    window_y = (Main.event.motion.y - adj_size(2) + pWindow->size.h < Main.screen->h) ?
             (Main.event.motion.y - adj_size(2)) :
             (Main.screen->h - pWindow->size.h - adj_size(10));
    
  }

  widget_set_position(pWindow, window_x, window_y);
  
  w = area.w;
  
  if (pFind_City_Dlg->pScroll)
  {
    w -= n;
  }
  
  /* exit button */
  pBuf = pWindow->prev;
  
  pBuf->size.x = area.x + area.w - pBuf->size.w - 1;
  pBuf->size.y = pWindow->size.y + adj_size(2);
  
  /* cities */
  pBuf = pBuf->prev;
  setup_vertical_widgets_position(1,
	area.x, area.y,
	w, 0, pFind_City_Dlg->pBeginActiveWidgetList, pBuf);
  
  if (pFind_City_Dlg->pScroll)
  {
    setup_vertical_scrollbar_area(pFind_City_Dlg->pScroll,
	area.x + area.w, area.y,
    	area.h, TRUE);
  }
  
  /* -------------------- */
  /* redraw */
  redraw_group(pFind_City_Dlg->pBeginWidgetList, pWindow, 0);
  widget_mark_dirty(pWindow);
  
  flush_dirty();
}
