/**********************************************************************
 Freeciv - Copyright (C) 2005 - The Freeciv Project
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

#include <math.h>

/* utilities */
#include "log.h"
#include "rand.h"

#include "api_utilities.h"

/************************************************************************
  Generate random number.
************************************************************************/
int api_utilities_random(int min, int max)
{
  double roll = (double)(myrand(MAX_UINT32) % MAX_UINT32) / (double)MAX_UINT32;

  return (min + floor(roll * (max - min + 1)));
}

/************************************************************************
  Error message from script to log
************************************************************************/
void api_utilities_error_log(const char *msg)
{
  freelog(LOG_ERROR, "%s", msg);
}

/************************************************************************
  Debug message from script to log
************************************************************************/
void api_utilities_debug_log(const char *msg)
{
  freelog(LOG_DEBUG, "%s", msg);
}
