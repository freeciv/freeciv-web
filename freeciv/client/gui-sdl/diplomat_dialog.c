/**********************************************************************
 Freeciv - Copyright (C) 1996-2006 - Freeciv Development Team
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

/* utility */
#include "fcintl.h"
#include "log.h"

/* common */
#include "game.h"
#include "unitlist.h"

/* client */
#include "client_main.h"
#include "control.h"

/* gui-sdl */
#include "colors.h"
#include "dialogs.h"
#include "graphics.h"
#include "gui_id.h"
#include "gui_tilespec.h"
#include "mapview.h"
#include "repodlgs.h"
#include "themespec.h"
#include "widget.h"

#include "dialogs_g.h"

struct diplomat_dialog {
  int diplomat_id;
  int diplomat_target_id;
  struct ADVANCED_DLG *pdialog;
};

struct small_diplomat_dialog {
  int diplomat_id;
  int diplomat_target_id;
  struct SMALL_DLG *pdialog;
};
 
extern bool is_unit_move_blocked;

void popdown_diplomat_dialog(void);
void popdown_incite_dialog(void);
void popdown_bribe_dialog(void);

/* ====================================================================== */
/* ============================ DIPLOMAT DIALOG ========================= */
/* ====================================================================== */
static struct diplomat_dialog *pDiplomat_Dlg = NULL;

/****************************************************************
...
*****************************************************************/
static int diplomat_dlg_window_callback(struct widget *pWindow)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    move_window_group(pDiplomat_Dlg->pdialog->pBeginWidgetList, pWindow);
  }
  return -1;
}

/****************************************************************
...
*****************************************************************/
static int diplomat_embassy_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (game_find_unit_by_number(pDiplomat_Dlg->diplomat_id)
       && game_find_city_by_number(pDiplomat_Dlg->diplomat_target_id)) {  
      request_diplomat_action(DIPLOMAT_EMBASSY, pDiplomat_Dlg->diplomat_id,
                                         pDiplomat_Dlg->diplomat_target_id, 0);
    }
  
    popdown_diplomat_dialog();  
  }
  return -1;
}

/****************************************************************
...
*****************************************************************/
static int diplomat_investigate_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (game_find_unit_by_number(pDiplomat_Dlg->diplomat_id)
       && game_find_city_by_number(pDiplomat_Dlg->diplomat_target_id)) {  
      request_diplomat_action(DIPLOMAT_INVESTIGATE, pDiplomat_Dlg->diplomat_id,
                                         pDiplomat_Dlg->diplomat_target_id, 0);
    }
  
    popdown_diplomat_dialog();
  }
  return -1;
}

/****************************************************************
...
*****************************************************************/
static int spy_poison_callback( struct widget *pWidget )
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (game_find_unit_by_number(pDiplomat_Dlg->diplomat_id)
       && game_find_city_by_number(pDiplomat_Dlg->diplomat_target_id)) {  
      request_diplomat_action(SPY_POISON, pDiplomat_Dlg->diplomat_id,
                                         pDiplomat_Dlg->diplomat_target_id, 0);
    }
  
    popdown_diplomat_dialog();
  }
  return -1;
}

/****************************************************************
 Requests up-to-date list of improvements, the return of
 which will trigger the popup_sabotage_dialog() function.
*****************************************************************/
static int spy_sabotage_request(struct widget *pWidget)
{
  if (game_find_unit_by_number(pDiplomat_Dlg->diplomat_id)
     && game_find_city_by_number(pDiplomat_Dlg->diplomat_target_id)) {  
    request_diplomat_answer(DIPLOMAT_SABOTAGE, pDiplomat_Dlg->diplomat_id,
                                       pDiplomat_Dlg->diplomat_target_id, 0);
  }

  popdown_diplomat_dialog();
  
  return -1;
}

/****************************************************************
...
*****************************************************************/
static int diplomat_sabotage_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (game_find_unit_by_number(pDiplomat_Dlg->diplomat_id)
       && game_find_city_by_number(pDiplomat_Dlg->diplomat_target_id)) {  
      request_diplomat_action(DIPLOMAT_SABOTAGE, pDiplomat_Dlg->diplomat_id,
                                         pDiplomat_Dlg->diplomat_target_id, 0);
    }
    
    popdown_diplomat_dialog();
  }
  return -1;
}
/* --------------------------------------------------------- */

static int spy_steal_dlg_window_callback(struct widget *pWindow)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    move_window_group(pDiplomat_Dlg->pdialog->pBeginWidgetList, pWindow);
  }
  return -1;
}

static int exit_spy_steal_dlg_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    popdown_diplomat_dialog();
  }
  return -1;  
}

static int spy_steal_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    int steal_advance = MAX_ID - pWidget->ID;
  
    if (game_find_unit_by_number(pDiplomat_Dlg->diplomat_id)
       && game_find_city_by_number(pDiplomat_Dlg->diplomat_target_id)) {  
      request_diplomat_action(DIPLOMAT_STEAL, pDiplomat_Dlg->diplomat_id,
                              pDiplomat_Dlg->diplomat_target_id, steal_advance);
    }
    
    popdown_diplomat_dialog();
  }
  return -1;
}

