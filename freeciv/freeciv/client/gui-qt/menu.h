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

#ifndef FC__MENU_H
#define FC__MENU_H

extern "C" {
#include "menu_g.h"
}

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

// Qt
#include <QDialog>
#include <QMenuBar>

// client
#include "control.h"

class QLabel;
class QPushButton;
class QSignalMapper;
class QScrollArea;
struct fc_shortcut;

void qt_start_turn();

/** used for indicating menu about current option - for renaming
 * and enabling, disabling */
enum munit {
  STANDARD,
  EXPLORE,
  LOAD,
  UNLOAD,
  TRANSPORTER,
  DISBAND,
  CONVERT,
  MINE,
  IRRIGATION,
  TRANSFORM,
  PILLAGE,
  BUILD,
  ROAD,
  FORTIFY,
  FORTRESS,
  AIRBASE,
  POLLUTION,
  FALLOUT,
  SENTRY,
  HOMECITY,
  WAKEUP,
  AUTOSETTLER,
  CONNECT_ROAD,
  CONNECT_RAIL,
  CONNECT_IRRIGATION,
  GOTO_CITY,
  AIRLIFT,
  BUILD_WONDER,
  AUTOTRADEROUTE,
  ORDER_TRADEROUTE,
  ORDER_DIPLOMAT_DLG,
  UPGRADE,
  NOT_4_OBS,
  MULTIPLIERS,
  ENDGAME,
  SAVE
};

enum delay_order{
  D_GOTO,
  D_NUKE,
  D_PARADROP,
  D_FORT
};

/**************************************************************************
  Struct holding rally point for city
**************************************************************************/
struct qfc_rally
{
  struct city *pcity;
  struct tile *ptile;
};

/**************************************************************************
  Class holding city list for rally points
**************************************************************************/
class qfc_rally_list
{
public:
  qfc_rally_list() {
    hover_tile = false;
    hover_city = false;
  };
  void add(qfc_rally* rally);
  bool clear(struct city *rcity);
  QList<qfc_rally*> rally_list;
  void run();
  bool hover_tile;
  bool hover_city;
  struct city *rally_city;
};

void multiairlift(struct city *acity, Unit_type_id ut);

/**************************************************************************
  Class representing one unit for delayed goto
**************************************************************************/
class qfc_delayed_unit_item
{
public:
  qfc_delayed_unit_item(delay_order dg, int i) {
   order = dg;
   id = i;
   ptile = nullptr;
  }
  delay_order order;
  int id;
  struct tile *ptile;
};

/**************************************************************************
  Class holding unit list for delayed goto
**************************************************************************/
class qfc_units_list
{
public:
  qfc_units_list();
  void add(qfc_delayed_unit_item* fui);
  void clear();
  QList<qfc_delayed_unit_item*> unit_list;
  int nr_units;
};

/**************************************************************************
  Helper item for trade calculation
***************************************************************************/
class trade_city
{
public:
  trade_city(struct city *pcity);

  bool done;
  int over_max;
  int poss_trade_num;
  int trade_num; // already created + generated
  QList<struct city *> curr_tr_cities;
  QList<struct city *> new_tr_cities;
  QList<struct city *> pos_cities;
  struct city *city;
  struct tile *tile;

};

/**************************************************************************
  Struct of 2 tiles, used for drawing trade routes.
  Also assigned caravan if it was sent
***************************************************************************/
struct qtiles
{
  struct tile *t1;
  struct tile *t2;
  struct unit *autocaravan;

  bool operator==(const qtiles& a) const
  {
    return (t1 == a.t1 && t2 == a.t2 && autocaravan == a.autocaravan);
  }
};

/**************************************************************************
  Class trade generator, used for calulating possible trade routes
***************************************************************************/
class trade_generator
{
public:
  trade_generator();

  bool hover_city;
  QList<qtiles> lines;
  QList<struct city *> virtual_cities;
  QList<trade_city*> cities;

  void add_all_cities();
  void add_city(struct city *pcity);
  void add_tile(struct tile *ptile);
  void calculate();
  void clear_trade_planing();
  void remove_city(struct city *pcity);
  void remove_virtual_city(struct tile *ptile);

private:
  bool discard_any(trade_city *tc, int freeroutes);
  bool discard_one(trade_city *tc);
  int find_over_max(struct city *pcity);
  trade_city* find_most_free();
  void check_if_done(trade_city *tc1, trade_city *tc2);
  void discard();
  void discard_trade(trade_city *tc1, trade_city *tc2);
  void find_certain_routes();
};


/****************************************************************************
  Instantiable government menu.
****************************************************************************/
class gov_menu : public QMenu
{
  Q_OBJECT
  static QSet<gov_menu *> instances;

  QSignalMapper *gov_mapper;
  QVector<QAction *> actions;

public:
  gov_menu(QWidget* parent = 0);
  virtual ~gov_menu();

  static void create_all();
  static void update_all();

public slots:
  void revolution();
  void change_gov(int target_gov);

  void create();
  void update();
};

