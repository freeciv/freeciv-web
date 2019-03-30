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

#ifndef FC__MPGUI_QT_WORKER_H
#define FC__MPGUI_QT_WORKER_H

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

// Qt
#include <QString>
#include <QThread>

// tools
#include "download.h"

class mpgui;
struct fcmp_params;

class mpqt_worker : public QThread
{
  Q_OBJECT

  public:
    void run();
    void download(QString URL_in, class mpgui *gui_in,
                  struct fcmp_params *fcmp_in,
                  dl_msg_callback msg_callback_in,
                  dl_pb_callback pb_callback_in);

  private:
    QString URL;
    class mpgui *gui;
    struct fcmp_params *fcmp;
    dl_msg_callback msg_callback;
    dl_pb_callback pb_callback;
};

#endif // FC__MPGUI_QT_WORKER_H
