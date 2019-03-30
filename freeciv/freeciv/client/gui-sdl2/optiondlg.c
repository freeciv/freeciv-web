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
                          optiondlg.c  -  description
                             -------------------
    begin                : Sun Aug 11 2002
    copyright            : (C) 2002 by Rafał Bursig
    email                : Rafał Bursig <bursig@poczta.fm>
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <stdarg.h>
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
#include "string_vector.h"

/* common */
#include "fc_types.h"
#include "game.h"

/* client */
#include "client_main.h"
#include "climisc.h"
#include "clinet.h"
#include "connectdlg_common.h"
#include "global_worklist.h"

/* gui-sdl2 */
#include "colors.h"
#include "connectdlg.h"
#include "dialogs.h"
#include "graphics.h"
#include "gui_iconv.h"
#include "gui_id.h"
#include "gui_main.h"
#include "gui_tilespec.h"
#include "helpdlg.h"
#include "mapctrl.h"
#include "mapview.h"
#include "menu.h"
#include "messagewin.h"
#include "pages.h"
#include "themespec.h"
#include "widget.h"
#include "wldlg.h"

#include "optiondlg.h"

enum option_dialog_mode {
  ODM_MAIN,
  ODM_OPTSET,
  ODM_WORKLIST
};

struct option_dialog_optset {
  const struct option_set *poptset;
  struct widget *widget_list;
  int category;
};

struct option_dialog_worklist {
  struct widget *edited_name;
};

struct option_dialog {
  struct widget *end_widget_list;
  struct widget *core_widget_list;
  struct widget *main_widget_list;
  struct widget *begin_widget_list;
  struct ADVANCED_DLG *advanced;
  enum option_dialog_mode mode;
  union {
    struct option_dialog_optset optset;
    struct option_dialog_worklist worklist;
  };
};


static struct option_dialog *option_dialog = NULL;
struct widget *pOptions_Button = NULL;
static bool restore_meswin_dialog = FALSE;


static struct widget *option_widget_new(struct option *poption,
                                        struct widget *window,
                                        bool hide);
static void option_widget_update(struct option *poption);
static void option_widget_apply(struct option *poption);

static struct option_dialog *option_dialog_new(void);
static void option_dialog_destroy(struct option_dialog *pdialog);

static void option_dialog_optset(struct option_dialog *pdialog,
                                 const struct option_set *poptset);
static void option_dialog_optset_category(struct option_dialog *pdialog,
                                          int category);

static void option_dialog_worklist(struct option_dialog *pdialog);

/************************************************************************//**
  Arrange the widgets. NB: end argument is excluded. End the argument
  list with the icons on the top, terminated by NULL.
****************************************************************************/
static void arrange_widgets(struct widget *window, int widgets_per_row,
                            int rows_shown, struct widget *begin,
                            struct widget *end, ...)
{
  struct widget *widget;
  SDL_Surface *logo;
  SDL_Rect area;
  int longest[widgets_per_row], xpos[widgets_per_row];
  int w, h, i, j;
  va_list args;

  fc_assert_ret(NULL != window);
  fc_assert_ret(NULL != begin);
  fc_assert_ret(NULL != end);
  fc_assert_ret(0 < widgets_per_row);

  /* Get window dimensions. */
  memset(longest, 0, sizeof(longest));
  for (widget = begin, i = 0; widget != end; widget = widget->next, i++) {
    j = i % widgets_per_row;
    longest[j] = MAX(longest[j], widget->size.w);
  }

  fc_assert(0 == (i % widgets_per_row));

  if (-1 == rows_shown) {
    h = 30 * (i / widgets_per_row);
  } else {
    h = 30 * MIN((i / widgets_per_row), rows_shown);
  }

  w = (1 - widgets_per_row) * adj_size(20);
  for (j = 0; j < widgets_per_row; j++) {
    w += longest[j];
  }
  if (-1 != rows_shown) {
    w += adj_size(20);
  }

  /* Clear former area. */
  area = window->area;
  area.w += window->size.x;
  area.h += window->size.y;
  dirty_sdl_rect(&area);

  /* Resize window. */
  logo = theme_get_background(theme, BACKGROUND_OPTIONDLG);
  if (resize_window(window, logo, NULL,
                    adj_size(w + 80), adj_size(h + 80))) {
    FREESURFACE(logo);
  }

  /* Set window position. */
  widget_set_position(window, (main_window_width() - window->size.w) / 2,
                      (main_window_height() - window->size.h) / 2);

  area = window->area;

  /* Set icons position. */
  va_start(args, end);
  w = 0;
  while ((widget = va_arg(args, struct widget *))) {
    w += widget->size.w;
    widget_set_position(widget, area.x + area.w - w - 1,
                        window->size.y + adj_size(2));
  }
  va_end(args);

  if (1 < widgets_per_row) {
    xpos[widgets_per_row - 1] = area.x + adj_size(20);
    for (j = widgets_per_row - 2; j >= 0; j--) {
      xpos[j] = xpos[j + 1] + adj_size(20) + longest[j + 1];
    }
  }

  /* Set button position. */
  h = 30 * (i / widgets_per_row + 1);
  for (widget = begin, i = 0; widget != end; widget = widget->next, i++) {
    j = i % widgets_per_row;
    if (0 == j) {
      h -= 30;
    }
    widget_resize(widget, longest[j], widget->size.h);
    if (1 == widgets_per_row) {
      widget_set_position(widget, area.x + (area.w - widget->size.w) / 2,
                          area.y + adj_size(h));
    } else {
      widget_set_position(widget, xpos[j], area.y + adj_size(h));
    }
  }

  redraw_group(begin, window, 0);
  widget_mark_dirty(window);
  flush_all();
}

