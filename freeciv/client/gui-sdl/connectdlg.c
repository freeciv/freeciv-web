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
                          connectdlg.c  -  description
                             -------------------
    begin                : Mon Jul 1 2002
    copyright            : (C) 2002 by Rafał Bursig
    email                : Rafał Bursig <bursig@poczta.fm>
 **********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>

#include "SDL.h"

/* utility */
#include "fcintl.h"
#include "log.h"

/* client */
#include "client_main.h"
#include "clinet.h"		/* connect_to_server() */
#include "packhand.h"
#include "servers.h"

/* gui-sdl */
#include "chatline.h"
#include "colors.h"
#include "graphics.h"
#include "gui_iconv.h"
#include "gui_id.h"
#include "gui_main.h"
#include "gui_tilespec.h"
#include "mapview.h"
#include "messagewin.h"
#include "optiondlg.h"
#include "pages.h"
#include "themespec.h"
#include "widget.h"

#include "connectdlg.h"

static const struct server_list *pServer_list = NULL;
static struct server_scan *pServer_scan = NULL; 
    
static struct ADVANCED_DLG *pMeta_Severs = NULL;
static struct SMALL_DLG *pConnectDlg = NULL;

static int connect_callback(struct widget *pWidget);
static int convert_portnr_callback(struct widget *pWidget);
static int convert_playername_callback(struct widget *pWidget);
static int convert_servername_callback(struct widget *pWidget);

/*
  THOSE FUNCTIONS ARE ONE BIG TMP SOLUTION AND SHOULD BE FULL REWRITEN !!
*/

/**************************************************************************
 really close and destroy the dialog.
**************************************************************************/
void really_close_connection_dialog(void)
{
  /* PORTME */
}

/**************************************************************************
 provide a packet handler for packet_game_load
**************************************************************************/
void handle_game_load(struct packet_game_load *packet)
{ 
  /* PORTME */
}

/**************************************************************************
  ...
**************************************************************************/
static int connect_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    char errbuf[512];
  
    if (connect_to_server(user_name, server_host, server_port,
                          errbuf, sizeof(errbuf)) != -1) {
    } else {
      output_window_append(NULL, NULL, errbuf);
      real_update_meswin_dialog();
  
      /* button up */
      unsellect_widget_action();
      set_wstate(pWidget, FC_WS_SELLECTED);
      pSellected_Widget = pWidget;
      widget_redraw(pWidget);
      widget_flush(pWidget);
    }
  }
  return -1;
}
/* ======================================================== */


/**************************************************************************
  ...
**************************************************************************/
static int meta_severs_window_callback(struct widget *pWindow)
{
  return -1;
}

/**************************************************************************
  ...
**************************************************************************/
static int exit_meta_severs_dlg_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    queue_flush();
      
    server_scan_finish(pServer_scan);
    pServer_scan = NULL;
    pServer_list = NULL;
      
    set_client_page(PAGE_NETWORK);
    popup_meswin_dialog(true);
  }    
  return -1;
}


/**************************************************************************
  ...
**************************************************************************/
static int sellect_meta_severs_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct server *pServer = (struct server *)pWidget->data.ptr;
        
    sz_strlcpy(server_host, pServer->host);
    server_port = pServer->port;
    
    exit_meta_severs_dlg_callback(NULL);
  }
  return -1;
}

/**************************************************************************
  Callback function for when there's an error in the server scan.
**************************************************************************/
static void server_scan_error(struct server_scan *scan,
			      const char *message)
{
  output_window_append(FTC_CLIENT_INFO, NULL, message);
  freelog(LOG_NORMAL, "%s", message);

  switch (server_scan_get_type(scan)) {
  case SERVER_SCAN_LOCAL:
    server_scan_finish(pServer_scan);
    pServer_scan = NULL;
    pServer_list = NULL;
    break;
  case SERVER_SCAN_GLOBAL:
    server_scan_finish(pServer_scan);
    pServer_scan = NULL;
    pServer_list = NULL;
    break;
  case SERVER_SCAN_LAST:
    break;
  }
}

/**************************************************************************
  SDL wraper on create_server_list(...) function witch add
  same functionality for LAN server dettection.
  WARING !: for LAN scan use "finish_lanserver_scan()" to free server list.
**************************************************************************/
static const struct server_list *sdl_create_server_list(bool lan)
{
  const struct server_list *server_list = NULL;
  int i;
    
  if (lan) {
    pServer_scan = server_scan_begin(SERVER_SCAN_LOCAL, server_scan_error);      
  } else {
    pServer_scan = server_scan_begin(SERVER_SCAN_GLOBAL, server_scan_error);      
  }

  if (!pServer_scan) {
    return NULL;
  }
  
  SDL_Delay(5000);
    
  for (i = 0; i < 100; i++) {
    server_scan_poll(pServer_scan);
    server_list = server_scan_get_list(pServer_scan);
    if (server_list) {
      break;
    }
    SDL_Delay(100);
  }
  
  return server_list;
}


