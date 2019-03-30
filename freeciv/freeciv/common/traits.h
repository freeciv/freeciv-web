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
#ifndef FC__TRAITS_H
#define FC__TRAITS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define SPECENUM_NAME trait
#define SPECENUM_VALUE0 TRAIT_EXPANSIONIST
#define SPECENUM_VALUE0NAME "Expansionist"
#define SPECENUM_VALUE1 TRAIT_TRADER
#define SPECENUM_VALUE1NAME "Trader"
#define SPECENUM_VALUE2 TRAIT_AGGRESSIVE
#define SPECENUM_VALUE2NAME "Aggressive"
#define SPECENUM_COUNT TRAIT_COUNT
#include "specenum_gen.h"

#define TRAIT_DEFAULT_VALUE 50
#define TRAIT_MAX_VALUE (TRAIT_DEFAULT_VALUE * TRAIT_DEFAULT_VALUE)
#define TRAIT_MAX_VALUE_SR (TRAIT_DEFAULT_VALUE)

struct ai_trait
{
  int val;   /* Value assigned in the beginning */
  int mod;   /* This is modification that changes during game. */
};

struct trait_limits
{
  int min;
  int max;
  int fixed;
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__TRAITS_H */
