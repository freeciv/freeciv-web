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


typedef void (*WorklistOkCallback) (struct worklist * pwl, void *data);
typedef void (*WorklistCancelCallback) (void *data);

struct worklist_window_init {
  struct worklist *pwl;
  struct city *pcity;
  HWND parent;
  void *user_data;
  WorklistOkCallback ok_cb;
  WorklistCancelCallback cancel_cb;
};

void popup_worklist(struct worklist_window_init *init);

void popup_worklists_report(void);
                                        /* The global worklist view */ 
void update_worklist_report_dialog(void);
void update_worklist_editor_win(HWND win);
LONG CALLBACK worklist_editor_proc(HWND hwnd,UINT message,WPARAM wParam,
				   LPARAM lParam);
#endif  /* FC__WLDLG_H */
