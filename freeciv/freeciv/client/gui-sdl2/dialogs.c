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
                          dialogs.c  -  description
                             -------------------
    begin                : Wed Jul 24 2002
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
#include "bitvector.h"
#include "fcintl.h"
#include "log.h"
#include "rand.h"

/* common */
#include "combat.h"
#include "game.h"
#include "government.h"
#include "movement.h"
#include "unitlist.h"

/* client */
#include "client_main.h"
#include "climap.h" /* for client_tile_get_known() */
#include "goto.h"
#include "helpdata.h" /* for helptext_nation() */
#include "packhand.h"
#include "text.h"

/* gui-sdl2 */
#include "chatline.h"
#include "citydlg.h"
#include "cityrep.h"
#include "cma_fe.h"
#include "colors.h"
#include "finddlg.h"
#include "gotodlg.h"
#include "graphics.h"
#include "gui_iconv.h"
#include "gui_id.h"
#include "gui_main.h"
#include "gui_tilespec.h"
#include "helpdlg.h"
#include "inteldlg.h"
#include "mapctrl.h"
#include "mapview.h"
#include "menu.h"
#include "messagewin.h"
#include "optiondlg.h"
#include "plrdlg.h"
#include "ratesdlg.h"
#include "repodlgs.h"
#include "sprite.h"
#include "themespec.h"
#include "widget.h"
#include "wldlg.h"

#include "dialogs.h"

struct player *races_player;

extern bool is_unit_move_blocked;
extern void popdown_diplomat_dialog(void);
extern void popdown_incite_dialog(void);
extern void popdown_bribe_dialog(void);

void popdown_advanced_terrain_dialog(void);
int advanced_terrain_window_dlg_callback(struct widget *pWindow);
int exit_advanced_terrain_dlg_callback(struct widget *pWidget);

static char *pLeaderName = NULL;

static void unit_select_dialog_popdown(void);
static void popdown_terrain_info_dialog(void);
static void popdown_pillage_dialog(void);
static void popdown_connect_dialog(void);
static void popdown_revolution_dialog(void);
static void popdown_unit_upgrade_dlg(void);
static void popdown_unit_disband_dlg(void);

/**********************************************************************//**
  Place window near given tile on screen.
**************************************************************************/
void put_window_near_map_tile(struct widget *pWindow,
                              int window_width, int window_height,
                              struct tile *ptile)
{
  float canvas_x, canvas_y;
  int window_x = 0, window_y = 0;

  if (tile_to_canvas_pos(&canvas_x, &canvas_y, ptile)) {
    if (canvas_x + tileset_tile_width(tileset) + window_width >= main_window_width()) {
      if (canvas_x - window_width < 0) {
        window_x = (main_window_width() - window_width) / 2;
      } else {
        window_x = canvas_x - window_width;
      }
    } else {
      window_x = canvas_x + tileset_tile_width(tileset);
    }

    canvas_y += (tileset_tile_height(tileset) - window_height) / 2;
    if (canvas_y + window_height >= main_window_height()) {
      window_y = main_window_height() - window_height - 1;
    } else {
      if (canvas_y < 0) {
        window_y = 0;
      } else {
        window_y = canvas_y;
      }
    }
  } else {
    window_x = (main_window_width() - window_width) / 2;
    window_y = (main_window_height() - window_height) / 2;
  }

  widget_set_position(pWindow, window_x, window_y);
}

/**********************************************************************//**
  This function is called when the client disconnects or the game is
  over.  It should close all dialog windows for that game.
**************************************************************************/
void popdown_all_game_dialogs(void)
{
  unit_select_dialog_popdown();
  popdown_advanced_terrain_dialog();
  popdown_terrain_info_dialog();
  popdown_newcity_dialog();
  popdown_optiondlg(TRUE);
  undraw_order_widgets();
  popdown_diplomat_dialog();
  popdown_pillage_dialog();
  popdown_incite_dialog();
  popdown_connect_dialog();
  popdown_bribe_dialog();
  popdown_find_dialog();
  popdown_revolution_dialog();
  science_report_dialogs_popdown_all();
  meswin_dialog_popdown();
  popdown_worklist_editor();
  popdown_all_city_dialogs();
  city_report_dialog_popdown();
  economy_report_dialog_popdown();
  units_report_dialog_popdown();
  popdown_intel_dialogs();
  popdown_players_nations_dialog();
  popdown_players_dialog();
  popdown_goto_airlift_dialog();
  popdown_unit_upgrade_dlg();
  popdown_unit_disband_dlg();
  popdown_help_dialog();
  popdown_notify_goto_dialog();

  /* clear gui buffer */
  if (C_S_PREPARING == client_state()) {
    clear_surface(Main.gui->surface, NULL);
  }
}

/* ======================================================================= */

/**********************************************************************//**
  Find the my unit's (focus) chance of success at attacking/defending the
  given enemy unit.  Return FALSE if the values cannot be determined (e.g., no
  units given).
**************************************************************************/
static bool sdl_get_chance_to_win(int *att_chance, int *def_chance,
                                  struct unit *enemy_unit, struct unit *my_unit)
{  
  if (!my_unit || !enemy_unit) {
    return FALSE;
  }

  /* chance to win when active unit is attacking the selected unit */
  *att_chance = unit_win_chance(my_unit, enemy_unit) * 100;

  /* chance to win when selected unit is attacking the active unit */
  *def_chance = (1.0 - unit_win_chance(enemy_unit, my_unit)) * 100;

  return TRUE;
}

/**************************************************************************
  Notify goto dialog.
**************************************************************************/
struct notify_goto_data {
  char *headline;
  char *lines;
  struct tile *ptile;
};

#define SPECLIST_TAG notify_goto
#define SPECLIST_TYPE struct notify_goto_data
#include "speclist.h"

struct notify_goto_dialog {
  struct widget *window;
  struct widget *close_button;
  struct widget *label;
  struct notify_goto_list *datas;
};

static struct notify_goto_dialog *notify_goto_dialog = NULL;

static void notify_goto_dialog_advance(struct notify_goto_dialog *pdialog);

/**********************************************************************//**
  Create a notify goto data.
**************************************************************************/
static struct notify_goto_data *notify_goto_data_new(const char *headline,
                                                     const char *lines,
                                                     struct tile *ptile)
{
  struct notify_goto_data *pdata = fc_malloc(sizeof(*pdata));

  pdata->headline = fc_strdup(headline);
  pdata->lines = fc_strdup(lines);
  pdata->ptile = ptile;

  return pdata;
}

/**********************************************************************//**
  Destroy a notify goto data.
**************************************************************************/
static void notify_goto_data_destroy(struct notify_goto_data *pdata)
{
  free(pdata->headline);
  free(pdata->lines);
}

/**********************************************************************//**
  Move the notify dialog.
**************************************************************************/
static int notify_goto_dialog_callback(struct widget *widget)
{
  struct notify_goto_dialog *pdialog =
      (struct notify_goto_dialog *) widget->data.ptr;

  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    move_window_group(pdialog->label, pdialog->window);
  }

  return -1;
}

/**********************************************************************//**
  Close the notify dialog.
**************************************************************************/
static int notify_goto_dialog_close_callback(struct widget *widget)
{
  struct notify_goto_dialog *pdialog =
      (struct notify_goto_dialog *) widget->data.ptr;

  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    notify_goto_dialog_advance(pdialog);
  }

  return -1;
}

/**********************************************************************//**
  Goto callback.
**************************************************************************/
static int notify_goto_dialog_goto_callback(struct widget *widget)
{
  struct notify_goto_dialog *pdialog =
      (struct notify_goto_dialog *) widget->data.ptr;
  const struct notify_goto_data *pdata = notify_goto_list_get(pdialog->datas,
                                                              0);

  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (NULL != pdata->ptile) {
      center_tile_mapcanvas(pdata->ptile);
    }
  } else if (Main.event.button.button == SDL_BUTTON_RIGHT) {
     struct city *pcity;

     if (NULL != pdata->ptile && (pcity = tile_city(pdata->ptile))) {
       popup_city_dialog(pcity);
    }
  }

  return -1;
}

/**********************************************************************//**
  Create a notify dialog.
**************************************************************************/
static struct notify_goto_dialog *notify_goto_dialog_new(void)
{
  struct notify_goto_dialog *pdialog = fc_malloc(sizeof(*pdialog));
  utf8_str *str;

  /* Window. */
  str = create_utf8_from_char("", adj_font(12));
  str->style |= TTF_STYLE_BOLD;

  pdialog->window = create_window_skeleton(NULL, str, 0);
  pdialog->window->action = notify_goto_dialog_callback;
  pdialog->window->data.ptr = pdialog;
  set_wstate(pdialog->window, FC_WS_NORMAL);
  add_to_gui_list(ID_WINDOW, pdialog->window);

  /* Close button. */
  pdialog->close_button = create_themeicon(current_theme->Small_CANCEL_Icon,
                                           pdialog->window->dst,
                                           WF_WIDGET_HAS_INFO_LABEL
                                           | WF_RESTORE_BACKGROUND);
  pdialog->close_button->info_label =
      create_utf8_from_char(_("Close Dialog (Esc)"), adj_font(12));
  pdialog->close_button->action = notify_goto_dialog_close_callback;
  pdialog->close_button->data.ptr = pdialog;
  set_wstate(pdialog->close_button, FC_WS_NORMAL);
  pdialog->close_button->key = SDLK_ESCAPE;
  add_to_gui_list(ID_BUTTON, pdialog->close_button);

  pdialog->label = NULL;

  /* Data list. */
  pdialog->datas = notify_goto_list_new_full(notify_goto_data_destroy);

  return pdialog;
}

/**********************************************************************//**
  Destroy a notify dialog.
**************************************************************************/
static void notify_goto_dialog_destroy(struct notify_goto_dialog *pdialog)
{
  widget_undraw(pdialog->window);
  widget_mark_dirty(pdialog->window);
  remove_gui_layer(pdialog->window->dst);

  del_widget_pointer_from_gui_list(pdialog->window);
  del_widget_pointer_from_gui_list(pdialog->close_button);
  if (NULL != pdialog->label) {
    del_widget_pointer_from_gui_list(pdialog->label);
  }

  notify_goto_list_destroy(pdialog->datas);
  free(pdialog);
}

/**********************************************************************//**
  Update a notify dialog.
**************************************************************************/
static void notify_goto_dialog_update(struct notify_goto_dialog *pdialog)
{
  const struct notify_goto_data *pdata = notify_goto_list_get(pdialog->datas,
                                                              0);

  if (NULL == pdata) {
    return;
  }

  widget_undraw(pdialog->window);
  widget_mark_dirty(pdialog->window);

  copy_chars_to_utf8_str(pdialog->window->string_utf8, pdata->headline);
  if (NULL != pdialog->label) {
    del_widget_pointer_from_gui_list(pdialog->label);
  }
  pdialog->label = create_iconlabel_from_chars(NULL, pdialog->window->dst,
                                               pdata->lines, adj_font(12),
                                               WF_RESTORE_BACKGROUND);
  pdialog->label->action = notify_goto_dialog_goto_callback;
  pdialog->label->data.ptr = pdialog;
  set_wstate(pdialog->label, FC_WS_NORMAL);
  add_to_gui_list(ID_LABEL, pdialog->label);

  resize_window(pdialog->window, NULL, NULL,
                adj_size(pdialog->label->size.w + 40),
                adj_size(pdialog->label->size.h + 60));
  widget_set_position(pdialog->window,
                      (main_window_width() - pdialog->window->size.w) / 2,
                      (main_window_height() - pdialog->window->size.h) / 2);
  widget_set_position(pdialog->close_button, pdialog->window->size.w
                      - pdialog->close_button->size.w - 1,
                      pdialog->window->size.y + adj_size(2));
  widget_set_position(pdialog->label, adj_size(20), adj_size(40));

  widget_redraw(pdialog->window);
  widget_redraw(pdialog->close_button);
  widget_redraw(pdialog->label);
  widget_mark_dirty(pdialog->window);
  flush_all();
}

/**********************************************************************//**
  Update a notify dialog.
**************************************************************************/
static void notify_goto_dialog_advance(struct notify_goto_dialog *pdialog)
{
  if (1 < notify_goto_list_size(pdialog->datas)) {
    notify_goto_list_remove(pdialog->datas,
                            notify_goto_list_get(pdialog->datas, 0));
    notify_goto_dialog_update(pdialog);
  } else {
    notify_goto_dialog_destroy(pdialog);
    if (pdialog == notify_goto_dialog) {
      notify_goto_dialog = NULL;
    }
  }
}

/**********************************************************************//**
  Popup a dialog to display information about an event that has a
  specific location.  The user should be given the option to goto that
  location.
**************************************************************************/
void popup_notify_goto_dialog(const char *headline, const char *lines,
                              const struct text_tag_list *tags,
                              struct tile *ptile)
{
  if (NULL == notify_goto_dialog) {
    notify_goto_dialog = notify_goto_dialog_new();
  }

  fc_assert(NULL != notify_goto_dialog);

  notify_goto_list_prepend(notify_goto_dialog->datas,
                           notify_goto_data_new(headline, lines, ptile));
  notify_goto_dialog_update(notify_goto_dialog);
}

/**********************************************************************//**
  Popdown the notify goto dialog.
**************************************************************************/
void popdown_notify_goto_dialog(void)
{
  if (NULL != notify_goto_dialog) {
    notify_goto_dialog_destroy(notify_goto_dialog);
    notify_goto_dialog = NULL;
  }
}

/**********************************************************************//**
  Popup a dialog to display connection message from server.
**************************************************************************/
void popup_connect_msg(const char *headline, const char *message)
{
  log_error("popup_connect_msg() PORT ME");
}

/* ----------------------------------------------------------------------- */
struct ADVANCED_DLG *pNotifyDlg = NULL;

/**********************************************************************//**
  User interacted with generic notify dialog.
**************************************************************************/
static int notify_dialog_window_callback(struct widget *pWindow)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    move_window_group(pNotifyDlg->pBeginWidgetList, pWindow);
  }

  return -1;
}

/**********************************************************************//**
  User interacted with notify dialog close button.
**************************************************************************/
static int exit_notify_dialog_callback(struct widget *pWidget)
{
  if (Main.event.type == SDL_KEYDOWN
      || (Main.event.type == SDL_MOUSEBUTTONDOWN
          && Main.event.button.button == SDL_BUTTON_LEFT)) {
    if (pNotifyDlg) {
      popdown_window_group_dialog(pNotifyDlg->pBeginWidgetList,
                                  pNotifyDlg->pEndWidgetList);
      FC_FREE(pNotifyDlg->pScroll);
      FC_FREE(pNotifyDlg);
      flush_dirty();
    }
  }
  return -1;
}

