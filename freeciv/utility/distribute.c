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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include "distribute.h"

/****************************************************************************
  Distribute "number" elements into "groups" groups with ratios given by
  the elements in "ratios".  The resulting division is put into the "result"
  array.

  For instance this code is used to distribute trade among science, tax, and
  luxury.  In this case "number" is the amount of trade, "groups" is 3,
  and ratios[3] = {sci_rate, tax_rate, lux_rate}.

  The algorithm used to determine the distribution is Hamilton's Method.
****************************************************************************/
void distribute(int number, int groups, int *ratios, int *result)
{
  int i, sum = 0, rest[groups], max_groups[groups], max_count, max;
#ifndef NDEBUG
  const int original_number = number;
#endif

  /* 
   * Distribution of a number of items into a number of groups with a given
   * ratio.  This follows a modified Hare/Niemeyer algorithm (also known
   * as "Hamilton's Method"):
   *
   * 1) distribute the whole-numbered part of the targets
   * 2) sort the remaining fractions (called rest[])
   * 3) divide the remaining source among the targets starting with the
   *    biggest fraction. (If two targets have the same fraction the
   *    target with the smaller whole-numbered value is chosen.  If two
   *    values are still equal it is the _first_ group which will be given
   *    the item.)
   */

  for (i = 0; i < groups; i++) {
    assert(ratios[i] >= 0);
    sum += ratios[i];
  }

  /* 1.  Distribute the whole-numbered part of the targets. */
  for (i = 0; i < groups; i++) {
    result[i] = number * ratios[i] / sum;
  }

  /* 2a.  Determine the remaining fractions. */
  for (i = 0; i < groups; i++) {
    rest[i] = number * ratios[i] - result[i] * sum;
  }

  /* 2b. Find how much source is left to be distributed. */
  for (i = 0; i < groups; i++) {
    number -= result[i];
  }

  while (number > 0) {
    max = max_count = 0;

    /* Find the largest remaining fraction(s). */
    for (i = 0; i < groups; i++) {
      if (rest[i] > max) {
	max_count = 1;
	max_groups[0] = i;
	max = rest[i];
      } else if (rest[i] == max) {
	max_groups[max_count] = i;
	max_count++;
      }
    }

    if (max_count == 1) {
      /* Give an extra source to the target with largest remainder. */
      result[max_groups[0]]++;
      rest[max_groups[0]] = 0;
      number--;
    } else {
      int min = result[max_groups[0]], which_min = max_groups[0];

      /* Give an extra source to the target with largest remainder and
       * smallest whole number. */
      assert(max_count > 1);
      for (i = 1; i < max_count; i++) {
	if (result[max_groups[i]] < min) {
	  min = result[max_groups[i]];
	  which_min = max_groups[i];
	}
      }
      result[which_min]++;
      rest[which_min] = 0;
      number--;
    }
  }

#ifndef NDEBUG
  number = original_number;
  for (i = 0; i < groups; i++) {
    assert(result[i] >= 0);
    number -= result[i];
  }
  assert(number == 0);
#endif
}

