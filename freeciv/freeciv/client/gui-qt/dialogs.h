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

#ifndef FC__DIALOGS_H
#define FC__DIALOGS_H

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

extern "C" {
#include "dialogs_g.h"
}

// Qt
#include <QDialog>
#include <QMessageBox>
#include <QVariant>

// gui-qt
#include "fonts.h"
#include "hudwidget.h"
#include "mapview.h"

class QComboBox;
class QGridLayout;
class QGroupBox;
class QItemSelection;
class QRadioButton;
class QSignalMapper;
class QTableView;
class QTableWidget;
class QTextEdit;
class QWidget;

typedef void (*pfcn_void)(QVariant, QVariant);
void update_nationset_combo();
void popup_races_dialog(struct player *pplayer);
void restart_notify_dialogs();


class qdef_act
{
  Q_DISABLE_COPY(qdef_act);

private:
  explicit qdef_act();
  static qdef_act* m_instance;
  action_id vs_city;
  action_id vs_unit;
public:
  static qdef_act* action();
  static void drop();
  void vs_city_set(int i);
  void vs_unit_set(int i);
  action_id vs_city_get();
  action_id vs_unit_get();
};

/***************************************************************************
  Dialog with themed titlebar
***************************************************************************/
class qfc_dialog : public QDialog
{
  Q_OBJECT
public:
  qfc_dialog(QWidget *parent);
private:
  int titlebar_height;
  QPoint point;
  bool moving_now;
  QPixmap close_pix;
protected:
  void paintEvent(QPaintEvent* event);
  void mouseMoveEvent(QMouseEvent *event);
  void mousePressEvent(QMouseEvent *event);
  void mouseReleaseEvent(QMouseEvent *event);
};

/***************************************************************************
  Nonmodal message box for disbanding units
***************************************************************************/
class disband_box : public hud_message_box
{
  Q_OBJECT
  struct unit_list *cpunits;
public:
  explicit disband_box(struct unit_list *punits, QWidget *parent = 0);
  ~disband_box();
private slots:
  void disband_clicked();
};

/***************************************************************************
 Dialog for goto popup
***************************************************************************/
class notify_goto : public QMessageBox
{
  Q_OBJECT
  QPushButton *goto_but;
  QPushButton *inspect_but;
  QPushButton *close_but;
  struct tile *gtile;
public:
  notify_goto(const char *headline, const char *lines,
              const struct text_tag_list *tags, struct tile *ptile,
              QWidget *parent);

private slots:
  void goto_tile();
  void inspect_city();
};

/***************************************************************************
 Dialog for selecting nation, style and leader leader
***************************************************************************/
class races_dialog:public qfc_dialog
{
  Q_OBJECT
  QGridLayout *main_layout;
  QTableWidget *nation_tabs;
  QList<QWidget*> *nations_tabs_list;
  QTableWidget *selected_nation_tabs;
  QComboBox *leader_name;
  QComboBox *qnation_set;
  QRadioButton *is_male;
  QRadioButton *is_female;
  QTableWidget *styles;
  QTextEdit *description;
  QPushButton *ok_button;
  QPushButton *random_button;

public:
  races_dialog(struct player *pplayer, QWidget *parent = 0);
  ~races_dialog();
  void refresh();
  void update_nationset_combo();

private slots:
  void set_index(int index);
  void nation_selected(const QItemSelection &sl, const QItemSelection &ds);
  void style_selected(const QItemSelection &sl, const QItemSelection &ds);
  void group_selected(const QItemSelection &sl, const QItemSelection &ds);
  void nationset_changed(int index);
  void leader_selected(int index);
  void ok_pressed();
  void cancel_pressed();
  void random_pressed();

private:
  int selected_nation;
  int selected_style;
  int selected_sex;
  struct player *tplayer;
  int last_index;
};