/**********************************************************************//**
  Popup a generic dialog to display some generic information.
**************************************************************************/
void popup_notify_dialog(const char *caption, const char *headline,
                         const char *lines)
{
  struct widget *pBuf, *pWindow;
  utf8_str *pstr;
  SDL_Surface *pHeadline, *pLines;
  SDL_Rect dst;
  SDL_Rect area;

  if (pNotifyDlg) {
    return;
  }

  pNotifyDlg = fc_calloc(1, sizeof(struct ADVANCED_DLG));

  pstr = create_utf8_from_char(caption, adj_font(12));
  pstr->style |= TTF_STYLE_BOLD;

  pWindow = create_window_skeleton(NULL, pstr, 0);

  pWindow->action = notify_dialog_window_callback;
  set_wstate(pWindow, FC_WS_NORMAL);

  add_to_gui_list(ID_WINDOW, pWindow);
  pNotifyDlg->pEndWidgetList = pWindow;

  area = pWindow->area;

  /* ---------- */
  /* create exit button */
  pBuf = create_themeicon(current_theme->Small_CANCEL_Icon, pWindow->dst,
                          WF_WIDGET_HAS_INFO_LABEL | WF_RESTORE_BACKGROUND);
  pBuf->info_label = create_utf8_from_char(_("Close Dialog (Esc)"),
                                           adj_font(12));
  pBuf->action = exit_notify_dialog_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->key = SDLK_ESCAPE;
  area.w += (pBuf->size.w + adj_size(10));

  add_to_gui_list(ID_BUTTON, pBuf);
  pNotifyDlg->pBeginWidgetList = pBuf;

  pstr = create_utf8_from_char(headline, adj_font(16));
  pstr->style |= TTF_STYLE_BOLD;

  pHeadline = create_text_surf_from_utf8(pstr);

  if (lines && *lines != '\0') {
    change_ptsize_utf8(pstr, adj_font(12));
    pstr->style &= ~TTF_STYLE_BOLD;
    copy_chars_to_utf8_str(pstr, lines);
    pLines = create_text_surf_from_utf8(pstr);
  } else {
    pLines = NULL;
  }

  FREEUTF8STR(pstr);

  area.w = MAX(area.w, pHeadline->w);
  if (pLines) {
    area.w = MAX(area.w, pLines->w);
  }
  area.w += adj_size(60);
  area.h = MAX(area.h, adj_size(10) + pHeadline->h + adj_size(10));
  if (pLines) {
    area.h += pLines->h + adj_size(10);
  }

  resize_window(pWindow, NULL, get_theme_color(COLOR_THEME_BACKGROUND),
                (pWindow->size.w - pWindow->area.w) + area.w,
                (pWindow->size.h - pWindow->area.h) + area.h);

  area = pWindow->area;

  widget_set_position(pWindow,
                      (main_window_width() - pWindow->size.w) / 2,
                      (main_window_height() - pWindow->size.h) / 2);

  dst.x = area.x + (area.w - pHeadline->w) / 2;
  dst.y = area.y + adj_size(10);

  alphablit(pHeadline, NULL, pWindow->theme, &dst, 255);
  if (pLines) {
    dst.y += pHeadline->h + adj_size(10);
    if (pHeadline->w < pLines->w) {
      dst.x = area.x + (area.w - pLines->w) / 2;
    }

    alphablit(pLines, NULL, pWindow->theme, &dst, 255);
  }

  FREESURFACE(pHeadline);
  FREESURFACE(pLines);

  /* exit button */
  pBuf = pWindow->prev;
  pBuf->size.x = area.x + area.w - pBuf->size.w - 1;
  pBuf->size.y = pWindow->size.y + adj_size(2);

  /* redraw */
  redraw_group(pNotifyDlg->pBeginWidgetList, pWindow, 0);
  widget_flush(pWindow);
}

/* =======================================================================*/
/* ========================= UNIT UPGRADE DIALOG =========================*/
/* =======================================================================*/
static struct SMALL_DLG *pUnit_Upgrade_Dlg = NULL;

/**********************************************************************//**
  User interacted with upgrade unit widget.
**************************************************************************/
static int upgrade_unit_window_callback(struct widget *pWindow)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    move_window_group(pUnit_Upgrade_Dlg->pBeginWidgetList, pWindow);
  }
  return -1;
}

/**********************************************************************//**
  User interacted with upgrade unit dialog cancel -button 
**************************************************************************/
static int cancel_upgrade_unit_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    popdown_unit_upgrade_dlg();
    /* enable city dlg */
    enable_city_dlg_widgets();
    flush_dirty();
  }
  return -1;
}

/**********************************************************************//**
  User interacted with unit upgrade dialog "Upgrade" -button.
**************************************************************************/
static int ok_upgrade_unit_window_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct unit *pUnit = pWidget->data.unit;

    popdown_unit_upgrade_dlg();
    /* enable city dlg */
    enable_city_dlg_widgets();
    free_city_units_lists();
    request_unit_upgrade(pUnit);
    flush_dirty();
  }
  return -1;
}

/***********************************************************************//**
  Popup dialog for upgrade units
***************************************************************************/
void popup_upgrade_dialog(struct unit_list *punits)
{
  /* PORTME: is support for more than one unit in punits required? */

  /* Assume only one unit for now. */
  fc_assert_msg(unit_list_size(punits) <= 1,
                "SDL2 popup_upgrade_dialog() handles max 1 unit.");
  popup_unit_upgrade_dlg(unit_list_get(punits, 0), FALSE);
}

/**********************************************************************//**
  Open unit upgrade dialog.
**************************************************************************/
void popup_unit_upgrade_dlg(struct unit *pUnit, bool city)
{
  char cBuf[128];
  struct widget *pBuf = NULL, *pWindow;
  utf8_str *pstr;
  SDL_Surface *pText;
  SDL_Rect dst;
  int window_x = 0, window_y = 0;
  enum unit_upgrade_result unit_upgrade_result;
  SDL_Rect area;

  if (pUnit_Upgrade_Dlg) {
    /* just in case */
    flush_dirty();
    return;
  }

  pUnit_Upgrade_Dlg = fc_calloc(1, sizeof(struct SMALL_DLG));

  unit_upgrade_result = unit_upgrade_info(pUnit, cBuf, sizeof(cBuf));

  pstr = create_utf8_from_char(_("Upgrade Obsolete Units"), adj_font(12));
  pstr->style |= TTF_STYLE_BOLD;

  pWindow = create_window_skeleton(NULL, pstr, 0);

  pWindow->action = upgrade_unit_window_callback;
  set_wstate(pWindow, FC_WS_NORMAL);

  pUnit_Upgrade_Dlg->pEndWidgetList = pWindow;

  add_to_gui_list(ID_WINDOW, pWindow);

  area = pWindow->area;

  /* ============================================================= */

  /* create text label */
  pstr = create_utf8_from_char(cBuf, adj_font(10));
  pstr->style |= (TTF_STYLE_BOLD|SF_CENTER);
  pstr->fgcol = *get_theme_color(COLOR_THEME_UNITUPGRADE_TEXT);

  pText = create_text_surf_from_utf8(pstr);
  FREEUTF8STR(pstr);

  area.w = MAX(area.w, pText->w + adj_size(20));
  area.h += (pText->h + adj_size(10));

  /* cancel button */
  pBuf = create_themeicon_button_from_chars(current_theme->CANCEL_Icon,
                                            pWindow->dst, _("Cancel"),
                                            adj_font(12), 0);

  pBuf->action = cancel_upgrade_unit_callback;
  set_wstate(pBuf, FC_WS_NORMAL);

  area.h += (pBuf->size.h + adj_size(20));

  add_to_gui_list(ID_BUTTON, pBuf);

  if (UU_OK == unit_upgrade_result) {
    pBuf = create_themeicon_button_from_chars(current_theme->OK_Icon, pWindow->dst,
                                              _("Upgrade"), adj_font(12), 0);

    pBuf->action = ok_upgrade_unit_window_callback;
    set_wstate(pBuf, FC_WS_NORMAL);
    pBuf->data.unit = pUnit;
    add_to_gui_list(ID_BUTTON, pBuf);
    pBuf->size.w = MAX(pBuf->size.w, pBuf->next->size.w);
    pBuf->next->size.w = pBuf->size.w;
    area.w = MAX(area.w, adj_size(30) + pBuf->size.w * 2);
  } else {
    area.w = MAX(area.w, pBuf->size.w + adj_size(20));
  }
  /* ============================================ */

  pUnit_Upgrade_Dlg->pBeginWidgetList = pBuf;

  resize_window(pWindow, NULL, get_theme_color(COLOR_THEME_BACKGROUND),
                (pWindow->size.w - pWindow->area.w) + area.w,
                (pWindow->size.h - pWindow->area.h) + area.h);

  area = pWindow->area;

  if (city) {
    window_x = Main.event.motion.x;
    window_y = Main.event.motion.y;
  } else {
    put_window_near_map_tile(pWindow, pWindow->size.w, pWindow->size.h,
                             unit_tile(pUnit));
  }

  widget_set_position(pWindow, window_x, window_y);

  /* setup rest of widgets */
  /* label */
  dst.x = area.x + (area.w - pText->w) / 2;
  dst.y = area.y + adj_size(10);
  alphablit(pText, NULL, pWindow->theme, &dst, 255);
  FREESURFACE(pText);

  /* cancel button */
  pBuf = pWindow->prev;
  pBuf->size.y = area.y + area.h - pBuf->size.h - adj_size(7);

  if (UU_OK == unit_upgrade_result) {
    /* upgrade button */
    pBuf = pBuf->prev;
    pBuf->size.x = area.x + (area.w - (2 * pBuf->size.w + adj_size(10))) / 2;
    pBuf->size.y = pBuf->next->size.y;

    /* cancel button */
    pBuf->next->size.x = pBuf->size.x + pBuf->size.w + adj_size(10);
  } else {
    /* x position of cancel button */
    pBuf->size.x = area.x + area.w - pBuf->size.w - adj_size(10);
  }

  /* ================================================== */
  /* redraw */
  redraw_group(pUnit_Upgrade_Dlg->pBeginWidgetList, pWindow, 0);

  widget_mark_dirty(pWindow);
  flush_dirty();
}

/**********************************************************************//**
  Close unit upgrade dialog.
**************************************************************************/
static void popdown_unit_upgrade_dlg(void)
{
  if (pUnit_Upgrade_Dlg) {
    popdown_window_group_dialog(pUnit_Upgrade_Dlg->pBeginWidgetList,
                                pUnit_Upgrade_Dlg->pEndWidgetList);
    FC_FREE(pUnit_Upgrade_Dlg);
  }
}

/* =======================================================================*/
/* ========================= UNIT DISBAND DIALOG =========================*/
/* =======================================================================*/
static struct SMALL_DLG *pUnit_Disband_Dlg = NULL;

/**********************************************************************//**
  User interacted with disband unit widget.
**************************************************************************/
static int disband_unit_window_callback(struct widget *pWindow)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    move_window_group(pUnit_Disband_Dlg->pBeginWidgetList, pWindow);
  }

  return -1;
}

/**********************************************************************//**
  User interacted with disband unit dialog cancel -button
**************************************************************************/
static int cancel_disband_unit_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    popdown_unit_disband_dlg();
    /* enable city dlg */
    enable_city_dlg_widgets();
    flush_dirty();
  }
  return -1;
}

/**********************************************************************//**
  User interacted with unit disband dialog "Disband" -button.
**************************************************************************/
static int ok_disband_unit_window_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct unit *pUnit = pWidget->data.unit;

    popdown_unit_disband_dlg();
    /* enable city dlg */
    enable_city_dlg_widgets();
    free_city_units_lists();
    request_unit_disband(pUnit);
    flush_dirty();
  }
  return -1;
}

/**********************************************************************//**
  Open unit disband dialog.
**************************************************************************/
void popup_unit_disband_dlg(struct unit *pUnit, bool city)
{
  char cBuf[128];
  struct widget *pBuf = NULL, *pWindow;
  utf8_str *pstr;
  SDL_Surface *pText;
  SDL_Rect dst;
  int window_x = 0, window_y = 0;
  bool unit_disband_result;
  SDL_Rect area;

  if (pUnit_Disband_Dlg) {
    /* just in case */
    flush_dirty();
    return;
  }

  pUnit_Disband_Dlg = fc_calloc(1, sizeof(struct SMALL_DLG));

  {
    struct unit_list *pUnits = unit_list_new();

    unit_list_append(pUnits, pUnit);
    unit_disband_result = get_units_disband_info(cBuf, sizeof(cBuf), pUnits);
    unit_list_destroy(pUnits);
  }

  pstr = create_utf8_from_char(_("Disband Units"), adj_font(12));
  pstr->style |= TTF_STYLE_BOLD;

  pWindow = create_window_skeleton(NULL, pstr, 0);

  pWindow->action = disband_unit_window_callback;
  set_wstate(pWindow, FC_WS_NORMAL);

  pUnit_Disband_Dlg->pEndWidgetList = pWindow;

  add_to_gui_list(ID_WINDOW, pWindow);

  area = pWindow->area;

  /* ============================================================= */

  /* create text label */
  pstr = create_utf8_from_char(cBuf, adj_font(10));
  pstr->style |= (TTF_STYLE_BOLD|SF_CENTER);
  pstr->fgcol = *get_theme_color(COLOR_THEME_UNITDISBAND_TEXT);

  pText = create_text_surf_from_utf8(pstr);
  FREEUTF8STR(pstr);

  area.w = MAX(area.w, pText->w + adj_size(20));
  area.h += (pText->h + adj_size(10));

  /* cancel button */
  pBuf = create_themeicon_button_from_chars(current_theme->CANCEL_Icon,
                                            pWindow->dst, _("Cancel"),
                                            adj_font(12), 0);

  pBuf->action = cancel_disband_unit_callback;
  set_wstate(pBuf, FC_WS_NORMAL);

  area.h += (pBuf->size.h + adj_size(20));

  add_to_gui_list(ID_BUTTON, pBuf);

  if (unit_disband_result) {
    pBuf = create_themeicon_button_from_chars(current_theme->OK_Icon, pWindow->dst,
                                              _("Disband"), adj_font(12), 0);

    pBuf->action = ok_disband_unit_window_callback;
    set_wstate(pBuf, FC_WS_NORMAL);
    pBuf->data.unit = pUnit;
    add_to_gui_list(ID_BUTTON, pBuf);
    pBuf->size.w = MAX(pBuf->size.w, pBuf->next->size.w);
    pBuf->next->size.w = pBuf->size.w;
    area.w = MAX(area.w, adj_size(30) + pBuf->size.w * 2);
  } else {
    area.w = MAX(area.w, pBuf->size.w + adj_size(20));
  }
  /* ============================================ */

  pUnit_Disband_Dlg->pBeginWidgetList = pBuf;

  resize_window(pWindow, NULL, get_theme_color(COLOR_THEME_BACKGROUND),
                (pWindow->size.w - pWindow->area.w) + area.w,
                (pWindow->size.h - pWindow->area.h) + area.h);

  area = pWindow->area;

  if (city) {
    window_x = Main.event.motion.x;
    window_y = Main.event.motion.y;
  } else {
    put_window_near_map_tile(pWindow, pWindow->size.w, pWindow->size.h,
                             unit_tile(pUnit));
  }

  widget_set_position(pWindow, window_x, window_y);

  /* setup rest of widgets */
  /* label */
  dst.x = area.x + (area.w - pText->w) / 2;
  dst.y = area.y + adj_size(10);
  alphablit(pText, NULL, pWindow->theme, &dst, 255);
  FREESURFACE(pText);

  /* cancel button */
  pBuf = pWindow->prev;
  pBuf->size.y = area.y + area.h - pBuf->size.h - adj_size(7);

  if (unit_disband_result) {
    /* disband button */
    pBuf = pBuf->prev;
    pBuf->size.x = area.x + (area.w - (2 * pBuf->size.w + adj_size(10))) / 2;
    pBuf->size.y = pBuf->next->size.y;

    /* cancel button */
    pBuf->next->size.x = pBuf->size.x + pBuf->size.w + adj_size(10);
  } else {
    /* x position of cancel button */
    pBuf->size.x = area.x + area.w - pBuf->size.w - adj_size(10);
  }

  /* ================================================== */
  /* redraw */
  redraw_group(pUnit_Disband_Dlg->pBeginWidgetList, pWindow, 0);

  widget_mark_dirty(pWindow);
  flush_dirty();
}

/**********************************************************************//**
  Close unit disband dialog.
**************************************************************************/
static void popdown_unit_disband_dlg(void)
{
  if (pUnit_Disband_Dlg) {
    popdown_window_group_dialog(pUnit_Disband_Dlg->pBeginWidgetList,
                                pUnit_Disband_Dlg->pEndWidgetList);
    FC_FREE(pUnit_Disband_Dlg);
  }
}

/* =======================================================================*/
/* ======================== UNIT SELECTION DIALOG ========================*/
/* =======================================================================*/
static struct ADVANCED_DLG *pUnit_Select_Dlg = NULL;

