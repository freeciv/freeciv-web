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

#include <assert.h>
#include <stdio.h>

#include <windows.h>
#include <windowsx.h>

/* common & utility */
#include "fcintl.h"
#include "game.h"
#include "log.h"
#include "government.h"         /* government_graphic() */
#include "map.h"
#include "mem.h"
#include "player.h"
#include "rand.h"
#include "support.h"
#include "timing.h"
#include "unitlist.h"
#include "version.h"

/* client */
#include "canvas.h"
#include "citydlg.h" 
#include "client_main.h"
#include "climap.h"
#include "climisc.h"
#include "colors.h"
#include "control.h" /* get_unit_in_focus() */
#include "graphics.h"
#include "gui_stuff.h"
#include "mapctrl.h"
#include "options.h"
#include "overview_common.h"
#include "tilespec.h"      
#include "goto.h"
#include "gui_main.h"
#include "mapview.h"
#include "sprite.h"
#include "text.h"

extern HCURSOR cursors[];

static struct sprite *indicator_sprite[3];

static HBITMAP intro_gfx;

static int cursor_type = -1;

extern void do_mainwin_layout();

extern int seconds_to_turndone;   
void update_map_canvas_scrollbars_size(void);
void refresh_overview_viewrect_real(HDC hdcp);
static void draw_rates(HDC hdc);

/**************************************************************************
  This function is used to animate the mouse cursor. 
**************************************************************************/
void anim_cursor(float time)
{
  int cursor_frame = (int)(time * 15.0f) % NUM_CURSOR_FRAMES;

  if (cursor_type == CURSOR_DEFAULT) {
    SetCursor (LoadCursor(NULL, IDC_ARROW));
  } else {
    SetCursor(cursors[cursor_type * NUM_CURSOR_FRAMES + cursor_frame]);
  }
}

/**************************************************************************

**************************************************************************/
void map_expose(int x, int y, int width, int height)
{
  HBITMAP bmsave;
  HDC introgfxdc;
  HDC hdc;

  if (!can_client_change_view()) {
    if (!intro_gfx_sprite) {
      load_intro_gfx();
    }
    if (!intro_gfx) {
      intro_gfx = BITMAP2HBITMAP(intro_gfx_sprite->img);
    }
    hdc = GetDC(map_window);
    introgfxdc = CreateCompatibleDC(hdc);
    bmsave = SelectObject(introgfxdc, intro_gfx);
    StretchBlt(hdc, 0, 0, map_win_width, map_win_height,
	       introgfxdc, 0, 0,
	       intro_gfx_sprite->width,
	       intro_gfx_sprite->height,
	       SRCCOPY);
    SelectObject(introgfxdc, bmsave);
    DeleteDC(introgfxdc);
    ReleaseDC(map_window, hdc);
  } else {
    /* First we mark the area to be updated as dirty.  Then we unqueue
     * any pending updates, to make sure only the most up-to-date data
     * is written (otherwise drawing bugs happen when old data is copied
     * to screen).  Then we draw all changed areas to the screen. */
    unqueue_mapview_updates(FALSE);
    canvas_copy(get_mapview_window(), mapview.store, x, y, x, y,
		width, height);
  }
} 

/**************************************************************************
hack to ensure that mapstorebitmap is usable. 
On win95/win98 mapstorebitmap becomes somehow invalid. 
**************************************************************************/
void check_mapstore()
{
  static int n=0;
  HDC hdc;
  BITMAP bmp;
  if (GetObject(mapstorebitmap,sizeof(BITMAP),&bmp)==0) {
    DeleteObject(mapstorebitmap);
    hdc=GetDC(map_window);
    mapstorebitmap=CreateCompatibleBitmap(hdc,map_win_width,map_win_height);
    update_map_canvas_visible();
    n++;
    assert(n<5);
  }
}

/**************************************************************************

**************************************************************************/
static void draw_rates(HDC hdc)
{
  int d = 0;

  for (; d < (client.conn.playing->economic.luxury)/10; d++) {
    draw_sprite(get_tax_sprite(tileset, O_LUXURY), hdc,
		tileset_small_sprite_width(tileset)*d,taxinfoline_y);/* elvis tile */
  }

  for (; d < (client.conn.playing->economic.science + client.conn.playing->economic.luxury)/10; d++) {
    draw_sprite(get_tax_sprite(tileset, O_SCIENCE), hdc,
		tileset_small_sprite_width(tileset)*d,taxinfoline_y); /* scientist tile */
  }

  for (; d < 10; d++) {
    draw_sprite(get_tax_sprite(tileset, O_GOLD), hdc,
		tileset_small_sprite_width(tileset)*d,taxinfoline_y); /* taxman tile */
  }
}

