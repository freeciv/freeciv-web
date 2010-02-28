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
#include "diptreaty.h"

/* client */
#include "client_main.h"
#include "climisc.h"
#include "packhand.h"

/* gui-sdl */
#include "chatline.h"
#include "colors.h"
#include "dialogs.h"
#include "graphics.h"
#include "gui_iconv.h"
#include "gui_id.h"
#include "gui_main.h"
#include "gui_tilespec.h"
#include "mapview.h"
#include "themespec.h"
#include "widget.h"

#include "diplodlg.h"

#define MAX_NUM_CLAUSES 64

struct diplomacy_dialog {
  struct Treaty treaty;
  struct ADVANCED_DLG *pdialog;
  struct ADVANCED_DLG *pwants;
  struct ADVANCED_DLG *poffers;
};

#define SPECLIST_TAG dialog
#define SPECLIST_TYPE struct diplomacy_dialog
#include "speclist.h"

#define dialog_list_iterate(dialoglist, pdialog) \
    TYPED_LIST_ITERATE(struct diplomacy_dialog, dialoglist, pdialog)
#define dialog_list_iterate_end  LIST_ITERATE_END

static struct dialog_list *dialog_list;

static void update_diplomacy_dialog(struct diplomacy_dialog *pdialog);
static void update_acceptance_icons(struct diplomacy_dialog *pdialog);
static void update_clauses_list(struct diplomacy_dialog *pdialog);
static void remove_clause_widget_from_list(int counterpart, int giver,
                                           enum clause_type type, int value);
static void popdown_diplomacy_dialog(int counterpart);
static void popdown_diplomacy_dialogs(void);
static void popdown_sdip_dialog(void);

/****************************************************************
...
*****************************************************************/
void diplomacy_dialog_init()
{
  dialog_list = dialog_list_new();
}

/****************************************************************
...
*****************************************************************/
void diplomacy_dialog_done()
{
  dialog_list_free(dialog_list);
}

/****************************************************************
...
*****************************************************************/
static struct diplomacy_dialog *get_diplomacy_dialog(int other_player_id)
{
  struct player *plr0 = client.conn.playing;
  struct player *plr1 = player_by_number(other_player_id);

  dialog_list_iterate(dialog_list, pdialog) {
    if ((pdialog->treaty.plr0 == plr0 && pdialog->treaty.plr1 == plr1) ||
	(pdialog->treaty.plr0 == plr1 && pdialog->treaty.plr1 == plr0)) {
      return pdialog;
    }
  } dialog_list_iterate_end;

  return NULL;
}

/**************************************************************************
  Update a player's acceptance status of a treaty (traditionally shown
  with the thumbs-up/thumbs-down sprite).
**************************************************************************/
void handle_diplomacy_accept_treaty(int counterpart, bool I_accepted,
				    bool other_accepted)
{
  struct diplomacy_dialog *pdialog = get_diplomacy_dialog(counterpart);

  if (!pdialog) {
    return;
  }
  
  pdialog->treaty.accept0 = I_accepted;
  pdialog->treaty.accept1 = other_accepted;
  
  update_acceptance_icons(pdialog);
}

/**************************************************************************
  Update the diplomacy dialog when the meeting is canceled (the dialog
  should be closed).
**************************************************************************/
void handle_diplomacy_cancel_meeting(int counterpart, int initiated_from)
{
  popdown_diplomacy_dialog(counterpart);
  flush_dirty();
}
/* ----------------------------------------------------------------------- */

static int remove_clause_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct diplomacy_dialog *pdialog;
      
    if (!(pdialog = get_diplomacy_dialog(pWidget->data.cont->id1))) {
      pdialog = get_diplomacy_dialog(pWidget->data.cont->id0);
    }
    
    dsend_packet_diplomacy_remove_clause_req(&client.conn,
                                             player_number(pdialog->treaty.plr1),
                                             pWidget->data.cont->id0,
                                             (enum clause_type) ((pWidget->data.
                                             cont->value >> 16) & 0xFFFF),
                                             pWidget->data.cont->value & 0xFFFF);
  }
  return -1;
}

/**************************************************************************
  Update the diplomacy dialog by adding a clause.
**************************************************************************/
void handle_diplomacy_create_clause(int counterpart, int giver,
				    enum clause_type type, int value)
{
  struct diplomacy_dialog *pdialog = get_diplomacy_dialog(counterpart);  
  
  if (!pdialog) {
    return;
  }
  
  clause_list_iterate(pdialog->treaty.clauses, pclause) {
    remove_clause_widget_from_list(player_number(pdialog->treaty.plr1),
                                   player_number(pclause->from),
                                   pclause->type,
                                   pclause->value);
  } clause_list_iterate_end;
  
  add_clause(&pdialog->treaty, player_by_number(giver), type, value);
  
  update_clauses_list(pdialog);
  update_acceptance_icons(pdialog);
}

/**************************************************************************
  Update the diplomacy dialog by removing a clause.
**************************************************************************/
void handle_diplomacy_remove_clause(int counterpart, int giver,
				    enum clause_type type, int value)
{
  struct diplomacy_dialog *pdialog = get_diplomacy_dialog(counterpart);  
    
  if (!pdialog) {
    return;
  }
  
  clause_list_iterate(pdialog->treaty.clauses, pclause) {
    remove_clause_widget_from_list(player_number(pdialog->treaty.plr1),
                                   player_number(pclause->from),
                                   pclause->type,
                                   pclause->value);
  } clause_list_iterate_end;

  remove_clause(&pdialog->treaty, player_by_number(giver), type, value);
  
  update_clauses_list(pdialog);
  update_acceptance_icons(pdialog);  
}

/* ================================================================= */

static int cancel_meeting_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    dsend_packet_diplomacy_cancel_meeting_req(&client.conn,
					    pWidget->data.cont->id1);
  }
  return -1;
}

static int accept_treaty_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    dsend_packet_diplomacy_accept_treaty_req(&client.conn,
					   pWidget->data.cont->id1);
  }
  return -1;
}

/* ------------------------------------------------------------------------ */

static int pact_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    int clause_type;
    
    struct diplomacy_dialog *pdialog;
      
    if (!(pdialog = get_diplomacy_dialog(pWidget->data.cont->id1))) {
      pdialog = get_diplomacy_dialog(pWidget->data.cont->id0);
    }
    
    switch(MAX_ID - pWidget->ID) {
      case 2:
        clause_type = CLAUSE_CEASEFIRE;
      break;
      case 1:
        clause_type = CLAUSE_PEACE;
      break;
      default:
        clause_type = CLAUSE_ALLIANCE;
      break;
    }
    
    dsend_packet_diplomacy_create_clause_req(&client.conn,
                                             player_number(pdialog->treaty.plr1),
                                             pWidget->data.cont->id0,
                                             clause_type, 0);
  }  
  return -1;
}

static int vision_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct diplomacy_dialog *pdialog;
      
    if (!(pdialog = get_diplomacy_dialog(pWidget->data.cont->id1))) {
      pdialog = get_diplomacy_dialog(pWidget->data.cont->id0);
    }
  
    dsend_packet_diplomacy_create_clause_req(&client.conn,
                                             player_number(pdialog->treaty.plr1),
                                             pWidget->data.cont->id0,
                                             CLAUSE_VISION, 0);
  }
  return -1;
}

static int embassy_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct diplomacy_dialog *pdialog;
      
    if (!(pdialog = get_diplomacy_dialog(pWidget->data.cont->id1))) {
      pdialog = get_diplomacy_dialog(pWidget->data.cont->id0);
    }
  
    dsend_packet_diplomacy_create_clause_req(&client.conn,
                                             player_number(pdialog->treaty.plr1),
                                             pWidget->data.cont->id0,
                                             CLAUSE_EMBASSY, 0);
  }
  return -1;
}