/**********************************************************************//**
  User interacted with unit selection window.
**************************************************************************/
static int unit_select_window_callback(struct widget *pWindow)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    move_window_group(pUnit_Select_Dlg->pBeginWidgetList, pWindow);
  }

  return -1;
}

/**********************************************************************//**
  User requested unit select window to be closed.
**************************************************************************/
static int exit_unit_select_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    unit_select_dialog_popdown();
    is_unit_move_blocked = FALSE;
  }

  return -1;
}

/**********************************************************************//**
  User selected unit from unit select window.
**************************************************************************/
static int unit_select_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct unit *pUnit =
      player_unit_by_number(client_player(), MAX_ID - pWidget->ID);

    unit_select_dialog_popdown();
    if (pUnit) {
      request_new_unit_activity(pUnit, ACTIVITY_IDLE);
      unit_focus_set(pUnit);
    }
  }
  return -1;
}

/**********************************************************************//**
  Popdown a dialog window to select units on a particular tile.
**************************************************************************/
static void unit_select_dialog_popdown(void)
{
  if (pUnit_Select_Dlg) {
    is_unit_move_blocked = FALSE;
    popdown_window_group_dialog(pUnit_Select_Dlg->pBeginWidgetList,
                                pUnit_Select_Dlg->pEndWidgetList);

    FC_FREE(pUnit_Select_Dlg->pScroll);
    FC_FREE(pUnit_Select_Dlg);
    flush_dirty();
  }
}

/**********************************************************************//**
  Popup a dialog window to select units on a particular tile.
**************************************************************************/
void unit_select_dialog_popup(struct tile *ptile)
{
  struct widget *pBuf = NULL, *pWindow;
  utf8_str *pstr;
  struct unit *pUnit = NULL, *pFocus = head_of_units_in_focus();
  struct unit_type *pUnitType;
  char cBuf[255];
  int i, w = 0, n;
  SDL_Rect area;

#define NUM_SEEN	20

  n = unit_list_size(ptile->units);

  if (!n || pUnit_Select_Dlg) {
    return;
  }

  is_unit_move_blocked = TRUE;
  pUnit_Select_Dlg = fc_calloc(1, sizeof(struct ADVANCED_DLG));

  fc_snprintf(cBuf , sizeof(cBuf),"%s (%d)", _("Unit selection") , n);
  pstr = create_utf8_from_char(cBuf , adj_font(12));
  pstr->style |= TTF_STYLE_BOLD;

  pWindow = create_window_skeleton(NULL, pstr, 0);

  pWindow->action = unit_select_window_callback;
  set_wstate(pWindow, FC_WS_NORMAL);

  add_to_gui_list(ID_UNIT_SELECT_DLG_WINDOW, pWindow);
  pUnit_Select_Dlg->pEndWidgetList = pWindow;

  area = pWindow->area;

  /* ---------- */
  /* create exit button */
  pBuf = create_themeicon(current_theme->Small_CANCEL_Icon, pWindow->dst,
                          WF_WIDGET_HAS_INFO_LABEL | WF_RESTORE_BACKGROUND);
  pBuf->info_label = create_utf8_from_char(_("Close Dialog (Esc)"),
                                           adj_font(12));
  pBuf->action = exit_unit_select_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->key = SDLK_ESCAPE;
  area.w += (pBuf->size.w + adj_size(10));

  add_to_gui_list(ID_UNIT_SELECT_DLG_EXIT_BUTTON, pBuf);

  /* ---------- */

  for (i = 0; i < n; i++) {
    const char *vetname;

    pUnit = unit_list_get(ptile->units, i);
    pUnitType = unit_type_get(pUnit);
    vetname = utype_veteran_name_translation(pUnitType, pUnit->veteran);

    if (unit_owner(pUnit) == client.conn.playing) {
      fc_snprintf(cBuf , sizeof(cBuf), _("Contact %s (%d / %d) %s(%d,%d,%s) %s"),
                  (vetname != NULL ? vetname : ""),
                  pUnit->hp, pUnitType->hp,
                  utype_name_translation(pUnitType),
                  pUnitType->attack_strength,
                  pUnitType->defense_strength,
                  move_points_text(pUnitType->move_rate, FALSE),
                  unit_activity_text(pUnit));
    } else {
      int att_chance, def_chance;

      fc_snprintf(cBuf , sizeof(cBuf), _("%s %s %s(A:%d D:%d M:%s FP:%d) HP:%d%%"),
                  nation_adjective_for_player(unit_owner(pUnit)),
                  (vetname != NULL ? vetname : ""),
                  utype_name_translation(pUnitType),
                  pUnitType->attack_strength,
                  pUnitType->defense_strength,
                  move_points_text(pUnitType->move_rate, FALSE),
                  pUnitType->firepower,
                  (pUnit->hp * 100 / pUnitType->hp + 9) / 10);

      /* calculate chance to win */
      if (sdl_get_chance_to_win(&att_chance, &def_chance, pUnit, pFocus)) {
        /* TRANS: "CtW" = "Chance to Win" */
        cat_snprintf(cBuf, sizeof(cBuf), _(" CtW: Att:%d%% Def:%d%%"),
                     att_chance, def_chance);
      }
    }

    create_active_iconlabel(pBuf, pWindow->dst, pstr, cBuf,
                            unit_select_callback);

    add_to_gui_list(MAX_ID - pUnit->id , pBuf);

    area.w = MAX(area.w, pBuf->size.w);
    area.h += pBuf->size.h;
    if (unit_owner(pUnit) == client.conn.playing) {
      set_wstate(pBuf, FC_WS_NORMAL);
    }

    if (i > NUM_SEEN - 1) {
      set_wflag(pBuf , WF_HIDDEN);
    }
  }

  pUnit_Select_Dlg->pBeginWidgetList = pBuf;
  pUnit_Select_Dlg->pBeginActiveWidgetList = pUnit_Select_Dlg->pBeginWidgetList;
  pUnit_Select_Dlg->pEndActiveWidgetList = pWindow->prev->prev;
  pUnit_Select_Dlg->pActiveWidgetList = pUnit_Select_Dlg->pEndActiveWidgetList;

  area.w += adj_size(2);
  if (n > NUM_SEEN) {
    n = create_vertical_scrollbar(pUnit_Select_Dlg, 1, NUM_SEEN, TRUE, TRUE);
    area.w += n;

    /* ------- window ------- */
    area.h = NUM_SEEN * pWindow->prev->prev->size.h;
  }

  resize_window(pWindow, NULL, NULL,
                (pWindow->size.w - pWindow->area.w) + area.w,
                (pWindow->size.h - pWindow->area.h) + area.h);

  area = pWindow->area;

  put_window_near_map_tile(pWindow, pWindow->size.w, pWindow->size.h,
                           unit_tile(pUnit));

  w = area.w;

  if (pUnit_Select_Dlg->pScroll) {
    w -= n;
  }

  /* exit button */
  pBuf = pWindow->prev;
  pBuf->size.x = area.x + area.w - pBuf->size.w - 1;
  pBuf->size.y = pWindow->size.y + adj_size(2);
  pBuf = pBuf->prev;

  setup_vertical_widgets_position(1, area.x + 1, area.y, w, 0,
                                  pUnit_Select_Dlg->pBeginActiveWidgetList,
                                  pBuf);

  if (pUnit_Select_Dlg->pScroll) {
    setup_vertical_scrollbar_area(pUnit_Select_Dlg->pScroll,
                                  area.x + area.w, area.y,
                                  area.h, TRUE);
  }

  /* ==================================================== */
  /* redraw */
  redraw_group(pUnit_Select_Dlg->pBeginWidgetList, pWindow, 0);

  widget_flush(pWindow);
}

/**********************************************************************//**
  Update the dialog window to select units on a particular tile.
**************************************************************************/
void unit_select_dialog_update_real(void *unused)
{
  /* PORTME */
}

/* ====================================================================== */
/* ============================ TERRAIN INFO ============================ */
/* ====================================================================== */
static struct SMALL_DLG *pTerrain_Info_Dlg = NULL;


/**********************************************************************//**
  Popdown terrain information dialog.
**************************************************************************/
static int terrain_info_window_dlg_callback(struct widget *pWindow)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    move_window_group(pTerrain_Info_Dlg->pBeginWidgetList, pWindow);
  }
  return -1;
}

/**********************************************************************//**
  Popdown terrain information dialog.
**************************************************************************/
static void popdown_terrain_info_dialog(void)
{
  if (pTerrain_Info_Dlg) {
    popdown_window_group_dialog(pTerrain_Info_Dlg->pBeginWidgetList,
				pTerrain_Info_Dlg->pEndWidgetList);
    FC_FREE(pTerrain_Info_Dlg);
    flush_dirty();
  }
}

/**********************************************************************//**
  Popdown terrain information dialog.
**************************************************************************/
static int exit_terrain_info_dialog_callback(struct widget *pButton)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    popdown_terrain_info_dialog();
  }
  return -1;
}

/**********************************************************************//**
  Return a (static) string with terrain defense bonus.
  This does not include bonuses some units may get out of bases.
**************************************************************************/
const char *sdl_get_tile_defense_info_text(struct tile *ptile)
{
  static char buffer[64];
  int bonus = (tile_terrain(ptile)->defense_bonus - 10) * 10;

  extra_type_iterate(pextra) {
    if (tile_has_extra(ptile, pextra)
        && pextra->category == ECAT_NATURAL) {
      bonus += pextra->defense_bonus;
    }
  } extra_type_iterate_end;

  fc_snprintf(buffer, sizeof(buffer), _("Terrain Defense Bonus: +%d%% "), bonus);

  return buffer;
}

/**********************************************************************//**
  Popup terrain information dialog.
**************************************************************************/
static void popup_terrain_info_dialog(SDL_Surface *pDest, struct tile *ptile)
{
  SDL_Surface *pSurf;
  struct widget *pBuf, *pWindow;
  utf8_str *pstr;  
  char cBuf[256];  
  SDL_Rect area;

  if (pTerrain_Info_Dlg) {
    flush_dirty();
    return;
  }

  pSurf = get_terrain_surface(ptile);
  pTerrain_Info_Dlg = fc_calloc(1, sizeof(struct SMALL_DLG));

  /* ----------- */
  fc_snprintf(cBuf, sizeof(cBuf), "%s [%d,%d]", _("Terrain Info"),
              TILE_XY(ptile));

  pWindow = create_window_skeleton(NULL, create_utf8_from_char(cBuf , adj_font(12)), 0);
  pWindow->string_utf8->style |= TTF_STYLE_BOLD;

  pWindow->action = terrain_info_window_dlg_callback;
  set_wstate(pWindow, FC_WS_NORMAL);

  add_to_gui_list(ID_TERRAIN_INFO_DLG_WINDOW, pWindow);
  pTerrain_Info_Dlg->pEndWidgetList = pWindow;

  area = pWindow->area;

  /* ---------- */
  pstr = create_utf8_from_char(popup_info_text(ptile), adj_font(12));
  pstr->style |= SF_CENTER;
  pBuf = create_iconlabel(pSurf, pWindow->dst, pstr, 0);

  pBuf->size.h += tileset_tile_height(tileset) / 2;

  add_to_gui_list(ID_LABEL, pBuf);

  /* ------ window ---------- */
  area.w = MAX(area.w, pBuf->size.w + adj_size(20));
  area.h = MAX(area.h, pBuf->size.h);

  resize_window(pWindow, NULL, get_theme_color(COLOR_THEME_BACKGROUND),
                (pWindow->size.w - pWindow->area.w) + area.w,
                (pWindow->size.h - pWindow->area.h) + area.h);

  area = pWindow->area;

  put_window_near_map_tile(pWindow, pWindow->size.w, pWindow->size.h, ptile);

  /* ------------------------ */

  pBuf->size.x = area.x + adj_size(10);
  pBuf->size.y = area.y;

  /* exit icon */
  pBuf = create_themeicon(current_theme->Small_CANCEL_Icon, pWindow->dst,
                          WF_WIDGET_HAS_INFO_LABEL | WF_RESTORE_BACKGROUND);
  pBuf->info_label = create_utf8_from_char(_("Close Dialog (Esc)"),
                                           adj_font(12));
  pBuf->size.x = area.x + area.w - pBuf->size.w - 1;
  pBuf->size.y = pWindow->size.y + adj_size(2);
  pBuf->action = exit_terrain_info_dialog_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->key = SDLK_ESCAPE;

  add_to_gui_list(ID_TERRAIN_INFO_DLG_EXIT_BUTTON, pBuf);

  pTerrain_Info_Dlg->pBeginWidgetList = pBuf;
  /* --------------------------------- */
  /* redraw */
  redraw_group(pTerrain_Info_Dlg->pBeginWidgetList, pWindow, 0);
  widget_mark_dirty(pWindow);
  flush_dirty();
}

/* ====================================================================== */
/* ========================= ADVANCED_TERRAIN_MENU ====================== */
/* ====================================================================== */
struct ADVANCED_DLG  *pAdvanced_Terrain_Dlg = NULL;

/**********************************************************************//**
  Popdown a generic dialog to display some generic information about
  terrain : tile, units , cities, etc.
**************************************************************************/
void popdown_advanced_terrain_dialog(void)
{
  if (pAdvanced_Terrain_Dlg) {
    is_unit_move_blocked = FALSE;
    popdown_window_group_dialog(pAdvanced_Terrain_Dlg->pBeginWidgetList,
                                pAdvanced_Terrain_Dlg->pEndWidgetList);

    FC_FREE(pAdvanced_Terrain_Dlg->pScroll);
    FC_FREE(pAdvanced_Terrain_Dlg);
  }
}

/**********************************************************************//**
  User selected "Advanced Menu"
**************************************************************************/
int advanced_terrain_window_dlg_callback(struct widget *pWindow)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    move_window_group(pAdvanced_Terrain_Dlg->pBeginWidgetList, pWindow);
  }
  return -1;
}

/**********************************************************************//**
  User requested closing of advanced terrain dialog.
**************************************************************************/
int exit_advanced_terrain_dlg_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    popdown_advanced_terrain_dialog();
    flush_dirty();
  }
  return -1;
}

/**********************************************************************//**
  User requested terrain info.
**************************************************************************/
static int terrain_info_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    int x = pWidget->data.cont->id0;
    int y = pWidget->data.cont->id1;

    popdown_advanced_terrain_dialog();

    popup_terrain_info_dialog(NULL, map_pos_to_tile(&(wld.map), x , y));
  }

  return -1;
}

/**********************************************************************//**
  User requested zoom to city.
**************************************************************************/
static int zoom_to_city_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct city *pCity = pWidget->data.city;

    popdown_advanced_terrain_dialog();

    popup_city_dialog(pCity);
  }
  return -1;
}

/**********************************************************************//**
  User requested production change.
**************************************************************************/
static int change_production_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct city *pCity = pWidget->data.city;

    popdown_advanced_terrain_dialog();
    popup_worklist_editor(pCity, NULL);
  }
  return -1;
}

/**********************************************************************//**
  User requested hurry production.
**************************************************************************/
static int hurry_production_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct city *pCity = pWidget->data.city;

    popdown_advanced_terrain_dialog();

    popup_hurry_production_dialog(pCity, NULL);
  }
  return -1;
}

/**********************************************************************//**
  User requested opening of cma settings.
**************************************************************************/
static int cma_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct city *pCity = pWidget->data.city;

    popdown_advanced_terrain_dialog();
    popup_city_cma_dialog(pCity);
  }
  return -1;
}

/**********************************************************************//**
  User selected unit.
**************************************************************************/
static int adv_unit_select_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct unit *pUnit = pWidget->data.unit;

    popdown_advanced_terrain_dialog();

    if (pUnit) {
      request_new_unit_activity(pUnit, ACTIVITY_IDLE);
      unit_focus_set(pUnit);
    }
  }
  return -1;
}

