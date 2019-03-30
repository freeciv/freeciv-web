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
#include <QHeaderView>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QTableWidget>

// utility
#include "fcintl.h"
#include "log.h"
#include "registry.h"

// common
#include "achievements.h"
#include "game.h"
#include "government.h"
#include "specialist.h"

// server
#include "rssanity.h"

// ruledit
#include "ruledit.h"
#include "ruledit_qt.h"
#include "rulesave.h"

#include "tab_misc.h"

/**********************************************************************//**
  Setup tab_misc object
**************************************************************************/
tab_misc::tab_misc(ruledit_gui *ui_in) : QWidget()
{
  QGridLayout *main_layout = new QGridLayout(this);
  QLabel *save_label;
  QLabel *save_ver_label;
  QLabel *name_label;
  QLabel *version_label;
  QPushButton *save_button;
  QPushButton *always_active_effects;
  QPushButton *all_effects;
  QPushButton *refresh_button;
  int row = 0;
  QTableWidgetItem *item;

  ui = ui_in;

  main_layout->setSizeConstraint(QLayout::SetMaximumSize);

  name_label = new QLabel(QString::fromUtf8(R__("Ruleset name")));
  name_label->setParent(this);
  main_layout->addWidget(name_label, row, 0);
  name = new QLineEdit(this);
  main_layout->addWidget(name, row++, 1);
  version_label = new QLabel(QString::fromUtf8(R__("Ruleset version")));
  version_label->setParent(this);
  main_layout->addWidget(version_label, row, 0);
  version = new QLineEdit(this);
  main_layout->addWidget(version, row++, 1);
  save_label = new QLabel(QString::fromUtf8(R__("Save to directory")));
  save_label->setParent(this);
  main_layout->addWidget(save_label, row, 0);
  savedir = new QLineEdit(this);
  savedir->setText("ruledit-tmp");
  savedir->setFocus();
  main_layout->addWidget(savedir, row++, 1);
  save_ver_label = new QLabel(QString::fromUtf8(R__("Version suffix to directory name")));
  save_ver_label->setParent(this);
  main_layout->addWidget(save_ver_label, row, 0);
  savedir_version = new QRadioButton(this);
  main_layout->addWidget(savedir_version, row++, 1);
  save_button = new QPushButton(QString::fromUtf8(R__("Save now")), this);
  connect(save_button, SIGNAL(pressed()), this, SLOT(save_now()));
  main_layout->addWidget(save_button, row++, 1);
  always_active_effects = new QPushButton(QString::fromUtf8(R__("Always active Effects")), this);
  connect(always_active_effects, SIGNAL(pressed()), this, SLOT(edit_aae_effects()));
  main_layout->addWidget(always_active_effects, row++, 1);
  all_effects = new QPushButton(QString::fromUtf8(R__("All Effects")), this);
  connect(all_effects, SIGNAL(pressed()), this, SLOT(edit_all_effects()));
  main_layout->addWidget(all_effects, row++, 1);

  stats = new QTableWidget(this);
  stats->setColumnCount(8);
  stats->setRowCount(6);
  item = new QTableWidgetItem(QString::fromUtf8(RQ_("?stat:Terrains")));
  stats->setItem(0, 0, item);
  item = new QTableWidgetItem("-");
  stats->setItem(0, 1, item);
  item = new QTableWidgetItem(QString::fromUtf8(RQ_("?stat:Resources")));
  stats->setItem(1, 0, item);
  item = new QTableWidgetItem("-");
  stats->setItem(1, 1, item);
  item = new QTableWidgetItem(QString::fromUtf8(RQ_("?stat:Tech Classes")));
  stats->setItem(2, 0, item);
  item = new QTableWidgetItem("-");
  stats->setItem(2, 1, item);
  item = new QTableWidgetItem(QString::fromUtf8(RQ_("?stat:Techs")));
  stats->setItem(3, 0, item);
  item = new QTableWidgetItem("-");
  stats->setItem(3, 1, item);
  item = new QTableWidgetItem(QString::fromUtf8(RQ_("?stat:Unit Classes")));
  stats->setItem(4, 0, item);
  item = new QTableWidgetItem("-");
  stats->setItem(4, 1, item);
  item = new QTableWidgetItem(QString::fromUtf8(RQ_("?stat:Unit Types")));
  stats->setItem(5, 0, item);
  item = new QTableWidgetItem("-");
  stats->setItem(5, 1, item);
  item = new QTableWidgetItem(QString::fromUtf8(RQ_("?stat:Buildings")));
  stats->setItem(0, 3, item);
  item = new QTableWidgetItem("-");
  stats->setItem(0, 4, item);
  item = new QTableWidgetItem(QString::fromUtf8(RQ_("?stat:Nations")));
  stats->setItem(1, 3, item);
  item = new QTableWidgetItem("-");
  stats->setItem(1, 4, item);
  item = new QTableWidgetItem(QString::fromUtf8(RQ_("?stat:Styles")));
  stats->setItem(2, 3, item);
  item = new QTableWidgetItem("-");
  stats->setItem(2, 4, item);
  item = new QTableWidgetItem(QString::fromUtf8(RQ_("?stat:Specialists")));
  stats->setItem(3, 3, item);
  item = new QTableWidgetItem("-");
  stats->setItem(3, 4, item);
  item = new QTableWidgetItem(QString::fromUtf8(RQ_("?stat:Governments")));
  stats->setItem(4, 3, item);
  item = new QTableWidgetItem("-");
  stats->setItem(4, 4, item);
  item = new QTableWidgetItem(QString::fromUtf8(RQ_("?stat:Disasters")));
  stats->setItem(5, 3, item);
  item = new QTableWidgetItem("-");
  stats->setItem(5, 4, item);
  item = new QTableWidgetItem(QString::fromUtf8(RQ_("?stat:Achievements")));
  stats->setItem(0, 6, item);
  item = new QTableWidgetItem("-");
  stats->setItem(0, 7, item);
  item = new QTableWidgetItem(QString::fromUtf8(RQ_("?stat:Extras")));
  stats->setItem(1, 6, item);
  item = new QTableWidgetItem("-");
  stats->setItem(1, 7, item);
  item = new QTableWidgetItem(QString::fromUtf8(RQ_("?stat:Bases")));
  stats->setItem(2, 6, item);
  item = new QTableWidgetItem("-");
  stats->setItem(2, 7, item);
  item = new QTableWidgetItem(QString::fromUtf8(RQ_("?stat:Roads")));
  stats->setItem(3, 6, item);
  item = new QTableWidgetItem("-");
  stats->setItem(3, 7, item);
  item = new QTableWidgetItem(QString::fromUtf8(RQ_("?stat:Goods")));
  stats->setItem(4, 6, item);
  item = new QTableWidgetItem("-");
  stats->setItem(4, 7, item);
  item = new QTableWidgetItem(QString::fromUtf8(RQ_("?stat:Multipliers")));
  stats->setItem(5, 6, item);
  item = new QTableWidgetItem("-");
  stats->setItem(5, 7, item);
  stats->verticalHeader()->setVisible(false);
  stats->horizontalHeader()->setVisible(false);
  stats->setEditTriggers(QAbstractItemView::NoEditTriggers);
  main_layout->addWidget(stats, row++, 0, 1, 2);
  refresh_button = new QPushButton(QString::fromUtf8(R__("Refresh Stats")), this);
  connect(refresh_button, SIGNAL(pressed()), this, SLOT(refresh_stats()));
  main_layout->addWidget(refresh_button, row++, 0, 1, 2);

  // Stats never change except with experimental features. Hide useless
  // button.
  show_experimental(refresh_button);

  refresh();

  setLayout(main_layout);
}