static int maps_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    int clause_type;
    
    struct diplomacy_dialog *pdialog;
      
    if (!(pdialog = get_diplomacy_dialog(pWidget->data.cont->id1))) {
      pdialog = get_diplomacy_dialog(pWidget->data.cont->id0);
    }
    
    switch(MAX_ID - pWidget->ID) {
      case 1:
        clause_type = CLAUSE_MAP;
      break;
      default:
        clause_type = CLAUSE_SEAMAP;
      break;
    }
  
    dsend_packet_diplomacy_create_clause_req(&client.conn,
                                             player_number(pdialog->treaty.plr1),
                                             pWidget->data.cont->id0,
                                             clause_type, 0);
  }
  return -1;
}

static int techs_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct diplomacy_dialog *pdialog;
      
    if (!(pdialog = get_diplomacy_dialog(pWidget->data.cont->id1))) {
      pdialog = get_diplomacy_dialog(pWidget->data.cont->id0);
    }
    
    dsend_packet_diplomacy_create_clause_req(&client.conn,
                                             player_number(pdialog->treaty.plr1),
                                             pWidget->data.cont->id0,
                                             CLAUSE_ADVANCE,
                                             (MAX_ID - pWidget->ID));
  }  
  return -1;
}

static int gold_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    int amount;
    
    struct diplomacy_dialog *pdialog;
      
    if (!(pdialog = get_diplomacy_dialog(pWidget->data.cont->id1))) {
      pdialog = get_diplomacy_dialog(pWidget->data.cont->id0);
    }
    
    if(pWidget->string16->text) {
      char cBuf[16];
      
      convertcopy_to_chars(cBuf, sizeof(cBuf), pWidget->string16->text);
      sscanf(cBuf, "%d", &amount);
      
      if(amount > pWidget->data.cont->value) {
        /* max player gold */
        amount = pWidget->data.cont->value;
      }
      
    } else {
      amount = 0;
    }
    
    if (amount > 0) {
      dsend_packet_diplomacy_create_clause_req(&client.conn,
                                               player_number(pdialog->treaty.plr1),
                                               pWidget->data.cont->id0,
                                               CLAUSE_GOLD, amount);
      
    } else {
      output_window_append(FTC_CLIENT_INFO, NULL,
                           _("Invalid amount of gold specified."));
    }
    
    if(amount || !pWidget->string16->text) {
      copy_chars_to_string16(pWidget->string16, "0");
      widget_redraw(pWidget);
      widget_flush(pWidget);
    }
  }  
  return -1;
}


static int cities_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct diplomacy_dialog *pdialog;
      
    if (!(pdialog = get_diplomacy_dialog(pWidget->data.cont->id1))) {
      pdialog = get_diplomacy_dialog(pWidget->data.cont->id0);
    }
    
    dsend_packet_diplomacy_create_clause_req(&client.conn,
                                             player_number(pdialog->treaty.plr1),  
                                             pWidget->data.cont->id0,
                                             CLAUSE_CITY,
                                             (MAX_ID - pWidget->ID));
  }  
  return -1;
}


static int dipomatic_window_callback(struct widget *pWindow)
{
  return -1;
}

static struct ADVANCED_DLG * popup_diplomatic_objects(struct player *pPlayer0,
  				struct player *pPlayer1,
  				struct widget *pMainWindow, bool L_R)
{
  struct ADVANCED_DLG *pDlg = fc_calloc(1, sizeof(struct ADVANCED_DLG));
  struct CONTAINER *pCont = fc_calloc(1, sizeof(struct CONTAINER));
  int width, height, count = 0, scroll_w = 0;
  char cBuf[128];
  struct widget *pBuf = NULL, *pWindow;
  SDL_String16 *pStr;
  int window_x = 0, window_y = 0;
  SDL_Rect area;
  
  enum diplstate_type type =
		  pplayer_get_diplstate(pPlayer0, pPlayer1)->type;
  
  pCont->id0 = player_number(pPlayer0);
  pCont->id1 = player_number(pPlayer1);
  
  pStr = create_str16_from_char(nation_adjective_for_player(pPlayer0), adj_font(12));
  pStr->style |= TTF_STYLE_BOLD;

  pWindow = create_window_skeleton(NULL, pStr, WF_FREE_DATA);

  pWindow->action = dipomatic_window_callback;
  set_wstate(pWindow, FC_WS_NORMAL);
  
  pDlg->pEndWidgetList = pWindow;
  pWindow->data.cont = pCont;
  add_to_gui_list(ID_WINDOW, pWindow);

  area = pWindow->area;
  
  /* ============================================================= */
  width = 0;
  height = 0;
  
  /* Pacts. */
  if (pPlayer0 == client.conn.playing && DS_ALLIANCE != type) {
    
    pBuf = create_iconlabel_from_chars(NULL, pWindow->dst,
			  _("Pacts"), adj_font(12), WF_RESTORE_BACKGROUND);
    pBuf->string16->fgcol = *get_game_colorRGB(COLOR_THEME_DIPLODLG_MEETING_HEADING_TEXT);
    width = pBuf->size.w;
    height = pBuf->size.h;
    add_to_gui_list(ID_LABEL, pBuf);
    count++;
    
    /*if(type == DS_WAR || type == DS_NEUTRAL) {*/
    if(type != DS_CEASEFIRE) {
      my_snprintf(cBuf, sizeof(cBuf), "  %s", Q_("?diplomatic_state:Cease-fire"));
      pBuf = create_iconlabel_from_chars(NULL, pWindow->dst,
	cBuf, adj_font(12), (WF_RESTORE_BACKGROUND|WF_DRAW_TEXT_LABEL_WITH_SPACE));
      pBuf->string16->fgcol = *get_game_colorRGB(COLOR_THEME_DIPLODLG_MEETING_TEXT);
      width = MAX(width, pBuf->size.w);
      height = MAX(height, pBuf->size.h);
      pBuf->action = pact_callback;
      pBuf->data.cont = pCont;
      set_wstate(pBuf, FC_WS_NORMAL);
      add_to_gui_list(MAX_ID - 2, pBuf);
      count++;
    }
    
    if(type != DS_PEACE) {
      my_snprintf(cBuf, sizeof(cBuf), "  %s", Q_("?diplomatic_state:Peace"));
  
      pBuf = create_iconlabel_from_chars(NULL, pWindow->dst,
	cBuf, adj_font(12), (WF_RESTORE_BACKGROUND|WF_DRAW_TEXT_LABEL_WITH_SPACE));
      pBuf->string16->fgcol = *get_game_colorRGB(COLOR_THEME_DIPLODLG_MEETING_TEXT);
      width = MAX(width, pBuf->size.w);
      height = MAX(height, pBuf->size.h);
      pBuf->action = pact_callback;
      pBuf->data.cont = pCont;
      set_wstate(pBuf, FC_WS_NORMAL);
      add_to_gui_list(MAX_ID - 1, pBuf);
      count++;
    }
    
    if(pplayer_can_make_treaty(pPlayer0, pPlayer1, DS_ALLIANCE)) {
      my_snprintf(cBuf, sizeof(cBuf), "  %s", Q_("?diplomatic_state:Alliance"));
      
      pBuf = create_iconlabel_from_chars(NULL, pWindow->dst,
	cBuf, adj_font(12), (WF_RESTORE_BACKGROUND|WF_DRAW_TEXT_LABEL_WITH_SPACE));
      pBuf->string16->fgcol = *get_game_colorRGB(COLOR_THEME_DIPLODLG_MEETING_TEXT);
      width = MAX(width, pBuf->size.w);
      height = MAX(height, pBuf->size.h);
      pBuf->action = pact_callback;
      pBuf->data.cont = pCont;
      set_wstate(pBuf, FC_WS_NORMAL);
      add_to_gui_list(MAX_ID, pBuf);
      count++;
    }
    
  }
  