/************************************************************************//**
  User interacted with the option dialog window.
****************************************************************************/
static int main_optiondlg_callback(struct widget *pWindow)
{
  if (NULL != option_dialog && Main.event.button.button == SDL_BUTTON_LEFT) {
    move_window_group(option_dialog->begin_widget_list,
                      option_dialog->end_widget_list);
  }

  return -1;
}

/************************************************************************//**
  Back requested.
****************************************************************************/
static int back_callback(struct widget *pWidget)
{
  if (NULL == option_dialog || Main.event.button.button != SDL_BUTTON_LEFT) {
    return -1;
  }

  if (ODM_MAIN == option_dialog->mode) {
    if (client.conn.established) {
      /* Back to game. */
      popdown_optiondlg(FALSE);
      enable_options_button();
      widget_redraw(pOptions_Button);
      widget_mark_dirty(pOptions_Button);
      flush_dirty();
    } else {
      /* Back to main page. */
      popdown_optiondlg(TRUE);
      set_client_page(PAGE_MAIN);
    }
    return -1;
  }

  if (ODM_OPTSET == option_dialog->mode
      && -1 != option_dialog->optset.category) {
    /* Back to option set category menu. */
    options_iterate(option_dialog->optset.poptset, poption) {
      if (option_dialog->optset.category == option_category(poption)) {
        option_set_gui_data(poption, NULL);
      }
    } options_iterate_end;
    option_dialog->optset.category = -1;
    FC_FREE(option_dialog->advanced->pScroll);
    FC_FREE(option_dialog->advanced);

    del_group_of_widgets_from_gui_list(option_dialog->begin_widget_list,
                                       option_dialog->optset.widget_list->prev);

    option_dialog->begin_widget_list = option_dialog->optset.widget_list;

    show_group(option_dialog->begin_widget_list,
               option_dialog->main_widget_list->prev);

    arrange_widgets(option_dialog->end_widget_list, 1, -1,
                    option_dialog->begin_widget_list,
                    option_dialog->main_widget_list,
                    option_dialog->core_widget_list, NULL);
    return -1;
  }

  if (ODM_WORKLIST == option_dialog->mode
      && NULL != option_dialog->advanced) {
    FC_FREE(option_dialog->advanced->pScroll);
    FC_FREE(option_dialog->advanced);
    option_dialog->worklist.edited_name = NULL;
  }

  /* Back to main options menu. */
  del_group_of_widgets_from_gui_list(option_dialog->begin_widget_list,
                                     option_dialog->main_widget_list->prev);

  option_dialog->begin_widget_list = option_dialog->main_widget_list;

  show_group(option_dialog->begin_widget_list,
             option_dialog->core_widget_list->prev);
  option_dialog->mode = ODM_MAIN;
  arrange_widgets(option_dialog->end_widget_list, 1, -1,
                  option_dialog->begin_widget_list,
                  option_dialog->core_widget_list,
                  option_dialog->core_widget_list, NULL);

  return -1;
}

/************************************************************************//**
  Create the client options dialog.
****************************************************************************/
static int client_options_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    option_dialog_popup(_("Local Options"), client_optset);
  }

  return -1;
}

/************************************************************************//**
  Create the server options dialog.
****************************************************************************/
static int server_options_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    option_dialog_popup(_("Server options"), server_optset);
  }

  return -1;
}

/************************************************************************//**
  Create the worklist editor.
****************************************************************************/
static int work_lists_callback(struct widget *widget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    option_dialog_worklist(option_dialog);
  }

  return -1;
}

/************************************************************************//**
  Option set category selected.
****************************************************************************/
static int save_client_options_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    options_save(NULL);
  }

  return -1;
}

/************************************************************************//**
  Save game callback.
****************************************************************************/
static int save_game_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    send_save_game(NULL);
    back_callback(NULL);
  }

  return -1;
}

/************************************************************************//**
  Open Help Browser callback
****************************************************************************/
static int help_browser_callback(struct widget *pwidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    popup_help_browser();
  }

  return -1;
}

/************************************************************************//**
  Client disconnect from server callback.
****************************************************************************/
static int disconnect_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    popdown_optiondlg(TRUE);
    enable_options_button();
    disconnect_from_server();
  }

  return -1;
}

/************************************************************************//**
  Exit callback.
****************************************************************************/
static int exit_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    popdown_optiondlg(TRUE);
    force_exit_from_event_loop();
  }

  return 0;
}

/************************************************************************//**
  Option set category selected.
****************************************************************************/
static int option_category_callback(struct widget *widget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    option_dialog_optset_category(option_dialog, MAX_ID - widget->ID);
  }

  return -1;
}

/************************************************************************//**
  Apply the changes for the option category.
****************************************************************************/
static int apply_callback(struct widget *widget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT
      && NULL != option_dialog
      && ODM_OPTSET == option_dialog->mode
      && -1 != option_dialog->optset.category) {
    options_iterate(option_dialog->optset.poptset, poption) {
      if (option_dialog->optset.category == option_category(poption)) {
        option_widget_apply(poption);
      }
    } options_iterate_end;
  }

  return back_callback(widget);
}

/************************************************************************//**
  Dummy callback. Disable exit().
****************************************************************************/
static int none_callback(struct widget *widget)
{
  return -1;
}

/************************************************************************//**
  Return a string vector containing all video modes.
****************************************************************************/
static struct strvec *video_mode_list(void)
{
  struct strvec *video_modes = strvec_new();
  int active_display = 0; /* TODO: Support multiple displays */
  int mode_count;
  int i;

  mode_count = SDL_GetNumDisplayModes(active_display);
  for (i = 0; i < mode_count; i++) {
    SDL_DisplayMode mode;

    if (!SDL_GetDisplayMode(active_display, i, &mode)) {
      char buf[64];
      struct video_mode vmod = { .width = mode.w, .height = mode.h };

      if (video_mode_to_string(buf, sizeof(buf), &vmod)) {
        strvec_append(video_modes, buf);
      }
    }
  }

