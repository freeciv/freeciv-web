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
                          chatline.c  -  description
                             -------------------
    begin                : Sun Jun 30 2002
    copyright            : (C) 2002 by Rafał Bursig
    email                : Rafał Bursig <bursig@poczta.fm>
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

/* SDL2 */
#ifdef SDL2_PLAIN_INCLUDE
#include <SDL.h>
#else  /* SDL2_PLAIN_INCLUDE */
#include <SDL2/SDL.h>
#endif /* SDL2_PLAIN_INCLUDE */

/* utility */
#include "fcintl.h"
#include "log.h"

/* common */
#include "game.h"
#include "packets.h"

/* client */
#include "client_main.h"
#include "clinet.h"
#include "connectdlg_common.h"

/* gui-sdl2 */
#include "colors.h"
#include "dialogs.h"
#include "graphics.h"
#include "gui_iconv.h"
#include "gui_id.h"
#include "gui_main.h"
#include "gui_tilespec.h"
#include "mapview.h"
#include "messagewin.h"
#include "pages.h"
#include "themespec.h"
#include "unistring.h"
#include "utf8string.h"
#include "widget.h"

#include "chatline.h"

#define PTSIZE_LOG_FONT adj_font(10)

struct CONNLIST {
  struct ADVANCED_DLG *pUsers_Dlg;
  struct ADVANCED_DLG *pChat_Dlg;
  struct widget *pBeginWidgetList;
  struct widget *pEndWidgetList;
  struct widget *pStartButton;
  struct widget *pSelectNationButton;
  struct widget *pLoadGameButton;
  struct widget *pConfigure;
  struct widget *pBackButton;
  struct widget *pEdit;
  int text_width;
  int active;
} *pConnDlg = NULL;

static void popup_conn_list_dialog(void);
static void add_to_chat_list(char *msg, size_t n_alloc);

/**************************************************************************
                                  LOAD GAME
**************************************************************************/

struct ADVANCED_DLG *pLoadDialog;

/**********************************************************************//**
  User event to load game dialog window.
**************************************************************************/
static int move_load_game_dlg_callback(struct widget *pWindow)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    move_window_group(pLoadDialog->pBeginWidgetList, pWindow);
  }

  return -1;
}

/**********************************************************************//**
  Close load game dialog
**************************************************************************/
void popdown_load_game_dialog(void)
{
  if (pLoadDialog) {
    popdown_window_group_dialog(pLoadDialog->pBeginWidgetList, pLoadDialog->pEndWidgetList);
    FC_FREE(pLoadDialog->pScroll);
    FC_FREE(pLoadDialog);

    /* enable buttons */
    set_wstate(pConnDlg->pBackButton, FC_WS_NORMAL);
    widget_redraw(pConnDlg->pBackButton);
    widget_mark_dirty(pConnDlg->pBackButton);
    set_wstate(pConnDlg->pLoadGameButton, FC_WS_NORMAL);
    widget_redraw(pConnDlg->pLoadGameButton);
    widget_mark_dirty(pConnDlg->pLoadGameButton);
    set_wstate(pConnDlg->pStartButton, FC_WS_NORMAL);
    widget_redraw(pConnDlg->pStartButton);
    widget_mark_dirty(pConnDlg->pStartButton);

    flush_dirty();
  }
}

/**********************************************************************//**
  User clicked load game dialog close-button.
**************************************************************************/
static int exit_load_dlg_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (get_client_page() == PAGE_LOAD) {
      set_client_page(PAGE_START);
    } else {
      popdown_load_game_dialog();
    }
  }

  return -1;
}

/**********************************************************************//**
  User selected file to load.
**************************************************************************/
static int load_selected_game_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    char *filename = (char*)pWidget->data.ptr;

    if (is_server_running()) {
      send_chat_printf("/load %s", filename);

      if (get_client_page() == PAGE_LOAD) {
        set_client_page(PAGE_START);
      } else if (get_client_page() == PAGE_START) {
        popdown_load_game_dialog();
      }
    } else {
      set_client_page(PAGE_MAIN);
    }
  }

  return -1;
}

