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

#include "cityrep.h"

/**********************************************************************//**
  Display the city report dialog.  Optionally raise it.
**************************************************************************/
void city_report_dialog_popup(bool raise)
{
  /* PORTME */
}

/**********************************************************************//**
  Update (refresh) the entire city report dialog.
**************************************************************************/
void real_city_report_dialog_update(void *unused)
{
  /* PORTME */
}

/**********************************************************************//**
  Update the information for a single city in the city report.
**************************************************************************/
void real_city_report_update_city(struct city *pcity)
{
  /* PORTME */
}

/**********************************************************************//**
  After a selection rectangle is defined, make the cities that
  are hilited on the canvas exclusively hilited in the
  City List window.
**************************************************************************/
void hilite_cities_from_canvas(void)
{
  /* PORTME */
}

/**********************************************************************//**
  Toggle a city's hilited status.
**************************************************************************/
void toggle_city_hilite(struct city *pcity, bool on_off)
{
  /* PORTME */
}
