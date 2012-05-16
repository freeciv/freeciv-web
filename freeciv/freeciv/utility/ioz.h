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

#ifndef FC__IOZ_H
#define FC__IOZ_H

/********************************************************************** 
  An IO layer to support transparent compression/uncompression.
  (Currently only "required" functionality is supported.)
***********************************************************************/

#include <stdio.h>		/* FILE */

#include "shared.h"		/* fc__attribute */

struct fz_FILE_s;		  /* opaque */
typedef struct fz_FILE_s fz_FILE;

/* (possibly) supported methods (depending on config.h) */
enum fz_method { FZ_PLAIN = 0, FZ_ZLIB = 1, FZ_BZIP2 = 2, FZ_LAST };
#define FZ_NOT_USED FZ_LAST

fz_FILE *fz_from_file(const char *filename, const char *in_mode,
		      enum fz_method method, int compress_level);
fz_FILE *fz_from_stream(FILE *stream);
int fz_fclose(fz_FILE *fp);
char *fz_fgets(char *buffer, int size, fz_FILE *fp);
int fz_fprintf(fz_FILE *fp, const char *format, ...)
     fc__attribute((__format__ (__printf__, 2, 3)));

int fz_ferror(fz_FILE *fp);     
const char *fz_strerror(fz_FILE *fp);

#endif  /* FC__IOZ_H */