  /* ---------------------------- */
  if (!gives_shared_vision(pPlayer0, pPlayer1)) {
    
    pBuf = create_iconlabel_from_chars(NULL, pWindow->dst,
	_("Give shared vision"), adj_font(12),
    		(WF_RESTORE_BACKGROUND|WF_DRAW_TEXT_LABEL_WITH_SPACE));
    pBuf->string16->fgcol = *get_game_colorRGB(COLOR_THEME_DIPLODLG_MEETING_TEXT);
    width = MAX(width, pBuf->size.w);
    height = MAX(height, pBuf->size.h);
    pBuf->action = vision_callback;
    pBuf->data.cont = pCont;
    set_wstate(pBuf, FC_WS_NORMAL);
    add_to_gui_list(ID_LABEL, pBuf);
    count++;
    
    /* ---------------------------- */
    /* you can't give maps if you give shared vision */
    pBuf = create_iconlabel_from_chars(NULL, pWindow->dst,
		_("Maps"), adj_font(12), WF_RESTORE_BACKGROUND);
    pBuf->string16->fgcol = *get_game_colorRGB(COLOR_THEME_DIPLODLG_MEETING_HEADING_TEXT);
    width = MAX(width, pBuf->size.w);
    height = MAX(height, pBuf->size.h);
    add_to_gui_list(ID_LABEL, pBuf);
    count++;
    
    /* ----- */
    my_snprintf(cBuf, sizeof(cBuf), "  %s", _("World map"));
  
    pBuf = create_iconlabel_from_chars(NULL, pWindow->dst,
	cBuf, adj_font(12), (WF_RESTORE_BACKGROUND|WF_DRAW_TEXT_LABEL_WITH_SPACE));
    pBuf->string16->fgcol = *get_game_colorRGB(COLOR_THEME_DIPLODLG_MEETING_TEXT);
    width = MAX(width, pBuf->size.w);
    height = MAX(height, pBuf->size.h);
    pBuf->action = maps_callback;
    pBuf->data.cont = pCont;
    set_wstate(pBuf, FC_WS_NORMAL);
    add_to_gui_list(MAX_ID - 1, pBuf);
    count++;
    
    /* ----- */
    my_snprintf(cBuf, sizeof(cBuf), "  %s", _("Sea map"));
  
    pBuf = create_iconlabel_from_chars(NULL, pWindow->dst,
	cBuf, adj_font(12), (WF_RESTORE_BACKGROUND|WF_DRAW_TEXT_LABEL_WITH_SPACE));
    pBuf->string16->fgcol = *get_game_colorRGB(COLOR_THEME_DIPLODLG_MEETING_TEXT);
    width = MAX(width, pBuf->size.w);
    height = MAX(height, pBuf->size.h);
    pBuf->action = maps_callback;
    pBuf->data.cont = pCont;
    set_wstate(pBuf, FC_WS_NORMAL);
    add_to_gui_list(MAX_ID, pBuf);
    count++;
  }
  
  /* Don't take in account the embassy effects. */
  if (!BV_ISSET(pPlayer1->embassy, player_index(pPlayer0))) {  
    pBuf = create_iconlabel_from_chars(NULL, pWindow->dst,
        _("Give embassy"), adj_font(12),
                (WF_RESTORE_BACKGROUND|WF_DRAW_TEXT_LABEL_WITH_SPACE));
    pBuf->string16->fgcol = *get_game_colorRGB(COLOR_THEME_DIPLODLG_MEETING_TEXT);
    width = MAX(width, pBuf->size.w);
    height = MAX(height, pBuf->size.h);
    pBuf->action = embassy_callback;
    pBuf->data.cont = pCont;
    set_wstate(pBuf, FC_WS_NORMAL);
    add_to_gui_list(ID_LABEL, pBuf);
    count++;
  }
    
  /* ---------------------------- */
  if(pPlayer0->economic.gold > 0) {
    pCont->value = pPlayer0->economic.gold;
    
    my_snprintf(cBuf, sizeof(cBuf), _("Gold(max %d)"), pPlayer0->economic.gold);
    pBuf = create_iconlabel_from_chars(NULL, pWindow->dst,
			  cBuf, adj_font(12), WF_RESTORE_BACKGROUND);
    pBuf->string16->fgcol = *get_game_colorRGB(COLOR_THEME_DIPLODLG_MEETING_HEADING_TEXT);
    width = MAX(width, pBuf->size.w);
    height = MAX(height, pBuf->size.h);
    add_to_gui_list(ID_LABEL, pBuf);
    count++;
    
    pBuf = create_edit(NULL, pWindow->dst,
    	create_str16_from_char("0", adj_font(10)), 0,
    		(WF_RESTORE_BACKGROUND|WF_FREE_STRING));
    pBuf->data.cont = pCont;
    pBuf->action = gold_callback;
    set_wstate(pBuf, FC_WS_NORMAL);
    width = MAX(width, pBuf->size.w);
    height = MAX(height, pBuf->size.h);
    add_to_gui_list(ID_LABEL, pBuf);
    count++;
  }
  /* ---------------------------- */
  
  /* Advances */
  {
    int flag = A_NONE;
    
    advance_index_iterate(A_FIRST, i) {
      if (player_invention_state(pPlayer0, i) == TECH_KNOWN &&
         player_invention_reachable(pPlayer1, i) &&
	(player_invention_state(pPlayer1, i) == TECH_UNKNOWN || 
	 player_invention_state(pPlayer1, i) == TECH_PREREQS_KNOWN)) {
	     
	     pBuf = create_iconlabel_from_chars(NULL, pWindow->dst,
		_("Advances"), adj_font(12), WF_RESTORE_BACKGROUND);
             pBuf->string16->fgcol = *get_game_colorRGB(COLOR_THEME_DIPLODLG_MEETING_HEADING_TEXT);
	     width = MAX(width, pBuf->size.w);
	     height = MAX(height, pBuf->size.h);
             add_to_gui_list(ID_LABEL, pBuf);
	     count++;
	     
	     my_snprintf(cBuf, sizeof(cBuf), "  %s", advance_name_translation(advance_by_number(i)));
  
             pBuf = create_iconlabel_from_chars(NULL, pWindow->dst, cBuf, adj_font(12),
	         (WF_RESTORE_BACKGROUND|WF_DRAW_TEXT_LABEL_WITH_SPACE));
             pBuf->string16->fgcol = *get_game_colorRGB(COLOR_THEME_DIPLODLG_MEETING_TEXT);
	     width = MAX(width, pBuf->size.w);
	     height = MAX(height, pBuf->size.h);
	     pBuf->action = techs_callback;
	     set_wstate(pBuf, FC_WS_NORMAL);
	     pBuf->data.cont = pCont;
             add_to_gui_list(MAX_ID - i, pBuf);
	     count++;	
	     flag = ++i;
	     break;
      }
    } advance_index_iterate_end;
    
    if(flag > A_NONE) {
      advance_index_iterate(flag, i) {
	if (player_invention_state(pPlayer0, i) == TECH_KNOWN &&
	   player_invention_reachable(pPlayer1, i) &&
	  (player_invention_state(pPlayer1, i) == TECH_UNKNOWN || 
	   player_invention_state(pPlayer1, i) == TECH_PREREQS_KNOWN)) {
	     
	     my_snprintf(cBuf, sizeof(cBuf), "  %s", advance_name_translation(advance_by_number(i)));
  
             pBuf = create_iconlabel_from_chars(NULL, pWindow->dst, cBuf, adj_font(12),
	         (WF_RESTORE_BACKGROUND|WF_DRAW_TEXT_LABEL_WITH_SPACE));
             pBuf->string16->fgcol = *get_game_colorRGB(COLOR_THEME_DIPLODLG_MEETING_TEXT);
	     width = MAX(width, pBuf->size.w);
	     height = MAX(height, pBuf->size.h);
	     pBuf->action = techs_callback;
	     set_wstate(pBuf, FC_WS_NORMAL);
             pBuf->data.cont = pCont;
             add_to_gui_list(MAX_ID - i, pBuf);
	     count++;
	}
      }
    } advance_index_iterate_end;
    
  }  /* Advances */
  
