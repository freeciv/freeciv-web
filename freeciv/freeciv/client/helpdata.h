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
#ifndef FC__HELPDATA_H
#define FC__HELPDATA_H

#include <stddef.h>		/* size_t */

#include "improvement.h" 	/* Impr_type_id */

#include "helpdlg_g.h"		/* enum help_page_type */

struct help_item {
  char *topic, *text;
  enum help_page_type type;
};

void helpdata_init(void);
void helpdata_done(void);

void boot_help_texts(struct player *pplayer);
void free_help_texts(void);

int num_help_items(void);
const struct help_item *get_help_item(int pos);
const struct help_item *get_help_item_spec(const char *name,
					   enum help_page_type htype,
					   int *pos);
void help_iter_start(void);
const struct help_item *help_iter_next(void);

char *helptext_building(char *buf, size_t bufsz, struct player *pplayer,
			const char *user_text, struct impr_type *pimprove);
char *helptext_unit(char *buf, size_t bufsz, struct player *pplayer,
		    const char *user_text, struct unit_type *utype);
void helptext_advance(char *buf, size_t bufsz, struct player *pplayer,
		      const char *user_text, int i);
void helptext_terrain(char *buf, size_t bufsz, struct player *pplayer,
		      const char *user_text, struct terrain *pterrain);
void helptext_government(char *buf, size_t bufsz, struct player *pplayer,
			 const char *user_text, struct government *gov);

char *helptext_unit_upkeep_str(struct unit_type *punittype);

#define help_items_iterate(pitem) {       \
        const struct help_item *pitem;    \
        help_iter_start();                \
        while((pitem=help_iter_next())) {   
#define help_items_iterate_end }}

#endif  /* FC__HELPDATA_H */