/**************************************************************************
  ...
**************************************************************************/
static int spy_steal_popup(struct widget *pWidget)
{
  struct city *pVcity = pWidget->data.city;
  int id = MAX_ID - pWidget->ID;
  struct player *pVictim = NULL;
  struct CONTAINER *pCont;
  struct widget *pBuf = NULL;
  struct widget *pWindow;
  SDL_String16 *pStr;
  SDL_Surface *pSurf;
  int max_col, max_row, col, i, count = 0;
  SDL_Rect area;

  popdown_diplomat_dialog();
  
  if(pVcity)
  {
    pVictim = city_owner(pVcity);
  }
  
  if (pDiplomat_Dlg || !pVictim) {
    return 1;
  }
  
  count = 0;
  advance_index_iterate(A_FIRST, i) {
    if (player_invention_reachable(client.conn.playing, i)
     && TECH_KNOWN == player_invention_state(pVictim, i)
     && (TECH_UNKNOWN == player_invention_state(client.conn.playing, i)
         || TECH_PREREQS_KNOWN ==
              player_invention_state(client.conn.playing, i))) {
      count++;
    }
  } advance_index_iterate_end;
  
  if(!count) {    
    /* if there is no known tech to steal then 
       send steal order at Spy's Discretion */
    int target_id = pVcity->id;

    request_diplomat_action(DIPLOMAT_STEAL, id, target_id, advance_count());
    return -1;
  }
    
  pCont = fc_calloc(1, sizeof(struct CONTAINER));
  pCont->id0 = pVcity->id;
  pCont->id1 = id;/* spy id */
  
  pDiplomat_Dlg = fc_calloc(1, sizeof(struct diplomat_dialog));
  pDiplomat_Dlg->diplomat_id = id;
  pDiplomat_Dlg->diplomat_target_id = pVcity->id;
  pDiplomat_Dlg->pdialog = fc_calloc(1, sizeof(struct ADVANCED_DLG));
      
  pStr = create_str16_from_char(_("Select Advance to Steal"), adj_font(12));
  pStr->style |= TTF_STYLE_BOLD;

  pWindow = create_window_skeleton(NULL, pStr, 0);
  
  pWindow->action = spy_steal_dlg_window_callback;
  set_wstate(pWindow , FC_WS_NORMAL);
  
  add_to_gui_list(ID_DIPLOMAT_DLG_WINDOW, pWindow);
  pDiplomat_Dlg->pdialog->pEndWidgetList = pWindow;
  
  area = pWindow->area;
  area.w = MAX(area.w, adj_size(8));  
  
  /* ------------------ */
  /* exit button */
  pBuf = create_themeicon(pTheme->Small_CANCEL_Icon, pWindow->dst,
  			  WF_WIDGET_HAS_INFO_LABEL | WF_RESTORE_BACKGROUND);
  pBuf->string16 = create_str16_from_char(_("Close Dialog (Esc)"), adj_font(12));
  area.w += pBuf->size.w + adj_size(10);
  pBuf->action = exit_spy_steal_dlg_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->key = SDLK_ESCAPE;
  
  add_to_gui_list(ID_TERRAIN_ADV_DLG_EXIT_BUTTON, pBuf);  
  /* ------------------------- */
  
  count++; /* count + at Spy's Discretion */
  /* max col - 104 is steal tech widget width */
  max_col = (Main.screen->w - (pWindow->size.w - pWindow->area.w) - adj_size(2)) / adj_size(104);
  /* max row - 204 is steal tech widget height */
  max_row = (Main.screen->h - (pWindow->size.h - pWindow->area.h)) / adj_size(204);
  
  /* make space on screen for scrollbar */
  if (max_col * max_row < count) {
    max_col--;
  }

  if (count < max_col + 1) {
    col = count;
  } else {
    if (count < max_col + adj_size(3)) {
      col = max_col - adj_size(2);
    } else {
      if (count < max_col + adj_size(5)) {
        col = max_col - 1;
      } else {
        col = max_col;
      }
    }
  }
  
  pStr = create_string16(NULL, 0, adj_font(10));
  pStr->style |= (TTF_STYLE_BOLD | SF_CENTER);
  
  count = 0;
  advance_index_iterate(A_FIRST, i) {
    if (player_invention_reachable(client.conn.playing, i)
     && TECH_KNOWN == player_invention_state(pVictim, i)
     && (TECH_UNKNOWN == player_invention_state(client.conn.playing, i)
         || TECH_PREREQS_KNOWN ==
              player_invention_state(client.conn.playing, i))) {
      count++;

      copy_chars_to_string16(pStr, advance_name_translation(advance_by_number(i)));
      pSurf = create_sellect_tech_icon(pStr, i, FULL_MODE);
      pBuf = create_icon2(pSurf, pWindow->dst,
      		WF_FREE_THEME | WF_RESTORE_BACKGROUND);

      set_wstate(pBuf, FC_WS_NORMAL);
      pBuf->action = spy_steal_callback;
      pBuf->data.cont = pCont;

      add_to_gui_list(MAX_ID - i, pBuf);
    
      if (count > (col * max_row)) {
        set_wflag(pBuf, WF_HIDDEN);
      }
    }
  } advance_index_iterate_end;
  
  /* get spy tech */
  i = advance_number(unit_type(game_find_unit_by_number(id))->require_advance);
  copy_chars_to_string16(pStr, _("At Spy's Discretion"));
  pSurf = create_sellect_tech_icon(pStr, i, FULL_MODE);
	
  pBuf = create_icon2(pSurf, pWindow->dst,
    	(WF_FREE_THEME | WF_RESTORE_BACKGROUND| WF_FREE_DATA));
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->action = spy_steal_callback;
  pBuf->data.cont = pCont;
    
  add_to_gui_list(MAX_ID - advance_count(), pBuf);
  count++;
  
  /* --------------------------------------------------------- */
  FREESTRING16(pStr);
  pDiplomat_Dlg->pdialog->pBeginWidgetList = pBuf;
  pDiplomat_Dlg->pdialog->pBeginActiveWidgetList = pDiplomat_Dlg->pdialog->pBeginWidgetList;
  pDiplomat_Dlg->pdialog->pEndActiveWidgetList = pDiplomat_Dlg->pdialog->pEndWidgetList->prev->prev;
  
  /* -------------------------------------------------------------- */
  
  i = 0;
  if (count > col) {
    count = (count + (col - 1)) / col;
    if (count > max_row) {
      pDiplomat_Dlg->pdialog->pActiveWidgetList = pDiplomat_Dlg->pdialog->pEndActiveWidgetList;
      count = max_row;
      i = create_vertical_scrollbar(pDiplomat_Dlg->pdialog, col, count, TRUE, TRUE);  
    }
  } else {
    count = 1;
  }

  area.w = MAX(area.w, (col * pBuf->size.w + adj_size(2) + i));
  area.h = count * pBuf->size.h + adj_size(2);

  /* alloca window theme and win background buffer */
  pSurf = theme_get_background(theme, BACKGROUND_SPYSTEALDLG);
  if (resize_window(pWindow, pSurf, NULL,
                    (pWindow->size.w - pWindow->area.w) + area.w,
                    (pWindow->size.h - pWindow->area.h) + area.h))
  {
    FREESURFACE(pSurf);
  }
  
  area = pWindow->area;
  
  widget_set_position(pWindow,
                      (Main.screen->w - pWindow->size.w) / 2,
                      (Main.screen->h - pWindow->size.h) / 2);
  
    /* exit button */
  pBuf = pWindow->prev;
  pBuf->size.x = area.x + area.w - pBuf->size.w - 1;
  pBuf->size.y = pWindow->size.y + adj_size(2);
  
  setup_vertical_widgets_position(col, area.x + 1,
		  area.y, 0, 0,
		  pDiplomat_Dlg->pdialog->pBeginActiveWidgetList,
  		  pDiplomat_Dlg->pdialog->pEndActiveWidgetList);
    
  if(pDiplomat_Dlg->pdialog->pScroll) {
    setup_vertical_scrollbar_area(pDiplomat_Dlg->pdialog->pScroll,
	area.x + area.w, area.y,
    	area.h, TRUE);
  }

  redraw_group(pDiplomat_Dlg->pdialog->pBeginWidgetList, pWindow, FALSE);
  widget_mark_dirty(pWindow);
  
  return -1;
}

