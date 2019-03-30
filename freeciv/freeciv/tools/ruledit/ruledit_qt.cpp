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
#include <QCloseEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedLayout>
#include <QVBoxLayout>

// utility
#include "fcintl.h"
#include "log.h"
#include "registry.h"

// common
#include "game.h"
#include "version.h"

// server
#include "ruleset.h"

// ruledit
#include "conversion_log.h"
#include "effect_edit.h"
#include "requirers_dlg.h"
#include "req_edit.h"
#include "ruledit.h"
#include "tab_building.h"
#include "tab_enablers.h"
#include "tab_extras.h"
#include "tab_good.h"
#include "tab_gov.h"
#include "tab_misc.h"
#include "tab_multiplier.h"
#include "tab_nation.h"
#include "tab_tech.h"
#include "tab_terrains.h"
#include "tab_unit.h"

#include "ruledit_qt.h"

static ruledit_gui *gui;
static QApplication *qapp;
static conversion_log *convlog;

/**********************************************************************//**
  Run ruledit-qt gui.
**************************************************************************/
int ruledit_qt_run(int argc, char **argv)
{
  ruledit_main *main_window;
  QWidget *central;
  int ret;

  qapp = new QApplication(argc, argv);
  central = new QWidget;
  main_window = new ruledit_main(qapp, central);

  gui = new ruledit_gui;
  gui->setup(central);
  main_window->setCentralWidget(central);
  main_window->setVisible(true);

  ret = qapp->exec();

  delete gui;
  delete qapp;

  return ret;
}

/**********************************************************************//**
  Display requirer list.
**************************************************************************/
void ruledit_qt_display_requirers(const char *msg, void *data)
{
  requirers_dlg *requirers = (requirers_dlg *)data;

  gui->show_required(requirers, msg);
}

/**********************************************************************//**
  Setup GUI object
**************************************************************************/
void ruledit_gui::setup(QWidget *central_in)
{
  QVBoxLayout *full_layout = new QVBoxLayout();
  QVBoxLayout *preload_layout = new QVBoxLayout();
  QWidget *preload_widget = new QWidget();
  QVBoxLayout *edit_layout = new QVBoxLayout();
  QWidget *edit_widget = new QWidget();
  QPushButton *ruleset_accept;
  QLabel *rs_label;
  QLabel *version_label;
  char verbuf[2048];
  const char *rev_ver;

  data.nationlist = NULL;
  data.nationlist_saved = NULL;

  central = central_in;

  rev_ver = fc_git_revision();

  if (rev_ver == NULL) {
    fc_snprintf(verbuf, sizeof(verbuf), "%s%s", word_version(), VERSION_STRING);
  } else {
    fc_snprintf(verbuf, sizeof(verbuf), _("%s%s\ncommit: %s"),
                word_version(), VERSION_STRING, rev_ver);
  }

  main_layout = new QStackedLayout();

  preload_layout->setSizeConstraint(QLayout::SetMaximumSize);
  version_label = new QLabel(verbuf);
  version_label->setAlignment(Qt::AlignHCenter);
  version_label->setParent(central);
  preload_layout->addWidget(version_label);
  rs_label = new QLabel(QString::fromUtf8(R__("Give ruleset to use as starting point.")));
  rs_label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
  preload_layout->addWidget(rs_label);
  ruleset_select = new QLineEdit(central);
  if (reargs.ruleset) {
    ruleset_select->setText(reargs.ruleset);
  } else {
    ruleset_select->setText(GAME_DEFAULT_RULESETDIR);
  }
  connect(ruleset_select, SIGNAL(returnPressed()),
          this, SLOT(launch_now()));
  preload_layout->addWidget(ruleset_select);
  ruleset_accept = new QPushButton(QString::fromUtf8(R__("Start editing")));
  connect(ruleset_accept, SIGNAL(pressed()), this, SLOT(launch_now()));
  preload_layout->addWidget(ruleset_accept);

  preload_widget->setLayout(preload_layout);
  main_layout->addWidget(preload_widget);

  stack = new QTabWidget(central);

  misc = new tab_misc(this);
  stack->addTab(misc, QString::fromUtf8(R__("Misc")));
  tech = new tab_tech(this);
  stack->addTab(tech, QString::fromUtf8(R__("Tech")));
  bldg = new tab_building(this);
  stack->addTab(bldg, QString::fromUtf8(R__("Buildings")));
  unit = new tab_unit(this);
  stack->addTab(unit, QString::fromUtf8(R__("Units")));
  good = new tab_good(this);
  stack->addTab(good, QString::fromUtf8(R__("Goods")));
  gov = new tab_gov(this);
  stack->addTab(gov, QString::fromUtf8(R__("Governments")));
  enablers = new tab_enabler(this);
  stack->addTab(enablers, QString::fromUtf8(R__("Enablers")));
  extras = new tab_extras(this);
  stack->addTab(extras, QString::fromUtf8(R__("Extras")));
  terrains = new tab_terrains(this);
  stack->addTab(terrains, QString::fromUtf8(R__("Terrains")));
  multipliers = new tab_multiplier(this);
  stack->addTab(multipliers, QString::fromUtf8(R__("Multipliers")));
  nation = new tab_nation(this);
  stack->addTab(nation, QString::fromUtf8(R__("Nations")));

  edit_layout->addWidget(stack);

  edit_widget->setLayout(edit_layout);
  main_layout->addWidget(edit_widget);

  full_layout->addLayout(main_layout);

  msg_dspl = new QLabel(QString::fromUtf8(R__("Welcome to freeciv-ruledit")));
  msg_dspl->setParent(central);

  msg_dspl->setAlignment(Qt::AlignHCenter);

  full_layout->addWidget(msg_dspl);

  central->setLayout(full_layout);

  req_edits = req_edit_list_new();
  effect_edits = effect_edit_list_new();
}

