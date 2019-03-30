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

#ifndef FC__EFFECT_EDIT_H
#define FC__EFFECT_EDIT_H

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

// Qt
#include <QDialog>
#include <QListWidget>
#include <QToolButton>

// common
#include "requirements.h"

class QSpinBox;

class ruledit_gui;

enum effect_filter_main_class { EFMC_NORMAL,
                                EFMC_NONE, /* No requirements */
                                EFMC_ALL   /* Any requirements */
};

struct effect_list_fill_data
{
  struct universal *filter;
  enum effect_filter_main_class efmc;
  class effect_edit *edit;
  int num;
};

class effect_edit : public QDialog
{
  Q_OBJECT

  public:
    explicit effect_edit(ruledit_gui *ui_in, QString target,
                         struct universal *filter_in, enum effect_filter_main_class efmc_in);
    ~effect_edit();
    void refresh();
    void add(const char *msg);
    void add_effect_to_list(struct effect *peffect,
                            struct effect_list_fill_data *data);

    struct universal *filter_get();

    enum effect_filter_main_class efmc;

  private:
    ruledit_gui *ui;

    QString name;
    QListWidget *list_widget;
    struct universal filter;
    struct effect_list *effects;

    struct effect *selected;
    int selected_nbr;

    QToolButton *edit_type_button;
    QSpinBox *value_box;

  private slots:
    void select_effect();
    void fill_active();
    void edit_reqs();
    void close_now();

    void effect_type_menu(QAction *action);
    void set_value(int value);

 protected:
    void closeEvent(QCloseEvent *event);
};

#endif // FC__EFFECT_EDIT_H