void popup_connection_dialog(bool lan_scan)
{
  SDL_Color bg_color = {255, 255, 255, 128};

  char cBuf[512];
  int w = 0, h = 0, count = 0, meta_h;
  struct widget *pNewWidget, *pWindow, *pLabelWindow;
  SDL_String16 *pStr;
  SDL_Surface *pLogo;
  SDL_Rect area, area2;
  
  queue_flush();
  close_connection_dialog();
  popdown_meswin_dialog();

  /* Text Label */  
  pLabelWindow = create_window_skeleton(NULL, NULL, 0);
  add_to_gui_list(ID_WINDOW, pLabelWindow);
  
  area = pLabelWindow->area;
  
  my_snprintf(cBuf, sizeof(cBuf), _("Creating Server List..."));
  pStr = create_str16_from_char(cBuf, adj_font(16));
  pStr->style = TTF_STYLE_BOLD;
  pStr->bgcol = (SDL_Color) {0, 0, 0, 0};
  pNewWidget = create_iconlabel(NULL, pLabelWindow->dst, pStr, 
                (WF_RESTORE_BACKGROUND | WF_DRAW_TEXT_LABEL_WITH_SPACE));
  add_to_gui_list(ID_LABEL, pNewWidget);

  area.w = MAX(area.w, pNewWidget->size.w + (adj_size(60) -
                       (pLabelWindow->size.w - pLabelWindow->area.w)));
  area.h += pNewWidget->size.h + (adj_size(30) - 
            (pLabelWindow->size.w - pLabelWindow->area.w));
  
  resize_window(pLabelWindow, NULL, &bg_color,
                (pLabelWindow->size.w - pLabelWindow->area.w) + area.w,
                (pLabelWindow->size.h - pLabelWindow->area.h) + area.h);
  
  area = pLabelWindow->area;

  widget_set_position(pLabelWindow,
                      (Main.screen->w - pLabelWindow->size.w) / 2,
                      (Main.screen->h - pLabelWindow->size.h) / 2);

  widget_set_area(pNewWidget, area);
  widget_set_position(pNewWidget,
                      area.x + (area.w - pNewWidget->size.w)/2,
                      area.y + (area.h - pNewWidget->size.h)/2);
  
  redraw_group(pNewWidget, pLabelWindow, TRUE);
  flush_dirty();
  
  /* create server list */
  pServer_list = sdl_create_server_list(lan_scan);

  /* clear label */
  popdown_window_group_dialog(pNewWidget, pLabelWindow);

  popup_meswin_dialog(true);        
  
  if(!pServer_list) {
    if (lan_scan) {
      output_window_append(FTC_CLIENT_INFO, NULL,
                           _("No LAN servers found")); 
    } else {
      output_window_append(FTC_CLIENT_INFO, NULL,
                           _("No public servers found")); 
    }        
    real_update_meswin_dialog();
    set_client_page(PAGE_NETWORK);
    return;
  }
  
  /* Server list window */  
  pMeta_Severs = fc_calloc(1, sizeof(struct ADVANCED_DLG));
    
  pWindow = create_window_skeleton(NULL, NULL, 0);
  pWindow->action = meta_severs_window_callback;
  set_wstate(pWindow, FC_WS_NORMAL);
  clear_wflag(pWindow, WF_DRAW_FRAME_AROUND_WIDGET);
  if (lan_scan)
  {
    add_to_gui_list(ID_LAN_SERVERS_WINDOW, pWindow);
  } else {
    add_to_gui_list(ID_META_SERVERS_WINDOW, pWindow);
  }
  pMeta_Severs->pEndWidgetList = pWindow;
  
  area = pWindow->area;
  
  /* Cancel button */
  pNewWidget = create_themeicon_button_from_chars(pTheme->CANCEL_Icon, pWindow->dst,
						     _("Cancel"), adj_font(14), 0);
  pNewWidget->action = exit_meta_severs_dlg_callback;
  set_wstate(pNewWidget, FC_WS_NORMAL);
  add_to_gui_list(ID_BUTTON, pNewWidget);
  
  /* servers */
  server_list_iterate(pServer_list, pServer) {

    my_snprintf(cBuf, sizeof(cBuf), "%s Port %d Ver: %s %s %s %d\n%s",
    	pServer->host, pServer->port, pServer->version, _(pServer->state),
    		_("Players"), pServer->nplayers, pServer->message);

    pNewWidget = create_iconlabel_from_chars(NULL, pWindow->dst, cBuf, adj_font(10),
	WF_FREE_STRING|WF_DRAW_TEXT_LABEL_WITH_SPACE|WF_RESTORE_BACKGROUND);
    
    pNewWidget->string16->style |= SF_CENTER;
    pNewWidget->string16->bgcol = (SDL_Color) {0, 0, 0, 0};
    
    pNewWidget->action = sellect_meta_severs_callback;
    set_wstate(pNewWidget, FC_WS_NORMAL);
    pNewWidget->data.ptr = (void *)pServer;
    
    add_to_gui_list(ID_BUTTON, pNewWidget);
    
    w = MAX(w, pNewWidget->size.w);
    h = MAX(h, pNewWidget->size.h);
    count++;
    
    if(count > 10) {
      set_wflag(pNewWidget, WF_HIDDEN);
    }
    
  } server_list_iterate_end;

  if(!count) {
    if (lan_scan) {
      output_window_append(FTC_CLIENT_INFO, NULL,
                           _("No LAN servers found")); 
    } else {
      output_window_append(FTC_CLIENT_INFO, NULL,
                           _("No public servers found"));
    }
    real_update_meswin_dialog();
    set_client_page(PAGE_NETWORK);
    return;
  }
  
  pMeta_Severs->pBeginWidgetList = pNewWidget;
  pMeta_Severs->pBeginActiveWidgetList = pMeta_Severs->pBeginWidgetList;
  pMeta_Severs->pEndActiveWidgetList = pMeta_Severs->pEndWidgetList->prev->prev;
  pMeta_Severs->pActiveWidgetList = pMeta_Severs->pEndActiveWidgetList;
    
  if (count > 10) {
    meta_h = 10 * h;
  
    count = create_vertical_scrollbar(pMeta_Severs, 1, 10, TRUE, TRUE);
    w += count;
  } else {
    meta_h = count * h;
  }
  
  w += adj_size(20);
  area2.h = meta_h;
  
  meta_h += pMeta_Severs->pEndWidgetList->prev->size.h + adj_size(10) + adj_size(20);

  pLogo = theme_get_background(theme, BACKGROUND_CONNECTDLG);
  if (resize_window(pWindow , pLogo , NULL , w , meta_h)) {
    FREESURFACE(pLogo);
  }

  area = pWindow->area;
  
  widget_set_position(pWindow,
                      (Main.screen->w - w) / 2,
                      (Main.screen->h - meta_h) / 2);
  
  w -= adj_size(20);
  
  area2.w = w + 1;
  
  if(pMeta_Severs->pScroll) {
    w -= count;
  }
  
  /* exit button */
  pNewWidget = pWindow->prev;
  pNewWidget->size.x = area.x + area.w - pNewWidget->size.w - adj_size(10);
  pNewWidget->size.y = area.y + area.h - pNewWidget->size.h - adj_size(10);
  
  /* meta labels */
  pNewWidget = pNewWidget->prev;
  
  pNewWidget->size.x = area.x + adj_size(10);
  pNewWidget->size.y = area.y + adj_size(10);
  pNewWidget->size.w = w;
  pNewWidget->size.h = h;
  pNewWidget = convert_iconlabel_to_themeiconlabel2(pNewWidget);
  
  pNewWidget = pNewWidget->prev;
  while(pNewWidget)
  {
        
    pNewWidget->size.w = w;
    pNewWidget->size.h = h;
    pNewWidget->size.x = pNewWidget->next->size.x;
    pNewWidget->size.y = pNewWidget->next->size.y + pNewWidget->next->size.h;
    pNewWidget = convert_iconlabel_to_themeiconlabel2(pNewWidget);
    
    if (pNewWidget == pMeta_Severs->pBeginActiveWidgetList) {
      break;
    }
    pNewWidget = pNewWidget->prev;  
  }
  
  if(pMeta_Severs->pScroll) {
    setup_vertical_scrollbar_area(pMeta_Severs->pScroll,
	area.x + area.w - adj_size(6),
	pMeta_Severs->pEndActiveWidgetList->size.y,
	area.h - adj_size(24) - pWindow->prev->size.h, TRUE);
  }
  
  /* -------------------- */
  /* redraw */
  
  widget_redraw(pWindow);
  
  area2.x = pMeta_Severs->pEndActiveWidgetList->size.x;
  area2.y = pMeta_Severs->pEndActiveWidgetList->size.y;

  SDL_FillRectAlpha(pWindow->dst->surface, &area2, &bg_color);
  
  putframe(pWindow->dst->surface, area2.x - 1, area2.y - 1, 
	area2.x + area2.w , area2.y + area2.h,
        map_rgba(pWindow->dst->surface->format, *get_game_colorRGB(COLOR_THEME_CONNECTDLG_INNERFRAME)));
  
  redraw_group(pMeta_Severs->pBeginWidgetList, pWindow->prev, 0);

  putframe(pWindow->dst->surface, pWindow->size.x , pWindow->size.y , 
     area.x + area.w - 1,
     area.y + area.h - 1,
     map_rgba(pWindow->dst->surface->format, *get_game_colorRGB(COLOR_THEME_CONNECTDLG_FRAME)));
    
  widget_flush(pWindow);
}

