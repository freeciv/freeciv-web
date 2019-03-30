/********************************************************************** 
 Freeciv - Copyright (C) 2003 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#ifndef FC__MESSAGEDLG_H
#define FC__MESSAGEDLG_H

extern "C" {
#include "messagedlg_g.h"
}

//Qt
#include <QWidget>

class QTableWidget;
class QGridLayout;

/**************************************************************************
  Widget for displaying messages options
**************************************************************************/
class message_dlg : public QWidget
{
  Q_OBJECT
  QTableWidget *msgtab;
  QGridLayout *layout;
public:
  message_dlg();
  ~message_dlg();
  void fill_data();
private slots:
  void apply_changes();
  void cancel_changes();
};

void popup_messageopt_dialog(void);

#endif /* FC__MESSAGEDLG_H */