  /* Cities */
  /****************************************************************
  Creates a sorted list of pPlayer0's cities, excluding the capital and
  any cities not visible to pPlayer1.  This means that you can only trade 
  cities visible to requesting player.  

			      - Kris Bubendorfer
  *****************************************************************/
  {
    int i = 0, j = 0, n = city_list_size(pPlayer0->cities);
    struct city **city_list_ptrs;

    if (n > 0) {
      city_list_ptrs = fc_calloc(1, sizeof(struct city *) * n);
      city_list_iterate(pPlayer0->cities, pCity) {
        if (!is_capital(pCity)) {
	  city_list_ptrs[i] = pCity;
	  i++;
        }
      } city_list_iterate_end;	
    } else {
      city_list_ptrs = NULL;
    }
    

    if(i > 0) {
      
      pBuf = create_iconlabel_from_chars(NULL, pWindow->dst,
		_("Cities"), adj_font(12), WF_RESTORE_BACKGROUND);
      pBuf->string16->fgcol = *get_game_colorRGB(COLOR_THEME_DIPLODLG_MEETING_HEADING_TEXT);
      pBuf->string16->style &= ~SF_CENTER;
      width = MAX(width, pBuf->size.w);
      height = MAX(height, pBuf->size.h);
      add_to_gui_list(ID_LABEL, pBuf);
      count++;
      
      qsort(city_list_ptrs, i, sizeof(struct city *), city_name_compare);
        
      for (j = 0; j < i; j++) {
	my_snprintf(cBuf, sizeof(cBuf), "  %s", city_name(city_list_ptrs[j]));
  
        pBuf = create_iconlabel_from_chars(NULL, pWindow->dst, cBuf, adj_font(12),
	     (WF_RESTORE_BACKGROUND|WF_DRAW_TEXT_LABEL_WITH_SPACE));
        pBuf->string16->fgcol = *get_game_colorRGB(COLOR_THEME_DIPLODLG_MEETING_TEXT);
	width = MAX(width, pBuf->size.w);
	height = MAX(height, pBuf->size.h);
        pBuf->data.cont = pCont;
	pBuf->action = cities_callback;
	set_wstate(pBuf, FC_WS_NORMAL);
	/* MAX_ID is unigned short type range and city id must be in this range */
	assert(city_list_ptrs[j]->id <= MAX_ID);
        add_to_gui_list(MAX_ID - city_list_ptrs[j]->id, pBuf);
	count++;      
      }
    }
    FC_FREE(city_list_ptrs);    
  } /* Cities */
  
  pDlg->pBeginWidgetList = pBuf;
  pDlg->pBeginActiveWidgetList = pDlg->pBeginWidgetList;
  pDlg->pEndActiveWidgetList = pDlg->pEndWidgetList->prev;
  pDlg->pScroll = NULL;
  
  area.h = (Main.screen->h - adj_size(100) - (pWindow->size.h - pWindow->area.h));
  
  if (area.h < (count * height)) {
    pDlg->pActiveWidgetList = pDlg->pEndActiveWidgetList;
    count = area.h / height;
    scroll_w = create_vertical_scrollbar(pDlg, 1, count, TRUE, TRUE);
    pBuf = pWindow;
    /* hide not seen widgets */
    do {
      pBuf = pBuf->prev;
      if(count) {
	count--;
      } else {
	set_wflag(pBuf, WF_HIDDEN);
      }
    } while(pBuf != pDlg->pBeginActiveWidgetList);
  }
  
  area.w = MAX(width + adj_size(4) + scroll_w, adj_size(150) - (pWindow->size.w - pWindow->area.w));

  resize_window(pWindow, NULL, get_game_colorRGB(COLOR_THEME_BACKGROUND),
                (pWindow->size.w - pWindow->area.w) + area.w,
                (pWindow->size.h - pWindow->area.h) + area.h);

  area = pWindow->area;
  
  if(L_R) {
    window_x = pMainWindow->dst->dest_rect.x + pMainWindow->size.w + adj_size(20);
  } else {
    window_x = pMainWindow->dst->dest_rect.x - adj_size(20) - pWindow->size.w;
  }
  window_y = (Main.screen->h - pWindow->size.h) / 2;
  
  widget_set_position(pWindow, window_x, window_y);
  
  setup_vertical_widgets_position(1,
     area.x + adj_size(2), area.y + 1,
     width, height, pDlg->pBeginActiveWidgetList, pDlg->pEndActiveWidgetList);
  
  if(pDlg->pScroll) {
    setup_vertical_scrollbar_area(pDlg->pScroll,
	area.x + area.w,
    	area.y,
    	area.h, TRUE);
  }
  
  return pDlg;
}

/**************************************************************************
  ...
**************************************************************************/
static struct diplomacy_dialog *create_diplomacy_dialog(struct player *plr0, 
							struct player *plr1)
{
  struct diplomacy_dialog *pdialog = fc_calloc(1, sizeof(struct diplomacy_dialog));

  init_treaty(&pdialog->treaty, plr0, plr1);
  
  pdialog->pdialog = fc_calloc(1, sizeof(struct ADVANCED_DLG));
    
  dialog_list_prepend(dialog_list, pdialog);  
  
  return pdialog;
}

