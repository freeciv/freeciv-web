/***********************************************************************
 Freeciv - Copyright (C) 1996-2018 - Freeciv Development Team
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#ifndef FC__UNITSELEXTRADLG_H
#define FC__UNITSELEXTRADLG_H

bool select_tgt_extra(struct unit *actor, struct tile *ptile,
                      bv_extras potential_tgt_extras,
                      struct extra_type *suggested_tgt_extra,
                      const gchar *dlg_title,
                      const gchar *actor_label,
                      const gchar *tgt_label,
                      const gchar *do_label,
                      GCallback do_callback);

#endif  /* FC__UNITSELEXTRADLG_H */