/**************************************************************************
  ...
**************************************************************************/
static int convert_playername_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    char *tmp = convert_to_chars(pWidget->string16->text);
    
    if (tmp) {  
      sz_strlcpy(user_name, tmp);
      FC_FREE(tmp);
    } else {
      /* empty input -> restore previous content */
      copy_chars_to_string16(pWidget->string16, user_name);
      widget_redraw(pWidget);
      widget_mark_dirty(pWidget);
      flush_dirty();
    }
  }  
  return -1;
}

/**************************************************************************
...
**************************************************************************/
static int convert_servername_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    char *tmp = convert_to_chars(pWidget->string16->text);
    
    if (tmp) {
      sz_strlcpy(server_host, tmp);
      FC_FREE(tmp);
    } else {
      /* empty input -> restore previous content */
      copy_chars_to_string16(pWidget->string16, server_host);
      widget_redraw(pWidget);
      widget_mark_dirty(pWidget);
      flush_dirty();
    }
  }  
  return -1;
}

/**************************************************************************
  ...
**************************************************************************/
static int convert_portnr_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    char pCharPort[6];
    char *tmp = convert_to_chars(pWidget->string16->text);
    
    if (tmp) {
      sscanf(tmp, "%d", &server_port);
      FC_FREE(tmp);
    } else {
      /* empty input -> restore previous content */
      my_snprintf(pCharPort, sizeof(pCharPort), "%d", server_port);
      copy_chars_to_string16(pWidget->string16, pCharPort);
      widget_redraw(pWidget);
      widget_mark_dirty(pWidget);
      flush_dirty();
    }
  }  
  return -1;
}

