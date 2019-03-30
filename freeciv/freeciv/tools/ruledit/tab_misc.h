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

#ifndef FC__TAB_MISC_H
#define FC__TAB_MISC_H

// Qt
#include <QWidget>

class QLineEdit;
class QRadioButton;
class QTableWidget;

class ruledit_gui;

class tab_misc : public QWidget
{
  Q_OBJECT

  public:
    explicit tab_misc(ruledit_gui *ui_in);
    void refresh();
    void flush_widgets();

  private slots:
    void save_now();
    void refresh_stats();
    void edit_aae_effects();
    void edit_all_effects();

  private:
    ruledit_gui *ui;
    QLineEdit *name;
    QLineEdit *version;
    QLineEdit *savedir;
    QRadioButton *savedir_version;
    QTableWidget *stats;
};


#endif // FC__TAB_MISC_H
