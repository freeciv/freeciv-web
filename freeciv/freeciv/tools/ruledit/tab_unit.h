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

#ifndef FC__TAB_UNIT_H
#define FC__TAB_UNIT_H

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

// Qt
#include <QWidget>

class QLineEdit;
class QListWidget;
class QRadioButton;

class ruledit_gui;

class tab_unit : public QWidget
{
  Q_OBJECT

  public:
    explicit tab_unit(ruledit_gui *ui_in);
    void refresh();

  private:
    ruledit_gui *ui;
    void update_utype_info(struct unit_type *ptype);
    bool initialize_new_utype(struct unit_type *ptype);

    QLineEdit *name;
    QLineEdit *rname;
    QListWidget *unit_list;
    QRadioButton *same_name;

    struct unit_type *selected;

  private slots:
    void name_given();
    void select_unit();
    void add_now();
    void delete_now();
    void edit_now();
    void same_name_toggle(bool checked);
    void edit_effects();
};

#endif // FC__TAB_UNIT_H
