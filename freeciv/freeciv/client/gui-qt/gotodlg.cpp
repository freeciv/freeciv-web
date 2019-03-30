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
#include <fc_config.h>
#endif

// Qt
#include <QApplication>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QTableWidget>

/* common */
#include "game.h"

/* client */
#include "client_main.h"
#include "control.h"
#include "goto.h"
#include "text.h"

// gui-qt
#include "fc_client.h"
#include "gotodlg.h"
#include "qtg_cxxside.h"
#include "sprite.h"

/***********************************************************************//**
  Constructor for goto_dialog
***************************************************************************/
goto_dialog::goto_dialog(QWidget *parent)
{
  QStringList headers_lst;
  QHBoxLayout *hb;

  setParent(parent);
  headers_lst << QString(_("City")) << QString(_("Nation"))
              << QString(_("Airlift"));
  goto_tab = new QTableWidget;
  goto_city = new QPushButton(_("&Goto"));
  airlift_city = new QPushButton(_("&Airlift"));
  close_but = new QPushButton(_("&Close"));
  layout = new QGridLayout;

  show_all = new QCheckBox;
  show_all->setChecked(false);
  show_all_label = new QLabel(_("Show All Cities"));
  show_all_label->setAlignment(Qt::AlignLeft);
  hb = new QHBoxLayout;
  hb->addWidget(show_all);
  hb->addWidget(show_all_label, Qt::AlignLeft);

  goto_tab->setProperty("showGrid", "false");
  goto_tab->setSelectionBehavior(QAbstractItemView::SelectRows);
  goto_tab->setEditTriggers(QAbstractItemView::NoEditTriggers);
  goto_tab->verticalHeader()->setVisible(false);
  goto_tab->horizontalHeader()->setVisible(true);
  goto_tab->setSelectionMode(QAbstractItemView::SingleSelection);
  goto_tab->setColumnCount(3);
  goto_tab->setHorizontalHeaderLabels(headers_lst);
  goto_tab->setSortingEnabled(true);
  goto_tab->horizontalHeader()->setSectionResizeMode(
                                             QHeaderView::ResizeToContents);

  layout->addWidget(goto_tab, 0, 0, 4, 4);
  layout->addItem(hb, 4, 0, 1, 2);
  layout->addWidget(goto_city, 5, 0, 1, 1);
  layout->addWidget(airlift_city, 5, 1, 1, 1);
  layout->addWidget(close_but, 5, 3, 1, 1);

  setFixedWidth(goto_tab->horizontalHeader()->width());
  connect(close_but, &QAbstractButton::clicked, this, &goto_dialog::close_dlg);
  connect(goto_city, &QAbstractButton::clicked, this, &goto_dialog::go_to_city);
  connect(airlift_city, &QAbstractButton::clicked, this, &goto_dialog::airlift_to);
  connect(show_all, &QCheckBox::stateChanged,
          this, &goto_dialog::checkbox_changed);
  connect(goto_tab->selectionModel(),
          SIGNAL(selectionChanged(const QItemSelection &,
                                  const QItemSelection &)),
          SLOT(item_selected(const QItemSelection &,
                             const QItemSelection &)));

  setLayout(layout);
  original_tile = NULL;
  setFocus();
}

/***********************************************************************//**
  Sets variables which must be destroyed later
***************************************************************************/
void goto_dialog::init()
{
  if (original_tile) {
    tile_virtual_destroy(original_tile);
  }
  original_tile = tile_virtual_new(get_center_tile_mapcanvas());
}

/***********************************************************************//**
  Destructor for goto dialog
***************************************************************************/
goto_dialog::~goto_dialog()
{
  if (original_tile) {
    tile_virtual_destroy(original_tile);
  }
}

/***********************************************************************//**
  Slot for checkbox 'all nations'
***************************************************************************/
void goto_dialog::checkbox_changed(int state)
{
  update_dlg();
}

/***********************************************************************//**
  User has chosen some city on table
***************************************************************************/
void goto_dialog::item_selected(const QItemSelection &sl,
                                const QItemSelection &ds)
{
  int i;
  int city_id;
  QModelIndex index;
  QModelIndexList indexes = sl.indexes();
  QTableWidgetItem *item;
  struct city *dest;
  bool can_airlift;

  if (indexes.isEmpty()) {
    return;
  }
  index = indexes.at(0);
  i = index.row();
  item = goto_tab->item(i, 0);
  city_id = item->data(Qt::UserRole).toInt();
  dest = game_city_by_number(city_id);
  center_tile_mapcanvas(city_tile(dest));
  can_airlift = false;
  unit_list_iterate(get_units_in_focus(), punit) {
    if (unit_can_airlift_to(punit, dest)) {
      can_airlift = true;
      break;
    }
  } unit_list_iterate_end;

  if (can_airlift) {
    airlift_city->setEnabled(true);
  } else {
    airlift_city->setDisabled(true);
  }

}

/***********************************************************************//**
  Sorts dialog by default column (0)
***************************************************************************/
void goto_dialog::sort_def()
{
  goto_tab->sortByColumn(0, Qt::AscendingOrder);
}