/**********************************************************************//**
  User selected all units from tile.
**************************************************************************/
static int adv_unit_select_all_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct unit *pUnit = pWidget->data.unit;

    popdown_advanced_terrain_dialog();

    if (pUnit) {
      activate_all_units(unit_tile(pUnit));
    }
  }
  return -1;
}

/**********************************************************************//**
  Sentry unit widget contains.
**************************************************************************/
static int adv_unit_sentry_idle_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct unit *pUnit = pWidget->data.unit;

    popdown_advanced_terrain_dialog();

    if (pUnit) {
      struct tile *ptile = unit_tile(pUnit);

      unit_list_iterate(ptile->units, punit) {
        if (unit_owner(punit) == client.conn.playing
            && ACTIVITY_IDLE == punit->activity
            && !punit->ai_controlled
            && can_unit_do_activity(punit, ACTIVITY_SENTRY)) {
          request_new_unit_activity(punit, ACTIVITY_SENTRY);
        }
      } unit_list_iterate_end;
    }
  }
  return -1;
}

/**********************************************************************//**
  Initiate goto to selected tile.
**************************************************************************/
static int goto_here_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    int x = pWidget->data.cont->id0;
    int y = pWidget->data.cont->id1;

    popdown_advanced_terrain_dialog();

    /* may not work */
    send_goto_tile(head_of_units_in_focus(),
                   map_pos_to_tile(&(wld.map), x, y));
  }

  return -1;
}

/**********************************************************************//**
  Initiate patrol to selected tile.
**************************************************************************/
static int patrol_here_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {

/* FIXME */
#if 0
    int x = pWidget->data.cont->id0;
    int y = pWidget->data.cont->id1;
    struct unit *pUnit = head_of_units_in_focus();
#endif

    popdown_advanced_terrain_dialog();

#if 0
    if (pUnit) {
      enter_goto_state(pUnit);
      /* may not work */
      do_unit_patrol_to(pUnit, map_pos_to_tile(x, y));
      exit_goto_state();
    }
#endif /* 0 */
  }
  return -1;
}

/**********************************************************************//**
  Initiate paradrop to selected tile.
**************************************************************************/
static int paradrop_here_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
/* FIXME */
#if 0
    int x = pWidget->data.cont->id0;
    int y = pWidget->data.cont->id1;
#endif

    popdown_advanced_terrain_dialog();

#if 0
    /* may not work */
    do_unit_paradrop_to(get_unit_in_focus(), map_pos_to_tile(x, y));
#endif
  }
  return -1;
}

/**********************************************************************//**
  Show help about unit type.
**************************************************************************/
static int unit_help_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    Unit_type_id unit_id = MAX_ID - pWidget->ID;

    popdown_advanced_terrain_dialog();
    popup_unit_info(unit_id);
  }
  return -1;
}

/**********************************************************************//**
  Popup a generic dialog to display some generic information about
  terrain : tile, units , cities, etc.
**************************************************************************/
void popup_advanced_terrain_dialog(struct tile *ptile, Uint16 pos_x, Uint16 pos_y)
{
  struct widget *pWindow = NULL, *pBuf = NULL;
  struct city *pCity;
  struct unit *pFocus_Unit;
  utf8_str *pstr;
  SDL_Rect area2;
  struct CONTAINER *pCont;
  char cBuf[255];
  int n, w = 0, h, units_h = 0;
  SDL_Rect area;

  if (pAdvanced_Terrain_Dlg) {
    return;
  }

  pCity = tile_city(ptile);
  n = unit_list_size(ptile->units);
  pFocus_Unit = head_of_units_in_focus();

  if (!n && !pCity && !pFocus_Unit) {
    popup_terrain_info_dialog(NULL, ptile);

    return;
  }

  area.h = adj_size(2);
  is_unit_move_blocked = TRUE;

  pAdvanced_Terrain_Dlg = fc_calloc(1, sizeof(struct ADVANCED_DLG));

  pCont = fc_calloc(1, sizeof(struct CONTAINER));
  pCont->id0 = index_to_map_pos_x(tile_index(ptile));
  pCont->id1 = index_to_map_pos_y(tile_index(ptile));

  pstr = create_utf8_from_char(_("Advanced Menu") , adj_font(12));
  pstr->style |= TTF_STYLE_BOLD;

  pWindow = create_window_skeleton(NULL, pstr, 0);

  pWindow->action = advanced_terrain_window_dlg_callback;
  set_wstate(pWindow , FC_WS_NORMAL);

  add_to_gui_list(ID_TERRAIN_ADV_DLG_WINDOW, pWindow);
  pAdvanced_Terrain_Dlg->pEndWidgetList = pWindow;

  area = pWindow->area;

  /* ---------- */
  /* exit button */
  pBuf = create_themeicon(current_theme->Small_CANCEL_Icon, pWindow->dst,
                          WF_WIDGET_HAS_INFO_LABEL | WF_RESTORE_BACKGROUND);
  pBuf->info_label = create_utf8_from_char(_("Close Dialog (Esc)"),
                                           adj_font(12));
  area.w += pBuf->size.w + adj_size(10);
  pBuf->action = exit_advanced_terrain_dlg_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->key = SDLK_ESCAPE;

  add_to_gui_list(ID_TERRAIN_ADV_DLG_EXIT_BUTTON, pBuf);
  /* ---------- */

  pstr = create_utf8_from_char(_("Terrain Info") , adj_font(10));
  pstr->style |= TTF_STYLE_BOLD;

  pBuf = create_iconlabel(NULL, pWindow->dst, pstr,
    (WF_RESTORE_BACKGROUND|WF_DRAW_TEXT_LABEL_WITH_SPACE|WF_FREE_DATA));

  pBuf->string_utf8->bgcol = (SDL_Color) {0, 0, 0, 0};

  pBuf->data.cont = pCont;

  pBuf->action = terrain_info_callback;
  set_wstate(pBuf, FC_WS_NORMAL);

  add_to_gui_list(ID_LABEL, pBuf);

  area.w = MAX(area.w, pBuf->size.w);
  area.h += pBuf->size.h;

  /* ---------- */
  if (pCity && city_owner(pCity) == client.conn.playing) {
    /* separator */
    pBuf = create_iconlabel(NULL, pWindow->dst, NULL, WF_FREE_THEME);

    add_to_gui_list(ID_SEPARATOR, pBuf);
    area.h += pBuf->next->size.h;
    /* ------------------ */

    fc_snprintf(cBuf, sizeof(cBuf), _("Zoom to : %s"), city_name_get(pCity));

    create_active_iconlabel(pBuf, pWindow->dst,
                            pstr, cBuf, zoom_to_city_callback);
    pBuf->data.city = pCity;
    set_wstate(pBuf, FC_WS_NORMAL);

    add_to_gui_list(ID_LABEL, pBuf);

    area.w = MAX(area.w, pBuf->size.w);
    area.h += pBuf->size.h;
    /* ----------- */

    create_active_iconlabel(pBuf, pWindow->dst, pstr,
                            _("Change Production"), change_production_callback);

    pBuf->data.city = pCity;
    set_wstate(pBuf, FC_WS_NORMAL);

    add_to_gui_list(ID_LABEL, pBuf);

    area.w = MAX(area.w, pBuf->size.w);
    area.h += pBuf->size.h;
    /* -------------- */

    create_active_iconlabel(pBuf, pWindow->dst, pstr,
                            _("Hurry production"), hurry_production_callback);

    pBuf->data.city = pCity;
    set_wstate(pBuf, FC_WS_NORMAL);

    add_to_gui_list(ID_LABEL, pBuf);

    area.w = MAX(area.w, pBuf->size.w);
    area.h += pBuf->size.h;
    /* ----------- */

    create_active_iconlabel(pBuf, pWindow->dst, pstr,
                            _("Change City Governor settings"), cma_callback);

    pBuf->data.city = pCity;
    set_wstate(pBuf, FC_WS_NORMAL);

    add_to_gui_list(ID_LABEL, pBuf);

    area.w = MAX(area.w, pBuf->size.w);
    area.h += pBuf->size.h;
  }
  /* ---------- */

  if (pFocus_Unit
      && (tile_index(unit_tile(pFocus_Unit)) != tile_index(ptile))) {
    /* separator */
    pBuf = create_iconlabel(NULL, pWindow->dst, NULL, WF_FREE_THEME);

    add_to_gui_list(ID_SEPARATOR, pBuf);
    area.h += pBuf->next->size.h;
    /* ------------------ */

    create_active_iconlabel(pBuf, pWindow->dst, pstr, _("Goto here"),
                            goto_here_callback);
    pBuf->data.cont = pCont;
    set_wstate(pBuf, FC_WS_NORMAL);

    add_to_gui_list(MAX_ID - 1000 - pFocus_Unit->id, pBuf);

    area.w = MAX(area.w, pBuf->size.w);
    area.h += pBuf->size.h;
    /* ----------- */

    create_active_iconlabel(pBuf, pWindow->dst, pstr, _("Patrol here"),
                            patrol_here_callback);
    pBuf->data.cont = pCont;
    set_wstate(pBuf, FC_WS_NORMAL);

    add_to_gui_list(MAX_ID - 1000 - pFocus_Unit->id, pBuf);

    area.w = MAX(area.w, pBuf->size.w);
    area.h += pBuf->size.h;
    /* ----------- */

#if 0 /* FIXME: specific connect buttons */
    if (unit_has_type_flag(pFocus_Unit, UTYF_SETTLERS)) {
      create_active_iconlabel(pBuf, pWindow->dst->surface, pstr, _("Connect here"),
                              connect_here_callback);
      pBuf->data.cont = pCont;
      set_wstate(pBuf, FC_WS_NORMAL);

      add_to_gui_list(ID_LABEL, pBuf);

      area.w = MAX(area.w, pBuf->size.w);
      area.h += pBuf->size.h;
    }
#endif /* 0 */

    /* FIXME: This logic seems to try to mirror do_paradrop() why? */
    if (can_unit_paradrop(pFocus_Unit) && client_tile_get_known(ptile)
        && !(((pCity && pplayers_non_attack(client.conn.playing, city_owner(pCity)))
              || is_non_attack_unit_tile(ptile, client.conn.playing)))
        && (unit_type_get(pFocus_Unit)->paratroopers_range >=
            real_map_distance(unit_tile(pFocus_Unit), ptile))) {

      create_active_iconlabel(pBuf, pWindow->dst, pstr, _("Paradrop here"),
                              paradrop_here_callback);
      pBuf->data.cont = pCont;
      set_wstate(pBuf, FC_WS_NORMAL);

      add_to_gui_list(ID_LABEL, pBuf);

      area.w = MAX(area.w, pBuf->size.w);
      area.h += pBuf->size.h;
    }

  }
  pAdvanced_Terrain_Dlg->pBeginWidgetList = pBuf;

  /* ---------- */
  if (n) {
    int i;
    struct unit *pUnit;
    struct unit_type *pUnitType = NULL;

    units_h = 0;
    /* separator */
    pBuf = create_iconlabel(NULL, pWindow->dst, NULL, WF_FREE_THEME);

    add_to_gui_list(ID_SEPARATOR, pBuf);
    area.h += pBuf->next->size.h;
    /* ---------- */
    if (n > 1) {
      struct unit *pDefender, *pAttacker;
      struct widget *pLast = pBuf;
      bool reset = FALSE;
      int my_units = 0;
      const char *vetname;

      #define ADV_NUM_SEEN  15

      pDefender = (pFocus_Unit ? get_defender(pFocus_Unit, ptile) : NULL);
      pAttacker = (pFocus_Unit ? get_attacker(pFocus_Unit, ptile) : NULL);
      for (i = 0; i < n; i++) {
        pUnit = unit_list_get(ptile->units, i);
        if (pUnit == pFocus_Unit) {
          continue;
        }
        pUnitType = unit_type_get(pUnit);
        vetname = utype_veteran_name_translation(pUnitType, pUnit->veteran);

        if (unit_owner(pUnit) == client.conn.playing) {
          fc_snprintf(cBuf, sizeof(cBuf),
                      _("Activate %s (%d / %d) %s (%d,%d,%s) %s"),
                      (vetname != NULL ? vetname : ""),
                      pUnit->hp, pUnitType->hp,
                      utype_name_translation(pUnitType),
                      pUnitType->attack_strength,
                      pUnitType->defense_strength,
                      move_points_text(pUnitType->move_rate, FALSE),
                      unit_activity_text(pUnit));
    
          create_active_iconlabel(pBuf, pWindow->dst, pstr,
                                  cBuf, adv_unit_select_callback);
          pBuf->data.unit = pUnit;
          set_wstate(pBuf, FC_WS_NORMAL);
          add_to_gui_list(ID_LABEL, pBuf);
          my_units++;
        } else {
          int att_chance, def_chance;

          fc_snprintf(cBuf, sizeof(cBuf), _("%s %s %s (A:%d D:%d M:%s FP:%d) HP:%d%%"),
                      nation_adjective_for_player(unit_owner(pUnit)),
                      (vetname != NULL ? vetname : ""),
                      utype_name_translation(pUnitType),
                      pUnitType->attack_strength,
                      pUnitType->defense_strength,
                      move_points_text(pUnitType->move_rate, FALSE),
                      pUnitType->firepower,
                      ((pUnit->hp * 100) / pUnitType->hp));

          /* calculate chance to win */
          if (sdl_get_chance_to_win(&att_chance, &def_chance, pUnit, pFocus_Unit)) {
            /* TRANS: "CtW" = "Chance to Win" */
            cat_snprintf(cBuf, sizeof(cBuf), _(" CtW: Att:%d%% Def:%d%%"),
                         att_chance, def_chance);
          }

          if (pAttacker && pAttacker == pUnit) {
            pstr->fgcol = *(get_game_color(COLOR_OVERVIEW_ENEMY_UNIT));
            reset = TRUE;
          } else {
            if (pDefender && pDefender == pUnit) {
              pstr->fgcol = *(get_game_color(COLOR_OVERVIEW_MY_UNIT));
              reset = TRUE;
            }
          }

	  create_active_iconlabel(pBuf, pWindow->dst, pstr, cBuf, NULL);

	  if (reset) {
            pstr->fgcol = *get_theme_color(COLOR_THEME_ADVANCEDTERRAINDLG_TEXT);
            reset = FALSE;
	  }

	  add_to_gui_list(ID_LABEL, pBuf);
	}

        area.w = MAX(area.w, pBuf->size.w);
        units_h += pBuf->size.h;

        if (i > ADV_NUM_SEEN - 1) {
          set_wflag(pBuf, WF_HIDDEN);
        }
      }

      pAdvanced_Terrain_Dlg->pEndActiveWidgetList = pLast->prev;
      pAdvanced_Terrain_Dlg->pActiveWidgetList = pAdvanced_Terrain_Dlg->pEndActiveWidgetList;
      pAdvanced_Terrain_Dlg->pBeginWidgetList = pBuf;
      pAdvanced_Terrain_Dlg->pBeginActiveWidgetList = pAdvanced_Terrain_Dlg->pBeginWidgetList;

      if (n > ADV_NUM_SEEN) {
        units_h = ADV_NUM_SEEN * pBuf->size.h;
        n = create_vertical_scrollbar(pAdvanced_Terrain_Dlg,
                                      1, ADV_NUM_SEEN, TRUE, TRUE);
        area.w += n;
      }

      if (my_units > 1) {
        fc_snprintf(cBuf, sizeof(cBuf), "%s (%d)", _("Ready all"), my_units);
        create_active_iconlabel(pBuf, pWindow->dst, pstr,
                                cBuf, adv_unit_select_all_callback);
        pBuf->data.unit = pAdvanced_Terrain_Dlg->pEndActiveWidgetList->data.unit;
        set_wstate(pBuf, FC_WS_NORMAL);
        pBuf->ID = ID_LABEL;
        DownAdd(pBuf, pLast);
        area.h += pBuf->size.h;

        fc_snprintf(cBuf, sizeof(cBuf), "%s (%d)", _("Sentry idle"), my_units);
        create_active_iconlabel(pBuf, pWindow->dst, pstr,
                                cBuf, adv_unit_sentry_idle_callback);
        pBuf->data.unit = pAdvanced_Terrain_Dlg->pEndActiveWidgetList->data.unit;
        set_wstate(pBuf, FC_WS_NORMAL);
        pBuf->ID = ID_LABEL;
        DownAdd(pBuf, pLast->prev);
        area.h += pBuf->size.h;

        /* separator */
        pBuf = create_iconlabel(NULL, pWindow->dst, NULL, WF_FREE_THEME);
        pBuf->ID = ID_SEPARATOR;
        DownAdd(pBuf, pLast->prev->prev);
        area.h += pBuf->next->size.h;
      }
#undef ADV_NUM_SEEN
    } else { /* n == 1 */
      /* one unit - give orders */
      pUnit = unit_list_get(ptile->units, 0);
      pUnitType = unit_type_get(pUnit);
      if (pUnit != pFocus_Unit) {
        const char *vetname;

        vetname = utype_veteran_name_translation(pUnitType, pUnit->veteran);
        if ((pCity && city_owner(pCity) == client.conn.playing)
            || (unit_owner(pUnit) == client.conn.playing)) {
          fc_snprintf(cBuf, sizeof(cBuf),
                      _("Activate %s (%d / %d) %s (%d,%d,%s) %s"),
                      (vetname != NULL ? vetname : ""),
                      pUnit->hp, pUnitType->hp,
                      utype_name_translation(pUnitType),
                      pUnitType->attack_strength,
                      pUnitType->defense_strength,
                      move_points_text(pUnitType->move_rate, FALSE),
                      unit_activity_text(pUnit));

          create_active_iconlabel(pBuf, pWindow->dst, pstr,
                                  cBuf, adv_unit_select_callback);
          pBuf->data.unit = pUnit;
          set_wstate(pBuf, FC_WS_NORMAL);

          add_to_gui_list(ID_LABEL, pBuf);

          area.w = MAX(area.w, pBuf->size.w);
          units_h += pBuf->size.h;
          /* ---------------- */
          /* separator */
          pBuf = create_iconlabel(NULL, pWindow->dst, NULL, WF_FREE_THEME);

          add_to_gui_list(ID_SEPARATOR, pBuf);
          area.h += pBuf->next->size.h;
        } else {
          int att_chance, def_chance;

          fc_snprintf(cBuf, sizeof(cBuf), _("%s %s %s (A:%d D:%d M:%s FP:%d) HP:%d%%"),
                      nation_adjective_for_player(unit_owner(pUnit)),
                      (vetname != NULL ? vetname : ""),
                      utype_name_translation(pUnitType),
                      pUnitType->attack_strength,
                      pUnitType->defense_strength,
                      move_points_text(pUnitType->move_rate, FALSE),
                      pUnitType->firepower,
                      ((pUnit->hp * 100) / pUnitType->hp));

          /* calculate chance to win */
            if (sdl_get_chance_to_win(&att_chance, &def_chance, pUnit, pFocus_Unit)) {
              cat_snprintf(cBuf, sizeof(cBuf), _(" CtW: Att:%d%% Def:%d%%"),
                           att_chance, def_chance);
            }
            create_active_iconlabel(pBuf, pWindow->dst, pstr, cBuf, NULL);
            add_to_gui_list(ID_LABEL, pBuf);
            area.w = MAX(area.w, pBuf->size.w);
            units_h += pBuf->size.h;
            /* ---------------- */

            /* separator */
            pBuf = create_iconlabel(NULL, pWindow->dst, NULL, WF_FREE_THEME);

            add_to_gui_list(ID_SEPARATOR, pBuf);
            area.h += pBuf->next->size.h;
        }
      }
      /* ---------------- */
      fc_snprintf(cBuf, sizeof(cBuf),
            _("Look up \"%s\" in the Help Browser"),
            utype_name_translation(pUnitType));
      create_active_iconlabel(pBuf, pWindow->dst, pstr,
                              cBuf, unit_help_callback);
      set_wstate(pBuf , FC_WS_NORMAL);
      add_to_gui_list(MAX_ID - utype_number(pUnitType), pBuf);

      area.w = MAX(area.w, pBuf->size.w);
      units_h += pBuf->size.h;
      /* ---------------- */
      pAdvanced_Terrain_Dlg->pBeginWidgetList = pBuf;
    }

  }
  /* ---------- */

  area.w += adj_size(2);
  area.h += units_h;

  resize_window(pWindow, NULL, NULL,
                (pWindow->size.w - pWindow->area.w) + area.w,
                (pWindow->size.h - pWindow->area.h) + area.h);

  area = pWindow->area;

  widget_set_position(pWindow, pos_x, pos_y);

  w = area.w - adj_size(2);

  if (pAdvanced_Terrain_Dlg->pScroll) {
    units_h = n;
  } else {
    units_h = 0;
  }

  /* exit button */
  pBuf = pWindow->prev;

  pBuf->size.x = area.x + area.w - pBuf->size.w - 1;
  pBuf->size.y = pWindow->size.y + adj_size(2);

  /* terrain info */
  pBuf = pBuf->prev;

  pBuf->size.x = area.x + 1;
  pBuf->size.y = area.y + 1;
  pBuf->size.w = w;
  h = pBuf->size.h;

  area2.x = adj_size(10);
  area2.h = adj_size(2);

  pBuf = pBuf->prev;
  while (pBuf) {
    if (pBuf == pAdvanced_Terrain_Dlg->pEndActiveWidgetList) {
      w -= units_h;
    }

    pBuf->size.w = w;
    pBuf->size.x = pBuf->next->size.x;
    pBuf->size.y = pBuf->next->size.y + pBuf->next->size.h;

    if (pBuf->ID == ID_SEPARATOR) {
      FREESURFACE(pBuf->theme);
      pBuf->size.h = h;
      pBuf->theme = create_surf(w , h , SDL_SWSURFACE);

      area2.y = pBuf->size.h / 2 - 1;
      area2.w = pBuf->size.w - adj_size(20);

      SDL_FillRect(pBuf->theme, &area2, map_rgba(pBuf->theme->format,
                                      *get_theme_color(COLOR_THEME_ADVANCEDTERRAINDLG_TEXT)));
    }

    if (pBuf == pAdvanced_Terrain_Dlg->pBeginWidgetList
        || pBuf == pAdvanced_Terrain_Dlg->pBeginActiveWidgetList) {
      break;
    }
    pBuf = pBuf->prev;
  }

  if (pAdvanced_Terrain_Dlg->pScroll) {
    setup_vertical_scrollbar_area(pAdvanced_Terrain_Dlg->pScroll,
	area.x + area.w,
    	pAdvanced_Terrain_Dlg->pEndActiveWidgetList->size.y,
    	area.y - pAdvanced_Terrain_Dlg->pEndActiveWidgetList->size.y + area.h,
        TRUE);
  }

  /* -------------------- */
  /* redraw */
  redraw_group(pAdvanced_Terrain_Dlg->pBeginWidgetList, pWindow, 0);

  widget_flush(pWindow);
}

