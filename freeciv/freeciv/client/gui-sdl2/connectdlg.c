/***********************************************************************
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

/***********************************************************************
                          connectdlg.c  -  description
                             -------------------
    begin                : Mon Jul 1 2002
    copyright            : (C) 2002 by Rafał Bursig
    email                : Rafał Bursig <bursig@poczta.fm>
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <stdlib.h>

/* SDL2 */
#ifdef SDL2_PLAIN_INCLUDE
#include <SDL.h>
#else  /* SDL2_PLAIN_INCLUDE */
#include <SDL2/SDL.h>
#endif /* SDL2_PLAIN_INCLUDE */

/* utility */
#include "fcintl.h"
#include "log.h"

/* client */
#include "client_main.h"
#include "clinet.h"        /* connect_to_server() */
#include "packhand.h"
#include "servers.h"

/* gui-sdl2 */
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

static struct server_list *pServer_list = NULL;
static struct server_scan *pServer_scan = NULL;

static struct ADVANCED_DLG *pMeta_Server = NULL;
static struct SMALL_DLG *pConnectDlg = NULL;

static int connect_callback(struct widget *pWidget);
static int convert_portnr_callback(struct widget *pWidget);
static int convert_playername_callback(struct widget *pWidget);
static int convert_servername_callback(struct widget *pWidget);
static void popup_new_user_passwd_dialog(const char *pMessage);

/*
  THESE FUNCTIONS ARE ONE BIG TMP SOLUTION AND SHOULD BE FULLY REWRITTEN !!
*/

/**********************************************************************//**
  Provide a packet handler for packet_game_load
**************************************************************************/
void handle_game_load(bool load_successful, const char *filename)
{ 
  /* PORTME */
}

/**********************************************************************//**
  User interacted with connect -widget
**************************************************************************/
static int connect_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    char errbuf[512];

    if (connect_to_server(user_name, server_host, server_port,
                          errbuf, sizeof(errbuf)) != -1) {
    } else {
      output_window_append(ftc_any, errbuf);

      /* button up */
      unselect_widget_action();
      set_wstate(pWidget, FC_WS_SELECTED);
      selected_widget = pWidget;
      widget_redraw(pWidget);
      widget_flush(pWidget);
    }
  }

  return -1;
}
/* ======================================================== */


/**********************************************************************//**
  User interacted with server list window.
**************************************************************************/
static int meta_server_window_callback(struct widget *pWindow)
{
  return -1;
}

/**********************************************************************//**
  Close servers dialog.
**************************************************************************/
static int exit_meta_server_dlg_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    queue_flush();

    server_scan_finish(pServer_scan);
    pServer_scan = NULL;
    pServer_list = NULL;

    set_client_page(PAGE_NETWORK);
    meswin_dialog_popup(TRUE);
  }

  return -1;
}

/**********************************************************************//**
  Server selected from dialog.
**************************************************************************/
static int select_meta_servers_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct server *pServer = (struct server *)pWidget->data.ptr;

    sz_strlcpy(server_host, pServer->host);
    server_port = pServer->port;

    exit_meta_server_dlg_callback(NULL);
  }

  return -1;
}

/**********************************************************************//**
  Callback function for when there's an error in the server scan.
**************************************************************************/
static void server_scan_error(struct server_scan *scan,
                              const char *message)
{
  output_window_append(ftc_client, message);
  log_normal("%s", message);

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

/**********************************************************************//**
  SDL wrapper on create_server_list(...) function witch add
  same functionality for LAN server dettection.
  WARING !: for LAN scan use "finish_lanserver_scan()" to free server list.
**************************************************************************/
static struct srv_list *sdl_create_server_list(bool lan)
{
  struct srv_list *list = NULL;
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
    list = server_scan_get_list(pServer_scan);
    if (list) {
      break;
    }
    SDL_Delay(100);
  }

  return list;
}