/****************************************************************
...
*****************************************************************/
static int diplomat_steal_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (game_find_unit_by_number(pDiplomat_Dlg->diplomat_id)
       && game_find_city_by_number(pDiplomat_Dlg->diplomat_target_id)) {  
      request_diplomat_action(DIPLOMAT_STEAL, pDiplomat_Dlg->diplomat_id,
                                     pDiplomat_Dlg->diplomat_target_id, A_UNSET);
    }
    
    popdown_diplomat_dialog();  
  }
  return -1;
}

/****************************************************************
...  Ask the server how much the revolt is going to cost us
*****************************************************************/
static int diplomat_incite_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (game_find_unit_by_number(pDiplomat_Dlg->diplomat_id)
       && game_find_city_by_number(pDiplomat_Dlg->diplomat_target_id)) {  
      request_diplomat_answer(DIPLOMAT_INCITE,
			      pDiplomat_Dlg->diplomat_id,
			      pDiplomat_Dlg->diplomat_target_id, 0);
    }
    
    popdown_diplomat_dialog();
  }  
  return -1;
}

/****************************************************************
  Callback from diplomat/spy dialog for "keep moving".
  (This should only occur when entering allied city.)
*****************************************************************/
static int diplomat_keep_moving_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct unit *punit;
    struct city *pcity;
    
    if( (punit=game_find_unit_by_number(pDiplomat_Dlg->diplomat_id))
        && (pcity=game_find_city_by_number(pDiplomat_Dlg->diplomat_target_id))
        && !same_pos(punit->tile, pcity->tile)) {
      request_diplomat_action(DIPLOMAT_MOVE, pDiplomat_Dlg->diplomat_id,
                              pDiplomat_Dlg->diplomat_target_id, 0);
    }
    
    popdown_diplomat_dialog();  
  }
  return -1;
}

/****************************************************************
...  Ask the server how much the bribe is
*****************************************************************/
static int diplomat_bribe_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
  
    if (game_find_unit_by_number(pDiplomat_Dlg->diplomat_id)
       && game_find_unit_by_number(pDiplomat_Dlg->diplomat_target_id)) {  
      request_diplomat_answer(DIPLOMAT_BRIBE,
			      pDiplomat_Dlg->diplomat_id,
			      pDiplomat_Dlg->diplomat_target_id, 0);
    }
    
    popdown_diplomat_dialog();
  }  
  return -1;
}

/****************************************************************
...
*****************************************************************/
static int spy_sabotage_unit_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    int diplomat_id = MAX_ID - pWidget->ID;
    int target_id = pWidget->data.unit->id;
    
    popdown_diplomat_dialog();
    request_diplomat_action(SPY_SABOTAGE_UNIT, diplomat_id, target_id, 0);
  }
  return -1;
}

/****************************************************************
...
*****************************************************************/
static int diplomat_close_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    popdown_diplomat_dialog();
  }
  return -1;
}

/**************************************************************************
  Popdown a dialog giving a diplomatic unit some options when moving into
  the target tile.
**************************************************************************/
void popdown_diplomat_dialog(void)
{
  if (pDiplomat_Dlg) {
    is_unit_move_blocked = FALSE;
    popdown_window_group_dialog(pDiplomat_Dlg->pdialog->pBeginWidgetList,
				pDiplomat_Dlg->pdialog->pEndWidgetList);
    FC_FREE(pDiplomat_Dlg->pdialog->pScroll);
    FC_FREE(pDiplomat_Dlg->pdialog);
    FC_FREE(pDiplomat_Dlg);
    queue_flush();
  }
}

