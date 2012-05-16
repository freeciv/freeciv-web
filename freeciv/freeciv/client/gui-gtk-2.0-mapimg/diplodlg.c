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

#include "diplodlg.h"

/**************************************************************************
  Update a player's acceptance status of a treaty (traditionally shown
  with the thumbs-up/thumbs-down sprite).
**************************************************************************/
void handle_diplomacy_accept_treaty(int counterpart, bool I_accepted,
				    bool other_accepted)
{
  /* PORTME */
}

/**************************************************************************
  Handle the start of a diplomacy meeting - usually by poping up a
  diplomacy dialog.
**************************************************************************/
void handle_diplomacy_init_meeting(int counterpart, int initiated_from)
{
  /* PORTME */
}

/**************************************************************************
  Update the diplomacy dialog by adding a clause.
**************************************************************************/
void handle_diplomacy_create_clause(int counterpart, int giver,
				    enum clause_type type, int value)
{
  /* PORTME */
}

/**************************************************************************
  Update the diplomacy dialog when the meeting is canceled (the dialog
  should be closed).
**************************************************************************/
void handle_diplomacy_cancel_meeting(int counterpart, int initiated_from)
{
  /* PORTME */
}

/**************************************************************************
  Update the diplomacy dialog by removing a clause.
**************************************************************************/
void handle_diplomacy_remove_clause(int counterpart, int giver,
				    enum clause_type type, int value)
{
  /* PORTME */
}

/**************************************************************************
  Close all open diplomacy dialogs.

  Called when the client disconnects from game.
**************************************************************************/
void close_all_diplomacy_dialogs(void)
{
  /* PORTME */
}
