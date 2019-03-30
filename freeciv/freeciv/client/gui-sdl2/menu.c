/***********************************************************************
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

/***********************************************************************
                          menu.c  -  description
                             -------------------
    begin                : Wed Sep 04 2002
    copyright            : (C) 2002 by Rafał Bursig
    email                : Rafał Bursig <bursig@poczta.fm>
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

/* utility */
#include "fcintl.h"
#include "log.h"

/* common */
#include "game.h"
#include "road.h"
#include "traderoutes.h"
#include "unitlist.h"

/* client */
#include "client_main.h" /* client_state */
#include "climisc.h"
#include "control.h"

/* gui-sdl2 */
#include "dialogs.h"
#include "gotodlg.h"
#include "graphics.h"
#include "gui_iconv.h"
#include "gui_id.h"
#include "gui_main.h"
#include "gui_tilespec.h"
#include "mapctrl.h"
#include "mapview.h"
#include "pages.h"
#include "widget.h"

#include "menu.h"

extern struct widget *pOptions_Button;

static struct widget *pBeginOrderWidgetList;
static struct widget *pEndOrderWidgetList;

static struct widget *pOrder_Automate_Unit_Button;
static struct widget *pOrder_Build_AddTo_City_Button;
static struct widget *pOrder_Mine_Button;
static struct widget *pOrder_Irrigation_Button;
static struct widget *pOrder_Road_Button;
static struct widget *pOrder_Transform_Button;
static struct widget *pOrder_Trade_Button;

#define local_show(ID)                                                \
  clear_wflag(get_widget_pointer_form_ID(pBeginOrderWidgetList, ID, SCAN_FORWARD), \
              WF_HIDDEN)

#define local_hide(ID)                                             \
  set_wflag(get_widget_pointer_form_ID(pBeginOrderWidgetList, ID, SCAN_FORWARD), \
            WF_HIDDEN )


/**********************************************************************//**
  User interacted with some unit order widget.
**************************************************************************/
static int unit_order_callback(struct widget *pOrder_Widget)
{
  if (Main.event.type == SDL_KEYDOWN
      || (Main.event.type == SDL_MOUSEBUTTONDOWN
          && Main.event.button.button == SDL_BUTTON_LEFT)) {
    struct unit *pUnit = head_of_units_in_focus();

    set_wstate(pOrder_Widget, FC_WS_SELECTED);
    selected_widget = pOrder_Widget;

    if (!pUnit) {
      return -1;
    }

    switch (pOrder_Widget->ID) {
    case ID_UNIT_ORDER_BUILD_CITY:
      /* Enable the button for adding to a city in all cases, so we
         get an eventual error message from the server if we try. */
      key_unit_build_city();
      break;
    case ID_UNIT_ORDER_BUILD_WONDER:
      key_unit_build_wonder();
      break;
    case ID_UNIT_ORDER_ROAD:
      key_unit_road();
      break;
    case ID_UNIT_ORDER_TRADE_ROUTE:
      key_unit_trade_route();
      break;
    case ID_UNIT_ORDER_IRRIGATE:
      key_unit_irrigate();
      break;
    case ID_UNIT_ORDER_MINE:
      key_unit_mine();
      break;
    case ID_UNIT_ORDER_TRANSFORM:
      key_unit_transform();
      break;
    case ID_UNIT_ORDER_FORTRESS:
      key_unit_fortress();
      break;
    case ID_UNIT_ORDER_FORTIFY:
      key_unit_fortify();
      break;
    case ID_UNIT_ORDER_AIRBASE:
      key_unit_airbase();
      break;
    case ID_UNIT_ORDER_POLLUTION:
      key_unit_pollution();
      break;
    case ID_UNIT_ORDER_PARADROP:
      key_unit_paradrop();
      break;
    case ID_UNIT_ORDER_FALLOUT:
      key_unit_fallout();
      break;
    case ID_UNIT_ORDER_SENTRY:
      key_unit_sentry();
      break;
    case ID_UNIT_ORDER_PILLAGE:
      key_unit_pillage();
      break;
    case ID_UNIT_ORDER_HOMECITY:
      key_unit_homecity();
      break;
    case ID_UNIT_ORDER_UNLOAD_TRANSPORTER:
      key_unit_unload_all();
      break;
    case ID_UNIT_ORDER_LOAD:
      unit_list_iterate(get_units_in_focus(), punit) {
        request_unit_load(punit, NULL, unit_tile(punit));
      } unit_list_iterate_end;
      break;
    case ID_UNIT_ORDER_UNLOAD:
      unit_list_iterate(get_units_in_focus(), punit) {
        request_unit_unload(punit);
      } unit_list_iterate_end;
      break;
    case ID_UNIT_ORDER_WAKEUP_OTHERS:
      key_unit_wakeup_others();
      break;
    case ID_UNIT_ORDER_AUTO_SETTLER:
      unit_list_iterate(get_units_in_focus(), punit) {
        request_unit_autosettlers(punit);
      } unit_list_iterate_end;
      break;
    case ID_UNIT_ORDER_AUTO_EXPLORE:
      key_unit_auto_explore();
      break;
    case ID_UNIT_ORDER_CONNECT_IRRIGATE:
      {
        struct extra_type_list *extras = extra_type_list_by_cause(EC_IRRIGATION);

        if (extra_type_list_size(extras) > 0) {
          struct extra_type *pextra;

          pextra = extra_type_list_get(extra_type_list_by_cause(EC_IRRIGATION), 0);

          key_unit_connect(ACTIVITY_IRRIGATE, pextra);
        }
      }
      break;
    case ID_UNIT_ORDER_CONNECT_ROAD:
      {
        struct road_type *proad = road_by_compat_special(ROCO_ROAD);

        if (proad != NULL) {
          struct extra_type *tgt;

          tgt = road_extra_get(proad);

          key_unit_connect(ACTIVITY_GEN_ROAD, tgt);
        }
      }
      break;
    case ID_UNIT_ORDER_CONNECT_RAILROAD:
      {
        struct road_type *prail = road_by_compat_special(ROCO_RAILROAD);

        if (prail != NULL) {
          struct extra_type *tgt;

          tgt = road_extra_get(prail);

          key_unit_connect(ACTIVITY_GEN_ROAD, tgt);
        }
      }
      break;
    case ID_UNIT_ORDER_PATROL:
      key_unit_patrol();
      break;
    case ID_UNIT_ORDER_GOTO:
      key_unit_goto();
      break;
    case ID_UNIT_ORDER_GOTO_CITY:
      popup_goto_dialog();
      break;
    case ID_UNIT_ORDER_AIRLIFT:
      popup_airlift_dialog();
      break;
    case ID_UNIT_ORDER_RETURN:
      unit_list_iterate(get_units_in_focus(), punit) {
        request_unit_return(punit);
      } unit_list_iterate_end;
      break;
    case ID_UNIT_ORDER_UPGRADE:
      popup_unit_upgrade_dlg(pUnit, FALSE);
      break;
    case ID_UNIT_ORDER_CONVERT:
      key_unit_convert();
      break;
    case ID_UNIT_ORDER_DISBAND:
      popup_unit_disband_dlg(pUnit, FALSE);
      break;
    case ID_UNIT_ORDER_DIPLOMAT_DLG:
      key_unit_action_select_tgt();
      break;
    case ID_UNIT_ORDER_NUKE:
      request_unit_goto(ORDER_PERFORM_ACTION, ACTION_NUKE, -1);
      break;
    case ID_UNIT_ORDER_WAIT:
      key_unit_wait();
      flush_dirty();
      break;
    case ID_UNIT_ORDER_DONE:
      key_unit_done();
      flush_dirty();
      break;

    default:
      break;
    }
  }

  return -1;
}