/**************************************************************************
  Popup a dialog giving a diplomatic unit some options when moving into
  the target tile.
**************************************************************************/
void popup_diplomat_dialog(struct unit *pUnit, struct tile *ptile)
{
  struct widget *pWindow = NULL, *pBuf = NULL;
  SDL_String16 *pStr;
  struct city *pCity;
  struct unit *pTunit;
  bool spy;
  SDL_Rect area;
  
  if (pDiplomat_Dlg) {
    return;
  }
  
  is_unit_move_blocked = TRUE;
  pCity = tile_city(ptile);
  spy = unit_has_type_flag(pUnit, F_SPY);
  
  pDiplomat_Dlg = fc_calloc(1, sizeof(struct diplomat_dialog));
  pDiplomat_Dlg->diplomat_id = pUnit->id;
  pDiplomat_Dlg->pdialog = fc_calloc(1, sizeof(struct ADVANCED_DLG));
  
  /* window */
  if (pCity)
  {
    if(spy) {
      pStr = create_str16_from_char(_("Choose Your Spy's Strategy") , adj_font(12));
    }
    else
    {
      pStr = create_str16_from_char(_("Choose Your Diplomat's Strategy"), adj_font(12));
    }
  }
  else
  {
    pStr = create_str16_from_char(_("Subvert Enemy Unit"), adj_font(12));
  }
  
  pStr->style |= TTF_STYLE_BOLD;
  
  pWindow = create_window_skeleton(NULL, pStr, 0);
    
  pWindow->action = diplomat_dlg_window_callback;
  set_wstate(pWindow, FC_WS_NORMAL);
  
  add_to_gui_list(ID_CARAVAN_DLG_WINDOW, pWindow);
  pDiplomat_Dlg->pdialog->pEndWidgetList = pWindow;
    
  area = pWindow->area;
  area.w = MAX(area.w, adj_size(8));
  area.h = MAX(area.h, adj_size(2));
  
  /* ---------- */
  if((pCity))
  {
    /* Spy/Diplomat acting against a city */

    pDiplomat_Dlg->diplomat_target_id = pCity->id;    
    
    /* -------------- */
    if (diplomat_can_do_action(pUnit, DIPLOMAT_EMBASSY, ptile))
    {
	
      create_active_iconlabel(pBuf, pWindow->dst, pStr,
	    _("Establish Embassy"), diplomat_embassy_callback);
      
      pBuf->data.city = pCity;
      set_wstate(pBuf, FC_WS_NORMAL);
  
      add_to_gui_list(MAX_ID - pUnit->id, pBuf);
    
      area.w = MAX(area.w, pBuf->size.w);
      area.h += pBuf->size.h;
    }
  
    /* ---------- */
    if (diplomat_can_do_action(pUnit, DIPLOMAT_INVESTIGATE, ptile)) {
    
      create_active_iconlabel(pBuf, pWindow->dst, pStr,
			      _("Investigate City"),
			      diplomat_investigate_callback);
      pBuf->data.city = pCity;
      set_wstate(pBuf, FC_WS_NORMAL);
  
      add_to_gui_list(MAX_ID - pUnit->id, pBuf);
    
      area.w = MAX(area.w, pBuf->size.w);
      area.h += pBuf->size.h;
    }
  
    /* ---------- */
    if (spy && diplomat_can_do_action(pUnit, SPY_POISON, ptile)) {
    
      create_active_iconlabel(pBuf, pWindow->dst, pStr,
	    _("Poison City"), spy_poison_callback);
      
      pBuf->data.city = pCity;
      set_wstate(pBuf, FC_WS_NORMAL);
  
      add_to_gui_list(MAX_ID - pUnit->id, pBuf);
    
      area.w = MAX(area.w, pBuf->size.w);
      area.h += pBuf->size.h;
    }    
    /* ---------- */
    if (diplomat_can_do_action(pUnit, DIPLOMAT_SABOTAGE, ptile)) {
    
      create_active_iconlabel(pBuf, pWindow->dst, pStr,
	    _("Sabotage City"), 
      		spy ? spy_sabotage_request : diplomat_sabotage_callback);
      
      pBuf->data.city = pCity;
      set_wstate(pBuf, FC_WS_NORMAL);
  
      add_to_gui_list(MAX_ID - pUnit->id, pBuf);
    
      area.w = MAX(area.w, pBuf->size.w);
      area.h += pBuf->size.h;
    }
  
    /* ---------- */
    if (diplomat_can_do_action(pUnit, DIPLOMAT_STEAL, ptile)) {
    
      create_active_iconlabel(pBuf, pWindow->dst, pStr,
	    _("Steal Technology"),
      			spy ? spy_steal_popup : diplomat_steal_callback);
      pBuf->data.city = pCity;
      set_wstate(pBuf , FC_WS_NORMAL);
  
      add_to_gui_list(MAX_ID - pUnit->id , pBuf);
    
      area.w = MAX(area.w , pBuf->size.w);
      area.h += pBuf->size.h;
    }
      
    /* ---------- */
    if (diplomat_can_do_action(pUnit, DIPLOMAT_INCITE, ptile)) {
    
      create_active_iconlabel(pBuf, pWindow->dst, pStr,
	    _("Incite a Revolt"), diplomat_incite_callback);
      pBuf->data.city = pCity;
      set_wstate(pBuf , FC_WS_NORMAL);
  
      add_to_gui_list(MAX_ID - pUnit->id , pBuf);
    
      area.w = MAX(area.w , pBuf->size.w);
      area.h += pBuf->size.h;
    }
      
    /* ---------- */
    if (diplomat_can_do_action(pUnit, DIPLOMAT_MOVE, ptile)) {
    
      create_active_iconlabel(pBuf, pWindow->dst, pStr,
	    _("Keep moving"), diplomat_keep_moving_callback);
      
      pBuf->data.city = pCity;
      set_wstate(pBuf , FC_WS_NORMAL);
  
      add_to_gui_list(MAX_ID - pUnit->id , pBuf);
    
      area.w = MAX(area.w, pBuf->size.w);
      area.h += pBuf->size.h;
    }
  }
  else
  {
    if((pTunit=unit_list_get(ptile->units, 0))){
       /* Spy/Diplomat acting against a unit */ 
      
      pDiplomat_Dlg->diplomat_target_id = pTunit->id;
      
      /* ---------- */
      if (diplomat_can_do_action(pUnit, DIPLOMAT_BRIBE, ptile)) {
    
        create_active_iconlabel(pBuf, pWindow->dst, pStr,
	    _("Bribe Enemy Unit"), diplomat_bribe_callback);
	
        pBuf->data.unit = pTunit;
        set_wstate(pBuf , FC_WS_NORMAL);
  
        add_to_gui_list(MAX_ID - pUnit->id , pBuf);
    
        area.w = MAX(area.w , pBuf->size.w);
        area.h += pBuf->size.h;
      }
      
      /* ---------- */
      if (diplomat_can_do_action(pUnit, SPY_SABOTAGE_UNIT, ptile)) {
    
        create_active_iconlabel(pBuf, pWindow->dst, pStr,
	    _("Sabotage Enemy Unit"), spy_sabotage_unit_callback);
	
        pBuf->data.unit = pTunit;
        set_wstate(pBuf , FC_WS_NORMAL);
  
        add_to_gui_list(MAX_ID - pUnit->id , pBuf);
    
        area.w = MAX(area.w , pBuf->size.w);
        area.h += pBuf->size.h;
      }
    }
  }
  /* ---------- */
  
  create_active_iconlabel(pBuf, pWindow->dst, pStr,
	    _("Cancel"), diplomat_close_callback);
    
  set_wstate(pBuf , FC_WS_NORMAL);
  pBuf->key = SDLK_ESCAPE;
  
  add_to_gui_list(ID_LABEL , pBuf);
    
  area.w = MAX(area.w, pBuf->size.w);
  area.h += pBuf->size.h;
  /* ---------- */
  pDiplomat_Dlg->pdialog->pBeginWidgetList = pBuf;
  
  /* setup window size and start position */

  resize_window(pWindow, NULL, NULL, 
                (pWindow->size.w - pWindow->area.w) + area.w,
                (pWindow->size.h - pWindow->area.h) + area.h);
  
  area = pWindow->area;
  
  auto_center_on_focus_unit();
  put_window_near_map_tile(pWindow, pWindow->size.w, pWindow->size.h,
                           pUnit->tile);
 
  /* setup widget size and start position */
    
  pBuf = pWindow->prev;
  setup_vertical_widgets_position(1,
	area.x,
  	area.y + 1, area.w, 0,
	pDiplomat_Dlg->pdialog->pBeginWidgetList, pBuf);
  
  /* --------------------- */
  /* redraw */
  redraw_group(pDiplomat_Dlg->pdialog->pBeginWidgetList, pWindow, 0);

  widget_flush(pWindow);
  
}