/**************************************************************************

**************************************************************************/
void
update_info_label(void)
{
  char buffer2[512];
  char buffer[512];
  HDC hdc;
  my_snprintf(buffer, sizeof(buffer),
	      _("Population: %s\nYear: %s\nGold: %d\nTax: %d Lux: %d Sci: %d"),
		population_to_text(civ_population(client.conn.playing)),
		textyear( game.info.year ),
		client.conn.playing->economic.gold,
		client.conn.playing->economic.tax,
		client.conn.playing->economic.luxury,
		client.conn.playing->economic.science );
  my_snprintf(buffer2,sizeof(buffer2),
	      "%s\n%s",
	      nation_adjective_for_player(client.conn.playing),
	      buffer);
  SetWindowText(infolabel_win,buffer2);
  do_mainwin_layout();
  set_indicator_icons(client_research_sprite(),
		      client_warming_sprite(),
                      client_cooling_sprite(),
		      client_government_sprite());
  
  hdc=GetDC(root_window);
  draw_rates(hdc);
  ReleaseDC(root_window,hdc);
  update_timeout_label();    
  
}

/**************************************************************************

**************************************************************************/
void
update_unit_info_label(struct unit_list *punitlist)
{
  SetWindowText(unit_info_frame, get_unit_info_label_text1(punitlist));
  SetWindowText(unit_info_label, get_unit_info_label_text2(punitlist, 0));

  /* Cursor handling has changed a lot. New form is not yet implemented
   * for gui-win32. Old code below is probably never needed again, but
   * I left it here just in case. Remove when new implementation is in
   * place. */
#if 0
  switch (hover_state) {
    case HOVER_NONE:
      if (action_state == CURSOR_ACTION_SELECT) {
        cursor_type = CURSOR_SELECT;
      } else if (action_state == CURSOR_ACTION_PARATROOPER) {
        cursor_type = CURSOR_PARADROP;
      } else if (action_state == CURSOR_ACTION_NUKE) {
        cursor_type = CURSOR_NUKE;
      } else {
        cursor_type = CURSOR_DEFAULT;
      }
      break;
    case HOVER_PATROL:
      if (action_state == CURSOR_ACTION_INVALID) {
        cursor_type = CURSOR_INVALID;
      } else {
        cursor_type = CURSOR_PATROL;
      }
      break;
    case HOVER_GOTO:
      if (action_state == CURSOR_ACTION_GOTO) {
        cursor_type = CURSOR_GOTO;
      } else if (action_state == CURSOR_ACTION_DEFAULT) {
        cursor_type = CURSOR_DEFAULT;
      } else if (action_state == CURSOR_ACTION_ATTACK) {
        cursor_type = CURSOR_ATTACK;
      } else {
        cursor_type = CURSOR_INVALID;
      }
      break;
    case HOVER_CONNECT:
      if (action_state == CURSOR_ACTION_INVALID) {
        cursor_type = CURSOR_INVALID;
      } else {
        cursor_type = CURSOR_GOTO;
      }
      break;
    case HOVER_NUKE:
      cursor_type = CURSOR_NUKE;
      break;
    case HOVER_PARADROP:
      cursor_type = CURSOR_PARADROP;
      break;
  }
#endif

  do_mainwin_layout();
}

/**************************************************************************

**************************************************************************/
void update_timeout_label(void)
{
  SetWindowText(timeout_label, get_timeout_label_text());
}

/**************************************************************************
  This function will change the current mouse cursor.
**************************************************************************/
void update_mouse_cursor(enum cursor_type new_cursor_type)
{
  /* PORT ME */
}

/**************************************************************************

**************************************************************************/
void update_turn_done_button(bool do_restore)
{
  static bool flip = FALSE;

  if (!get_turn_done_button_state()) {
    return;
  }

  if (do_restore) {
    flip = FALSE;
    Button_SetState(turndone_button, 0);
  } else {
    Button_SetState(turndone_button, flip);
    flip = !flip;
  }
}

/****************************************************************************
  Set information for the indicator icons typically shown in the main
  client window.  The parameters tell which sprite to use for the
  indicator.
****************************************************************************/
void set_indicator_icons(struct sprite *bulb, struct sprite *sol,
			 struct sprite *flake, struct sprite *gov)
{
  int i;
  HDC hdc;
  
  indicator_sprite[0] = bulb;
  indicator_sprite[1] = sol;
  indicator_sprite[2] = flake;
  indicator_sprite[3] = gov;