static int cancel_connect_dlg_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    close_connection_dialog();
    set_client_page(PAGE_MAIN);
  }
  return -1;
}

void popup_join_game_dialog()
{
  char pCharPort[6];
  struct widget *pBuf, *pWindow;
  SDL_String16 *pPlayer_name = NULL;
  SDL_String16 *pServer_name = NULL;
  SDL_String16 *pPort_nr = NULL;
  SDL_Surface *pLogo;
  SDL_Rect area;
  
  int start_x;
  int pos_y;
  int dialog_w, dialog_h;
  
  queue_flush();
  close_connection_dialog();
  
  pConnectDlg = fc_calloc(1, sizeof(struct SMALL_DLG));

  /* window */
  pWindow = create_window_skeleton(NULL, NULL, 0);
  add_to_gui_list(ID_WINDOW, pWindow);
  pConnectDlg->pEndWidgetList = pWindow;
  
  area = pWindow->area;
  
  /* player name label */
  pPlayer_name = create_str16_from_char(_("Player Name :"), adj_font(10));
  pPlayer_name->fgcol = *get_game_colorRGB(COLOR_THEME_JOINGAMEDLG_TEXT);
  pBuf = create_iconlabel(NULL, pWindow->dst, pPlayer_name,
          (WF_RESTORE_BACKGROUND|WF_DRAW_TEXT_LABEL_WITH_SPACE));
  add_to_gui_list(ID_LABEL, pBuf);
  area.h += pBuf->size.h + adj_size(20);
  
  /* player name edit */
  pBuf = create_edit_from_chars(NULL, pWindow->dst, user_name, adj_font(14), adj_size(210),
				(WF_RESTORE_BACKGROUND|WF_FREE_DATA));
  pBuf->action = convert_playername_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  add_to_gui_list(ID_PLAYER_NAME_EDIT, pBuf);
  area.h += pBuf->size.h + adj_size(5);

  /* server name label */
  pServer_name = create_str16_from_char(_("Freeciv Server :"), adj_font(10));
  pServer_name->fgcol = *get_game_colorRGB(COLOR_THEME_JOINGAMEDLG_TEXT);
  pBuf = create_iconlabel(NULL, pWindow->dst, pServer_name,
          (WF_RESTORE_BACKGROUND|WF_DRAW_TEXT_LABEL_WITH_SPACE));
  add_to_gui_list(ID_LABEL, pBuf);
  area.h += pBuf->size.h + adj_size(5);
  
  /* server name edit */
  pBuf = create_edit_from_chars(NULL, pWindow->dst, server_host, adj_font(14), adj_size(210),
					 WF_RESTORE_BACKGROUND);

  pBuf->action = convert_servername_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  add_to_gui_list(ID_SERVER_NAME_EDIT, pBuf);
  area.h += pBuf->size.h + adj_size(5);

  /* port label */
  pPort_nr = create_str16_from_char(_("Port :"), adj_font(10));
  pPort_nr->fgcol = *get_game_colorRGB(COLOR_THEME_JOINGAMEDLG_TEXT);
  pBuf = create_iconlabel(NULL, pWindow->dst, pPort_nr,
          (WF_RESTORE_BACKGROUND|WF_DRAW_TEXT_LABEL_WITH_SPACE));
  add_to_gui_list(ID_LABEL, pBuf);
  area.h += pBuf->size.h + adj_size(5);
  
  /* port edit */
  my_snprintf(pCharPort, sizeof(pCharPort), "%d", server_port);
  
  pBuf = create_edit_from_chars(NULL, pWindow->dst, pCharPort, adj_font(14), adj_size(210),
					 WF_RESTORE_BACKGROUND);

  pBuf->action = convert_portnr_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  add_to_gui_list(ID_PORT_EDIT, pBuf);
  area.h += pBuf->size.h + adj_size(20);
  
  /* Connect button */
  pBuf = create_themeicon_button_from_chars(pTheme->OK_Icon, pWindow->dst,
					      _("Connect"), adj_font(14), 0);
  pBuf->action = connect_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->key = SDLK_RETURN;
  add_to_gui_list(ID_CONNECT_BUTTON, pBuf);
  
  /* Cancel button */
  pBuf = create_themeicon_button_from_chars(pTheme->CANCEL_Icon, pWindow->dst,
					       _("Cancel"), adj_font(14), 0);
  pBuf->action = cancel_connect_dlg_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->key = SDLK_ESCAPE;
  add_to_gui_list(ID_CANCEL_BUTTON, pBuf);
  pBuf->size.w = MAX(pBuf->size.w, pBuf->next->size.w);
  pBuf->next->size.w = pBuf->size.w;
  area.h += pBuf->size.h + adj_size(10);
  /* ------------------------------ */
  
  pConnectDlg->pBeginWidgetList = pBuf;
  
  dialog_w = MAX(adj_size(40) + pBuf->size.w * 2, adj_size(210)) + adj_size(80);
  
  #ifdef SMALL_SCREEN
  dialog_h = area.h + (pWindow->size.h - pWindow->area.h);
  #else
  dialog_h = area.h + (pWindow->size.h - pWindow->area.h);
  #endif

  pLogo = theme_get_background(theme, BACKGROUND_JOINGAMEDLG);
  if (resize_window(pWindow, pLogo, NULL, dialog_w, dialog_h)) {
    FREESURFACE(pLogo);
  }

  area = pWindow->area;
  
  widget_set_position(pWindow,
                      (Main.screen->w - pWindow->size.w)/ 2,
                      (Main.screen->h - pWindow->size.h)/ 2 + adj_size(40));

  /* player name label */
  pBuf = pConnectDlg->pEndWidgetList->prev;
  
  start_x = area.x + (area.w - pBuf->prev->size.w) / 2;
  pos_y = area.y + adj_size(20);
  
  pBuf->size.x = start_x + adj_size(5);
  pBuf->size.y = pos_y;
  
  pos_y += pBuf->size.h;

  pBuf = pBuf->prev;
  pBuf->size.x = start_x;
  pBuf->size.y = pos_y;

  pos_y += pBuf->size.h + adj_size(5);
  
  /* server name label */
  pBuf = pBuf->prev;
  pBuf->size.x = start_x + adj_size(5);
  pBuf->size.y = pos_y;
               
  pos_y += pBuf->size.h;
                  
  /* server name edit */
  pBuf = pBuf->prev;
  pBuf->size.x = start_x;
  pBuf->size.y = pos_y;

  pos_y += pBuf->size.h + adj_size(5);

  /* port label */
  pBuf = pBuf->prev;
  pBuf->size.x = start_x + adj_size(5);
  pBuf->size.y = pos_y;

  pos_y += pBuf->size.h;

  /* port edit */
  pBuf = pBuf->prev;
  pBuf->size.x = start_x;
  pBuf->size.y = pos_y;

  pos_y += pBuf->size.h + adj_size(20);

  /* connect button */
  pBuf = pBuf->prev;
  pBuf->size.x = area.x + (dialog_w - (adj_size(40) + pBuf->size.w * 2)) / 2;
  pBuf->size.y = pos_y;
  
  /* cancel button */
  pBuf = pBuf->prev;
  pBuf->size.x = pBuf->next->size.x + pBuf->size.w + adj_size(40);
  pBuf->size.y = pos_y;

  redraw_group(pConnectDlg->pBeginWidgetList, pConnectDlg->pEndWidgetList, FALSE);
  
  flush_all();
}

