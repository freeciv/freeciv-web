/**********************************************************************
 Freeciv - Copyright (C) 2005 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

/* utility */
#include "registry.h"

/* gui-sdl */
#include "graphics.h"
#include "themespec.h"

#include "themebackgrounds.h"

struct theme_background_system {
  char *backgrounds[BACKGROUND_LAST];
};

static char *background_names[] = {
  "changeresearchdlg",
  "choosegovernmentdlg",
  "citydlg",
  "citygovdlg",
  "cityrep",
  "connectdlg",
  "connlistdlg",
  "economydlg",
  "hlpdlg",
  "joingamedlg",
  "loadgamedlg",
  "mainpage",
  "messagewin",
  "nationdlg",
  "newcitydlg",
  "optiondlg",
  "revolutiondlg",
  "sciencedlg",
  "spystealdlg",
  "startmenu",
  "unitsrep",
  "userpasswddlg",
  "wldlg",
};

struct theme_background_system *theme_background_system_read(struct section_file *file)
{
  int i;
  struct theme_background_system *backgrounds = fc_malloc(sizeof(*backgrounds));

  assert(ARRAY_SIZE(background_names) == BACKGROUND_LAST);
  
  for (i = 0; i < BACKGROUND_LAST; i++) {
    backgrounds->backgrounds[i] = 
      themespec_gfx_filename(secfile_lookup_str(file, "backgrounds.%s", background_names[i]));
  }
  
  return backgrounds;
}

/****************************************************************************
  Called when the client first starts to free any allocated backgrounds.
****************************************************************************/
void theme_background_system_free(struct theme_background_system *backgrounds)
{
  int i;

  for (i = 0; i < BACKGROUND_LAST; i++) {
    FC_FREE(backgrounds->backgrounds[i]);
  }
  
  free(backgrounds);
}

/****************************************************************************
  Return a pointer to the given theme background.
****************************************************************************/
SDL_Surface *theme_get_background(const struct theme *t, enum theme_background background)
{
  return load_surf(theme_get_background_system(t)->backgrounds[background]);
}