/****************************************************************
...
*****************************************************************/
static void update_diplomacy_dialog(struct diplomacy_dialog *pdialog)
{
  SDL_Color bg_color = {255, 255, 255, 136};
  
  struct player *pPlayer0, *pPlayer1;
  struct CONTAINER *pCont = fc_calloc(1, sizeof(struct CONTAINER));
  char cBuf[128];
  struct widget *pBuf = NULL, *pWindow;
  SDL_String16 *pStr;
  SDL_Rect dst;
  SDL_Rect area;
  
  if(pdialog) {
    
    /* delete old content */
    if (pdialog->pdialog->pEndWidgetList) {
      popdown_window_group_dialog(pdialog->poffers->pBeginWidgetList,
                                  pdialog->poffers->pEndWidgetList);
      FC_FREE(pdialog->poffers->pScroll);
      FC_FREE(pdialog->poffers);
      
      popdown_window_group_dialog(pdialog->pwants->pBeginWidgetList,
                                  pdialog->pwants->pEndWidgetList);
      FC_FREE(pdialog->pwants->pScroll);
      FC_FREE(pdialog->pwants);
      
      popdown_window_group_dialog(pdialog->pdialog->pBeginWidgetList,
                                            pdialog->pdialog->pEndWidgetList);
    }
   
    pPlayer0 = pdialog->treaty.plr0;
    pPlayer1 = pdialog->treaty.plr1;

    pCont->id0 = player_number(pPlayer0);
    pCont->id1 = player_number(pPlayer1);
    
    my_snprintf(cBuf, sizeof(cBuf), _("Diplomacy meeting"));
    
    pStr = create_str16_from_char(cBuf, adj_font(12));
    pStr->style |= TTF_STYLE_BOLD;

    pWindow = create_window_skeleton(NULL, pStr, 0);

    pWindow->action = dipomatic_window_callback;
    set_wstate(pWindow, FC_WS_NORMAL);
    pWindow->data.cont = pCont;
    pdialog->pdialog->pEndWidgetList = pWindow;

    add_to_gui_list(ID_WINDOW, pWindow);

    /* ============================================================= */
  
    pStr = create_str16_from_char(nation_adjective_for_player(pPlayer0), adj_font(12));
    pStr->style |= (TTF_STYLE_BOLD|SF_CENTER);
    pStr->fgcol = *get_game_colorRGB(COLOR_THEME_DIPLODLG_MEETING_TEXT);
    
    pBuf = create_iconlabel(
    	create_icon_from_theme(pTheme->CANCEL_PACT_Icon, 0),
		pWindow->dst, pStr,
		(WF_ICON_ABOVE_TEXT|WF_FREE_PRIVATE_DATA|WF_FREE_THEME|
						WF_RESTORE_BACKGROUND));
						
    pBuf->private_data.cbox = fc_calloc(1, sizeof(struct CHECKBOX));
    pBuf->private_data.cbox->state = FALSE;
    pBuf->private_data.cbox->pTRUE_Theme = pTheme->OK_PACT_Icon;
    pBuf->private_data.cbox->pFALSE_Theme = pTheme->CANCEL_PACT_Icon;
    
    add_to_gui_list(ID_ICON, pBuf);
    
    pStr = create_str16_from_char(nation_adjective_for_player(pPlayer1), adj_font(12));
    pStr->style |= (TTF_STYLE_BOLD|SF_CENTER);
    pStr->fgcol = *get_game_colorRGB(COLOR_THEME_DIPLODLG_MEETING_TEXT);
    
    pBuf = create_iconlabel(
    	create_icon_from_theme(pTheme->CANCEL_PACT_Icon, 0),
		pWindow->dst, pStr,
		(WF_ICON_ABOVE_TEXT|WF_FREE_PRIVATE_DATA|WF_FREE_THEME|
    						WF_RESTORE_BACKGROUND));
    pBuf->private_data.cbox = fc_calloc(1, sizeof(struct CHECKBOX));
    pBuf->private_data.cbox->state = FALSE;
    pBuf->private_data.cbox->pTRUE_Theme = pTheme->OK_PACT_Icon;
    pBuf->private_data.cbox->pFALSE_Theme = pTheme->CANCEL_PACT_Icon;
    add_to_gui_list(ID_ICON, pBuf);
    /* ============================================================= */
    
    pBuf = create_themeicon(pTheme->CANCEL_PACT_Icon, pWindow->dst, 
    	(WF_WIDGET_HAS_INFO_LABEL|WF_FREE_STRING|WF_RESTORE_BACKGROUND));
	
    pBuf->string16 = create_str16_from_char(_("Cancel meeting"), adj_font(12));
    
    pBuf->action = cancel_meeting_callback;
    pBuf->data.cont = pCont;
    set_wstate(pBuf, FC_WS_NORMAL);
    
    add_to_gui_list(ID_ICON, pBuf);
    
    pBuf = create_themeicon(pTheme->OK_PACT_Icon, pWindow->dst, 
    	(WF_FREE_DATA|WF_WIDGET_HAS_INFO_LABEL|
				WF_FREE_STRING|WF_RESTORE_BACKGROUND));
	
    pBuf->string16 = create_str16_from_char(_("Accept treaty"), adj_font(12));
    
    pBuf->action = accept_treaty_callback;
    pBuf->data.cont = pCont;
    set_wstate(pBuf, FC_WS_NORMAL);
    
    add_to_gui_list(ID_ICON, pBuf);
    /* ============================================================= */
    
    pdialog->pdialog->pBeginWidgetList = pBuf;
    
    create_vertical_scrollbar(pdialog->pdialog, 1, 7, TRUE, TRUE);
    hide_scrollbar(pdialog->pdialog->pScroll);
    
    /* ============================================================= */

    resize_window(pWindow, NULL, get_game_colorRGB(COLOR_THEME_BACKGROUND),
                  adj_size(250), adj_size(300));

    area = pWindow->area;

    widget_set_position(pWindow,
                        (Main.screen->w - pWindow->size.w) / 2,
                        (Main.screen->h - pWindow->size.h) / 2);

    pBuf = pWindow->prev;
    pBuf->size.x = area.x + adj_size(17);
    pBuf->size.y = area.y + adj_size(6);
    
    dst.y = area.y + adj_size(6) + pBuf->size.h + adj_size(10);
    dst.x = adj_size(10);
    dst.w = area.w - adj_size(14);
        
    pBuf = pBuf->prev;
    pBuf->size.x = area.x + area.w - pBuf->size.w - adj_size(20);
    pBuf->size.y = area.y + adj_size(6);
    
    pBuf = pBuf->prev;
    pBuf->size.x = area.x + (area.w - (2 * pBuf->size.w + adj_size(40))) / 2;
    pBuf->size.y = area.y + area.h - pBuf->size.w - adj_size(17);
    
    pBuf = pBuf->prev;
    pBuf->size.x = pBuf->next->size.x + pBuf->next->size.w + adj_size(40);
    pBuf->size.y = area.y + area.h - pBuf->size.w - adj_size(17);
    
    dst.h = area.h - pBuf->size.w - adj_size(3) - dst.y;
    /* ============================================================= */
    
    SDL_FillRectAlpha(pWindow->theme, &dst, &bg_color);
    
    /* ============================================================= */
    setup_vertical_scrollbar_area(pdialog->pdialog->pScroll,
	area.x + dst.x + dst.w,
    	dst.y,
    	dst.h, TRUE);
    /* ============================================================= */
    pdialog->poffers = popup_diplomatic_objects(pPlayer0, pPlayer1, pWindow, FALSE);
    
    pdialog->pwants = popup_diplomatic_objects(pPlayer1, pPlayer0, pWindow, TRUE);
    /* ============================================================= */
    /* redraw */
    redraw_group(pdialog->pdialog->pBeginWidgetList, pWindow, 0);
    widget_mark_dirty(pWindow);
    
    redraw_group(pdialog->poffers->pBeginWidgetList, pdialog->poffers->pEndWidgetList, 0);
    widget_mark_dirty(pdialog->poffers->pEndWidgetList);
    
    redraw_group(pdialog->pwants->pBeginWidgetList, pdialog->pwants->pEndWidgetList, 0);
    widget_mark_dirty(pdialog->pwants->pEndWidgetList);
    
    flush_dirty();
  }
}

/****************************************************************
...
*****************************************************************/
static void update_acceptance_icons(struct diplomacy_dialog *pdialog)
{
  struct widget *pLabel;
  SDL_Surface *pThm;
  SDL_Rect src = {0, 0, 0, 0};

  /* updates your own acceptance status */
  pLabel = pdialog->pdialog->pEndWidgetList->prev;

  pLabel->private_data.cbox->state = pdialog->treaty.accept0;  
  if (pLabel->private_data.cbox->state) {
    pThm = pLabel->private_data.cbox->pTRUE_Theme;
  } else {
    pThm = pLabel->private_data.cbox->pFALSE_Theme;
  }
      
  src.w = pThm->w / 4;
  src.h = pThm->h;
    
  alphablit(pThm, &src, pLabel->theme, NULL);
  SDL_SetAlpha(pThm, SDL_SRCALPHA, 255);
  
  widget_redraw(pLabel);
  widget_flush(pLabel);
  
  /* updates other player's acceptance status */
  pLabel = pdialog->pdialog->pEndWidgetList->prev->prev;
  
  pLabel->private_data.cbox->state = pdialog->treaty.accept1;  
  if (pLabel->private_data.cbox->state) {
    pThm = pLabel->private_data.cbox->pTRUE_Theme;
  } else {
    pThm = pLabel->private_data.cbox->pFALSE_Theme;
  }
      
  src.w = pThm->w / 4;
  src.h = pThm->h;
    
  alphablit(pThm, &src, pLabel->theme, NULL);
  
  widget_redraw(pLabel);
  widget_flush(pLabel);
}

