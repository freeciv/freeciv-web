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

#ifndef FC__TAB_GOV_H
#define FC__TAB_GOV_H

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

// Qt
#include <QWidget>

class QLineEdit;
class QListWidget;
class QRadioButton;

class ruledit_gui;

class tab_gov : public QWidget
{
  Q_OBJECT

  public:
    explicit tab_gov(ruledit_gui *ui_in);
    void refresh();

  private:
    ruledit_gui *ui;
    void update_gov_info(struct government *pgov);
    bool initialize_new_gov(struct government *pgov);

    QLineEdit *name;
    QLineEdit *rname;
    QListWidget *gov_list;
    QRadioButton *same_name;

    struct government *selected;

  private slots:
    void name_given();
    void select_gov();
    void add_now();
    void delete_now();
    void same_name_toggle(bool checked);
    void edit_reqs();
    void edit_effects();
};

#endif // FC__TAB_GOV_H