/****************************************************************
  Returns id of a diplomat currently handled in diplomat dialog
*****************************************************************/
int diplomat_handled_in_diplomat_dialog(void)
{
  if (!pDiplomat_Dlg) {
    return -1;
  }

  return pDiplomat_Dlg->diplomat_id;
}

/****************************************************************
  Closes the diplomat dialog
****************************************************************/
void close_diplomat_dialog(void)
{
  popdown_diplomat_dialog();
}

/* ====================================================================== */
/* ============================ SABOTAGE DIALOG ========================= */
/* ====================================================================== */

static int sabotage_impr_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    int sabotage_improvement = MAX_ID - pWidget->ID;
    int diplomat_target_id = pWidget->data.cont->id0;
    int diplomat_id = pWidget->data.cont->id1;
      
    popdown_advanced_terrain_dialog();
    
    if(sabotage_improvement == 1000)
    {
      sabotage_improvement = -1;
    }
    
    if(game_find_unit_by_number(diplomat_id)
      && game_find_city_by_number(diplomat_target_id)) { 
      request_diplomat_action(DIPLOMAT_SABOTAGE, diplomat_id,
                              diplomat_target_id, sabotage_improvement + 1);
    }
  }
  return -1;
}

/****************************************************************
 Pops-up the Spy sabotage dialog, upon return of list of
 available improvements requested by the above function.
*****************************************************************/
void popup_sabotage_dialog(struct city *pCity)
{
  struct widget *pWindow = NULL, *pBuf = NULL , *pLast = NULL;
  struct CONTAINER *pCont;
  struct unit *pUnit = head_of_units_in_focus();
  SDL_String16 *pStr;
  SDL_Rect area, area2;
  int n, w = 0, h, imp_h = 0;
  
  if (pDiplomat_Dlg || !pUnit || !unit_has_type_flag(pUnit, F_SPY)) {
    return;
  }
  
  is_unit_move_blocked = TRUE;
    
  pDiplomat_Dlg = fc_calloc(1, sizeof(struct diplomat_dialog));
  pDiplomat_Dlg->diplomat_id = pUnit->id;
  pDiplomat_Dlg->diplomat_target_id = pCity->id;
  pDiplomat_Dlg->pdialog = fc_calloc(1, sizeof(struct ADVANCED_DLG));
  
  pCont = fc_calloc(1, sizeof(struct CONTAINER));
  pCont->id0 = pCity->id;
  pCont->id1 = pUnit->id;/* spy id */
    
  pStr = create_str16_from_char(_("Select Improvement to Sabotage") , adj_font(12));
  pStr->style |= TTF_STYLE_BOLD;
  
  pWindow = create_window_skeleton(NULL, pStr, 0);
    
  pWindow->action = diplomat_dlg_window_callback;
  set_wstate(pWindow, FC_WS_NORMAL);
  
  add_to_gui_list(ID_TERRAIN_ADV_DLG_WINDOW, pWindow);
  pDiplomat_Dlg->pdialog->pEndWidgetList = pWindow;
  
  area = pWindow->area;
  area.h = MAX(area.h, adj_size(2));
  
  /* ---------- */
  /* exit button */
  pBuf = create_themeicon(pTheme->Small_CANCEL_Icon, pWindow->dst,
  			  WF_WIDGET_HAS_INFO_LABEL | WF_RESTORE_BACKGROUND);
  pBuf->string16 = create_str16_from_char(_("Close Dialog (Esc)"), adj_font(12));
  area.w += pBuf->size.w + adj_size(10);
  pBuf->action = diplomat_close_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->key = SDLK_ESCAPE;
  
  add_to_gui_list(ID_TERRAIN_ADV_DLG_EXIT_BUTTON, pBuf);
  /* ---------- */
  
  create_active_iconlabel(pBuf, pWindow->dst, pStr,
	    _("City Production"), sabotage_impr_callback);
  pBuf->data.cont = pCont;  
  set_wstate(pBuf, FC_WS_NORMAL);
  set_wflag(pBuf, WF_FREE_DATA);
  add_to_gui_list(MAX_ID - 1000, pBuf);
    
  area.w = MAX(area.w, pBuf->size.w);
  area.h += pBuf->size.h;

  /* separator */
  pBuf = create_iconlabel(NULL, pWindow->dst, NULL, WF_FREE_THEME);
    
  add_to_gui_list(ID_SEPARATOR, pBuf);
  area.h += pBuf->next->size.h;
  /* ------------------ */
  n = 0;
  city_built_iterate(pCity, pImprove) {
    if (pImprove->sabotage > 0) {
      
      create_active_iconlabel(pBuf, pWindow->dst, pStr,
	      (char *) city_improvement_name_translation(pCity, pImprove),
				      sabotage_impr_callback);
      pBuf->data.cont = pCont;
      set_wstate(pBuf , FC_WS_NORMAL);
  
      add_to_gui_list(MAX_ID - improvement_number(pImprove), pBuf);
    
      area.w = MAX(area.w , pBuf->size.w);
      imp_h += pBuf->size.h;
      
      if (!pDiplomat_Dlg->pdialog->pEndActiveWidgetList)
      {
	pDiplomat_Dlg->pdialog->pEndActiveWidgetList = pBuf;
      }
    
      if (improvement_number(pImprove) > 9)
      {
        set_wflag(pBuf, WF_HIDDEN);
      }
      
      n++;    
      /* ----------- */
    }  
  } city_built_iterate_end;

  pDiplomat_Dlg->pdialog->pBeginActiveWidgetList = pBuf;
  
  if (n > 0) {
    /* separator */
    pBuf = create_iconlabel(NULL, pWindow->dst, NULL, WF_FREE_THEME);
    
    add_to_gui_list(ID_SEPARATOR, pBuf);
    area.h += pBuf->next->size.h;
  /* ------------------ */
  }
  
  create_active_iconlabel(pBuf, pWindow->dst, pStr, _("At Spy's Discretion"),
			  sabotage_impr_callback);
  pBuf->data.cont = pCont;  
  set_wstate(pBuf, FC_WS_NORMAL);
  
  add_to_gui_list(MAX_ID - B_LAST, pBuf);
    
  area.w = MAX(area.w, pBuf->size.w);
  area.h += pBuf->size.h;
  /* ----------- */
  
  pLast = pBuf;
  pDiplomat_Dlg->pdialog->pBeginWidgetList = pLast;
  pDiplomat_Dlg->pdialog->pActiveWidgetList = pDiplomat_Dlg->pdialog->pEndActiveWidgetList;
  
  /* ---------- */
  if (n > 10)
  {
    imp_h = 10 * pBuf->size.h;
    
    n = create_vertical_scrollbar(pDiplomat_Dlg->pdialog,
		  1, 10, TRUE, TRUE);
    area.w += n;
  }
  /* ---------- */
  
  
  area.h += imp_h;

  resize_window(pWindow, NULL, NULL, 
                (pWindow->size.w - pWindow->area.w) + area.w,
                (pWindow->size.h - pWindow->area.h) + area.h);
  
  area = pWindow->area;
  
  auto_center_on_focus_unit();
  put_window_near_map_tile(pWindow, pWindow->size.w, pWindow->size.h,
                           pUnit->tile);        
  
  w = area.w;
  
  if (pDiplomat_Dlg->pdialog->pScroll)
  {
    w -= n;
    imp_h = pBuf->size.w;
  }
  else
  {
    imp_h = 0;
  }
  
  /* exit button */
  pBuf = pWindow->prev;
  pBuf->size.x = area.x + area.w - pBuf->size.w - 1;
  pBuf->size.y = pWindow->size.y + adj_size(2);
  
  /* Production sabotage */
  pBuf = pBuf->prev;
  
  pBuf->size.x = area.x;
  pBuf->size.y = area.y + 1;
  pBuf->size.w = w;
  h = pBuf->size.h;
  
  area2.x = adj_size(10);
  area2.h = adj_size(2);
  
  pBuf = pBuf->prev;
  while(pBuf)
  {
    
    if (pBuf == pDiplomat_Dlg->pdialog->pEndActiveWidgetList)
    {
      w -= imp_h;
    }
    
    pBuf->size.w = w;
    pBuf->size.x = pBuf->next->size.x;
    pBuf->size.y = pBuf->next->size.y + pBuf->next->size.h;
    
    if (pBuf->ID == ID_SEPARATOR)
    {
      FREESURFACE(pBuf->theme);
      pBuf->size.h = h;
      pBuf->theme = create_surf(w, h, SDL_SWSURFACE);
    
      area2.y = pBuf->size.h / 2 - 1;
      area2.w = pBuf->size.w - adj_size(20);
      
      SDL_FillRect(pBuf->theme , &area2, map_rgba(pBuf->theme->format, *get_game_colorRGB(COLOR_THEME_SABOTAGEDLG_SEPARATOR)));
    }
    
    if (pBuf == pLast) {
      break;
    }
    pBuf = pBuf->prev;  
  }
  
  if (pDiplomat_Dlg->pdialog->pScroll)
  {
    setup_vertical_scrollbar_area(pDiplomat_Dlg->pdialog->pScroll,
	area.x + area.w,
    	pDiplomat_Dlg->pdialog->pEndActiveWidgetList->size.y,
    	area.y - pDiplomat_Dlg->pdialog->pEndActiveWidgetList->size.y +
	    area.h, TRUE);
  }
  
  /* -------------------- */
  /* redraw */
  redraw_group(pDiplomat_Dlg->pdialog->pBeginWidgetList, pWindow, 0);

  widget_flush(pWindow);
  
}

