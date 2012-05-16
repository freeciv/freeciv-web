/**********************************************************************
 Freeciv - Copyright (C) 2006 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifndef FC__THEMEBACKGROUNDS_H
#define FC__THEMEBACKGROUNDS_H

#include "SDL.h"

#include "registry.h"

enum theme_background {
  BACKGROUND_CHANGERESEARCHDLG,
  BACKGROUND_CHOOSEGOVERNMENTDLG,
  BACKGROUND_CITYDLG,
  BACKGROUND_CITYGOVDLG,
  BACKGROUND_CITYREP,
  BACKGROUND_CONNECTDLG,
  BACKGROUND_CONNLISTDLG,
  BACKGROUND_ECONOMYDLG,
  BACKGROUND_HELPDLG,
  BACKGROUND_JOINGAMEDLG,
  BACKGROUND_LOADGAMEDLG,
  BACKGROUND_MAINPAGE,
  BACKGROUND_MESSAGEWIN,
  BACKGROUND_NATIONDLG,
  BACKGROUND_NEWCITYDLG,
  BACKGROUND_OPTIONDLG,
  BACKGROUND_REVOLUTIONDLG,
  BACKGROUND_SCIENCEDLG,
  BACKGROUND_SPYSTEALDLG,
  BACKGROUND_STARTMENU,
  BACKGROUND_UNITSREP,
  BACKGROUND_USERPASSWDDLG,
  BACKGROUND_WLDLG,
  
  BACKGROUND_LAST
};

struct theme_background_system;
struct theme;

SDL_Surface *theme_get_background(const struct theme *t, enum theme_background background);
        
/* Functions used by the theme to allocate the background system. */
struct theme_background_system *theme_background_system_read(struct section_file *file);

void theme_background_system_free(struct theme_background_system *backgrounds);

#endif /* FC__THEMEBACKGROUNDS_H */
