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

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <gdk/gdkkeysyms.h>

/* utility */
#include "fcintl.h"
#include "genlist.h"
#include "log.h"
#include "mem.h"
#include "support.h"

/* common */
#include "chat.h"
#include "featured_text.h"
#include "game.h"
#include "packets.h"

/* client */
#include "client_main.h"
#include "climap.h"
#include "control.h"
#include "mapview_common.h"

/* client/gui-gtk-3.0 */
#include "colors.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "pages.h"

#include "chatline.h"

#define	MAX_CHATLINE_HISTORY 20

static struct genlist *history_list = NULL;
static int history_pos = -1;

static struct inputline_toolkit {
  GtkWidget *main_widget;
  GtkWidget *entry;
  GtkWidget *button_box;
  GtkWidget *toolbar;
  GtkWidget *toggle_button;
  bool toolbar_displayed;
} toolkit;      /* Singleton. */

static void inputline_make_tag(GtkEntry *entry, enum text_tag_type type);

/**********************************************************************//**
  Returns TRUE iff the input line has focus.
**************************************************************************/
bool inputline_has_focus(void)
{
  return gtk_widget_has_focus(toolkit.entry);
}

/**********************************************************************//**
  Gives the focus to the intput line.
**************************************************************************/
void inputline_grab_focus(void)
{
  gtk_widget_grab_focus(toolkit.entry);
}

/**********************************************************************//**
  Returns TRUE iff the input line is currently visible.
**************************************************************************/
bool inputline_is_visible(void)
{
  return gtk_widget_get_mapped(toolkit.entry);
}

/**********************************************************************//**
  Helper function to determine if a given client input line is intended as
  a "plain" public message. Note that messages prefixed with : are a
  special case (explicit public messages), and will return FALSE.
**************************************************************************/
static bool is_plain_public_message(const char *s)
{
  const char *p;

  /* If it is a server command or an explicit ally
   * message, then it is not a public message. */
  if (s[0] == SERVER_COMMAND_PREFIX || s[0] == CHAT_ALLIES_PREFIX) {
    return FALSE;
  }

  /* It might be a private message of the form
   *   'player name with spaces':the message
   * or with ". So skip past the player name part. */
  if (s[0] == '\'' || s[0] == '"') {
    p = strchr(s + 1, s[0]);
  } else {
    p = s;
  }

  /* Now we just need to check that it is not a private
   * message. If we encounter a space then the preceeding
   * text could not have been a user/player name (the
   * quote check above eliminated names with spaces) so
   * it must be a public message. Otherwise if we encounter
   * the message prefix : then the text parsed up until now
   * was a player/user name and the line is intended as
   * a private message (or explicit public message if the
   * first character is :). */
  while (p != NULL && *p != '\0') {
    if (fc_isspace(*p)) {
      return TRUE;
    } else if (*p == CHAT_DIRECT_PREFIX) {
      return FALSE;
    }
    p++;
  }
  return TRUE;
}

/**********************************************************************//**
  Called when the return key is pressed.
**************************************************************************/
static void inputline_return(GtkEntry *w, gpointer data)
{
  const char *theinput;

  theinput = gtk_entry_get_text(w);
  
  if (*theinput) {
    if (client_state() == C_S_RUNNING
        && GUI_GTK_OPTION(allied_chat_only)
        && is_plain_public_message(theinput)) {
      char buf[MAX_LEN_MSG];

      fc_snprintf(buf, sizeof(buf), ". %s", theinput);
      send_chat(buf);
    } else {
      send_chat(theinput);
    }

    if (genlist_size(history_list) >= MAX_CHATLINE_HISTORY) {
      void *history_data;

      history_data = genlist_get(history_list, -1);
      genlist_remove(history_list, history_data);
      free(history_data);
    }

    genlist_prepend(history_list, fc_strdup(theinput));
    history_pos=-1;
  }

  gtk_entry_set_text(w, "");
}

/**********************************************************************//**
  Returns the name of player or user, set in the same list.
**************************************************************************/
static const char *get_player_or_user_name(int id)
{
  size_t size = conn_list_size(game.all_connections);

  if (id < size) {
    return conn_list_get(game.all_connections, id)->username;
  } else {
    struct player *pplayer = player_by_number(id - size);
    if (pplayer) {
      return pplayer->name;
    } else {
      /* Empty slot. Relies on being used with comparison function
       * which can cope with NULL. */
      return NULL;
    }
  }
}

/**********************************************************************//**
  Find a player or a user by prefix.

  prefix - The prefix.
  matches - A string array to set the matches result.
  max_matches - The maximum of matches.
  match_len - The length of the string used to returns matches.

  Returns the number of the matches names.
**************************************************************************/
static int check_player_or_user_name(const char *prefix,
                                     const char **matches,
                                     const int max_matches)
{
  int matches_id[max_matches * 2], ind, num;

  switch (match_prefix_full(get_player_or_user_name,
                            player_slot_count()
                            + conn_list_size(game.all_connections),
                            MAX_LEN_NAME, fc_strncasecmp, strlen,
                            prefix, &ind, matches_id,
                            max_matches * 2, &num)) {
  case M_PRE_EXACT:
  case M_PRE_ONLY:
    matches[0] = get_player_or_user_name(ind);
    return 1;
  case M_PRE_AMBIGUOUS:
    {
      /* Remove duplications playername/username. */
      const char *name;
      int i, j, c = 0;

      for (i = 0; i < num && c < max_matches; i++) {
        name = get_player_or_user_name(matches_id[i]);
        for (j = 0; j < c; j++) {
          if (0 == fc_strncasecmp(name, matches[j], MAX_LEN_NAME)) {
            break;
          }
        }
        if (j >= c) {
          matches[c++] = name;
        }
      }
      return c;
    }
  case M_PRE_EMPTY:
  case M_PRE_LONG:
  case M_PRE_FAIL:
  case M_PRE_LAST:
    break;
  }

  return 0;
}

