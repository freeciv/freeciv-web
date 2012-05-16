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
#ifndef FC__GUI_MAIN_H
#define FC__GUI_MAIN_H

#define ID_TURNDONE 26
#define ID_ESCAPE 27

#define ID_MAPWINDOW 28
#define ID_OUTPUTWINDOW 29
#define ID_MAPHSCROLL 30
#define ID_MAPVSCROLL 31



#define ID_CONNECTDLG_NAME 129
#define ID_CONNECTDLG_HOST 130
#define ID_CONNECTDLG_PORT 131
#define ID_CONNECTDLG_CONNECT 132
#define ID_CONNECTDLG_QUIT 133
#define ID_CONNECTDLG_BACK 134
#define ID_STARTDLG_NEWGAME 135
#define ID_STARTDLG_LOADGAME 136
#define ID_STARTDLG_CONNECTGAME 137

#define ID_RACESDLG_NATION 144
#define ID_RACESDLG_LEADER 145
#define ID_RACESDLG_MALE 146
#define ID_RACESDLG_FEMALE 147
#define ID_RACESDLG_EURO 148
#define ID_RACESDLG_CLASSIC 149
#define ID_RACESDLG_OK 152
#define ID_RACESDLG_DISCONNECT 151
#define ID_RACESDLG_QUIT 150


#define ID_PRODCHANGE_CHANGE 167
#define ID_PRODCHANGE_CANCEL 172
#define ID_PRODCHANGE_HELP 173
#define ID_PRODCHANGE_LIST 174
#define ID_PRODCHANGE_FROM 175
#define ID_PRODCHANGE_TO 166

#define ID_INPUT_HEADLINE 168
#define ID_INPUT_OK 169
#define ID_INPUT_CANCEL 170
#define ID_INPUT_TEXT 171

#define ID_SCIENCE_TOP 176
#define ID_SCIENCE_RESEARCH 177
#define ID_SCIENCE_PROG 178
#define ID_SCIENCE_HELP 179
#define ID_SCIENCE_GOAL 180
#define ID_SCIENCE_STEPS 181
#define ID_SCIENCE_LIST 182
#define ID_SCIENCE_CLOSE 183

#define ID_RATES_TAX 192
#define ID_RATES_LUXURY 193
#define ID_RATES_SCIENCE 194
#define ID_RATES_OK 195
#define ID_RATES_CANCEL 196
#define ID_RATES_MAX 197
#define ID_RATES_TAXLOCK 198
#define ID_RATES_LUXURYLOCK 199
#define ID_RATES_SCIENCELOCK 200


#define ID_FINDCITY_LIST 208
#define ID_FINDCITY_CENTER 209
#define ID_FINDCITY_CANCEL 210

#define ID_MILITARY_INPROG 224
#define ID_MILITARY_ACTIVE 225
#define ID_MILITARY_SHIELD 226
#define ID_MILITARY_FOOD 227
#define ID_MILITARY_CLOSE 228
#define ID_MILITARY_UPGRADE 229
#define ID_MILITARY_REFRESH 230
#define ID_MILITARY_TOP 231
#define ID_MILITARY_LIST 232

#define ID_CITYREP_TOP 240
#define ID_CITYREP_NAME 241
#define ID_CITYREP_STATE 242
#define ID_CITYREP_WORKERS 243
#define ID_CITYREP_SURPLUS 244
#define ID_CITYREP_ECONOMY 245
#define ID_CITYREP_FOOD 246
#define ID_CITYREP_LIST 247
#define ID_CITYREP_BUILDING 248
#define ID_CITYREP_CLOSE 249
#define ID_CITYREP_CENTER 250
#define ID_CITYREP_POPUP 251
#define ID_CITYREP_BUY 252
#define ID_CITYREP_CHANGE 253
#define ID_CITYREP_CHANGEALL 254
#define ID_CITYREP_REFRESH 255
#define ID_CITYREP_SELECT 256
#define ID_CITYREP_CONFIG 257

#define ID_NOTIFY_CLOSE 272
#define ID_NOTIFY_MSG 273
#define ID_NOTIFY_HEADLINE 274

#define ID_TRADEREP_TOP 288
#define ID_TRADEREP_LIST 289
#define ID_TRADEREP_CASH 290
#define ID_TRADEREP_CLOSE 291
#define ID_TRADEREP_OBSOLETE 292
#define ID_TRADEREP_ALL 293

#ifndef RC_INVOKED
#include "gui_main_g.h"
extern HFONT main_font;
extern HFONT city_descriptions_font;
extern HFONT font_8courier;
extern HFONT font_12courier;
extern HFONT font_12arial;
extern int map_win_x;
extern int map_win_y;
extern HWND root_window;
extern void do_mainwin_layout(void);
extern int overview_win_x;
extern int overview_win_y;
extern int overview_win_width;
extern int overview_win_height;
extern int indicator_y;
extern HWND infolabel_win;
extern int taxinfoline_y; 
extern HWND logoutput_win;
extern HWND turndone_button;
extern HINSTANCE freecivhinst;
extern HWND unit_info_frame;
extern HWND unit_info_label;
extern HWND timeout_label;
extern HWND hchatline;
extern HWND map_window;
extern HWND map_scroll_h;
extern HWND map_scroll_v;
extern struct fcwin_box *output_box;

void set_overview_win_dim(int w, int h);

#endif  /* RC_INVOKED */
#endif  /* FC__GUI_MAIN_H */
