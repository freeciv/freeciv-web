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

/**********************************************************************
                          messagewin.c  -  description
                             -------------------
    begin                : Feb 2 2003
    copyright            : (C) 2003 by Rafał Bursig
    email                : Rafał Bursig <bursig@poczta.fm>
 **********************************************************************/

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

/* client */
#include "options.h"

/* gui-sdl2 */
#include "citydlg.h"
#include "colors.h"
#include "graphics.h"
#include "gui_id.h"
#include "gui_main.h"
#include "gui_tilespec.h"
#include "mapview.h"
#include "themespec.h"
#include "unistring.h"
#include "utf8string.h"
#include "widget.h"

#include "messagewin.h"


#ifdef SMALL_SCREEN
#define N_MSG_VIEW               3    /* max before scrolling happens */
#else
#define N_MSG_VIEW		 6
#endif

#define PTSIZE_LOG_FONT		adj_font(10)

static struct ADVANCED_DLG *pMsg_Dlg = NULL;

/**********************************************************************//**
  Called from default clicks on a message.
**************************************************************************/
static int msg_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    int message_index = *(int*)pWidget->data.ptr;

    pWidget->string_utf8->fgcol = *get_theme_color(COLOR_THEME_MESWIN_ACTIVE_TEXT2);
    unselect_widget_action();

    meswin_double_click(message_index);
    meswin_set_visited_state(message_index, TRUE);
  }

  return -1;
}

/**********************************************************************//**
  Called from default clicks on a messages window.
**************************************************************************/
static int move_msg_window_callback(struct widget *pWindow)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    move_window_group(pMsg_Dlg->pBeginWidgetList, pWindow);
  }

  return -1;
}

/* ======================================================================
				Public
   ====================================================================== */

/**********************************************************************//**
  Really update message window.
**************************************************************************/
void real_meswin_dialog_update(void *unused)
{
  int msg_count;
  int current_count;
  const struct message *pMsg = NULL;
  struct widget *pBuf = NULL, *pWindow = NULL;
  utf8_str *pstr = NULL;
  SDL_Rect area = {0, 0, 0, 0};
  bool create;
  int label_width;

  if (pMsg_Dlg == NULL) {
    meswin_dialog_popup(TRUE);
  }

  msg_count = meswin_get_num_messages();
  current_count = pMsg_Dlg->pScroll->count;

  if (current_count > 0) {
    undraw_group(pMsg_Dlg->pBeginActiveWidgetList, pMsg_Dlg->pEndActiveWidgetList);
    del_group_of_widgets_from_gui_list(pMsg_Dlg->pBeginActiveWidgetList,
                                       pMsg_Dlg->pEndActiveWidgetList);
    pMsg_Dlg->pBeginActiveWidgetList = NULL;
    pMsg_Dlg->pEndActiveWidgetList = NULL;
    pMsg_Dlg->pActiveWidgetList = NULL;
    /* hide scrollbar */
    hide_scrollbar(pMsg_Dlg->pScroll);
    pMsg_Dlg->pScroll->count = 0;
    current_count = 0;
  }
  create = (current_count == 0);

  pWindow = pMsg_Dlg->pEndWidgetList;

  area = pWindow->area;

  label_width = area.w - pMsg_Dlg->pScroll->pUp_Left_Button->size.w - adj_size(3);

  if (msg_count > 0) {
    for (; current_count < msg_count; current_count++) {
      pMsg = meswin_get_message(current_count);
      pstr = create_utf8_from_char(pMsg->descr, PTSIZE_LOG_FONT);

      if (convert_utf8_str_to_const_surface_width(pstr, label_width - adj_size(10))) {
        /* string must be divided to fit into the given area */
        utf8_str *pstr2;
        char **utf8_texts = create_new_line_utf8strs(pstr->text);
        int count = 0;

        while (utf8_texts[count]) {
          pstr2 = create_utf8_str(utf8_texts[count],
                                  strlen(utf8_texts[count]) + 1, PTSIZE_LOG_FONT);

          pBuf = create_iconlabel(NULL, pWindow->dst, pstr2,
                   (WF_RESTORE_BACKGROUND|WF_DRAW_TEXT_LABEL_WITH_SPACE|WF_FREE_DATA));

          /* this block is duplicated in the "else" branch */
          {
            pBuf->string_utf8->bgcol = (SDL_Color) {0, 0, 0, 0};

            pBuf->size.w = label_width;
            pBuf->data.ptr = fc_calloc(1, sizeof(int));
            *(int*)pBuf->data.ptr = current_count;
            pBuf->action = msg_callback;
            if (pMsg->tile) {
              set_wstate(pBuf, FC_WS_NORMAL);
              if (pMsg->visited) {
                pBuf->string_utf8->fgcol = *get_theme_color(COLOR_THEME_MESWIN_ACTIVE_TEXT2);
              } else {
                pBuf->string_utf8->fgcol = *get_theme_color(COLOR_THEME_MESWIN_ACTIVE_TEXT);
              }
            }

            pBuf->ID = ID_LABEL;

            widget_set_area(pBuf, area);

            /* add to widget list */
            if (create) {
              add_widget_to_vertical_scroll_widget_list(pMsg_Dlg, pBuf, pWindow, FALSE,
                                                        area.x, area.y);
              create = FALSE;
            } else {
              add_widget_to_vertical_scroll_widget_list(pMsg_Dlg, pBuf,
                                                        pMsg_Dlg->pBeginActiveWidgetList,
                                                        FALSE, area.x, area.y);
            }
          }
          count++;
        } /* while */
        FREEUTF8STR(pstr);
      } else {
        pBuf = create_iconlabel(NULL, pWindow->dst, pstr,
                  (WF_RESTORE_BACKGROUND|WF_DRAW_TEXT_LABEL_WITH_SPACE|WF_FREE_DATA));

        /* duplicated block */
        {
          pBuf->string_utf8->bgcol = (SDL_Color) {0, 0, 0, 0};

          pBuf->size.w = label_width;
          pBuf->data.ptr = fc_calloc(1, sizeof(int));
          *(int*)pBuf->data.ptr = current_count;	
          pBuf->action = msg_callback;
          if (pMsg->tile) {
            set_wstate(pBuf, FC_WS_NORMAL);
            if (pMsg->visited) {
              pBuf->string_utf8->fgcol = *get_theme_color(COLOR_THEME_MESWIN_ACTIVE_TEXT2);
            } else {
              pBuf->string_utf8->fgcol = *get_theme_color(COLOR_THEME_MESWIN_ACTIVE_TEXT);
            }
          }

          pBuf->ID = ID_LABEL;

          widget_set_area(pBuf, area);

          /* add to widget list */
          if (create) {
            add_widget_to_vertical_scroll_widget_list(pMsg_Dlg, pBuf,
                                                      pWindow, FALSE,
                                                      area.x, area.y);
            create = FALSE;
          } else {
            add_widget_to_vertical_scroll_widget_list(pMsg_Dlg, pBuf,
                                                      pMsg_Dlg->pBeginActiveWidgetList,
                                                      FALSE, area.x, area.y);
          }
        }
      } /* if */
    } /* for */
  } /* if */

  redraw_group(pMsg_Dlg->pBeginWidgetList, pWindow, 0);
  widget_flush(pWindow);
}

