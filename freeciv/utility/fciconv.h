/********************************************************************** 
 Freeciv - Copyright (C) 2003-2004 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#ifndef FC__FCICONV_H
#define FC__FCICONV_H

#include <stdio.h>

#include "shared.h"

#define FC_DEFAULT_DATA_ENCODING "UTF-8"

void init_character_encodings(const char *internal_encoding,
			      bool use_transliteration);

const char *get_data_encoding(void);
const char *get_local_encoding(void);
const char *get_internal_encoding(void);

char *data_to_internal_string_malloc(const char *text);
char *internal_to_data_string_malloc(const char *text);
char *internal_to_local_string_malloc(const char *text);
char *local_to_internal_string_malloc(const char *text);

char *local_to_internal_string_buffer(const char *text,
				      char *buf, size_t bufsz);

#define fc_printf(...) fc_fprintf(stdout, __VA_ARGS__)
void fc_fprintf(FILE *stream, const char *format, ...)
      fc__attribute((__format__ (__printf__, 2, 3)));

char *convert_string(const char *text,
		     const char *from,
		     const char *to,
		     char *buf, size_t bufsz);

size_t get_internal_string_length(const char *text);

#endif /* FC__FCICONV_H */
