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

#ifndef FC__SPACESHIPDLG_H
#define FC__SPACESHIPDLG_H

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

extern "C" {
#include "spaceshipdlg_g.h"
}

/* client */
#include "text.h"

// Qt
#include <QWidget>

class QLabel;
class QPixmap;
class QPushButton;

/****************************************************************************
  Tab widget to display spaceship report (F12)
****************************************************************************/
class ss_report: public QWidget
{
  Q_OBJECT
  QPushButton *launch_button;
  QLabel *ss_pix_label;
  QLabel *ss_label;
  struct canvas *can;

public:
  ss_report(struct player *pplayer);
  ~ss_report();
  void update_report();
  void init();

private slots:
  void launch();

private:
  struct player *player;
};

void popup_spaceship_dialog(struct player *pplayer);
void popdown_all_spaceships_dialogs();

#endif /* FC__SPACESHIPDLG_H */