/**********************************************************************//**
  Find the larger common prefix.

  prefixes - A list of prefixes.
  num_prefixes - The number of prefixes.
  buf - The buffer to set.
  buf_len - The maximal size of the buffer.

  Returns the length of the common prefix (in characters).
**************************************************************************/
static size_t get_common_prefix(const char *const *prefixes,
                                size_t num_prefixes,
                                char *buf, size_t buf_len)
{
  const char *p;
  char *q;
  size_t i;

  fc_strlcpy(buf, prefixes[0], buf_len);
  for (i = 1; i < num_prefixes; i++) {
    for (p = prefixes[i], q = buf; *p != '\0' && *q != '\0';
         p = g_utf8_next_char(p), q = g_utf8_next_char(q)) {
      if (g_unichar_toupper(g_utf8_get_char(p))
          != g_unichar_toupper(g_utf8_get_char(q))) {
        *q = '\0';
        break;
      }
    }
  }

 return g_utf8_strlen(buf, -1);
}

/**********************************************************************//**
  Autocompletes the input line with a player or user name.
  Returns FALSE if there is no string to complete.
**************************************************************************/
static bool chatline_autocomplete(GtkEditable *editable)
{
#define MAX_MATCHES 10
  const char *name[MAX_MATCHES];
  char buf[MAX_LEN_NAME * MAX_MATCHES];
  gint pos;
  gchar *chars, *p, *prev;
  int num, i;
  size_t prefix_len;

  /* Part 1: get the string to complete. */
  pos = gtk_editable_get_position(editable);
  chars = gtk_editable_get_chars(editable, 0, pos);

  p = chars + strlen(chars);
  while ((prev = g_utf8_find_prev_char(chars, p))) {
    if (!g_unichar_isalnum(g_utf8_get_char(prev))) {
      break;
    }
    p = prev;
  }
  /* p points to the start of the last word, or the start of the string. */

  prefix_len = g_utf8_strlen(p, -1);
  if (0 == prefix_len) {
    /* Empty: nothing to complete, propagate the event. */
    g_free(chars);
    return FALSE;
  }

  /* Part 2: compare with player and user names. */
  num = check_player_or_user_name(p, name, MAX_MATCHES);
  if (1 == num) {
    gtk_editable_delete_text(editable, pos - prefix_len, pos);
    pos -= prefix_len;
    gtk_editable_insert_text(editable, name[0], strlen(name[0]), &pos);
    gtk_editable_set_position(editable, pos);
    g_free(chars);
    return TRUE;
  } else if (num > 1) {
    if (get_common_prefix(name, num, buf, sizeof(buf)) > prefix_len) {
      gtk_editable_delete_text(editable, pos - prefix_len, pos);
      pos -= prefix_len;
      gtk_editable_insert_text(editable, buf, strlen(buf), &pos);
      gtk_editable_set_position(editable, pos);
    }
    sz_strlcpy(buf, name[0]);
    for (i = 1; i < num; i++) {
      cat_snprintf(buf, sizeof(buf), ", %s", name[i]);
    }
    /* TRANS: comma-separated list of player/user names for completion */
    output_window_printf(ftc_client, _("Suggestions: %s."), buf);
  }

  g_free(chars);
  return TRUE;
}

/**********************************************************************//**
  Called when a key is pressed.
**************************************************************************/
static gboolean inputline_handler(GtkWidget *w, GdkEventKey *ev)
{
  if ((ev->state & GDK_CONTROL_MASK)) {
    /* Chatline featured text support. */
    switch (ev->keyval) {
    case GDK_KEY_b:
      inputline_make_tag(GTK_ENTRY(w), TTT_BOLD);
      return TRUE;

    case GDK_KEY_c:
      inputline_make_tag(GTK_ENTRY(w), TTT_COLOR);
      return TRUE;

    case GDK_KEY_i:
      inputline_make_tag(GTK_ENTRY(w), TTT_ITALIC);
      return TRUE;

    case GDK_KEY_s:
      inputline_make_tag(GTK_ENTRY(w), TTT_STRIKE);
      return TRUE;

    case GDK_KEY_u:
      inputline_make_tag(GTK_ENTRY(w), TTT_UNDERLINE);
      return TRUE;

    default:
      break;
    }

  } else {
    /* Chatline history controls. */
    switch (ev->keyval) {
    case GDK_KEY_Up:
      if (history_pos < genlist_size(history_list) - 1) {
        gtk_entry_set_text(GTK_ENTRY(w),
                           genlist_get(history_list, ++history_pos));
        gtk_editable_set_position(GTK_EDITABLE(w), -1);
      }
      return TRUE;

    case GDK_KEY_Down:
      if (history_pos >= 0) {
        history_pos--;
      }

      if (history_pos >= 0) {
        gtk_entry_set_text(GTK_ENTRY(w),
                           genlist_get(history_list, history_pos));
      } else {
        gtk_entry_set_text(GTK_ENTRY(w), "");
      }
      gtk_editable_set_position(GTK_EDITABLE(w), -1);
      return TRUE;

    case GDK_KEY_Tab:
      if (GUI_GTK_OPTION(chatline_autocompletion)) {
        return chatline_autocomplete(GTK_EDITABLE(w));
      }

    default:
      break;
    }
  }

  return FALSE;
}

/**********************************************************************//**
  Make a text tag for the selected text.
**************************************************************************/
void inputline_make_tag(GtkEntry *entry, enum text_tag_type type)
{
  char buf[MAX_LEN_MSG];
  GtkEditable *editable = GTK_EDITABLE(entry);
  gint start_pos, end_pos;
  gchar *selection;
  gchar *fg_color_text = NULL, *bg_color_text = NULL;

  if (!gtk_editable_get_selection_bounds(editable, &start_pos, &end_pos)) {
    /* Let's say the selection starts and ends at the current position. */
    start_pos = end_pos = gtk_editable_get_position(editable);
  }

  selection = gtk_editable_get_chars(editable, start_pos, end_pos);

  if (type == TTT_COLOR) {
    /* Get the color arguments. */
    GdkRGBA *fg_color = g_object_get_data(G_OBJECT(entry), "fg_color");
    GdkRGBA *bg_color = g_object_get_data(G_OBJECT(entry), "bg_color");

    if (!fg_color && !bg_color) {
      goto CLEAN_UP;
    }

    if (fg_color) {
      fg_color_text = gdk_rgba_to_string(fg_color);
    }
    if (bg_color) {
      bg_color_text = gdk_rgba_to_string(bg_color);
    }

    if (0 == featured_text_apply_tag(selection, buf, sizeof(buf),
                                     TTT_COLOR, 0, FT_OFFSET_UNSET,
                                     ft_color_construct(fg_color_text,
                                                        bg_color_text))) {
      goto CLEAN_UP;
    }
  } else if (0 == featured_text_apply_tag(selection, buf, sizeof(buf),
                                          type, 0, FT_OFFSET_UNSET)) {
    goto CLEAN_UP;
  }

  /* Replace the selection. */
  gtk_editable_delete_text(editable, start_pos, end_pos);
  end_pos = start_pos;
  gtk_editable_insert_text(editable, buf, -1, &end_pos);
  gtk_editable_select_region(editable, start_pos, end_pos);

CLEAN_UP:
  g_free(selection);
  g_free(fg_color_text);
  g_free(bg_color_text);
}

