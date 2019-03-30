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
#include "unittype.h"

// ruledit
#include "edit_utype.h"
#include "ruledit.h"
#include "ruledit_qt.h"
#include "validity.h"

#include "tab_unit.h"

/**********************************************************************//**
  Setup tab_unit object
**************************************************************************/
tab_unit::tab_unit(ruledit_gui *ui_in) : QWidget()
{
  QVBoxLayout *main_layout = new QVBoxLayout(this);
  QGridLayout *unit_layout = new QGridLayout();
  QLabel *label;
  QPushButton *effects_button;
  QPushButton *add_button;
  QPushButton *delete_button;
  QPushButton *edit_button;

  ui = ui_in;
  selected = 0;

  unit_list = new QListWidget(this);

  connect(unit_list, SIGNAL(itemSelectionChanged()), this, SLOT(select_unit()));
  main_layout->addWidget(unit_list);

  unit_layout->setSizeConstraint(QLayout::SetMaximumSize);

  label = new QLabel(QString::fromUtf8(R__("Rule Name")));
  label->setParent(this);
  rname = new QLineEdit(this);
  rname->setText("None");
  connect(rname, SIGNAL(returnPressed()), this, SLOT(name_given()));
  unit_layout->addWidget(label, 0, 0);
  unit_layout->addWidget(rname, 0, 2);

  label = new QLabel(QString::fromUtf8(R__("Name")));
  label->setParent(this);
  same_name = new QRadioButton();
  connect(same_name, SIGNAL(toggled(bool)), this, SLOT(same_name_toggle(bool)));
  name = new QLineEdit(this);
  name->setText("None");
  connect(name, SIGNAL(returnPressed()), this, SLOT(name_given()));
  unit_layout->addWidget(label, 1, 0);
  unit_layout->addWidget(same_name, 1, 1);
  unit_layout->addWidget(name, 1, 2);

  edit_button = new QPushButton(QString::fromUtf8(R__("Edit Unit")), this);
  connect(edit_button, SIGNAL(pressed()), this, SLOT(edit_now()));
  unit_layout->addWidget(edit_button, 2, 2);

  effects_button = new QPushButton(QString::fromUtf8(R__("Effects")), this);
  connect(effects_button, SIGNAL(pressed()), this, SLOT(edit_effects()));
  unit_layout->addWidget(effects_button, 3, 2);

  add_button = new QPushButton(QString::fromUtf8(R__("Add Unit")), this);
  connect(add_button, SIGNAL(pressed()), this, SLOT(add_now()));
  unit_layout->addWidget(add_button, 4, 0);
  show_experimental(add_button);

  delete_button = new QPushButton(QString::fromUtf8(R__("Remove this Unit")), this);
  connect(delete_button, SIGNAL(pressed()), this, SLOT(delete_now()));
  unit_layout->addWidget(delete_button, 4, 2);
  show_experimental(delete_button);

  refresh();

  main_layout->addLayout(unit_layout);

  setLayout(main_layout);
}

/**********************************************************************//**
  Refresh the information.
**************************************************************************/
void tab_unit::refresh()
{
  unit_list->clear();

  unit_type_iterate(ptype) {
    if (!ptype->ruledit_disabled) {
      QListWidgetItem *item = new QListWidgetItem(utype_rule_name(ptype));

      unit_list->insertItem(utype_index(ptype), item);
    }
  } unit_type_iterate_end;
}

/**********************************************************************//**
  Update info of the unit
**************************************************************************/
void tab_unit::update_utype_info(struct unit_type *ptype)
{
  selected = ptype;

  if (selected != nullptr) {
    QString dispn = QString::fromUtf8(untranslated_name(&(ptype->name)));
    QString rulen = QString::fromUtf8(utype_rule_name(ptype));

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
  User selected unit from the list.
**************************************************************************/
void tab_unit::select_unit()
{
  QList<QListWidgetItem *> select_list = unit_list->selectedItems();

  if (!select_list.isEmpty()) {
    update_utype_info(unit_type_by_rule_name(select_list.at(0)->text().toUtf8().data()));
  }
}

/**********************************************************************//**
  User entered name for the unit
**************************************************************************/
void tab_unit::name_given()
{
  if (selected != nullptr) {
    unit_type_iterate(ptype) {
      if (ptype != selected && !ptype->ruledit_disabled) {
        if (!strcmp(utype_rule_name(ptype), rname->text().toUtf8().data())) {
          ui->display_msg(R__("A unit type with that rule name already "
                              "exists!"));
          return;
        }
      }
    } unit_type_iterate_end;

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
  User requested unit deletion 
**************************************************************************/
void tab_unit::delete_now()
{
  if (selected != 0) {
    requirers_dlg *requirers;

    requirers = ui->create_requirers(utype_rule_name(selected));
    if (is_utype_needed(selected, &ruledit_qt_display_requirers, requirers)) {
      return;
    }

    selected->ruledit_disabled = true;

    refresh();
    update_utype_info(nullptr);
  }
}

/**********************************************************************//**
  User requested unit edit dialog
**************************************************************************/
void tab_unit::edit_now()
{
  if (selected != nullptr) {
    edit_utype *edit = new edit_utype(ui, selected);

    edit->show();
  }
}

/**********************************************************************//**
  Initialize new tech for use.
**************************************************************************/
bool tab_unit::initialize_new_utype(struct unit_type *ptype)
{
  if (unit_type_by_rule_name("New Unit") != nullptr) {
    return false;
  }

  name_set(&(ptype->name), 0, "New Unit");
  return true;
}

/**********************************************************************//**
  User requested new unit
**************************************************************************/
void tab_unit::add_now()
{
  struct unit_type *new_utype;

  // Try to reuse freed utype slot
  unit_type_iterate(ptype) {
    if (ptype->ruledit_disabled) {
      if (initialize_new_utype(ptype)) {
        ptype->ruledit_disabled = false;
        update_utype_info(ptype);
        refresh();
      }
      return;
    }
  } unit_type_iterate_end;

  // Try to add completely new unit type
  if (game.control.num_unit_types >= U_LAST) {
    return;
  }

  // num_unit_types must be big enough to hold new unit or
  // utype_by_number() fails.
  game.control.num_unit_types++;
  new_utype = utype_by_number(game.control.num_unit_types - 1);
  if (initialize_new_utype(new_utype)) {
    update_utype_info(new_utype);

    refresh();
  } else {
    game.control.num_unit_types--; // Restore
  }
}

/**********************************************************************//**
  Toggled whether rule_name and name should be kept identical
**************************************************************************/
void tab_unit::same_name_toggle(bool checked)
{
  name->setEnabled(!checked);
  if (checked) {
    name->setText(rname->text());
  }
}

/**********************************************************************//**
  User wants to edit effects
**************************************************************************/
void tab_unit::edit_effects()
{
  if (selected != nullptr) {
    struct universal uni;

    uni.value.utype = selected;
    uni.kind = VUT_UTYPE;

    ui->open_effect_edit(QString::fromUtf8(utype_rule_name(selected)),
                         &uni, EFMC_NORMAL);
  }
}
