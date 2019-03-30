/*****************************************************************************
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*****************************************************************************/
#ifndef FC__FC_CMDHELP_H
#define FC__FC_CMDHELP_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct cmdhelp;

struct cmdhelp *cmdhelp_new(const char *cmdname);
void cmdhelp_destroy(struct cmdhelp *pcmdhelp);
void cmdhelp_add(struct cmdhelp *pcmdhelp, const char *shortarg,
                 const char *longarg, const char *helpstr, ...)
                 fc__attribute((__format__(__printf__, 4, 5)));
void cmdhelp_display(struct cmdhelp *pcmdhelp, bool sort, bool gui_options,
                     bool report_bugs);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__FC_CMDHELP_H */
