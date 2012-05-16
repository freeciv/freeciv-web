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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>

#include "cityrep.h"
#include "repodlgs.h"
#include "repodlgs_common.h"

/**************************************************************************
  Update all report dialogs.
**************************************************************************/
void update_report_dialogs(void)
{
  if (!is_report_dialogs_frozen()) {
    activeunits_report_dialog_update();
    economy_report_dialog_update();
    city_report_dialog_update(); 
    science_dialog_update();
  }
}

/**************************************************************************
  Update the science report.
**************************************************************************/
void science_dialog_update(void)
{
  /* PORTME */
}

/**************************************************************************
  Popup (or raise) the science report(F6).  It may or may not be modal.
**************************************************************************/
void popup_science_dialog(bool make_modal)
{
  /* PORTME */
}

/**************************************************************************
  Update the economy report.
**************************************************************************/
void economy_report_dialog_update(void)
{
  /* PORTME */
}

/**************************************************************************
  Popup (or raise) the economy report (F5).  It may or may not be modal.
**************************************************************************/
void popup_economy_report_dialog(bool make_modal)
{
  /* PORTME */
}

/**************************************************************************
  Update the units report.
**************************************************************************/
void activeunits_report_dialog_update(void)
{
  /* PORTME */
}

/**************************************************************************
  Popup (or raise) the units report (F2).  It may or may not be modal.
**************************************************************************/
void popup_activeunits_report_dialog(bool make_modal)
{
  /* PORTME */
}

void popup_endgame_report_dialog(struct packet_endgame_report *packet){}
void popup_settable_options_dialog(void){}