  return video_modes;
}

/************************************************************************//**
  Free correctly the memory assigned to the enum_widget.
****************************************************************************/
static void enum_widget_destroy(struct widget *widget)
{
  strvec_destroy((struct strvec *) widget->data.vector);
}

/************************************************************************//**
  Free correctly the memory assigned to the video_mode_widget.
****************************************************************************/
static void video_mode_widget_destroy(struct widget *widget)
{
  combo_popdown(widget);
  strvec_destroy((struct strvec *) widget->data.vector);
}

/************************************************************************//**
  Create a widget for the option.
****************************************************************************/
static struct widget *option_widget_new(struct option *poption,
                                        struct widget *window,
                                        bool hide)
{
  struct widget *widget;
  char *help_text;
  Uint32 flags = (hide ? WF_HIDDEN | WF_RESTORE_BACKGROUND
                  : WF_RESTORE_BACKGROUND);

  fc_assert_ret_val(NULL != poption, NULL);
  fc_assert_ret_val(NULL != window, NULL);

  help_text = fc_strdup(option_help_text(poption));
  fc_break_lines(help_text, 50);

  widget = create_iconlabel_from_chars(NULL, window->dst,
                                       option_description(poption),
                                       adj_font(12),
                                       flags | WF_WIDGET_HAS_INFO_LABEL);
  widget->string_utf8->style |= TTF_STYLE_BOLD;
  widget->info_label = create_utf8_from_char(help_text, adj_font(12));
  widget->action = none_callback;
  set_wstate(widget, FC_WS_NORMAL);
  remake_label_size(widget);
  add_to_gui_list(MAX_ID - option_number(poption), widget);

  widget = NULL;
  switch (option_type(poption)) {
  case OT_BOOLEAN:
    widget = create_checkbox(window->dst, option_bool_get(poption),
                             flags | WF_WIDGET_HAS_INFO_LABEL);
    break;

  case OT_INTEGER:
    {
      char buf[64];

      fc_snprintf(buf, sizeof(buf), "%d", option_int_get(poption));
      widget = create_edit_from_chars(NULL, window->dst, buf, adj_font(12),
                                      adj_size(25),
                                      flags | WF_WIDGET_HAS_INFO_LABEL);
    }
    break;

  case OT_STRING:
    {
      const struct strvec *values = option_str_values(poption);

      if (NULL != values) {
        widget = combo_new_from_chars(NULL, window->dst, adj_font(12),
                                      option_str_get(poption), values,
                                      adj_size(25),
                                      flags | WF_WIDGET_HAS_INFO_LABEL);
      } else {
        widget = create_edit_from_chars(NULL, window->dst,
                                        option_str_get(poption),
                                        adj_font(12), adj_size(25),
                                        flags | WF_WIDGET_HAS_INFO_LABEL);
      }
    }
    break;

  case OT_ENUM:
    {
      const struct strvec *values = option_enum_values(poption);
      struct strvec *translated_values = strvec_new();
      int i;

      strvec_reserve(translated_values, strvec_size(values));
      for (i = 0; i < strvec_size(values); i++) {
        strvec_set(translated_values, i, _(strvec_get(values, i)));
      }

      widget = combo_new_from_chars(NULL, window->dst, adj_font(12),
                                    _(option_enum_get_str(poption)),
                                    translated_values, adj_size(25),
                                    flags | WF_WIDGET_HAS_INFO_LABEL);
      widget->destroy = enum_widget_destroy;
    }
    break;

  case OT_VIDEO_MODE:
    {
      char buf[64];
      struct video_mode vmod;

      vmod = option_video_mode_get(poption);
      if (!video_mode_to_string(buf, sizeof(buf), &vmod)) {
        /* Always fails. */
        fc_assert(video_mode_to_string(buf, sizeof(buf), &vmod));
      }

      widget = combo_new_from_chars(NULL, window->dst, adj_font(12),
                                    buf, video_mode_list(), adj_size(25),
                                    flags | WF_WIDGET_HAS_INFO_LABEL);
      widget->destroy = video_mode_widget_destroy;
    }
    break;

  case OT_FONT:
    {
      widget = create_edit_from_chars(NULL, window->dst,
                                      option_font_get(poption),
                                      adj_font(12), adj_size(25),
                                      flags | WF_WIDGET_HAS_INFO_LABEL);
    }
    break;

  case OT_BITWISE:
  case OT_COLOR:
    log_error("Option type %s (%d) not supported yet.",
              option_type_name(option_type(poption)),
              option_type(poption));
    break;
  }

  if (NULL == widget) {
    /* Not implemented. */
    widget = create_iconlabel_from_chars(NULL, window->dst, "",
                                         adj_font(12), flags);
  } else {
    widget->info_label = create_utf8_from_char(help_text, adj_font(12));
    widget->action = none_callback;
    if (option_is_changeable(poption)) {
      set_wstate(widget, FC_WS_NORMAL);
    }
  }

  add_to_gui_list(MAX_ID - option_number(poption), widget);
  option_set_gui_data(poption, widget);

  free(help_text);

  return widget;
}

