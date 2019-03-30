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
#ifndef FC__MODPACK_DOWNLOAD_H
#define FC__MODPACK_DOWNLOAD_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* modinst */
#include "modinst.h"

#define MODPACKDL_SUFFIX ".mpdl"

#define MODPACK_CAPSTR "+Freeciv-modpack-Devel-2016.Feb.05"
#define MODLIST_CAPSTR "+Freeciv-modlist-Devel-2016.Feb.05"

#define FCMP_CONTROLD ".control"

typedef void (*dl_msg_callback)(const char *msg);
typedef void (*dl_pb_callback)(int downloaded, int max);

const char *download_modpack(const char *URL,
			     const struct fcmp_params *fcmp,
                             dl_msg_callback mcb,
                             dl_pb_callback pbcb);

typedef void (*modpack_list_setup_cb)(const char *name, const char *URL,
                                      const char *version,
                                      const char *license,
                                      enum modpack_type type,
                                      const char *subtype,
                                      const char *notes);

const char *download_modpack_list(const struct fcmp_params *fcmp,
                                  modpack_list_setup_cb cb,
                                  dl_msg_callback mcb);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FC__MODPACK_DOWNLOAD_H */