/* ====================================================================== */
/* ============================== INCITE DIALOG ========================= */
/* ====================================================================== */
static struct small_diplomat_dialog *pIncite_Dlg = NULL;

/****************************************************************
...
*****************************************************************/
static int incite_dlg_window_callback(struct widget *pWindow)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    move_window_group(pIncite_Dlg->pdialog->pBeginWidgetList, pWindow);
  }
  return -1;
}

/****************************************************************
...
*****************************************************************/
static int diplomat_incite_yes_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (game_find_unit_by_number(pIncite_Dlg->diplomat_id)
       && game_find_city_by_number(pIncite_Dlg->diplomat_target_id)) {  
      request_diplomat_action(DIPLOMAT_INCITE, pIncite_Dlg->diplomat_id,
                                         pIncite_Dlg->diplomat_target_id, 0);       
    }
    popdown_incite_dialog();
  }  
  return -1;
}

/****************************************************************
...
*****************************************************************/
static int exit_incite_dlg_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    popdown_incite_dialog();
  }
  return -1;
}

/**************************************************************************
  Popdown a window asking a diplomatic unit if it wishes to incite the
  given enemy city.
**************************************************************************/
void popdown_incite_dialog(void)
{
  if (pIncite_Dlg) {
    is_unit_move_blocked = FALSE;
    popdown_window_group_dialog(pIncite_Dlg->pdialog->pBeginWidgetList,
				pIncite_Dlg->pdialog->pEndWidgetList);
    FC_FREE(pIncite_Dlg->pdialog);
    FC_FREE(pIncite_Dlg);
    flush_dirty();
  }
}