/**********************************************************************//**
  Make a chat link at the current position or make the current selection
  clickable.
**************************************************************************/
void inputline_make_chat_link(struct tile *ptile, bool unit)
{
  char buf[MAX_LEN_MSG];
  GtkWidget *entry = toolkit.entry;
  GtkEditable *editable = GTK_EDITABLE(entry);
  gint start_pos, end_pos;
  gchar *chars;
  struct unit *punit;

  /* Get the target. */
  if (unit) {
    punit = find_visible_unit(ptile);
    if (!punit) {
      output_window_append(ftc_client, _("No visible unit on this tile."));
      return;
    }
  } else {
    punit = NULL;
  }

  if (gtk_editable_get_selection_bounds(editable, &start_pos, &end_pos)) {
    /* There is a selection, make it clickable. */
    gpointer target;
    enum text_link_type type;

    chars = gtk_editable_get_chars(editable, start_pos, end_pos);
    if (punit) {
      type = TLT_UNIT;
      target = punit;
    } else if (tile_city(ptile)) {
      type = TLT_CITY;
      target = tile_city(ptile);
    } else {
      type = TLT_TILE;
      target = ptile;
    }

    if (0 != featured_text_apply_tag(chars, buf, sizeof(buf), TTT_LINK,
                                     0, FT_OFFSET_UNSET, type, target)) {
      /* Replace the selection. */
      gtk_editable_delete_text(editable, start_pos, end_pos);
      end_pos = start_pos;
      gtk_editable_insert_text(editable, buf, -1, &end_pos);
      gtk_widget_grab_focus(entry);
      gtk_editable_select_region(editable, start_pos, end_pos);
    }
  } else {
    /* Just insert the link at the current position. */
    start_pos = gtk_editable_get_position(editable);
    end_pos = start_pos;
    chars = gtk_editable_get_chars(editable, MAX(start_pos - 1, 0),
                                   start_pos + 1);
    if (punit) {
      sz_strlcpy(buf, unit_link(punit));
    } else if (tile_city(ptile)) {
      sz_strlcpy(buf, city_link(tile_city(ptile)));
    } else {
      sz_strlcpy(buf, tile_link(ptile));
    }

    if (start_pos > 0 && strlen(chars) > 0 && chars[0] != ' ') {
      /* Maybe insert an extra space. */
      gtk_editable_insert_text(editable, " ", 1, &end_pos);
    }
    gtk_editable_insert_text(editable, buf, -1, &end_pos);
    if (chars[start_pos > 0 ? 1 : 0] != '\0'
        && chars[start_pos > 0 ? 1 : 0] != ' ') {
      /* Maybe insert an extra space. */
      gtk_editable_insert_text(editable, " ", 1, &end_pos);
    }
    gtk_widget_grab_focus(entry);
    gtk_editable_set_position(editable, end_pos);
  }

  g_free(chars);
}

/**********************************************************************//**
  Scroll a textview so that the given mark is visible, but only if the
  scroll window containing the textview is very close to the bottom. The
  text mark 'scroll_target' should probably be the first character of the
  last line in the text buffer.
**************************************************************************/
void scroll_if_necessary(GtkTextView *textview, GtkTextMark *scroll_target)
{
  GtkWidget *sw;
  GtkAdjustment *vadj;
  gdouble val, max, upper, page_size;

  fc_assert_ret(textview != NULL);
  fc_assert_ret(scroll_target != NULL);

  sw = gtk_widget_get_parent(GTK_WIDGET(textview));
  fc_assert_ret(sw != NULL);
  fc_assert_ret(GTK_IS_SCROLLED_WINDOW(sw));

  vadj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(sw));
  val = gtk_adjustment_get_value(vadj);
  g_object_get(G_OBJECT(vadj), "upper", &upper,
               "page-size", &page_size, NULL);
  max = upper - page_size;
  if (max - val < 10.0) {
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(textview), scroll_target,
                                 0.0, TRUE, 1.0, 0.0);
  }
}

