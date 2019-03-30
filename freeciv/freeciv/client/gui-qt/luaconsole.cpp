/*****************************************************************************
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*****************************************************************************/
#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

// Qt
#include <QFileDialog>
#include <QString>

// utility
#include "shared.h"

// common
#include "featured_text.h"

/* client/luascript */
#include "script_client.h"

// gui-qt
#include "fc_client.h"
#include "luaconsole.h"
#include "qtg_cxxside.h"

QString qlua_filename;

/*************************************************************************//**
  Popup the lua console inside the main-window, and optionally raise it.
*****************************************************************************/
void luaconsole_dialog_popup(bool raise)
{
  /* lua output is in chat */
}

/*************************************************************************//**
  Return true if the lua console is open.
*****************************************************************************/
bool luaconsole_dialog_is_open(void)
{
  return true;
}

/*************************************************************************//**
  Update the lua console.
*****************************************************************************/
void real_luaconsole_dialog_update(void)
{
}

/*************************************************************************//**
  Appends the string to the chat output window.  The string should be
  inserted on its own line, although it will have no newline.
*****************************************************************************/
void real_luaconsole_append(const char *astring,
                            const struct text_tag_list *tags)
{
  qtg_real_output_window_append(astring, tags, 0);
}

/*************************************************************************//**
  Load and execute lua script
*****************************************************************************/
void qload_lua_script()
{
  QString str;
  str = QString(_("Lua scripts")) + QString(" (*.lua)");
  qlua_filename = QFileDialog::getOpenFileName(gui()->central_wdg,
                                              _("Load lua script"),
                                              QDir::homePath(), str);
  if (qlua_filename.isEmpty() == false) {
    script_client_do_file(qlua_filename.toLocal8Bit().constData());
  }
}

/*************************************************************************//**
  Reload last lua script
*****************************************************************************/
void qreload_lua_script()
{
  if (qlua_filename.isEmpty() == false) {
    script_client_do_file(qlua_filename.toLocal8Bit().constData());
  }
}