/**********************************************************************//**
  Open connection dialog for either meta or lan scan.
**************************************************************************/
void popup_connection_dialog(bool lan_scan)
{
  SDL_Color bg_color = {255, 255, 255, 128};
  char cBuf[512];
  int w = 0, h = 0, count = 0, meta_h;
  struct widget *pNewWidget, *pWindow, *pLabelWindow;
  utf8_str *pstr;
  SDL_Surface *pLogo;
  SDL_Rect area, area2;
  struct srv_list *srvrs;

  queue_flush();
  close_connection_dialog();
  meswin_dialog_popdown();

  /* Text Label */
  pLabelWindow = create_window_skeleton(NULL, NULL, 0);
  add_to_gui_list(ID_WINDOW, pLabelWindow);

  area = pLabelWindow->area;

  fc_snprintf(cBuf, sizeof(cBuf), _("Creating Server List..."));
  pstr = create_utf8_from_char(cBuf, adj_font(16));
  pstr->style = TTF_STYLE_BOLD;
  pstr->bgcol = (SDL_Color) {0, 0, 0, 0};
  pNewWidget = create_iconlabel(NULL, pLabelWindow->dst, pstr,
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
                      (main_window_width() - pLabelWindow->size.w) / 2,
                      (main_window_height() - pLabelWindow->size.h) / 2);

  widget_set_area(pNewWidget, area);
  widget_set_position(pNewWidget,
                      area.x + (area.w - pNewWidget->size.w) / 2,
                      area.y + (area.h - pNewWidget->size.h) / 2);

  redraw_group(pNewWidget, pLabelWindow, TRUE);
  flush_dirty();

  /* create server list */
  srvrs = sdl_create_server_list(lan_scan);

  /* Copy list */
  pServer_list = server_list_new();
  fc_allocate_mutex(&srvrs->mutex);
  server_list_iterate(srvrs->servers, pserver) {
    server_list_append(pServer_list, pserver);
  } server_list_iterate_end;
  fc_release_mutex(&srvrs->mutex);

  /* clear label */
  popdown_window_group_dialog(pNewWidget, pLabelWindow);

  meswin_dialog_popup(TRUE);

  if (!pServer_list) {
    if (lan_scan) {
      output_window_append(ftc_client, _("No LAN servers found")); 
    } else {
      output_window_append(ftc_client, _("No public servers found"));
    }
    set_client_page(PAGE_NETWORK);
    return;
  }

  /* Server list window */
  pMeta_Server = fc_calloc(1, sizeof(struct ADVANCED_DLG));

  pWindow = create_window_skeleton(NULL, NULL, 0);
  pWindow->action = meta_server_window_callback;
  set_wstate(pWindow, FC_WS_NORMAL);
  clear_wflag(pWindow, WF_DRAW_FRAME_AROUND_WIDGET);
  if (lan_scan) {
    add_to_gui_list(ID_LAN_SERVERS_WINDOW, pWindow);
  } else {
    add_to_gui_list(ID_META_SERVERS_WINDOW, pWindow);
  }
  pMeta_Server->pEndWidgetList = pWindow;

  area = pWindow->area;

  /* Cancel button */
  pNewWidget = create_themeicon_button_from_chars(current_theme->CANCEL_Icon,
                                                  pWindow->dst, _("Cancel"),
                                                  adj_font(14), 0);
  pNewWidget->action = exit_meta_server_dlg_callback;
  set_wstate(pNewWidget, FC_WS_NORMAL);
  add_to_gui_list(ID_BUTTON, pNewWidget);

  /* servers */
  server_list_iterate(pServer_list, pServer) {

    /* TRANS: "host.example.com Port 5556 Ver: 2.6.0 Running Players 3\n
     * [server message]" */
    fc_snprintf(cBuf, sizeof(cBuf), _("%s Port %d Ver: %s %s %s %d\n%s"),
                pServer->host, pServer->port, pServer->version, _(pServer->state),
                Q_("?header:Players"), pServer->nplayers, pServer->message);

    pNewWidget = create_iconlabel_from_chars(NULL, pWindow->dst, cBuf, adj_font(10),
                     WF_FREE_STRING|WF_DRAW_TEXT_LABEL_WITH_SPACE|WF_RESTORE_BACKGROUND);

    pNewWidget->string_utf8->style |= SF_CENTER;
    pNewWidget->string_utf8->bgcol = (SDL_Color) {0, 0, 0, 0};

    pNewWidget->action = select_meta_servers_callback;
    set_wstate(pNewWidget, FC_WS_NORMAL);
    pNewWidget->data.ptr = (void *)pServer;

    add_to_gui_list(ID_BUTTON, pNewWidget);

    w = MAX(w, pNewWidget->size.w);
    h = MAX(h, pNewWidget->size.h);
    count++;

    if (count > 10) {
      set_wflag(pNewWidget, WF_HIDDEN);
    }

  } server_list_iterate_end;

  if (!count) {
    if (lan_scan) {
      output_window_append(ftc_client, _("No LAN servers found"));
    } else {
      output_window_append(ftc_client, _("No public servers found"));
    }
    set_client_page(PAGE_NETWORK);
    return;
  }

  pMeta_Server->pBeginWidgetList = pNewWidget;
  pMeta_Server->pBeginActiveWidgetList = pMeta_Server->pBeginWidgetList;
  pMeta_Server->pEndActiveWidgetList = pMeta_Server->pEndWidgetList->prev->prev;
  pMeta_Server->pActiveWidgetList = pMeta_Server->pEndActiveWidgetList;

  if (count > 10) {
    meta_h = 10 * h;

    count = create_vertical_scrollbar(pMeta_Server, 1, 10, TRUE, TRUE);
    w += count;
  } else {
    meta_h = count * h;
  }

  w += adj_size(20);
  area2.h = meta_h;

  meta_h += pMeta_Server->pEndWidgetList->prev->size.h + adj_size(10) + adj_size(20);

  pLogo = theme_get_background(theme, BACKGROUND_CONNECTDLG);
  if (resize_window(pWindow, pLogo, NULL, w, meta_h)) {
    FREESURFACE(pLogo);
  }

  area = pWindow->area;

  widget_set_position(pWindow,
                      (main_window_width() - w) / 2,
                      (main_window_height() - meta_h) / 2);

  w -= adj_size(20);

  area2.w = w + 1;

  if (pMeta_Server->pScroll) {
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
  while (pNewWidget) {
    pNewWidget->size.w = w;
    pNewWidget->size.h = h;
    pNewWidget->size.x = pNewWidget->next->size.x;
    pNewWidget->size.y = pNewWidget->next->size.y + pNewWidget->next->size.h;
    pNewWidget = convert_iconlabel_to_themeiconlabel2(pNewWidget);

    if (pNewWidget == pMeta_Server->pBeginActiveWidgetList) {
      break;
    }
    pNewWidget = pNewWidget->prev;
  }

  if (pMeta_Server->pScroll) {
    setup_vertical_scrollbar_area(pMeta_Server->pScroll,
                                  area.x + area.w - adj_size(6),
                                  pMeta_Server->pEndActiveWidgetList->size.y,
                                  area.h - adj_size(24) - pWindow->prev->size.h, TRUE);
  }

  /* -------------------- */
  /* redraw */

  widget_redraw(pWindow);

  area2.x = pMeta_Server->pEndActiveWidgetList->size.x;
  area2.y = pMeta_Server->pEndActiveWidgetList->size.y;

  fill_rect_alpha(pWindow->dst->surface, &area2, &bg_color);

  create_frame(pWindow->dst->surface,
               area2.x - 1, area2.y - 1, area2.w, area2.h,
               get_theme_color(COLOR_THEME_CONNECTDLG_INNERFRAME));

  redraw_group(pMeta_Server->pBeginWidgetList, pWindow->prev, 0);

  create_frame(pWindow->dst->surface,
               pWindow->size.x, pWindow->size.y,
               area.w - 1, area.h - 1,
               get_theme_color(COLOR_THEME_CONNECTDLG_FRAME));

  widget_flush(pWindow);
}

/**********************************************************************//**
  User interacted with playername widget.
**************************************************************************/
static int convert_playername_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (pWidget->string_utf8->text != NULL) {
      sz_strlcpy(user_name, pWidget->string_utf8->text);
    } else {
      /* empty input -> restore previous content */
      copy_chars_to_utf8_str(pWidget->string_utf8, user_name);
      widget_redraw(pWidget);
      widget_mark_dirty(pWidget);
      flush_dirty();
    }
  }

  return -1;
}

