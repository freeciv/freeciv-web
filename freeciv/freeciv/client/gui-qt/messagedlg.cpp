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
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>

// client
#include "options.h"

// gui-qt
#include "fc_client.h"
#include "messagedlg.h"

extern QApplication *qapp;
/**********************************************************************//**
  Message widget constructor
**************************************************************************/
message_dlg::message_dlg()
{
  int index;
  QStringList slist;
  QLabel *empty1, *empty2;
  QPushButton *but1;
  QPushButton *but2;
  QMargins margins;
  int len;

  setAttribute(Qt::WA_DeleteOnClose);
  empty1 = new QLabel;
  empty2 = new QLabel;
  layout = new QGridLayout;
  msgtab = new QTableWidget;
  slist << _("Event") << _("Out") << _("Mes") << _("Pop");
  msgtab->setColumnCount(slist.count());
  msgtab->setHorizontalHeaderLabels(slist);
  msgtab->setProperty("showGrid", "false");
  msgtab->setEditTriggers(QAbstractItemView::NoEditTriggers);
  msgtab->horizontalHeader()->resizeSections(QHeaderView::
                                             ResizeToContents);
  msgtab->verticalHeader()->setVisible(false);
  msgtab->setSelectionMode(QAbstractItemView::NoSelection);
  msgtab->setSelectionBehavior(QAbstractItemView::SelectColumns);

  but1 = new QPushButton(style()->standardIcon(
                         QStyle::SP_DialogCancelButton), _("Cancel"));
  connect(but1, &QAbstractButton::clicked, this, &message_dlg::cancel_changes);
  layout->addWidget(but1, 1, 1, 1, 1);
  but2 = new QPushButton(style()->standardIcon(QStyle::SP_DialogOkButton),
                         _("Ok"));
  connect(but2, &QAbstractButton::clicked, this, &message_dlg::apply_changes);
  layout->addWidget(but2, 1, 2, 1, 1, Qt::AlignRight);
  layout->addWidget(empty1, 0, 0, 1, 1);
  layout->addWidget(msgtab, 0, 1, 1, 2);
  layout->addWidget(empty2, 0, 3, 1, 1);
  layout->setColumnStretch(0, 1);
  layout->setColumnStretch(1, 10);
  layout->setColumnStretch(3, 1);
  setLayout(layout);
  gui()->gimme_place(this, "MSD");
  index = gui()->add_game_tab(this);
  gui()->game_tab_widget->setCurrentIndex(index);

  fill_data();
  margins = msgtab->contentsMargins();
  len = msgtab->horizontalHeader()->length() + margins.left()
        + margins.right()
        + qapp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
  msgtab->setFixedWidth(len);
  msgtab->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  but1->setFixedWidth(len / 3);
  but2->setFixedWidth(len / 3);
}

/**********************************************************************//**
  Message widget destructor
**************************************************************************/
message_dlg::~message_dlg()
{
  gui()->remove_repo_dlg("MSD");
}

/**********************************************************************//**
  Fills column in table
**************************************************************************/
void message_dlg::fill_data()
{
  int i, j;
  QTableWidgetItem *item;
  i = 0;
  msgtab->setRowCount(0);

  sorted_event_iterate(ev) {
    item =  new QTableWidgetItem;
    item->setText(get_event_message_text(ev));
    msgtab->insertRow(i);
    msgtab->setItem(i, 0, item);
    for (j = 0; j < NUM_MW; j++) {
      bool checked;
      item =  new QTableWidgetItem;
      checked =  messages_where[ev] & (1 << j);
      if (checked) {
        item->setCheckState(Qt::Checked);
      } else {
        item->setCheckState(Qt::Unchecked);
      }
      msgtab->setItem(i, j + 1, item);
    }
    i++;
  } sorted_event_iterate_end;
  msgtab->resizeColumnsToContents();

}

/**********************************************************************//**
  Apply changes and closes widget
**************************************************************************/
void message_dlg::apply_changes()
{
  int i, j;
  QTableWidgetItem *item;
  Qt::CheckState state;
  for (i = 0; i <= event_type_max(); i++) {
    /* Include possible undefined messages. */
    messages_where[i] = 0;
  }
  i = 0;
  sorted_event_iterate(ev) {
    for (j = 0; j < NUM_MW; j++) {
      bool checked;
      item = msgtab->item(i, j + 1);
      checked =  messages_where[ev] & (1 << j);
      state = item->checkState();
      if ((state == Qt::Checked && !checked)
          || (state == Qt::Unchecked && checked)) {
        messages_where[ev] |= (1 << j);
      }
    }
    i++;
  } sorted_event_iterate_end;
  close();
}

/**********************************************************************//**
  Closes widget
**************************************************************************/
void message_dlg::cancel_changes()
{
  close();
}

/**********************************************************************//**
  Popup a window to let the user edit their message options.
**************************************************************************/
void popup_messageopt_dialog(void)
{
  message_dlg *mdlg;
  int i;
  QWidget *w;

  if (!gui()->is_repo_dlg_open("MSD")) {
    mdlg = new message_dlg;
  } else {
    i = gui()->gimme_index_of("MSD");
    fc_assert(i != -1);
    if (gui()->game_tab_widget->currentIndex() == i) {
      return;
    }
    w = gui()->game_tab_widget->widget(i);
    mdlg = reinterpret_cast<message_dlg *>(w);
    gui()->game_tab_widget->setCurrentWidget(mdlg);
  }
}