/**************************************************************************
  ...
**************************************************************************/
static int convert_passwd_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    char *tmp = convert_to_chars(pWidget->string16->text);
    if(tmp) {
      my_snprintf(password, MAX_LEN_NAME, "%s", tmp);
      FC_FREE(tmp);
    }
  }
  return -1;
}

/**************************************************************************
  ...
**************************************************************************/
static int send_passwd_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct packet_authentication_reply reply;
      
    sz_strlcpy(reply.password, password);
    
    memset(password, 0, MAX_LEN_NAME);
    password[0] = '\0';
    
    set_wstate(pWidget, FC_WS_DISABLED);
    set_wstate(pWidget->prev, FC_WS_DISABLED);
    
    widget_redraw(pWidget);
    widget_redraw(pWidget->prev);
    
    widget_mark_dirty(pWidget);
    widget_mark_dirty(pWidget->prev);
    
    flush_dirty();
    
    send_packet_authentication_reply(&client.conn, &reply);
  }
  return -1;
}

/**************************************************************************
  ...
**************************************************************************/
static void popup_user_passwd_dialog(char *pMessage)
{
  struct widget *pBuf, *pWindow;
  SDL_String16 *pLabelStr = NULL, *pPasswdStr = NULL;
  SDL_Surface *pBackground;
  int start_x, start_y;
  int start_button_y;
  SDL_Rect area;
        
  queue_flush();
  close_connection_dialog();
  
  pConnectDlg = fc_calloc(1, sizeof(struct SMALL_DLG));
    
  pWindow = create_window_skeleton(NULL, NULL, 0);
  add_to_gui_list(ID_WINDOW, pWindow);
  pConnectDlg->pEndWidgetList = pWindow;
  
  area = pWindow->area;
  
  /* text label */
  pLabelStr = create_str16_from_char(pMessage, adj_font(12));
  pLabelStr->fgcol = *get_game_colorRGB(COLOR_THEME_USERPASSWDDLG_TEXT);
  pBuf = create_iconlabel(NULL, pWindow->dst, pLabelStr,
          (WF_RESTORE_BACKGROUND|WF_DRAW_TEXT_LABEL_WITH_SPACE));
  add_to_gui_list(ID_LABEL, pBuf);
  area.h += adj_size(10) + pBuf->size.h + adj_size(5);
  
  /* password edit */
  pPasswdStr = create_str16_from_char(pMessage, adj_font(16));
  pPasswdStr->fgcol = *get_game_colorRGB(COLOR_THEME_TEXT);
  pBuf = create_edit(NULL, pWindow->dst, pPasswdStr, adj_size(210),
		(WF_PASSWD_EDIT|WF_RESTORE_BACKGROUND|WF_FREE_DATA));
  pBuf->action = convert_passwd_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  add_to_gui_list(ID_EDIT, pBuf);
  area.h += pBuf->size.h + adj_size(10);
  
  /* Next button */
  pBuf = create_themeicon_button_from_chars(pTheme->OK_Icon, pWindow->dst,
						     _("Next"), adj_font(14), 0);
  pBuf->action = send_passwd_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->key = SDLK_RETURN;
  add_to_gui_list(ID_BUTTON, pBuf);
  
  /* Cancel button */
  pBuf = create_themeicon_button_from_chars(pTheme->CANCEL_Icon, pWindow->dst,
						     _("Cancel"), adj_font(14), 0);
  pBuf->action = cancel_connect_dlg_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->key = SDLK_ESCAPE;
  add_to_gui_list(ID_CANCEL_BUTTON, pBuf);
  pBuf->size.w = MAX(pBuf->size.w, pBuf->next->size.w);
  pBuf->next->size.w = pBuf->size.w;
  area.h += pBuf->size.h + adj_size(10);
  
  /* ------------------------------ */
  
  pConnectDlg->pBeginWidgetList = pBuf;
  
  area.w = MAX(area.w, pBuf->size.w * 2 + adj_size(40));
  area.w = MAX(area.w, adj_size(210) - (pWindow->size.w - pWindow->area.w));
  area.w = MAX(area.w, pWindow->prev->size.w);
  area.w += adj_size(80);

  pBackground = theme_get_background(theme, BACKGROUND_USERPASSWDDLG);
  if (resize_window(pWindow, pBackground, NULL,
                    (pWindow->size.w - pWindow->area.w) + area.w,
                    (pWindow->size.h - pWindow->area.h) + area.h)) {
    FREESURFACE(pBackground);
  }

  area = pWindow->area;
  
  widget_set_position(pWindow,
                      (Main.screen->w - pWindow->size.w)/ 2,
                      (Main.screen->h - pWindow->size.h)/ 2);

  /* text label */
  pBuf = pConnectDlg->pEndWidgetList->prev;
  
  start_x = area.x + (area.w - pBuf->size.w) / 2;
  start_y = area.y + adj_size(10);

  widget_set_area(pBuf, area);
  widget_set_position(pBuf, start_x, start_y);
  
  start_y += pBuf->size.h + adj_size(5);

  /* password edit */    
  pBuf = pBuf->prev;
  start_x = area.x + (area.w - pBuf->size.w) / 2;

  widget_set_area(pBuf, area);  
  widget_set_position(pBuf, start_x, start_y);
  
  /* --------------------------------- */
  start_button_y = pBuf->size.y + pBuf->size.h + adj_size(10);

  /* connect button */
  pBuf = pBuf->prev;
  widget_set_area(pBuf, area);
  widget_set_position(pBuf,
                      area.x + (area.w - (adj_size(40) + pBuf->size.w * 2)) / 2,
                      start_button_y);
  
  /* cancel button */
  pBuf = pBuf->prev;
  widget_set_area(pBuf, area);
  widget_set_position(pBuf, 
                      pBuf->next->size.x + pBuf->size.w + adj_size(40),
                      start_button_y);

  redraw_group(pConnectDlg->pBeginWidgetList, pConnectDlg->pEndWidgetList, FALSE);

  flush_all();
}


