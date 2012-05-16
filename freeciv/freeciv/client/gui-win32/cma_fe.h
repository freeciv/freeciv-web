/**********************************************************************
 Freeciv - Copyright (C) 2004 A. Kemnade
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#ifndef FC__CMA_FE_H
#define FC__CMA_FE_H
LONG CALLBACK cma_proc(HWND win, UINT message,
		       WPARAM wParam, LPARAM lParam);
struct cma_dialog;

struct cma_gui_initdata {
  struct city *pcity;
  struct cma_dialog *pdialog;
};

enum cma_refresh {
  REFRESH_ALL,
  DONT_REFRESH_SELECT,
  DONT_REFRESH_HSCALES
};

void refresh_cma_dialog(struct city *pcity, enum cma_refresh refresh);
struct cma_dialog *get_cma_dialog(struct city *pcity);
#endif
