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

#ifndef FC__REPODLGS_H
#define FC__REPODLGS_H

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

extern "C" {
#include "repodlgs_g.h"
}

// client
#include "climisc.h"

// gui-qt
#include "fonts.h"
#include "mapview.h"

// Qt
#include <QLabel>
#include <QPushButton>
#include <QWidget>

class progress_bar;
class QComboBox;
class QGridLayout;
class QGroupBox;
class QHBoxLayout;
class QItemSelection;
class QPushButton;
class QScrollArea;
class QTableWidget;
class QTableWidgetItem;

class unittype_item: public QFrame {

  Q_OBJECT

  bool entered;
  int unit_scroll;
  QLabel label_pix;

public:
  unittype_item(QWidget *parent, unit_type *ut);
  ~unittype_item();
  void init_img();
  QLabel food_upkeep;
  QLabel gold_upkeep;
  QLabel label_info_active;
  QLabel label_info_inbuild;
  QLabel label_info_unit;
  QLabel shield_upkeep;
  QPushButton upgrade_button;

private:
  unit_type *utype;

private slots:
  void upgrade_units();

protected:
  void enterEvent(QEvent *event);
  void leaveEvent(QEvent *event);
  void paintEvent(QPaintEvent *event);
  void wheelEvent(QWheelEvent *event);
};

class units_reports: public fcwidget {

  Q_DISABLE_COPY(units_reports);
  Q_OBJECT

  close_widget *cw;
  explicit units_reports();
  QHBoxLayout *scroll_layout;
  QScrollArea *scroll;
  QWidget scroll_widget;
  static units_reports *m_instance;

public:
  ~units_reports();
  static units_reports *instance();
  static void drop();
  void clear_layout();
  void init_layout();
  void update_units(bool show = false);
  void add_item(unittype_item *item);
  virtual void update_menu();
  QHBoxLayout *layout;
  QList<unittype_item *> unittype_list;

protected:
  void paintEvent(QPaintEvent *event);
};

/****************************************************************************
  Helper item for comboboxes, holding string of tech and its id
****************************************************************************/
struct qlist_item {
  QString tech_str;
  Tech_type_id id;
};

/****************************************************************************
  Helper item for research diagram, about drawn rectangles and what
  tech/unit/improvement they point to.
****************************************************************************/
class req_tooltip_help
{
public:
  req_tooltip_help();
  QRect rect;
  Tech_type_id tech_id;
  struct unit_type *tunit;
  struct impr_type *timpr;
  struct government *tgov;
};
/****************************************************************************
  Custom widget representing research diagram in science_report
****************************************************************************/
class research_diagram: public QWidget
{
  Q_OBJECT

public:
  research_diagram(QWidget *parent = 0);
  ~research_diagram();
  void update_reqtree();
  void reset();
  QSize size();
private slots:
  void show_tooltip();
private:
  void mousePressEvent(QMouseEvent *event);
  void mouseMoveEvent(QMouseEvent *event);
  void paintEvent(QPaintEvent *event);
  void create_tooltip_help();
  struct canvas *pcanvas;
  struct reqtree *req;
  bool timer_active;
  int width;
  int height;
  QList<req_tooltip_help*> tt_help;
  QPoint tooltip_pos;
  QString tooltip_text;
  QRect tooltip_rect;
  
};

/****************************************************************************
  Widget embedded as tab on game view (F6 default)
  Uses string "SCI" to mark it as opened
  You can check it using if (gui()->is_repo_dlg_open("SCI"))
****************************************************************************/
class science_report: public QWidget
{
  Q_OBJECT

  QComboBox *goal_combo;
  QComboBox *researching_combo;
  QGridLayout *sci_layout;
  progress_bar *progress;
  QLabel *info_label;
  QLabel *progress_label;
  QList<qlist_item> *curr_list;
  QList<qlist_item> *goal_list;
  research_diagram *res_diag;
  QScrollArea *scroll;

public:
  science_report();
  ~science_report();
  void update_report();
  void init(bool raise);
  void redraw();
  void reset_tree();

private:
  void update_reqtree();
  int index;

private slots:
  void current_tech_changed(int index);
  void goal_tech_changed(int index);
};


/****************************************************************************
  Tab widget to display economy report (F5)
****************************************************************************/
class eco_report: public QWidget
{
  Q_OBJECT
  QPushButton *disband_button;
  QPushButton *sell_button;
  QPushButton *sell_redun_button;
  QTableWidget *eco_widget;
  QLabel *eco_label;

public:
  eco_report();
  ~eco_report();
  void update_report();
  void init();

private:
  int index;
  int curr_row;
  int max_row;
  cid uid;
  int counter;

private slots:
  void disband_units();
  void sell_buildings();
  void sell_redundant();
  void selection_changed(const QItemSelection &sl,
                         const QItemSelection &ds);
};

/****************************************************************************
  Tab widget to display economy report (F5)
****************************************************************************/
class endgame_report: public QWidget
{
  Q_OBJECT
  QTableWidget *end_widget;

public:
  endgame_report(const struct packet_endgame_report *packet);
  ~endgame_report();
  void update_report(const struct packet_endgame_player *packet);
  void init();

private:
  int index;
  int players;

};


bool comp_less_than(const qlist_item &q1, const qlist_item &q2);
void popdown_economy_report();
void popdown_units_report();
void popdown_science_report();
void popdown_endgame_report();
void popup_endgame_report();
void toggle_units_report(bool);

#endif /* FC__REPODLGS_H */