/**********************************************************************//**
   Refresh order widgets.
**************************************************************************/
static Uint16 redraw_order_widgets(void)
{
  Uint16 count = 0;
  struct widget *pTmpWidget = pBeginOrderWidgetList;

  while (TRUE) {
    if (!(get_wflags(pTmpWidget) & WF_HIDDEN)) {
      if (get_wflags(pTmpWidget) & WF_RESTORE_BACKGROUND) {
        refresh_widget_background(pTmpWidget);
      }
      widget_redraw(pTmpWidget);
      widget_mark_dirty(pTmpWidget);
      count++;
    }

    if (pTmpWidget == pEndOrderWidgetList) {
      break;
    }

    pTmpWidget = pTmpWidget->next;
  }

  return count;
}

/**********************************************************************//**
   Reposition order widgets as they fit.
**************************************************************************/
static void set_new_order_widget_start_pos(void)
{
  struct widget *pMiniMap = get_minimap_window_widget();
  struct widget *pInfoWind = get_unit_info_window_widget();
  struct widget *pTmpWidget = pBeginOrderWidgetList;
  Sint16 sx, sy, xx, yy = 0;
  int count = 0, lines = 1, w = 0, count_on_line;

  xx = pMiniMap->dst->dest_rect.x + pMiniMap->size.w + adj_size(10);
  w = (pInfoWind->dst->dest_rect.x - adj_size(10)) - xx;

  if (w < (pTmpWidget->size.w + adj_size(10)) * 2) {
    if (pMiniMap->size.h == pInfoWind->size.h) {
      xx = 0;
      w = main_window_width();
      yy = pInfoWind->size.h;
    } else {
      if (pMiniMap->size.h > pInfoWind->size.h) {
        w = main_window_width() - xx - adj_size(20);
        if (w < (pTmpWidget->size.w + adj_size(10)) * 2) {
	  xx = 0;
	  w = pMiniMap->size.w;
	  yy = pMiniMap->size.h;
        } else {
          yy = pInfoWind->size.h;
        }
      } else {
	w = pInfoWind->dst->dest_rect.x - adj_size(20);
        if (w < (pTmpWidget->size.w + adj_size(10)) * 2) {
	  xx = pInfoWind->dst->dest_rect.x;
	  w = pInfoWind->size.w;
	  yy = pInfoWind->size.h;
        } else {
	  xx = adj_size(10);
          yy = pMiniMap->size.h;
        }
      }
    }
  }

  count_on_line = w / (pTmpWidget->size.w + adj_size(5));

  /* find how many to reposition */
  while (TRUE) {
    if (!(get_wflags(pTmpWidget) & WF_HIDDEN)) {
      count++;
    }

    if (pTmpWidget == pEndOrderWidgetList) {
      break;
    }

    pTmpWidget = pTmpWidget->next;
  }

  pTmpWidget = pBeginOrderWidgetList;

  if (count - count_on_line > 0) {
    lines = (count + (count_on_line - 1)) / count_on_line;

    count = count_on_line - ((count_on_line * lines) - count);
  }

  sx = xx + (w - count * (pTmpWidget->size.w + adj_size(5))) / 2;

  sy = pTmpWidget->dst->surface->h - yy - lines * (pTmpWidget->size.h + adj_size(5));

  while (TRUE) {
    if (!(get_wflags(pTmpWidget) & WF_HIDDEN)) {
      pTmpWidget->size.x = sx;
      pTmpWidget->size.y = sy;

      count--;
      sx += (pTmpWidget->size.w + adj_size(5));
      if (!count) {
	count = count_on_line;
	lines--;

	sx = xx + (w - count * (pTmpWidget->size.w + adj_size(5))) / 2;

	sy = pTmpWidget->dst->surface->h - yy - lines * (pTmpWidget->size.h + adj_size(5));
      }
    }

    if (pTmpWidget == pEndOrderWidgetList) {
      break;
    }

    pTmpWidget = pTmpWidget->next;
  }
}

/* ================================ PUBLIC ================================ */

