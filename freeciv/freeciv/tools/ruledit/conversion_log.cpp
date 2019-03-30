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

#include "conversion_log.h"

/**********************************************************************//**
  Setup conversion_log object
**************************************************************************/
conversion_log::conversion_log() : QDialog()
{
  QGridLayout *main_layout = new QGridLayout(this);
  QPushButton *close_button;
  int row = 0;

  area = new QTextEdit();
  area->setParent(this);
  area->setReadOnly(true);
  main_layout->addWidget(area, row++, 0);

  close_button = new QPushButton(QString::fromUtf8(R__("Close")), this);
  connect(close_button, SIGNAL(pressed()), this, SLOT(close_now()));
  main_layout->addWidget(close_button, row++, 0);

  setLayout(main_layout);
  setWindowTitle(QString::fromUtf8(R__("Old ruleset to a new format")));

  setVisible(false);
}

/**********************************************************************//**
  Add entry
**************************************************************************/
void conversion_log::add(const char *msg)
{
  area->append(QString::fromUtf8(msg));
  setVisible(true);
}

/**********************************************************************//**
  User pushed close button
**************************************************************************/
void conversion_log::close_now()
{
  done(0);
}