/**************************************************************************
  New Password
**************************************************************************/
static int convert_first_passwd_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    char *tmp = convert_to_chars(pWidget->string16->text);
    
    if(tmp) {
      my_snprintf(password, MAX_LEN_NAME, "%s", tmp);
      FC_FREE(tmp);
      set_wstate(pWidget->prev, FC_WS_NORMAL);
      widget_redraw(pWidget->prev);
      widget_flush(pWidget->prev);
    }
  }
  return -1;
}

/**************************************************************************
  Verify Password
**************************************************************************/
static int convert_secound_passwd_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    char *tmp = convert_to_chars(pWidget->string16->text);
    
    if (tmp && strncmp(password, tmp, MAX_LEN_NAME) == 0) {
      set_wstate(pWidget->prev, FC_WS_NORMAL); /* next button */
      widget_redraw(pWidget->prev);
      widget_flush(pWidget->prev);
    } else {
      memset(password, 0, MAX_LEN_NAME);
      password[0] = '\0';
      
      FC_FREE(pWidget->next->string16->text);/* first edit */
      FC_FREE(pWidget->string16->text); /* secound edit */
      
      set_wstate(pWidget, FC_WS_DISABLED);
      
      widget_redraw(pWidget);
      widget_redraw(pWidget->next);
    
      widget_mark_dirty(pWidget);
      widget_mark_dirty(pWidget->next);
    
      flush_dirty();
    }
  }
  return -1;
}

