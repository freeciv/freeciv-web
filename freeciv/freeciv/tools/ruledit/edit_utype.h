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

#ifndef FC__EDIT_UTYPE_H
#define FC__EDIT_UTYPE_H

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

// Qt
#include <QDialog>

class QToolButton;

class ruledit_gui;

class edit_utype : public QDialog
{
  Q_OBJECT

  public:
    explicit edit_utype(ruledit_gui *ui_in, struct unit_type *utype_in);
    void refresh();

  private:
    ruledit_gui *ui;
    struct unit_type *utype;
    QToolButton *req_button;

  private slots:
    void req_menu(QAction *action);
};


#endif // FC__EDIT_UTYPE_H