/**************************************************************************
  Popup a window asking a diplomatic unit if it wishes to incite the
  given enemy city.
**************************************************************************/
void popup_incite_dialog(struct city *pCity, int cost)
{
  struct widget *pWindow = NULL, *pBuf = NULL;
  SDL_String16 *pStr;
  struct unit *pUnit;
  char cBuf[255]; 
  bool exit = FALSE;
  SDL_Rect area;
  
  if (pIncite_Dlg) {
    return;
  }
  
  /* ugly hack */
  pUnit = head_of_units_in_focus();
  
  if (!pUnit || !is_diplomat_unit(pUnit)) {
    return;
  }
  
  is_unit_move_blocked = TRUE;
  
  pIncite_Dlg = fc_calloc(1, sizeof(struct small_diplomat_dialog));
  pIncite_Dlg->diplomat_id = pUnit->id;
  pIncite_Dlg->diplomat_target_id = pCity->id;
  pIncite_Dlg->pdialog = fc_calloc(1, sizeof(struct SMALL_DLG));  
  
  /* window */
  pStr = create_str16_from_char(_("Incite a Revolt!"), adj_font(12));
    
  pStr->style |= TTF_STYLE_BOLD;
  
  pWindow = create_window_skeleton(NULL, pStr, 0);
    
  pWindow->action = incite_dlg_window_callback;
  set_wstate(pWindow, FC_WS_NORMAL);
  
  add_to_gui_list(ID_INCITE_DLG_WINDOW, pWindow);
  pIncite_Dlg->pdialog->pEndWidgetList = pWindow;
  
  area = pWindow->area;
  area.w  =MAX(area.w, adj_size(8));
  area.h = MAX(area.h, adj_size(2));
  
  if (INCITE_IMPOSSIBLE_COST == cost) {
    
    /* exit button */
    pBuf = create_themeicon(pTheme->Small_CANCEL_Icon, pWindow->dst,
  			    WF_WIDGET_HAS_INFO_LABEL | WF_RESTORE_BACKGROUND);  
    pBuf->string16 = create_str16_from_char(_("Close Dialog (Esc)"), adj_font(12));
    area.w += pBuf->size.w + adj_size(10);
    pBuf->action = exit_incite_dlg_callback;
    set_wstate(pBuf, FC_WS_NORMAL);
    pBuf->key = SDLK_ESCAPE;
  
    add_to_gui_list(ID_INCITE_DLG_EXIT_BUTTON, pBuf);
    exit = TRUE;
    /* --------------- */
    
    my_snprintf(cBuf, sizeof(cBuf), _("You can't incite a revolt in %s."),
		city_name(pCity));

    create_active_iconlabel(pBuf, pWindow->dst, pStr, cBuf, NULL);
        
    add_to_gui_list(ID_LABEL , pBuf);
    
    area.w = MAX(area.w , pBuf->size.w);
    area.h += pBuf->size.h;
    /*------------*/
    create_active_iconlabel(pBuf, pWindow->dst, pStr,
	    _("City can't be incited!"), NULL);
        
    add_to_gui_list(ID_LABEL , pBuf);
    
    area.w = MAX(area.w , pBuf->size.w);
    area.h += pBuf->size.h;
    
  } else if (cost <= client.conn.playing->economic.gold) {
    my_snprintf(cBuf, sizeof(cBuf),
		_("Incite a revolt for %d gold?\nTreasury contains %d gold."), 
		cost, client.conn.playing->economic.gold);
    
    create_active_iconlabel(pBuf, pWindow->dst, pStr, cBuf, NULL);
        
  
    add_to_gui_list(ID_LABEL , pBuf);
    
    area.w = MAX(area.w, pBuf->size.w);
    area.h += pBuf->size.h;
    
    /*------------*/
    create_active_iconlabel(pBuf, pWindow->dst, pStr,
	    _("Yes") , diplomat_incite_yes_callback);
    
    pBuf->data.city = pCity;
    set_wstate(pBuf, FC_WS_NORMAL);
  
    add_to_gui_list(MAX_ID - pUnit->id, pBuf);
    
    area.w = MAX(area.w, pBuf->size.w);
    area.h += pBuf->size.h;
    /* ------- */
    create_active_iconlabel(pBuf, pWindow->dst, pStr,
	    _("No") , exit_incite_dlg_callback);
    
    set_wstate(pBuf, FC_WS_NORMAL);
    pBuf->key = SDLK_ESCAPE;
    
    add_to_gui_list(ID_LABEL, pBuf);
    
    area.w = MAX(area.w, pBuf->size.w);
    area.h += pBuf->size.h;
    
  } else {
    /* exit button */
    pBuf = create_themeicon(pTheme->Small_CANCEL_Icon, pWindow->dst,
  			    WF_WIDGET_HAS_INFO_LABEL | WF_RESTORE_BACKGROUND);
  pBuf->string16 = create_str16_from_char(_("Close Dialog (Esc)"), adj_font(12));
    area.w += pBuf->size.w + adj_size(10);
    pBuf->action = exit_incite_dlg_callback;
    set_wstate(pBuf, FC_WS_NORMAL);
    pBuf->key = SDLK_ESCAPE;
  
    add_to_gui_list(ID_INCITE_DLG_EXIT_BUTTON, pBuf);
    exit = TRUE;
    /* --------------- */

    my_snprintf(cBuf, sizeof(cBuf),
		_("Inciting a revolt costs %d gold.\n"
		  "Treasury contains %d gold."), 
		cost, client.conn.playing->economic.gold);
    
    create_active_iconlabel(pBuf, pWindow->dst, pStr, cBuf, NULL);
        
  
    add_to_gui_list(ID_LABEL, pBuf);
    
    area.w = MAX(area.w, pBuf->size.w);
    area.h += pBuf->size.h;
    
    /*------------*/
    create_active_iconlabel(pBuf, pWindow->dst, pStr,
	    _("Traitors Demand Too Much!"), NULL);
        
    add_to_gui_list(ID_LABEL , pBuf);
    
    area.w = MAX(area.w, pBuf->size.w);
    area.h += pBuf->size.h;
  }
  pIncite_Dlg->pdialog->pBeginWidgetList = pBuf;
  
  /* setup window size and start position */

  resize_window(pWindow, NULL, NULL,
                (pWindow->size.w - pWindow->area.w) + area.w,
                (pWindow->size.h - pWindow->area.h) + area.h);
  
  area = pWindow->area;
  
  auto_center_on_focus_unit();
  put_window_near_map_tile(pWindow, pWindow->size.w, pWindow->size.h,
                           pCity->tile);
  
  /* setup widget size and start position */
  pBuf = pWindow;
  
  if (exit)
  {/* exit button */
    pBuf = pBuf->prev;
    pBuf->size.x = area.x + area.w - pBuf->size.w - 1;
    pBuf->size.y = pWindow->size.y + adj_size(2);
  }
  
  pBuf = pBuf->prev;
  setup_vertical_widgets_position(1,
	area.x,
  	area.y + 1, area.w, 0,
	pIncite_Dlg->pdialog->pBeginWidgetList, pBuf);
    
  /* --------------------- */
  /* redraw */
  redraw_group(pIncite_Dlg->pdialog->pBeginWidgetList, pWindow, 0);

  widget_flush(pWindow);
  
}

/* ====================================================================== */
/* ============================ BRIBE DIALOG ========================== */
/* ====================================================================== */
static struct small_diplomat_dialog *pBribe_Dlg = NULL;

static int bribe_dlg_window_callback(struct widget *pWindow)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    move_window_group(pBribe_Dlg->pdialog->pBeginWidgetList, pWindow);
  }
  return -1;
}

static int diplomat_bribe_yes_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (game_find_unit_by_number(pBribe_Dlg->diplomat_id)
       && game_find_unit_by_number(pBribe_Dlg->diplomat_target_id)) {  
       request_diplomat_action(DIPLOMAT_BRIBE, pBribe_Dlg->diplomat_id,
                                          pBribe_Dlg->diplomat_target_id, 0);       
    }
    popdown_bribe_dialog();
  }  
  return -1;
}

static int exit_bribe_dlg_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    popdown_bribe_dialog();
  }
  return -1;
}

