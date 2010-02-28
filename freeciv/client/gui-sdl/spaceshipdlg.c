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

/* utility */
#include "fcintl.h"
#include "log.h"

/* common */
#include "game.h"
#include "packets.h"

/* client */
#include "client_main.h"
#include "text.h"

/* gui-sdl */
#include "graphics.h"
#include "gui_id.h"
#include "gui_main.h"
#include "gui_tilespec.h"
#include "mapview.h"
#include "widget.h"

#include "spaceshipdlg.h"

#define SPECLIST_TAG dialog
#define SPECLIST_TYPE struct SMALL_DLG
#include "speclist.h"

#define dialog_list_iterate(dialoglist, pdialog) \
    TYPED_LIST_ITERATE(struct SMALL_DLG, dialoglist, pdialog)
#define dialog_list_iterate_end  LIST_ITERATE_END
    
static struct dialog_list *dialog_list = NULL;
static bool dialog_list_has_been_initialised = FALSE;

/****************************************************************
...
*****************************************************************/
static struct SMALL_DLG *get_spaceship_dialog(struct player *pplayer)
{
  if (!dialog_list_has_been_initialised) {
    dialog_list = dialog_list_new();
    dialog_list_has_been_initialised = TRUE;
  }

  dialog_list_iterate(dialog_list, pDialog) {
    if (pDialog->pEndWidgetList->data.player == pplayer) {
      return pDialog;
    }
  } dialog_list_iterate_end;

  return NULL;
}

static int space_dialog_window_callback(struct widget *pWindow)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    move_window_group(pWindow->private_data.small_dlg->pBeginWidgetList, pWindow);
  }
  return -1;
}

static int exit_space_dialog_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    popdown_spaceship_dialog(pWidget->data.player);
    flush_dirty();
  }
  return -1;
}

static int launch_spaceship_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    send_packet_spaceship_launch(&client.conn);
  }
  return -1;
}

/**************************************************************************
  Refresh (update) the spaceship dialog for the given player.
**************************************************************************/
void refresh_spaceship_dialog(struct player *pPlayer)
{
  struct SMALL_DLG *pSpaceShp;
  struct widget *pBuf;
    
  if(!(pSpaceShp = get_spaceship_dialog(pPlayer)))
    return;
  
  /* launch button */
  pBuf = pSpaceShp->pEndWidgetList->prev->prev;
  if(game.info.spacerace
     && pPlayer == client.conn.playing
     && pPlayer->spaceship.state == SSHIP_STARTED
     && pPlayer->spaceship.success_rate > 0.0) {
    set_wstate(pBuf, FC_WS_NORMAL);
  }
  
  /* update text info */
  pBuf = pBuf->prev;
  copy_chars_to_string16(pBuf->string16,
				get_spaceship_descr(&pPlayer->spaceship));
  /* ------------------------------------------ */
  
    /* redraw */
  redraw_group(pSpaceShp->pBeginWidgetList, pSpaceShp->pEndWidgetList, 0);
  widget_mark_dirty(pSpaceShp->pEndWidgetList);
  
  flush_dirty();
}

