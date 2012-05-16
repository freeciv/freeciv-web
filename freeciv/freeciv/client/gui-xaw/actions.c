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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* utility */
#include "log.h"

/* common */
#include "unitlist.h"

/* client */
#include "client_main.h"
#include "control.h"

/* gui-xaw */
#include "chatline.h"
#include "citydlg.h"
#include "cityrep.h"
#include "connectdlg.h"
#include "dialogs.h"
#include "diplodlg.h"
#include "finddlg.h"
#include "gotodlg.h"
#include "gui_main.h"
#include "helpdlg.h"
#include "inputdlg.h"
#include "mapctrl.h"
#include "menu.h"
#include "messagewin.h"
#include "pages.h"
#include "plrdlg.h"
#include "inteldlg.h"
#include "ratesdlg.h"
#include "repodlgs.h"
#include "spaceshipdlg.h"
#include "wldlg.h"

#include "actions.h"

/*******************************************************************************
 Action Routines!!
*******************************************************************************/
static void xaw_mouse_moved(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  XButtonEvent *ev=&event->xbutton;
  update_line(ev->x, ev->y);
}

static void xaw_btn_adjust_workers(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  adjust_workers_button_pressed(event->xbutton.x, event->xbutton.y);
}

static void xaw_btn_select_citymap(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  citydlg_btn_select_citymap(w, event);
}

static void xaw_btn_select_mapcanvas(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  mapctrl_btn_mapcanvas(event);
}

static void xaw_btn_select_overviewcanvas(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  mapctrl_btn_overviewcanvas(event);
}

static void xaw_btn_show_info_popup(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  main_show_info_popup(event);
}

static void xaw_key_cancel_action(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  key_cancel_action();
}

static void xaw_key_center_on_unit(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  center_on_unit();
}

static void xaw_key_chatline_send(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  chatline_key_send(w);
}

static void xaw_key_city_names_toggle(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  key_city_names_toggle();
}

static void xaw_key_city_productions_toggle(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  key_city_productions_toggle();
}

static void xaw_key_city_workers(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  mapctrl_key_city_workers(event);
}

static void xaw_key_dialog_city_close(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  citydlg_key_close(w);
}

static void xaw_key_dialog_connect_connect(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  connectdlg_key_connect(w);
}

static void xaw_key_dialog_diplo_gold(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  diplodlg_key_gold(w);
}

static void xaw_key_dialog_input_ok(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  inputdlg_key_ok(w);
}

static void xaw_key_dialog_races_ok(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  racesdlg_key_ok(w);
}

static void xaw_key_dialog_spaceship_close(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  spaceshipdlg_key_close(w);
}

static void xaw_key_end_turn(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  key_end_turn();
}

static void xaw_key_focus_to_next_unit(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  advance_unit_focus();
}

static void xaw_key_map_grid_toggle(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  key_map_grid_toggle();
}

/*************************************************************************
  Called when the key to toggle borders is pressed.
**************************************************************************/
static void xaw_key_map_borders_toggle(Widget w, XEvent *event,
				       String *argv, Cardinal *argc)
{
  key_map_borders_toggle();
}

static void xaw_key_move_north(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  key_unit_move(DIR8_NORTH);
}

static void xaw_key_move_north_east(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  key_unit_move(DIR8_NORTHEAST);
}

static void xaw_key_move_east(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  key_unit_move(DIR8_EAST);
}

static void xaw_key_move_south_east(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  key_unit_move(DIR8_SOUTHEAST);
}

static void xaw_key_move_south(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  key_unit_move(DIR8_SOUTH);
}

static void xaw_key_move_south_west(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  key_unit_move(DIR8_SOUTHWEST);
}

static void xaw_key_move_west(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  key_unit_move(DIR8_WEST);
}

static void xaw_key_move_north_west(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  key_unit_move(DIR8_NORTHWEST);
}

static void xaw_key_open_city_report(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  if (can_client_change_view() &&
     is_menu_item_active(MENU_REPORT, MENU_REPORT_CITIES))
    popup_city_report_dialog(0);
}

static void xaw_key_open_demographics(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  if (can_client_change_view() &&
     is_menu_item_active(MENU_REPORT, MENU_REPORT_DEMOGRAPHIC))
    send_report_request(REPORT_DEMOGRAPHIC);
}

