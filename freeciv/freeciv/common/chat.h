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
#ifndef FC__CHAT_H
#define FC__CHAT_H

/* Definitions related to interpreting chat messages.
 * Behaviour generally can't be changed at whim because client and
 * server are assumed to agree on these details. */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Special characters in chatlines. */

/* The character to mark chatlines as server commands */
/* FIXME this is still hard-coded in a lot of places */
#define SERVER_COMMAND_PREFIX '/'
#define CHAT_ALLIES_PREFIX '.'
#define CHAT_DIRECT_PREFIX ':'

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__CHAT_H */
