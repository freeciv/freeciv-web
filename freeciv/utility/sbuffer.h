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
#ifndef FC__SBUFFER_H
#define FC__SBUFFER_H

#include <stddef.h>		/* size_t */

struct sbuffer;			/* opaque type */

/* get a new initialized sbuffer: (uses default buffer size) */
struct sbuffer *sbuf_new(void);

/* malloc and strdup: */
void *sbuf_malloc(struct sbuffer *sb, size_t size);
char *sbuf_strdup(struct sbuffer *sb, const char *str);

/* free all memory associated with sb; after this sb itself points
   to deallocated memory, so should not be used */
void sbuf_free(struct sbuffer *sb);

#endif /* FC__SBUFFER_H */