static void xaw_key_open_economy_report(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  if (can_client_change_view() &&
     is_menu_item_active(MENU_REPORT, MENU_REPORT_ECONOMY))
    popup_economy_report_dialog(0);
}

/****************************************************************************
  Invoked when the key binding for government->find_city is pressed.
****************************************************************************/
static void xaw_key_open_find_city(Widget w, XEvent *event,
				   String *argv, Cardinal *argc)
{
  if (can_client_change_view()
      && is_menu_item_active(MENU_GOVERNMENT, MENU_GOVERNMENT_FIND_CITY)) {
    popup_find_dialog();
  }
}

static void xaw_key_open_goto_airlift(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  if (can_client_change_view() &&
     is_menu_item_active(MENU_ORDER, MENU_ORDER_GOTO_CITY))
    popup_goto_dialog();
}

static void xaw_key_open_messages(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  if (can_client_change_view() &&
     is_menu_item_active(MENU_REPORT, MENU_REPORT_MESSAGES))
    popup_meswin_dialog(FALSE);
}

static void xaw_key_open_players(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  if (can_client_change_view() &&
     is_menu_item_active(MENU_REPORT, MENU_REPORT_PLAYERS))
    popup_players_dialog(FALSE);
}

/****************************************************************************
  Invoked when the key binding for government->rates is pressed.
****************************************************************************/
static void xaw_key_open_rates(Widget w, XEvent *event,
			       String *argv, Cardinal *argc)
{
  if (can_client_change_view()
      && is_menu_item_active(MENU_GOVERNMENT, MENU_GOVERNMENT_RATES)) {
    popup_rates_dialog();
  }
}

/****************************************************************************
  Invoked when the key binding for government->revolution is pressed.
****************************************************************************/
static void xaw_key_open_revolution(Widget w, XEvent *event,
				    String *argv, Cardinal *argc)
{
  if (can_client_change_view()
      && is_menu_item_active(MENU_GOVERNMENT, MENU_GOVERNMENT_REVOLUTION)) {
    popup_revolution_dialog(NULL);
  }
}

static void xaw_key_open_science_report(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  if (can_client_change_view() &&
     is_menu_item_active(MENU_REPORT, MENU_REPORT_SCIENCE))
    popup_science_dialog(0);
}

static void xaw_key_open_spaceship(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  if (can_client_change_view() &&
     is_menu_item_active(MENU_REPORT, MENU_REPORT_SPACESHIP))
    popup_spaceship_dialog(client.conn.playing);
}

/****************************************************************************
  Invoked when the key binding for report->top_five_cities is pressed.
****************************************************************************/
static void xaw_key_open_top_five(Widget w, XEvent *event,
				  String *argv, Cardinal *argc)
{
  if (can_client_change_view()
      && is_menu_item_active(MENU_REPORT, MENU_REPORT_TOP_CITIES)) {
    send_report_request(REPORT_TOP_5_CITIES);
  }
}

/****************************************************************************
  Invoked when the key binding for report->units is pressed.
****************************************************************************/
static void xaw_key_open_units_report(Widget w, XEvent *event,
				      String *argv, Cardinal *argc)
{
  if (can_client_change_view()
      && is_menu_item_active(MENU_REPORT, MENU_REPORT_UNITS)) {
    popup_activeunits_report_dialog(0);
  }
}

/****************************************************************************
  Invoked when the key binding for report->wonders is pressed.
****************************************************************************/
static void xaw_key_open_wonders(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  if (can_client_change_view() &&
     is_menu_item_active(MENU_REPORT, MENU_REPORT_WOW))
    send_report_request(REPORT_WONDERS_OF_THE_WORLD);
}

/****************************************************************************
  Invoked when the key binding for government->worklists is pressed.
****************************************************************************/
static void xaw_key_open_worklists(Widget w, XEvent *event,
				   String *argv, Cardinal *argc)
{
  if (can_client_change_view()
      && is_menu_item_active(MENU_GOVERNMENT, MENU_GOVERNMENT_WORKLISTS)) {
    popup_worklists_dialog(client.conn.playing);
  }
}

/****************************************************************************
  Invoked when the key binding for orders->airbase is pressed.
****************************************************************************/
static void xaw_key_unit_airbase(Widget w, XEvent *event,
				 String *argv, Cardinal *argc)
{
  if (can_client_issue_orders()
      && is_menu_item_active(MENU_ORDER, MENU_ORDER_AIRBASE)) {
    key_unit_airbase();
  }
}