/**********************************************************************//**
  Click a link.
**************************************************************************/
static gboolean event_after(GtkWidget *text_view, GdkEventButton *event)
{
  GtkTextIter start, end, iter;
  GtkTextBuffer *buffer;
  GSList *tags, *tagp;
  gint x, y;
  struct tile *ptile = NULL;

  if (event->type != GDK_BUTTON_RELEASE || event->button != 1) {
    return FALSE;
  }

  buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

  /* We shouldn't follow a link if the user has selected something. */
  gtk_text_buffer_get_selection_bounds(buffer, &start, &end);
  if (gtk_text_iter_get_offset(&start) != gtk_text_iter_get_offset(&end)) {
    return FALSE;
  }

  gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW (text_view), 
                                        GTK_TEXT_WINDOW_WIDGET,
                                        event->x, event->y, &x, &y);

  gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(text_view), &iter, x, y);

  if ((tags = gtk_text_iter_get_tags(&iter))) {
    for (tagp = tags; tagp; tagp = tagp->next) {
      GtkTextTag *tag = tagp->data;
      enum text_link_type type =
        GPOINTER_TO_INT(g_object_get_data(G_OBJECT(tag), "type"));

      if (type != 0) {
        /* This is a link. */
        int id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(tag), "id"));
        ptile = NULL;

        /* Real type is type - 1.
         * See comment in apply_text_tag() for g_object_set_data(). */
        type--;

        switch (type) {
        case TLT_CITY:
          {
            struct city *pcity = game_city_by_number(id);

            if (pcity) {
              ptile = client_city_tile(pcity);
            } else {
              output_window_append(ftc_client, _("This city isn't known!"));
            }
          }
          break;
        case TLT_TILE:
          ptile = index_to_tile(&(wld.map), id);

          if (!ptile) {
            output_window_append(ftc_client,
                                 _("This tile doesn't exist in this game!"));
          }
          break;
        case TLT_UNIT:
          {
            struct unit *punit = game_unit_by_number(id);

            if (punit) {
              ptile = unit_tile(punit);
            } else {
              output_window_append(ftc_client, _("This unit isn't known!"));
            }
          }
          break;
        }

        if (ptile) {
          center_tile_mapcanvas(ptile);
          link_mark_restore(type, id);
          gtk_widget_grab_focus(GTK_WIDGET(map_canvas));
        }
      }
    }
    g_slist_free(tags);
  }

  return FALSE;
}

/**********************************************************************//**
  Set the "hand" cursor when moving over a link.
**************************************************************************/
static void set_cursor_if_appropriate(GtkTextView *text_view, gint x, gint y)
{
  static gboolean hovering_over_link = FALSE;
  static GdkCursor *hand_cursor = NULL;
  static GdkCursor *regular_cursor = NULL;
  GSList *tags, *tagp;
  GtkTextIter iter;
  gboolean hovering = FALSE;

  /* Initialize the cursors. */
  if (!hand_cursor) {
    hand_cursor = gdk_cursor_new_for_display(
        gtk_widget_get_display(GTK_WIDGET(text_view)), GDK_HAND2);
  }
  if (!regular_cursor) {
    regular_cursor = gdk_cursor_new_for_display(
        gtk_widget_get_display(GTK_WIDGET(text_view)), GDK_XTERM);
  }

  gtk_text_view_get_iter_at_location(text_view, &iter, x, y);

  tags = gtk_text_iter_get_tags(&iter);
  for (tagp = tags; tagp; tagp = tagp->next) {
    enum text_link_type type =
      GPOINTER_TO_INT(g_object_get_data(G_OBJECT(tagp->data), "type"));

    if (type != 0) {
      hovering = TRUE;
      break;
    }
  }

  if (hovering != hovering_over_link) {
    hovering_over_link = hovering;

    if (hovering_over_link) {
      gdk_window_set_cursor(gtk_text_view_get_window(text_view,
                                                     GTK_TEXT_WINDOW_TEXT),
                            hand_cursor);
    } else {
      gdk_window_set_cursor(gtk_text_view_get_window(text_view,
                                                     GTK_TEXT_WINDOW_TEXT),
                            regular_cursor);
    }
  }

  if (tags) {
    g_slist_free(tags);
  }
}

/**********************************************************************//**
  Maybe are the mouse is moving over a link.
**************************************************************************/
static gboolean motion_notify_event(GtkWidget *text_view,
                                    GdkEventMotion *event)
{
  gint x, y;

  gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(text_view), 
                                        GTK_TEXT_WINDOW_WIDGET,
                                        event->x, event->y, &x, &y);
  set_cursor_if_appropriate(GTK_TEXT_VIEW(text_view), x, y);

  return FALSE;
}

/**********************************************************************//**
  Set the appropriate callbacks for the message buffer.
**************************************************************************/
void set_message_buffer_view_link_handlers(GtkWidget *view)
{
  g_signal_connect(view, "event-after",
		   G_CALLBACK(event_after), NULL);
  g_signal_connect(view, "motion-notify-event",
		   G_CALLBACK(motion_notify_event), NULL);
}

