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
#include <QApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

// utility
#include "fcintl.h"
#include "log.h"
#include "registry.h"

// modinst
#include "download.h"
#include "mpgui_qt.h"

#include "mpgui_qt_worker.h"

/**********************************************************************//**
  Run download thread
**************************************************************************/
void mpqt_worker::run()
{
  const char *errmsg;

  errmsg = download_modpack(URL.toUtf8().data(),
			    fcmp, msg_callback, pb_callback);

  if (errmsg != nullptr) {
    msg_callback(errmsg);
  } else {
    msg_callback(_("Ready"));
  }

  gui->refresh_list_versions_thr();
}

/**********************************************************************//**
  Start thread to download and install given modpack.
**************************************************************************/
void mpqt_worker::download(QString URL_in, class mpgui *gui_in,
                           struct fcmp_params *fcmp_in,
                           dl_msg_callback msg_callback_in,
                           dl_pb_callback pb_callback_in)
{
  URL = URL_in;
  gui = gui_in;
  fcmp = fcmp_in;
  msg_callback = msg_callback_in;
  pb_callback = pb_callback_in;

  start();
}