/****************************************************************************
  Invoked when the key binding for orders->auto_explore is pressed.
****************************************************************************/
static void xaw_key_unit_auto_explore(Widget w, XEvent *event, String *argv,
				      Cardinal *argc)
{
  if(is_menu_item_active(MENU_ORDER, MENU_ORDER_AUTO_EXPLORE))
    key_unit_auto_explore();
}

/****************************************************************************
  Invoked when the key binding for orders->auto_settle is pressed.
****************************************************************************/
static void xaw_key_unit_auto_settle(Widget w, XEvent *event, String *argv,
				     Cardinal *argc)
{
  if(is_menu_item_active(MENU_ORDER, MENU_ORDER_AUTO_SETTLER))
    key_unit_auto_settle();
}

/****************************************************************************
  Invoked when the key binding for auto-settle or auto-attack is pressed.
  Since there is no more auto-attack this function is just like another
  way of calling key_unit_auto_settle.
****************************************************************************/
static void xaw_key_unit_auto_attack_or_settle(Widget w, XEvent *event,
					       String *argv, Cardinal *argc)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    request_unit_autosettlers(punit);
  } unit_list_iterate_end;
}

static void xaw_key_unit_build_city(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  if(is_menu_item_active(MENU_ORDER, MENU_ORDER_BUILD_CITY))
    key_unit_build_city();
}

static void xaw_key_unit_build_city_or_wonder(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    if (can_unit_add_or_build_city(punit)) {
      request_unit_build_city(punit);
    } else {
      request_unit_caravan_action(punit, PACKET_UNIT_HELP_BUILD_WONDER);
    }
  } unit_list_iterate_end;
}

static void xaw_key_unit_build_wonder(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  if(is_menu_item_active(MENU_ORDER, MENU_ORDER_BUILD_WONDER))
    key_unit_build_wonder();
}

static void xaw_key_unit_connect_road(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  if(is_menu_item_active(MENU_ORDER, MENU_ORDER_CONNECT_ROAD))
    key_unit_connect(ACTIVITY_ROAD);
}

static void xaw_key_unit_connect_rail(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  if(is_menu_item_active(MENU_ORDER, MENU_ORDER_CONNECT_RAIL))
    key_unit_connect(ACTIVITY_RAILROAD);
}

static void xaw_key_unit_connect_irrigate(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  if(is_menu_item_active(MENU_ORDER, MENU_ORDER_CONNECT_IRRIGATE))
    key_unit_connect(ACTIVITY_IRRIGATE);
}

static void xaw_key_unit_diplomat_spy_action(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  if(is_menu_item_active(MENU_ORDER, MENU_ORDER_DIPLOMAT_DLG))
    key_unit_diplomat_actions();
}

static void xaw_key_unit_disband(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  if(is_menu_item_active(MENU_ORDER, MENU_ORDER_DISBAND))
    key_unit_disband();
}

static void xaw_key_unit_done(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  if(is_menu_item_active(MENU_ORDER, MENU_ORDER_DONE))
    key_unit_done();
}

static void xaw_key_unit_fallout(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  if(is_menu_item_active(MENU_ORDER, MENU_ORDER_FALLOUT))
    key_unit_fallout();
}

static void xaw_key_unit_fortify(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  if(is_menu_item_active(MENU_ORDER, MENU_ORDER_FORTIFY))
    key_unit_fortify();
}

static void xaw_key_unit_fortify_or_fortress(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    struct base_type *pbase = get_base_by_gui_type(BASE_GUI_FORTRESS,
                                                   punit, punit->tile);
    if (pbase != NULL) {
      key_unit_fortress();
    } else {
      key_unit_fortify();
    }
  } unit_list_iterate_end;
}

static void xaw_key_unit_fortress(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  if(is_menu_item_active(MENU_ORDER, MENU_ORDER_FORTRESS))
    key_unit_fortress();
}

static void xaw_key_unit_goto(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  if(is_menu_item_active(MENU_ORDER, MENU_ORDER_GOTO))
    key_unit_goto();
}

static void xaw_key_unit_homecity(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  if(is_menu_item_active(MENU_ORDER, MENU_ORDER_HOMECITY))
    key_unit_homecity();
}

static void xaw_key_unit_irrigate(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  if(is_menu_item_active(MENU_ORDER, MENU_ORDER_IRRIGATE))
    key_unit_irrigate();
}