/**************************************************************************
  ...
**************************************************************************/
static void popup_new_user_passwd_dialog(char *pMessage)
{
  struct widget *pBuf, *pWindow;
  SDL_String16 *pLabelStr = NULL, *pPasswdStr = NULL;
  SDL_Surface *pBackground;
  int start_x, start_y;
  int start_button_y;
  SDL_Rect area;

  queue_flush();
  close_connection_dialog();

  pConnectDlg = fc_calloc(1, sizeof(struct SMALL_DLG));

  pWindow = create_window_skeleton(NULL, NULL, 0);
  add_to_gui_list(ID_WINDOW, pWindow);
  pConnectDlg->pEndWidgetList = pWindow;

  area = pWindow->area;
  
  /* text label */
  pLabelStr = create_str16_from_char(pMessage, adj_font(12));
  pLabelStr->fgcol = *get_game_colorRGB(COLOR_THEME_USERPASSWDDLG_TEXT);
  pBuf = create_iconlabel(NULL, pWindow->dst, pLabelStr,
          (WF_RESTORE_BACKGROUND|WF_DRAW_TEXT_LABEL_WITH_SPACE));
  add_to_gui_list(ID_LABEL, pBuf);
  area.h += adj_size(10) + pBuf->size.h + adj_size(5);

  /* password edit */
  pPasswdStr = create_str16_from_char(pMessage, adj_font(16));
  pPasswdStr->fgcol = *get_game_colorRGB(COLOR_THEME_TEXT);
  pPasswdStr->n_alloc = 0;  
  pBuf = create_edit(NULL, pWindow->dst, pPasswdStr, adj_size(210),
		(WF_PASSWD_EDIT|WF_RESTORE_BACKGROUND|WF_FREE_DATA));
  pBuf->action = convert_first_passwd_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  add_to_gui_list(ID_EDIT, pBuf);
  area.h += pBuf->size.h + adj_size(5);

  /* second password edit */
  pBuf = create_edit(NULL, pWindow->dst, create_string16(NULL, 0, adj_font(16)) , adj_size(210),
		(WF_PASSWD_EDIT|WF_RESTORE_BACKGROUND|WF_FREE_DATA));
  pBuf->action = convert_secound_passwd_callback;
  add_to_gui_list(ID_EDIT, pBuf);
  area.h += pBuf->size.h + adj_size(10);
  
  /* Next button */    
  pBuf = create_themeicon_button_from_chars(pTheme->OK_Icon, pWindow->dst,
						     _("Next"), adj_font(14), 0);
  pBuf->action = send_passwd_callback;
  pBuf->key = SDLK_RETURN;  
  add_to_gui_list(ID_BUTTON, pBuf);
  
  /* Cancel button */
  pBuf = create_themeicon_button_from_chars(pTheme->CANCEL_Icon, pWindow->dst,
						     _("Cancel"), adj_font(14), 0);
  pBuf->action = cancel_connect_dlg_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->key = SDLK_ESCAPE;
  add_to_gui_list(ID_CANCEL_BUTTON, pBuf);
  pBuf->size.w = MAX(pBuf->size.w, pBuf->next->size.w);
  pBuf->next->size.w = pBuf->size.w;
  area.h += pBuf->size.h + adj_size(10);
  
  /* ------------------------------ */
  
  pConnectDlg->pBeginWidgetList = pBuf;

  area.w = pBuf->size.w * 2 + adj_size(40);
  area.w = MAX(area.w, adj_size(210) - (pWindow->size.w - pWindow->area.w));
  area.w = MAX(area.w, pWindow->prev->size.w);
  area.w += adj_size(80);

  pBackground = theme_get_background(theme, BACKGROUND_USERPASSWDDLG);
  if (resize_window(pWindow, pBackground, NULL,
                    (pWindow->size.w - pWindow->area.w) + area.w,
                    (pWindow->size.h - pWindow->area.h) + area.h)) {
    FREESURFACE(pBackground);
  }

  area = pWindow->area;
  
  widget_set_position(pWindow,
                      (Main.screen->w - pWindow->size.w)/ 2,
                      (Main.screen->h - pWindow->size.h)/ 2);

  /* text label */
  pBuf = pConnectDlg->pEndWidgetList->prev;
  
  start_x = area.x + (area.w - pBuf->size.w) / 2;
  start_y = area.y + adj_size(10);
  
  widget_set_area(pBuf, area);
  widget_set_position(pBuf, start_x, start_y);
  
  start_y += pBuf->size.h + adj_size(5);
  
  /* passwd edit */
  pBuf = pBuf->prev;
  start_x = area.x + (area.w - pBuf->size.w) / 2;

  widget_set_area(pBuf, area);  
  widget_set_position(pBuf, start_x, start_y);

  start_y += pBuf->size.h + adj_size(5);
  
  /* retype passwd */
  pBuf = pBuf->prev;
  widget_set_area(pBuf, area);  
  widget_set_position(pBuf, start_x, start_y);
  
  start_button_y = pBuf->size.y + pBuf->size.h + adj_size(10);

  /* connect button */
  pBuf = pBuf->prev;
  widget_set_area(pBuf, area);
  widget_set_position(pBuf,
                      area.x + (area.w - (adj_size(40) + pBuf->size.w * 2)) / 2,
                      start_button_y);
  
  /* cancel button */
  pBuf = pBuf->prev;
  widget_set_area(pBuf, area);
  widget_set_position(pBuf, 
                      pBuf->next->size.x + pBuf->size.w + adj_size(40),
                      start_button_y);

  redraw_group(pConnectDlg->pBeginWidgetList, pConnectDlg->pEndWidgetList, FALSE);

  flush_all();
}

