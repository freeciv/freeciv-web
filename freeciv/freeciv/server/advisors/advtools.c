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

#include <math.h>

#include "advtools.h"

/**********************************************************************//**
  Amortize means gradually paying off a cost or debt over time. In freeciv
  terms this means we calculate how much less worth something is to us
  depending on how long it will take to complete.

  This is based on a global interest rate as defined by the MORT value.
**************************************************************************/
adv_want amortize(adv_want benefit, int delay)
{
  double discount = 1.0 - 1.0 / ((double)MORT);

  /* Note there's no rounding here.  We could round but it would probably
   * be better just to return (and take) a double for the benefit. */
  return benefit * pow(discount, delay);
}
