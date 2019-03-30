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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

/* utility */
#include "fcintl.h"
#include "log.h"
#include "support.h"

/* common */
#include "packets.h"
#include "version.h"

/* client */
#include "client_main.h"
#include "chatline.h"
#include "colors.h"
#include "connectdlg_common.h"
#include "dialogs.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "options.h"
#include "packhand.h"
#include "tilespec.h"

#include "connectdlg.h"


/**********************************************************************//**
  Close and destroy the dialog.
**************************************************************************/
void close_connection_dialog() 
{
}

/**********************************************************************//**
  gtk client does nothing here. This gets called when one is rejected
  from game.
**************************************************************************/
void server_connect(void)
{
}