/**********************************************************************//**
  Convert a struct text_tag to a GtkTextTag.
**************************************************************************/
void apply_text_tag(const struct text_tag *ptag, GtkTextBuffer *buf,
                    ft_offset_t text_start_offset, const char *text)
{
  static bool initalized = FALSE;
  GtkTextIter start, stop;

  if (!initalized) {
    gtk_text_buffer_create_tag(buf, "bold",
                               "weight", PANGO_WEIGHT_BOLD, NULL);
    gtk_text_buffer_create_tag(buf, "italic",
                               "style", PANGO_STYLE_ITALIC, NULL);
    gtk_text_buffer_create_tag(buf, "strike",
                               "strikethrough", TRUE, NULL);
    gtk_text_buffer_create_tag(buf, "underline",
                               "underline", PANGO_UNDERLINE_SINGLE, NULL);
    initalized = TRUE;
  }

  /* Get the position. */
  /*
   * N.B.: text_tag_*_offset() value is in bytes, so we need to convert it
   * to utf8 character offset.
   */
  gtk_text_buffer_get_iter_at_offset(buf, &start, text_start_offset
                                     + g_utf8_pointer_to_offset(text,
                                     text + text_tag_start_offset(ptag)));
  if (text_tag_stop_offset(ptag) == FT_OFFSET_UNSET) {
    gtk_text_buffer_get_end_iter(buf, &stop);
  } else {
    gtk_text_buffer_get_iter_at_offset(buf, &stop, text_start_offset
                                       + g_utf8_pointer_to_offset(text,
                                       text + text_tag_stop_offset(ptag)));
  }

  switch (text_tag_type(ptag)) {
  case TTT_BOLD:
    gtk_text_buffer_apply_tag_by_name(buf, "bold", &start, &stop);
    break;
  case TTT_ITALIC:
    gtk_text_buffer_apply_tag_by_name(buf, "italic", &start, &stop);
    break;
  case TTT_STRIKE:
    gtk_text_buffer_apply_tag_by_name(buf, "strike", &start, &stop);
    break;
  case TTT_UNDERLINE:
    gtk_text_buffer_apply_tag_by_name(buf, "underline", &start, &stop);
    break;
  case TTT_COLOR:
    {
      /* We have to make a new tag every time. */
      GtkTextTag *tag = NULL;
      const char *foreground = text_tag_color_foreground(ptag);
      const char *background = text_tag_color_background(ptag);

      if (foreground && foreground[0]) {
        if (background && background[0]) {
          tag = gtk_text_buffer_create_tag(buf, NULL,
                                           "foreground", foreground,
                                           "background", background,
                                           NULL);
        } else {
          tag = gtk_text_buffer_create_tag(buf, NULL,
                                           "foreground", foreground,
                                           NULL);
        }
      } else if (background && background[0]) {
        tag = gtk_text_buffer_create_tag(buf, NULL,
                                         "background", background,
                                         NULL);
      }

      if (!tag) {
        break; /* No color. */
      }
      gtk_text_buffer_apply_tag(buf, tag, &start, &stop);
      g_object_unref(G_OBJECT(tag));
    }
    break;
  case TTT_LINK:
    {
      struct color *pcolor = NULL;
      GtkTextTag *tag;

      switch (text_tag_link_type(ptag)) {
      case TLT_CITY:
        pcolor = get_color(tileset, COLOR_MAPVIEW_CITY_LINK);
        break;
      case TLT_TILE:
        pcolor = get_color(tileset, COLOR_MAPVIEW_TILE_LINK);
        break;
      case TLT_UNIT:
        pcolor = get_color(tileset, COLOR_MAPVIEW_UNIT_LINK);
        break;
      }

      if (!pcolor) {
        break; /* Not a valid link type case. */
      }

      tag = gtk_text_buffer_create_tag(buf, NULL, 
                                       "foreground-rgba", &pcolor->color,
                                       "underline", PANGO_UNDERLINE_SINGLE,
                                       NULL);

      /* Type 0 is reserved for non-link tags.  So, add 1 to the
       * type value. */
      g_object_set_data(G_OBJECT(tag), "type",
                        GINT_TO_POINTER(text_tag_link_type(ptag) + 1));
      g_object_set_data(G_OBJECT(tag), "id",
                        GINT_TO_POINTER(text_tag_link_id(ptag)));
      gtk_text_buffer_apply_tag(buf, tag, &start, &stop);
      g_object_unref(G_OBJECT(tag));
      break;
    }
  }
}

/**********************************************************************//**
  Appends the string to the chat output window.  The string should be
  inserted on its own line, although it will have no newline.
**************************************************************************/
void real_output_window_append(const char *astring,
                               const struct text_tag_list *tags,
                               int conn_id)
{
  GtkTextBuffer *buf;
  GtkTextIter iter;
  GtkTextMark *mark;
  ft_offset_t text_start_offset;

  buf = message_buffer;

  if (buf == NULL) {
    log_error("Output when no message buffer: %s", astring);

    return;
  }

  gtk_text_buffer_get_end_iter(buf, &iter);
  gtk_text_buffer_insert(buf, &iter, "\n", -1);
  mark = gtk_text_buffer_create_mark(buf, NULL, &iter, TRUE);

  if (GUI_GTK_OPTION(show_chat_message_time)) {
    char timebuf[64];
    time_t now;
    struct tm *now_tm;

    now = time(NULL);
    now_tm = localtime(&now);
    strftime(timebuf, sizeof(timebuf), "[%H:%M:%S] ", now_tm);
    gtk_text_buffer_insert(buf, &iter, timebuf, -1);
  }

  text_start_offset = gtk_text_iter_get_offset(&iter);
  gtk_text_buffer_insert(buf, &iter, astring, -1);
  text_tag_list_iterate(tags, ptag) {
    apply_text_tag(ptag, buf, text_start_offset, astring);
  } text_tag_list_iterate_end;

  if (main_message_area) {
    scroll_if_necessary(GTK_TEXT_VIEW(main_message_area), mark);
  }
  if (start_message_area) {
    scroll_if_necessary(GTK_TEXT_VIEW(start_message_area), mark);
  }
  gtk_text_buffer_delete_mark(buf, mark);

  append_network_statusbar(astring, FALSE);
}

/**********************************************************************//**
  I have no idea what module this belongs in -- Syela
  I've decided to put output_window routines in chatline.c, because
  the are somewhat related and output_window_* is already here.  --dwp
**************************************************************************/
void log_output_window(void)
{
  GtkTextIter start, end;
  gchar *txt;

  gtk_text_buffer_get_bounds(message_buffer, &start, &end);
  txt = gtk_text_buffer_get_text(message_buffer, &start, &end, TRUE);

  write_chatline_content(txt);
  g_free(txt);
}

/**********************************************************************//**
  Clear output window. This does *not* destroy it, or free its resources
**************************************************************************/
void clear_output_window(void)
{
  set_output_window_text(_("Cleared output window."));
}

/**********************************************************************//**
  Set given text to output window
**************************************************************************/
void set_output_window_text(const char *text)
{
  gtk_text_buffer_set_text(message_buffer, text, -1);
}

/**********************************************************************//**
  Returns whether the chatline is scrolled to the bottom.
**************************************************************************/
bool chatline_is_scrolled_to_bottom(void)
{
  GtkWidget *sw, *w;
  GtkAdjustment *vadj;
  gdouble val, max, upper, page_size;

  if (get_client_page() == PAGE_GAME) {
    w = GTK_WIDGET(main_message_area);
  } else {
    w = GTK_WIDGET(start_message_area);
  }

  if (w == NULL) {
    return TRUE;
  }

  sw = gtk_widget_get_parent(w);
  vadj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(sw));
  val = gtk_adjustment_get_value(vadj);
  g_object_get(G_OBJECT(vadj), "upper", &upper,
               "page-size", &page_size, NULL);
  max = upper - page_size;

  /* Approximation. */
  return max - val < 0.00000001;
}

/**********************************************************************//**
  Scrolls the pregame and in-game chat windows all the way to the bottom.

  Why do we do it in such a convuluted fasion rather than calling
  chatline_scroll_to_bottom directly from toplevel_configure?
  Because the widget is not at its final size yet when the configure
  event occurs.
**************************************************************************/
static gboolean chatline_scroll_callback(gpointer data)
{
  chatline_scroll_to_bottom(FALSE);     /* Not delayed this time! */

  *((guint *) data) = 0;
  return FALSE;         /* Remove this idle function. */
}

