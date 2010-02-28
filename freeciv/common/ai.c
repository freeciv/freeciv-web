/****************************************************************************
 Freeciv - Copyright (C) 2004 - The Freeciv Team
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
****************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

/* utility */
#include "fcintl.h"
#include "log.h"

/* common */
#include "ai.h"

static struct ai_type default_ai;

/***************************************************************
  Returns ai_type of given id. Currently only one ai_type,
  id AI_DEFAULT, is supported.
***************************************************************/
struct ai_type *get_ai_type(int id)
{
  assert(id == AI_DEFAULT);

  return &default_ai;
}

/***************************************************************
  Initializes AI structure.
***************************************************************/
void init_ai(struct ai_type *ai)
{
  memset(ai, 0, sizeof(*ai));
}
