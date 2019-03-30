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
#include "extras.h"

// ruledit
#include "ruledit.h"
#include "ruledit_qt.h"
#include "validity.h"

#include "tab_extras.h"

/**********************************************************************//**
  Setup tab_extras object
**************************************************************************/
tab_extras::tab_extras(ruledit_gui *ui_in) : QWidget()
{
  QVBoxLayout *main_layout = new QVBoxLayout(this);
  QGridLayout *extra_layout = new QGridLayout();
  QLabel *label;
  QPushButton *effects_button;
  QPushButton *add_button;
  QPushButton *delete_button;
  QPushButton *reqs_button;

  ui = ui_in;
  selected = 0;

  extra_list = new QListWidget(this);

  connect(extra_list, SIGNAL(itemSelectionChanged()), this, SLOT(select_extra()));
  main_layout->addWidget(extra_list);

  extra_layout->setSizeConstraint(QLayout::SetMaximumSize);

  label = new QLabel(QString::fromUtf8(R__("Rule Name")));
  label->setParent(this);
  rname = new QLineEdit(this);
  rname->setText("None");
  connect(rname, SIGNAL(returnPressed()), this, SLOT(name_given()));
  extra_layout->addWidget(label, 0, 0);
  extra_layout->addWidget(rname, 0, 2);

  label = new QLabel(QString::fromUtf8(R__("Name")));
  label->setParent(this);
  same_name = new QRadioButton();
  connect(same_name, SIGNAL(toggled(bool)), this, SLOT(same_name_toggle(bool)));
  name = new QLineEdit(this);
  name->setText("None");
  connect(name, SIGNAL(returnPressed()), this, SLOT(name_given()));
  extra_layout->addWidget(label, 1, 0);
  extra_layout->addWidget(same_name, 1, 1);
  extra_layout->addWidget(name, 1, 2);

  reqs_button = new QPushButton(QString::fromUtf8(R__("Requirements")), this);
  connect(reqs_button, SIGNAL(pressed()), this, SLOT(edit_reqs()));
  extra_layout->addWidget(reqs_button, 2, 2);

  effects_button = new QPushButton(QString::fromUtf8(R__("Effects")), this);
  connect(effects_button, SIGNAL(pressed()), this, SLOT(edit_effects()));
  extra_layout->addWidget(effects_button, 3, 2);

  add_button = new QPushButton(QString::fromUtf8(R__("Add Extra")), this);
  connect(add_button, SIGNAL(pressed()), this, SLOT(add_now()));
  extra_layout->addWidget(add_button, 4, 0);
  show_experimental(add_button);

  delete_button = new QPushButton(QString::fromUtf8(R__("Remove this Extra")), this);
  connect(delete_button, SIGNAL(pressed()), this, SLOT(delete_now()));
  extra_layout->addWidget(delete_button, 4, 2);
  show_experimental(delete_button);

  refresh();

  main_layout->addLayout(extra_layout);

  setLayout(main_layout);
}

/**********************************************************************//**
  Refresh the information.
**************************************************************************/
void tab_extras::refresh()
{
  extra_list->clear();

  extra_type_iterate(pextra) {
    if (!pextra->ruledit_disabled) {
      QListWidgetItem *item =
        new QListWidgetItem(QString::fromUtf8(extra_rule_name(pextra)));

      extra_list->insertItem(extra_index(pextra), item);
    }
  } extra_type_iterate_end;
}

/**********************************************************************//**
  Update info of the extra
**************************************************************************/
void tab_extras::update_extra_info(struct extra_type *pextra)
{
  selected = pextra;

  if (selected != nullptr) {
    QString dispn = QString::fromUtf8(untranslated_name(&(pextra->name)));
    QString rulen = QString::fromUtf8(extra_rule_name(pextra));

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
  User selected extra from the list.
**************************************************************************/
void tab_extras::select_extra()
{
  QList<QListWidgetItem *> select_list = extra_list->selectedItems();

  if (!select_list.isEmpty()) {
    update_extra_info(extra_type_by_rule_name(select_list.at(0)->text().toUtf8().data()));
  }
}

/**********************************************************************//**
  User entered name for the extra
**************************************************************************/
void tab_extras::name_given()
{
  if (selected != nullptr) {
    extra_type_iterate(pextra) {
      if (pextra != selected && !pextra->ruledit_disabled) {
        if (!strcmp(extra_rule_name(pextra), rname->text().toUtf8().data())) {
          ui->display_msg(R__("An extra with that rule name already exists!"));
          return;
        }
      }
    } extra_type_iterate_end;

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
  User requested extra deletion 
**************************************************************************/
void tab_extras::delete_now()
{
  if (selected != nullptr) {
    requirers_dlg *requirers;

    requirers = ui->create_requirers(extra_rule_name(selected));
    if (is_extra_needed(selected, &ruledit_qt_display_requirers, requirers)) {
      return;
    }

    selected->ruledit_disabled = true;

    refresh();
    update_extra_info(nullptr);
  }
}

/**********************************************************************//**
  Initialize new extra for use.
**************************************************************************/
bool tab_extras::initialize_new_extra(struct extra_type *pextra)
{
  if (extra_type_by_rule_name("New Extra") != nullptr) {
    return false;
  }

  name_set(&(pextra->name), 0, "New Extra");

  return true;
}

/**********************************************************************//**
  User requested new extra
**************************************************************************/
void tab_extras::add_now()
{
  struct extra_type *new_extra;

  // Try to reuse freed extra slot
  extra_type_iterate(pextra) {
    if (pextra->ruledit_disabled) {
      if (initialize_new_extra(pextra)) {
        pextra->ruledit_disabled = false;
        update_extra_info(pextra);
        refresh();
      }
      return;
    }
  } extra_type_iterate_end;

  // Try to add completely new extra
  if (game.control.num_extra_types >= MAX_EXTRA_TYPES) {
    return;
  }

  // num_extra_types must be big enough to hold new extra or
  // extra_by_number() fails.
  game.control.num_extra_types++;
  new_extra = extra_by_number(game.control.num_extra_types - 1);
  if (initialize_new_extra(new_extra)) {
    update_extra_info(new_extra);

    refresh();
  } else {
    game.control.num_extra_types--; // Restore
  }
}

/**********************************************************************//**
  Toggled whether rule_name and name should be kept identical
**************************************************************************/
void tab_extras::same_name_toggle(bool checked)
{
  name->setEnabled(!checked);
  if (checked) {
    name->setText(rname->text());
  }
}

/**********************************************************************//**
  User wants to edit reqs
**************************************************************************/
void tab_extras::edit_reqs()
{
  if (selected != nullptr) {
    ui->open_req_edit(QString::fromUtf8(extra_rule_name(selected)),
                      &selected->reqs);
  }
}

/**********************************************************************//**
  User wants to edit effects
**************************************************************************/
void tab_extras::edit_effects()
{
  if (selected != nullptr) {
    struct universal uni;

    uni.value.extra = selected;
    uni.kind = VUT_EXTRA;

    ui->open_effect_edit(QString::fromUtf8(extra_rule_name(selected)),
                         &uni, EFMC_NORMAL);
  }
}