/* ====================================================================== */
/* ============================ PILLAGE DIALOG ========================== */
/* ====================================================================== */
static struct SMALL_DLG *pPillage_Dlg = NULL;

/**********************************************************************//**
  User interacted with pillage dialog.
**************************************************************************/
static int pillage_window_callback(struct widget *pWindow)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    move_window_group(pPillage_Dlg->pBeginWidgetList, pWindow);
  }
  return -1;
}

/**********************************************************************//**
  User selected what to pillage.
**************************************************************************/
static int pillage_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct unit *pUnit = pWidget->data.unit;
    int what = MAX_ID - pWidget->ID;

    popdown_pillage_dialog();

    if (pUnit) {
      struct extra_type *target = extra_by_number(what);

      request_new_unit_activity_targeted(pUnit, ACTIVITY_PILLAGE, target);
    }
  }
  return -1;
}

/**********************************************************************//**
  User requested closing of pillage dialog.
**************************************************************************/
static int exit_pillage_dlg_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    popdown_pillage_dialog();
  }
  return -1;
}

/**********************************************************************//**
  Popdown a dialog asking the unit which improvement they would like to
  pillage.
**************************************************************************/
static void popdown_pillage_dialog(void)
{
  if (pPillage_Dlg) {
    is_unit_move_blocked = FALSE;
    popdown_window_group_dialog(pPillage_Dlg->pBeginWidgetList,
                                pPillage_Dlg->pEndWidgetList);
    FC_FREE(pPillage_Dlg);
    flush_dirty();
  }
}

/**********************************************************************//**
  Popup a dialog asking the unit which improvement they would like to
  pillage.
**************************************************************************/
void popup_pillage_dialog(struct unit *pUnit, bv_extras extras)
{
  struct widget *pWindow = NULL, *pBuf = NULL;
  utf8_str *pstr;
  SDL_Rect area;
  struct extra_type *tgt;

  if (pPillage_Dlg) {
    return;
  }

  is_unit_move_blocked = TRUE;
  pPillage_Dlg = fc_calloc(1, sizeof(struct SMALL_DLG));

  /* window */
  pstr = create_utf8_from_char(_("What To Pillage") , adj_font(12));
  pstr->style |= TTF_STYLE_BOLD;

  pWindow = create_window_skeleton(NULL, pstr, 0);

  pWindow->action = pillage_window_callback;
  set_wstate(pWindow, FC_WS_NORMAL);

  add_to_gui_list(ID_PILLAGE_DLG_WINDOW, pWindow);
  pPillage_Dlg->pEndWidgetList = pWindow;

  area = pWindow->area;

  area.h = MAX(area.h, adj_size(2));

  /* ---------- */
  /* exit button */
  pBuf = create_themeicon(current_theme->Small_CANCEL_Icon, pWindow->dst,
                          WF_WIDGET_HAS_INFO_LABEL | WF_RESTORE_BACKGROUND);
  pBuf->info_label = create_utf8_from_char(_("Close Dialog (Esc)"),
                                           adj_font(12));
  area.w += pBuf->size.w + adj_size(10);
  pBuf->action = exit_pillage_dlg_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->key = SDLK_ESCAPE;

  add_to_gui_list(ID_PILLAGE_DLG_EXIT_BUTTON, pBuf);
  /* ---------- */

  while ((tgt = get_preferred_pillage(extras))) {
    const char *name = NULL;
    int what;

    BV_CLR(extras, extra_index(tgt));
    name = extra_name_translation(tgt);
    what = extra_index(tgt);

    fc_assert(name != NULL);

    create_active_iconlabel(pBuf, pWindow->dst, pstr,
                            (char *) name, pillage_callback);

    pBuf->data.unit = pUnit;
    set_wstate(pBuf, FC_WS_NORMAL);

    add_to_gui_list(MAX_ID - what, pBuf);

    area.w = MAX(area.w, pBuf->size.w);
    area.h += pBuf->size.h;
  }
  pPillage_Dlg->pBeginWidgetList = pBuf;

  /* setup window size and start position */

  resize_window(pWindow, NULL, NULL,
                (pWindow->size.w - pWindow->area.w) + area.w,
                (pWindow->size.h - pWindow->area.h) + area.h);

  area = pWindow->area;

  put_window_near_map_tile(pWindow, pWindow->size.w, pWindow->size.h,
                           unit_tile(pUnit));

  /* setup widget size and start position */

  /* exit button */  
  pBuf = pWindow->prev;
  pBuf->size.x = area.x + area.w - pBuf->size.w - 1;
  pBuf->size.y = pWindow->size.y + adj_size(2);

  /* first special to pillage */
  pBuf = pBuf->prev;
  setup_vertical_widgets_position(1,
	area.x,	area.y + 1, area.w, 0,
	pPillage_Dlg->pBeginWidgetList, pBuf);

  /* --------------------- */
  /* redraw */
  redraw_group(pPillage_Dlg->pBeginWidgetList, pWindow, 0);

  widget_flush(pWindow);
}

/* ======================================================================= */
/* =========================== CONNECT DIALOG ============================ */
/* ======================================================================= */
static struct SMALL_DLG *pConnect_Dlg = NULL;

/**********************************************************************//**
  Popdown a dialog asking the unit how they want to "connect" their
  location to the destination.
**************************************************************************/
static void popdown_connect_dialog(void)
{
  if (pConnect_Dlg) {
    is_unit_move_blocked = FALSE;
    popdown_window_group_dialog(pConnect_Dlg->pBeginWidgetList,
                                pConnect_Dlg->pEndWidgetList);
    FC_FREE(pConnect_Dlg);
  }
}

/**************************************************************************
                                  Revolutions
**************************************************************************/
static struct SMALL_DLG *pRevolutionDlg = NULL;
  
/**********************************************************************//**
  User confirmed revolution.
**************************************************************************/
static int revolution_dlg_ok_callback(struct widget *pButton)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    start_revolution();

    popdown_revolution_dialog();

    flush_dirty();
  }
  return (-1);
}

/**********************************************************************//**
  User cancelled revolution.
**************************************************************************/
static int revolution_dlg_cancel_callback(struct widget *pCancel_Button)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    popdown_revolution_dialog();
    flush_dirty();
  }
  return (-1);
}

/**********************************************************************//**
  User requested move of revolution dialog.
**************************************************************************/
static int move_revolution_dlg_callback(struct widget *pWindow)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    move_window_group(pRevolutionDlg->pBeginWidgetList, pWindow);
  }
  return -1;
}

/**********************************************************************//**
  Close the revolution dialog.
**************************************************************************/
static void popdown_revolution_dialog(void)
{
  if (pRevolutionDlg) {
    popdown_window_group_dialog(pRevolutionDlg->pBeginWidgetList,
                                pRevolutionDlg->pEndWidgetList);
    FC_FREE(pRevolutionDlg);
    enable_and_redraw_revolution_button();
  }
}

/* ==================== Public ========================= */

/**************************************************************************
                           Select Goverment Type
**************************************************************************/
static struct SMALL_DLG *pGov_Dlg = NULL;

/**********************************************************************//**
  Close the government dialog.
**************************************************************************/
static void popdown_government_dialog(void)
{
  if (pGov_Dlg) {
    popdown_window_group_dialog(pGov_Dlg->pBeginWidgetList,
                                pGov_Dlg->pEndWidgetList);
    FC_FREE(pGov_Dlg);
    enable_and_redraw_revolution_button();
  }
}

/**********************************************************************//**
  User selected government button.
**************************************************************************/
static int government_dlg_callback(struct widget *pGov_Button)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    set_government_choice(government_by_number(MAX_ID - pGov_Button->ID));

    popdown_government_dialog();
  }
  return (-1);
}

/**********************************************************************//**
  User requested move of government dialog.
**************************************************************************/
static int move_government_dlg_callback(struct widget *pWindow)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    move_window_group(pGov_Dlg->pBeginWidgetList, pWindow);
  }
  return -1;
}