/**********************************************************************//**
   Create units order widgets.
**************************************************************************/
void create_units_order_widgets(void)
{
  struct widget *pBuf = NULL;
  char cBuf[128];
  struct road_type *proad;
  struct road_type *prail;

  /* No orders */
  fc_snprintf(cBuf, sizeof(cBuf),"%s (%s)", _("No Orders"),
              /* TRANS: "Space" refers to the space bar on a keyboard. */
              _("Space"));
  pBuf = create_themeicon(current_theme->ODone_Icon, Main.gui,
                          WF_HIDDEN | WF_RESTORE_BACKGROUND
                          | WF_WIDGET_HAS_INFO_LABEL);
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->action = unit_order_callback;
  pBuf->info_label = create_utf8_from_char(cBuf, adj_font(10));
  pBuf->key = SDLK_SPACE;
  add_to_gui_list(ID_UNIT_ORDER_DONE, pBuf);
  /* --------- */

  pEndOrderWidgetList = pBuf;

  /* Wait */
  fc_snprintf(cBuf, sizeof(cBuf),"%s (%s)", _("Wait"), "W");
  pBuf = create_themeicon(current_theme->OWait_Icon, Main.gui,
                          WF_HIDDEN | WF_RESTORE_BACKGROUND
                          | WF_WIDGET_HAS_INFO_LABEL);
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->action = unit_order_callback;
  pBuf->info_label = create_utf8_from_char(cBuf, adj_font(10));
  pBuf->key = SDLK_w;
  add_to_gui_list(ID_UNIT_ORDER_WAIT, pBuf);
  /* --------- */

  /* Explode Nuclear */
  fc_snprintf(cBuf, sizeof(cBuf),"%s (%s)",
              action_id_name_translation(ACTION_NUKE), "Shift+N");
  pBuf = create_themeicon(current_theme->ONuke_Icon, Main.gui,
                          WF_HIDDEN | WF_RESTORE_BACKGROUND
                          | WF_WIDGET_HAS_INFO_LABEL);
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->action = unit_order_callback;
  pBuf->info_label = create_utf8_from_char(cBuf, adj_font(10));
  pBuf->key = SDLK_n;
  pBuf->mod = KMOD_SHIFT;
  add_to_gui_list(ID_UNIT_ORDER_NUKE, pBuf);
  /* --------- */

  /* Act against the specified tile. */
  /* TRANS: Button to bring up the action selection dialog. */
  fc_snprintf(cBuf, sizeof(cBuf),"%s (%s)", _("Do..."), "D");
  pBuf = create_themeicon(current_theme->OSpy_Icon, Main.gui,
                          WF_HIDDEN | WF_RESTORE_BACKGROUND
                          | WF_WIDGET_HAS_INFO_LABEL);
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->action = unit_order_callback;
  pBuf->info_label = create_utf8_from_char(cBuf, adj_font(10));
  pBuf->key = SDLK_d;
  add_to_gui_list(ID_UNIT_ORDER_DIPLOMAT_DLG, pBuf);
  /* --------- */

  /* Disband */
  fc_snprintf(cBuf, sizeof(cBuf),"%s (%s)", _("Disband Unit"), "Shift+D");
  pBuf = create_themeicon(current_theme->ODisband_Icon, Main.gui,
                          WF_HIDDEN | WF_RESTORE_BACKGROUND
                          | WF_WIDGET_HAS_INFO_LABEL);
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->action = unit_order_callback;
  pBuf->info_label = create_utf8_from_char(cBuf, adj_font(10));
  pBuf->key = SDLK_d;
  pBuf->mod = KMOD_SHIFT;
  add_to_gui_list(ID_UNIT_ORDER_DISBAND, pBuf);
  /* --------- */

  /* Upgrade */
  fc_snprintf(cBuf, sizeof(cBuf),"%s (%s)", _("Upgrade Unit"), "Shift+U");
  pBuf = create_themeicon(current_theme->Order_Icon, Main.gui,
                          WF_HIDDEN | WF_RESTORE_BACKGROUND
                          | WF_WIDGET_HAS_INFO_LABEL);
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->action = unit_order_callback;
  pBuf->info_label = create_utf8_from_char(cBuf, adj_font(10));
  pBuf->key = SDLK_u;
  pBuf->mod = KMOD_SHIFT;
  add_to_gui_list(ID_UNIT_ORDER_UPGRADE, pBuf);
  /* --------- */

  /* Convert */
  fc_snprintf(cBuf, sizeof(cBuf),"%s (%s)", _("Convert Unit"), "Shift+O");
  pBuf = create_themeicon(current_theme->Order_Icon, Main.gui,
                          WF_HIDDEN | WF_RESTORE_BACKGROUND
                          | WF_WIDGET_HAS_INFO_LABEL);
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->action = unit_order_callback;
  pBuf->info_label = create_utf8_from_char(cBuf, adj_font(10));
  pBuf->key = SDLK_o;
  pBuf->mod = KMOD_SHIFT;
  add_to_gui_list(ID_UNIT_ORDER_CONVERT, pBuf);
  /* --------- */

  /* Return to nearest city */
  fc_snprintf(cBuf, sizeof(cBuf),"%s (%s)", _("Return to Nearest City"), "Shift+G");
  pBuf = create_themeicon(current_theme->OReturn_Icon, Main.gui,
                          WF_HIDDEN | WF_RESTORE_BACKGROUND
                          | WF_WIDGET_HAS_INFO_LABEL);
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->action = unit_order_callback;
  pBuf->info_label = create_utf8_from_char(cBuf, adj_font(10));
  pBuf->key = SDLK_g;
  pBuf->mod = KMOD_SHIFT;
  add_to_gui_list(ID_UNIT_ORDER_RETURN, pBuf);
  /* --------- */

  /* Goto City */
  fc_snprintf(cBuf, sizeof(cBuf),"%s (%s)", _("Go to City"), "T");
  pBuf = create_themeicon(current_theme->OGotoCity_Icon, Main.gui,
                          WF_HIDDEN | WF_RESTORE_BACKGROUND
                          | WF_WIDGET_HAS_INFO_LABEL);
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->action = unit_order_callback;
  pBuf->info_label = create_utf8_from_char(cBuf, adj_font(10));
  pBuf->key = SDLK_t;
  add_to_gui_list(ID_UNIT_ORDER_GOTO_CITY, pBuf);
  /* --------- */

  /* Airlift */
  fc_snprintf(cBuf, sizeof(cBuf),"%s (%s)", _("Airlift to City"), "T");
  pBuf = create_themeicon(current_theme->Order_Icon, Main.gui,
                          WF_HIDDEN | WF_RESTORE_BACKGROUND
                          | WF_WIDGET_HAS_INFO_LABEL);
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->action = unit_order_callback;
  pBuf->info_label = create_utf8_from_char(cBuf, adj_font(10));
  pBuf->key = SDLK_t;
  add_to_gui_list(ID_UNIT_ORDER_AIRLIFT, pBuf);
  /* --------- */

  /* Goto location */
  fc_snprintf(cBuf, sizeof(cBuf),"%s (%s)", _("Go to Tile"), "G");
  pBuf = create_themeicon(current_theme->OGoto_Icon, Main.gui,
                          WF_HIDDEN | WF_RESTORE_BACKGROUND
                          | WF_WIDGET_HAS_INFO_LABEL);
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->action = unit_order_callback;
  pBuf->info_label = create_utf8_from_char(cBuf, adj_font(10));
  pBuf->key = SDLK_g;
  add_to_gui_list(ID_UNIT_ORDER_GOTO, pBuf);
  /* --------- */

  /* Patrol */
  fc_snprintf(cBuf, sizeof(cBuf),"%s (%s)", _("Patrol"), "Q");
  pBuf = create_themeicon(current_theme->OPatrol_Icon, Main.gui,
                          WF_HIDDEN | WF_RESTORE_BACKGROUND
                          | WF_WIDGET_HAS_INFO_LABEL);
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->action = unit_order_callback;
  pBuf->info_label = create_utf8_from_char(cBuf, adj_font(10));
  pBuf->key = SDLK_q;
  add_to_gui_list(ID_UNIT_ORDER_PATROL, pBuf);
  /* --------- */

  /* Connect irrigation */
  fc_snprintf(cBuf, sizeof(cBuf),"%s (%s)", _("Connect With Irrigation"), "Shift+I");
  pBuf = create_themeicon(current_theme->OAutoConnect_Icon, Main.gui,
                          WF_HIDDEN | WF_RESTORE_BACKGROUND
                          | WF_WIDGET_HAS_INFO_LABEL);
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->action = unit_order_callback;
  pBuf->info_label = create_utf8_from_char(cBuf, adj_font(10));
  pBuf->key = SDLK_i;
  pBuf->mod = KMOD_SHIFT;
  add_to_gui_list(ID_UNIT_ORDER_CONNECT_IRRIGATE, pBuf);
  /* --------- */

  /* Connect road */
  proad = road_by_compat_special(ROCO_ROAD);

  if (proad != NULL) {
    fc_snprintf(cBuf, sizeof(cBuf),
                _("Connect With %s (%s)"),
                extra_name_translation(road_extra_get(proad)),
                "Shift+R");
    pBuf = create_themeicon(current_theme->OAutoConnect_Icon, Main.gui,
                            WF_HIDDEN | WF_RESTORE_BACKGROUND
                            | WF_WIDGET_HAS_INFO_LABEL);
    set_wstate(pBuf, FC_WS_NORMAL);
    pBuf->action = unit_order_callback;
    pBuf->info_label = create_utf8_from_char(cBuf, adj_font(10));
    pBuf->key = SDLK_r;
    pBuf->mod = KMOD_SHIFT;
    add_to_gui_list(ID_UNIT_ORDER_CONNECT_ROAD, pBuf);
  }
  /* --------- */

  /* Connect railroad */
  prail = road_by_compat_special(ROCO_RAILROAD);
  if (prail != NULL) {
    fc_snprintf(cBuf, sizeof(cBuf),
                _("Connect With %s (%s)"),
                extra_name_translation(road_extra_get(prail)),
                "Shift+L");
    pBuf = create_themeicon(current_theme->OAutoConnect_Icon, Main.gui,
                            WF_HIDDEN | WF_RESTORE_BACKGROUND
                            | WF_WIDGET_HAS_INFO_LABEL);
    set_wstate(pBuf, FC_WS_NORMAL);
    pBuf->action = unit_order_callback;
    pBuf->info_label = create_utf8_from_char(cBuf, adj_font(10));
    pBuf->key = SDLK_l;
    pBuf->mod = KMOD_SHIFT;
    add_to_gui_list(ID_UNIT_ORDER_CONNECT_RAILROAD, pBuf);
  }
  /* --------- */

  /* Auto-Explore */
  fc_snprintf(cBuf, sizeof(cBuf),"%s (%s)", _("Auto Explore"), "X");
  pBuf = create_themeicon(current_theme->OAutoExp_Icon, Main.gui,
                          WF_HIDDEN | WF_RESTORE_BACKGROUND
                          | WF_WIDGET_HAS_INFO_LABEL);
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->action = unit_order_callback;
  pBuf->info_label = create_utf8_from_char(cBuf, adj_font(10));
  pBuf->key = SDLK_x;
  add_to_gui_list(ID_UNIT_ORDER_AUTO_EXPLORE, pBuf);
  /* --------- */

  /* Auto-Attack / Auto-Settler */
  fc_snprintf(cBuf, sizeof(cBuf),"%s (%s)", _("Auto Settler"), "A");

  pBuf = create_themeicon(current_theme->OAutoSett_Icon, Main.gui,
                          WF_HIDDEN | WF_RESTORE_BACKGROUND
                          | WF_WIDGET_HAS_INFO_LABEL);
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->action = unit_order_callback;
  pBuf->info_label = create_utf8_from_char(cBuf, adj_font(10));
  pBuf->key = SDLK_a;
  add_to_gui_list(ID_UNIT_ORDER_AUTO_SETTLER, pBuf);

  pOrder_Automate_Unit_Button = pBuf;
  /* --------- */    

  /* Wake Up Others */
  fc_snprintf(cBuf, sizeof(cBuf),"%s (%s)", _("Unsentry All On Tile"), "Shift+S");
  pBuf = create_themeicon(current_theme->OWakeUp_Icon, Main.gui,
                          WF_HIDDEN | WF_RESTORE_BACKGROUND
                          | WF_WIDGET_HAS_INFO_LABEL);
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->action = unit_order_callback;
  pBuf->info_label = create_utf8_from_char(cBuf, adj_font(10));
  pBuf->key = SDLK_s;
  pBuf->mod = KMOD_SHIFT;
  add_to_gui_list(ID_UNIT_ORDER_WAKEUP_OTHERS, pBuf);
  /* --------- */

  /* Unload */
  fc_snprintf(cBuf, sizeof(cBuf),"%s (%s)", _("Unload All From Transporter"), "Shift+T");
  pBuf = create_themeicon(current_theme->OUnload_Icon, Main.gui,
                          WF_HIDDEN | WF_RESTORE_BACKGROUND
                          | WF_WIDGET_HAS_INFO_LABEL);
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->action = unit_order_callback;
  pBuf->info_label = create_utf8_from_char(cBuf, adj_font(10));
  pBuf->mod = KMOD_SHIFT;
  pBuf->key = SDLK_t;
  add_to_gui_list(ID_UNIT_ORDER_UNLOAD_TRANSPORTER, pBuf);
  /* --------- */

  /* Load */
  fc_snprintf(cBuf, sizeof(cBuf),"%s (%s)", _("Load Unit"), "L");
  pBuf = create_themeicon(current_theme->OLoad_Icon, Main.gui,
                          WF_HIDDEN | WF_RESTORE_BACKGROUND
                          | WF_WIDGET_HAS_INFO_LABEL);
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->action = unit_order_callback;
  pBuf->info_label = create_utf8_from_char(cBuf, adj_font(10));
  pBuf->key = SDLK_l;
  add_to_gui_list(ID_UNIT_ORDER_LOAD, pBuf);
  /* --------- */

  /* Unload */
  fc_snprintf(cBuf, sizeof(cBuf),"%s (%s)", _("Unload Unit"), "U");
  pBuf = create_themeicon(current_theme->OUnload_Icon, Main.gui,
                          WF_HIDDEN | WF_RESTORE_BACKGROUND
                          | WF_WIDGET_HAS_INFO_LABEL);
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->action = unit_order_callback;
  pBuf->info_label = create_utf8_from_char(cBuf, adj_font(10));
  pBuf->key = SDLK_u;
  add_to_gui_list(ID_UNIT_ORDER_UNLOAD, pBuf);
  /* --------- */

  /* Find Homecity */
  fc_snprintf(cBuf, sizeof(cBuf),"%s (%s)",
              action_id_name_translation(ACTION_HOME_CITY), "H");
  pBuf = create_themeicon(current_theme->OHomeCity_Icon, Main.gui,
                          WF_HIDDEN | WF_RESTORE_BACKGROUND
                          | WF_WIDGET_HAS_INFO_LABEL);
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->action = unit_order_callback;
  pBuf->info_label = create_utf8_from_char(cBuf, adj_font(10));
  pBuf->key = SDLK_h;
  add_to_gui_list(ID_UNIT_ORDER_HOMECITY, pBuf);
  /* --------- */

  /* Pillage */
  fc_snprintf(cBuf, sizeof(cBuf),"%s (%s)", _("Pillage"), "Shift+P");
  pBuf = create_themeicon(current_theme->OPillage_Icon, Main.gui,
                          WF_HIDDEN | WF_RESTORE_BACKGROUND
                          | WF_WIDGET_HAS_INFO_LABEL);
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->action = unit_order_callback;
  pBuf->info_label = create_utf8_from_char(cBuf, adj_font(10));
  pBuf->key = SDLK_p;
  pBuf->mod = KMOD_SHIFT;
  add_to_gui_list(ID_UNIT_ORDER_PILLAGE, pBuf);
  /* --------- */

  /* Sentry */
  fc_snprintf(cBuf, sizeof(cBuf),"%s (%s)", _("Sentry Unit"), "S");
  pBuf = create_themeicon(current_theme->OSentry_Icon, Main.gui,
                          WF_HIDDEN | WF_RESTORE_BACKGROUND
                          | WF_WIDGET_HAS_INFO_LABEL);
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->action = unit_order_callback;
  pBuf->info_label = create_utf8_from_char(cBuf, adj_font(10));
  pBuf->key = SDLK_s;
  add_to_gui_list(ID_UNIT_ORDER_SENTRY, pBuf);
  /* --------- */

  /* Clean Nuclear Fallout */
  fc_snprintf(cBuf, sizeof(cBuf),"%s (%s)", _("Clean Nuclear Fallout"), "N");
  pBuf = create_themeicon(current_theme->OFallout_Icon, Main.gui,
                          WF_HIDDEN | WF_RESTORE_BACKGROUND
                          | WF_WIDGET_HAS_INFO_LABEL);
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->action = unit_order_callback;
  pBuf->info_label = create_utf8_from_char(cBuf, adj_font(10));
  pBuf->key = SDLK_n;
  add_to_gui_list(ID_UNIT_ORDER_FALLOUT, pBuf);
  /* --------- */

  /* Paradrop */
  fc_snprintf(cBuf, sizeof(cBuf),"%s (%s)",
              action_id_name_translation(ACTION_PARADROP), "P");
  pBuf = create_themeicon(current_theme->OParaDrop_Icon, Main.gui,
                          WF_HIDDEN | WF_RESTORE_BACKGROUND
                          | WF_WIDGET_HAS_INFO_LABEL);
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->action = unit_order_callback;
  pBuf->info_label =  create_utf8_from_char(cBuf, adj_font(10));
  pBuf->key = SDLK_p;
  add_to_gui_list(ID_UNIT_ORDER_PARADROP, pBuf);
  /* --------- */

  /* Clean Pollution */
  fc_snprintf(cBuf, sizeof(cBuf),"%s (%s)", _("Clean Pollution"), "P");
  pBuf = create_themeicon(current_theme->OPollution_Icon, Main.gui,
                          WF_HIDDEN | WF_RESTORE_BACKGROUND
                          | WF_WIDGET_HAS_INFO_LABEL);
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->action = unit_order_callback;
  pBuf->info_label = create_utf8_from_char(cBuf, adj_font(10));
  pBuf->key = SDLK_p;
  add_to_gui_list(ID_UNIT_ORDER_POLLUTION, pBuf);
  /* --------- */

  /* Build Airbase */
  fc_snprintf(cBuf, sizeof(cBuf),"%s (%s)", _("Build Airbase"), "Shift+E");
  pBuf = create_themeicon(current_theme->OAirBase_Icon, Main.gui,
                          WF_HIDDEN | WF_RESTORE_BACKGROUND
                          | WF_WIDGET_HAS_INFO_LABEL);
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->action = unit_order_callback;
  pBuf->info_label = create_utf8_from_char(cBuf, adj_font(10));
  pBuf->key = SDLK_e;
  add_to_gui_list(ID_UNIT_ORDER_AIRBASE, pBuf);
  /* --------- */

  /* Fortify */
  fc_snprintf(cBuf, sizeof(cBuf),"%s (%s)", _("Fortify Unit"), "F");
  pBuf = create_themeicon(current_theme->OFortify_Icon, Main.gui,
                          WF_HIDDEN | WF_RESTORE_BACKGROUND
                          | WF_WIDGET_HAS_INFO_LABEL);
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->action = unit_order_callback;
  pBuf->info_label = create_utf8_from_char(cBuf, adj_font(10));
  pBuf->key = SDLK_f;
  add_to_gui_list(ID_UNIT_ORDER_FORTIFY, pBuf);
  /* --------- */

  /* Build Fortress */
  fc_snprintf(cBuf, sizeof(cBuf),"%s (%s)", _("Build Fortress"), "Shift+F");
  pBuf = create_themeicon(current_theme->OFortress_Icon, Main.gui,
                          WF_HIDDEN | WF_RESTORE_BACKGROUND
                          | WF_WIDGET_HAS_INFO_LABEL);
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->action = unit_order_callback;
  pBuf->info_label = create_utf8_from_char(cBuf, adj_font(10));
  pBuf->key = SDLK_f;
  add_to_gui_list(ID_UNIT_ORDER_FORTRESS, pBuf);
  /* --------- */

  /* Transform Tile */
  fc_snprintf(cBuf, sizeof(cBuf),"%s (%s)", _("Transform Tile"), "O");
  pBuf = create_themeicon(current_theme->OTransform_Icon, Main.gui,
                          WF_HIDDEN | WF_RESTORE_BACKGROUND
                          | WF_WIDGET_HAS_INFO_LABEL);
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->action = unit_order_callback;
  pBuf->info_label = create_utf8_from_char(cBuf, adj_font(10));
  pBuf->key = SDLK_o;
  pOrder_Transform_Button = pBuf;
  add_to_gui_list(ID_UNIT_ORDER_TRANSFORM, pBuf);
  /* --------- */

  /* Build Mine */
  fc_snprintf(cBuf, sizeof(cBuf),"%s (%s)", _("Build Mine"), "M");
  pBuf = create_themeicon(current_theme->OMine_Icon, Main.gui,
                          WF_HIDDEN | WF_RESTORE_BACKGROUND
                          | WF_WIDGET_HAS_INFO_LABEL);
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->action = unit_order_callback;
  pBuf->info_label = create_utf8_from_char(cBuf, adj_font(10));
  pBuf->key = SDLK_m;
  add_to_gui_list(ID_UNIT_ORDER_MINE, pBuf);

  pOrder_Mine_Button = pBuf;
  /* --------- */    

  /* Build Irrigation */
  fc_snprintf(cBuf, sizeof(cBuf),"%s (%s)", _("Build Irrigation"), "I");
  pBuf = create_themeicon(current_theme->OIrrigation_Icon, Main.gui,
                          WF_HIDDEN | WF_RESTORE_BACKGROUND
                          | WF_WIDGET_HAS_INFO_LABEL);
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->action = unit_order_callback;
  pBuf->key = SDLK_i;
  pBuf->info_label = create_utf8_from_char(cBuf, adj_font(10));
  add_to_gui_list(ID_UNIT_ORDER_IRRIGATE, pBuf);

  pOrder_Irrigation_Button = pBuf;
  /* --------- */    

  /* Establish Trade route */
  fc_snprintf(cBuf, sizeof(cBuf), "%s (%s)",
              action_id_name_translation(ACTION_TRADE_ROUTE), "R");
  pBuf = create_themeicon(current_theme->OTrade_Icon, Main.gui,
                          WF_HIDDEN | WF_RESTORE_BACKGROUND
                          | WF_WIDGET_HAS_INFO_LABEL);
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->action = unit_order_callback;
  pBuf->info_label = create_utf8_from_char(cBuf, adj_font(10));
  pBuf->key = SDLK_r;
  add_to_gui_list(ID_UNIT_ORDER_TRADE_ROUTE, pBuf);

  pOrder_Trade_Button = pBuf;
  /* --------- */

  /* Build (Rail-)Road */
  /* TRANS: "Build Road (R) 999 turns" */
  if (proad != NULL) {
    fc_snprintf(cBuf, sizeof(cBuf), _("Build %s (%s) %d %s"),
                extra_name_translation(road_extra_get(proad)), "R", 999,
                PL_("turn", "turns", 999));
  } else if (prail != NULL) {
    /* TRANS: "Build Railroad (R) 999 turns" */
    fc_snprintf(cBuf, sizeof(cBuf), _("Build %s (%s) %d %s"),
                extra_name_translation(road_extra_get(prail)), "R", 999,
                PL_("turn", "turns", 999));
  } else {
    cBuf[0] = '\0';
  }

  pBuf = create_themeicon(current_theme->ORoad_Icon, Main.gui,
                          WF_HIDDEN | WF_RESTORE_BACKGROUND
                          | WF_WIDGET_HAS_INFO_LABEL);
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->action = unit_order_callback;
  pBuf->info_label = create_utf8_from_char(cBuf, adj_font(10));
  pBuf->key = SDLK_r;
  add_to_gui_list(ID_UNIT_ORDER_ROAD, pBuf);

  pOrder_Road_Button = pBuf;
  /* --------- */

  /* Help Build Wonder */
  fc_snprintf(cBuf, sizeof(cBuf),"%s (%s)",
              action_id_name_translation(ACTION_HELP_WONDER), "B");
  pBuf = create_themeicon(current_theme->OWonder_Icon, Main.gui,
                          WF_HIDDEN | WF_RESTORE_BACKGROUND
                          | WF_WIDGET_HAS_INFO_LABEL);
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->action = unit_order_callback;
  pBuf->info_label = create_utf8_from_char(cBuf, adj_font(10));
  pBuf->key = SDLK_b;
  add_to_gui_list(ID_UNIT_ORDER_BUILD_WONDER, pBuf);
  /* --------- */

  /* Add to City / Build New City */
  fc_snprintf(cBuf, sizeof(cBuf), "%s (%s)",
              action_id_name_translation(ACTION_JOIN_CITY), "B");
  fc_snprintf(cBuf, sizeof(cBuf), "%s (%s)",
              action_id_name_translation(ACTION_FOUND_CITY), "B");

  pBuf = create_themeicon(current_theme->OBuildCity_Icon, Main.gui,
                          WF_HIDDEN | WF_RESTORE_BACKGROUND
                          | WF_WIDGET_HAS_INFO_LABEL);
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->action = unit_order_callback;
  pBuf->info_label = create_utf8_from_char(cBuf, adj_font(10));
  pBuf->key = SDLK_b;
  add_to_gui_list(ID_UNIT_ORDER_BUILD_CITY, pBuf);

  pOrder_Build_AddTo_City_Button = pBuf;
  /* --------- */

  pBeginOrderWidgetList = pBuf;

  SDL_Client_Flags |= CF_ORDERS_WIDGETS_CREATED;
}

