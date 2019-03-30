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
#include <QPushButton>
#include <QRadioButton>
#include <QToolButton>

// utility
#include "fcintl.h"
#include "log.h"

// common
#include "game.h"
#include "multipliers.h"

// ruledit
#include "req_edit.h"
#include "ruledit.h"
#include "ruledit_qt.h"
#include "validity.h"

#include "tab_multiplier.h"

/**********************************************************************//**
  Setup tab_multiplier object
**************************************************************************/
tab_multiplier::tab_multiplier(ruledit_gui *ui_in) : QWidget()
{
  QVBoxLayout *main_layout = new QVBoxLayout(this);
  QGridLayout *mpr_layout = new QGridLayout();
  QLabel *label;
  QPushButton *add_button;
  QPushButton *delete_button;
  QPushButton *reqs_button;

  ui = ui_in;
  selected = 0;

  mpr_list = new QListWidget(this);

  connect(mpr_list, SIGNAL(itemSelectionChanged()), this, SLOT(select_multiplier()));
  main_layout->addWidget(mpr_list);

  mpr_layout->setSizeConstraint(QLayout::SetMaximumSize);

  label = new QLabel(QString::fromUtf8(R__("Rule Name")));
  label->setParent(this);
  rname = new QLineEdit(this);
  rname->setText("None");
  connect(rname, SIGNAL(returnPressed()), this, SLOT(name_given()));
  mpr_layout->addWidget(label, 0, 0);
  mpr_layout->addWidget(rname, 0, 2);

  label = new QLabel(QString::fromUtf8(R__("Name")));
  label->setParent(this);
  same_name = new QRadioButton();
  connect(same_name, SIGNAL(toggled(bool)), this, SLOT(same_name_toggle(bool)));
  name = new QLineEdit(this);
  name->setText("None");
  connect(name, SIGNAL(returnPressed()), this, SLOT(name_given()));
  mpr_layout->addWidget(label, 1, 0);
  mpr_layout->addWidget(same_name, 1, 1);
  mpr_layout->addWidget(name, 1, 2);

  reqs_button = new QPushButton(QString::fromUtf8(R__("Requirements")), this);
  connect(reqs_button, SIGNAL(pressed()), this, SLOT(edit_reqs()));
  mpr_layout->addWidget(reqs_button, 2, 2);

  add_button = new QPushButton(QString::fromUtf8(R__("Add Multiplier")), this);
  connect(add_button, SIGNAL(pressed()), this, SLOT(add_now()));
  mpr_layout->addWidget(add_button, 4, 0);
  show_experimental(add_button);

  delete_button = new QPushButton(QString::fromUtf8(R__("Remove this Multiplier")), this);
  connect(delete_button, SIGNAL(pressed()), this, SLOT(delete_now()));
  mpr_layout->addWidget(delete_button, 4, 2);
  show_experimental(delete_button);

  refresh();

  main_layout->addLayout(mpr_layout);

  setLayout(main_layout);
}

/**********************************************************************//**
  Refresh the information.
**************************************************************************/
void tab_multiplier::refresh()
{
  mpr_list->clear();

  multipliers_iterate(pmul) {
    if (!pmul->ruledit_disabled) {
      QListWidgetItem *item =
        new QListWidgetItem(QString::fromUtf8(multiplier_rule_name(pmul)));

      mpr_list->insertItem(multiplier_index(pmul), item);
    }
  } multipliers_iterate_end;
}

/**********************************************************************//**
  Update info of the multiplier
**************************************************************************/
void tab_multiplier::update_multiplier_info(struct multiplier *pmul)
{
  selected = pmul;

  if (selected != 0) {
    QString dispn = QString::fromUtf8(untranslated_name(&(pmul->name)));
    QString rulen = QString::fromUtf8(multiplier_rule_name(pmul));

    name->setText(dispn);
    rname->setText(rulen);
    if (dispn == rulen) {
      name->setEnabled(false);
      same_name->setChecked(true);
    } else {
      same_name->setChecked(false);
      name->setEnabled(true);
    }
  } else {
    name->setText("None");
    rname->setText("None");
    same_name->setChecked(true);
    name->setEnabled(false);
  }
}

/**********************************************************************//**
  User selected multiplier from the list.
**************************************************************************/
void tab_multiplier::select_multiplier()
{
  QList<QListWidgetItem *> select_list = mpr_list->selectedItems();

  if (!select_list.isEmpty()) {
    update_multiplier_info(multiplier_by_rule_name(select_list.at(0)->text().toUtf8().data()));
  }
}

/**********************************************************************//**
  User entered name for the multiplier
**************************************************************************/
void tab_multiplier::name_given()
{
  if (selected != nullptr) {
    multipliers_iterate(pmul) {
      if (pmul != selected && !pmul->ruledit_disabled) {
        if (!strcmp(multiplier_rule_name(pmul), rname->text().toUtf8().data())) {
          ui->display_msg(R__("A multiplier with that rule name already exists!"));
          return;
        }
      }
    } multipliers_iterate_end;

    if (same_name->isChecked()) {
      name->setText(rname->text());
    }

    names_set(&(selected->name), 0,
              name->text().toUtf8().data(),
              rname->text().toUtf8().data());
    refresh();
  }
}

/**********************************************************************//**
  User requested multiplier deletion
**************************************************************************/
void tab_multiplier::delete_now()
{
  if (selected != 0) {
    requirers_dlg *requirers;

    requirers = ui->create_requirers(multiplier_rule_name(selected));
    if (is_multiplier_needed(selected, &ruledit_qt_display_requirers, requirers)) {
      return;
    }

    selected->ruledit_disabled = true;

    refresh();
    update_multiplier_info(nullptr);
  }
}

/**********************************************************************//**
  Initialize new multiplier for use.
**************************************************************************/
bool tab_multiplier::initialize_new_multiplier(struct multiplier *pmul)
{
  if (multiplier_by_rule_name("New Multiplier") != nullptr) {
    return false;
  }

  name_set(&(pmul->name), 0, "New Multiplier");

  return true;
}

/**********************************************************************//**
  User requested new multiplier
**************************************************************************/
void tab_multiplier::add_now()
{
  struct multiplier *new_multiplier;

  // Try to reuse freed multiplier slot
  multipliers_iterate(pmul) {
    if (pmul->ruledit_disabled) {
      if (initialize_new_multiplier(pmul)) {
        pmul->ruledit_disabled = false;
        update_multiplier_info(pmul);
        refresh();
      }
      return;
    }
  } multipliers_iterate_end;

  // Try to add completely new multiplier
  if (game.control.num_multipliers >= MAX_NUM_MULTIPLIERS) {
    return;
  }

  // num_multipliers must be big enough to hold new multiplier or
  // multiplier_by_number() fails.
  game.control.num_multipliers++;
  new_multiplier = multiplier_by_number(game.control.num_multipliers - 1);
  if (initialize_new_multiplier(new_multiplier)) {
    update_multiplier_info(new_multiplier);

    refresh();
  } else {
    game.control.num_multipliers--; // Restore
  }
}

/**********************************************************************//**
  Toggled whether rule_name and name should be kept identical
**************************************************************************/
void tab_multiplier::same_name_toggle(bool checked)
{
  name->setEnabled(!checked);
  if (checked) {
    name->setText(rname->text());
  }
}

/**********************************************************************//**
  User wants to edit reqs
**************************************************************************/
void tab_multiplier::edit_reqs()
{
  if (selected != nullptr) {
    req_edit *redit = new req_edit(ui, QString::fromUtf8(multiplier_rule_name(selected)),
                                   &selected->reqs);

    redit->show();
  }
}