/**********************************************************************//**
  Open load game dialog
**************************************************************************/
static void popup_load_game_dialog(void)
{
  struct widget *pWindow;
  struct widget *pCloseButton;
  struct widget *pFilenameLabel = NULL;
  struct widget *pFirstLabel = NULL;
  struct widget *pLastLabel = NULL;
  struct widget *pNextLabel = NULL;
  utf8_str *title, *filename;
  SDL_Rect area;
  struct fileinfo_list *files;
  int count = 0;
  int scrollbar_width = 0;
  int max_label_width = 0;

  if (pLoadDialog) {
    return;
  }

  /* disable buttons */
  set_wstate(pConnDlg->pBackButton, FC_WS_DISABLED);
  widget_redraw(pConnDlg->pBackButton);
  widget_mark_dirty(pConnDlg->pBackButton);
  set_wstate(pConnDlg->pLoadGameButton, FC_WS_DISABLED);
  widget_redraw(pConnDlg->pLoadGameButton);
  widget_mark_dirty(pConnDlg->pLoadGameButton);
  set_wstate(pConnDlg->pSelectNationButton, FC_WS_DISABLED);
  widget_redraw(pConnDlg->pSelectNationButton);
  widget_mark_dirty(pConnDlg->pSelectNationButton);
  set_wstate(pConnDlg->pStartButton, FC_WS_DISABLED);
  widget_redraw(pConnDlg->pStartButton);
  widget_mark_dirty(pConnDlg->pStartButton);

  /* create dialog */
  pLoadDialog = fc_calloc(1, sizeof(struct ADVANCED_DLG));

  title = create_utf8_from_char(_("Choose Saved Game to Load"), adj_font(12));
  title->style |= TTF_STYLE_BOLD;

  pWindow = create_window_skeleton(NULL, title, 0);
  pWindow->action = move_load_game_dlg_callback; 
  set_wstate(pWindow, FC_WS_NORMAL);

  add_to_gui_list(ID_WINDOW, pWindow);

  pLoadDialog->pEndWidgetList = pWindow;

  area = pWindow->area;

  /* close button */
  pCloseButton = create_themeicon(current_theme->Small_CANCEL_Icon, pWindow->dst,
                                  WF_WIDGET_HAS_INFO_LABEL
                                  | WF_RESTORE_BACKGROUND);
  pCloseButton->info_label = create_utf8_from_char(_("Close Dialog (Esc)"),
                                                   adj_font(12));
  pCloseButton->action = exit_load_dlg_callback;
  set_wstate(pCloseButton, FC_WS_NORMAL);
  pCloseButton->key = SDLK_ESCAPE;

  add_to_gui_list(ID_BUTTON, pCloseButton);

  area.w += pCloseButton->size.w;

  pLoadDialog->pBeginWidgetList = pCloseButton;

  /* create scrollbar */
  scrollbar_width = create_vertical_scrollbar(pLoadDialog, 1, 20, TRUE, TRUE);
  hide_scrollbar(pLoadDialog->pScroll);

  /* search for user saved games. */
  files = fileinfolist_infix(get_save_dirs(), ".sav", FALSE);
  fileinfo_list_iterate(files, pfile) {
    count++;

    filename = create_utf8_from_char(pfile->name, adj_font(13));
    filename->style |= SF_CENTER;
    pFilenameLabel = create_iconlabel(NULL, pWindow->dst, filename,
      (WF_FREE_DATA | WF_SELECT_WITHOUT_BAR | WF_RESTORE_BACKGROUND));

    /* store filename */
    pFilenameLabel->data.ptr = fc_calloc(1, strlen(pfile->fullname) + 1);
    fc_strlcpy((char*)pFilenameLabel->data.ptr, pfile->fullname, strlen(pfile->fullname) + 1);

    pFilenameLabel->action = load_selected_game_callback;

    set_wstate(pFilenameLabel, FC_WS_NORMAL);

    /* FIXME: this was supposed to be add_widget_to_vertical_scroll_widget_list(), but
     * add_widget_to_vertical_scroll_widget_list() needs the scrollbar area to be defined
     * for updating the scrollbar position, but the area is not known yet (depends on
     * maximum label width) */
    add_to_gui_list(ID_LABEL, pFilenameLabel);

    if (count == 1) {
      pFirstLabel = pFilenameLabel;
    }

    max_label_width = MAX(max_label_width, pFilenameLabel->size.w);
  } fileinfo_list_iterate_end;
  fileinfo_list_destroy(files);

  pLastLabel = pFilenameLabel;

  area.w = MAX(area.w, max_label_width + scrollbar_width + 1);

  if (count > 0) {
    area.h = (pLoadDialog->pScroll->active * pFilenameLabel->size.h) + adj_size(5);
  }

  resize_window(pWindow, theme_get_background(theme, BACKGROUND_LOADGAMEDLG),
                NULL,
                (pWindow->size.w - pWindow->area.w) + area.w,
                (pWindow->size.h - pWindow->area.h) + area.h);

  area = pWindow->area;

  setup_vertical_scrollbar_area(pLoadDialog->pScroll,
                                area.x + area.w - 1,
                                area.y + 1,
                                area.h - adj_size(2), TRUE);

  /* add filename labels to list */
  pFilenameLabel = pFirstLabel;
  while (pFilenameLabel) {
    pFilenameLabel->size.w = area.w - scrollbar_width - 3;

    pNextLabel = pFilenameLabel->prev;

    del_widget_pointer_from_gui_list(pFilenameLabel);
    if (pFilenameLabel == pFirstLabel) {
      add_widget_to_vertical_scroll_widget_list(pLoadDialog,
          pFilenameLabel, pCloseButton,
          FALSE,
          area.x + 1,
          area.y + adj_size(2));
    } else {
      add_widget_to_vertical_scroll_widget_list(pLoadDialog,
          pFilenameLabel,
          pLoadDialog->pBeginActiveWidgetList,
          FALSE,
          area.x + 1,
          area.y + adj_size(2));
    }

    if (pFilenameLabel == pLastLabel) {
      break;
    }

    pFilenameLabel = pNextLabel;
  }

  widget_set_position(pWindow,
                      (main_window_width() - pWindow->size.w) / 2,
                      (main_window_height() - pWindow->size.h) / 2);

  widget_set_position(pCloseButton,
                      area.x + area.w - pCloseButton->size.w - 1,
                      pWindow->size.y + adj_size(2));

  /* FIXME: the scrollbar already got a background saved in
   * add_widget_to_vertical_scroll_widget_list(), but the window
   * is not drawn yet, so this saved background is wrong.
   * Deleting it here as a workaround. */
  FREESURFACE(pLoadDialog->pScroll->pScrollBar->gfx);

  redraw_group(pLoadDialog->pBeginWidgetList, pWindow, 1);
  flush_dirty();
}