/****************************************************************
...
*****************************************************************/
static void update_clauses_list(struct diplomacy_dialog *pdialog) {
  SDL_String16 *pStr;
  struct widget *pBuf, *pWindow = pdialog->pdialog->pEndWidgetList;
  char cBuf[64];
  bool redraw_all, scroll = (pdialog->pdialog->pActiveWidgetList != NULL);
  int len = pdialog->pdialog->pScroll->pUp_Left_Button->size.w;
  
  clause_list_iterate(pdialog->treaty.clauses, pclause) {

    client_diplomacy_clause_string(cBuf, sizeof(cBuf), pclause);
    
    pStr = create_str16_from_char(cBuf, adj_font(12));
    pBuf = create_iconlabel(NULL, pWindow->dst, pStr,
     (WF_FREE_DATA|WF_DRAW_TEXT_LABEL_WITH_SPACE|WF_RESTORE_BACKGROUND));
        
    if (pclause->from != client.conn.playing) {
       pBuf->string16->style |= SF_CENTER_RIGHT;  
    }
  
    pBuf->data.cont = fc_calloc(1, sizeof(struct CONTAINER));
    pBuf->data.cont->id0 = player_number(pclause->from);
    pBuf->data.cont->id1 = player_number(pdialog->treaty.plr1);
    pBuf->data.cont->value = ((int)pclause->type << 16) + pclause->value;
    
    pBuf->action = remove_clause_callback;
    set_wstate(pBuf, FC_WS_NORMAL);
    
    pBuf->size.w = pWindow->size.w - adj_size(24) - (scroll ? len : 0);
    
    redraw_all = add_widget_to_vertical_scroll_widget_list(pdialog->pdialog,
                  pBuf, pdialog->pdialog->pBeginWidgetList,
                  FALSE,
                  pWindow->size.x + adj_size(12),
                  pdialog->pdialog->pScroll->pUp_Left_Button->size.y + adj_size(2));

    if(!scroll && (pdialog->pdialog->pActiveWidgetList != NULL)) {
      /* -> the scrollbar has been activated */
      pBuf = pdialog->pdialog->pEndActiveWidgetList->next;
      do {
        pBuf = pBuf->prev;
        pBuf->size.w -= len;
        /* we need to save a new background because the width has changed */
        FREESURFACE(pBuf->gfx);
      } while(pBuf != pdialog->pdialog->pBeginActiveWidgetList);
      scroll = TRUE;
    }

    /* redraw */
    if(redraw_all) {
      redraw_group(pdialog->pdialog->pBeginWidgetList, pWindow, 0);
      widget_mark_dirty(pWindow);
    } else {
      widget_redraw(pBuf);
      widget_mark_dirty(pBuf);
    }
    
  } clause_list_iterate_end;
  
  flush_dirty();
}

/****************************************************************
...
*****************************************************************/
static void remove_clause_widget_from_list(int counterpart, int giver,
                                           enum clause_type type, int value)
{
  struct widget *pBuf;
  SDL_Rect src = {0, 0, 0, 0};
  bool scroll = TRUE;

  struct diplomacy_dialog *pdialog = get_diplomacy_dialog(counterpart);  
  
  /* find widget with clause */
  pBuf = pdialog->pdialog->pEndActiveWidgetList->next;
  
  do {
    pBuf = pBuf->prev;
  } while(!((pBuf->data.cont->id0 == giver) &&
            (((pBuf->data.cont->value >> 16) & 0xFFFF) == (int)type) &&
            ((pBuf->data.cont->value & 0xFFFF) == value)) &&
          (pBuf != pdialog->pdialog->pBeginActiveWidgetList));
  
  if(!(pBuf->data.cont->id0 == giver &&
            ((pBuf->data.cont->value >> 16) & 0xFFFF) == (int)type &&
            (pBuf->data.cont->value & 0xFFFF) == value)) {
     return;
  }
  
  scroll = (pdialog->pdialog->pActiveWidgetList != NULL);
  del_widget_from_vertical_scroll_widget_list(pdialog->pdialog, pBuf);

  if(scroll && (pdialog->pdialog->pActiveWidgetList == NULL)) {
    /* -> the scrollbar has been deactivated */
    
    int len = pdialog->pdialog->pScroll->pUp_Left_Button->size.w;
    pBuf = pdialog->pdialog->pEndActiveWidgetList->next;
    do {
      pBuf = pBuf->prev;
      widget_undraw(pBuf);
      pBuf->size.w += len;
      /* we need to save a new background because the width has changed */
      FREESURFACE(pBuf->gfx);
    } while(pBuf != pdialog->pdialog->pBeginActiveWidgetList);
    scroll = FALSE;
  }
    
  /* update state icons */
  pBuf = pdialog->pdialog->pEndWidgetList->prev;
  if(pBuf->private_data.cbox->state) {
    pBuf->private_data.cbox->state = FALSE;
    src.w = pBuf->private_data.cbox->pFALSE_Theme->w / 4;
    src.h = pBuf->private_data.cbox->pFALSE_Theme->h;
    
    alphablit(pBuf->private_data.cbox->pFALSE_Theme, &src, pBuf->theme, NULL);
  }
  
}

/**************************************************************************
  Handle the start of a diplomacy meeting - usually by poping up a
  diplomacy dialog.
**************************************************************************/
void handle_diplomacy_init_meeting(int counterpart, int initiated_from)
{
  struct diplomacy_dialog *pdialog;

  if (!can_client_issue_orders()) {
    return;
  }

  if (client.conn.playing->ai_data.control) {
    return;			/* Don't show if we are AI controlled. */
  }

  if (!(pdialog = get_diplomacy_dialog(counterpart))) {
    pdialog = create_diplomacy_dialog(client.conn.playing,
				player_by_number(counterpart));
  } else {
    /* bring existing dialog to front */
    sellect_window_group_dialog(pdialog->pdialog->pBeginWidgetList,
                                         pdialog->pdialog->pEndWidgetList);
  }

  update_diplomacy_dialog(pdialog);
}

/**************************************************************************
  ...
**************************************************************************/
static void popdown_diplomacy_dialog(int counterpart)
{
  struct diplomacy_dialog *pdialog = get_diplomacy_dialog(counterpart);
    
  if (pdialog) {
    popdown_window_group_dialog(pdialog->poffers->pBeginWidgetList,
			        pdialog->poffers->pEndWidgetList);
    FC_FREE(pdialog->poffers->pScroll);
    FC_FREE(pdialog->poffers);
    
    popdown_window_group_dialog(pdialog->pwants->pBeginWidgetList,
			        pdialog->pwants->pEndWidgetList);
    FC_FREE(pdialog->pwants->pScroll);
    FC_FREE(pdialog->pwants);
    
    popdown_window_group_dialog(pdialog->pdialog->pBeginWidgetList,
			                  pdialog->pdialog->pEndWidgetList);
      
    dialog_list_unlink(dialog_list, pdialog);
      
    FC_FREE(pdialog->pdialog->pScroll);
    FC_FREE(pdialog->pdialog);  
    FC_FREE(pdialog);
  }
}