static void xaw_key_unit_mine(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  if(is_menu_item_active(MENU_ORDER, MENU_ORDER_MINE))
    key_unit_mine();
}

static void xaw_key_unit_nuke(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  if(is_menu_item_active(MENU_ORDER, MENU_ORDER_NUKE))
    key_unit_nuke();
}

static void xaw_key_unit_paradrop(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  if(is_menu_item_active(MENU_ORDER, MENU_ORDER_PARADROP))
    key_unit_paradrop();
}

static void xaw_key_unit_paradrop_or_pollution(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    if (can_unit_paradrop(punit)) {
      key_unit_paradrop();
    } else {
      key_unit_pollution();
    }
  } unit_list_iterate_end;
}

static void xaw_key_unit_pillage(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  if(is_menu_item_active(MENU_ORDER, MENU_ORDER_PILLAGE))
    key_unit_pillage();
}

static void xaw_key_unit_pollution(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  if(is_menu_item_active(MENU_ORDER, MENU_ORDER_POLLUTION))
    key_unit_pollution();
}

static void xaw_key_unit_patrol(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  if(is_menu_item_active(MENU_ORDER, MENU_ORDER_PATROL))
    key_unit_patrol();
}

static void xaw_key_unit_road(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  if(is_menu_item_active(MENU_ORDER, MENU_ORDER_ROAD))
    key_unit_road();
}

static void xaw_key_unit_road_or_traderoute(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    if (unit_can_est_traderoute_here(punit)) {
      key_unit_traderoute();
    } else {
      key_unit_road();
    }
  } unit_list_iterate_end;
}

/****************************************************************************
  Invoked when the key binding for orders->sentry is pressed.
****************************************************************************/
static void xaw_key_unit_sentry(Widget w, XEvent *event,
				String *argv, Cardinal *argc)
{
  if (is_menu_item_active(MENU_ORDER, MENU_ORDER_SENTRY)) {
    key_unit_sentry();
  }
}

/****************************************************************************
  Invoked when the key binding for orders->make_traderout is pressed.
****************************************************************************/
static void xaw_key_unit_traderoute(Widget w, XEvent *event,
				    String *argv, Cardinal *argc)
{
  if (is_menu_item_active(MENU_ORDER, MENU_ORDER_TRADEROUTE)) {
    key_unit_traderoute();
  }
}

/****************************************************************************
  Invoked when the key binding for orders->transform is pressed.
****************************************************************************/
static void xaw_key_unit_transform(Widget w, XEvent *event,
				   String *argv, Cardinal *argc)
{
  if (is_menu_item_active(MENU_ORDER, MENU_ORDER_TRANSFORM)) {
    key_unit_transform();
  }
}

/****************************************************************************
  Invoked when the key binding for orders->unload_transporter is pressed.
****************************************************************************/
static void xaw_key_unit_unload_all(Widget w, XEvent *event,
				    String *argv, Cardinal *argc)
{
  if (is_menu_item_active(MENU_ORDER, MENU_ORDER_UNLOAD_TRANSPORTER)) {
    key_unit_unload_all();
  }
}

/****************************************************************************
  Invoked when the key binding for orders->load is pressed.
****************************************************************************/
static void xaw_key_unit_load(Widget w, XEvent *event,
			      String *argv, Cardinal *argc)
{
  if (can_client_issue_orders()) {
    unit_list_iterate(get_units_in_focus(), punit) {
      request_unit_load(punit, NULL);
    } unit_list_iterate_end;
  }
}

/****************************************************************************
  Invoked when the key binding for orders->unload is pressed.
****************************************************************************/
static void xaw_key_unit_unload(Widget w, XEvent *event,
				String *argv, Cardinal *argc)
{
  if (can_client_issue_orders()) {
    unit_list_iterate(get_units_in_focus(), punit) {
      request_unit_unload(punit);
    } unit_list_iterate_end;
  }
}
/****************************************************************************
  Invoked when the key binding for orders->wait is pressed.
****************************************************************************/
static void xaw_key_unit_wait(Widget w, XEvent *event,
			      String *argv, Cardinal *argc)
{
  if (is_menu_item_active(MENU_ORDER, MENU_ORDER_WAIT)) {
    key_unit_wait();
  }
}

