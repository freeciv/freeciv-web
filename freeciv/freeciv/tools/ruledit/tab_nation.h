/********************************************************************** 
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

#ifndef FC__TAB_NATION_H
#define FC__TAB_NATION_H

// Qt
#include <QLineEdit>
#include <QRadioButton>
#include <QWidget>

class ruledit_gui;

class tab_nation : public QWidget
{
  Q_OBJECT

  public:
    explicit tab_nation(ruledit_gui *ui_in);
    void refresh();
    void flush_widgets();

  private slots:
    void nationlist_toggle(bool checked);

  private:
    ruledit_gui *ui;

    QRadioButton *via_include;
    QLineEdit *nationlist;
};


#endif // FC__TAB_MISC_H