/**********************************************************************//**
   Free resources allocated for unit order widgets.
**************************************************************************/
void delete_units_order_widgets(void)
{
  del_group(pBeginOrderWidgetList, pEndOrderWidgetList);

  pBeginOrderWidgetList = NULL;
  pEndOrderWidgetList = NULL;
  SDL_Client_Flags &= ~CF_ORDERS_WIDGETS_CREATED;
}

/**********************************************************************//**
   Draw order widgets to their currently correct place.
**************************************************************************/
void update_order_widgets(void)
{
  set_new_order_widget_start_pos();
  redraw_order_widgets();
}

/**********************************************************************//**
   Clear unit order widgets from view.
**************************************************************************/
void undraw_order_widgets(void)
{
  struct widget *pTmpWidget = pBeginOrderWidgetList;

  if (pTmpWidget == NULL) {
    return;
  }

  while (TRUE) {
    if (!(get_wflags(pTmpWidget) & WF_HIDDEN) && (pTmpWidget->gfx)) {
      widget_undraw(pTmpWidget);
      widget_mark_dirty(pTmpWidget);
    }

    if (pTmpWidget == pEndOrderWidgetList) {
      break;
    }

    pTmpWidget = pTmpWidget->next;
  }
}

/**********************************************************************//**
  Free surfaces of the unit order widgets.
  TODO: Unused -> Remove
**************************************************************************/
void free_bcgd_order_widgets(void)
{
  struct widget *pTmpWidget = pBeginOrderWidgetList;

  while (TRUE) {
    FREESURFACE(pTmpWidget->gfx);

    if (pTmpWidget == pEndOrderWidgetList) {
      break;
    }

    pTmpWidget = pTmpWidget->next;
  }
}