/************************************************************************//**
  Update the widget of the option.
****************************************************************************/
static void option_widget_update(struct option *poption)
{
  struct widget *widget;

  fc_assert_ret(NULL != poption);
  widget = (struct widget *) option_get_gui_data(poption);
  fc_assert_ret(NULL != widget);

  set_wstate(widget,
             option_is_changeable(poption)
             ? FC_WS_NORMAL : FC_WS_DISABLED);

  switch (option_type(poption)) {
  case OT_BOOLEAN:
    if (option_bool_get(poption) != get_checkbox_state(widget)) {
      toggle_checkbox(widget);
    }
    break;

  case OT_INTEGER:
    {
      char buf[64];

      fc_snprintf(buf, sizeof(buf), "%d", option_int_get(poption));
      copy_chars_to_utf8_str(widget->string_utf8, buf);
    }
    break;

  case OT_STRING:
    copy_chars_to_utf8_str(widget->string_utf8, option_str_get(poption));
    break;

  case OT_ENUM:
    copy_chars_to_utf8_str(widget->string_utf8,
                           _(option_enum_get_str(poption)));
    break;

  case OT_VIDEO_MODE:
    {
      char buf[64];
      struct video_mode vmod;

      vmod = option_video_mode_get(poption);
      if (video_mode_to_string(buf, sizeof(buf), &vmod)) {
        copy_chars_to_utf8_str(widget->string_utf8, buf);
      } else {
        /* Always fails. */
        fc_assert(video_mode_to_string(buf, sizeof(buf), &vmod));
      }
    }
    break;

  case OT_FONT:
    copy_chars_to_utf8_str(widget->string_utf8, option_font_get(poption));
    break;

  case OT_BITWISE:
  case OT_COLOR:
    log_error("Option type %s (%d) not supported yet.",
              option_type_name(option_type(poption)),
              option_type(poption));
    break;
  }

  widget_redraw(widget);
  widget_mark_dirty(widget);
}

/************************************************************************//**
  Apply the changes for the option.
****************************************************************************/
static void option_widget_apply(struct option *poption)
{
  struct widget *widget;

  fc_assert_ret(NULL != poption);
  widget = (struct widget *) option_get_gui_data(poption);
  fc_assert_ret(NULL != widget);

  switch (option_type(poption)) {
  case OT_BOOLEAN:
    (void) option_bool_set(poption, get_checkbox_state(widget));
    break;

  case OT_INTEGER:
    {
      int value;

      if (str_to_int(widget->string_utf8->text, &value)) {
        (void) option_int_set(poption, value);
      }
    }
    break;

  case OT_STRING:
    (void) option_str_set(poption, widget->string_utf8->text);
    break;

  case OT_ENUM:
    {
      int i;

      /* 'str' is translated, so we cannot use directly
       * option_enum_set_str(). */
      for (i = 0; i < strvec_size(widget->data.vector); i++) {
        if (!strcmp(strvec_get(widget->data.vector, i), widget->string_utf8->text)) {
          (void) option_enum_set_int(poption, i);
          break;
        }
      }
    }
    break;

  case OT_VIDEO_MODE:
    {
      struct video_mode mode;

      if (string_to_video_mode(widget->string_utf8->text, &mode)) {
        option_video_mode_set(poption, mode);
      } else {
        /* Always fails. */
        fc_assert(string_to_video_mode(widget->string_utf8->text, &mode));
      }
    }
    break;

  case OT_FONT:
    (void) option_font_set(poption, widget->string_utf8->text);
    break;

  case OT_BITWISE:
  case OT_COLOR:
    log_error("Option type %s (%d) not supported yet.",
              option_type_name(option_type(poption)),
              option_type(poption));
    break;
  }
}

/************************************************************************//**
  Return a new option dialog.
****************************************************************************/
static struct option_dialog *option_dialog_new(void)
{
  struct option_dialog *pdialog = fc_calloc(1, sizeof(*pdialog));
  struct widget *window, *close_button, *widget;
  utf8_str *str;

  pdialog->mode = ODM_MAIN;

  /* Create window widget. */
  str = create_utf8_from_char(_("Options"), adj_font(12));
  str->style |= TTF_STYLE_BOLD;

  window = create_window_skeleton(NULL, str, 0);
  window->action = main_optiondlg_callback;

  set_wstate(window, FC_WS_NORMAL);
  add_to_gui_list(ID_OPTIONS_WINDOW, window);
  pdialog->end_widget_list = window;

  /* Create close button widget. */
  close_button = create_themeicon(current_theme->Small_CANCEL_Icon, window->dst,
                                  WF_WIDGET_HAS_INFO_LABEL
                                  | WF_RESTORE_BACKGROUND);
  close_button->info_label = create_utf8_from_char(_("Close Dialog (Esc)"),
                                                   adj_font(12));
  close_button->action = back_callback;
  set_wstate(close_button, FC_WS_NORMAL);
  close_button->key = SDLK_ESCAPE;
  add_to_gui_list(ID_OPTIONS_BACK_BUTTON, close_button);
  pdialog->core_widget_list = close_button;

  /* Create client options button widget. */
  widget = create_icon_button_from_chars(NULL, window->dst,
                                         _("Local options"),
                                         adj_font(12), 0);
  widget->action = client_options_callback;
  set_wstate(widget, FC_WS_NORMAL);
  widget_resize(widget, widget->size.w, widget->size.h + adj_size(4));
  add_to_gui_list(ID_OPTIONS_CLIENT_BUTTON, widget);

  /* Create server options button widget. */
  widget = create_icon_button_from_chars(NULL, window->dst,
                                         _("Server options"),
                                         adj_font(12), 0);
  widget->action = server_options_callback;
  if (client.conn.established) {
    set_wstate(widget, FC_WS_NORMAL);
  }
  widget_resize(widget, widget->size.w, widget->size.h + adj_size(4));
  add_to_gui_list(ID_OPTIONS_SERVER_BUTTON, widget);

  /* Create global worklists button widget. */
  widget = create_icon_button_from_chars(NULL, window->dst,
                                         _("Worklists"), adj_font(12), 0);
  widget->action = work_lists_callback;
  if (C_S_RUNNING == client_state()) {
    set_wstate(widget, FC_WS_NORMAL);
  }
  widget_resize(widget, widget->size.w, widget->size.h + adj_size(4));
  add_to_gui_list(ID_OPTIONS_WORKLIST_BUTTON, widget);

