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
#include <QGridLayout>
#include <QLineEdit>
#include <QPushButton>

// utility
#include "fcintl.h"
#include "log.h"
#include "registry.h"

// common
#include "game.h"

// ruledit
#include "ruledit_qt.h"

#include "tab_nation.h"

/**********************************************************************//**
  Setup tab_nation object
**************************************************************************/
tab_nation::tab_nation(ruledit_gui *ui_in) : QWidget()
{
  QGridLayout *main_layout = new QGridLayout(this);
  QLabel *nationlist_label;
  int row = 0;

  ui = ui_in;

  main_layout->setSizeConstraint(QLayout::SetMaximumSize);

  via_include = new QRadioButton(QString::fromUtf8(R__("Use nationlist")));
  main_layout->addWidget(via_include, row++, 0);
  connect(via_include, SIGNAL(toggled(bool)), this, SLOT(nationlist_toggle(bool)));

  nationlist_label = new QLabel(QString::fromUtf8(R__("Nationlist")));
  nationlist_label->setParent(this);
  main_layout->addWidget(nationlist_label, row, 0);
  nationlist = new QLineEdit(this);
  main_layout->addWidget(nationlist, row++, 1);

  refresh();

  setLayout(main_layout);
}

/**********************************************************************//**
  Refresh the information.
**************************************************************************/
void tab_nation::refresh()
{
  if (ui->data.nationlist == NULL) {
    via_include->setChecked(false);
    nationlist->setEnabled(false);
  } else {
    via_include->setChecked(true);
    nationlist->setText(ui->data.nationlist);
    nationlist->setEnabled(true);
  }
}

/**********************************************************************//**
  Flush information from widgets to stores where it can be saved from.
**************************************************************************/
void tab_nation::flush_widgets()
{
  FC_FREE(ui->data.nationlist);

  if (via_include->isChecked()) {
    ui->data.nationlist = fc_strdup(nationlist->text().toUtf8().data());
  } else {
    ui->data.nationlist = NULL;
  }
}

/**********************************************************************//**
  Toggled nationlist include setting
**************************************************************************/
void tab_nation::nationlist_toggle(bool checked)
{
  if (checked) {
    if (ui->data.nationlist_saved != NULL) {
      ui->data.nationlist = ui->data.nationlist_saved;
    } else {
      ui->data.nationlist = fc_strdup("default/nationlist.ruleset");
    }
  } else {
    FC_FREE(ui->data.nationlist_saved);
    ui->data.nationlist_saved = fc_strdup(nationlist->text().toUtf8().data());
    ui->data.nationlist = NULL;
  }

  refresh();
}
