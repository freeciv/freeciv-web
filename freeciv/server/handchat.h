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
#ifndef FC__HANDCHAT_H
#define FC__HANDCHAT_H

#define ALLIESCHAT_COMMAND_PREFIX '.'

struct connection;

void handle_chat_msg_req(struct connection *pc, char *message);

#endif  /* FC__HANDCHAT_H */
