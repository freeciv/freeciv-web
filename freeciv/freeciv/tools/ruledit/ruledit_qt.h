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

#ifndef FC__RULEDIT_QT_H
#define FC__RULEDIT_QT_H

// Qt
#include <QApplication>
#include <QMainWindow>
#include <QObject>
#include <QLabel>
#include <QTabWidget>

// ruledit
#include "effect_edit.h"
#include "rulesave.h"

class QLineEdit;
class QStackedLayout;

class requirers_dlg;
class tab_building;
class tab_good;
class tab_gov;
class tab_misc;
class tab_multiplier;
class tab_tech;
class tab_unit;
class tab_nation;
class tab_enabler;
class tab_extras;
class tab_terrains;
class req_edit;

class ruledit_main : public QMainWindow
{
  Q_OBJECT

public:
  ruledit_main(QApplication *qapp_in, QWidget *central_in);

private:
  void popup_quit_dialog();
  QApplication *qapp;
  QWidget *central;

protected:
  void closeEvent(QCloseEvent *cevent);
};

/* get 'struct req_edit_list' and related functions: */
#define SPECLIST_TAG req_edit
#define SPECLIST_TYPE class req_edit
#include "speclist.h"

#define req_edit_list_iterate(reqeditlist, preqedit) \
  TYPED_LIST_ITERATE(class req_edit, reqeditlist, preqedit)
#define req_edit_list_iterate_end LIST_ITERATE_END

/* get 'struct effect_edit_list' and related functions: */
#define SPECLIST_TAG effect_edit
#define SPECLIST_TYPE class effect_edit
#include "speclist.h"

#define effect_edit_list_iterate(effecteditlist, peffectedit) \
  TYPED_LIST_ITERATE(class effect_edit, effecteditlist, peffectedit)
#define effect_edit_list_iterate_end LIST_ITERATE_END

class ruledit_gui : public QObject
{
  Q_OBJECT

  public:
    void setup(QWidget *central_in);
    void display_msg(const char *msg);
    requirers_dlg *create_requirers(const char *title);
    void show_required(requirers_dlg *requirers, const char *msg);
    void flush_widgets();

    void open_req_edit(QString target, struct requirement_vector *preqs);
    void unregister_req_edit(class req_edit *redit);

    void open_effect_edit(QString target, struct universal *uni,
                          enum effect_filter_main_class efmc);
    void unregister_effect_edit(class effect_edit *e_edit);
    void refresh_effect_edits();

    struct rule_data data;

  private:
    QLabel *msg_dspl;
    QTabWidget *stack;
    QLineEdit *ruleset_select;
    QWidget *central;
    QStackedLayout *main_layout;

    tab_building *bldg;
    tab_misc *misc;
    tab_tech *tech;
    tab_unit *unit;
    tab_good *good;
    tab_gov *gov;
    tab_enabler *enablers;
    tab_extras *extras;
    tab_multiplier *multipliers;
    tab_terrains *terrains;
    tab_nation *nation;

    struct req_edit_list *req_edits;
    struct effect_edit_list *effect_edits;

  private slots:
    void launch_now();
};

int ruledit_qt_run(int argc, char **argv);
void ruledit_qt_display_requirers(const char *msg, void *data);

#endif // FC__RULEDIT_QT_H
