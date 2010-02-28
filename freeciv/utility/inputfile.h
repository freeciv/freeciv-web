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

/********************************************************************** 
  A low-level object for reading a registry-format file.
  See comments in inputfile.c
***********************************************************************/

#ifndef FC__INPUTFILE_H
#define FC__INPUTFILE_H

#include "ioz.h"
#include "shared.h"		/* bool type */

struct inputfile;		/* opaque */

typedef char *(*datafilename_fn_t)(const char *filename);

struct inputfile *inf_from_file(const char *filename,
				datafilename_fn_t datafn);
struct inputfile *inf_from_stream(fz_FILE * stream,
				  datafilename_fn_t datafn);
void inf_close(struct inputfile *inf);
bool inf_at_eof(struct inputfile *inf);

enum inf_token_type {
  INF_TOK_SECTION_NAME,
  INF_TOK_ENTRY_NAME,
  INF_TOK_EOL,
  INF_TOK_TABLE_START,
  INF_TOK_TABLE_END,
  INF_TOK_COMMA,
  INF_TOK_VALUE,
  INF_TOK_LAST
};
#define INF_TOK_FIRST INF_TOK_SECTION_NAME

const char *inf_token(struct inputfile *inf, enum inf_token_type type);
const char *inf_token_required(struct inputfile *inf, enum inf_token_type type);
int inf_discard_tokens(struct inputfile *inf, enum inf_token_type type);

void inf_log(struct inputfile *inf, int loglevel, const char *message);

#endif  /* FC__INPUTFILE_H */