  /* Create save game button widget. */
  widget = create_icon_button_from_chars(NULL, window->dst,
                                         _("Save Local Options"),
                                         adj_font(12), 0);
  widget->action = save_client_options_callback;
  set_wstate(widget, FC_WS_NORMAL);
  widget_resize(widget, widget->size.w, widget->size.h + adj_size(4));
  add_to_gui_list(ID_OPTIONS_SAVE_BUTTON, widget);

  /* Create save game button widget. */
  widget = create_icon_button_from_chars(NULL, window->dst,
                                         _("Save Game"), adj_font(12), 0);
  widget->action = save_game_callback;
  if (C_S_RUNNING == client_state()) {
    set_wstate(widget, FC_WS_NORMAL);
  }
  widget_resize(widget, widget->size.w, widget->size.h + adj_size(4));
  add_to_gui_list(ID_OPTIONS_SAVE_GAME_BUTTON, widget);

  /* Create help browser button widget. */
  widget = create_icon_button_from_chars(NULL, window->dst,
                                         _("Help Browser"), adj_font(12), 0);
  widget->action = help_browser_callback;
  widget->key = SDLK_h;
  if (client.conn.established) {
    set_wstate(widget, FC_WS_NORMAL);
  }
  widget_resize(widget, widget->size.w, widget->size.h + adj_size(4));
  add_to_gui_list(ID_OPTIONS_HELP_BROWSER_BUTTON, widget);

  /* Create leave game button widget. */
  widget = create_icon_button_from_chars(NULL, window->dst,
                                         _("Leave Game"), adj_font(12), 0);
  widget->action = disconnect_callback;
  widget->key = SDLK_q;
  if (client.conn.established) {
    set_wstate(widget, FC_WS_NORMAL);
  }
  widget_resize(widget, widget->size.w, widget->size.h + adj_size(4));
  add_to_gui_list(ID_OPTIONS_DISC_BUTTON, widget);

  /* Create quit widget button. */
  widget = create_icon_button_from_chars(NULL, window->dst,
                                         _("Quit"), adj_font(12), 0);
  widget->action = exit_callback;
  widget->key = SDLK_q;
  set_wstate(widget, FC_WS_NORMAL);
  widget_resize(widget, widget->size.w, widget->size.h + adj_size(4));
  add_to_gui_list(ID_OPTIONS_EXIT_BUTTON, widget);

  pdialog->begin_widget_list = widget;
  pdialog->main_widget_list = widget;

  arrange_widgets(window, 1, -1, widget, pdialog->core_widget_list,
                  pdialog->core_widget_list, NULL);

  return pdialog;
}

/************************************************************************//**
  Destroys an option dialog.
****************************************************************************/
static void option_dialog_destroy(struct option_dialog *pdialog)
{
  fc_assert_ret(NULL != pdialog);

  if (ODM_OPTSET == pdialog->mode && -1 != pdialog->optset.category) {
    options_iterate(pdialog->optset.poptset, poption) {
      if (pdialog->optset.category == option_category(poption)) {
        option_set_gui_data(poption, NULL);
      }
    } options_iterate_end;
  }

  if (NULL != pdialog->advanced) {
    free(pdialog->advanced->pScroll);
    free(pdialog->advanced);
  }

  popdown_window_group_dialog(pdialog->begin_widget_list,
                              pdialog->end_widget_list);

  free(pdialog);
}

/************************************************************************//**
  Return the number of options of the category.
****************************************************************************/
static int optset_category_option_count(const struct option_set *poptset,
                                        int category)
{
  int count = 0;

  options_iterate(poptset, poption) {
    if (category == option_category(poption)) {
      count++;
    }
  } options_iterate_end;

  return count;
}

/************************************************************************//**
  Initialize a option set page.
****************************************************************************/
static void option_dialog_optset(struct option_dialog *pdialog,
                                 const struct option_set *poptset)
{
  struct option_dialog_optset *poptset_dialog;
  struct widget *window;
  struct widget *widget = NULL;
  int i,  category_num;

  fc_assert_ret(NULL != pdialog);
  fc_assert_ret(NULL != poptset);
  category_num = optset_category_number(poptset);
  fc_assert_ret(0 < category_num);

  poptset_dialog = &pdialog->optset;
  pdialog->mode = ODM_OPTSET;
  poptset_dialog->poptset = poptset;
  poptset_dialog->category = -1;

  window = pdialog->end_widget_list;

  /* Hide ODM_MAIN widget group. */
  hide_group(pdialog->main_widget_list, pdialog->core_widget_list->prev);

  /* Otherwise we don't enter next loop at all, and widget will remain NULL */
  fc_assert(category_num > 0);

  /* Make the category buttons. */
  for (i = 0; i < category_num; i++) {
    if (0 == optset_category_option_count(poptset, i)) {
      continue;
    }

    widget = create_icon_button_from_chars(NULL, window->dst,
                                           optset_category_name(poptset, i),
                                           adj_font(12), 0);
    widget->action = option_category_callback;
    set_wstate(widget, FC_WS_NORMAL);
    widget_resize(widget, widget->size.w, widget->size.h + adj_size(4));
    add_to_gui_list(MAX_ID - i, widget);
  }

  poptset_dialog->widget_list = widget;
  pdialog->begin_widget_list = widget;

  arrange_widgets(window, 1, -1, widget, pdialog->main_widget_list,
                  pdialog->core_widget_list, NULL);
}