/***********************************************************************//**
  Shows and moves widget
***************************************************************************/
void goto_dialog::show_me()
{
  QPoint p, final_p;
  p = QCursor::pos();
  p = parentWidget()->mapFromGlobal(p);
  final_p.setX(p.x());
  final_p.setY(p.y());
  if (p.x() + width() > parentWidget()->width()) {
    final_p.setX(parentWidget()->width() - width());
  }
  if (p.y() - height() < 0) {
    final_p.setY(height());
  }
  move(final_p.x(), final_p.y() - height());
  show();
}

/***********************************************************************//**
  Updates table in widget
***************************************************************************/
void goto_dialog::update_dlg()
{
  goto_tab->clearContents();
  goto_tab->setRowCount(0);
  goto_tab->setSortingEnabled(false);
  if (show_all->isChecked()) {
    players_iterate(pplayer) {
      fill_tab(pplayer);
    } players_iterate_end;
  } else {
    fill_tab(client_player());
  }
  goto_tab->setSortingEnabled(true);
  goto_tab->horizontalHeader()->setStretchLastSection(false);
  goto_tab->resizeRowsToContents();
  goto_tab->horizontalHeader()->setStretchLastSection(true);
}

/***********************************************************************//**
  Helper for function for filling table
***************************************************************************/
void goto_dialog::fill_tab(player *pplayer)
{
  int i;

  QString str;
  const char *at;
  QFont f = QApplication::font();
  QFontMetrics fm(f);
  int h;
  struct sprite *sprite;
  QPixmap *pix;
  QPixmap pix_scaled;
  QTableWidgetItem *item;


  h = fm.height() + 6;
  i = goto_tab->rowCount();
  city_list_iterate(pplayer->cities, pcity) {
    goto_tab->insertRow(i);
    for (int j = 0; j < 3; j++) {
      item = new QTableWidgetItem;
      switch (j) {
      case 0:
        str = city_name_get(pcity);
        break;
      case 1:
        sprite = get_nation_flag_sprite(tileset, nation_of_player(pplayer));
        if (sprite != NULL) {
          pix = sprite->pm;
          pix_scaled = pix->scaledToHeight(h);
          item->setData(Qt::DecorationRole, pix_scaled);
        }
        str = nation_adjective_translation(nation_of_player(pplayer));
        break;
      case 2:
        at = get_airlift_text(get_units_in_focus(), pcity);
        if (at == NULL) {
          str = "-";
        } else {
          str = at;
        }
        item->setTextAlignment(Qt::AlignHCenter);
        break;
      }
      item->setText(str);
      item->setData(Qt::UserRole, pcity->id);
      goto_tab->setItem(i, j, item);
    }
    i++;
  } city_list_iterate_end;
}

/***********************************************************************//**
  Slot for airlifting unit
***************************************************************************/
void goto_dialog::airlift_to()
{
  struct city *pdest;

  if (goto_tab->currentRow() == -1) {
    return;
  }
  pdest = game_city_by_number(goto_tab->item(goto_tab->currentRow(),
                                             0)->data(Qt::UserRole).toInt());
  if (pdest) {
    unit_list_iterate(get_units_in_focus(), punit) {
      if (unit_can_airlift_to(punit, pdest)) {
        request_unit_airlift(punit, pdest);
      }
    } unit_list_iterate_end;
  }
}

/***********************************************************************//**
  Slot for goto for city
***************************************************************************/
void goto_dialog::go_to_city()
{
  struct city *pdest;

  if (goto_tab->currentRow() == -1) {
    return;
  }

  pdest = game_city_by_number(goto_tab->item(goto_tab->currentRow(),
                                             0)->data(Qt::UserRole).toInt());
  if (pdest) {
    unit_list_iterate(get_units_in_focus(), punit) {
      send_goto_tile(punit, pdest->tile);
    } unit_list_iterate_end;
  }
}

/***********************************************************************//**
  Slot for hiding dialog
***************************************************************************/
void goto_dialog::close_dlg()
{
  center_tile_mapcanvas(original_tile);
  hide();
}

/***********************************************************************//**
  Paints rectangles for goto_dialog
***************************************************************************/
void goto_dialog::paint(QPainter *painter, QPaintEvent *event)
{
  painter->setBrush(QColor(0, 0, 30, 85));
  painter->drawRect(0, 0, width(), height());
  painter->setBrush(QColor(0, 0, 0, 85));
  painter->drawRect(5, 5, width() - 10, height() - 10);
}

/***********************************************************************//**
  Paint event for goto_dialog
***************************************************************************/
void goto_dialog::paintEvent(QPaintEvent *event)
{
  QPainter painter;

  painter.begin(this);
  paint(&painter, event);
  painter.end();
}

/***********************************************************************//*
  Popup a dialog to have the focus unit goto to a city.
**************************************************************************/
void popup_goto_dialog(void)
{
  if (C_S_RUNNING != client_state()) {
    return;
  }
  if (get_num_units_in_focus() == 0) {
    return;
  }
  if (!client_has_player()) {
    return;
  }

  if (gui()->gtd != NULL) {
    gui()->gtd->init();
    gui()->gtd->update_dlg();
    gui()->gtd->sort_def();
    gui()->gtd->show_me();
  }
}
