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
#ifndef FC__WLDLG_H
#define FC__WLDLG_H

#include <X11/Intrinsic.h>

#include "fc_types.h"

typedef void (*WorklistOkCallback) (struct worklist *pwl, void *data);
typedef void (*WorklistCancelCallback) (void *data);

void popup_worklists_dialog(struct player *pplay); 
                                        /* The global worklist view */

Widget popup_worklist(struct worklist *pwl,struct city *pcity,
		      Widget parent, void *parent_data,
		      WorklistOkCallback ok_cb,
		      WorklistCancelCallback cancel_cb);
                                        /* An individual worklist */

void update_worklist_report_dialog(void);

#endif  /* FC__WLDLG_H */