/************************************************************************//**
  Initialize a option set category page.
****************************************************************************/
static void option_dialog_optset_category(struct option_dialog *pdialog,
                                          int category)
{
  struct option_dialog_optset *poptset_dialog;
  const struct option_set *poptset;
  struct widget *window, *widget = NULL, *apply_button;
  const int MAX_SHOWN = 10;
  SDL_Rect area;
  int i;

  fc_assert_ret(NULL != pdialog);
  fc_assert_ret(ODM_OPTSET == pdialog->mode);
  fc_assert_ret(NULL == pdialog->advanced);
  poptset_dialog = &pdialog->optset;
  poptset = poptset_dialog->poptset;
  fc_assert_ret(0 < optset_category_option_count(poptset, category));

  /* Hide ODM_OPTSET widget group. */
  hide_group(poptset_dialog->widget_list, pdialog->main_widget_list->prev);

  poptset_dialog->category = category;
  window = pdialog->end_widget_list;

  /* Create the apply button. */
  apply_button = create_themeicon(current_theme->Small_OK_Icon, window->dst,
                                  WF_WIDGET_HAS_INFO_LABEL
                                  | WF_RESTORE_BACKGROUND);
  apply_button->info_label = create_utf8_from_char(_("Apply changes"),
                                                   adj_font(12));
  apply_button->action = apply_callback;
  set_wstate(apply_button, FC_WS_NORMAL);
  add_to_gui_list(ID_OPTIONS_APPLY_BUTTON, apply_button);

  /* Create the option widgets. */
  i = 0;
  options_iterate(poptset, poption) {
    if (category != option_category(poption)) {
      continue;
    }

    widget = option_widget_new(poption, window, i >= MAX_SHOWN);
    i++;
  } options_iterate_end;

  /* Scrollbar. */
  pdialog->advanced = fc_calloc(1, sizeof(*pdialog->advanced));
  pdialog->advanced->pEndWidgetList = pdialog->end_widget_list;
  pdialog->advanced->pEndActiveWidgetList = apply_button->prev;
  pdialog->advanced->pBeginWidgetList = widget;
  pdialog->advanced->pBeginActiveWidgetList = widget;

  create_vertical_scrollbar(pdialog->advanced, 2, MAX_SHOWN, TRUE, TRUE);

  if (i >= MAX_SHOWN) {
    pdialog->advanced->pActiveWidgetList =
        pdialog->advanced->pEndActiveWidgetList;
  } else {
    hide_scrollbar(pdialog->advanced->pScroll);
  }

  pdialog->begin_widget_list = pdialog->advanced->pBeginWidgetList;

  arrange_widgets(window, 2, MAX_SHOWN,
                  pdialog->advanced->pBeginActiveWidgetList,
                  apply_button, pdialog->core_widget_list,
                  apply_button, NULL);

  area = window->area;
  setup_vertical_scrollbar_area(pdialog->advanced->pScroll,
                                area.x + area.w - 1, area.y + 1,
                                area.h - adj_size(32), TRUE);

  redraw_group(pdialog->begin_widget_list,
               pdialog->advanced->pActiveWidgetList, 0);
  widget_flush(window);
}


/************************************************************************//**
  Clicked on a global worklist name.
****************************************************************************/
static int edit_worklist_callback(struct widget *widget)
{
  struct global_worklist *pgwl = global_worklist_by_id(MAX_ID - widget->ID);

  if (NULL == option_dialog
      || ODM_WORKLIST != option_dialog->mode
      || NULL == pgwl) {
    return -1;
  }

  switch (Main.event.button.button) {
  case SDL_BUTTON_LEFT:
    /* Edit. */
    option_dialog->worklist.edited_name = widget;
    popup_worklist_editor(NULL, pgwl);
    break;

  case SDL_BUTTON_RIGHT:
    {
      /* Delete. */
      struct ADVANCED_DLG *advanced = option_dialog->advanced;
      bool scroll = (NULL != advanced->pActiveWidgetList);

      global_worklist_destroy(pgwl);
      del_widget_from_vertical_scroll_widget_list(advanced, widget);

      /* Find if there was scrollbar hide. */
      if (scroll && advanced->pActiveWidgetList == NULL) {
        int len = advanced->pScroll->pUp_Left_Button->size.w;

        widget = advanced->pEndActiveWidgetList->next;
        do {
          widget = widget->prev;
          widget->size.w += len;
          FREESURFACE(widget->gfx);
        } while (widget != advanced->pBeginActiveWidgetList);
      }

      redraw_group(option_dialog->begin_widget_list,
                   option_dialog->end_widget_list, 0);
      widget_mark_dirty(option_dialog->end_widget_list);
      flush_dirty();
    }
    break;
  }

  return -1;
}

/************************************************************************//**
  Callback to append a global worklist.
****************************************************************************/
static int add_new_worklist_callback(struct widget *widget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct widget *new_worklist_widget = NULL;
    struct widget *window = option_dialog->end_widget_list;
    struct global_worklist *pgwl = global_worklist_new(_("empty worklist"));
    struct ADVANCED_DLG *advanced = option_dialog->advanced;
    bool scroll = advanced->pActiveWidgetList == NULL;
    bool redraw_all = FALSE;

    set_wstate(widget, FC_WS_NORMAL);
    selected_widget = NULL;

    /* Create list element. */
    new_worklist_widget =
      create_iconlabel_from_chars(NULL, widget->dst,
                                  global_worklist_name(pgwl),
                                  adj_font(12), WF_RESTORE_BACKGROUND);
    new_worklist_widget->ID = MAX_ID - global_worklist_id(pgwl);
    new_worklist_widget->string_utf8->style |= SF_CENTER;
    set_wstate(new_worklist_widget, FC_WS_NORMAL);
    new_worklist_widget->size.w = widget->size.w;
    new_worklist_widget->action = edit_worklist_callback;

    /* Add to widget list. */
    redraw_all = add_widget_to_vertical_scroll_widget_list(advanced,
                     new_worklist_widget, widget, TRUE,
                     window->area.x + adj_size(17),
                     window->area.y + adj_size(17));

    /* Find if there was scrollbar shown. */
    if (scroll && advanced->pActiveWidgetList != NULL) {
      int len = advanced->pScroll->pUp_Left_Button->size.w;

      window = advanced->pEndActiveWidgetList->next;
      do {
        window = window->prev;
        window->size.w -= len;
        window->area.w -= len;
        FREESURFACE(window->gfx);
      } while (window != advanced->pBeginActiveWidgetList);
    }

    if (redraw_all) {
      redraw_group(option_dialog->begin_widget_list,
                   option_dialog->end_widget_list, 0);
      widget_mark_dirty(option_dialog->end_widget_list);
    } else {
      /* Redraw only new widget and dock widget. */
      if (!widget->gfx && (get_wflags(widget) & WF_RESTORE_BACKGROUND)) {
        refresh_widget_background(widget);
      }
      widget_redraw(widget);
      widget_mark_dirty(widget);

      if (!new_worklist_widget->gfx &&
          (get_wflags(new_worklist_widget) & WF_RESTORE_BACKGROUND)) {
        refresh_widget_background(new_worklist_widget);
      }
      widget_redraw(new_worklist_widget);
      widget_mark_dirty(new_worklist_widget);
    }
    flush_dirty();
  }

  return -1;
}

