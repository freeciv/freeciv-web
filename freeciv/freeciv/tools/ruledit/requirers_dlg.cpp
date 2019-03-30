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
#include <QGridLayout>
#include <QPushButton>

// utility
#include "fcintl.h"

#include "requirers_dlg.h"

/**********************************************************************//**
  Setup requirers_dlg object
**************************************************************************/
requirers_dlg::requirers_dlg(ruledit_gui *ui_in) : QDialog()
{
  QGridLayout *main_layout = new QGridLayout(this);
  QPushButton *close_button;
  int row = 0;

  ui = ui_in;

  area = new QTextEdit();
  area->setParent(this);
  area->setReadOnly(true);
  main_layout->addWidget(area, row++, 0);

  close_button = new QPushButton(QString::fromUtf8(R__("Close")), this);
  connect(close_button, SIGNAL(pressed()), this, SLOT(close_now()));
  main_layout->addWidget(close_button, row++, 0);

  setLayout(main_layout);
}

/**********************************************************************//**
  Clear text area
**************************************************************************/
void requirers_dlg::clear(const char *title)
{
  char buffer[256];

  fc_snprintf(buffer, sizeof(buffer), R__("Removing %s"), title);

  setWindowTitle(QString::fromUtf8(buffer));
  area->clear();
}

/**********************************************************************//**
  Add requirer entry
**************************************************************************/
void requirers_dlg::add(const char *msg)
{
  char buffer[2048];

  /* TRANS: %s could be any of a number of ruleset items (e.g., tech,
   * unit type, ... */
  fc_snprintf(buffer, sizeof(buffer), R__("Needed by %s"), msg);

  area->append(QString::fromUtf8(buffer));
}

/**********************************************************************//**
  User pushed close button
**************************************************************************/
void requirers_dlg::close_now()
{
  done(0);
}