/**********************************************************************//**
  Scrolls the pregame and in-game chat windows all the way to the bottom.
  If delayed is TRUE, it will be done in a idle_callback.
**************************************************************************/
void chatline_scroll_to_bottom(bool delayed)
{
  static guint callback_id = 0;

  if (delayed) {
    if (callback_id == 0) {
      callback_id = g_idle_add(chatline_scroll_callback, &callback_id);
    }
  } else if (message_buffer) {
    GtkTextIter end;

    gtk_text_buffer_get_end_iter(message_buffer, &end);

    if (main_message_area) {
      gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(main_message_area),
                                   &end, 0.0, TRUE, 1.0, 0.0);
    }
    if (start_message_area) {
      gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(start_message_area),
                                   &end, 0.0, TRUE, 1.0, 0.0);
    }
  }
}

/**********************************************************************//**
  Tool button clicked.
**************************************************************************/
static void make_tag_callback(GtkToolButton *button, gpointer data)
{
  inputline_make_tag(GTK_ENTRY(data),
                     GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button),
                                                       "text_tag_type")));
}

/**********************************************************************//**
  Set the color for an object.  Update the button if not NULL.
**************************************************************************/
static void color_set(GObject *object, const gchar *color_target,
                      GdkRGBA *color, GtkToolButton *button)
{
  GdkRGBA *current_color = g_object_get_data(object, color_target);

  if (NULL == color) {
    /* Clears the current color. */
    if (NULL != current_color) {
      gdk_rgba_free(current_color);
      g_object_set_data(object, color_target, NULL);
      if (NULL != button) {
        gtk_tool_button_set_icon_widget(button, NULL);
      }
    }
  } else {
    /* Apply the new color. */
    if (NULL != current_color) {
      /* We already have a GdkRGBA pointer. */
      *current_color = *color;
    } else {
      /* We need to make a GdkRGBA pointer. */
      current_color = gdk_rgba_copy(color);
      g_object_set_data(object, color_target, current_color);
    }

    if (NULL != button) {
      /* Update the button. */
      GdkPixbuf *pixbuf;
      GtkWidget *image;

      {
        cairo_surface_t *surface = cairo_image_surface_create(
            CAIRO_FORMAT_RGB24, 16, 16);
        cairo_t *cr = cairo_create(surface);
        gdk_cairo_set_source_rgba(cr, current_color);
        cairo_paint(cr);
        cairo_destroy(cr);
        pixbuf = gdk_pixbuf_get_from_surface(surface, 0, 0, 16, 16);
        cairo_surface_destroy(surface);
      }
      image = gtk_image_new_from_pixbuf(pixbuf);
      gtk_tool_button_set_icon_widget(button, image);
      gtk_widget_show(image);
      g_object_unref(G_OBJECT(pixbuf));
    }
  }
}

/**********************************************************************//**
  Color selection dialog response.
**************************************************************************/
static void color_selected(GtkDialog *dialog, gint res, gpointer data)
{
  GtkToolButton *button =
    GTK_TOOL_BUTTON(g_object_get_data(G_OBJECT(dialog), "button"));
  const gchar *color_target =
    g_object_get_data(G_OBJECT(button), "color_target");

  if (res == GTK_RESPONSE_REJECT) {
    /* Clears the current color. */
    color_set(G_OBJECT(data), color_target, NULL, button);
  } else if (res == GTK_RESPONSE_OK) {
    /* Apply the new color. */
    GtkColorChooser *chooser =
      GTK_COLOR_CHOOSER(g_object_get_data(G_OBJECT(dialog), "chooser"));
    GdkRGBA new_color;

    gtk_color_chooser_get_rgba(chooser, &new_color);
    color_set(G_OBJECT(data), color_target, &new_color, button);
  }

  gtk_widget_destroy(GTK_WIDGET(dialog));
}

/**********************************************************************//**
  Color selection tool button clicked.
**************************************************************************/
static void select_color_callback(GtkToolButton *button, gpointer data)
{
  GtkWidget *dialog, *chooser;
  /* "fg_color" or "bg_color". */
  const gchar *color_target = g_object_get_data(G_OBJECT(button),
                                                "color_target");
  GdkRGBA *current_color = g_object_get_data(G_OBJECT(data), color_target);

  /* TRANS: "text" or "background". */
  gchar *buf = g_strdup_printf(_("Select the %s color"),
              (const char *) g_object_get_data(G_OBJECT(button),
                                               "color_info"));
  dialog = gtk_dialog_new_with_buttons(buf, NULL, GTK_DIALOG_MODAL,
                                       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                       GTK_STOCK_CLEAR, GTK_RESPONSE_REJECT,
                                       GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
  setup_dialog(dialog, toplevel);
  g_object_set_data(G_OBJECT(dialog), "button", button);
  g_signal_connect(dialog, "response", G_CALLBACK(color_selected), data);

  chooser = gtk_color_chooser_widget_new();
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                     chooser, FALSE, FALSE, 0);
  g_object_set_data(G_OBJECT(dialog), "chooser", chooser);
  if (current_color) {
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(chooser), current_color);
  }

  gtk_widget_show_all(dialog);
  g_free(buf);
}

