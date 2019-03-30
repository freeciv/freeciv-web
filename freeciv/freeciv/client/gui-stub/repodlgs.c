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

#include <stdlib.h>

/* gui main header */
#include "gui_stub.h"

/* client */
#include "repodlgs_common.h"

#include "cityrep.h"

#include "repodlgs.h"

/**********************************************************************//**
  Update the science report.
**************************************************************************/
void real_science_report_dialog_update(void *unused)
{
  /* PORTME */
}

/**********************************************************************//**
  Display the science report.  Optionally raise it.
  Typically triggered by F6.
**************************************************************************/
void science_report_dialog_popup(bool raise)
{
  /* PORTME */
}

/**********************************************************************//**
  Update the economy report.
**************************************************************************/
void real_economy_report_dialog_update(void *unused)
{
  /* PORTME */
}

/**********************************************************************//**
  Display the economy report.  Optionally raise it.
  Typically triggered by F5.
**************************************************************************/
void economy_report_dialog_popup(bool raise)
{
  /* PORTME */
}

/**********************************************************************//**
  Update the units report.
**************************************************************************/
void real_units_report_dialog_update(void *unused)
{
  /* PORTME */
}

/**********************************************************************//**
  Display the units report.  Optionally raise it.
  Typically triggered by F2.
**************************************************************************/
void units_report_dialog_popup(bool raise)
{
  /* PORTME */
}

/**********************************************************************//**
  Show a dialog with player statistics at endgame.
**************************************************************************/
void endgame_report_dialog_start(const struct packet_endgame_report *packet)
{
  /* PORTME */
}

/**********************************************************************//**
  Received endgame report information about single player.
**************************************************************************/
void endgame_report_dialog_player(const struct packet_endgame_player *packet)
{
  /* PORTME */
}

/**********************************************************************//**
  Resize and redraw the requirement tree.
**************************************************************************/
void science_report_dialog_redraw(void)
{
  /* PORTME */
}