/**********************************************************************//**
  Public -

  Popup a dialog asking the player what government to switch to (this
  happens after a revolution completes).
**************************************************************************/
static void popup_government_dialog(void)
{
  SDL_Surface *pLogo = NULL;
  struct utf8_str *pstr = NULL;
  struct widget *pGov_Button = NULL;
  struct widget *pWindow = NULL;
  int j;
  Uint16 max_w = 0, max_h = 0;
  SDL_Rect area;

  if (pGov_Dlg) {
    return;
  }

  pGov_Dlg = fc_calloc(1, sizeof(struct SMALL_DLG));

  /* create window */
  pstr = create_utf8_from_char(_("Choose Your New Government"), adj_font(12));
  pstr->style |= TTF_STYLE_BOLD;
  /* this win. size is temp. */
  pWindow = create_window_skeleton(NULL, pstr, 0);
  pWindow->action = move_government_dlg_callback;
  add_to_gui_list(ID_GOVERNMENT_DLG_WINDOW, pWindow);

  pGov_Dlg->pEndWidgetList = pWindow;

  area = pWindow->area;

  /* create gov. buttons */
  j = 0;
  governments_iterate(pGov) {
    if (pGov == game.government_during_revolution) {
      continue;
    }

    if (can_change_to_government(client.conn.playing, pGov)) {
      pstr = create_utf8_from_char(government_name_translation(pGov), adj_font(12));
      pGov_Button =
          create_icon_button(get_government_surface(pGov), pWindow->dst, pstr, 0);
      pGov_Button->action = government_dlg_callback;

      max_w = MAX(max_w, pGov_Button->size.w);
      max_h = MAX(max_h, pGov_Button->size.h);

      /* ugly hack */
      add_to_gui_list((MAX_ID - government_number(pGov)), pGov_Button);
      j++;

    }
  } governments_iterate_end;

  pGov_Dlg->pBeginWidgetList = pGov_Button;

  max_w += adj_size(10);
  max_h += adj_size(4);

  area.w = MAX(area.w, max_w + adj_size(20));
  area.h = MAX(area.h, j * (max_h + adj_size(10)) + adj_size(5));

  /* create window background */
  pLogo = theme_get_background(theme, BACKGROUND_CHOOSEGOVERNMENTDLG);
  if (resize_window(pWindow, pLogo, NULL,
                    (pWindow->size.w - pWindow->area.w) + area.w,
                    (pWindow->size.h - pWindow->area.h) + area.h)) {
    FREESURFACE(pLogo);
  }

  area = pWindow->area;

  /* set window start positions */
  widget_set_position(pWindow,
                      (main_window_width() - pWindow->size.w) / 2,
                      (main_window_height() - pWindow->size.h) / 2);

  /* set buttons start positions and size */
  j = 1;
  while (pGov_Button != pGov_Dlg->pEndWidgetList) {
    pGov_Button->size.w = max_w;
    pGov_Button->size.h = max_h;
    pGov_Button->size.x = area.x + adj_size(10);
    pGov_Button->size.y = area.y + area.h - (j++) * (max_h + adj_size(10));
    set_wstate(pGov_Button, FC_WS_NORMAL);

    pGov_Button = pGov_Button->next;
  }

  set_wstate(pWindow, FC_WS_NORMAL);

  /* redraw */
  redraw_group(pGov_Dlg->pBeginWidgetList, pWindow, 0);

  widget_flush(pWindow);
}

/**********************************************************************//**
  Popup a dialog asking if the player wants to start a revolution.
**************************************************************************/
void popup_revolution_dialog(void)
{
  SDL_Surface *pLogo;
  utf8_str *pstr = NULL;
  struct widget *pLabel = NULL;
  struct widget *pWindow = NULL;
  struct widget *pCancel_Button = NULL;
  struct widget *pOK_Button = NULL;
  SDL_Rect area;

  if (pRevolutionDlg) {
    return;
  }

  if (0 <= client.conn.playing->revolution_finishes) {
    popup_government_dialog();
    return;
  }

  pRevolutionDlg = fc_calloc(1, sizeof(struct SMALL_DLG));

  pstr = create_utf8_from_char(_("REVOLUTION!"), adj_font(12));
  pstr->style |= TTF_STYLE_BOLD;

  pWindow = create_window_skeleton(NULL, pstr, 0);
  pWindow->action = move_revolution_dlg_callback;
  set_wstate(pWindow, FC_WS_NORMAL);
  add_to_gui_list(ID_REVOLUTION_DLG_WINDOW, pWindow);
  pRevolutionDlg->pEndWidgetList = pWindow;

  area = pWindow->area;

  /* create text label */
  pstr = create_utf8_from_char(_("You say you wanna revolution?"), adj_font(10));
  pstr->style |= (TTF_STYLE_BOLD|SF_CENTER);
  pstr->fgcol = *get_theme_color(COLOR_THEME_REVOLUTIONDLG_TEXT);
  pLabel = create_iconlabel(NULL, pWindow->dst, pstr, 0);
  add_to_gui_list(ID_REVOLUTION_DLG_LABEL, pLabel);

  /* create cancel button */
  pCancel_Button =
      create_themeicon_button_from_chars(current_theme->Small_CANCEL_Icon,
                                         pWindow->dst, _("Cancel"), adj_font(10), 0);
  pCancel_Button->action = revolution_dlg_cancel_callback;
  pCancel_Button->size.w += adj_size(6);
  set_wstate(pCancel_Button, FC_WS_NORMAL);
  add_to_gui_list(ID_REVOLUTION_DLG_CANCEL_BUTTON, pCancel_Button);

  /* create ok button */
  pOK_Button =
      create_themeicon_button_from_chars(current_theme->Small_OK_Icon,
                                         pWindow->dst, _("Revolution!"),
                                         adj_font(10), 0);
  pOK_Button->action = revolution_dlg_ok_callback;
  pOK_Button->key = SDLK_RETURN;
  set_wstate(pOK_Button, FC_WS_NORMAL);
  add_to_gui_list(ID_REVOLUTION_DLG_OK_BUTTON, pOK_Button);

  pRevolutionDlg->pBeginWidgetList = pOK_Button;

  if ((pOK_Button->size.w + pCancel_Button->size.w + adj_size(30)) >
      pLabel->size.w + adj_size(20)) {
    area.w = MAX(area.w, pOK_Button->size.w + pCancel_Button->size.w + adj_size(30));
  } else {
    area.w = MAX(area.w, pLabel->size.w + adj_size(20));
  }

  area.h = MAX(area.h, pOK_Button->size.h + pLabel->size.h + adj_size(24));

  /* create window background */
  pLogo = theme_get_background(theme, BACKGROUND_REVOLUTIONDLG);
  if (resize_window(pWindow, pLogo, NULL,
                    (pWindow->size.w - pWindow->area.w) + area.w,
                    (pWindow->size.h - pWindow->area.h) + area.h)) {
    FREESURFACE(pLogo);
  }

  area = pWindow->area;

  /* set start positions */
  widget_set_position(pWindow,
                      (main_window_width() - pWindow->size.w) / 2,
                      (main_window_height() - pWindow->size.h) / 2);

  pOK_Button->size.x = area.x + adj_size(10);
  pOK_Button->size.y = area.y + area.h - pOK_Button->size.h - adj_size(10);

  pCancel_Button->size.y = pOK_Button->size.y;
  pCancel_Button->size.x = area.x + area.w - pCancel_Button->size.w - adj_size(10);

  pLabel->size.x = area.x;
  pLabel->size.y = area.y + adj_size(4);
  pLabel->size.w = area.w;

  /* redraw */
  redraw_group(pOK_Button, pWindow, 0);
  widget_mark_dirty(pWindow);
  flush_dirty();
}

/**************************************************************************
                                Nation Wizard
**************************************************************************/
static struct ADVANCED_DLG *pNationDlg = NULL;
static struct SMALL_DLG *pHelpDlg = NULL;

struct NAT {
  unsigned char nation_style;      /* selected style */
  unsigned char selected_leader;   /* if not unique -> selected leader */
  Nation_type_id nation;           /* selected nation */
  bool leader_sex;                 /* selected leader sex */
  struct nation_set *set;
  struct widget *pChange_Sex;
  struct widget *pName_Edit;
  struct widget *pName_Next;
  struct widget *pName_Prev;
  struct widget *pset_name;
  struct widget *pset_next;
  struct widget *pset_prev;
};

static int next_set_callback(struct widget *next_button);
static int prev_set_callback(struct widget *prev_button);
static int nations_dialog_callback(struct widget *pWindow);
static int nation_button_callback(struct widget *pNation);
static int races_dialog_ok_callback(struct widget *pStart_Button);
static int races_dialog_cancel_callback(struct widget *pButton);
static int next_name_callback(struct widget *pNext_Button);
static int prev_name_callback(struct widget *pPrev_Button);
static int change_sex_callback(struct widget *pSex);
static void select_random_leader(Nation_type_id nation);
static void change_nation_label(void);

/**********************************************************************//**
  User interacted with nations dialog.
**************************************************************************/
static int nations_dialog_callback(struct widget *pWindow)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (select_window_group_dialog(pNationDlg->pBeginWidgetList, pWindow)) {
      widget_flush(pWindow);
    }
  }
  return -1;
}

/**********************************************************************//**
  User accepted nation.
**************************************************************************/
static int races_dialog_ok_callback(struct widget *pStart_Button)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct NAT *pSetup = (struct NAT *)(pNationDlg->pEndWidgetList->data.ptr);
    char *pstr = pSetup->pName_Edit->string_utf8->text;

    /* perform a minimum of sanity test on the name */
    if (strlen(pstr) == 0) {
      output_window_append(ftc_client, _("You must type a legal name."));
      selected_widget = pStart_Button;
      set_wstate(pStart_Button, FC_WS_SELECTED);
      widget_redraw(pStart_Button);
      widget_flush(pStart_Button);

      return (-1);
    }

    dsend_packet_nation_select_req(&client.conn, player_number(races_player),
                                   pSetup->nation,
                                   pSetup->leader_sex, pstr,
                                   pSetup->nation_style);

    popdown_races_dialog();  
    flush_dirty();
  }

  return -1;
}

/**********************************************************************//**
  User requested leader gender change.
**************************************************************************/
static int change_sex_callback(struct widget *pSex)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct NAT *pSetup = (struct NAT *)(pNationDlg->pEndWidgetList->data.ptr);

    if (pSetup->leader_sex) {
      copy_chars_to_utf8_str(pSetup->pChange_Sex->string_utf8, _("Female"));
    } else {
      copy_chars_to_utf8_str(pSetup->pChange_Sex->string_utf8, _("Male"));
    }
    pSetup->leader_sex = !pSetup->leader_sex;

    if (pSex) {
      selected_widget = pSex;
      set_wstate(pSex, FC_WS_SELECTED);

      widget_redraw(pSex);
      widget_flush(pSex);
    }
  }
  return -1;
}

/**********************************************************************//**
  User requested next leader name.
**************************************************************************/
static int next_name_callback(struct widget *pNext)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct NAT *pSetup = (struct NAT *)(pNationDlg->pEndWidgetList->data.ptr);
    const struct nation_leader_list *leaders =
        nation_leaders(nation_by_number(pSetup->nation));
    const struct nation_leader *pleader;

    pSetup->selected_leader++;
    pleader = nation_leader_list_get(leaders, pSetup->selected_leader);

    /* change leader sex */
    if (pSetup->leader_sex != nation_leader_is_male(pleader)) {
      change_sex_callback(NULL);
    }

    /* change leader name */
    copy_chars_to_utf8_str(pSetup->pName_Edit->string_utf8,
                           nation_leader_name(pleader));

    FC_FREE(pLeaderName);
    pLeaderName = fc_strdup(nation_leader_name(pleader));

    if (nation_leader_list_size(leaders) - 1 == pSetup->selected_leader) {
      set_wstate(pSetup->pName_Next, FC_WS_DISABLED);
    }

    if (get_wstate(pSetup->pName_Prev) == FC_WS_DISABLED) {
      set_wstate(pSetup->pName_Prev, FC_WS_NORMAL);
    }

    if (!(get_wstate(pSetup->pName_Next) == FC_WS_DISABLED)) {
      selected_widget = pSetup->pName_Next;
      set_wstate(pSetup->pName_Next, FC_WS_SELECTED);
    }

    widget_redraw(pSetup->pName_Edit);
    widget_redraw(pSetup->pName_Prev);
    widget_redraw(pSetup->pName_Next);
    widget_mark_dirty(pSetup->pName_Edit);
    widget_mark_dirty(pSetup->pName_Prev);
    widget_mark_dirty(pSetup->pName_Next);

    widget_redraw(pSetup->pChange_Sex);
    widget_mark_dirty(pSetup->pChange_Sex);

    flush_dirty();
  }
  return -1;
}

/**********************************************************************//**
  User requested previous leader name.
**************************************************************************/
static int prev_name_callback(struct widget *pPrev)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct NAT *pSetup = (struct NAT *)(pNationDlg->pEndWidgetList->data.ptr);
    const struct nation_leader_list *leaders =
        nation_leaders(nation_by_number(pSetup->nation));
    const struct nation_leader *pleader;

    pSetup->selected_leader--;
    pleader = nation_leader_list_get(leaders, pSetup->selected_leader);

    /* change leader sex */
    if (pSetup->leader_sex != nation_leader_is_male(pleader)) {
      change_sex_callback(NULL);
    }

    /* change leader name */
    copy_chars_to_utf8_str(pSetup->pName_Edit->string_utf8,
                           nation_leader_name(pleader));

    FC_FREE(pLeaderName);
    pLeaderName = fc_strdup(nation_leader_name(pleader));

    if (!pSetup->selected_leader) {
      set_wstate(pSetup->pName_Prev, FC_WS_DISABLED);
    }

    if (get_wstate(pSetup->pName_Next) == FC_WS_DISABLED) {
      set_wstate(pSetup->pName_Next, FC_WS_NORMAL);
    }

    if (!(get_wstate(pSetup->pName_Prev) == FC_WS_DISABLED)) {
      selected_widget = pSetup->pName_Prev;
      set_wstate(pSetup->pName_Prev, FC_WS_SELECTED);
    }

    widget_redraw(pSetup->pName_Edit);
    widget_redraw(pSetup->pName_Prev);
    widget_redraw(pSetup->pName_Next);
    widget_mark_dirty(pSetup->pName_Edit);
    widget_mark_dirty(pSetup->pName_Prev);
    widget_mark_dirty(pSetup->pName_Next);

    widget_redraw(pSetup->pChange_Sex);
    widget_mark_dirty(pSetup->pChange_Sex);

    flush_dirty();
  }
  return -1;
}

/**********************************************************************//**
  User requested next nationset
**************************************************************************/
static int next_set_callback(struct widget *next_button)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct NAT *pSetup = (struct NAT *)(pNationDlg->pEndWidgetList->data.ptr);
    struct option *poption = optset_option_by_name(server_optset, "nationset");

    fc_assert(pSetup->set != NULL
              && nation_set_index(pSetup->set) < nation_set_count() - 1);

    pSetup->set = nation_set_by_number(nation_set_index(pSetup->set) + 1);

    option_str_set(poption, nation_set_rule_name(pSetup->set));
  }

  return -1;
}

/**********************************************************************//**
  User requested prev nationset
**************************************************************************/
static int prev_set_callback(struct widget *prev_button)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct NAT *pSetup = (struct NAT *)(pNationDlg->pEndWidgetList->data.ptr);
    struct option *poption = optset_option_by_name(server_optset, "nationset");

    fc_assert(pSetup->set != NULL && nation_set_index(pSetup->set) > 0);

    pSetup->set = nation_set_by_number(nation_set_index(pSetup->set) - 1);

    option_str_set(poption, nation_set_rule_name(pSetup->set));
  }

  return -1;
}

/**********************************************************************//**
  User cancelled nations dialog.
**************************************************************************/
static int races_dialog_cancel_callback(struct widget *pButton)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    popdown_races_dialog();
    flush_dirty();
  }
  return -1;
}

/**********************************************************************//**
  User interacted with style widget.
**************************************************************************/
static int style_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct NAT *pSetup = (struct NAT *)(pNationDlg->pEndWidgetList->data.ptr);
    struct widget *pGUI = get_widget_pointer_form_main_list(MAX_ID - 1000 -
                                                            pSetup->nation_style);

    set_wstate(pGUI, FC_WS_NORMAL);
    widget_redraw(pGUI);
    widget_mark_dirty(pGUI);

    set_wstate(pWidget, FC_WS_DISABLED);
    widget_redraw(pWidget);
    widget_mark_dirty(pWidget);

    pSetup->nation_style = MAX_ID - 1000 - pWidget->ID;

    flush_dirty();
    selected_widget = NULL;
  }
  return -1;
}

