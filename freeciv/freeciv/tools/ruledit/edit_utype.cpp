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
#include <QMenu>
#include <QToolButton>

// common
#include "unittype.h"

// ruledit
#include "ruledit.h"
#include "ruledit_qt.h"
#include "tab_tech.h"

#include "edit_utype.h"

/**********************************************************************//**
  Setup edit_utype object
**************************************************************************/
edit_utype::edit_utype(ruledit_gui *ui_in, struct unit_type *utype_in) : QDialog()
{
  QVBoxLayout *main_layout = new QVBoxLayout(this);
  QGridLayout *unit_layout = new QGridLayout();
  QLabel *label;
  QMenu *req;

  ui = ui_in;
  utype = utype_in;

  setWindowTitle(QString::fromUtf8(utype_rule_name(utype)));

  label = new QLabel(QString::fromUtf8(R__("Requirement")));
  label->setParent(this);

  req = new QMenu();
  req_button = new QToolButton();
  req_button->setParent(this);
  req_button->setMenu(req);
  tab_tech::techs_to_menu(req);
  connect(req_button, SIGNAL(triggered(QAction *)), this, SLOT(req_menu(QAction *)));

  unit_layout->addWidget(label, 0, 0);
  unit_layout->addWidget(req_button, 0, 1);

  refresh();

  main_layout->addLayout(unit_layout);

  setLayout(main_layout);
}

/**********************************************************************//**
  Refresh the information.
**************************************************************************/
void edit_utype::refresh()
{
  req_button->setText(tab_tech::tech_name(utype->require_advance));
}

/**********************************************************************//**
  User selected tech to be req of utype
**************************************************************************/
void edit_utype::req_menu(QAction *action)
{
  struct advance *padv = advance_by_rule_name(action->text().toUtf8().data());

  if (padv != nullptr) {
    utype->require_advance = padv;

    refresh();
  }
}
