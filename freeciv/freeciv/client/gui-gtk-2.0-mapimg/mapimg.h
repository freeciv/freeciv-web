/**********************************************************************
 Freeciv - Copyright (C) 2010 - The Freeciv Team
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

/****************************************************************************
  Map images:

  * The color struct and the functions color_new() and color_destroy() are
    used to define the colors for the player as well as for the terrain.

  * Basic functions:

    mapimg_init()       Initialise the map images.
    mapimg_reset()      Reset the map images.
    mapimg_free()       Free all memory needed for map images.
    mapimg_count()      Return the number of map image definitions.
    mapimg_error()      Return the last error message.
    mapimg_help()       Return a help text.

  * Advanced functions:

    mapimg_define()     Define a new map image.
    mapimg_isvalid()    Check if the map image is valid. this is only
                        possible  after the game is started or if it is a
                        savegame.
    mapimg_delete()     Delete a map image definition.
    mapimg_show()       Show the map image definition.
    mapimg_create()     Create all defined and valid map images.
    mapimg_colortest()  Create an image which shows all defined colors.
    mapimg_id2str()     Convert the map image definition to strings. Usefull
                        to save the definitions.

    These functions return TRUE on success and FALSE on error. In the later
    case the error message is available with mapimg_error().
****************************************************************************/

#ifndef FC__MAPIMG_H
#define FC__MAPIMG_H

/* common */
#include "player.h" /* struct player */

void mapimg_init(void);
void mapimg_reset(void);
void mapimg_free(void);
int mapimg_count(void);
const char *mapimg_help(void);
const char *mapimg_error(void);

bool mapimg_define(char *maparg, bool check);
bool mapimg_isvalid(int id);
bool mapimg_delete(int id);
bool mapimg_show(int id, const char **str_show, bool detail);
bool mapimg_create(int id, const char *basename, bool force);
bool mapimg_colortest(const char *basename);
bool mapimg_id2str(int id, const char **str_def);


#endif  /* FC__MAPIMG_H */