/**********************************************************************//**
  Sent msg/command from input dlg to server
**************************************************************************/
static int inputline_return_callback(struct widget *pWidget)
{
  if (Main.event.type == SDL_KEYDOWN
      && (Main.event.key.keysym.sym == SDLK_RETURN
          || Main.event.key.keysym.sym == SDLK_KP_ENTER)) {

    if (pWidget->string_utf8->text == NULL) {
      return -1;
    }

    if (pWidget->string_utf8->text[0] != '\0') {
      send_chat(pWidget->string_utf8->text);

      output_window_append(ftc_any, pWidget->string_utf8->text);
    }
  }

  return -1;
}

/**********************************************************************//**
  This function is main chat/command client input.
**************************************************************************/
void popup_input_line(void)
{
  struct widget *pInput_Edit;

  pInput_Edit = create_edit_from_chars(NULL, Main.gui, "", adj_font(12),
                                       adj_size(400), 0);

  pInput_Edit->size.x = (main_window_width() - pInput_Edit->size.w) / 2;
  pInput_Edit->size.y = (main_window_height() - pInput_Edit->size.h) / 2;

  if (edit(pInput_Edit) != ED_ESC) {
    inputline_return_callback(pInput_Edit);
  }

  widget_undraw(pInput_Edit);
  widget_mark_dirty(pInput_Edit);
  FREEWIDGET(pInput_Edit);

  flush_dirty();
}

