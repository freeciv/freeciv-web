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
#ifndef FC__CAPSTR_H
#define FC__CAPSTR_H

#define NETWORK_CAPSTRING (NETWORK_CAPSTRING_MANDATORY " "	\
			   NETWORK_CAPSTRING_OPTIONAL)

extern const char * const our_capability;

void init_our_capability(void);

#endif
