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
#ifndef FC__CITYREPDATA_H
#define FC__CITYREPDATA_H

#include "shared.h"		/* bool type */

#include "fc_types.h"

/* Number of city report columns: have to set this manually now... */
#define NUM_CREPORT_COLS (num_city_report_spec())

struct city_report_spec {
  bool show;			/* modify this to customize */
  int width;			/* 0 means variable; rightmost only */
  int space;			/* number of leading spaces (see below) */
  const char *title1;		/* already translated or NULL */
  const char *title2;		/* already translated or NULL */
  const char *explanation;	/* already translated */ 
  void *data;
  const char *(*func)(const struct city * pcity, const void *data);
  const char *tagname;		/* for save_options */
};

extern struct city_report_spec *city_report_specs;

/* Use tagname rather than index for load/save, because later
   additions won't necessarily be at the end.
*/

/* Note on space: you can do spacing and alignment in various ways;
   you can avoid explicit space between columns if they are bracketted,
   but the problem is that with a configurable report you don't know
   what's going to be next to what.
   
   Here specify width, and leading space, although different clients
   may interpret these differently (gui-gtk and gui-mui ignore space
   field, handling columns without additional spacing).
   For some clients negative width means left justified (gui-gtk
   always treats width as negative; gui-mui ignores width field).
*/

/* Following are wanted to save/load options; use wrappers rather
   than expose the grotty details of the city_report_spec:
   (well, the details are exposed now too, but still keep
   this "clean" interface...)
*/
int num_city_report_spec(void);
bool *city_report_spec_show_ptr(int i);
const char *city_report_spec_tagname(int i);

void init_city_report_game_data(void);

int cityrepfield_compare(const char *field1, const char *field2);

#endif  /* FC__CITYREPDATA_H */