/**********************************************************************//**
  User interacted with help dialog.
**************************************************************************/
static int help_dlg_callback(struct widget *pWindow)
{
  return -1;
}

/**********************************************************************//**
  User requested closing of help dialog.
**************************************************************************/
static int cancel_help_dlg_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (pHelpDlg) {
      popdown_window_group_dialog(pHelpDlg->pBeginWidgetList,
                                  pHelpDlg->pEndWidgetList);
      FC_FREE(pHelpDlg);
      if (pWidget) {
        flush_dirty();
      }
    }
  }
  return -1;
}

/**********************************************************************//**
  User selected nation.
**************************************************************************/
static int nation_button_callback(struct widget *pNationButton)
{
  set_wstate(pNationButton, FC_WS_SELECTED);
  selected_widget = pNationButton;

  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct NAT *pSetup = (struct NAT *)(pNationDlg->pEndWidgetList->data.ptr);

    if (pSetup->nation == MAX_ID - pNationButton->ID) {
      widget_redraw(pNationButton);
      widget_flush(pNationButton);
      return -1;
    }

    pSetup->nation = MAX_ID - pNationButton->ID;

    change_nation_label();

    enable(MAX_ID - 1000 - pSetup->nation_style);
    pSetup->nation_style = style_number(style_of_nation(nation_by_number(pSetup->nation)));
    disable(MAX_ID - 1000 - pSetup->nation_style);

    select_random_leader(pSetup->nation);

    redraw_group(pNationDlg->pBeginWidgetList, pNationDlg->pEndWidgetList, 0);
    widget_flush(pNationDlg->pEndWidgetList);
  } else {
    /* pop up nation description */
    struct widget *pWindow, *pOK_Button;
    utf8_str *pstr;
    SDL_Surface *pText;
    SDL_Rect area, area2;
    struct nation_type *pNation = nation_by_number(MAX_ID - pNationButton->ID);

    widget_redraw(pNationButton);
    widget_mark_dirty(pNationButton);

    if (!pHelpDlg) {
      pHelpDlg = fc_calloc(1, sizeof(struct SMALL_DLG));

      pstr = create_utf8_from_char(nation_plural_translation(pNation),
                                   adj_font(12));
      pstr->style |= TTF_STYLE_BOLD;

      pWindow = create_window_skeleton(NULL, pstr, 0);
      pWindow->action = help_dlg_callback;

      set_wstate(pWindow, FC_WS_NORMAL);

      pHelpDlg->pEndWidgetList = pWindow;
      add_to_gui_list(ID_WINDOW, pWindow);

      pOK_Button = create_themeicon_button_from_chars(current_theme->OK_Icon,
                               pWindow->dst, _("OK"), adj_font(14), 0);
      pOK_Button->action = cancel_help_dlg_callback;
      set_wstate(pOK_Button, FC_WS_NORMAL);
      pOK_Button->key = SDLK_ESCAPE;
      add_to_gui_list(ID_BUTTON, pOK_Button);
      pHelpDlg->pBeginWidgetList = pOK_Button;
    } else {
      pWindow = pHelpDlg->pEndWidgetList;
      pOK_Button = pHelpDlg->pBeginWidgetList;
      /* undraw window */
      widget_undraw(pWindow);
      widget_mark_dirty(pWindow);
    }

    area = pWindow->area;

    {
      char info[4096];

      helptext_nation(info, sizeof(info), pNation, NULL);
      pstr = create_utf8_from_char(info, adj_font(12));
    }

    pstr->fgcol = *get_theme_color(COLOR_THEME_NATIONDLG_LEGEND);
    pText = create_text_surf_smaller_than_w(pstr, main_window_width() - adj_size(20));

    FREEUTF8STR(pstr);

    /* create window background */
    area.w = MAX(area.w, pText->w + adj_size(20));
    area.w = MAX(area.w, pOK_Button->size.w + adj_size(20));
    area.h = MAX(area.h, adj_size(9) + pText->h
                         + adj_size(10) + pOK_Button->size.h + adj_size(10));

    resize_window(pWindow, NULL, get_theme_color(COLOR_THEME_BACKGROUND),
                  (pWindow->size.w - pWindow->area.w) + area.w,
                  (pWindow->size.h - pWindow->area.h) + area.h);

    widget_set_position(pWindow,
                        (main_window_width() - pWindow->size.w) / 2,
                        (main_window_height() - pWindow->size.h) / 2);

    area2.x = area.x + adj_size(7);
    area2.y = area.y + adj_size(6);
    alphablit(pText, NULL, pWindow->theme, &area2, 255);
    FREESURFACE(pText);

    pOK_Button->size.x = area.x + (area.w - pOK_Button->size.w) / 2;
    pOK_Button->size.y = area.y + area.h - pOK_Button->size.h - adj_size(10);

    /* redraw */
    redraw_group(pOK_Button, pWindow, 0);

    widget_mark_dirty(pWindow);

    flush_dirty();

  }
  return -1;
}

/**********************************************************************//**
  User interacted with leader name edit widget.
**************************************************************************/
static int leader_name_edit_callback(struct widget *pEdit)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (pEdit->string_utf8->text != NULL) {
      /* empty input -> restore previous content */
      copy_chars_to_utf8_str(pEdit->string_utf8, pLeaderName);
      widget_redraw(pEdit);
      widget_mark_dirty(pEdit);
      flush_dirty();
    }
  }

  return -1;
}
/* =========================================================== */

/**********************************************************************//**
  Update nation label.
**************************************************************************/
static void change_nation_label(void)
{
  SDL_Surface *pTmp_Surf, *pTmp_Surf_zoomed;
  struct widget *pWindow = pNationDlg->pEndWidgetList;
  struct NAT *pSetup = (struct NAT *)(pWindow->data.ptr);  
  struct widget *pLabel = pSetup->pName_Edit->next;
  struct nation_type *pNation = nation_by_number(pSetup->nation);

  pTmp_Surf = get_nation_flag_surface(pNation);
  pTmp_Surf_zoomed = zoomSurface(pTmp_Surf, DEFAULT_ZOOM * 1.0, DEFAULT_ZOOM * 1.0, 1);

  FREESURFACE(pLabel->theme);
  pLabel->theme = pTmp_Surf_zoomed;

  copy_chars_to_utf8_str(pLabel->string_utf8, nation_plural_translation(pNation));

  remake_label_size(pLabel);

  pLabel->size.x = pWindow->size.x + pWindow->size.w / 2 +
  				(pWindow->size.w/2 - pLabel->size.w) / 2;

}

/**********************************************************************//**
  Selectes a leader and the appropriate sex. Updates the gui elements
  and the selected_* variables.
**************************************************************************/
static void select_random_leader(Nation_type_id nation)
{
  struct NAT *pSetup = (struct NAT *)(pNationDlg->pEndWidgetList->data.ptr);
  const struct nation_leader_list *leaders =
      nation_leaders(nation_by_number(pSetup->nation));
  const struct nation_leader *pleader;

  pSetup->selected_leader = fc_rand(nation_leader_list_size(leaders));
  pleader = nation_leader_list_get(leaders, pSetup->selected_leader);
  copy_chars_to_utf8_str(pSetup->pName_Edit->string_utf8,
                         nation_leader_name(pleader));

  FC_FREE(pLeaderName);
  pLeaderName = fc_strdup(nation_leader_name(pleader));

  /* initialize leader sex */
  pSetup->leader_sex = nation_leader_is_male(pleader);

  if (pSetup->leader_sex) {
    copy_chars_to_utf8_str(pSetup->pChange_Sex->string_utf8, _("Male"));
  } else {
    copy_chars_to_utf8_str(pSetup->pChange_Sex->string_utf8, _("Female"));
  }

  /* disable navigation buttons */
  set_wstate(pSetup->pName_Prev, FC_WS_DISABLED);
  set_wstate(pSetup->pName_Next, FC_WS_DISABLED);

  if (1 < nation_leader_list_size(leaders)) {
    /* if selected leader is not the first leader, enable "previous leader" button */
    if (pSetup->selected_leader > 0) {
      set_wstate(pSetup->pName_Prev, FC_WS_NORMAL);
    }

    /* if selected leader is not the last leader, enable "next leader" button */
    if (pSetup->selected_leader < (nation_leader_list_size(leaders) - 1)) {
      set_wstate(pSetup->pName_Next, FC_WS_NORMAL);
    }
  }
}

/**********************************************************************//**
   Count available playable nations.
**************************************************************************/
static int get_playable_nation_count(void)
{
  int playable_nation_count = 0;

  nations_iterate(pnation) {
    if (pnation->is_playable && !pnation->player
        && is_nation_pickable(pnation))
      playable_nation_count++;
  } nations_iterate_end;

  return playable_nation_count;
}