/**********************************************************************//**
  Moves the tool kit to the toolkit view.
**************************************************************************/
static gboolean move_toolkit(GtkWidget *toolkit_view,
                             gpointer data)
{
  struct inputline_toolkit *ptoolkit = (struct inputline_toolkit *) data;
  GtkWidget *parent = gtk_widget_get_parent(ptoolkit->main_widget);
  GtkWidget *button_box = GTK_WIDGET(g_object_get_data(G_OBJECT(toolkit_view),
                                                       "button_box"));
  GList *list, *iter;

  if (parent) {
    if (parent == toolkit_view) {
      return FALSE;     /* Already owned. */
    }

    /* N.B.: We need to hide/show the toolbar to reset the sensitivity
     * of the tool buttons. */
    if (ptoolkit->toolbar_displayed) {
      gtk_widget_hide(ptoolkit->toolbar);
    }
    gtk_widget_reparent(ptoolkit->main_widget, toolkit_view);
    if (ptoolkit->toolbar_displayed) {
      gtk_widget_show(ptoolkit->toolbar);
    }

    if (!gtk_widget_get_parent(button_box)) {
      /* Attach to the toolkit button_box. */
      gtk_container_add(GTK_CONTAINER(ptoolkit->button_box), button_box);
    }
    gtk_widget_show_all(ptoolkit->main_widget);
    if (!ptoolkit->toolbar_displayed) {
      gtk_widget_hide(ptoolkit->toolbar);
    }

    /* Hide all other buttons boxes. */
    list = gtk_container_get_children(GTK_CONTAINER(ptoolkit->button_box));
    for (iter = list; iter != NULL; iter = g_list_next(iter)) {
      GtkWidget *widget = GTK_WIDGET(iter->data);

      if (widget != button_box) {
        gtk_widget_hide(widget);
      }
    }
    g_list_free(list);

  } else {
    /* First time attached to a parent. */
    gtk_container_add(GTK_CONTAINER(toolkit_view), ptoolkit->main_widget);
    gtk_container_add(GTK_CONTAINER(ptoolkit->button_box), button_box);
    gtk_widget_show_all(ptoolkit->main_widget);
  }

  return FALSE;
}

/**********************************************************************//**
  Show/Hide the toolbar.
**************************************************************************/
static gboolean set_toolbar_visibility(GtkWidget *w,
                                       gpointer data)
{
  struct inputline_toolkit *ptoolkit = (struct inputline_toolkit *) data;
  GtkToggleButton *button = GTK_TOGGLE_BUTTON(toolkit.toggle_button);

  if (ptoolkit->toolbar_displayed) {
    if (!gtk_toggle_button_get_active(button)) {
      /* button_toggled() will be called and the toolbar shown. */
      gtk_toggle_button_set_active(button, TRUE);
    } else {
      /* Unsure the widget is visible. */
      gtk_widget_show(ptoolkit->toolbar);
    }
  } else {
    if (gtk_toggle_button_get_active(button)) {
      /* button_toggled() will be called and the toolbar hiden. */
      gtk_toggle_button_set_active(button, FALSE);
    } else {
      /* Unsure the widget is visible. */
      gtk_widget_hide(ptoolkit->toolbar);
    }
  }

  return FALSE;
}

/**********************************************************************//**
  Show/Hide the toolbar.
**************************************************************************/
static void button_toggled(GtkToggleButton *button, gpointer data)
{
  struct inputline_toolkit *ptoolkit = (struct inputline_toolkit *) data;

  if (gtk_toggle_button_get_active(button)) {
    gtk_widget_show(ptoolkit->toolbar);
    ptoolkit->toolbar_displayed = TRUE;
    if (chatline_is_scrolled_to_bottom()) {
      /* Make sure to be still at the end. */
      chatline_scroll_to_bottom(TRUE);
    }
  } else {
    gtk_widget_hide(ptoolkit->toolbar);
    ptoolkit->toolbar_displayed = FALSE;
  }
}

/**********************************************************************//**
  Returns a new inputline toolkit view widget that can contain the
  inputline.

  This widget has the following datas:
  "button_box": pointer to the GtkHBox where to append buttons.
**************************************************************************/
GtkWidget *inputline_toolkit_view_new(void)
{
  GtkWidget *toolkit_view, *bbox;

  /* Main widget. */
  toolkit_view = gtk_grid_new();
  gtk_orientable_set_orientation(GTK_ORIENTABLE(toolkit_view),
                                 GTK_ORIENTATION_VERTICAL);
  g_signal_connect_after(toolkit_view, "map",
                   G_CALLBACK(move_toolkit), &toolkit);

  /* Button box. */
  bbox = gtk_grid_new();
  gtk_grid_set_column_spacing(GTK_GRID(bbox), 12);
  g_object_set_data(G_OBJECT(toolkit_view), "button_box", bbox);

  return toolkit_view;
}

/**********************************************************************//**
  Appends a button to the inputline toolkit view widget.
**************************************************************************/
void inputline_toolkit_view_append_button(GtkWidget *toolkit_view,
                                          GtkWidget *button)
{
  gtk_container_add(GTK_CONTAINER(g_object_get_data(G_OBJECT(toolkit_view),
                    "button_box")), button);
}

