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
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QToolButton>

// utility
#include "fcintl.h"
#include "log.h"

// common
#include "game.h"
#include "government.h"

// ruledit
#include "ruledit.h"
#include "ruledit_qt.h"
#include "validity.h"

#include "tab_enablers.h"

/**********************************************************************//**
  Setup tab_enabler object
**************************************************************************/
tab_enabler::tab_enabler(ruledit_gui *ui_in) : QWidget()
{
  QVBoxLayout *main_layout = new QVBoxLayout(this);
  QGridLayout *enabler_layout = new QGridLayout();
  QLabel *label;
  QPushButton *add_button;
  QPushButton *delete_button;
  QPushButton *reqs_button;

  ui = ui_in;
  selected = 0;

  enabler_list = new QListWidget(this);

  connect(enabler_list, SIGNAL(itemSelectionChanged()), this, SLOT(select_enabler()));
  main_layout->addWidget(enabler_list);

  enabler_layout->setSizeConstraint(QLayout::SetMaximumSize);

  label = new QLabel(QString::fromUtf8(R__("Type")));
  label->setParent(this);
  enabler_layout->addWidget(label, 0, 0);

  type_button = new QToolButton();
  type_menu = new QMenu();

  action_iterate(act) {
    type_menu->addAction(action_id_rule_name(act));
  } action_iterate_end;

  connect(type_menu, SIGNAL(triggered(QAction *)),
          this, SLOT(edit_type(QAction *)));

  type_button->setToolButtonStyle(Qt::ToolButtonTextOnly);
  type_button->setPopupMode(QToolButton::MenuButtonPopup);

  type_button->setMenu(type_menu);

  enabler_layout->addWidget(type_button, 0, 2);

  reqs_button = new QPushButton(QString::fromUtf8(R__("Actor Requirements")), this);
  connect(reqs_button, SIGNAL(pressed()), this, SLOT(edit_actor_reqs()));
  enabler_layout->addWidget(reqs_button, 1, 2);

  reqs_button = new QPushButton(QString::fromUtf8(R__("Target Requirements")), this);
  connect(reqs_button, SIGNAL(pressed()), this, SLOT(edit_target_reqs()));
  enabler_layout->addWidget(reqs_button, 2, 2);

  add_button = new QPushButton(QString::fromUtf8(R__("Add Enabler")), this);
  connect(add_button, SIGNAL(pressed()), this, SLOT(add_now()));
  enabler_layout->addWidget(add_button, 3, 0);
  show_experimental(add_button);

  delete_button = new QPushButton(QString::fromUtf8(R__("Remove this Enabler")), this);
  connect(delete_button, SIGNAL(pressed()), this, SLOT(delete_now()));
  enabler_layout->addWidget(delete_button, 3, 2);
  show_experimental(delete_button);

  refresh();

  main_layout->addLayout(enabler_layout);

  setLayout(main_layout);
}

/**********************************************************************//**
  Refresh the information.
**************************************************************************/
void tab_enabler::refresh()
{
  int n = 0;

  enabler_list->clear();

  action_enablers_iterate(enabler) {
    if (!enabler->disabled) {
      char buffer[512];
      QListWidgetItem *item;

      fc_snprintf(buffer, sizeof(buffer), "#%d: %s", n,
                  action_rule_name(enabler_get_action(enabler)));

      item = new QListWidgetItem(QString::fromUtf8(buffer));

      enabler_list->insertItem(n++, item);
    }
  } action_enablers_iterate_end;
}

/**********************************************************************//**
  Update info of the enabler
**************************************************************************/
void tab_enabler::update_enabler_info(struct action_enabler *enabler)
{
  selected = enabler;

  if (selected != nullptr) {
    QString dispn = QString::fromUtf8(action_rule_name(enabler_get_action(enabler)));

    type_button->setText(dispn);
  } else {
    type_button->setText("None");
  }
}

/**********************************************************************//**
  User selected enabler from the list.
**************************************************************************/
void tab_enabler::select_enabler()
{
  int i = 0;

  action_enablers_iterate(enabler) {
    QListWidgetItem *item = enabler_list->item(i++);

    if (item != nullptr && item->isSelected()) {
      update_enabler_info(enabler);
    }
  } action_enablers_iterate_end;
}

/**********************************************************************//**
  User requested enabler deletion 
**************************************************************************/
void tab_enabler::delete_now()
{
  if (selected != nullptr) {
    selected->disabled = true;

    refresh();
    update_enabler_info(nullptr);
  }
}

/**********************************************************************//**
  Initialize new enabler for use.
**************************************************************************/
bool tab_enabler::initialize_new_enabler(struct action_enabler *enabler)
{
  return true;
}

/**********************************************************************//**
  User requested new enabler
**************************************************************************/
void tab_enabler::add_now()
{
  struct action_enabler *new_enabler;

  // Try to reuse freed enabler slot
  action_enablers_iterate(enabler) {
    if (enabler->disabled) {
      if (initialize_new_enabler(enabler)) {
        enabler->disabled = false;
        update_enabler_info(enabler);
        refresh();
      }
      return;
    }
  } action_enablers_iterate_end;

  // Try to add completely new enabler
  new_enabler = action_enabler_new();

  fc_assert_ret(NUM_ACTIONS > 0);
  fc_assert_ret(action_id_exists(NUM_ACTIONS - 1));
  new_enabler->action = (NUM_ACTIONS - 1);

  action_enabler_add(new_enabler);

  update_enabler_info(new_enabler);
  refresh();
}

/**********************************************************************//**
  User selected action to enable
**************************************************************************/
void tab_enabler::edit_type(QAction *action)
{
  struct action *paction;

  paction = action_by_rule_name(action->text().toUtf8().data());

  if (selected != nullptr && paction != nullptr) {
    /* Store the old action so it can be changed back. */
    const action_id old_action = selected->action;

    /* Handle the new action's hard obligatory requirements. */
    selected->action = paction->id;
    if (action_enabler_obligatory_reqs_missing(selected)) {
      /* At least one hard obligatory requirement is missing. */

      QMessageBox *box = new QMessageBox();

      box->setWindowTitle(_("Obligatory hard requirements"));

      box->setText(
            QString(_("Changing action to %1 will modify enabler "
                      "requirements. Continue?"))
                   .arg(action_name_translation(paction)));
      box->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
      box->exec();

      if (box->result() == QMessageBox::Yes) {
        /* Add obligatory requirements. */
        action_enabler_obligatory_reqs_add(selected);
      } else {
        /* The user don't want to add the requirements. Cancel. */
        selected->action = old_action;
        return;
      }
    }

    /* Must remove and add back because enablers are stored by action. */
    selected->action = old_action;
    action_enabler_remove(selected);
    selected->action = paction->id;
    action_enabler_add(selected);

    /* Show the changes. */
    update_enabler_info(selected);
    refresh();
  }
}

/**********************************************************************//**
  User wants to edit target reqs
**************************************************************************/
void tab_enabler::edit_target_reqs()
{
  if (selected != nullptr) {
    ui->open_req_edit(QString::fromUtf8(R__("Enabler (target)")),
                      &selected->target_reqs);
  }
}

/**********************************************************************//**
  User wants to edit actor reqs
**************************************************************************/
void tab_enabler::edit_actor_reqs()
{
  if (selected != nullptr) {
    ui->open_req_edit(QString::fromUtf8(R__("Enabler (actor)")),
                      &selected->actor_reqs);
  }
}