/**********************************************************************//**
  Appends the string to the chat output window.  The string should be
  inserted on its own line, although it will have no newline.
**************************************************************************/
void real_output_window_append(const char *astring,
                               const struct text_tag_list *tags,
                               int conn_id)
{
  /* Currently this is a wrapper to the message subsystem. */
  if (pConnDlg) {
    size_t n = strlen(astring);
    char *buffer = fc_strdup(astring);

    add_to_chat_list(buffer, n);
  } else {
    meswin_add(astring, tags, NULL, E_CHAT_MSG,
               game.info.turn, game.info.phase);
  }
}

/**********************************************************************//**
  Get the text of the output window, and call write_chatline_content() to
  log it.
**************************************************************************/
void log_output_window(void)
{
  /* TODO */
}

/**********************************************************************//**
  Clear all text from the output window.
**************************************************************************/
void clear_output_window(void)
{
  /* TODO */
}

/* ====================================================================== */

/**********************************************************************//**
  User did something to connection list dialog.
**************************************************************************/
static int conn_dlg_callback(struct widget *pWindow)
{
  return -1;
}

/**********************************************************************//**
  User selected to get back from connection list dialog.
**************************************************************************/
static int disconnect_conn_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    popdown_conn_list_dialog();
    flush_dirty();
    disconnect_from_server();
  }

  return -1;
}

/**********************************************************************//**
  Handle chat messages when connection dialog open.
**************************************************************************/
static void add_to_chat_list(char *msg, size_t n_alloc)
{
  utf8_str *pstr;
  struct widget *pBuf, *pWindow = pConnDlg->pEndWidgetList;

  fc_assert_ret(msg != NULL);
  fc_assert_ret(n_alloc != 0);

  pstr = create_utf8_str(msg, n_alloc, adj_font(12));

  if (convert_utf8_str_to_const_surface_width(pstr, pConnDlg->text_width - adj_size(5))) {
    utf8_str *pstr2;
    int count = 0;
    char **utf8_texts = create_new_line_utf8strs(pstr->text);

    while (utf8_texts[count] != NULL) {
      pstr2 = create_utf8_str(utf8_texts[count],
                              strlen(utf8_texts[count]) + 1, adj_font(12));
      pstr2->bgcol = (SDL_Color) {0, 0, 0, 0};
      pBuf = create_themelabel2(NULL, pWindow->dst,
                                pstr2, pConnDlg->text_width, 0,
                                (WF_RESTORE_BACKGROUND|WF_DRAW_TEXT_LABEL_WITH_SPACE));

      pBuf->size.w = pConnDlg->text_width;
      add_widget_to_vertical_scroll_widget_list(pConnDlg->pChat_Dlg, pBuf,
                        pConnDlg->pChat_Dlg->pBeginActiveWidgetList, FALSE,
                        pWindow->size.x + adj_size(10 + 60 + 10),
                        pWindow->size.y + adj_size(14));
      count++;
    }
    redraw_group(pConnDlg->pChat_Dlg->pBeginWidgetList,
                 pConnDlg->pChat_Dlg->pEndWidgetList, TRUE);
    FREEUTF8STR(pstr);
  } else {
    pstr->bgcol = (SDL_Color) {0, 0, 0, 0};
    pBuf = create_themelabel2(NULL, pWindow->dst,
                              pstr, pConnDlg->text_width, 0,
                              (WF_RESTORE_BACKGROUND|WF_DRAW_TEXT_LABEL_WITH_SPACE));

    pBuf->size.w = pConnDlg->text_width;

    if (add_widget_to_vertical_scroll_widget_list(pConnDlg->pChat_Dlg, pBuf,
                        pConnDlg->pChat_Dlg->pBeginActiveWidgetList, FALSE,
                        pWindow->size.x + adj_size(10 + 60 + 10),
                        pWindow->size.y + adj_size(14))) {
      redraw_group(pConnDlg->pChat_Dlg->pBeginWidgetList,
                   pConnDlg->pChat_Dlg->pEndWidgetList, TRUE);
    } else {
      widget_redraw(pBuf);
      widget_mark_dirty(pBuf);
    }
  }

  flush_dirty();
}

