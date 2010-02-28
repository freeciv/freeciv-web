
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
#ifndef FC__NETSAVE_H
#define FC__NETSAVE_H

#include "ioz.h"
#include "shared.h"		/* bool type and fc__attribute */
#include "registry.h"


bool net_file_save(struct section_file *my_section_file,
                       const char *filename);

bool net_file_load(struct section_file *my_section_file,
			           const char *filename);

#endif  /* FC__NETSAVE_H */