/**********************************************************************//**
  Initializes the chatline stuff.
**************************************************************************/
void chatline_init(void)
{
  GtkWidget *vbox, *toolbar, *hbox, *button, *entry, *bbox;
  GtkToolItem *item;
  GdkRGBA color;

  /* Chatline history. */
  if (!history_list) {
    history_list = genlist_new();
    history_pos = -1;
  }

  /* Inputline toolkit. */
  memset(&toolkit, 0, sizeof(toolkit));

  vbox = gtk_grid_new();
  gtk_grid_set_row_spacing(GTK_GRID(vbox), 2);
  gtk_orientable_set_orientation(GTK_ORIENTABLE(vbox),
                                 GTK_ORIENTATION_VERTICAL);
  toolkit.main_widget = vbox;
  g_signal_connect_after(vbox, "map",
                   G_CALLBACK(set_toolbar_visibility), &toolkit);

  entry = gtk_entry_new();
  g_object_set(entry, "margin", 2, NULL);
  gtk_widget_set_hexpand(entry, TRUE);
  toolkit.entry = entry;

  /* First line: toolbar */
  toolbar = gtk_toolbar_new();
  gtk_container_add(GTK_CONTAINER(vbox), toolbar);
  gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar), GTK_ICON_SIZE_MENU);
  gtk_toolbar_set_show_arrow(GTK_TOOLBAR(toolbar), FALSE);
  gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_BOTH_HORIZ);
  gtk_orientable_set_orientation(GTK_ORIENTABLE(toolbar),
                                 GTK_ORIENTATION_HORIZONTAL);
  toolkit.toolbar = toolbar;

  /* Bold button. */
  item = gtk_tool_button_new_from_stock(GTK_STOCK_BOLD);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
  g_object_set_data(G_OBJECT(item), "text_tag_type",
                    GINT_TO_POINTER(TTT_BOLD));
  g_signal_connect(item, "clicked", G_CALLBACK(make_tag_callback), entry);
  gtk_widget_set_tooltip_text(GTK_WIDGET(item), _("Bold (Ctrl-B)"));

  /* Italic button. */
  item = gtk_tool_button_new_from_stock(GTK_STOCK_ITALIC);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
  g_object_set_data(G_OBJECT(item), "text_tag_type",
                    GINT_TO_POINTER(TTT_ITALIC));
  g_signal_connect(item, "clicked", G_CALLBACK(make_tag_callback), entry);
  gtk_widget_set_tooltip_text(GTK_WIDGET(item), _("Italic (Ctrl-I)"));

  /* Strike button. */
  item = gtk_tool_button_new_from_stock(GTK_STOCK_STRIKETHROUGH);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
  g_object_set_data(G_OBJECT(item), "text_tag_type",
                    GINT_TO_POINTER(TTT_STRIKE));
  g_signal_connect(item, "clicked", G_CALLBACK(make_tag_callback), entry);
  gtk_widget_set_tooltip_text(GTK_WIDGET(item), _("Strikethrough (Ctrl-S)"));

  /* Underline button. */
  item = gtk_tool_button_new_from_stock(GTK_STOCK_UNDERLINE);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
  g_object_set_data(G_OBJECT(item), "text_tag_type",
                    GINT_TO_POINTER(TTT_UNDERLINE));
  g_signal_connect(item, "clicked", G_CALLBACK(make_tag_callback), entry);
  gtk_widget_set_tooltip_text(GTK_WIDGET(item), _("Underline (Ctrl-U)"));

  /* Color button. */
  item = gtk_tool_button_new_from_stock(GTK_STOCK_SELECT_COLOR);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
  g_object_set_data(G_OBJECT(item), "text_tag_type",
                    GINT_TO_POINTER(TTT_COLOR));
  g_signal_connect(item, "clicked", G_CALLBACK(make_tag_callback), entry);
  gtk_widget_set_tooltip_text(GTK_WIDGET(item), _("Color (Ctrl-C)"));

  gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
                     gtk_separator_tool_item_new(), -1);

  /* Foreground selector. */
  item = gtk_tool_button_new(NULL, "");
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
  g_object_set_data(G_OBJECT(item), "color_target", fc_strdup("fg_color"));
  g_object_set_data(G_OBJECT(item), "color_info",
                    fc_strdup(_("foreground")));
  g_signal_connect(item, "clicked",
                   G_CALLBACK(select_color_callback), entry);
  gtk_widget_set_tooltip_text(GTK_WIDGET(item), _("Select the text color"));
  if (gdk_rgba_parse(&color, "#000000")) {
    /* Set default foreground color. */
    color_set(G_OBJECT(entry), "fg_color", &color, GTK_TOOL_BUTTON(item));
  } else {
    log_error("Failed to set the default foreground color.");
  }

  /* Background selector. */
  item = gtk_tool_button_new(NULL, "");
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
  g_object_set_data(G_OBJECT(item), "color_target", fc_strdup("bg_color"));
  g_object_set_data(G_OBJECT(item), "color_info",
                    fc_strdup(_("background")));
  g_signal_connect(item, "clicked",
                   G_CALLBACK(select_color_callback), entry);
  gtk_widget_set_tooltip_text(GTK_WIDGET(item),
                              _("Select the background color"));
  if (gdk_rgba_parse(&color, "#ffffff")) {
    /* Set default background color. */
    color_set(G_OBJECT(entry), "bg_color", &color, GTK_TOOL_BUTTON(item));
  } else {
    log_error("Failed to set the default background color.");
  }

  gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
                     gtk_separator_tool_item_new(), -1);

  /* Return button. */
  item = gtk_tool_button_new_from_stock(GTK_STOCK_OK);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
  g_signal_connect_swapped(item, "clicked",
                           G_CALLBACK(inputline_return), entry);
  gtk_widget_set_tooltip_text(GTK_WIDGET(item),
                              /* TRANS: "Return" means the return key. */
                              _("Send the chat (Return)"));

  /* Second line */
  hbox = gtk_grid_new();
  gtk_grid_set_column_spacing(GTK_GRID(hbox), 4);
  gtk_container_add(GTK_CONTAINER(vbox), hbox);

  /* Toggle button. */
  button = gtk_toggle_button_new();
  g_object_set(button, "margin", 2, NULL);
  gtk_container_add(GTK_CONTAINER(hbox), button);
  gtk_button_set_image(GTK_BUTTON(button),
                       gtk_image_new_from_stock(GTK_STOCK_EDIT,
                                                GTK_ICON_SIZE_MENU));
  g_signal_connect(button, "toggled", G_CALLBACK(button_toggled), &toolkit);
  gtk_widget_set_tooltip_text(GTK_WIDGET(button), _("Chat tools"));
  toolkit.toggle_button = button;

  /* Entry. */
  gtk_container_add(GTK_CONTAINER(hbox), entry);
  g_signal_connect(entry, "activate", G_CALLBACK(inputline_return), NULL);
  g_signal_connect(entry, "key_press_event",
                   G_CALLBACK(inputline_handler), NULL);

  /* Button box. */
  bbox = gtk_grid_new();
  gtk_container_add(GTK_CONTAINER(hbox), bbox);
  toolkit.button_box = bbox;
}

/**********************************************************************//**
  Main thread side callback to print version message
**************************************************************************/
static gboolean version_message_main_thread(gpointer user_data)
{
  char *vertext = (char *)user_data;

  output_window_append(ftc_client, vertext);

  FC_FREE(vertext);

  return G_SOURCE_REMOVE;
}

/**********************************************************************//**
  Got version message from metaserver thread.
**************************************************************************/
void version_message(const char *vertext)
{
  int len = strlen(vertext) + 1;
  char *persistent = fc_malloc(len);

  strncpy(persistent, vertext, len);

  gdk_threads_add_idle(version_message_main_thread, persistent);
}
