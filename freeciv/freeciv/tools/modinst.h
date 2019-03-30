/***********************************************************************
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
#ifndef FC__MODINST_H
#define FC__MODINST_H

struct fcmp_params
{
  const char *list_url;
  const char *inst_prefix;
  const char *autoinstall;
};

#if IS_DEVEL_VERSION && ! IS_FREEZE_VERSION
#ifndef MODPACK_LIST_URL
#define MODPACK_LIST_URL "http://files.freeciv.org/modinst/" DATASUBDIR "/modpack.list"
#endif
#define DEFAULT_URL_START "http://files.freeciv.org/modinst/" DATASUBDIR "/"
#else  /* IS_DEVEL_VERSION */
#ifndef MODPACK_LIST_URL
#define MODPACK_LIST_URL  "http://modpack.freeciv.org/" DATASUBDIR "/modpack.list"
#endif
#define DEFAULT_URL_START "http://modpack.freeciv.org/" DATASUBDIR "/"
#endif /* IS_DEVEL_VERSION */

#define EXAMPLE_URL DEFAULT_URL_START "ancients.modpack"

#define SPECENUM_NAME modpack_type
#define SPECENUM_VALUE0 MPT_RULESET
#define SPECENUM_VALUE0NAME N_("Ruleset")
#define SPECENUM_VALUE1 MPT_TILESET
#define SPECENUM_VALUE1NAME N_("Tileset")
#define SPECENUM_VALUE2 MPT_MODPACK
#define SPECENUM_VALUE2NAME N_("Modpack")
#define SPECENUM_VALUE3 MPT_SCENARIO
#define SPECENUM_VALUE3NAME N_("Scenario")
#define SPECENUM_VALUE4 MPT_SOUNDSET
#define SPECENUM_VALUE4NAME N_("Soundset")
#define SPECENUM_VALUE5 MPT_MUSICSET
#define SPECENUM_VALUE5NAME N_("Musicset")
#define SPECENUM_VALUE6 MPT_MODPACK_GROUP
#define SPECENUM_VALUE6NAME N_("Group")
#include "specenum_gen.h"

void fcmp_init(void);
void fcmp_deinit(void);

void load_install_info_lists(struct fcmp_params *fcmp);

#endif /* FC__MODINST_H */