/**********************************************************************//**
  Refresh the information.
**************************************************************************/
void tab_misc::refresh()
{
  name->setText(game.control.name);
  version->setText(game.control.version);
  refresh_stats();
}

/**********************************************************************//**
  User entered savedir
**************************************************************************/
void tab_misc::save_now()
{
  char nameUTF8[MAX_LEN_NAME];
  QString full_dir;

  ui->flush_widgets();

  strncpy(nameUTF8, name->text().toUtf8().data(), sizeof(nameUTF8) - 1);

  if (nameUTF8[0] != '\0') {
    strncpy(game.control.name, nameUTF8, sizeof(game.control.name));
  }

  strncpy(game.control.version, version->text().toUtf8().data(),
          sizeof(game.control.version) - 1);

  if (!autoadjust_ruleset_data() || !sanity_check_ruleset_data(false)) {
    QMessageBox *box = new QMessageBox();

    box->setText("Current data fails sanity checks. Save anyway?");
    box->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    box->exec();

    if (box->result() != QMessageBox::Yes) {
      return;
    }
  }

  if (savedir_version->isChecked()) {
    full_dir = savedir->text() + "-" + version->text();
  } else {
    full_dir = savedir->text();
  }

  save_ruleset(full_dir.toUtf8().data(), nameUTF8,
               &(ui->data));

  ui->display_msg(R__("Ruleset saved"));
}