/**********************************************************************//**
  User interacted with servername widget.
**************************************************************************/
static int convert_servername_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (pWidget->string_utf8->text != NULL) {
      sz_strlcpy(server_host, pWidget->string_utf8->text);
    } else {
      /* empty input -> restore previous content */
      copy_chars_to_utf8_str(pWidget->string_utf8, server_host);
      widget_redraw(pWidget);
      widget_mark_dirty(pWidget);
      flush_dirty();
    }
  }

  return -1;
}

/**********************************************************************//**
  User interacted with port number widget.
**************************************************************************/
static int convert_portnr_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    char pCharPort[6];

    if (pWidget->string_utf8->text != NULL) {
      sscanf(pWidget->string_utf8->text, "%d", &server_port);
    } else {
      /* empty input -> restore previous content */
      fc_snprintf(pCharPort, sizeof(pCharPort), "%d", server_port);
      copy_chars_to_utf8_str(pWidget->string_utf8, pCharPort);
      widget_redraw(pWidget);
      widget_mark_dirty(pWidget);
      flush_dirty();
    }
  }

  return -1;
}

/**********************************************************************//**
  User interacted with cancel -button
**************************************************************************/
static int cancel_connect_dlg_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    close_connection_dialog();
    set_client_page(PAGE_MAIN);
  }

  return -1;
}