/************************************************************************//**
  The Worklist Report part of Options dialog shows all the global worklists
  that the player has defined.
****************************************************************************/
static void option_dialog_worklist(struct option_dialog *pdialog)
{
  SDL_Color bg_color = {255, 255, 255, 128};
  struct widget *widget, *window, *background;
  int count = 0, scrollbar_width = 0, longest = 0;
  SDL_Rect area;

  pdialog->mode = ODM_WORKLIST;
  pdialog->worklist.edited_name = NULL;
  window = pdialog->end_widget_list;

  /* Hide main widget group. */
  hide_group(pdialog->main_widget_list, pdialog->core_widget_list->prev);

  /* Create white background. */
  background = create_iconlabel(NULL, window->dst, NULL, WF_FREE_THEME);
  add_to_gui_list(ID_LABEL, background);

  /* Build the global worklists list. */
  global_worklists_iterate(pgwl) {
    widget = create_iconlabel_from_chars(NULL, window->dst,
                                         global_worklist_name(pgwl),
                                         adj_font(12),
                                         WF_RESTORE_BACKGROUND);
    set_wstate(widget, FC_WS_NORMAL);
    add_to_gui_list(MAX_ID - global_worklist_id(pgwl), widget);
    widget->action = edit_worklist_callback;
    widget->string_utf8->style |= SF_CENTER;
    longest = MAX(longest, widget->size.w);
    count++;

    if (count > 13) {
      set_wflag(widget, WF_HIDDEN);
    }
  } global_worklists_iterate_end;

  /* Create the adding item. */
  widget = create_iconlabel_from_chars(NULL, window->dst,
                                       _("Add new worklist"), adj_font(12),
                                       WF_RESTORE_BACKGROUND);
  set_wstate(widget, FC_WS_NORMAL);
  add_to_gui_list(ID_ADD_NEW_WORKLIST, widget);
  widget->action = add_new_worklist_callback;
  widget->string_utf8->style |= SF_CENTER;
  longest = MAX(longest, widget->size.w);
  count++;

  if (count > 13) {
    set_wflag(widget, WF_HIDDEN);
  }

  /* Advanced dialog. */
  pdialog->advanced = fc_calloc(1, sizeof(*pdialog->advanced));
  pdialog->advanced->pEndWidgetList = pdialog->end_widget_list;
  pdialog->advanced->pEndActiveWidgetList =
      pdialog->main_widget_list->prev->prev;
  pdialog->advanced->pBeginWidgetList = widget;
  pdialog->advanced->pBeginActiveWidgetList = widget;

  /* Clear former area. */
  area = window->area;
  area.w += window->size.x;
  area.h += window->size.y;
  dirty_sdl_rect(&area);

  /* Resize window. */
  resize_window(window, NULL, NULL,
                adj_size(longest + 40), window->size.h);
  area = window->area;

  /* Move close button. */
  widget = pdialog->core_widget_list;
  widget_set_position(widget, area.x + area.w - widget->size.w - 1,
                      window->size.y + adj_size(2));

  /* Resize white background. */
  area.x += adj_size(12);
  area.y += adj_size(12);
  area.w -= adj_size(12) + adj_size(12);
  area.h -= adj_size(12) + adj_size(12);
  background->theme = create_surf(area.w, area.h, SDL_SWSURFACE);
  widget_set_area(background, area);
  widget_set_position(background, area.x, area.y);
  SDL_FillRect(background->theme, NULL,
               map_rgba(background->theme->format, bg_color));

  create_frame(background->theme,
               0, 0, background->theme->w - 1, background->theme->h - 1,
               get_theme_color(COLOR_THEME_OPTIONDLG_WORKLISTLIST_FRAME));

  /* Create the Scrollbar. */
  scrollbar_width = create_vertical_scrollbar(pdialog->advanced,
                                              1, 13, TRUE, TRUE);
  setup_vertical_scrollbar_area(pdialog->advanced->pScroll,
                                area.x + area.w - 1, area.y + 1,
                                area.h - adj_size(32), TRUE);

  if (count > 13) {
    pdialog->advanced->pActiveWidgetList =
        pdialog->advanced->pEndActiveWidgetList;
  } else {
    hide_scrollbar(pdialog->advanced->pScroll);
    scrollbar_width = 0;
  }

  /* Draw! */
  setup_vertical_widgets_position(1, area.x + adj_size(5),
                                  area.y + adj_size(5),
                                  area.w - adj_size(10) - scrollbar_width, 0,
                                  pdialog->advanced->pBeginActiveWidgetList,
                                  pdialog->advanced->pEndActiveWidgetList);

  pdialog->begin_widget_list = pdialog->advanced->pBeginWidgetList;

  redraw_group(pdialog->begin_widget_list, pdialog->end_widget_list, 0);
  widget_flush(window);
}