/**********************************************************************//**
  Popup (or raise) the message dialog; typically triggered by 'F9'.
**************************************************************************/
void meswin_dialog_popup(bool raise)
{
  utf8_str *pstr;
  struct widget *pWindow = NULL;
  SDL_Surface *pBackground;
  SDL_Rect area;
  SDL_Rect size;

  if (pMsg_Dlg) {
    return;
  }

  pMsg_Dlg = fc_calloc(1, sizeof(struct ADVANCED_DLG));

  /* create window */
  pstr = create_utf8_from_char(_("Messages"), adj_font(12));
  pstr->style = TTF_STYLE_BOLD;

  pWindow = create_window_skeleton(NULL, pstr, 0);

  pWindow->action = move_msg_window_callback;
  set_wstate(pWindow, FC_WS_NORMAL);
  add_to_gui_list(ID_CHATLINE_WINDOW, pWindow);

  pMsg_Dlg->pEndWidgetList = pWindow;
  pMsg_Dlg->pBeginWidgetList = pWindow;

  /* create scrollbar */
  create_vertical_scrollbar(pMsg_Dlg, 1, N_MSG_VIEW, TRUE, TRUE);

  pstr = create_utf8_from_char("sample text", PTSIZE_LOG_FONT);

  /* define content area */
  area.w = (adj_size(520) - (pWindow->size.w - pWindow->area.w));
  utf8_str_size(pstr, &size);
  area.h = (N_MSG_VIEW + 1) * size.h;

  FREEUTF8STR(pstr);

  /* create window background */
  pBackground = theme_get_background(theme, BACKGROUND_MESSAGEWIN);
  if (resize_window(pWindow, pBackground, NULL,
                    (pWindow->size.w - pWindow->area.w) + area.w,
                    (pWindow->size.h - pWindow->area.h) + area.h)) {
    FREESURFACE(pBackground);
  }

  area = pWindow->area;

  setup_vertical_scrollbar_area(pMsg_Dlg->pScroll,
                                area.x + area.w, area.y,
                                area.h, TRUE);

  hide_scrollbar(pMsg_Dlg->pScroll);

  widget_set_position(pWindow, (main_window_width() - pWindow->size.w)/2, adj_size(25));

  widget_redraw(pWindow);

  real_meswin_dialog_update(NULL);
}

/**********************************************************************//**
  Popdown the messages dialog; called by void popdown_all_game_dialogs(void)
**************************************************************************/
void meswin_dialog_popdown(void)
{
  if (pMsg_Dlg) {
    popdown_window_group_dialog(pMsg_Dlg->pBeginWidgetList,
                                pMsg_Dlg->pEndWidgetList);
    FC_FREE(pMsg_Dlg->pScroll);
    FC_FREE(pMsg_Dlg);
  }
}

/**********************************************************************//**
  Return whether the message dialog is open.
**************************************************************************/
bool meswin_dialog_is_open(void)
{
  return (pMsg_Dlg != NULL);
}
