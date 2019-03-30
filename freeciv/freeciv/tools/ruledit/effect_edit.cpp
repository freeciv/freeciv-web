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
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QSpinBox>

// utility
#include "fcintl.h"

// common
#include "effects.h"

// ruledit
#include "ruledit.h"
#include "ruledit_qt.h"
#include "validity.h"

#include "effect_edit.h"

/**********************************************************************//**
  Setup effect_edit object
**************************************************************************/
effect_edit::effect_edit(ruledit_gui *ui_in, QString target,
                         struct universal *filter_in,
                         enum effect_filter_main_class efmc_in) : QDialog()
{
  QVBoxLayout *main_layout = new QVBoxLayout(this);
  QGridLayout *effect_edit_layout = new QGridLayout();
  QHBoxLayout *active_layout = new QHBoxLayout();
  QPushButton *close_button;
  QPushButton *reqs_button;
  QMenu *menu;
  QLabel *lbl;
  enum effect_type eff;

  ui = ui_in;
  selected = nullptr;
  if (filter_in == nullptr) {
    fc_assert(efmc_in != EFMC_NORMAL);
    filter.kind = VUT_NONE;
  } else {
    fc_assert(efmc_in == EFMC_NORMAL);
    filter = *filter_in;
  }
  name = target;

  efmc = efmc_in;

  list_widget = new QListWidget(this);
  effects = effect_list_new();

  connect(list_widget, SIGNAL(itemSelectionChanged()), this, SLOT(select_effect()));
  main_layout->addWidget(list_widget);

  lbl = new QLabel(R__("Type:"));
  active_layout->addWidget(lbl, 0, 0);
  edit_type_button = new QToolButton(this);
  menu = new QMenu();
  edit_type_button->setToolButtonStyle(Qt::ToolButtonTextOnly);
  edit_type_button->setPopupMode(QToolButton::MenuButtonPopup);
  connect(menu, SIGNAL(triggered(QAction *)), this, SLOT(effect_type_menu(QAction *)));
  edit_type_button->setMenu(menu);
  for (eff = (enum effect_type)0; eff < EFT_COUNT;
       eff = (enum effect_type)(eff + 1)) {
    menu->addAction(effect_type_name(eff));
  }
  active_layout->addWidget(edit_type_button, 1, 0);

  lbl = new QLabel(R__("Value:"));
  active_layout->addWidget(lbl, 2, 0);
  value_box = new QSpinBox(this);
  active_layout->addWidget(value_box, 3, 0);
  connect(value_box, SIGNAL(valueChanged(int)), this, SLOT(set_value(int)));

  main_layout->addLayout(active_layout);

  reqs_button = new QPushButton(QString::fromUtf8(R__("Requirements")), this);
  connect(reqs_button, SIGNAL(pressed()), this, SLOT(edit_reqs()));
  effect_edit_layout->addWidget(reqs_button, 0, 0);

  close_button = new QPushButton(QString::fromUtf8(R__("Close")), this);
  connect(close_button, SIGNAL(pressed()), this, SLOT(close_now()));
  effect_edit_layout->addWidget(close_button, 1, 0);

  refresh();

  main_layout->addLayout(effect_edit_layout);

  setLayout(main_layout);
  setWindowTitle(target);
}

/**********************************************************************//**
  Effect edit destructor
**************************************************************************/
effect_edit::~effect_edit()
{
  effect_list_destroy(effects);
}

/**********************************************************************//**
  Callback to fill effects list from iterate_effect_cache()
**************************************************************************/
static bool effect_list_fill_cb(struct effect *peffect, void *data)
{
  struct effect_list_fill_data *cbdata = (struct effect_list_fill_data *)data;

  if (cbdata->filter->kind == VUT_NONE) {
    if (cbdata->efmc == EFMC_NONE) {
      // Look for empty req lists.
      if (requirement_vector_size(&peffect->reqs) == 0) {
        cbdata->edit->add_effect_to_list(peffect, cbdata);
      }
    } else {
      fc_assert(cbdata->efmc == EFMC_ALL);
      cbdata->edit->add_effect_to_list(peffect, cbdata);
    }
  } else if (universal_is_mentioned_by_requirements(&peffect->reqs,
                                                    cbdata->filter)) {
    cbdata->edit->add_effect_to_list(peffect, cbdata);
  }

  return true;
}

/**********************************************************************//**
  Refresh the information.
**************************************************************************/
void effect_edit::refresh()
{
  struct effect_list_fill_data cb_data;

  list_widget->clear();
  effect_list_clear(effects);
  cb_data.filter = &filter;
  cb_data.efmc = efmc;
  cb_data.edit = this;
  cb_data.num = 0;

  iterate_effect_cache(effect_list_fill_cb, &cb_data);

  fill_active();
}

/**********************************************************************//**
  Add entry to effect list.
**************************************************************************/
void effect_edit::add_effect_to_list(struct effect *peffect,
                                     struct effect_list_fill_data *data)
{
  char buf[512];
  QListWidgetItem *item;

  fc_snprintf(buf, sizeof(buf), _("Effect #%d: %s"),
              data->num + 1, effect_type_name(peffect->type));

  item = new QListWidgetItem(QString::fromUtf8(buf));
  list_widget->insertItem(data->num++, item);
  effect_list_append(effects, peffect);
  if (selected == peffect) {
    item->setSelected(true);
  }
}

/**********************************************************************//**
  Getter for filter
**************************************************************************/
struct universal *effect_edit::filter_get()
{
  return &filter;
}

/**********************************************************************//**
  User pushed close button
**************************************************************************/
void effect_edit::close_now()
{
  ui->unregister_effect_edit(this);
  done(0);
}

/**********************************************************************//**
  User selected effect from the list.
**************************************************************************/
void effect_edit::select_effect()
{
  int i = 0;

  effect_list_iterate(effects, peffect) {
    QListWidgetItem *item = list_widget->item(i++);

    if (item != nullptr && item->isSelected()) {
      selected = peffect;
      selected_nbr = i;
      fill_active();
      return;
    }
  } effect_list_iterate_end;
}

/**********************************************************************//**
  Fill active menus from selected effect.
**************************************************************************/
void effect_edit::fill_active()
{
  if (selected != nullptr) {
    edit_type_button->setText(effect_type_name(selected->type));
    value_box->setValue(selected->value);
  }
}

/**********************************************************************//**
  User selected type for the effect.
**************************************************************************/
void effect_edit::effect_type_menu(QAction *action)
{
  enum effect_type type = effect_type_by_name(action->text().toUtf8().data(),
                                              fc_strcasecmp);

  if (selected != nullptr) {
    selected->type = type;
  }

  ui->refresh_effect_edits();
}

/**********************************************************************//**
  Read value from spinbox to effect
**************************************************************************/
void effect_edit::set_value(int value)
{
  if (selected != nullptr) {
    selected->value = value;
  }

  ui->refresh_effect_edits();
}

/**********************************************************************//**
  User wants to edit requirements
**************************************************************************/
void effect_edit::edit_reqs()
{
  if (selected != nullptr) {
    char buf[128];

    fc_snprintf(buf, sizeof(buf), R__("%s effect #%d"), name.toUtf8().data(),
                selected_nbr);

    ui->open_req_edit(QString::fromUtf8(buf), &selected->reqs);
  }
}

/**********************************************************************//**
  User clicked windows close button.
**************************************************************************/
void effect_edit::closeEvent(QCloseEvent *event)
{
  ui->unregister_effect_edit(this);
}
