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

#ifndef FC__FC_CLIENT_H
#define FC__FC_CLIENT_H

// In this case we have to include fc_config.h from header file.
// Some other headers we include demand that fc_config.h must be
// included also. Usually all source files include fc_config.h, but
// there's moc generated meta_fc_client.cpp file which does not.
#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

// Qt
#include <QMainWindow>
#include <QPixmapCache>
#include <QStackedWidget>

// common
#include "packets.h"

// client
#include "chatline_common.h"
#include "client_main.h"
#include "climisc.h"
#include "clinet.h"
#include "mapview_common.h"
#include "servers.h"
#include "tilespec.h"

// gui-qt
#include "canvas.h"
#include "chatline.h"
#include "dialogs.h"
#include "fonts.h"
#include "gotodlg.h"
#include "mapview.h"
#include "menu.h"
#include "messagewin.h"
#include "pages.h"
#include "ratesdlg.h"
#include "voteinfo_bar.h"

enum connection_state {
  LOGIN_TYPE,
  NEW_PASSWORD_TYPE,
  ENTER_PASSWORD_TYPE,
  WAITING_TYPE
};

class fc_sidebar;
class fc_sidetax;
class fc_sidewidget;
class MainWindow;
class pregame_options;
class QApplication;
class QDialog;
class QLabel;
class QLineEdit;
class QMainWindow;
class QSocketNotifier;
class QSpinBox;
class QStackedLayout;
class QStatusBar;
class QString;
class QTableWidget;
class QTextEdit;
class QTimer;
class QTreeWidget;
struct fc_shortcut;


/****************************************************************************
  Class helping reading icons/pixmaps from themes/gui-qt/icons folder
****************************************************************************/
class fc_icons
{
  Q_DISABLE_COPY(fc_icons);

private:
  explicit fc_icons();
  static fc_icons* m_instance;

public:
  static fc_icons* instance();
  static void drop();
  QIcon get_icon(const QString& id);
  QPixmap *get_pixmap(const QString& id);
  QString get_path(const QString& id);
};

/****************************************************************************
  Widget holding all game tabs
****************************************************************************/
class fc_game_tab_widget: public QStackedWidget
{
  Q_OBJECT
public:
  fc_game_tab_widget();
  void init();
protected:
  void resizeEvent(QResizeEvent *event);
private slots:
  void current_changed(int index);
};

/****************************************************************************
  Some qt-specific options like size to save between restarts
****************************************************************************/
struct fc_settings
{
  float chat_fwidth;
  float chat_fheight;
  float chat_fx_pos;
  float chat_fy_pos;
  int player_repo_sort_col;
  bool show_new_turn_text;
  bool show_battle_log;
  Qt::SortOrder player_report_sort;
  int city_repo_sort_col;
  Qt::SortOrder city_report_sort;
  QByteArray city_geometry;
  QByteArray city_splitter1;
  QByteArray city_splitter2;
  QByteArray city_splitter3;
  QByteArray help_geometry;
  QByteArray help_splitter1;
  float unit_info_pos_fx;
  float unit_info_pos_fy;
  float minimap_x;
  float minimap_y;
  float minimap_width;
  float minimap_height;
  float battlelog_scale;
  float battlelog_x;
  float battlelog_y;
};

/****************************************************************************
  Corner widget for menu
****************************************************************************/
class fc_corner : public QWidget
{
  Q_OBJECT
  QMainWindow *mw;
public:
  fc_corner(QMainWindow *qmw);
public slots:
  void maximize();
  void minimize();
  void close_fc();
};