/**************************************************************************
  Popdown all diplomacy dialogs
**************************************************************************/
static void popdown_diplomacy_dialogs()
{
  dialog_list_iterate(dialog_list, pdialog) {
    popdown_diplomacy_dialog(player_number(pdialog->treaty.plr1));
  } dialog_list_iterate_end;
}

/**************************************************************************
  Close all open diplomacy dialogs, for when client disconnects from game.
**************************************************************************/
void close_all_diplomacy_dialogs(void)
{
  popdown_sdip_dialog();
  popdown_diplomacy_dialogs();
}

/* ================================================================= */
/* ========================== Small Diplomat Dialog ================ */
/* ================================================================= */
static struct SMALL_DLG *pSDip_Dlg = NULL;

static void popdown_sdip_dialog(void)
{
  if (pSDip_Dlg) {
    popdown_window_group_dialog(pSDip_Dlg->pBeginWidgetList,
			      pSDip_Dlg->pEndWidgetList);
    FC_FREE(pSDip_Dlg);
  }
}

static int sdip_window_callback(struct widget *pWindow)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    move_window_group(pSDip_Dlg->pBeginWidgetList, pWindow);
  }
  return -1;
}

static int withdraw_vision_dlg_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    popdown_sdip_dialog();
  
    dsend_packet_diplomacy_cancel_pact(&client.conn,
                                       player_number(pWidget->data.player),
                                       CLAUSE_VISION);
    
    flush_dirty();
  }
  return -1;
}

static int cancel_pact_dlg_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    popdown_sdip_dialog();
  
    dsend_packet_diplomacy_cancel_pact(&client.conn,
                                       player_number(pWidget->data.player),
                                       CLAUSE_CEASEFIRE);
    
    flush_dirty();
  }
  return -1;
}

static int call_meeting_dlg_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    popdown_sdip_dialog();
    
    dsend_packet_diplomacy_init_meeting_req(&client.conn,
                                            player_number(pWidget->data.player));
    
    flush_dirty();
  }
  return -1;
}


static int cancel_sdip_dlg_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {  
    popdown_sdip_dialog();
    flush_dirty();
  }
  return -1;
}

static void popup_war_dialog(struct player *pPlayer)
{
  char cBuf[128];
  struct widget *pBuf = NULL, *pWindow;
  SDL_String16 *pStr;
  SDL_Surface *pText;
  SDL_Rect dst;
  SDL_Rect area;
  
  if (pSDip_Dlg) {
    return;
  }
  
  pSDip_Dlg = fc_calloc(1, sizeof(struct SMALL_DLG));

  my_snprintf(cBuf, sizeof(cBuf),
              /* TRANS: "Polish incident !" FIXME!!! */
              _("%s incident !"),
              nation_adjective_for_player(pPlayer));
  pStr = create_str16_from_char(cBuf, adj_font(12));
  pStr->style |= TTF_STYLE_BOLD;

  pWindow = create_window_skeleton(NULL, pStr, 0);

  pWindow->action = sdip_window_callback;
  set_wstate(pWindow, FC_WS_NORMAL);

  add_to_gui_list(ID_WINDOW, pWindow);

  pSDip_Dlg->pEndWidgetList = pWindow;
  
  area = pWindow->area;
  
  /* ============================================================= */
  /* label */
  my_snprintf(cBuf, sizeof(cBuf), _("Shall we declare WAR on them?"));
  
  pStr = create_str16_from_char(cBuf, adj_font(14));
  pStr->style |= (TTF_STYLE_BOLD|SF_CENTER);
  pStr->fgcol = *get_game_colorRGB(COLOR_THEME_WARDLG_TEXT);

  pText = create_text_surf_from_str16(pStr);
  FREESTRING16(pStr);
  area.w = MAX(area.w, pText->w);
  area.h += pText->h + adj_size(10);


  pBuf = create_themeicon_button_from_chars(pTheme->CANCEL_Icon,
			    pWindow->dst, _("No"), adj_font(12), 0);

  pBuf->action = cancel_sdip_dlg_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->key = SDLK_ESCAPE;
  area.h += pBuf->size.h;

  add_to_gui_list(ID_BUTTON, pBuf);

  pBuf = create_themeicon_button_from_chars(pTheme->OK_Icon, pWindow->dst,
					      _("Yes"), adj_font(12), 0);

  pBuf->action = cancel_pact_dlg_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->data.player = pPlayer;
  pBuf->key = SDLK_RETURN;
  add_to_gui_list(ID_BUTTON, pBuf);
  pBuf->size.w = MAX(pBuf->next->size.w, pBuf->size.w);
  pBuf->next->size.w = pBuf->size.w;
  area.w = MAX(area.w , 2 * pBuf->size.w + adj_size(20));
    
  pSDip_Dlg->pBeginWidgetList = pBuf;
  
  /* setup window size and start position */
  area.w += adj_size(10);

  pBuf = pWindow->prev;
  
  area.h += adj_size(5);

  resize_window(pWindow, NULL, get_game_colorRGB(COLOR_THEME_BACKGROUND),
                (pWindow->size.w - pWindow->area.w) + area.w,
                (pWindow->size.h - pWindow->area.h) + area.h);

  area = pWindow->area;

  widget_set_position(pWindow,
                      (Main.screen->w - pWindow->size.w) / 2,
                      (Main.screen->h - pWindow->size.h) / 2);

  /* setup rest of widgets */
  /* label */
  dst.x = area.x + (area.w - pText->w) / 2;
  dst.y = area.y + 1;
  alphablit(pText, NULL, pWindow->theme, &dst);
  dst.y += pText->h + adj_size(5);
  FREESURFACE(pText);
  
  /* no */
  pBuf = pWindow->prev;
  pBuf->size.y = dst.y;
  
  /* yes */
  pBuf = pBuf->prev;
  pBuf->size.x = area.x + (area.w - (2 * pBuf->size.w + adj_size(20))) / 2;
  pBuf->size.y = dst.y;
    
  /* no */
  pBuf->next->size.x = pBuf->size.x + pBuf->size.w + adj_size(20);
  
  /* ================================================== */
  /* redraw */
  redraw_group(pSDip_Dlg->pBeginWidgetList, pWindow, 0);
  widget_mark_dirty(pWindow);
  flush_dirty();
}

/* ===================================================================== */