/***************************************************************************
 Widget around map view to display informations like demographics report,
 top 5 cities, traveler's report.
***************************************************************************/
class notify_dialog:public fcwidget
{
  Q_OBJECT
public:
  notify_dialog(const char *caption, const char *headline,
                const char *lines, QWidget *parent = 0);
  virtual void update_menu();
  ~notify_dialog();
  void restart();
protected:
  void mousePressEvent(QMouseEvent *event);
  void mouseMoveEvent(QMouseEvent *event);
  void mouseReleaseEvent(QMouseEvent *event);
private:
  void paintEvent(QPaintEvent *paint_event);
  void calc_size(int &x, int&y);
  close_widget *cw;
  QLabel *label;
  QVBoxLayout *layout;
  QString qcaption;
  QString qheadline;
  QStringList qlist;
  QFont small_font;
  QPoint cursor;
};

/***************************************************************************
 Transparent widget for selecting units
***************************************************************************/
class units_select: public fcwidget
{
  Q_OBJECT
  QPixmap *pix;
  QPixmap *h_pix; /** pixmap for highlighting */
  QSize item_size; /** size of each pixmap of unit */
  QList<unit*> unit_list; /** storing units only for drawing, for rest units
                            * iterate utile->units */
  QFont ufont;
  QFont info_font;
  int row_count;
  close_widget *cw;
public:
  units_select(struct tile *ptile, QWidget *parent);
  ~units_select();
  void update_menu();
  void update_units();
  void create_pixmap();
  tile *utile;
protected:
  void paint(QPainter *painter, QPaintEvent *event);
  void paintEvent(QPaintEvent *event);
  void mousePressEvent(QMouseEvent *event);
  void keyPressEvent(QKeyEvent *event);
  void mouseMoveEvent(QMouseEvent *event);
  void wheelEvent(QWheelEvent *event);
  void closeEvent(QCloseEvent *event);
private slots:
  void update_img();
private:
  bool more;
  int show_line;
  int highligh_num;
  int unit_count;
};

/**************************************************************************
  A QPushButton that includes data like function to call and parmeters
**************************************************************************/
class Choice_dialog_button: public QPushButton
{
  Q_OBJECT
  pfcn_void func;
  QVariant data1, data2;
public:
  Choice_dialog_button(const QString title, pfcn_void func_in,
                       QVariant data1_in, QVariant data2_in);
  pfcn_void getFunc();
  QVariant getData1();
  QVariant getData2();
  void setData1(QVariant wariat);
  void setData2(QVariant wariat);
};

/***************************************************************************
  Simple choice dialog, allowing choosing one of set actions
***************************************************************************/
class choice_dialog: public QWidget
{
  Q_OBJECT
  QPushButton *target_unit_button;
  QSignalMapper *signal_mapper;
  QVBoxLayout *layout;
  QHBoxLayout *unit_skip;
  QList<Choice_dialog_button *> buttons_list;
  QList<Choice_dialog_button *> last_buttons_stack;
  QList<Choice_dialog_button *> action_button_map;
  void (*run_on_close)(int);
  void switch_target();
public:
  choice_dialog(const QString title, const QString text,
                QWidget *parent = NULL,
                void (*run_on_close_in)(int) = NULL);
  ~choice_dialog();
  void set_layout();
  void add_item(QString title, pfcn_void func, QVariant data1,
                QVariant data2, QString tool_tip, const int button_id);
  void show_me();
  void stack_button(Choice_dialog_button *button);
  void unstack_all_buttons();
  QVBoxLayout *get_layout();
  Choice_dialog_button *get_identified_button(const int id);
  int unit_id;
  int target_id[ATK_COUNT];
  int target_extra_id;
  struct unit* targeted_unit;
  void update_dialog(const struct act_prob *act_probs);
public slots:
  void execute_action(const int action);
private slots:
  void prev_unit();
  void next_unit();
};

void popup_revolution_dialog(struct government *government = NULL);
void revolution_response(struct government *government);
void popup_upgrade_dialog(struct unit_list *punits);
void popup_disband_dialog(struct unit_list *punits);
bool try_default_unit_action(QVariant q1, QVariant q2);
bool try_default_city_action(QVariant q1, QVariant q2);

#endif /* FC__DIALOGS_H */