/**********************************************************************//**
  User interacted with connection dialog input field.
**************************************************************************/
static int input_edit_conn_callback(struct widget *pWidget)
{
  if (Main.event.type == SDL_KEYDOWN
      && (Main.event.key.keysym.sym == SDLK_RETURN
          || Main.event.key.keysym.sym == SDLK_KP_ENTER)) {
    if (pWidget->string_utf8->text != NULL) {
      if (pWidget->string_utf8->text[0] != '\0') {
        send_chat(pWidget->string_utf8->text);
      }

      free(pWidget->string_utf8->text);
      pWidget->string_utf8->text = fc_malloc(1);
      pWidget->string_utf8->n_alloc = 0;
    }
  }

  return -1;
}

/**********************************************************************//**
   User interacted with Start Game button.
**************************************************************************/
static int start_game_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    send_chat("/start");
  }

  return -1;
}

/**********************************************************************//**
  User interacted with Select Nation button.
**************************************************************************/
static int select_nation_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    popup_races_dialog(client.conn.playing);
  }

  return -1;
}

/* not implemented yet */
#if 0
/**********************************************************************//**
  User interacted with Server Settings button.
**************************************************************************/
static int server_config_callback(struct widget *pWidget)
{
  return -1;
}
#endif

/**********************************************************************//**
  User interacted with Load Game button.
**************************************************************************/
static int load_game_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    /* set_wstate(pConnDlg->pLoadGameButton, FC_WS_NORMAL);
     * widget_redraw(pConnDlg->pLoadGameButton);        
     * flush_dirty(); */
    popup_load_game_dialog();
  }

  return -1;
}