class fc_client : public QMainWindow,
                  private chat_listener
{
  Q_OBJECT
  QWidget *main_wdg;
  QWidget *pages[ (int) PAGE_GAME + 2];
  QWidget *connect_lan;
  QWidget *connect_metaserver;
  QWidget *game_main_widget;

  QGridLayout *pages_layout[PAGE_GAME + 2];
  QStackedLayout *central_layout;
  QGridLayout *game_layout;

  QTextEdit *output_window;
  QTextEdit *scenarios_view;
  QLabel *scenarios_text;
  QLabel *load_save_text;
  QLabel *load_pix;
  QCheckBox *show_preview;

  QLineEdit *connect_host_edit;
  QLineEdit *connect_port_edit;
  QLineEdit *connect_login_edit;
  QLineEdit *connect_password_edit;
  QLineEdit *connect_confirm_password_edit;

  QPushButton *button;
  QPushButton *obs_button;
  QPushButton *start_button;
  QPushButton *nation_button;

  QDialogButtonBox* button_box;

  QSocketNotifier *server_notifier;

  chat_input *chat_line;

  QTableWidget* lan_widget;
  QTableWidget* wan_widget;
  QTableWidget* info_widget;
  QTableWidget* saves_load;
  QTableWidget* scenarios_load;
  QTreeWidget* start_players_tree;

  QTimer* meta_scan_timer;
  QTimer* lan_scan_timer;
  QTimer *update_info_timer;

  QStatusBar *status_bar;
  QSignalMapper *switch_page_mapper;
  QLabel *status_bar_label;
  info_tile *info_tile_wdg;
  choice_dialog *opened_dialog;
  fc_sidewidget *sw_map;
  fc_sidewidget *sw_cities;
  fc_sidewidget *sw_tax;
  fc_sidewidget *sw_economy;

public:
  fc_client();
  ~fc_client();

  void fc_main(QApplication *);
  map_view *mapview_wdg;
  fc_sidebar *sidebar_wdg;
  minimap_view *minimapview_wdg;
  void add_server_source(int);
  void remove_server_source();
  bool event(QEvent *event);

  enum client_pages current_page();

  void set_status_bar(QString str, int timeout = 2000);
  int add_game_tab(QWidget *widget);
  void rm_game_tab(int index); /* doesn't delete widget */
  void update_start_page();
  void toggle_unit_sel_widget(struct tile *ptile);
  void update_unit_sel();
  void popdown_unit_sel();
  void popup_tile_info(struct tile *ptile);
  void popdown_tile_info();
  void set_diplo_dialog(choice_dialog *widget);
  void handle_authentication_req(enum authentication_type type,
                                 const char *message);
  choice_dialog *get_diplo_dialog();
  void update_sidebar_position();

  mr_idle mr_idler;
  QWidget *central_wdg;
  mr_menu *menu_bar;
  fc_corner *corner_wid;
  fc_game_tab_widget *game_tab_widget;
  messagewdg *msgwdg;
  info_tab *infotab;
  pregamevote *pre_vote;
  units_select *unit_sel;
  xvote *x_vote;
  goto_dialog *gtd;
  QCursor *fc_cursors[CURSOR_LAST][NUM_CURSOR_FRAMES];
  pregame_options *pr_options;
  fc_settings qt_settings;
  trade_generator trade_gen;
  qfc_rally_list rallies;
  hud_units *unitinfo_wdg;
  hud_battle_log *battlelog_wdg;
  bool interface_locked;
  fc_sidewidget *sw_cunit;
  fc_sidewidget *sw_science;
  fc_sidewidget *sw_endturn;
  fc_sidewidget *sw_indicators;
  fc_sidewidget *sw_diplo;
  float map_scale;
  bool map_font_scale;
  void gimme_place(QWidget* widget, QString str);
  int gimme_index_of(QString str);
  void remove_repo_dlg(QString str);
  bool is_repo_dlg_open(QString str);
  void write_settings();
  bool is_closing();
  void update_sidebar_tooltips();
  void reload_sidebar_icons();

private slots:
  void send_fake_chat_message(const QString &message);
  void server_input(int sock);
  void closing();
  void slot_lan_scan();
  void slot_meta_scan();
  void slot_connect();
  void slot_disconnect();
  void slot_pregame_observe();
  void slot_pregame_start();
  void update_network_lists();
  void start_page_menu(QPoint);
  void slot_pick_nation();
  void start_new_game();
  void start_scenario();
  void start_from_save();
  void browse_saves();
  void browse_scenarios();
  void clear_status_bar();
  void state_preview(int);

public slots:
  void switch_page(int i);
  void popup_client_options();
  void update_info_label();
  void quit();

protected slots:

  void slot_selection_changed(const QItemSelection&, const QItemSelection&);

private:
  void chat_message_received(const QString &message,
                             const struct text_tag_list *tags);
  void create_main_page();
  void create_network_page();
  void create_load_page();
  void create_scenario_page();
  void create_start_page();
  void create_game_page();
  void create_loading_page();
  bool chat_active_on_page(enum client_pages);
  void destroy_server_scans (void);
  void update_server_list(enum server_scan_type sstype,
                          const struct server_list *list);
  bool check_server_scan(server_scan *scan_data);
  void update_load_page(void);
  void create_cursors(void);
  void delete_cursors(void);
  void update_scenarios_page(void);
  void set_connection_state(enum connection_state state);
  void update_buttons();
  void init();
  void read_settings();

  enum client_pages page;
  QMap<QString, QWidget*> opened_repo_dlgs;
  QStringList status_bar_queue;
  QString current_file;
  bool send_new_aifill_to_server;
  bool quitting;

protected:
  void timerEvent(QTimerEvent *);
  void closeEvent(QCloseEvent *event);

signals:
  void keyCaught(QKeyEvent *e);

};

/***************************************************************************
  Class for showing options in PAGE_START, options like ai_fill, ruleset
  etc.
***************************************************************************/
class pregame_options : public QWidget
{
  Q_OBJECT
  QComboBox *ailevel;
  QComboBox *cruleset;
  QPushButton *nation;
  QSpinBox *max_players;
public:
  pregame_options(QWidget *parent);
  void init();

  void set_rulesets(int num_rulesets, char **rulesets);
  void set_aifill(int aifill);
  void update_ai_level();
  void update_buttons();
private slots:
  void max_players_change(int i);
  void ailevel_change(int i);
  void ruleset_change(int i);
  void pick_nation();
public slots:
  void popup_server_options();
};

// Return fc_client instance. Implementation in gui_main.cpp
class fc_client *gui();

#endif /* FC__FC_CLIENT_H */


