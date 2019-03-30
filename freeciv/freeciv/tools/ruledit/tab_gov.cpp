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
#include "government.h"

// ruledit
#include "ruledit.h"
#include "ruledit_qt.h"
#include "validity.h"

#include "tab_gov.h"

/**********************************************************************//**
  Setup tab_gov object
**************************************************************************/
tab_gov::tab_gov(ruledit_gui *ui_in) : QWidget()
{
  QVBoxLayout *main_layout = new QVBoxLayout(this);
  QGridLayout *gov_layout = new QGridLayout();
  QLabel *label;
  QPushButton *effects_button;
  QPushButton *add_button;
  QPushButton *delete_button;
  QPushButton *reqs_button;

  ui = ui_in;
  selected = 0;

  gov_list = new QListWidget(this);

  connect(gov_list, SIGNAL(itemSelectionChanged()), this, SLOT(select_gov()));
  main_layout->addWidget(gov_list);

  gov_layout->setSizeConstraint(QLayout::SetMaximumSize);

  label = new QLabel(QString::fromUtf8(R__("Rule Name")));
  label->setParent(this);
  rname = new QLineEdit(this);
  rname->setText("None");
  connect(rname, SIGNAL(returnPressed()), this, SLOT(name_given()));
  gov_layout->addWidget(label, 0, 0);
  gov_layout->addWidget(rname, 0, 2);

  label = new QLabel(QString::fromUtf8(R__("Name")));
  label->setParent(this);
  same_name = new QRadioButton();
  connect(same_name, SIGNAL(toggled(bool)), this, SLOT(same_name_toggle(bool)));
  name = new QLineEdit(this);
  name->setText("None");
  connect(name, SIGNAL(returnPressed()), this, SLOT(name_given()));
  gov_layout->addWidget(label, 1, 0);
  gov_layout->addWidget(same_name, 1, 1);
  gov_layout->addWidget(name, 1, 2);

  reqs_button = new QPushButton(QString::fromUtf8(R__("Requirements")), this);
  connect(reqs_button, SIGNAL(pressed()), this, SLOT(edit_reqs()));
  gov_layout->addWidget(reqs_button, 2, 2);

  effects_button = new QPushButton(QString::fromUtf8(R__("Effects")), this);
  connect(effects_button, SIGNAL(pressed()), this, SLOT(edit_effects()));
  gov_layout->addWidget(effects_button, 3, 2);

  add_button = new QPushButton(QString::fromUtf8(R__("Add Government")), this);
  connect(add_button, SIGNAL(pressed()), this, SLOT(add_now()));
  gov_layout->addWidget(add_button, 4, 0);
  show_experimental(add_button);

  delete_button = new QPushButton(QString::fromUtf8(R__("Remove this Government")), this);
  connect(delete_button, SIGNAL(pressed()), this, SLOT(delete_now()));
  gov_layout->addWidget(delete_button, 4, 2);
  show_experimental(delete_button);

  refresh();

  main_layout->addLayout(gov_layout);

  setLayout(main_layout);
}

/**********************************************************************//**
  Refresh the information.
**************************************************************************/
void tab_gov::refresh()
{
  gov_list->clear();

  governments_iterate(pgov) {
    if (!pgov->ruledit_disabled) {
      QListWidgetItem *item =
        new QListWidgetItem(QString::fromUtf8(government_rule_name(pgov)));

      gov_list->insertItem(government_index(pgov), item);
    }
  } governments_iterate_end;
}

/**********************************************************************//**
  Update info of the government
**************************************************************************/
void tab_gov::update_gov_info(struct government *pgov)
{
  selected = pgov;

  if (selected != 0) {
    QString dispn = QString::fromUtf8(untranslated_name(&(pgov->name)));
    QString rulen = QString::fromUtf8(government_rule_name(pgov));

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
  User selected government from the list.
**************************************************************************/
void tab_gov::select_gov()
{
  QList<QListWidgetItem *> select_list = gov_list->selectedItems();

  if (!select_list.isEmpty()) {
    update_gov_info(government_by_rule_name(select_list.at(0)->text().toUtf8().data()));
  }
}

/**********************************************************************//**
  User entered name for the government
**************************************************************************/
void tab_gov::name_given()
{
  if (selected != nullptr) {
    governments_iterate(pgov) {
      if (pgov != selected && !pgov->ruledit_disabled) {
        if (!strcmp(government_rule_name(pgov), rname->text().toUtf8().data())) {
          ui->display_msg(R__("A government with that rule name already "
                              "exists!"));
          return;
        }
      }
    } governments_iterate_end;

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
  User requested government deletion 
**************************************************************************/
void tab_gov::delete_now()
{
  if (selected != 0) {
    requirers_dlg *requirers;

    requirers = ui->create_requirers(government_rule_name(selected));
    if (is_government_needed(selected, &ruledit_qt_display_requirers, requirers)) {
      return;
    }

    selected->ruledit_disabled = true;

    refresh();
    update_gov_info(nullptr);
  }
}

/**********************************************************************//**
  Initialize new government for use.
**************************************************************************/
bool tab_gov::initialize_new_gov(struct government *pgov)
{
  if (government_by_rule_name("New Government") != nullptr) {
    return false;
  }

  name_set(&(pgov->name), 0, "New Government");

  return true;
}

/**********************************************************************//**
  User requested new government
**************************************************************************/
void tab_gov::add_now()
{
  struct government *new_gov;

  // Try to reuse freed government slot
  governments_iterate(pgov) {
    if (pgov->ruledit_disabled) {
      if (initialize_new_gov(pgov)) {
        pgov->ruledit_disabled = false;
        update_gov_info(pgov);
        refresh();
      }
      return;
    }
  } governments_iterate_end;

  // Try to add completely new government
  if (game.control.num_goods_types >= MAX_GOODS_TYPES) {
    return;
  }

  // government_count must be big enough to hold new government or
  // government_by_number() fails.
  game.control.government_count++;
  new_gov = government_by_number(game.control.government_count - 1);
  if (initialize_new_gov(new_gov)) {
    update_gov_info(new_gov);

    refresh();
  } else {
    game.control.government_count--; // Restore
  }
}

/**********************************************************************//**
  Toggled whether rule_name and name should be kept identical
**************************************************************************/
void tab_gov::same_name_toggle(bool checked)
{
  name->setEnabled(!checked);
  if (checked) {
    name->setText(rname->text());
  }
}

/**********************************************************************//**
  User wants to edit reqs
**************************************************************************/
void tab_gov::edit_reqs()
{
  if (selected != nullptr) {
    ui->open_req_edit(QString::fromUtf8(government_rule_name(selected)),
                      &selected->reqs);
  }
}

/**********************************************************************//**
  User wants to edit effects
**************************************************************************/
void tab_gov::edit_effects()
{
  if (selected != nullptr) {
    struct universal uni;

    uni.value.govern = selected;
    uni.kind = VUT_GOVERNMENT;

    ui->open_effect_edit(QString::fromUtf8(government_rule_name(selected)),
                         &uni, EFMC_NORMAL);
  }
}