/* ======================================================================== */

/**************************************************************************
 close and destroy the dialog.
**************************************************************************/
void close_connection_dialog(void)
{
  if(pConnectDlg) {
    popdown_window_group_dialog(pConnectDlg->pBeginWidgetList, 
                                pConnectDlg->pEndWidgetList);
    FC_FREE(pConnectDlg);
  }
  if(pMeta_Severs) {
    popdown_window_group_dialog(pMeta_Severs->pBeginWidgetList,
				  pMeta_Severs->pEndWidgetList);
    FC_FREE(pMeta_Severs->pScroll);
    FC_FREE(pMeta_Severs);
    if(pServer_list) {
      server_scan_finish(pServer_scan);
      pServer_scan = NULL;
      pServer_list = NULL;
    }
  }
}

/**************************************************************************
 popup passwd dialog depending on what type of authentication request the
 server is making.
**************************************************************************/
void handle_authentication_req(enum authentication_type type, char *message)
{
  switch (type) {
    case AUTH_NEWUSER_FIRST:
    case AUTH_NEWUSER_RETRY:
      popup_new_user_passwd_dialog(message);
    break;
    case AUTH_LOGIN_FIRST:
    /* if we magically have a password already present in 'password'
     * then, use that and skip the password entry dialog */
    if (password[0] != '\0') {
      struct packet_authentication_reply reply;

      sz_strlcpy(reply.password, password);
      send_packet_authentication_reply(&client.conn, &reply);
      return;
    } else {
      popup_user_passwd_dialog(message);
    }
    break;
    case AUTH_LOGIN_RETRY:
      popup_user_passwd_dialog(message);
    break;
    default:
      assert(0);
  }

}

/**************************************************************************
  Provide an interface for connecting to a FreeCiv server.
  SDLClient use it as popup main start menu which != connecting dlg.
**************************************************************************/
void gui_server_connect(void)
{
}