/**********************************************************************//**
  Update the connected users list at pregame state.
**************************************************************************/
void real_conn_list_dialog_update(void *unused)
{
  if (C_S_PREPARING == client_state()) {
    if (pConnDlg) {
      struct widget *pBuf = NULL, *pWindow = pConnDlg->pEndWidgetList;
      utf8_str *pstr = create_utf8_str(NULL, 0, adj_font(12));
      bool create;

      pstr->bgcol = (SDL_Color) {0, 0, 0, 0};

      if (pConnDlg->pUsers_Dlg) {
        del_group(pConnDlg->pUsers_Dlg->pBeginActiveWidgetList,
                  pConnDlg->pUsers_Dlg->pEndActiveWidgetList);
        pConnDlg->pUsers_Dlg->pActiveWidgetList = NULL;
        pConnDlg->pUsers_Dlg->pBeginWidgetList =
          pConnDlg->pUsers_Dlg->pScroll->pScrollBar;
        pConnDlg->pUsers_Dlg->pScroll->count = 0;
      } else {
        pConnDlg->pUsers_Dlg = fc_calloc(1, sizeof(struct ADVANCED_DLG));
        pConnDlg->pUsers_Dlg->pEndWidgetList = pConnDlg->pBeginWidgetList;
        pConnDlg->pUsers_Dlg->pBeginWidgetList = pConnDlg->pBeginWidgetList;

        create_vertical_scrollbar(pConnDlg->pUsers_Dlg, 1,
                                  pConnDlg->active, TRUE, TRUE);
        pConnDlg->pUsers_Dlg->pEndWidgetList =
          pConnDlg->pUsers_Dlg->pEndWidgetList->prev;
        setup_vertical_scrollbar_area(pConnDlg->pUsers_Dlg->pScroll,
          pWindow->size.x + pWindow->size.w - adj_size(30),
          pWindow->size.y + adj_size(14), pWindow->size.h - adj_size(44) - adj_size(40), FALSE);
      }

      hide_scrollbar(pConnDlg->pUsers_Dlg->pScroll);
      create = TRUE;
      conn_list_iterate(game.est_connections, pconn) {
        copy_chars_to_utf8_str(pstr, pconn->username);

        pBuf = create_themelabel2(NULL, pWindow->dst, pstr, adj_size(100), 0,
                (WF_RESTORE_BACKGROUND|WF_DRAW_TEXT_LABEL_WITH_SPACE));
        clear_wflag(pBuf, WF_FREE_STRING);

        pBuf->ID = ID_LABEL;

        /* add to widget list */
        if (create) {
          add_widget_to_vertical_scroll_widget_list(pConnDlg->pUsers_Dlg,
            pBuf, pConnDlg->pUsers_Dlg->pBeginWidgetList, FALSE,
            pWindow->area.x + pWindow->area.w - adj_size(130),
            pWindow->size.y + adj_size(14));
          create = FALSE;
        } else {
	  add_widget_to_vertical_scroll_widget_list(pConnDlg->pUsers_Dlg,
                pBuf, pConnDlg->pUsers_Dlg->pBeginActiveWidgetList, FALSE,
                pWindow->area.x + pWindow->area.w - adj_size(130),
                pWindow->size.y + adj_size(14));
        }
      } conn_list_iterate_end;

      pConnDlg->pBeginWidgetList = pConnDlg->pUsers_Dlg->pBeginWidgetList;
      FREEUTF8STR(pstr);

/* FIXME: implement the server settings dialog and then reactivate this part */
#if 0
      if (ALLOW_CTRL == client.conn.access_level
          || ALLOW_HACK == client.conn.access_level) {
        set_wstate(pConnDlg->pConfigure, FC_WS_NORMAL);
      } else {
        set_wstate(pConnDlg->pConfigure, FC_WS_DISABLED);
      }
#endif

      /* redraw */
      redraw_group(pConnDlg->pBeginWidgetList, pConnDlg->pEndWidgetList, 0);

      widget_flush(pConnDlg->pEndWidgetList);
    } else {
      popup_conn_list_dialog();
    }

    /* PAGE_LOAD -> the server was started from the main page to load a game */
    if (get_client_page() == PAGE_LOAD) {
      popup_load_game_dialog();
    }
  } else {
    if (popdown_conn_list_dialog()) {
      flush_dirty();
    }
  }
}