/**************************************************************************
  Popdown a dialog asking a diplomatic unit if it wishes to bribe the
  given enemy unit.
**************************************************************************/
void popdown_bribe_dialog(void)
{
  if (pBribe_Dlg) {
    is_unit_move_blocked = FALSE;
    popdown_window_group_dialog(pBribe_Dlg->pdialog->pBeginWidgetList,
				pBribe_Dlg->pdialog->pEndWidgetList);
    FC_FREE(pBribe_Dlg->pdialog);
    FC_FREE(pBribe_Dlg);
    flush_dirty();
  }
}

/**************************************************************************
  Popup a dialog asking a diplomatic unit if it wishes to bribe the
  given enemy unit.
**************************************************************************/
void popup_bribe_dialog(struct unit *pUnit, int cost)
{
  struct widget *pWindow = NULL, *pBuf = NULL;
  SDL_String16 *pStr;
  struct unit *pDiplomatUnit;
  char cBuf[255]; 
  bool exit = FALSE;
  SDL_Rect area;
  
  if (pBribe_Dlg) {
    return;
  }
    
  /* ugly hack */
  pDiplomatUnit = head_of_units_in_focus();
  
  if (!pDiplomatUnit || !is_diplomat_unit(pDiplomatUnit)) {
    return;
  }
  
  is_unit_move_blocked = TRUE;
  
  pBribe_Dlg = fc_calloc(1, sizeof(struct small_diplomat_dialog));
  pBribe_Dlg->diplomat_id = pDiplomatUnit->id;
  pBribe_Dlg->diplomat_target_id = pUnit->id;
  pBribe_Dlg->pdialog = fc_calloc(1, sizeof(struct SMALL_DLG));
  
  /* window */
  pStr = create_str16_from_char(_("Bribe Enemy Unit"), adj_font(12));
    
  pStr->style |= TTF_STYLE_BOLD;
  
  pWindow = create_window_skeleton(NULL, pStr, 0);
    
  pWindow->action = bribe_dlg_window_callback;
  set_wstate(pWindow, FC_WS_NORMAL);

  add_to_gui_list(ID_BRIBE_DLG_WINDOW, pWindow);
  pBribe_Dlg->pdialog->pEndWidgetList = pWindow;
  
  area = pWindow->area;
  area.w = MAX(area.w, adj_size(8));
  area.h = MAX(area.h, adj_size(2));
  
  if (cost <= client.conn.playing->economic.gold) {
    my_snprintf(cBuf, sizeof(cBuf),
		_("Bribe unit for %d gold?\nTreasury contains %d gold."), 
		cost, client.conn.playing->economic.gold);
    
    create_active_iconlabel(pBuf, pWindow->dst, pStr, cBuf, NULL);
  
    add_to_gui_list(ID_LABEL, pBuf);
    
    area.w = MAX(area.w, pBuf->size.w);
    area.h += pBuf->size.h;
    
    /*------------*/
    create_active_iconlabel(pBuf, pWindow->dst, pStr,
	    _("Yes"), diplomat_bribe_yes_callback);
    pBuf->data.unit = pUnit;
    set_wstate(pBuf, FC_WS_NORMAL);
  
    add_to_gui_list(MAX_ID - pDiplomatUnit->id, pBuf);
    
    area.w = MAX(area.w, pBuf->size.w);
    area.h += pBuf->size.h;
    /* ------- */
    create_active_iconlabel(pBuf, pWindow->dst, pStr,
	    _("No") , exit_bribe_dlg_callback);
    
    set_wstate(pBuf , FC_WS_NORMAL);
    pBuf->key = SDLK_ESCAPE;
    
    add_to_gui_list(ID_LABEL, pBuf);
    
    area.w = MAX(area.w, pBuf->size.w);
    area.h += pBuf->size.h;
    
  } else {
    /* exit button */
    pBuf = create_themeicon(pTheme->Small_CANCEL_Icon, pWindow->dst,
  			    WF_WIDGET_HAS_INFO_LABEL | WF_RESTORE_BACKGROUND);
    pBuf->string16 = create_str16_from_char(_("Close Dialog (Esc)"), adj_font(12));
    area.w += pBuf->size.w + adj_size(10);
    pBuf->action = exit_bribe_dlg_callback;
    set_wstate(pBuf, FC_WS_NORMAL);
    pBuf->key = SDLK_ESCAPE;
  
    add_to_gui_list(ID_BRIBE_DLG_EXIT_BUTTON, pBuf);
    exit = TRUE;
    /* --------------- */

    my_snprintf(cBuf, sizeof(cBuf),
		_("Bribing the unit costs %d gold.\n"
		  "Treasury contains %d gold."), 
		cost, client.conn.playing->economic.gold);
    
    create_active_iconlabel(pBuf, pWindow->dst, pStr, cBuf, NULL);
  
    add_to_gui_list(ID_LABEL, pBuf);
    
    area.w = MAX(area.w, pBuf->size.w);
    area.h += pBuf->size.h;
    
    /*------------*/
    create_active_iconlabel(pBuf, pWindow->dst, pStr,
	    _("Traitors Demand Too Much!"), NULL);
        
    add_to_gui_list(ID_LABEL, pBuf);
    
    area.w = MAX(area.w, pBuf->size.w);
    area.h += pBuf->size.h;
  }
  pBribe_Dlg->pdialog->pBeginWidgetList = pBuf;
  
  /* setup window size and start position */

  resize_window(pWindow, NULL, NULL,
                (pWindow->size.w - pWindow->area.w) + area.w,
                (pWindow->size.h - pWindow->area.h) + area.h);
  
  area = pWindow->area;
  
  auto_center_on_focus_unit();
  put_window_near_map_tile(pWindow, pWindow->size.w, pWindow->size.h,
                           pDiplomatUnit->tile);      
  
  /* setup widget size and start position */
  pBuf = pWindow;
  
  if (exit)
  {/* exit button */
    pBuf = pBuf->prev;
    pBuf->size.x = area.x + area.w - pBuf->size.w - 1;
    pBuf->size.y = pWindow->size.y + adj_size(2);
  }
  
  pBuf = pBuf->prev;
  setup_vertical_widgets_position(1,
	area.x,
  	area.y + 1, area.w, 0,
	pBribe_Dlg->pdialog->pBeginWidgetList, pBuf);
  
  /* --------------------- */
  /* redraw */
  redraw_group(pBribe_Dlg->pdialog->pBeginWidgetList, pWindow, 0);

  widget_flush(pWindow);
}
