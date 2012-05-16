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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <windows.h>
#include <windowsx.h>

#include "city.h"
#include "citydlg_common.h"
#include "fcintl.h"
#include "game.h"
#include "government.h"
#include "mem.h"
#include "support.h"

#include "text.h"
#include "tilespec.h"

#include "gui_main.h"
#include "gui_stuff.h"
#include "happiness.h"
#include "sprite.h"

#define HAPPINESS_PIX_WIDTH 23

#define NUM_HAPPINESS_MODIFIERS FEELING_LAST
enum { CITIES, LUXURIES, BUILDINGS, UNITS, WONDERS };

struct happiness_dlg {
  struct city *pcity;
  HWND win;
  HBITMAP mod_bmp[NUM_HAPPINESS_MODIFIERS];
  HWND mod_label[NUM_HAPPINESS_MODIFIERS];
  POINT mod_bmp_pos[NUM_HAPPINESS_MODIFIERS];
};

static void refresh_happiness_bitmap(HBITMAP bmp,
				     struct city *pcity,
				     enum citizen_feeling index);

/**************************************************************************
...
**************************************************************************/
static void bmp_row_minsize(POINT *minsize, void *data)
{
  minsize->x=HAPPINESS_PIX_WIDTH*tileset_small_sprite_width(tileset);
  minsize->y=tileset_small_sprite_height(tileset);
}

/**************************************************************************
...
**************************************************************************/
static void bmp_row_setsize(RECT *size, void *data)
{
  POINT *pt;
  pt=(POINT *)data;
  pt->x=size->left;
  pt->y=size->top;
}

/**************************************************************************
...
**************************************************************************/
struct happiness_dlg *create_happiness_box(struct city *pcity,
					   struct fcwin_box *hbox,
					   HWND win)
{
  int i;
  HDC hdc;
  struct happiness_dlg *dlg;
  struct fcwin_box *vbox;
  vbox=fcwin_vbox_new(win,FALSE);
  fcwin_box_add_groupbox(hbox,_("Happiness"),vbox,0,
			 TRUE,TRUE,0);
  dlg=fc_malloc(sizeof(struct happiness_dlg));
  dlg->win=win;
  dlg->pcity=pcity;
  for(i=0;i<NUM_HAPPINESS_MODIFIERS;i++) {
    fcwin_box_add_generic(vbox,bmp_row_minsize,bmp_row_setsize,NULL,
			  &(dlg->mod_bmp_pos[i]),FALSE,FALSE,0);
    dlg->mod_label[i]=fcwin_box_add_static(vbox," ",0,SS_LEFT,
					   FALSE,FALSE,5);
  }
  hdc=GetDC(win);
  for(i=0;i<NUM_HAPPINESS_MODIFIERS;i++) {
    dlg->mod_bmp[i]=
      CreateCompatibleBitmap(hdc,
			     HAPPINESS_PIX_WIDTH*tileset_small_sprite_width(tileset),
			     tileset_small_sprite_height(tileset));
  }
  ReleaseDC(win,hdc);
  return dlg;
}

/****************************************************************************

****************************************************************************/
void cleanup_happiness_box(struct happiness_dlg *dlg)
{
  int i;
  for(i=0;i<NUM_HAPPINESS_MODIFIERS;i++) {
    DeleteObject(dlg->mod_bmp[i]);
  }
  free(dlg);
}

/****************************************************************************

****************************************************************************/
void repaint_happiness_box(struct happiness_dlg *dlg, HDC hdc)
{
  int i;
  HDC hdcsrc;
  HBITMAP old;
  hdcsrc=CreateCompatibleDC(hdc);
  for(i=0;i<NUM_HAPPINESS_MODIFIERS;i++) {
    old=SelectObject(hdcsrc,dlg->mod_bmp[i]);
    BitBlt(hdc,dlg->mod_bmp_pos[i].x,dlg->mod_bmp_pos[i].y,
	   HAPPINESS_PIX_WIDTH*tileset_small_sprite_width(tileset),
	   tileset_small_sprite_height(tileset),hdcsrc,0,0,SRCCOPY);
    SelectObject(hdcsrc,old);
  }
  DeleteDC(hdcsrc);
}

/**************************************************************************
...
**************************************************************************/
static void refresh_happiness_bitmap(HBITMAP bmp,
				     struct city *pcity,
				     enum citizen_feeling index)
{
  enum citizen_category citizens[MAX_CITY_SIZE];
  RECT rc;
  int i;
  int num_citizens = get_city_citizen_types(pcity, index, citizens);
  int pix_width = HAPPINESS_PIX_WIDTH * tileset_small_sprite_width(tileset);
  int offset = MIN(tileset_small_sprite_width(tileset), pix_width / num_citizens);
  /* int true_pix_width = (num_citizens - 1) * offset + tileset_small_sprite_width(tileset); */
  HDC hdc = CreateCompatibleDC(NULL);
  HBITMAP old=SelectObject(hdc,bmp);

  rc.left=0;
  rc.top=0;
  rc.right=pix_width;
  rc.bottom=tileset_small_sprite_height(tileset);
  FillRect(hdc,&rc,(HBRUSH)GetClassLong(root_window,GCL_HBRBACKGROUND));

  for (i = 0; i < num_citizens; i++) {
    draw_sprite(get_citizen_sprite(tileset, citizens[i], i, pcity),
		hdc, i * offset, 0);
  }

  SelectObject(hdc,old);
  DeleteDC(hdc);
}

/****************************************************************
...
*****************************************************************/
void refresh_happiness_box(struct happiness_dlg *dlg)
{
  HDC hdc;
  int i;
  for(i=0;i<NUM_HAPPINESS_MODIFIERS;i++) {
    refresh_happiness_bitmap(dlg->mod_bmp[i],dlg->pcity,i);
  }
  hdc=GetDC(dlg->win);
  repaint_happiness_box(dlg,hdc);
  ReleaseDC(dlg->win,hdc);

  SetWindowText(dlg->mod_label[CITIES],
		text_happiness_cities(dlg->pcity));
  SetWindowText(dlg->mod_label[LUXURIES],
		text_happiness_luxuries(dlg->pcity));
  SetWindowText(dlg->mod_label[BUILDINGS],
		text_happiness_buildings(dlg->pcity));
  SetWindowText(dlg->mod_label[UNITS],
		text_happiness_units(dlg->pcity));
  SetWindowText(dlg->mod_label[WONDERS],
		text_happiness_wonders(dlg->pcity));
}
