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

#ifndef FC__PLRDLG_H
#define FC__PLRDLG_H

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

extern "C" {
#include "plrdlg_g.h"
}

//common
#include "colors.h"
#include "player.h"
#include "research.h"

// gui-qt
#include "sprite.h"

// Qt
#include <QAbstractListModel>
#include <QItemDelegate>
#include <QTreeView>
#include <QWidget>

class QHBoxLayout;
class QLabel;
class QPushButton;
class QSortFilterProxyModel;
class QSplitter;
class QTableWidget;
class QVBoxLayout;
class plr_report;

/***************************************************************************
  Item delegate for painting in model of nations view table
***************************************************************************/
class plr_item_delegate:public QItemDelegate {
  Q_OBJECT

public:
 plr_item_delegate(QObject *parent) : QItemDelegate(parent) {}
 ~plr_item_delegate() {}
 void paint(QPainter *painter, const QStyleOptionViewItem &option,
            const QModelIndex &index) const;
 virtual QSize sizeHint (const QStyleOptionViewItem & option,
                         const QModelIndex & index ) const;
};

/***************************************************************************
  Single item in model of nations view table
***************************************************************************/
class plr_item: public QObject {
Q_OBJECT

public:
  plr_item(struct player *pplayer);
  inline int columnCount() const { return num_player_dlg_columns; }
  QVariant data(int column, int role = Qt::DisplayRole) const;
  bool setData(int column, const QVariant &value, int role = Qt::DisplayRole);
private:
  struct player *ipplayer;
};

/***************************************************************************
  Nation/Player model
***************************************************************************/
class plr_model : public QAbstractListModel
{
  Q_OBJECT
public:
  plr_model(QObject *parent = 0);
  ~plr_model();
  inline int rowCount(const QModelIndex &index = QModelIndex()) const {
    Q_UNUSED(index);
    return plr_list.size();
  }
  int columnCount(const QModelIndex &parent = QModelIndex()) const {
    Q_UNUSED(parent);
    return num_player_dlg_columns;
  }
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  bool setData(const QModelIndex &index, const QVariant &value,
               int role = Qt::DisplayRole);
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role) const;
  QVariant hide_data(int section) const;
  void populate();
private slots:
  void notify_plr_changed(int row);
private:
  QList<plr_item *> plr_list;
};

/***************************************************************************
  Player widget to show player/nation model
***************************************************************************/
class plr_widget: public QTreeView
{
  Q_OBJECT
  plr_model *list_model;
  QSortFilterProxyModel *filter_model;
  plr_item_delegate *pid;
  plr_report *plr;
  QString techs_known;
  QString techs_unknown;
  struct player *selected_player;
public:
  plr_widget(plr_report *pr);
  ~plr_widget();
  void restore_selection();
  plr_model *get_model() const;
  QString intel_str;
  QString ally_str;
  QString tech_str;
  struct player *other_player;
public slots:
  void display_header_menu(const QPoint &);
  void nation_selected(const QItemSelection &sl, const QItemSelection &ds);
private:
  void mousePressEvent(QMouseEvent *event);
  void hide_columns();
};

/***************************************************************************
  Widget to show as tab widget in players view.
***************************************************************************/
class plr_report: public QWidget
{
  Q_OBJECT
  plr_widget *plr_wdg;
  QLabel *plr_label;
  QLabel *ally_label;
  QLabel *tech_label;
  QSplitter *v_splitter;
  QSplitter *h_splitter;
  QPushButton *meet_but;
  QPushButton *cancel_but;
  QPushButton *withdraw_but;
  QPushButton *toggle_ai_but;
  QVBoxLayout *layout;
  QHBoxLayout *hlayout;
public:
  plr_report();
  ~plr_report();
  void update_report(bool update_selection = true);
  void init();
  void call_meeting();
private:
  struct player *other_player;
  int index;
private slots:
  void req_meeeting();
  void req_caancel_threaty(); /** somehow autoconnect feature messes
                               *  here and names are bit odd to cheat 
                               *  autoconnect */
  void req_wiithdrw_vision();
  void toggle_ai_mode();
};

void popup_players_dialog(bool raise);
void popdown_players_report(void);


#endif /* FC__PLRDLG_H */