  hdc=GetDC(root_window);
  for(i=0;i<4;i++)
    draw_sprite(indicator_sprite[i],hdc,i*tileset_small_sprite_width(tileset),indicator_y); 
  ReleaseDC(root_window,hdc);
}

/****************************************************************************
  Return the dimensions of the area for the overview.
****************************************************************************/
void get_overview_area_dimensions(int *width, int *height)
{
  *width = overview_win_width;
  *height = overview_win_height;
}

/**************************************************************************

**************************************************************************/
void overview_size_changed(void)
{
  set_overview_win_dim(OVERVIEW_TILE_WIDTH * map.xsize,OVERVIEW_TILE_HEIGHT * map.ysize);
}

/**************************************************************************
  Flush the given part of the canvas buffer (if there is one) to the
  screen.
**************************************************************************/
void flush_mapcanvas(int canvas_x, int canvas_y,
		     int pixel_width, int pixel_height)
{
  canvas_copy(get_mapview_window(), mapview.store, canvas_x, canvas_y,
	      canvas_x, canvas_y, pixel_width, pixel_height);
}

#define MAX_DIRTY_RECTS 20
static int num_dirty_rects = 0;
static struct {
  int x, y, w, h;
} dirty_rects[MAX_DIRTY_RECTS];
bool is_flush_queued = FALSE;

/**************************************************************************
  A callback invoked as a result of a timer event, this function simply
  flushes the mapview canvas.
**************************************************************************/
static VOID CALLBACK unqueue_flush(HWND hwnd, UINT uMsg, UINT idEvent,
				   DWORD dwTime)
{
  flush_dirty();
  redraw_selection_rectangle();
  is_flush_queued = FALSE;
}

/**************************************************************************
  Called when a region is marked dirty, this function queues a flush event
  to be handled later.  The flush may end up being done by freeciv before
  then, in which case it will be a wasted call.
**************************************************************************/
static void queue_flush(void)
{
  if (!is_flush_queued) {
    SetTimer(root_window, 4, 0, unqueue_flush);
    is_flush_queued = TRUE;
  }
}

/**************************************************************************
  Mark the rectangular region as 'dirty' so that we know to flush it
  later.
**************************************************************************/
void dirty_rect(int canvas_x, int canvas_y,
		int pixel_width, int pixel_height)
{
  if (num_dirty_rects < MAX_DIRTY_RECTS) {
    dirty_rects[num_dirty_rects].x = canvas_x;
    dirty_rects[num_dirty_rects].y = canvas_y;
    dirty_rects[num_dirty_rects].w = pixel_width;
    dirty_rects[num_dirty_rects].h = pixel_height;
    num_dirty_rects++;
    queue_flush();
  }
}

/**************************************************************************
  Mark the entire screen area as "dirty" so that we can flush it later.
**************************************************************************/
void dirty_all(void)
{
  num_dirty_rects = MAX_DIRTY_RECTS;
  queue_flush();
}

/**************************************************************************
  Flush all regions that have been previously marked as dirty.  See
  dirty_rect and dirty_all.  This function is generally called after we've
  processed a batch of drawing operations.
**************************************************************************/
void flush_dirty(void)
{
  if (num_dirty_rects == MAX_DIRTY_RECTS) {
    flush_mapcanvas(0, 0, map_win_width, map_win_height);
  } else {
    int i;

    for (i = 0; i < num_dirty_rects; i++) {
      flush_mapcanvas(dirty_rects[i].x, dirty_rects[i].y,
		      dirty_rects[i].w, dirty_rects[i].h);
    }
  }
  num_dirty_rects = 0;
}

/****************************************************************************
  Do any necessary synchronization to make sure the screen is up-to-date.
  The canvas should have already been flushed to screen via flush_dirty -
  all this function does is make sure the hardware has caught up.
****************************************************************************/
void gui_flush(void)
{
  GdiFlush();
}

/**************************************************************************

**************************************************************************/
void update_map_canvas_scrollbars_size(void)
{
  int xmin, ymin, xmax, ymax, xsize, ysize;

  get_mapview_scroll_window(&xmin, &ymin, &xmax, &ymax, &xsize, &ysize);
  ScrollBar_SetRange(map_scroll_h, xmin, xmax, TRUE);
  ScrollBar_SetRange(map_scroll_v, ymin, ymax, TRUE);
}

/**************************************************************************

**************************************************************************/
void
update_map_canvas_scrollbars(void)
{
  int scroll_x, scroll_y;

  get_mapview_scroll_pos(&scroll_x, &scroll_y);
  ScrollBar_SetPos(map_scroll_h, scroll_x, TRUE);
  ScrollBar_SetPos(map_scroll_v, scroll_y, TRUE);
}

