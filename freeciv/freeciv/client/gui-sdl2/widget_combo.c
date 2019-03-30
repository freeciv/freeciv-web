/***********************************************************************
 Freeciv - Copyright (C) 2006 - The Freeciv Project
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
#include <fc_config.h>
#endif

/* SDL2 */
#ifdef SDL2_PLAIN_INCLUDE
#include <SDL.h>
#else  /* SDL2_PLAIN_INCLUDE */
#include <SDL2/SDL.h>
#endif /* SDL2_PLAIN_INCLUDE */

/* utility */
#include "log.h"
#include "string_vector.h"

/* client/gui-sdl2 */
#include "colors.h"
#include "gui_iconv.h"
#include "gui_id.h"
#include "gui_string.h"
#include "gui_tilespec.h"
#include "mapview.h"
#include "widget.h"
#include "widget_p.h"

struct combo_menu {
  struct widget *begin_widget_list;
  struct widget *end_widget_list;
};

static int (*baseclass_redraw) (struct widget *widget) = NULL;


/************************************************************************//**
  Redraw the combo box widget.
****************************************************************************/
static int combo_redraw(struct widget *combo)
{
  SDL_Rect dest = { combo->size.x, combo->size.y, 0, 0 };
  SDL_Surface *text, *surface;
  struct combo_menu *menu;
  int ret;

  ret = baseclass_redraw(combo);
  if (0 != ret) {
    return ret;
  }

  surface = create_bcgnd_surf(combo->theme, get_wstate(combo),
                              combo->size.w, combo->size.h);

  if (NULL == surface) {
    return -1;
  }

  /* Blit theme. */
  alphablit(surface, NULL, combo->dst->surface, &dest, 255);

  /* Set position and blit text. */
  text = create_text_surf_from_utf8(combo->string_utf8);
  if (NULL != text) {
    dest.y += (surface->h - surface->h) / 2;
    /* Blit centred text to botton. */
    if (combo->string_utf8->style & SF_CENTER) {
      dest.x += (surface->w - text->w) / 2;
    } else {
      if (combo->string_utf8->style & SF_CENTER_RIGHT) {
        dest.x += surface->w - text->w - adj_size(5);
      } else {
        dest.x += adj_size(5); /* center left */
      }
    }

    alphablit(text, NULL, combo->dst->surface, &dest, 255);
  }
  /* text. */
  ret = surface->h;

  /* Free memory */
  FREESURFACE(text);
  FREESURFACE(surface);

  menu = (struct combo_menu *) combo->private_data.ptr;
  if (NULL != menu) {
    ret = redraw_group(menu->begin_widget_list, menu->end_widget_list, 0);
  }

  return ret;
}

/************************************************************************//**
  User interacted with the combo menu window.
****************************************************************************/
static int combo_menu_callback(struct widget *window)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct combo_menu *menu =
        (struct combo_menu *)window->data.widget->private_data.ptr;

    move_window_group(menu->begin_widget_list, menu->end_widget_list);
  }

  return -1;
}

/************************************************************************//**
  User interacted with a single item of the combo menu.
****************************************************************************/
static int combo_menu_item_callback(struct widget *label)
{
  struct widget *combo = label->data.widget;

  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    copy_chars_to_utf8_str(combo->string_utf8, label->string_utf8->text);
    widget_redraw(combo);
    widget_mark_dirty(combo);
  }
  combo_popdown(combo);

  return -1;
}