/**********************************************************************//**
  Open dialog for joining to game.
**************************************************************************/
void popup_join_game_dialog()
{
  char pCharPort[6];
  struct widget *pBuf, *pWindow;
  utf8_str *plrname = NULL;
  utf8_str *srvname = NULL;
  utf8_str *port_nr = NULL;
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
  plrname = create_utf8_from_char(_("Player Name :"), adj_font(10));
  plrname->fgcol = *get_theme_color(COLOR_THEME_JOINGAMEDLG_TEXT);
  pBuf = create_iconlabel(NULL, pWindow->dst, plrname,
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
  srvname = create_utf8_from_char(_("Freeciv Server :"), adj_font(10));
  srvname->fgcol = *get_theme_color(COLOR_THEME_JOINGAMEDLG_TEXT);
  pBuf = create_iconlabel(NULL, pWindow->dst, srvname,
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
  port_nr = create_utf8_from_char(_("Port :"), adj_font(10));
  port_nr->fgcol = *get_theme_color(COLOR_THEME_JOINGAMEDLG_TEXT);
  pBuf = create_iconlabel(NULL, pWindow->dst, port_nr,
          (WF_RESTORE_BACKGROUND|WF_DRAW_TEXT_LABEL_WITH_SPACE));
  add_to_gui_list(ID_LABEL, pBuf);
  area.h += pBuf->size.h + adj_size(5);

  /* port edit */
  fc_snprintf(pCharPort, sizeof(pCharPort), "%d", server_port);

  pBuf = create_edit_from_chars(NULL, pWindow->dst, pCharPort, adj_font(14), adj_size(210),
                                WF_RESTORE_BACKGROUND);

  pBuf->action = convert_portnr_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  add_to_gui_list(ID_PORT_EDIT, pBuf);
  area.h += pBuf->size.h + adj_size(20);

  /* Connect button */
  pBuf = create_themeicon_button_from_chars(current_theme->OK_Icon, pWindow->dst,
                                            _("Connect"), adj_font(14), 0);
  pBuf->action = connect_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->key = SDLK_RETURN;
  add_to_gui_list(ID_CONNECT_BUTTON, pBuf);

  /* Cancel button */
  pBuf = create_themeicon_button_from_chars(current_theme->CANCEL_Icon, pWindow->dst,
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
                      (main_window_width() - pWindow->size.w) / 2,
                      (main_window_height() - pWindow->size.h) / 2 + adj_size(40));

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

/**********************************************************************//**
  User interacted with password widget
**************************************************************************/
static int convert_passwd_callback(struct widget *pWidget)
{
  if (Main.event.type == SDL_KEYDOWN
      || (Main.event.type == SDL_MOUSEBUTTONDOWN
          && Main.event.button.button == SDL_BUTTON_LEFT)) {
    if (pWidget->string_utf8->text != NULL) {
      fc_snprintf(password, MAX_LEN_NAME, "%s", pWidget->string_utf8->text);
    }
  }

  return -1;
}

/**********************************************************************//**
  User interacted with "Next" -button after entering password.
**************************************************************************/
static int send_passwd_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct packet_authentication_reply reply;

    sz_strlcpy(reply.password, password);

    memset(password, 0, MAX_LEN_NAME);
    password[0] = '\0';

    set_wstate(pWidget, FC_WS_DISABLED);

    widget_redraw(pWidget);

    widget_mark_dirty(pWidget);
    
    flush_dirty();

    send_packet_authentication_reply(&client.conn, &reply);
  }

  return -1;
}

/**********************************************************************//**
  Close password dialog with no password given.
**************************************************************************/
static int cancel_passwd_callback(struct widget *pWidget)
{
  memset(password, 0, MAX_LEN_NAME);
  password[0] = '\0';
  disconnect_from_server();

  return cancel_connect_dlg_callback(pWidget);
}

/**********************************************************************//**
  Open password dialog.
**************************************************************************/
static void popup_user_passwd_dialog(const char *pMessage)
{
  struct widget *pBuf, *pWindow;
  utf8_str *label_str = NULL;
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
  label_str = create_utf8_from_char(pMessage, adj_font(12));
  label_str->fgcol = *get_theme_color(COLOR_THEME_USERPASSWDDLG_TEXT);
  pBuf = create_iconlabel(NULL, pWindow->dst, label_str,
                          (WF_RESTORE_BACKGROUND|WF_DRAW_TEXT_LABEL_WITH_SPACE));
  add_to_gui_list(ID_LABEL, pBuf);
  area.h += adj_size(10) + pBuf->size.h + adj_size(5);

  /* password edit */
  pBuf = create_edit(NULL, pWindow->dst, create_utf8_str(NULL, 0, adj_font(16)),
                     adj_size(210),
                     (WF_PASSWD_EDIT|WF_RESTORE_BACKGROUND|WF_FREE_DATA));
  pBuf->action = convert_passwd_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  add_to_gui_list(ID_EDIT, pBuf);
  area.h += pBuf->size.h + adj_size(10);

  /* Next button */
  pBuf = create_themeicon_button_from_chars(current_theme->OK_Icon, pWindow->dst,
                                            _("Next"), adj_font(14), 0);
  pBuf->action = send_passwd_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->key = SDLK_RETURN;
  add_to_gui_list(ID_BUTTON, pBuf);

  /* Cancel button */
  pBuf = create_themeicon_button_from_chars(current_theme->CANCEL_Icon, pWindow->dst,
                                            _("Cancel"), adj_font(14), 0);
  pBuf->action = cancel_passwd_callback;
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
                      (main_window_width() - pWindow->size.w) / 2,
                      (main_window_height() - pWindow->size.h) / 2);

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

/**********************************************************************//**
  New Password
**************************************************************************/
static int convert_first_passwd_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (pWidget->string_utf8->text != NULL) {
      fc_snprintf(password, MAX_LEN_NAME, "%s", pWidget->string_utf8->text);
      set_wstate(pWidget->prev, FC_WS_NORMAL);
      widget_redraw(pWidget->prev);
      widget_flush(pWidget->prev);
    }
  }

  return -1;
}