/****************************************************************************
  Go to and... menu.
****************************************************************************/
class go_act_menu : public QMenu
{
  Q_OBJECT
  static QSet<go_act_menu *> instances;

  QSignalMapper *go_act_mapper;
  QMap<QAction *, int> items;

public:
  go_act_menu(QWidget* parent = 0);
  virtual ~go_act_menu();

  static void reset_all();
  static void update_all();

public slots:
  void start_go_act(int act_id);

  void reset();
  void create();
  void update();
};

/**************************************************************************
  Class representing global menus in gameview
**************************************************************************/
class mr_menu : public QMenuBar
{
  Q_OBJECT
  QMenu *airlift_menu;
  QMenu *bases_menu;
  QMenu *menu;
  QMenu *multiplayer_menu;
  QMenu *roads_menu;
  QActionGroup *airlift_type;
  QActionGroup *action_vs_city;
  QActionGroup *action_vs_unit;
  QMenu *action_unit_menu;
  QMenu *action_city_menu;
  QHash<munit, QAction*> menu_list;
  qfc_units_list units_list;
public:
  mr_menu();
  void setup_menus();
  void menus_sensitive();
  void update_airlift_menu();
  void update_roads_menu();
  void update_bases_menu();
  void set_tile_for_order(struct tile *ptile);
  void execute_shortcut(int sid);
  QString shortcut_exist(fc_shortcut *fcs);
  QString shortcut_2_menustring(int sid);
  QAction *minimap_status;
  QAction *scale_fonts_status;
  QAction *lock_status;
  QAction *osd_status;
  QAction *btlog_status;
  QAction *chat_status;
  QAction *messages_status;
  bool delayed_order;
  bool quick_airlifting;
  Unit_type_id airlift_type_id;
private slots:
  /* game menu */
  void local_options();
  void shortcut_options();
  void server_options();
  void messages_options();
  void save_options_now();
  void save_game();
  void save_game_as();
  void save_image();
  void tileset_custom_load();
  void load_new_tileset();
  void back_to_menu();
  void quit_game();

  /* help menu */
  void slot_help(const QString &topic);

  /*used by work menu*/
  void slot_build_path(int id);
  void slot_build_base(int id);
  void slot_build_city();
  void slot_auto_settler();
  void slot_build_road();
  void slot_build_irrigation();
  void slot_build_mine();
  void slot_conn_road();
  void slot_conn_rail();
  void slot_conn_irrigation();
  void slot_transform();
  void slot_clean_pollution();
  void slot_clean_fallout();

  /*used by unit menu */
  void slot_unit_sentry();
  void slot_unit_explore();
  void slot_unit_goto();
  void slot_airlift();
  void slot_return_to_city();
  void slot_patrol();
  void slot_unsentry();
  void slot_load();
  void slot_unload();
  void slot_unload_all();
  void slot_set_home();
  void slot_upgrade();
  void slot_convert();
  void slot_disband();

  /*used by combat menu*/
  void slot_unit_fortify();
  void slot_unit_fortress();
  void slot_unit_airbase();
  void slot_pillage();
  void slot_action();

  /*used by view menu*/
  void slot_center_view();
  void slot_minimap_view();
  void slot_show_new_turn_text();
  void slot_battlelog();
  void slot_fullscreen();
  void slot_lock();
  void slot_city_outlines();
  void slot_city_output();
  void slot_map_grid();
  void slot_borders();
  void slot_fullbar();
  void slot_native_tiles();
  void slot_city_growth();
  void slot_city_production();
  void slot_city_buycost();
  void slot_city_traderoutes();
  void slot_city_names();
  void zoom_in();
  void zoom_scale_fonts();
  void zoom_reset();
  void zoom_out();

  /*used by select menu */
  void slot_select_one();
  void slot_select_all_tile();
  void slot_select_same_tile();
  void slot_select_same_continent();
  void slot_select_same_everywhere();
  void slot_done_moving();
  void slot_wait();
  void slot_unit_filter();

  /* used by multiplayer menu */
  void slot_orders_clear();
  void slot_execute_orders();
  void slot_delayed_goto();
  void slot_trade_add_all();
  void slot_trade_city();
  void slot_calculate();
  void slot_clear_trade();
  void slot_autocaravan();
  void slot_rally();
  void slot_quickairlift_set();
  void slot_quickairlift();
  void slot_action_vs_unit();
  void slot_action_vs_city();

  /*used by civilization menu */
  void slot_show_map();
  void calc_trade_routes();
  void slot_popup_tax_rates();
  void slot_popup_mult_rates();
  void slot_show_eco_report();
  void slot_show_units_report();
  void slot_show_nations();
  void slot_show_cities();
  void slot_show_research_tab();
  void slot_spaceship();
  void slot_demographics();
  void slot_achievements();
  void slot_endgame();
  void slot_top_five();
  void slot_traveler();

private:
  struct tile* find_last_unit_pos(struct unit* punit, int pos);
  QSignalMapper *signal_help_mapper;
  QSignalMapper *build_bases_mapper;
  QSignalMapper *build_roads_mapper;
};

#endif /* FC__MENU_H */