/**********************************************************************//**
  Ruleset conversion log callback
**************************************************************************/
static void conversion_log_cb(const char *msg)
{
  convlog->add(msg);
}

/**********************************************************************//**
  User entered savedir
**************************************************************************/
void ruledit_gui::launch_now()
{
  convlog = new conversion_log();

  sz_strlcpy(game.server.rulesetdir, ruleset_select->text().toUtf8().data());

  if (load_rulesets(NULL, TRUE, conversion_log_cb, FALSE, TRUE)) {
    display_msg(R__("Ruleset loaded"));

    /* Make freeable copy */
    if (game.server.ruledit.nationlist != NULL) {
      data.nationlist = fc_strdup(game.server.ruledit.nationlist);
    } else {
      data.nationlist = NULL;
    }

    bldg->refresh();
    misc->refresh();
    nation->refresh();
    tech->refresh();
    unit->refresh();
    good->refresh();
    gov->refresh();
    enablers->refresh();
    extras->refresh();
    multipliers->refresh();
    terrains->refresh();
    main_layout->setCurrentIndex(1);
  } else {
    display_msg(R__("Ruleset loading failed!"));
  }
}

/**********************************************************************//**
  Display status message
**************************************************************************/
void ruledit_gui::display_msg(const char *msg)
{
  msg_dspl->setText(QString::fromUtf8(msg));
}

/**********************************************************************//**
  Create requirers dlg.
**************************************************************************/
requirers_dlg *ruledit_gui::create_requirers(const char *title)
{
  requirers_dlg *requirers;

  requirers = new requirers_dlg(this);

  requirers->clear(title);

  return requirers;
}

/**********************************************************************//**
  Add entry to requirers dlg.
**************************************************************************/
void ruledit_gui::show_required(requirers_dlg *requirers, const char *msg)
{
  requirers->add(msg);

  // Show dialog if not already visible
  requirers->show();
}

/**********************************************************************//**
  Flush information from widgets to stores where it can be saved from.
**************************************************************************/
void ruledit_gui::flush_widgets()
{
  nation->flush_widgets();
}

/**********************************************************************//**
  Open req_edit dialog
**************************************************************************/
void ruledit_gui::open_req_edit(QString target, struct requirement_vector *preqs)
{
  req_edit *redit;

  req_edit_list_iterate(req_edits, old_edit) {
    if (old_edit->req_vector == preqs) {
      // Already open
      return;
    }
  } req_edit_list_iterate_end;

  redit = new req_edit(this, target, preqs);

  redit->show();

  req_edit_list_append(req_edits, redit);
}

/**********************************************************************//**
  Unregisted closed req_edit dialog
**************************************************************************/
void ruledit_gui::unregister_req_edit(class req_edit *redit)
{
  req_edit_list_remove(req_edits, redit);
}

/**********************************************************************//**
  Open effect_edit dialog
**************************************************************************/
void ruledit_gui::open_effect_edit(QString target, struct universal *uni,
                                   enum effect_filter_main_class efmc)
{
  effect_edit *e_edit;

  effect_edit_list_iterate(effect_edits, old_edit) {
    struct universal *old = old_edit->filter_get();

    if (uni != nullptr) {
      if (are_universals_equal(old, uni)) {
        // Already open
        return;
      }
    } else if (old->kind == VUT_NONE && old_edit->efmc == efmc) {
      // Already open
      return;
    }
  } effect_edit_list_iterate_end;

  e_edit = new effect_edit(this, target, uni, efmc);

  e_edit->show();

  effect_edit_list_append(effect_edits, e_edit);
}

/**********************************************************************//**
  Unregisted closed effect_edit dialog
**************************************************************************/
void ruledit_gui::unregister_effect_edit(class effect_edit *e_edit)
{
  effect_edit_list_remove(effect_edits, e_edit);
}

/**********************************************************************//**
  Refresh all effect edit dialogs
**************************************************************************/
void ruledit_gui::refresh_effect_edits()
{
  effect_edit_list_iterate(effect_edits, e_edit) {
    e_edit->refresh();
  } effect_edit_list_iterate_end;
}

/**********************************************************************//**
  Main window constructor
**************************************************************************/
ruledit_main::ruledit_main(QApplication *qapp_in, QWidget *central_in) : QMainWindow()
{
  const QString title = QString::fromUtf8(R__("Freeciv Ruleset Editor"));

  qapp = qapp_in;
  central = central_in;

  setWindowTitle(title);
}

/**********************************************************************//**
  Open dialog to confirm that user wants to quit ruledit.
**************************************************************************/
void ruledit_main::popup_quit_dialog()
{
  QMessageBox ask(central);
  int ret;

  ask.setText(R__("Are you sure you want to quit?"));
  ask.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
  ask.setDefaultButton(QMessageBox::Cancel);
  ask.setIcon(QMessageBox::Warning);
  ask.setWindowTitle(R__("Quit?"));
  ret = ask.exec();

  switch (ret) {
  case QMessageBox::Cancel:
    return;
    break;
  case QMessageBox::Ok:
    qapp->quit();
    break;
  }
}

/**********************************************************************//**
  User clicked windows close button.
**************************************************************************/
void ruledit_main::closeEvent(QCloseEvent *cevent)
{
  // Handle quit via confirmation dialog.
  popup_quit_dialog();

  // Do not handle quit here, but let user to answer to confirmation dialog.
  cevent->ignore();
}