/**********************************************************************//**
  Verify Password
**************************************************************************/
static int convert_second_passwd_callback(struct widget *pWidget)
{
  if (Main.event.type == SDL_KEYDOWN
      || (Main.event.type == SDL_MOUSEBUTTONDOWN
          && Main.event.button.button == SDL_BUTTON_LEFT)) {
    if (pWidget->string_utf8->text != NULL
        && !strncmp(password, pWidget->string_utf8->text, MAX_LEN_NAME)) {
      set_wstate(pWidget->prev, FC_WS_NORMAL); /* next button */
      widget_redraw(pWidget->prev);
      widget_flush(pWidget->prev);
    } else {
      memset(password, 0, MAX_LEN_NAME);
      password[0] = '\0';

      FC_FREE(pWidget->next->string_utf8->text);/* first edit */
      FC_FREE(pWidget->string_utf8->text); /* second edit */

      popup_new_user_passwd_dialog(_("Passwords don't match, enter password."));
    }
  }

  return -1;
}

/**********************************************************************//**
  Open dialog for new password.
**************************************************************************/
static void popup_new_user_passwd_dialog(const char *pMessage)
{
  struct widget *pBuf, *pWindow;
  utf8_str *label_str = NULL;
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
  label_str = create_utf8_from_char(pMessage, adj_font(12));
  label_str->fgcol = *get_theme_color(COLOR_THEME_USERPASSWDDLG_TEXT);
  pBuf = create_iconlabel(NULL, pWindow->dst, label_str,
                          (WF_RESTORE_BACKGROUND|WF_DRAW_TEXT_LABEL_WITH_SPACE));
  add_to_gui_list(ID_LABEL, pBuf);
  area.h += adj_size(10) + pBuf->size.h + adj_size(5);

  /* password edit */
  pBuf = create_edit(NULL, pWindow->dst, create_utf8_str(NULL, 0, adj_font(16)),
                     adj_size(210),
                     (WF_PASSWD_EDIT|WF_RESTORE_BACKGROUND|WF_FREE_DATA));
  pBuf->action = convert_first_passwd_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  add_to_gui_list(ID_EDIT, pBuf);
  area.h += pBuf->size.h + adj_size(5);

  /* second password edit */
  pBuf = create_edit(NULL, pWindow->dst, create_utf8_str(NULL, 0, adj_font(16)),
                     adj_size(210),
                     (WF_PASSWD_EDIT|WF_RESTORE_BACKGROUND|WF_FREE_DATA));
  pBuf->action = convert_second_passwd_callback;
  add_to_gui_list(ID_EDIT, pBuf);
  area.h += pBuf->size.h + adj_size(10);

  /* Next button */
  pBuf = create_themeicon_button_from_chars(current_theme->OK_Icon, pWindow->dst,
                                            _("Next"), adj_font(14), 0);
  pBuf->action = send_passwd_callback;
  pBuf->key = SDLK_RETURN;
  add_to_gui_list(ID_BUTTON, pBuf);

  /* Cancel button */
  pBuf = create_themeicon_button_from_chars(current_theme->CANCEL_Icon, pWindow->dst,
                                            _("Cancel"), adj_font(14), 0);
  pBuf->action = cancel_passwd_callback;
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
                      (main_window_width() - pWindow->size.w) / 2,
                      (main_window_height() - pWindow->size.h) / 2);

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