/**************************************************************************
  Popup (or raise) the spaceship dialog for the given player.
**************************************************************************/
void popup_spaceship_dialog(struct player *pPlayer)
{
  struct SMALL_DLG *pSpaceShp;

  if(!(pSpaceShp = get_spaceship_dialog(pPlayer))) {
    struct widget *pBuf, *pWindow;
    SDL_String16 *pStr;
    char cBuf[128];
    SDL_Rect area;
  
    pSpaceShp = fc_calloc(1, sizeof(struct SMALL_DLG));
    
    my_snprintf(cBuf, sizeof(cBuf), _("The %s Spaceship"),
				    nation_adjective_for_player(pPlayer));
    pStr = create_str16_from_char(cBuf, adj_font(12));
    pStr->style |= TTF_STYLE_BOLD;
  
    pWindow = create_window_skeleton(NULL, pStr, 0);
  
    pWindow->action = space_dialog_window_callback;
    set_wstate(pWindow, FC_WS_NORMAL);
    pWindow->data.player = pPlayer;
    pWindow->private_data.small_dlg = pSpaceShp;
    add_to_gui_list(ID_WINDOW, pWindow);
    pSpaceShp->pEndWidgetList = pWindow;

    area = pWindow->area;
    
    /* ---------- */
    /* create exit button */
    pBuf = create_themeicon(pTheme->Small_CANCEL_Icon, pWindow->dst,
  			    WF_WIDGET_HAS_INFO_LABEL | WF_RESTORE_BACKGROUND);
    pBuf->string16 = create_str16_from_char(_("Close Dialog (Esc)"), adj_font(12));
    pBuf->data.player = pPlayer;
    pBuf->action = exit_space_dialog_callback;
    set_wstate(pBuf, FC_WS_NORMAL);
    pBuf->key = SDLK_ESCAPE;
    area.w = MAX(area.w, (pBuf->size.w + adj_size(10)));
  
    add_to_gui_list(ID_BUTTON, pBuf);
  
    pBuf = create_themeicon_button_from_chars(pTheme->OK_Icon, pWindow->dst,
					      _("Launch"), adj_font(12), 0);
        
    pBuf->action = launch_spaceship_callback;
    area.w = MAX(area.w, pBuf->size.w);
    area.h += pBuf->size.h + adj_size(20);
    add_to_gui_list(ID_BUTTON, pBuf);
    
    pStr = create_str16_from_char(get_spaceship_descr(NULL), adj_font(12));
    pStr->bgcol = (SDL_Color) {0, 0, 0, 0};
    pBuf = create_iconlabel(NULL, pWindow->dst, pStr, WF_RESTORE_BACKGROUND);
    area.w = MAX(area.w, pBuf->size.w);
    area.h += pBuf->size.h + adj_size(20);
    add_to_gui_list(ID_LABEL, pBuf);

    pSpaceShp->pBeginWidgetList = pBuf;
    /* -------------------------------------------------------- */
  
    area.w = MAX(area.w, adj_size(300) - (pWindow->size.w - pWindow->area.w));

    resize_window(pWindow, NULL, NULL,
                  (pWindow->size.w - pWindow->area.w) + area.w,
                  (pWindow->size.h - pWindow->area.h) + area.h);
    
    area = pWindow->area;
    
    widget_set_position(pWindow,
                        (Main.screen->w - pWindow->size.w) / 2,
                        (Main.screen->h - pWindow->size.h) / 2);
    
    /* exit button */
    pBuf = pWindow->prev;
    pBuf->size.x = area.x + area.w - pBuf->size.w - 1;
    pBuf->size.y = pWindow->size.y + adj_size(2);

    /* launch button */
    pBuf = pBuf->prev;
    pBuf->size.x = area.x + (area.w - pBuf->size.w) / 2;
    pBuf->size.y = area.y + area.h - pBuf->size.h - adj_size(7);
  
    /* info label */
    pBuf = pBuf->prev;
    pBuf->size.x = area.x + (area.w - pBuf->size.w) / 2;
    pBuf->size.y = area.y + adj_size(7);

    dialog_list_prepend(dialog_list, pSpaceShp);
    
    refresh_spaceship_dialog(pPlayer);
  } else {
    if (sellect_window_group_dialog(pSpaceShp->pBeginWidgetList,
				   pSpaceShp->pEndWidgetList)) {
      widget_flush(pSpaceShp->pEndWidgetList);
    }
  }
  
}

/**************************************************************************
  Close the spaceship dialog for the given player.
**************************************************************************/
void popdown_spaceship_dialog(struct player *pPlayer)
{
  struct SMALL_DLG *pSpaceShp;

  if((pSpaceShp = get_spaceship_dialog(pPlayer))) {
    popdown_window_group_dialog(pSpaceShp->pBeginWidgetList,
					    pSpaceShp->pEndWidgetList);
    dialog_list_unlink(dialog_list, pSpaceShp);
    FC_FREE(pSpaceShp);
  }
  
}