/**********************************************************************//**
  Recalculate stats
**************************************************************************/
void tab_misc::refresh_stats()
{
  int row = 0;
  int count;
  int base_count;
  int road_count;

  count = 0;
  terrain_re_active_iterate(pterr) {
    count++;
  } terrain_re_active_iterate_end;
  stats->item(row++, 1)->setText(QString::number(count));

  stats->item(row++, 1)->setText(QString::number(game.control.num_resource_types));

  count = 0;
  tech_class_re_active_iterate(ptclass) {
    count++;
  } tech_class_re_active_iterate_end;
  stats->item(row++, 1)->setText(QString::number(count));

  count = 0;
  advance_re_active_iterate(padv) {
    count++;
  } advance_re_active_iterate_end;
  stats->item(row++, 1)->setText(QString::number(count));

  count = 0;
  unit_class_re_active_iterate(pclass) {
    count++;
  } unit_class_re_active_iterate_end;
  stats->item(row++, 1)->setText(QString::number(count));

  count = 0;
  unit_type_re_active_iterate(ptype) {
    count++;
  } unit_type_re_active_iterate_end;
  stats->item(row++, 1)->setText(QString::number(count));

  // Second column
  row = 0;
  count = 0;
  improvement_re_active_iterate(pimpr) {
    count++;
  } improvement_re_active_iterate_end;
  stats->item(row++, 4)->setText(QString::number(count));

  stats->item(row++, 4)->setText(QString::number(game.control.nation_count));

  count = 0;
  styles_re_active_iterate(pstyle) {
    count++;
  } styles_re_active_iterate_end;
  stats->item(row++, 4)->setText(QString::number(count));

  count = 0;
  specialist_type_re_active_iterate(pspe) {
    count++;
  } specialist_type_re_active_iterate_end;
  stats->item(row++, 4)->setText(QString::number(count));

  count = 0;
  governments_re_active_iterate(pgov) {
    count++;
  } governments_re_active_iterate_end;
  stats->item(row++, 4)->setText(QString::number(count));

  stats->item(row++, 4)->setText(QString::number(game.control.num_disaster_types));

  // Third column
  row = 0;

  count = 0;
  achievements_re_active_iterate(pach) {
    count++;
  } achievements_re_active_iterate_end;
  stats->item(row++, 7)->setText(QString::number(count));

  count = 0;
  base_count = 0;
  road_count = 0;
  extra_type_re_active_iterate(pextra) {
    count++;
    if (is_extra_caused_by(pextra, EC_BASE)) {
      base_count++;
    }
    if (is_extra_caused_by(pextra, EC_ROAD)) {
      road_count++;
    }
  } extra_type_re_active_iterate_end;
  stats->item(row++, 7)->setText(QString::number(count));
  stats->item(row++, 7)->setText(QString::number(base_count));
  stats->item(row++, 7)->setText(QString::number(road_count));

  count = 0;
  goods_type_re_active_iterate(pg) {
    count++;
  } goods_type_re_active_iterate_end;
  stats->item(row++, 7)->setText(QString::number(count));

  count = 0;
  multipliers_re_active_iterate(pmul) {
    count++;
  } multipliers_re_active_iterate_end;
  stats->item(row++, 7)->setText(QString::number(count));

  stats->resizeColumnsToContents();
}

/**********************************************************************//**
  User wants to edit always active effects
**************************************************************************/
void tab_misc::edit_aae_effects()
{
  ui->open_effect_edit(QString::fromUtf8(R__("Always active")),
                       nullptr, EFMC_NONE);
}

/**********************************************************************//**
  User wants to edit effects from full list
**************************************************************************/
void tab_misc::edit_all_effects()
{
  ui->open_effect_edit(QString::fromUtf8(R__("All effects")),
                       nullptr, EFMC_ALL);
}