/****************************************************************************
  Invoked when the key binding for orders->wakeup_others is pressed.
****************************************************************************/
static void xaw_key_unit_wakeup_others(Widget w, XEvent *event,
				       String *argv, Cardinal *argc)
{
  if (is_menu_item_active(MENU_ORDER, MENU_ORDER_WAKEUP_OTHERS)) {
    key_unit_wakeup_others();
  }
}

/****************************************************************************
  Invoked when the key binding for assign_battlegroup is pressed.
****************************************************************************/
static void xaw_key_unit_assign_battlegroup1(Widget w, XEvent *event,
					     String *argv, Cardinal *argc)
{
  assign_battlegroup(0);
}

/****************************************************************************
  Invoked when the key binding for assign_battlegroup is pressed.
****************************************************************************/
static void xaw_key_unit_assign_battlegroup2(Widget w, XEvent *event,
					     String *argv, Cardinal *argc)
{
  assign_battlegroup(1);
}

/****************************************************************************
  Invoked when the key binding for assign_battlegroup is pressed.
****************************************************************************/
static void xaw_key_unit_assign_battlegroup3(Widget w, XEvent *event,
					     String *argv, Cardinal *argc)
{
  assign_battlegroup(2);
}

/****************************************************************************
  Invoked when the key binding for assign_battlegroup is pressed.
****************************************************************************/
static void xaw_key_unit_assign_battlegroup4(Widget w, XEvent *event,
					     String *argv, Cardinal *argc)
{
  assign_battlegroup(3);
}

/****************************************************************************
  Invoked when the key binding for select_battlegroup is pressed.
****************************************************************************/
static void xaw_key_unit_select_battlegroup1(Widget w, XEvent *event,
					     String *argv, Cardinal *argc)
{
  select_battlegroup(0);
}

/****************************************************************************
  Invoked when the key binding for select_battlegroup is pressed.
****************************************************************************/
static void xaw_key_unit_select_battlegroup2(Widget w, XEvent *event,
					     String *argv, Cardinal *argc)
{
  select_battlegroup(1);
}

/****************************************************************************
  Invoked when the key binding for select_battlegroup is pressed.
****************************************************************************/
static void xaw_key_unit_select_battlegroup3(Widget w, XEvent *event,
					     String *argv, Cardinal *argc)
{
  select_battlegroup(2);
}

/****************************************************************************
  Invoked when the key binding for select_battlegroup is pressed.
****************************************************************************/
static void xaw_key_unit_select_battlegroup4(Widget w, XEvent *event,
					     String *argv, Cardinal *argc)
{
  select_battlegroup(3);
}

/****************************************************************************
  Invoked when the key binding for add_unit_to_battlegroup is pressed.
****************************************************************************/
static void xaw_key_unit_add_to_battlegroup1(Widget w, XEvent *event,
					     String *argv, Cardinal *argc)
{
  add_unit_to_battlegroup(0);
}

/****************************************************************************
  Invoked when the key binding for add_unit_to_battlegroup is pressed.
****************************************************************************/
static void xaw_key_unit_add_to_battlegroup2(Widget w, XEvent *event,
					     String *argv, Cardinal *argc)
{
  add_unit_to_battlegroup(1);
}

/****************************************************************************
  Invoked when the key binding for add_unit_to_battlegroup is pressed.
****************************************************************************/
static void xaw_key_unit_add_to_battlegroup3(Widget w, XEvent *event,
					     String *argv, Cardinal *argc)
{
  add_unit_to_battlegroup(2);
}

/****************************************************************************
  Invoked when the key binding for add_unit_to_battlegroup is pressed.
****************************************************************************/
static void xaw_key_unit_add_to_battlegroup4(Widget w, XEvent *event,
					     String *argv, Cardinal *argc)
{
  add_unit_to_battlegroup(3);
}

static void xaw_msg_close_city(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  citydlg_msg_close(w);
}

static void xaw_msg_close_city_report(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  cityrep_msg_close(w);
}

static void xaw_msg_close_economy_report(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  economyreport_msg_close(w);
}

static void xaw_msg_close_help(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  helpdlg_msg_close(w);
}

static void xaw_msg_close_messages(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  meswin_msg_close(w);
}

static void xaw_msg_close_players(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  plrdlg_msg_close(w);
}

static void xaw_msg_close_intel(Widget w, XEvent *event, String *argv,
				      Cardinal *argc)
{
  intel_dialog_msg_close(w);
}

static void xaw_msg_close_intel_diplo(Widget w, XEvent *event, String *argv,
				      Cardinal *argc)
{
  intel_diplo_dialog_msg_close(w);
}

