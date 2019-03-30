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
#ifndef FC__RAND_H
#define FC__RAND_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>

/* Utility */
#include "log.h"
#include "support.h"            /* bool type */

typedef uint32_t RANDOM_TYPE;

typedef struct {
  RANDOM_TYPE v[56];
  int j, k, x;
  bool is_init;			/* initially 0 for static storage */
} RANDOM_STATE;

#define fc_rand(_size) \
  fc_rand_debug((_size), "fc_rand", __FC_LINE__, __FILE__)

RANDOM_TYPE fc_rand_debug(RANDOM_TYPE size, const char *called_as,
                          int line, const char *file);

void fc_srand(RANDOM_TYPE seed);

void fc_rand_uninit(void);
bool fc_rand_is_init(void);
RANDOM_STATE fc_rand_state(void);
void fc_rand_set_state(RANDOM_STATE state);

void test_random1(int n);

/*===*/

#define fc_randomly(_seed, _size) \
  fc_randomly_debug((_seed), (_size), "fc_randomly", __FC_LINE__, __FILE__)

RANDOM_TYPE fc_randomly_debug(RANDOM_TYPE seed, RANDOM_TYPE size,
                              const char *called_as,
                              int line, const char *file);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__RAND_H */