/************************************************************************//**
  User interacted with the option dialog button.
****************************************************************************/
int optiondlg_callback(struct widget *pbutton)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    set_wstate(pbutton, FC_WS_DISABLED);
    clear_surface(pbutton->dst->surface, &pbutton->size);
    widget_redraw(pbutton);
    widget_flush(pbutton);

    popup_optiondlg();
  }

  return -1;
}

/* ======================================================================= */
/* =================================== Public ============================ */
/* ======================================================================= */

/************************************************************************//**
  Enable button to open option dialog.
****************************************************************************/
void enable_options_button(void)
{
  set_wstate(pOptions_Button, FC_WS_NORMAL);
}

/************************************************************************//**
  Disable button to open option dialog.
****************************************************************************/
void disable_options_button(void)
{
  set_wstate(pOptions_Button, FC_WS_DISABLED);
}

/************************************************************************//**
  Create button to open option dialog.
****************************************************************************/
void init_options_button(void)
{
  char buf[256];

  pOptions_Button = create_themeicon(current_theme->Options_Icon, Main.gui,
                                     WF_WIDGET_HAS_INFO_LABEL
                                     | WF_RESTORE_BACKGROUND);
  pOptions_Button->action = optiondlg_callback;
  fc_snprintf(buf, sizeof(buf), "%s (%s)", _("Options"), "Esc");
  pOptions_Button->info_label = create_utf8_from_char(buf, adj_font(12));
  pOptions_Button->key = SDLK_ESCAPE;
  set_wflag(pOptions_Button, WF_HIDDEN);
  widget_set_position(pOptions_Button, adj_size(5), adj_size(5));

#ifndef SMALL_SCREEN
  add_to_gui_list(ID_CLIENT_OPTIONS, pOptions_Button);
#endif

  enable_options_button();
}

/************************************************************************//**
  If the Options Dlg is open, force Worklist List contents to be updated.
  This function is call by exiting worklist editor to update changed
  worklist name in global worklist report ( Options Dlg )
****************************************************************************/
void update_worklist_report_dialog(void)
{
  struct global_worklist *pgwl;

  if (NULL != option_dialog && ODM_WORKLIST == option_dialog->mode) {
    pgwl = global_worklist_by_id(MAX_ID
                                 - option_dialog->worklist.edited_name->ID);

    if (NULL != pgwl) {
      copy_chars_to_utf8_str(option_dialog->worklist.edited_name->string_utf8,
                             global_worklist_name(pgwl));
      option_dialog->worklist.edited_name = NULL;
    }

    redraw_group(option_dialog->begin_widget_list,
                 option_dialog->end_widget_list, 0);
    widget_mark_dirty(option_dialog->end_widget_list);
  }
}

/************************************************************************//**
  Popup the main option menu dialog.
****************************************************************************/
void popup_optiondlg(void)
{
  if (NULL != option_dialog) {
    return;
  }

  restore_meswin_dialog = meswin_dialog_is_open();
  popdown_all_game_dialogs();
  flush_dirty();

  option_dialog = option_dialog_new();

  disable_main_widgets();
  flush_dirty();
}

/************************************************************************//**
  Close option dialog.
****************************************************************************/
void popdown_optiondlg(bool leave_game)
{
  if (NULL == option_dialog) {
    return;
  }

  option_dialog_destroy(option_dialog);
  option_dialog = NULL;

  if (!leave_game) {
    enable_main_widgets();
  }

  if (restore_meswin_dialog) {
    meswin_dialog_popup(TRUE);
  }
}

/************************************************************************//**
  Popup the option dialog for the option set.
****************************************************************************/
void option_dialog_popup(const char *name, const struct option_set *poptset)
{
  if (NULL == option_dialog) {
    popup_optiondlg();
  } else if (ODM_OPTSET == option_dialog->mode
             && poptset == option_dialog->optset.poptset) {
    /* Already in use. */
    return;
  } else {
    while (ODM_MAIN != option_dialog->mode) {
      back_callback(NULL);
    }
  }

  option_dialog_optset(option_dialog, poptset);
}

/************************************************************************//**
  Popdown the option dialog for the option set.
****************************************************************************/
void option_dialog_popdown(const struct option_set *poptset)
{
  while (NULL != option_dialog
         && ODM_OPTSET == option_dialog->mode
         && poptset == option_dialog->optset.poptset) {
    back_callback(NULL);
  }
}

/************************************************************************//**
  Update the GUI for the option.
****************************************************************************/
void option_gui_update(struct option *poption)
{
  if (NULL != option_dialog
      && ODM_OPTSET == option_dialog->mode
      && option_optset(poption) == option_dialog->optset.poptset
      && option_category(poption) == option_dialog->optset.category) {
    option_widget_update(poption);
  }

  if (!strcmp(option_name(poption), "nationset")) {
    nationset_changed();
  }
}

/************************************************************************//**
  Add the GUI for the option.
****************************************************************************/
void option_gui_add(struct option *poption)
{
  if (NULL != option_dialog
      && ODM_OPTSET == option_dialog->mode
      && option_optset(poption) == option_dialog->optset.poptset
      && option_category(poption) == option_dialog->optset.category) {
    back_callback(NULL);
    option_dialog_optset_category(option_dialog, option_category(poption));
  }
}

/************************************************************************//**
  Remove the GUI for the option.
****************************************************************************/
void option_gui_remove(struct option *poption)
{
  if (NULL != option_dialog
      && ODM_OPTSET == option_dialog->mode
      && option_optset(poption) == option_dialog->optset.poptset
      && option_category(poption) == option_dialog->optset.category) {
    back_callback(NULL);
    option_dialog_optset_category(option_dialog, option_category(poption));
  }
}