static void xaw_msg_close_science_report(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  sciencereport_msg_close(w);
}

/****************************************************************************
  Action for msg-close-settable-options
****************************************************************************/
static void xaw_msg_close_settable_options(Widget w, XEvent *event,
					   String *argv, Cardinal *argc)
{
  settable_options_msg_close(w);
}

static void xaw_msg_close_spaceship(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  spaceshipdlg_msg_close(w);
}

static void xaw_msg_close_units_report(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  activeunits_msg_close(w);
}

static void xaw_msg_close_start_page(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  start_page_msg_close(w);
}

static void xaw_msg_quit_freeciv(Widget w, XEvent *event, String *argv, Cardinal *argc)
{
  xaw_ui_exit();
}

/*******************************************************************************
 Table of Actions!!
*******************************************************************************/
static XtActionsRec Actions[] = {
  { "mouse-moved", xaw_mouse_moved },
  { "btn-adjust-workers", xaw_btn_adjust_workers },
  { "btn-select-citymap", xaw_btn_select_citymap },
  { "btn-select-mapcanvas", xaw_btn_select_mapcanvas },
  { "btn-select-overviewcanvas", xaw_btn_select_overviewcanvas },
  { "btn-show-info-popup",  xaw_btn_show_info_popup },
  { "key-cancel-action", xaw_key_cancel_action },
  { "key-center-on-unit", xaw_key_center_on_unit },
  { "key-chatline-send", xaw_key_chatline_send },
  { "key-city-names-toggle", xaw_key_city_names_toggle },
  { "key-city-productions-toggle", xaw_key_city_productions_toggle },
  { "key-city-workers", xaw_key_city_workers },
  { "key-dialog-city-close", xaw_key_dialog_city_close },
  { "key-dialog-connect-connect", xaw_key_dialog_connect_connect },
  { "key-dialog-diplo-gold", xaw_key_dialog_diplo_gold },
  { "key-dialog-input-ok", xaw_key_dialog_input_ok },
  { "key-dialog-races-ok", xaw_key_dialog_races_ok },
  { "key-dialog-spaceship-close", xaw_key_dialog_spaceship_close },
  { "key-end-turn", xaw_key_end_turn },
  { "key-focus-to-next-unit", xaw_key_focus_to_next_unit },
  { "key-map-grid-toggle", xaw_key_map_grid_toggle },
  { "key-map-borders-toggle", xaw_key_map_borders_toggle },
  { "key-move-north", xaw_key_move_north },
  { "key-move-north-east", xaw_key_move_north_east },
  { "key-move-east", xaw_key_move_east },
  { "key-move-south-east", xaw_key_move_south_east },
  { "key-move-south", xaw_key_move_south },
  { "key-move-south-west", xaw_key_move_south_west },
  { "key-move-west", xaw_key_move_west },
  { "key-move-north-west", xaw_key_move_north_west },
  { "key-open-city-report", xaw_key_open_city_report },
  { "key-open-demographics", xaw_key_open_demographics },
  { "key-open-economy-report", xaw_key_open_economy_report },
  { "key-open-find-city", xaw_key_open_find_city },
  { "key-open-goto-airlift", xaw_key_open_goto_airlift },
  { "key-open-messages", xaw_key_open_messages },
  { "key-open-players", xaw_key_open_players },
  { "key-open-rates", xaw_key_open_rates },
  { "key-open-revolution", xaw_key_open_revolution },
  { "key-open-science-report", xaw_key_open_science_report },
  { "key-open-spaceship", xaw_key_open_spaceship },
  { "key-open-top-five", xaw_key_open_top_five },
  { "key-open-units-report", xaw_key_open_units_report },
  { "key-open-wonders", xaw_key_open_wonders },
  { "key-open-worklists", xaw_key_open_worklists },
  { "key-unit-airbase", xaw_key_unit_airbase },
  { "key-unit-auto-attack-or-settle", xaw_key_unit_auto_attack_or_settle },
  { "key-unit-auto-explore", xaw_key_unit_auto_explore },
  { "key-unit-auto-settle", xaw_key_unit_auto_settle },
  { "key-unit-build-city", xaw_key_unit_build_city },
  { "key-unit-build-city-or-wonder", xaw_key_unit_build_city_or_wonder },
  { "key-unit-build-wonder", xaw_key_unit_build_wonder },
  { "key-unit-connect-road", xaw_key_unit_connect_road },
  { "key-unit-connect-rail", xaw_key_unit_connect_rail },
  { "key-unit-connect-irrigate", xaw_key_unit_connect_irrigate },
  { "key-unit-diplomat-spy-action", xaw_key_unit_diplomat_spy_action },
  { "key-unit-disband", xaw_key_unit_disband },
  { "key-unit-done", xaw_key_unit_done },
  { "key-unit-fallout", xaw_key_unit_fallout },
  { "key-unit-fortify", xaw_key_unit_fortify },
  { "key-unit-fortify-or-fortress", xaw_key_unit_fortify_or_fortress },
  { "key-unit-fortress", xaw_key_unit_fortress },
  { "key-unit-goto", xaw_key_unit_goto },
  { "key-unit-homecity", xaw_key_unit_homecity },
  { "key-unit-irrigate", xaw_key_unit_irrigate },
  { "key-unit-mine", xaw_key_unit_mine },
  { "key-unit-nuke", xaw_key_unit_nuke },
  { "key-unit-paradrop", xaw_key_unit_paradrop },
  { "key-unit-paradrop-or-pollution", xaw_key_unit_paradrop_or_pollution },
  { "key-unit-pillage", xaw_key_unit_pillage },
  { "key-unit-pollution", xaw_key_unit_pollution },
  { "key-unit-patrol", xaw_key_unit_patrol },
  { "key-unit-road", xaw_key_unit_road },
  { "key-unit-road-or-traderoute", xaw_key_unit_road_or_traderoute },
  { "key-unit-traderoute", xaw_key_unit_traderoute },
  { "key-unit-sentry", xaw_key_unit_sentry },
  { "key-unit-transform", xaw_key_unit_transform },
  { "key-unit-unload-all", xaw_key_unit_unload_all },
  { "key-unit-load", xaw_key_unit_load },
  { "key-unit-unload", xaw_key_unit_unload },
  { "key-unit-wait", xaw_key_unit_wait },
  { "key-unit-wakeup-others", xaw_key_unit_wakeup_others },
  { "key-unit-assign-battlegroup1", xaw_key_unit_assign_battlegroup1 },
  { "key-unit-assign-battlegroup2", xaw_key_unit_assign_battlegroup2 },
  { "key-unit-assign-battlegroup3", xaw_key_unit_assign_battlegroup3 },
  { "key-unit-assign-battlegroup4", xaw_key_unit_assign_battlegroup4 },
  { "key-unit-select-battlegroup1", xaw_key_unit_select_battlegroup1 },
  { "key-unit-select-battlegroup2", xaw_key_unit_select_battlegroup2 },
  { "key-unit-select-battlegroup3", xaw_key_unit_select_battlegroup3 },
  { "key-unit-select-battlegroup4", xaw_key_unit_select_battlegroup4 },
  { "key-unit-add-to-battlegroup1", xaw_key_unit_add_to_battlegroup1 },
  { "key-unit-add-to-battlegroup2", xaw_key_unit_add_to_battlegroup2 },
  { "key-unit-add-to-battlegroup3", xaw_key_unit_add_to_battlegroup3 },
  { "key-unit-add-to-battlegroup4", xaw_key_unit_add_to_battlegroup4 },
  { "msg-close-city", xaw_msg_close_city },
  { "msg-close-city-report", xaw_msg_close_city_report },
  { "msg-close-economy-report", xaw_msg_close_economy_report },
  { "msg-close-help", xaw_msg_close_help },
  { "msg-close-messages", xaw_msg_close_messages },
  { "msg-close-players", xaw_msg_close_players },
  { "msg-close-intel", xaw_msg_close_intel },
  { "msg-close-intel-diplo", xaw_msg_close_intel_diplo },
  { "msg-close-science-report", xaw_msg_close_science_report },
  { "msg-close-settable-options", xaw_msg_close_settable_options },
  { "msg-close-spaceship", xaw_msg_close_spaceship },
  { "msg-close-units-report", xaw_msg_close_units_report },
  { "msg-close-start-page", xaw_msg_close_start_page },
  { "msg-quit-freeciv", xaw_msg_quit_freeciv }
};

/*******************************************************************************
 Initialize our Actions!!
*******************************************************************************/
void InitializeActions (XtAppContext app_context)
{
  XtAppAddActions(app_context, Actions, XtNumber(Actions));
}