void popup_diplomacy_dialog(struct player *pPlayer)
{
  enum diplstate_type type =
		  pplayer_get_diplstate(client.conn.playing, pPlayer)->type;

  if (!can_meet_with_player(pPlayer)) {
    if (DS_WAR == type || pPlayer == client.conn.playing) {
      flush_dirty();
      return;
    } else {
      popup_war_dialog(pPlayer);
      return;
    }
  } else {
    int button_w = 0, button_h = 0;
    char cBuf[128];
    struct widget *pBuf = NULL, *pWindow;
    SDL_String16 *pStr;
    SDL_Surface *pText;
    SDL_Rect dst;
    bool shared = FALSE;
    SDL_Rect area;
    
    if (pSDip_Dlg) {
      return;
    }
  
    pSDip_Dlg = fc_calloc(1, sizeof(struct SMALL_DLG));
          
    my_snprintf(cBuf, sizeof(cBuf),  _("Foreign Minister"));
    pStr = create_str16_from_char(cBuf, adj_font(12));
    pStr->style |= TTF_STYLE_BOLD;

    pWindow = create_window_skeleton(NULL, pStr, 0);

    pWindow->action = sdip_window_callback;
    set_wstate(pWindow, FC_WS_NORMAL);
    pSDip_Dlg->pEndWidgetList = pWindow;

    add_to_gui_list(ID_WINDOW, pWindow);

    area = pWindow->area;
    
    /* ============================================================= */
    /* label */
    my_snprintf(cBuf, sizeof(cBuf), _("Sir!, the %s ambassador has arrived\n"
    		"What are your wishes?"),
    		nation_adjective_for_player(pPlayer));
  
    pStr = create_str16_from_char(cBuf, adj_font(14));
    pStr->style |= (TTF_STYLE_BOLD|SF_CENTER);
    pStr->fgcol = *get_game_colorRGB(COLOR_THEME_DIPLODLG_TEXT);

    pText = create_text_surf_from_str16(pStr);
    FREESTRING16(pStr);
    area.w = MAX(area.w , pText->w);
    area.h += pText->h + adj_size(15);
          
    if(type != DS_WAR && can_client_issue_orders()) {
      
      if(type == DS_ARMISTICE) {
	my_snprintf(cBuf, sizeof(cBuf), _("Declare WAR"));
      } else {
	my_snprintf(cBuf, sizeof(cBuf), _("Cancel Treaty"));
      }
      
      /* cancel treaty */
      pBuf = create_themeicon_button_from_chars(pTheme->UNITS2_Icon,
			pWindow->dst, cBuf, adj_font(12), 0);

      pBuf->action = cancel_pact_dlg_callback;
      set_wstate(pBuf, FC_WS_NORMAL);
      pBuf->string16->fgcol = *get_game_colorRGB(COLOR_THEME_DIPLODLG_MEETING_TEXT);
      pBuf->data.player = pPlayer;
      pBuf->key = SDLK_c;
      add_to_gui_list(ID_BUTTON, pBuf);
      pBuf->size.w = MAX(pBuf->next->size.w, pBuf->size.w);
      pBuf->next->size.w = pBuf->size.w;
      button_w = MAX(button_w , pBuf->size.w);
      button_h = MAX(button_h , pBuf->size.h);

      shared = gives_shared_vision(client.conn.playing, pPlayer);

      if(shared) {
        /* shared vision */
        pBuf = create_themeicon_button_from_chars(pTheme->UNITS2_Icon, pWindow->dst,
					      _("Withdraw vision"), adj_font(12), 0);

        pBuf->action = withdraw_vision_dlg_callback;
        set_wstate(pBuf, FC_WS_NORMAL);
        pBuf->data.player = pPlayer;
        pBuf->key = SDLK_w;
	pBuf->string16->fgcol = *get_game_colorRGB(COLOR_THEME_DIPLODLG_MEETING_TEXT);
        add_to_gui_list(ID_BUTTON, pBuf);
        pBuf->size.w = MAX(pBuf->next->size.w, pBuf->size.w);
        pBuf->next->size.w = pBuf->size.w;
        button_w = MAX(button_w , pBuf->size.w);
        button_h = MAX(button_h , pBuf->size.h);
      }
    }
    
    /* meet */
    pBuf = create_themeicon_button_from_chars(pTheme->PLAYERS_Icon, pWindow->dst,
					      _("Call Diplomatic Meeting"), adj_font(12), 0);

    pBuf->action = call_meeting_dlg_callback;
    set_wstate(pBuf, FC_WS_NORMAL);
    pBuf->data.player = pPlayer;
    pBuf->key = SDLK_m;
    pBuf->string16->fgcol = *get_game_colorRGB(COLOR_THEME_DIPLODLG_MEETING_TEXT);
    add_to_gui_list(ID_BUTTON, pBuf);
    pBuf->size.w = MAX(pBuf->next->size.w, pBuf->size.w);
    pBuf->next->size.w = pBuf->size.w;
    button_w = MAX(button_w , pBuf->size.w);
    button_h = MAX(button_h , pBuf->size.h);

    pBuf = create_themeicon_button_from_chars(pTheme->CANCEL_Icon,
			    pWindow->dst, _("Send him back"), adj_font(12), 0);

    pBuf->action = cancel_sdip_dlg_callback;
    set_wstate(pBuf, FC_WS_NORMAL);
    pBuf->string16->fgcol = *get_game_colorRGB(COLOR_THEME_DIPLODLG_MEETING_TEXT);
    pBuf->key = SDLK_ESCAPE;
    button_w = MAX(button_w , pBuf->size.w);
    button_h = MAX(button_h , pBuf->size.h);
    
    button_h += adj_size(4);
    area.w = MAX(area.w, button_w + adj_size(20));
    
    if(type != DS_WAR) {
      if(shared) {
	area.h += 4 * (button_h + adj_size(10));
      } else {
        area.h += 3 * (button_h + adj_size(10));
      }
    } else {
      area.h += 2 * (button_h + adj_size(10));
    }
    
    add_to_gui_list(ID_BUTTON, pBuf);


    pSDip_Dlg->pBeginWidgetList = pBuf;
  
    /* setup window size and start position */
    area.w += adj_size(10);

    pBuf = pWindow->prev;
  
    area.h += adj_size(2);

    resize_window(pWindow, NULL, get_game_colorRGB(COLOR_THEME_BACKGROUND),
                  (pWindow->size.w - pWindow->area.w) + area.w,
                  (pWindow->size.h - pWindow->area.h) + area.h);

    area = pWindow->area;
    
    widget_set_position(pWindow,
                        (Main.screen->w - pWindow->size.w) / 2,
                        (Main.screen->h - pWindow->size.h) / 2);

    /* setup rest of widgets */
    /* label */
    dst.x = area.x + (area.w - pText->w) / 2;
    dst.y = area.y + 1;
    alphablit(pText, NULL, pWindow->theme, &dst);
    dst.y += pText->h + adj_size(15);
    FREESURFACE(pText);
         
    pBuf = pWindow;
  
    if(type != DS_WAR) {
      /* cancel treaty */
      pBuf = pBuf->prev;
      pBuf->size.w = button_w;
      pBuf->size.h = button_h;
      pBuf->size.x = area.x + (area.w - (pBuf->size.w)) / 2;
      pBuf->size.y = dst.y;
      
      if(shared) {
	/* vision */
        pBuf = pBuf->prev;
        pBuf->size.w = button_w;
        pBuf->size.h = button_h;
        pBuf->size.y = pBuf->next->size.y + pBuf->next->size.h + adj_size(10);
        pBuf->size.x = pBuf->next->size.x;
      }
      
      /* meet */
      pBuf = pBuf->prev;
      pBuf->size.w = button_w;
      pBuf->size.h = button_h;
      pBuf->size.y = pBuf->next->size.y + pBuf->next->size.h + adj_size(10);
      pBuf->size.x = pBuf->next->size.x;
            
    } else {
    
      /* meet */
      pBuf = pBuf->prev;
      pBuf->size.w = button_w;
      pBuf->size.h = button_h;
      pBuf->size.x = area.x + (area.w - (pBuf->size.w)) / 2;
      pBuf->size.y = dst.y;
      
    }
    
    /* cancel */
    pBuf = pBuf->prev;
    pBuf->size.w = button_w;
    pBuf->size.h = button_h;
    pBuf->size.y = pBuf->next->size.y + pBuf->next->size.h + adj_size(10);
    pBuf->size.x = pBuf->next->size.x;
  
    /* ================================================== */
    /* redraw */
    redraw_group(pSDip_Dlg->pBeginWidgetList, pWindow, 0);
    widget_mark_dirty(pWindow);
  
    flush_dirty();
  }
}
