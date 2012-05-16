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
#ifndef FC__REPORT_H
#define FC__REPORT_H

#include "shared.h"		/* bool type */

struct connection;
struct conn_list;

void page_conn(struct conn_list *dest, const char *caption, const char *headline,
	       const char *lines);

void log_civ_score(void);
void make_history_report(void);
void report_wonders_of_the_world(struct conn_list *dest);
void report_top_five_cities(struct conn_list *dest);
bool is_valid_demography(const char *demography,
                         struct connection *caller,
                         const char **error_message);
void report_demographics(struct connection *pconn);
void report_final_scores(struct conn_list *dest);

#endif  /* FC__REPORT_H */