/**********************************************************************//**
  Popup the nation selection dialog.
**************************************************************************/
void popup_races_dialog(struct player *pplayer)
{
  SDL_Color bg_color = {255,255,255,128};

  struct widget *pWindow, *pWidget = NULL, *pBuf, *pLast_City_Style;
  utf8_str *pstr;
  int len = 0;
  int w = adj_size(10), h = adj_size(10);
  SDL_Surface *pTmp_Surf, *pTmp_Surf_zoomed = NULL;
  SDL_Surface *pMain_Bg, *pText_Name, *pText_Class;
  SDL_Rect dst;
  float zoom;
  struct NAT *pSetup;
  SDL_Rect area;
  int i;
  struct nation_type *pnat;
  struct widget *nationsets = NULL;
  int natinfo_y, natinfo_h;

#define TARGETS_ROW 5
#define TARGETS_COL 1

  if (pNationDlg) {
    return;
  }

  races_player = pplayer;

  pNationDlg = fc_calloc(1, sizeof(struct ADVANCED_DLG));

  /* create window widget */
  pstr = create_utf8_from_char(_("What nation will you be?"), adj_font(12));
  pstr->style |= TTF_STYLE_BOLD;

  pWindow = create_window(NULL, pstr, w, h, WF_FREE_DATA);
  pWindow->action = nations_dialog_callback;
  set_wstate(pWindow, FC_WS_NORMAL);
  pSetup = fc_calloc(1, sizeof(struct NAT));
  pWindow->data.ptr = (void *)pSetup;

  pNationDlg->pEndWidgetList = pWindow;
  add_to_gui_list(ID_NATION_WIZARD_WINDOW, pWindow);
  /* --------------------------------------------------------- */
  /* create nations list */

  /* Create Imprv Background Icon */
  pMain_Bg = create_surf(adj_size(96*2), adj_size(64), SDL_SWSURFACE);

  SDL_FillRect(pMain_Bg, NULL, map_rgba(pMain_Bg->format, bg_color));

  create_frame(pMain_Bg,
               0, 0, pMain_Bg->w - 1, pMain_Bg->h - 1,
               get_theme_color(COLOR_THEME_NATIONDLG_FRAME));

  pstr = create_utf8_str(NULL, 0, adj_font(12));
  pstr->style |= (SF_CENTER|TTF_STYLE_BOLD);
  pstr->bgcol = (SDL_Color) {0, 0, 0, 0};

  /* fill list */
  pText_Class = NULL;

  nations_iterate(pNation) {
    if (!is_nation_playable(pNation) || !is_nation_pickable(pNation)) {
      continue;
    }

    pTmp_Surf_zoomed = adj_surf(get_nation_flag_surface(pNation));

    pTmp_Surf = crop_rect_from_surface(pMain_Bg, NULL);

    copy_chars_to_utf8_str(pstr, nation_plural_translation(pNation));
    change_ptsize_utf8(pstr, adj_font(12));
    pText_Name = create_text_surf_smaller_than_w(pstr, pTmp_Surf->w - adj_size(4));

    if (pNation->legend && *(pNation->legend) != '\0') {
      copy_chars_to_utf8_str(pstr, pNation->legend);
      change_ptsize_utf8(pstr, adj_font(10));
      pText_Class = create_text_surf_smaller_than_w(pstr, pTmp_Surf->w - adj_size(4));
    }

    dst.x = (pTmp_Surf->w - pTmp_Surf_zoomed->w) / 2;
    len = pTmp_Surf_zoomed->h +
      adj_size(10) + pText_Name->h + (pText_Class ? pText_Class->h : 0);
    dst.y = (pTmp_Surf->h - len) / 2;
    alphablit(pTmp_Surf_zoomed, NULL, pTmp_Surf, &dst, 255);
    dst.y += (pTmp_Surf_zoomed->h + adj_size(10));

    dst.x = (pTmp_Surf->w - pText_Name->w) / 2;
    alphablit(pText_Name, NULL, pTmp_Surf, &dst, 255);
    dst.y += pText_Name->h;
    FREESURFACE(pText_Name);

    if (pText_Class) {
      dst.x = (pTmp_Surf->w - pText_Class->w) / 2;
      alphablit(pText_Class, NULL, pTmp_Surf, &dst, 255);
      FREESURFACE(pText_Class);
    }

    pWidget = create_icon2(pTmp_Surf, pWindow->dst,
                           (WF_RESTORE_BACKGROUND|WF_FREE_THEME));

    set_wstate(pWidget, FC_WS_NORMAL);

    pWidget->action = nation_button_callback;

    w = MAX(w, pWidget->size.w);
    h = MAX(h, pWidget->size.h);

    add_to_gui_list(MAX_ID - nation_index(pNation), pWidget);

    if (nation_index(pNation) > (TARGETS_ROW * TARGETS_COL - 1)) {
      set_wflag(pWidget, WF_HIDDEN);
    }

  } nations_iterate_end;

  FREESURFACE(pMain_Bg);

  pNationDlg->pEndActiveWidgetList = pWindow->prev;
  pNationDlg->pBeginWidgetList = pWidget;
  pNationDlg->pBeginActiveWidgetList = pNationDlg->pBeginWidgetList;

  if (get_playable_nation_count() > TARGETS_ROW * TARGETS_COL) {
    pNationDlg->pActiveWidgetList = pNationDlg->pEndActiveWidgetList;
    create_vertical_scrollbar(pNationDlg,
                              TARGETS_COL, TARGETS_ROW, TRUE, TRUE);
  }

  /* ----------------------------------------------------------------- */

  /* nation set selection */
  if (nation_set_count() > 1) {
    utf8_str *natset_str;
    struct option *poption;

    natset_str = create_utf8_from_char(_("Nation set"), adj_font(12));
    change_ptsize_utf8(natset_str, adj_font(24));
    nationsets = create_iconlabel(NULL, pWindow->dst, natset_str, 0);
    add_to_gui_list(ID_LABEL, nationsets);

    /* create nation set name label */
    poption = optset_option_by_name(server_optset, "nationset");
    pSetup->set = nation_set_by_setting_value(option_str_get(poption));

    natset_str = create_utf8_from_char(nation_set_name_translation(pSetup->set),
                                       adj_font(12));
    change_ptsize_utf8(natset_str, adj_font(24));

    pWidget = create_iconlabel(NULL, pWindow->dst, natset_str, 0);

    add_to_gui_list(ID_LABEL, pWidget);
    pSetup->pset_name = pWidget;

    /* create next nationset button */
    pWidget = create_themeicon_button(current_theme->R_ARROW_Icon,
                                      pWindow->dst, NULL, 0);
    pWidget->action = next_set_callback;
    if (nation_set_index(pSetup->set) < nation_set_count() - 1) {
      set_wstate(pWidget, FC_WS_NORMAL);
    }
    add_to_gui_list(ID_NATION_NEXT_NATIONSET_BUTTON, pWidget);
    pWidget->size.h = pWidget->next->size.h;
    pSetup->pset_next = pWidget;

    /* create prev nationset button */
    pWidget = create_themeicon_button(current_theme->L_ARROW_Icon,
                                      pWindow->dst, NULL, 0);
    pWidget->action = prev_set_callback;
    if (nation_set_index(pSetup->set) > 0) {
      set_wstate(pWidget, FC_WS_NORMAL);
    }
    add_to_gui_list(ID_NATION_PREV_NATIONSET_BUTTON, pWidget);
    pWidget->size.h = pWidget->next->size.h;
    pSetup->pset_prev = pWidget;
  }

  /* nation name */
  pSetup->nation = fc_rand(get_playable_nation_count());
  pnat = nation_by_number(pSetup->nation);
  pSetup->nation_style = style_number(style_of_nation(pnat));

  copy_chars_to_utf8_str(pstr, nation_plural_translation(pnat));
  change_ptsize_utf8(pstr, adj_font(24));
  pstr->render = 2;
  pstr->fgcol = *get_theme_color(COLOR_THEME_NATIONDLG_TEXT);

  pTmp_Surf_zoomed = adj_surf(get_nation_flag_surface(pnat));

  pWidget = create_iconlabel(pTmp_Surf_zoomed, pWindow->dst, pstr,
                             (WF_ICON_ABOVE_TEXT|WF_ICON_CENTER|WF_FREE_GFX));
  if (nationsets == NULL) {
    pBuf = pWidget;
  } else {
    pBuf = nationsets;
  }

  add_to_gui_list(ID_LABEL, pWidget);

  /* create leader name edit */
  pWidget = create_edit_from_chars(NULL, pWindow->dst,
                                   NULL, adj_font(16), adj_size(200), 0);
  pWidget->size.h = adj_size(24);

  set_wstate(pWidget, FC_WS_NORMAL);
  pWidget->action = leader_name_edit_callback;
  add_to_gui_list(ID_NATION_WIZARD_LEADER_NAME_EDIT, pWidget);
  pSetup->pName_Edit = pWidget;

  /* create next leader name button */
  pWidget = create_themeicon_button(current_theme->R_ARROW_Icon,
                                    pWindow->dst, NULL, 0);
  pWidget->action = next_name_callback;
  add_to_gui_list(ID_NATION_WIZARD_NEXT_LEADER_NAME_BUTTON, pWidget);
  pWidget->size.h = pWidget->next->size.h;
  pSetup->pName_Next = pWidget;

  /* create prev leader name button */
  pWidget = create_themeicon_button(current_theme->L_ARROW_Icon,
                                    pWindow->dst, NULL, 0);
  pWidget->action = prev_name_callback;
  add_to_gui_list(ID_NATION_WIZARD_PREV_LEADER_NAME_BUTTON, pWidget);
  pWidget->size.h = pWidget->next->size.h;
  pSetup->pName_Prev = pWidget;

  /* change sex button */
  pWidget = create_icon_button_from_chars(NULL, pWindow->dst, _("Male"), adj_font(14), 0);
  pWidget->action = change_sex_callback;
  pWidget->size.w = adj_size(100);
  pWidget->size.h = adj_size(22);
  set_wstate(pWidget, FC_WS_NORMAL);
  pSetup->pChange_Sex = pWidget;

  /* add to main widget list */
  add_to_gui_list(ID_NATION_WIZARD_CHANGE_SEX_BUTTON, pWidget);

  /* ---------------------------------------------------------- */
  i = 0;
  zoom = DEFAULT_ZOOM * 1.0;

  len = 0;
  styles_iterate(pstyle) {
    i = basic_city_style_for_style(pstyle);

    pTmp_Surf = get_sample_city_surface(i);

    if (pTmp_Surf->w > 48) {
      zoom = DEFAULT_ZOOM * (48.0 / pTmp_Surf->w);
    }

    pTmp_Surf_zoomed = zoomSurface(get_sample_city_surface(i), zoom, zoom, 0);

    pWidget = create_icon2(pTmp_Surf_zoomed, pWindow->dst, WF_RESTORE_BACKGROUND);
    pWidget->action = style_callback;
    if (i != pSetup->nation_style) {
      set_wstate(pWidget, FC_WS_NORMAL);
    }
    len += pWidget->size.w;
    add_to_gui_list(MAX_ID - 1000 - i, pWidget);
  } styles_iterate_end;

  pLast_City_Style = pWidget;
  /* ---------------------------------------------------------- */

  /* create Cancel button */
  pWidget = create_themeicon_button_from_chars(current_theme->CANCEL_Icon,
                                               pWindow->dst, _("Cancel"),
                                               adj_font(12), 0);
  pWidget->action = races_dialog_cancel_callback;
  set_wstate(pWidget, FC_WS_NORMAL);

  add_to_gui_list(ID_NATION_WIZARD_DISCONNECT_BUTTON, pWidget);

  /* create OK button */
  pWidget =
    create_themeicon_button_from_chars(current_theme->OK_Icon, pWindow->dst,
                                       _("OK"), adj_font(12), 0);
  pWidget->action = races_dialog_ok_callback;

  set_wstate(pWidget, FC_WS_NORMAL);
  add_to_gui_list(ID_NATION_WIZARD_START_BUTTON, pWidget);
  pWidget->size.w = MAX(pWidget->size.w, pWidget->next->size.w);
  pWidget->next->size.w = pWidget->size.w;

  pNationDlg->pBeginWidgetList = pWidget;
  /* ---------------------------------------------------------- */

  pMain_Bg = theme_get_background(theme, BACKGROUND_NATIONDLG);
  if (resize_window(pWindow, pMain_Bg, NULL, adj_size(640), adj_size(480))) {
    FREESURFACE(pMain_Bg);
  }

  area = pWindow->area;

  widget_set_position(pWindow,
                      (main_window_width() - pWindow->size.w) / 2,
                      (main_window_height() - pWindow->size.h) / 2);

  /* nations */

  h = pNationDlg->pEndActiveWidgetList->size.h * TARGETS_ROW;
  i = (area.h - adj_size(43) - h) / 2;
  setup_vertical_widgets_position(TARGETS_COL,
                                  area.x + adj_size(10),
                                  area.y + i - adj_size(4),
                                  0, 0, pNationDlg->pBeginActiveWidgetList,
                                  pNationDlg->pEndActiveWidgetList);

  if (pNationDlg->pScroll) {
    SDL_Rect area2;

    w = pNationDlg->pEndActiveWidgetList->size.w * TARGETS_COL;
    setup_vertical_scrollbar_area(pNationDlg->pScroll,
                                  area.x + w + adj_size(12),
                                  area.y + i - adj_size(4), h, FALSE);

    area2.x = area.x + w + adj_size(11);
    area2.y = area.y + i - adj_size(4);
    area2.w = pNationDlg->pScroll->pUp_Left_Button->size.w + adj_size(2);
    area2.h = h;
    fill_rect_alpha(pWindow->theme, &area2, &bg_color);

    create_frame(pWindow->theme,
                 area2.x, area2.y - 1, area2.w, area2.h + 1,
                 get_theme_color(COLOR_THEME_NATIONDLG_FRAME));
  }

  if (nationsets != NULL) {
    /* Nationsets header */
    pBuf->size.x = area.x + area.w / 2 + (area.w / 2 - pBuf->size.w) / 2;
    pBuf->size.y = area.y + adj_size(46);

    natinfo_y = pBuf->size.y;
    natinfo_h = area.h -= pBuf->size.y;

    /* Nationset name */
    pBuf = pBuf->prev;
    pBuf->size.x = area.x + area.w / 2 + (area.w / 2 - pBuf->size.w) / 2;
    pBuf->size.y = natinfo_y + adj_size(46);

    natinfo_y += adj_size(46);
    natinfo_h -= adj_size(46);

    /* Next Nationset Button */
    pBuf = pBuf->prev;
    pBuf->size.x = pBuf->next->size.x + pBuf->next->size.w;
    pBuf->size.y = pBuf->next->size.y;

    /* Prev Nationset Button */
    pBuf = pBuf->prev;
    pBuf->size.x = pBuf->next->next->size.x - pBuf->size.w;
    pBuf->size.y = pBuf->next->size.y;

    pBuf = pBuf->prev;
  } else {
    natinfo_y = area.y;
    natinfo_h = area.h;
  }

  /* Selected Nation Name */
  pBuf->size.x = area.x + area.w / 2 + (area.w / 2 - pBuf->size.w) / 2;
  pBuf->size.y = natinfo_y + adj_size(46);

  /* Leader Name Edit */
  pBuf = pBuf->prev;
  pBuf->size.x = area.x + area.w / 2 + (area.w/2 - pBuf->size.w) / 2;
  pBuf->size.y = natinfo_y + (natinfo_h - pBuf->size.h) / 2 - adj_size(30);

  /* Next Leader Name Button */
  pBuf = pBuf->prev;
  pBuf->size.x = pBuf->next->size.x + pBuf->next->size.w;
  pBuf->size.y = pBuf->next->size.y;

  /* Prev Leader Name Button */
  pBuf = pBuf->prev;
  pBuf->size.x = pBuf->next->next->size.x - pBuf->size.w;
  pBuf->size.y = pBuf->next->size.y;

  /* Change Leader Sex Button */
  pBuf = pBuf->prev;
  pBuf->size.x = area.x + area.w / 2 + (area.w/2 - pBuf->size.w) / 2;
  pBuf->size.y = pBuf->next->size.y + pBuf->next->size.h + adj_size(20);

  /* First Style Button */
  pBuf = pBuf->prev;
  pBuf->size.x = area.x + area.w / 2 + (area.w/2 - len) / 2 - adj_size(20);
  pBuf->size.y = pBuf->next->size.y + pBuf->next->size.h + adj_size(20);

  /* Rest Style Buttons */
  while (pBuf != pLast_City_Style) {
    pBuf = pBuf->prev;
    pBuf->size.x = pBuf->next->size.x + pBuf->next->size.w + adj_size(3);
    pBuf->size.y = pBuf->next->size.y;
  }

  create_line(pWindow->theme,
              area.x,
              natinfo_y + natinfo_h - adj_size(7) - pBuf->prev->size.h - adj_size(10),
              area.w - 1, 
              natinfo_y + natinfo_h - adj_size(7) - pBuf->prev->size.h - adj_size(10),
              get_theme_color(COLOR_THEME_NATIONDLG_FRAME));

  /* Disconnect Button */
  pBuf = pBuf->prev;
  pBuf->size.x = area.x + adj_size(10);
  pBuf->size.y = natinfo_y + natinfo_h - adj_size(7) - pBuf->size.h;

  /* Start Button */
  pBuf = pBuf->prev;
  pBuf->size.x = area.w - adj_size(10) - pBuf->size.w;
  pBuf->size.y = pBuf->next->size.y;

  /* -------------------------------------------------------------------- */

  select_random_leader(pSetup->nation);

  redraw_group(pNationDlg->pBeginWidgetList, pWindow, 0);

  widget_flush(pWindow);
}

/**********************************************************************//**
  Close the nation selection dialog.  This should allow the user to
  (at least) select a unit to activate.
**************************************************************************/
void popdown_races_dialog(void)
{
  if (pNationDlg) {
    popdown_window_group_dialog(pNationDlg->pBeginWidgetList,
                                pNationDlg->pEndWidgetList);

    cancel_help_dlg_callback(NULL);

    FC_FREE(pLeaderName);

    FC_FREE(pNationDlg->pScroll);
    FC_FREE(pNationDlg);
  }
}

/**********************************************************************//**
  The server has changed the set of selectable nations.
**************************************************************************/
void races_update_pickable(bool nationset_change)
{
  /* If this is because of nationset change, update will take
   * place later when the new option value is received */
  if (pNationDlg != NULL && !nationset_change) {
    popdown_races_dialog();
    popup_races_dialog(client.conn.playing);
  }
}

/**********************************************************************//**
  Nationset selection update
**************************************************************************/
void nationset_changed(void)
{
  if (pNationDlg != NULL) {
    popdown_races_dialog();
    popup_races_dialog(client.conn.playing);
  }
}

/**********************************************************************//**
  In the nation selection dialog, make already-taken nations unavailable.
  This information is contained in the packet_nations_used packet.
**************************************************************************/
void races_toggles_set_sensitive()
{
  struct NAT *pSetup;
  bool change = FALSE;
  struct widget *pNat;

  if (!pNationDlg) {
    return;
  }

  pSetup = (struct NAT *)(pNationDlg->pEndWidgetList->data.ptr);

  nations_iterate(nation) {
    if (!is_nation_pickable(nation) || nation->player) {
      log_debug("  [%d]: %d = %s", nation_index(nation),
                (!is_nation_pickable(nation) || nation->player),
                nation_rule_name(nation));

      pNat = get_widget_pointer_form_main_list(MAX_ID - nation_index(nation));
      set_wstate(pNat, FC_WS_DISABLED);

      if (nation_index(nation) == pSetup->nation) {
        change = TRUE;
      }
    }
  } nations_iterate_end;

  if (change) {
    do {
      pSetup->nation = fc_rand(get_playable_nation_count());
      pNat = get_widget_pointer_form_main_list(MAX_ID - pSetup->nation);
    } while (get_wstate(pNat) == FC_WS_DISABLED);

    if (get_wstate(pSetup->pName_Edit) == FC_WS_PRESSED) {
      force_exit_from_event_loop();
      set_wstate(pSetup->pName_Edit, FC_WS_NORMAL);
    }
    change_nation_label();
    enable(MAX_ID - 1000 - pSetup->nation_style);
    pSetup->nation_style = style_number(style_of_nation(nation_by_number(pSetup->nation)));
    disable(MAX_ID - 1000 - pSetup->nation_style);
    select_random_leader(pSetup->nation);
  }
  redraw_group(pNationDlg->pBeginWidgetList, pNationDlg->pEndWidgetList, 0);
  widget_flush(pNationDlg->pEndWidgetList);
}

/**********************************************************************//**
  Ruleset (modpack) has suggested loading certain tileset. Confirm from
  user and load.
**************************************************************************/
void popup_tileset_suggestion_dialog(void)
{
}

/**********************************************************************//**
  Ruleset (modpack) has suggested loading certain soundset. Confirm from
  user and load.
**************************************************************************/
void popup_soundset_suggestion_dialog(void)
{
}

/**********************************************************************//**
  Ruleset (modpack) has suggested loading certain musicset. Confirm from
  user and load.
**************************************************************************/
void popup_musicset_suggestion_dialog(void)
{
}

/**********************************************************************//**
  Tileset (modpack) has suggested loading certain theme. Confirm from
  user and load.
**************************************************************************/
bool popup_theme_suggestion_dialog(const char *theme_name)
{
  /* Don't load */
  return FALSE;
}

/**********************************************************************//**
  Player has gained a new tech.
**************************************************************************/
void show_tech_gained_dialog(Tech_type_id tech)
{
  /* PORTME */
}

/**********************************************************************//**
  Show tileset error dialog.
**************************************************************************/
void show_tileset_error(const char *msg)
{
  /* PORTME */
}

/**********************************************************************//**
  Give a warning when user is about to edit scenario with manually
  set properties.
**************************************************************************/
bool handmade_scenario_warning(void)
{
  /* Just tell the client common code to handle this. */
  return FALSE;
}

/**********************************************************************//**
  Update multipliers (policies) dialog.
**************************************************************************/
void real_multipliers_dialog_update(void *unused)
{
  /* PORTME */
} 

/**********************************************************************//**
  Unit wants to get into some transport on given tile.
**************************************************************************/
bool request_transport(struct unit *pcargo, struct tile *ptile)
{
  return FALSE; /* Unit was not handled here. */
}

/**********************************************************************//**
  Popup detailed information about battle or save information for
  some kind of statistics
**************************************************************************/
void popup_combat_info(int attacker_unit_id, int defender_unit_id,
                       int attacker_hp, int defender_hp,
                       bool make_att_veteran, bool make_def_veteran)
{
}

/**********************************************************************//**
  Popup dialog showing given image and text,
  start playing given sound, stop playing sound when popup is closed.
  Take all space available to show image if fullsize is set.
  If there are other the same popups show them in queue.
***************************************************************************/
void show_img_play_snd(const char *img_path, const char *snd_path,
                       const char *desc, bool fullsize)
{
  /* PORTME */
}