/**********************************************************************//**
  Open connection list dialog
**************************************************************************/
static void popup_conn_list_dialog(void)
{
  SDL_Color window_bg_color = {255, 255, 255, 96};

  struct widget *pWindow = NULL, *pBuf = NULL, *pLabel = NULL;
  struct widget* pBackButton = NULL;
  struct widget *pStartGameButton = NULL;
  struct widget *pSelectNationButton = NULL;
/*  struct widget *pServerSettingsButton = NULL;*/
  utf8_str *pstr = NULL;
  int n;
  SDL_Rect area;
  SDL_Surface *pSurf;

  if (pConnDlg || !client.conn.established) {
    return;
  }

  meswin_dialog_popdown();

  pConnDlg = fc_calloc(1, sizeof(struct CONNLIST));

  pWindow = create_window_skeleton(NULL, NULL, 0);
  pWindow->action = conn_dlg_callback;
  set_wstate(pWindow, FC_WS_NORMAL);
  clear_wflag(pWindow, WF_DRAW_FRAME_AROUND_WIDGET);

  pConnDlg->pEndWidgetList = pWindow;
  add_to_gui_list(ID_WINDOW, pWindow);

  widget_set_position(pWindow, 0, 0);

  /* create window background */
  pSurf = theme_get_background(theme, BACKGROUND_CONNLISTDLG);
  if (resize_window(pWindow, pSurf, NULL, main_window_width(), main_window_height())) {
    FREESURFACE(pSurf);
  }

  pConnDlg->text_width = pWindow->size.w - adj_size(130) - adj_size(20) - adj_size(20);

  /* chat area background */
  area.x = adj_size(10);
  area.y = adj_size(14);
  area.w = pConnDlg->text_width + adj_size(20);
  area.h = pWindow->size.h - adj_size(44) - adj_size(40);
  fill_rect_alpha(pWindow->theme, &area, &window_bg_color);

  create_frame(pWindow->theme,
               area.x - 1, area.y - 1, area.w + 1, area.h + 1,
               get_theme_color(COLOR_THEME_CONNLISTDLG_FRAME));

  /* user list background */
  area.x = pWindow->size.w - adj_size(130);
  area.y = adj_size(14);
  area.w = adj_size(120);
  area.h = pWindow->size.h - adj_size(44) - adj_size(40);
  fill_rect_alpha(pWindow->theme, &area, &window_bg_color);

  create_frame(pWindow->theme,
               area.x - 1, area.y - 1, area.w + 1, area.h + 1,
               get_theme_color(COLOR_THEME_CONNLISTDLG_FRAME));

  draw_frame(pWindow->theme, 0, 0, pWindow->theme->w, pWindow->theme->h);

  /* -------------------------------- */

  /* chat area */

  pConnDlg->pChat_Dlg = fc_calloc(1, sizeof(struct ADVANCED_DLG));

  n = conn_list_size(game.est_connections);

  {
    char cbuf[256];

    fc_snprintf(cbuf, sizeof(cbuf), _("Total users logged in : %d"), n);
    pstr = create_utf8_from_char(cbuf, adj_font(12));
  }

  pstr->bgcol = (SDL_Color) {0, 0, 0, 0};

  pLabel = create_themelabel2(NULL, pWindow->dst,
                              pstr, pConnDlg->text_width, 0,
                              (WF_RESTORE_BACKGROUND|WF_DRAW_TEXT_LABEL_WITH_SPACE));

  widget_set_position(pLabel, adj_size(10), adj_size(14));

  add_to_gui_list(ID_LABEL, pLabel);

  pConnDlg->pChat_Dlg->pBeginWidgetList = pLabel;
  pConnDlg->pChat_Dlg->pEndWidgetList = pLabel;
  pConnDlg->pChat_Dlg->pBeginActiveWidgetList = pConnDlg->pChat_Dlg->pBeginWidgetList;
  pConnDlg->pChat_Dlg->pEndActiveWidgetList = pConnDlg->pChat_Dlg->pEndWidgetList;

  n = (pWindow->size.h - adj_size(44) - adj_size(40)) / pLabel->size.h;
  pConnDlg->active = n;

  create_vertical_scrollbar(pConnDlg->pChat_Dlg, 1,
                            pConnDlg->active, TRUE, TRUE);

  setup_vertical_scrollbar_area(pConnDlg->pChat_Dlg->pScroll,
                adj_size(10) + pConnDlg->text_width + 1,
                adj_size(14), pWindow->size.h - adj_size(44) - adj_size(40), FALSE);
  hide_scrollbar(pConnDlg->pChat_Dlg->pScroll);

  /* -------------------------------- */

  /* input field */

  pBuf = create_edit_from_chars(NULL, pWindow->dst, "",
                                adj_font(12), pWindow->size.w - adj_size(10) - adj_size(10),
                                (WF_RESTORE_BACKGROUND|WF_EDIT_LOOP));

  pBuf->size.x = adj_size(10);
  pBuf->size.y = pWindow->size.h - adj_size(40) - adj_size(5) - pBuf->size.h;
  pBuf->action = input_edit_conn_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  pConnDlg->pEdit = pBuf;
  add_to_gui_list(ID_EDIT, pBuf);

  /* buttons */

  pBuf = create_themeicon_button_from_chars(current_theme->BACK_Icon, pWindow->dst,
                                            _("Back"), adj_font(12), 0);
  pBuf->size.x = adj_size(10);
  pBuf->size.y = pWindow->size.h - adj_size(10) - pBuf->size.h;
  pConnDlg->pBackButton = pBuf;
  pBuf->action = disconnect_conn_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->key = SDLK_ESCAPE;
  add_to_gui_list(ID_BUTTON, pBuf);
  pBackButton = pBuf;

  pBuf = create_themeicon_button_from_chars(current_theme->OK_Icon, pWindow->dst,
                                            _("Start"), adj_font(12), 0);
  pBuf->size.x = pWindow->size.w - adj_size(10) - pBuf->size.w;
  pBuf->size.y = pBackButton->size.y;
  pConnDlg->pStartButton = pBuf;
  pBuf->action = start_game_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  add_to_gui_list(ID_BUTTON, pBuf);
  pStartGameButton = pBuf;

  pBuf = create_themeicon_button_from_chars(NULL, pWindow->dst,
                                            _("Pick Nation"), adj_font(12), 0);
  pBuf->size.h = pStartGameButton->size.h;
  pBuf->size.x = pStartGameButton->size.x - adj_size(10) - pBuf->size.w;
  pBuf->size.y = pStartGameButton->size.y;
  pConnDlg->pSelectNationButton = pBuf;
  pBuf->action = select_nation_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  add_to_gui_list(ID_BUTTON, pBuf);
  pSelectNationButton = pBuf;

  pBuf = create_themeicon_button_from_chars(NULL, pWindow->dst,
                                            _("Load Game"), adj_font(12), 0);
  pBuf->size.h = pSelectNationButton->size.h;
  pBuf->size.x = pSelectNationButton->size.x - adj_size(10) - pBuf->size.w;
  pBuf->size.y = pSelectNationButton->size.y;
  pConnDlg->pLoadGameButton = pBuf;
  pBuf->action = load_game_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  add_to_gui_list(ID_BUTTON, pBuf);

  /* not implemented yet */
#if 0
  pBuf = create_themeicon_button_from_chars(NULL, pWindow->dst,
                                            _("Server Settings"),
                                            adj_font(12), 0);
  pBuf->size.h = pSelectNationButton->size.h;
  pBuf->size.x = pSelectNationButton->size.x - adj_size(10) - pBuf->size.w;
  pBuf->size.y = pSelectNationButton->size.y;
  pConnDlg->pConfigure = pBuf;
  pBuf->action = server_config_callback;
  set_wstate(pBuf, FC_WS_DISABLED);
  add_to_gui_list(ID_BUTTON, pBuf);
  pServerSettingsButton = pBuf;
#endif

  /* not implemented yet */
#if 0
  pBuf = create_themeicon_button_from_chars(NULL, pWindow->dst->surface,
                                            "?", adj_font(12), 0);
  pBuf->size.y = pWindow->size.y + pWindow->size.h - (pBuf->size.h + 7);
  pBuf->size.x = pWindow->size.x + pWindow->size.w - (pBuf->size.w + 10) - 5;

  pBuf->action = client_config_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  add_to_gui_list(ID_BUTTON, pBuf);
#endif

  pConnDlg->pBeginWidgetList = pBuf;
  /* ------------------------------------------------------------ */

  conn_list_dialog_update();
}

/**********************************************************************//**
  Close connection list dialog.
**************************************************************************/
bool popdown_conn_list_dialog(void)
{
  if (pConnDlg) {
    if (get_wstate(pConnDlg->pEdit) == FC_WS_PRESSED) {
      force_exit_from_event_loop();
    }

    popdown_window_group_dialog(pConnDlg->pBeginWidgetList,
                                pConnDlg->pEndWidgetList);
    if (pConnDlg->pUsers_Dlg) {
      FC_FREE(pConnDlg->pUsers_Dlg->pScroll);
      FC_FREE(pConnDlg->pUsers_Dlg);
    }

    if (pConnDlg->pChat_Dlg) {
      FC_FREE(pConnDlg->pChat_Dlg->pScroll);
      FC_FREE(pConnDlg->pChat_Dlg);
    }

    FC_FREE(pConnDlg);

    return TRUE;
  }

  return FALSE;
}

/**********************************************************************//**
  Got version message from metaserver thread.
**************************************************************************/
void version_message(const char *vertext)
{
  output_window_append(ftc_client, vertext);
}
