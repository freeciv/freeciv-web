/********************************************************************** 
 Freeciv - Copyright (C) 2005-2007 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifndef FC__REQTREE_H
#define FC__REQTREE_H

#include "canvas_g.h"

/* Requirements Tree
 *
 * This file provides functions for drawing a tree-like graph of
 * requirements.  This can be used for creating an interactive diagram
 * showing the dependencies of various sources.
 *
 * A tree must first be created with create_reqtree; this will do all of the
 * calculations needed for the tree and is a fairly expensive operation.
 * After creating the tree, the other functions may be used to access or
 * draw it.
 *
 * Currently only techs are supported (as sources and requirements).
 */

struct reqtree;

struct reqtree *create_reqtree(struct player *pplayer);
void destroy_reqtree(struct reqtree *tree);

void get_reqtree_dimensions(struct reqtree *tree,
			    int *width, int *height);

void draw_reqtree(struct reqtree *tree,
		  struct canvas *pcanvas,
		  int canvas_x, int canvas_y,
		  int tt_x, int tt_y, int w, int h);

Tech_type_id get_tech_on_reqtree(struct reqtree *tree, int x, int y);
bool find_tech_on_reqtree(struct reqtree *tree, Tech_type_id tech,
			  int *x, int *y, int *w, int *h);

#endif