/**********************************************************************//**
  Close and destroy the dialog.
**************************************************************************/
void close_connection_dialog(void)
{
  if (pConnectDlg) {
    popdown_window_group_dialog(pConnectDlg->pBeginWidgetList,
                                pConnectDlg->pEndWidgetList);
    FC_FREE(pConnectDlg);
  }
  if (pMeta_Server) {
    popdown_window_group_dialog(pMeta_Server->pBeginWidgetList,
                                pMeta_Server->pEndWidgetList);
    FC_FREE(pMeta_Server->pScroll);
    FC_FREE(pMeta_Server);

    if (pServer_list) {
      server_scan_finish(pServer_scan);
      pServer_scan = NULL;
      pServer_list = NULL;
    }
  }
}

/**********************************************************************//**
  Popup passwd dialog depending on what type of authentication request the
  server is making.
**************************************************************************/
void handle_authentication_req(enum authentication_type type,
                               const char *message)
{
  switch (type) {
  case AUTH_NEWUSER_FIRST:
  case AUTH_NEWUSER_RETRY:
    popup_new_user_passwd_dialog(message);
    return;
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

    return;
  case AUTH_LOGIN_RETRY:
    popup_user_passwd_dialog(message);

    return;
  }

  log_error("Not supported authentication type %d: %s.", type, message);
}

/**********************************************************************//**
  Provide an interface for connecting to a Freeciv server.
  sdl2-client does it as popup main start menu which != connecting dlg.
**************************************************************************/
void server_connect(void)
{
}
