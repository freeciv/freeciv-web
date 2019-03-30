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

/* utility */
#include "shared.h"

/* common */
#include "featured_text.h"

#include "luaconsole.h"

/*************************************************************************//**
  Popup the lua console inside the main-window, and optionally raise it.
*****************************************************************************/
void luaconsole_dialog_popup(bool raise)
{
  /* PORTME */
}

/*************************************************************************//**
  Return TRUE iff the lua console is open.
*****************************************************************************/
bool luaconsole_dialog_is_open(void)
{
  /* PORTME */

  return FALSE;
}

/*************************************************************************//**
  Update the lua console.
*****************************************************************************/
void real_luaconsole_dialog_update(void)
{
  /* PORTME */
}

/*************************************************************************//**
  Appends the string to the chat output window.  The string should be
  inserted on its own line, although it will have no newline.
*****************************************************************************/
void real_luaconsole_append(const char *astring,
                            const struct text_tag_list *tags)
{
  /* PORTME */
}
