/**********************************************************************
 Freeciv - Copyright (C) 2004 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifndef FC__TEXT_RENDERER_H
#define FC__TEXT_RENDERER_H

#include "common_types.h"

struct tr_string_data;
struct osda;

void tr_init(void);
void tr_draw_string(struct osda *target,
		    const struct ct_point *position,
		    const struct ct_string *string);
void tr_string_get_size(struct ct_size *size, const struct ct_string *string);
void tr_prepare_string(struct ct_string *string);
void tr_free_string(struct ct_string *string);

#endif				/* FC__TEXT_RENDERER_H */
