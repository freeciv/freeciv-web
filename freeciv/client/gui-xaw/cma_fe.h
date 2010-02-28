/********************************************************************** 
 Freeciv - Copyright (C) 2003 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#ifndef FC__XAW_CMA_H
#define FC__XAW_CMA_H

#include <X11/Intrinsic.h>

#include "cma_core.h"

void show_cma_dialog(struct city *pcity, Widget citydlg);
void popdown_cma_dialog(void);

#endif   /* FC__XAW_CMA_H */