/************************************************************************//**
  Popup the combo box widget.
****************************************************************************/
void combo_popup(struct widget *combo)
{
  struct combo_menu *menu;
  struct widget *window, *label = NULL;
  int longest = 0, h = 0, x, y;

  fc_assert_ret(NULL != combo);
  fc_assert_ret(WT_COMBO == get_wtype(combo));

  if (NULL != combo->private_data.ptr) {
    return;
  }

  if (0 >= strvec_size(combo->data.vector)) {
    return;
  }

  /* Menu. */
  window = create_window_skeleton(NULL, NULL, 0);
  window->action = combo_menu_callback;
  window->data.widget = combo;
  set_wstate(window, FC_WS_NORMAL);
  add_to_gui_list(ID_COMBO_MENU, window);

  /* Labels. */
  strvec_iterate(combo->data.vector, string) {
    label = create_iconlabel_from_chars(NULL, window->dst, string,
                                        adj_font(10), WF_RESTORE_BACKGROUND);
    label->action = combo_menu_item_callback;
    label->data.widget = combo;
    set_wstate(label, FC_WS_NORMAL);
    add_to_gui_list(ID_LABEL, label);

    longest = MAX(longest, label->size.w);
    widget_set_position(label, adj_size(10), h);
    h += adj_size(15);
  } strvec_iterate_end;

  /* Resize and relocate the window. */
  resize_window(window, NULL, NULL, longest + 2 * adj_size(10), h);

  x = combo->size.x + combo->dst->dest_rect.x;
  if (x + window->size.w > main_window_width()) {
    x = main_window_width() - window->size.w;
  }
  if (x < 0) {
    x = 0;
  }

  y = combo->size.y - h + combo->dst->dest_rect.y;
  if (y + window->size.h > main_window_height()) {
    y = main_window_height() - window->size.h;
  }
  if (y < 0) {
    y = 0;
  }

  widget_set_position(window, x, y);

  /* Make data. */
  menu = fc_malloc(sizeof(*menu));
  menu->begin_widget_list = label;
  menu->end_widget_list = window;
  combo->private_data.ptr = menu;

  /* Redraw. */
  redraw_group(menu->begin_widget_list, menu->end_widget_list, 0);
  widget_mark_dirty(window);
  flush_dirty();
}

/************************************************************************//**
  Popdown the combo box widget.
****************************************************************************/
void combo_popdown(struct widget *combo)
{
  struct combo_menu *menu;

  fc_assert_ret(NULL != combo);
  fc_assert_ret(WT_COMBO == get_wtype(combo));

  menu = (struct combo_menu *) combo->private_data.ptr;
  if (NULL == menu) {
    return;
  }

  widget_mark_dirty(menu->end_widget_list);
  popdown_window_group_dialog(menu->begin_widget_list,
                              menu->end_widget_list);
  free(menu);
  combo->private_data.ptr = NULL;
  flush_dirty();
}

/************************************************************************//**
  Create a combo box widget.
****************************************************************************/
struct widget *combo_new(SDL_Surface *background, struct gui_layer *dest,
                         utf8_str *pstr, const struct strvec *vector,
                         int length, Uint32 flags)
{
  SDL_Rect buf = {0, 0, 0, 0};
  struct widget *combo = widget_new();

  combo->theme = current_theme->Edit;
  combo->theme2 = background;
  combo->string_utf8 = pstr;
  set_wflag(combo, WF_FREE_STRING | WF_FREE_GFX | flags);
  set_wstate(combo, FC_WS_DISABLED);
  set_wtype(combo, WT_COMBO);
  combo->mod = KMOD_NONE;

  baseclass_redraw = combo->redraw;
  combo->redraw = combo_redraw;
  combo->destroy = combo_popdown;

  if (NULL != pstr) {
    combo->string_utf8->style |= SF_CENTER;
    utf8_str_size(pstr, &buf);
    buf.h += adj_size(4);
  }

  length = MAX(length, buf.w + adj_size(10));
  correct_size_bcgnd_surf(current_theme->Edit, &length, &buf.h);
  combo->size.w = buf.w + adj_size(10);
  combo->size.h = buf.h;

  if (dest) {
    combo->dst = dest;
  } else {
    combo->dst = add_gui_layer(combo->size.w, combo->size.h);
  }
  combo->data.vector = vector;
  combo->private_data.ptr = NULL;

  return combo;
}
