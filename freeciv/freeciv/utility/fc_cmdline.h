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
#ifndef FC__CMDLINE_H
#define FC__CMDLINE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* utility */
#include "support.h"

char *get_option_malloc(const char *option_name,
                        char **argv, int *i, int argc,
                        bool gc);
bool is_option(const char *option_name,char *option);
int get_tokens(const char *str, char **tokens, size_t num_tokens,
               const char *delimiterset);
void free_tokens(char **tokens, size_t ntokens);

void cmdline_option_values_free(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__CMDLINE_H */
