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

/**********************************************************************
                          optiondlg.h  -  description
                             -------------------
    begin                : Sun Aug 11 2002
    copyright            : (C) 2002 by Rafał Bursig
    email                : Rafał Bursig <bursig@poczta.fm>
 **********************************************************************/

#ifndef FC__OPTIONDLG_H
#define FC__OPTIONDLG_H

struct widget *pOptions_Button;

void init_options_button(void);
void enable_options_button(void);
void disable_options_button(void);

void popup_optiondlg(void);
void popdown_optiondlg(void);
int optiondlg_callback(struct widget *pButton);

#endif				/* FC__OPTIONDLG_H */