/**************************************************************************

**************************************************************************/
void
update_city_descriptions(void)
{
  update_map_canvas_visible();   
      
}

/**************************************************************************

**************************************************************************/
void
put_cross_overlay_tile(struct tile *ptile)
{
  HDC hdc;
  int canvas_x, canvas_y;
  tile_to_canvas_pos(&canvas_x, &canvas_y, ptile);
  if (tile_visible_mapcanvas(ptile)) {
    hdc=GetDC(map_window);
    draw_sprite(get_attention_crosshair_sprite(tileset), hdc, canvas_x, canvas_y);
    ReleaseDC(map_window,hdc);
  }
}

/**************************************************************************

**************************************************************************/
void overview_expose(HDC hdc)
{
  HDC hdctest;
  HBITMAP old;
  HBITMAP bmp;
  int i;
  if (!can_client_change_view()) {
      if (!radar_gfx_sprite) {
	load_intro_gfx();
      }
      if (radar_gfx_sprite) {
	char s[64];
	int h;
	RECT rc;
	draw_sprite(radar_gfx_sprite,hdc,overview_win_x,overview_win_y);
	SetBkMode(hdc,TRANSPARENT);
	my_snprintf(s, sizeof(s), "%d.%d.%d%s",
		    MAJOR_VERSION, MINOR_VERSION,
		    PATCH_VERSION, VERSION_LABEL);
	DrawText(hdc, word_version(), strlen(word_version()), &rc, DT_CALCRECT);
	h=rc.bottom-rc.top;
	rc.left = overview_win_x;
	rc.right = overview_win_y + overview_win_width;
	rc.bottom = overview_win_y + overview_win_height - h - 2; 
	rc.top = rc.bottom - h;
	SetTextColor(hdc, RGB(0,0,0));
	DrawText(hdc, word_version(), strlen(word_version()), &rc, DT_CENTER);
	rc.top+=h;
	rc.bottom+=h;
	DrawText(hdc, s, strlen(s), &rc, DT_CENTER);
	rc.left++;
	rc.right++;
	rc.top--;
	rc.bottom--;
	SetTextColor(hdc, RGB(255,255,255));
	DrawText(hdc, s, strlen(s), &rc, DT_CENTER);
	rc.top-=h;
	rc.bottom-=h;
	DrawText(hdc, word_version(), strlen(word_version()), &rc, DT_CENTER);
      }
    }
  else
    {
      hdctest=CreateCompatibleDC(NULL);
      old=NULL;
      bmp=NULL;
      for(i=0;i<4;i++)
	if (indicator_sprite[i]) {
	  bmp=BITMAP2HBITMAP(indicator_sprite[i]->img);
	  if (!old)
	    old=SelectObject(hdctest,bmp);
	  else
	    DeleteObject(SelectObject(hdctest,bmp));
	  BitBlt(hdc,i*tileset_small_sprite_width(tileset),indicator_y,
		 tileset_small_sprite_width(tileset),tileset_small_sprite_height(tileset),
		 hdctest,0,0,SRCCOPY);
	}
      SelectObject(hdctest,old);
      if (bmp)
	DeleteObject(bmp);
      DeleteDC(hdctest);
      draw_rates(hdc);
      refresh_overview_canvas();
    }
}

/**************************************************************************

**************************************************************************/
void map_handle_hscroll(int pos)
{
  int scroll_x, scroll_y;

  if (!can_client_change_view()) {
    return;
  }

  get_mapview_scroll_pos(&scroll_x, &scroll_y);
  set_mapview_scroll_pos(pos, scroll_y);
}

/**************************************************************************

**************************************************************************/
void map_handle_vscroll(int pos)
{
  int scroll_x, scroll_y;

  if (!can_client_change_view()) {
    return;
  }

  get_mapview_scroll_pos(&scroll_x, &scroll_y);
  set_mapview_scroll_pos(scroll_x, pos);
}

/**************************************************************************
 Area Selection
**************************************************************************/
void draw_selection_rectangle(int canvas_x, int canvas_y, int w, int h)
{
  /* PORTME */
}

/**************************************************************************
  This function is called when the tileset is changed.
**************************************************************************/
void tileset_changed(void)
{
  /* PORTME */
  /* Here you should do any necessary redraws (for instance, the city
   * dialogs usually need to be resized).
   */
  
  indicator_sprite[0] = NULL;
  indicator_sprite[1] = NULL;
  indicator_sprite[2] = NULL;
  init_fog_bmp();
  map_canvas_resized(mapview.width, mapview.height);
  citydlg_tileset_change();
}