/* ============================== Native =============================== */

/**********************************************************************//**
  Initialize menus (sensitivity, name, etc.) based on the
  current state and current ruleset, etc.  Call menus_update().
**************************************************************************/
void real_menus_init(void)
{
  /* PORTME */
}

/**********************************************************************//**
  Update all of the menus (sensitivity, etc.) based on the current state.
**************************************************************************/
void real_menus_update(void)
{
  static Uint16 counter = 0;
  struct unit_list *punits = NULL;
  struct unit *pUnit = NULL;
  static char cBuf[128];

  if ((C_S_RUNNING != client_state())
      || (get_client_page() != PAGE_GAME)) {

    SDL_Client_Flags |= CF_GAME_JUST_STARTED;

    if (SDL_Client_Flags & CF_MAP_UNIT_W_CREATED) {
      set_wflag(pOptions_Button, WF_HIDDEN);
      hide_minimap_window_buttons();
      hide_unitinfo_window_buttons();
    }

    if (SDL_Client_Flags & CF_ORDERS_WIDGETS_CREATED) {
      hide_group(pBeginOrderWidgetList, pEndOrderWidgetList);
    }

  } else {
    /* Running state */
    if (SDL_Client_Flags & CF_MAP_UNIT_W_CREATED) {
      /* show options button */
      clear_wflag(pOptions_Button, WF_HIDDEN);
      widget_redraw(pOptions_Button);
      widget_mark_dirty(pOptions_Button);
    }

    if (NULL == client.conn.playing) {
      /* Global observer */
      if (SDL_Client_Flags & CF_MAP_UNIT_W_CREATED) {
        /* show minimap buttons and unitinfo buttons */
        show_minimap_window_buttons();
        show_unitinfo_window_buttons();
        /* disable minimap buttons and unitinfo buttons */
        disable_minimap_window_buttons();
        disable_unitinfo_window_buttons();
      }

      return;

    } else {
      /* running state with player */

      if (get_wstate(pEndOrderWidgetList) == FC_WS_DISABLED) {
        enable_group(pBeginOrderWidgetList, pEndOrderWidgetList);
      }

      if (counter) {
        undraw_order_widgets();
      }

      if (SDL_Client_Flags & CF_GAME_JUST_STARTED) {
        SDL_Client_Flags &= ~CF_GAME_JUST_STARTED;

        /* show minimap buttons and unitinfo buttons */
        show_minimap_window_buttons();
        show_unitinfo_window_buttons();

        counter = 0;
      }
    }

    punits = get_units_in_focus();
    pUnit = unit_list_get(punits, 0);

    if (pUnit && !pUnit->ai_controlled) {
      struct city *pHomecity;
      int time;
      struct tile *pTile = unit_tile(pUnit);
      struct city *pCity = tile_city(pTile);
      struct terrain *pTerrain = tile_terrain(pTile);
      struct base_type *pbase;
      struct extra_type *pextra = next_extra_for_tile(pTile, EC_ROAD,
                                                      unit_owner(pUnit), pUnit);

      if (!counter) {
	local_show(ID_UNIT_ORDER_GOTO);
	local_show(ID_UNIT_ORDER_DISBAND);

	local_show(ID_UNIT_ORDER_WAIT);
	local_show(ID_UNIT_ORDER_DONE);
      }

      /* Enable the button for adding to a city in all cases, so we
       * get an eventual error message from the server if we try. */

      if (unit_can_add_or_build_city(pUnit)) {
	if (pCity) {
          fc_snprintf(cBuf, sizeof(cBuf),"%s (%s)",
                      action_id_name_translation(ACTION_JOIN_CITY), "B");
        } else {
          fc_snprintf(cBuf, sizeof(cBuf),"%s (%s)",
                      action_id_name_translation(ACTION_FOUND_CITY), "B");
	}
        copy_chars_to_utf8_str(pOrder_Build_AddTo_City_Button->info_label,
                               cBuf);
	clear_wflag(pOrder_Build_AddTo_City_Button, WF_HIDDEN);
      } else {
	set_wflag(pOrder_Build_AddTo_City_Button, WF_HIDDEN);
      }

      if (unit_can_help_build_wonder_here(pUnit)) {
	local_show(ID_UNIT_ORDER_BUILD_WONDER);
      } else {
	local_hide(ID_UNIT_ORDER_BUILD_WONDER);
      }

      if (pextra != NULL) {
        struct road_type *proad = extra_road_get(pextra);
        enum road_compat compat = road_compat_special(proad);

        time = tile_activity_time(ACTIVITY_GEN_ROAD, pTile, road_extra_get(proad));

        /* TRANS: "Build Railroad (R) 3 turns" */
	fc_snprintf(cBuf, sizeof(cBuf), _("Build %s (%s) %d %s"),
                    extra_name_translation(pextra),
                    "R", time,
                    PL_("turn", "turns", time));

	if (compat == ROCO_RAILROAD) {
	  pOrder_Road_Button->theme = current_theme->ORailRoad_Icon;
	} else {
	  pOrder_Road_Button->theme = current_theme->ORoad_Icon;
	}
        copy_chars_to_utf8_str(pOrder_Road_Button->info_label, cBuf);
	clear_wflag(pOrder_Road_Button, WF_HIDDEN);
      } else {
	set_wflag(pOrder_Road_Button, WF_HIDDEN);
      }

      /* unit_can_est_trade_route_here(pUnit) */
      if (pCity && utype_can_do_action(unit_type_get(pUnit),
                                       ACTION_TRADE_ROUTE)
          && (pHomecity = game_city_by_number(pUnit->homecity))
          && can_cities_trade(pHomecity, pCity)) {
        int revenue = get_caravan_enter_city_trade_bonus(pHomecity, pCity,
                                                         pUnit->carrying,
                                                         TRUE);

        if (can_establish_trade_route(pHomecity, pCity)) {
          fc_snprintf(cBuf, sizeof(cBuf),
                      _("%s With %s ( %d one time bonus + %d trade ) (R)"),
                      action_id_name_translation(ACTION_TRADE_ROUTE),
                      city_name_get(pHomecity),
                      revenue,
                      trade_base_between_cities(pHomecity, pCity));
        } else {
          revenue = (revenue + 2) / 3;
          fc_snprintf(cBuf, sizeof(cBuf),
                      _("%s Of %s ( %d one time bonus ) (R)"),
                      action_id_name_translation(ACTION_MARKETPLACE),
                      city_name_get(pHomecity),
                      revenue);
        }
        copy_chars_to_utf8_str(pOrder_Trade_Button->info_label, cBuf);
        clear_wflag(pOrder_Trade_Button, WF_HIDDEN);
      } else {
        set_wflag(pOrder_Trade_Button, WF_HIDDEN);
      }

      if (can_unit_do_activity(pUnit, ACTIVITY_IRRIGATE)) {
        time = tile_activity_time(ACTIVITY_IRRIGATE, unit_tile(pUnit), NULL);

        if (!strcmp(terrain_rule_name(pTerrain), "Forest")
            || !strcmp(terrain_rule_name(pTerrain), "Jungle")) {
          /* set Crop Forest Icon */
          fc_snprintf(cBuf, sizeof(cBuf),"%s %s (%s) %d %s",
                      _("Cut Down to"),
                      terrain_name_translation(pTerrain->irrigation_result),
                      "I", time , PL_("turn", "turns", time));
          pOrder_Irrigation_Button->theme = current_theme->OCutDownForest_Icon;
        } else if (!strcmp(terrain_rule_name(pTerrain), "Swamp")) {
          fc_snprintf(cBuf, sizeof(cBuf),"%s %s (%s) %d %s",
                      _("Irrigate to"),
                      terrain_name_translation(pTerrain->irrigation_result),
                      "I", time , PL_("turn", "turns", time));
          pOrder_Irrigation_Button->theme = current_theme->OIrrigation_Icon;
        } else {
          /* set Irrigation Icon */
          fc_snprintf(cBuf, sizeof(cBuf),"%s (%s) %d %s",
                      _("Build Irrigation"), "I", time , 
                      PL_("turn", "turns", time));
          pOrder_Irrigation_Button->theme = current_theme->OIrrigation_Icon;
        }

        copy_chars_to_utf8_str(pOrder_Irrigation_Button->info_label, cBuf);
        clear_wflag(pOrder_Irrigation_Button, WF_HIDDEN);
      } else {
        set_wflag(pOrder_Irrigation_Button, WF_HIDDEN);
      }

      if (can_unit_do_activity(pUnit, ACTIVITY_MINE)) {
        time = tile_activity_time(ACTIVITY_MINE, unit_tile(pUnit), NULL);

        /* FIXME: THIS CODE IS WRONG */
        if (!strcmp(terrain_rule_name(pTerrain), "Forest")) {
	  /* set Irrigate Icon -> make swamp */
          fc_snprintf(cBuf, sizeof(cBuf),"%s %s (%s) %d %s",
                      _("Irrigate to"),
                      terrain_name_translation(pTerrain->mining_result),
                      "M", time , PL_("turn", "turns", time));
          pOrder_Mine_Button->theme = current_theme->OIrrigation_Icon;
        } else if (!strcmp(terrain_rule_name(pTerrain), "Jungle")
                   || !strcmp(terrain_rule_name(pTerrain), "Plains")
                   || !strcmp(terrain_rule_name(pTerrain), "Grassland")
                   || !strcmp(terrain_rule_name(pTerrain), "Swamp")) {
          /* set Forest Icon -> plant Forrest*/
          fc_snprintf(cBuf, sizeof(cBuf),"%s (%s) %d %s",
                      _("Plant Forest"), "M", time,
                      PL_("turn", "turns", time));
          pOrder_Mine_Button->theme = current_theme->OPlantForest_Icon;

        } else {
          /* set Mining Icon */
          fc_snprintf(cBuf, sizeof(cBuf),"%s (%s) %d %s",
                      _("Build Mine"), "M", time,
                      PL_("turn", "turns", time));
          pOrder_Mine_Button->theme = current_theme->OMine_Icon;
        }

        copy_chars_to_utf8_str(pOrder_Mine_Button->info_label, cBuf);
        clear_wflag(pOrder_Mine_Button, WF_HIDDEN);
      } else {
        set_wflag(pOrder_Mine_Button, WF_HIDDEN);
      }

      if (can_unit_do_activity(pUnit, ACTIVITY_TRANSFORM)) {
        time = tile_activity_time(ACTIVITY_TRANSFORM, unit_tile(pUnit), NULL);
        fc_snprintf(cBuf, sizeof(cBuf),"%s %s (%s) %d %s",
                    _("Transform to"),
                    terrain_name_translation(pTerrain->transform_result),
                    "O", time, PL_("turn", "turns", time));
        copy_chars_to_utf8_str(pOrder_Transform_Button->info_label, cBuf);
        clear_wflag(pOrder_Transform_Button, WF_HIDDEN);
      } else {
        set_wflag(pOrder_Transform_Button, WF_HIDDEN);
      }

      pbase = get_base_by_gui_type(BASE_GUI_FORTRESS, pUnit, unit_tile(pUnit));
      if (pbase != NULL) {
        local_show(ID_UNIT_ORDER_FORTRESS);
      } else {
        local_hide(ID_UNIT_ORDER_FORTRESS);
      }

      if (can_unit_do_activity(pUnit, ACTIVITY_FORTIFYING)) {
        local_show(ID_UNIT_ORDER_FORTIFY);
      } else {
        local_hide(ID_UNIT_ORDER_FORTIFY);
      }

      pbase = get_base_by_gui_type(BASE_GUI_AIRBASE, pUnit, unit_tile(pUnit));
      if (pbase != NULL) {
        local_show(ID_UNIT_ORDER_AIRBASE);
      } else {
        local_hide(ID_UNIT_ORDER_AIRBASE);
      }

      if (can_unit_do_activity(pUnit, ACTIVITY_POLLUTION)) {
        local_show(ID_UNIT_ORDER_POLLUTION);
      } else {
        local_hide(ID_UNIT_ORDER_POLLUTION);
      }

      if (can_unit_paradrop(pUnit)) {
        local_show(ID_UNIT_ORDER_PARADROP);
      } else {
        local_hide(ID_UNIT_ORDER_PARADROP);
      }

      if (can_unit_do_activity(pUnit, ACTIVITY_FALLOUT)) {
        local_show(ID_UNIT_ORDER_FALLOUT);
      } else {
        local_hide(ID_UNIT_ORDER_FALLOUT);
      }

      if (can_unit_do_activity(pUnit, ACTIVITY_SENTRY)) {
        local_show(ID_UNIT_ORDER_SENTRY);
      } else {
        local_hide(ID_UNIT_ORDER_SENTRY);
      }

      if (can_unit_do_activity(pUnit, ACTIVITY_PILLAGE)) {
        local_show(ID_UNIT_ORDER_PILLAGE);
      } else {
        local_hide(ID_UNIT_ORDER_PILLAGE);
      }

      if (pCity && can_unit_change_homecity(pUnit)
          && pCity->id != pUnit->homecity) {
        local_show(ID_UNIT_ORDER_HOMECITY);
      } else {
        local_hide(ID_UNIT_ORDER_HOMECITY);
      }

      if (pUnit->client.occupied) {
        local_show(ID_UNIT_ORDER_UNLOAD_TRANSPORTER);
      } else {
        local_hide(ID_UNIT_ORDER_UNLOAD_TRANSPORTER);
      }

      if (units_can_load(punits)) {
        local_show(ID_UNIT_ORDER_LOAD);
      } else {
        local_hide(ID_UNIT_ORDER_LOAD);
      }

      if (units_can_unload(punits)) {
        local_show(ID_UNIT_ORDER_UNLOAD);
      } else {
        local_hide(ID_UNIT_ORDER_UNLOAD);
      }

      if (is_unit_activity_on_tile(ACTIVITY_SENTRY, unit_tile(pUnit))) {
        local_show(ID_UNIT_ORDER_WAKEUP_OTHERS);
      } else {
        local_hide(ID_UNIT_ORDER_WAKEUP_OTHERS);
      }

      if (can_unit_do_autosettlers(pUnit)) {
        if (unit_has_type_flag(pUnit, UTYF_SETTLERS)) {
          if (pOrder_Automate_Unit_Button->theme != current_theme->OAutoSett_Icon) {
            fc_snprintf(cBuf, sizeof(cBuf),"%s (%s)", _("Auto Settler"), "A");
            pOrder_Automate_Unit_Button->theme = current_theme->OAutoSett_Icon;
            copy_chars_to_utf8_str(pOrder_Automate_Unit_Button->info_label,
                                   cBuf);
          }
        } else {
          if (pOrder_Automate_Unit_Button->theme != current_theme->OAutoAtt_Icon) {
            fc_snprintf(cBuf, sizeof(cBuf),"%s (%s)", _("Auto Attack"), "A");
            pOrder_Automate_Unit_Button->theme = current_theme->OAutoAtt_Icon;
            copy_chars_to_utf8_str(pOrder_Automate_Unit_Button->info_label,
                                   cBuf);
          }
        }
        clear_wflag(pOrder_Automate_Unit_Button, WF_HIDDEN);
      } else {
        set_wflag(pOrder_Automate_Unit_Button, WF_HIDDEN);
      }

      if (can_unit_do_activity(pUnit, ACTIVITY_EXPLORE)) {
        local_show(ID_UNIT_ORDER_AUTO_EXPLORE);
      } else {
        local_hide(ID_UNIT_ORDER_AUTO_EXPLORE);
      }

      {
        bool conn_possible = FALSE;
        struct extra_type_list *extras;

        extras = extra_type_list_by_cause(EC_IRRIGATION);

        if (extra_type_list_size(extras) > 0) {
          struct extra_type *tgt;

          tgt = extra_type_list_get(extras, 0);
          conn_possible = can_units_do_connect(punits, ACTIVITY_IRRIGATE, tgt);
        }

        if (conn_possible) {
          local_show(ID_UNIT_ORDER_CONNECT_IRRIGATE);
        } else {
          local_hide(ID_UNIT_ORDER_CONNECT_IRRIGATE);
        }
      }

      {
        struct road_type *proad = road_by_compat_special(ROCO_ROAD);
        bool road_conn_possible;

        if (proad != NULL) {
          struct extra_type *tgt;

          tgt = road_extra_get(proad);

          road_conn_possible = can_unit_do_connect(pUnit, ACTIVITY_GEN_ROAD, tgt);
        } else {
          road_conn_possible = FALSE;
        }

        if (road_conn_possible) {
          local_show(ID_UNIT_ORDER_CONNECT_ROAD);
        } else {
          local_hide(ID_UNIT_ORDER_CONNECT_ROAD);
        }
      }

      {
        struct road_type *proad = road_by_compat_special(ROCO_RAILROAD);
        bool road_conn_possible;

        if (proad != NULL) {
          struct extra_type *tgt;

          tgt = road_extra_get(proad);

          road_conn_possible = can_unit_do_connect(pUnit, ACTIVITY_GEN_ROAD, tgt);
        } else {
          road_conn_possible = FALSE;
        }

        if (road_conn_possible) {
          local_show(ID_UNIT_ORDER_CONNECT_RAILROAD);
        } else {
          local_hide(ID_UNIT_ORDER_CONNECT_RAILROAD);
        }
      }

     if (unit_can_do_action(pUnit, ACTION_ANY)) {
       local_show(ID_UNIT_ORDER_DIPLOMAT_DLG);
      } else {
       local_hide(ID_UNIT_ORDER_DIPLOMAT_DLG);
      }

      if (unit_can_do_action(pUnit, ACTION_NUKE)) {
        local_show(ID_UNIT_ORDER_NUKE);
      } else {
        local_hide(ID_UNIT_ORDER_NUKE);
      }

      if (pCity && pCity->airlift) {
        local_show(ID_UNIT_ORDER_AIRLIFT);
        hide(ID_UNIT_ORDER_GOTO_CITY);
      } else {
        local_show(ID_UNIT_ORDER_GOTO_CITY);
        local_hide(ID_UNIT_ORDER_AIRLIFT);
      }

      if (pCity && can_upgrade_unittype(client.conn.playing,
                                        unit_type_get(pUnit))) {
        local_show(ID_UNIT_ORDER_UPGRADE);
      } else {
        local_hide(ID_UNIT_ORDER_UPGRADE);
      }

      if (unit_can_convert(pUnit)) {
        local_show(ID_UNIT_ORDER_CONVERT);
      } else {
        local_hide(ID_UNIT_ORDER_CONVERT);
      }

      set_new_order_widget_start_pos();
      counter = redraw_order_widgets();

    } else {
      if (counter) {
        hide_group(pBeginOrderWidgetList, pEndOrderWidgetList);
      }

      counter = 0;
    }
  }
}

/**********************************************************************//**
  Disable all unit order buttons.
**************************************************************************/
void disable_order_buttons(void)
{
  undraw_order_widgets();
  disable_group(pBeginOrderWidgetList, pEndOrderWidgetList);
  redraw_order_widgets();
}

/**********************************************************************//**
  Enable all unit order buttons.
**************************************************************************/
void enable_order_buttons(void)
{
  if (can_client_issue_orders()) {
    undraw_order_widgets();
    enable_group(pBeginOrderWidgetList, pEndOrderWidgetList);
    redraw_order_widgets();
  }
}
