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

#include <gtk/gtk.h>

/* utility */
#include "fcintl.h"
#include "support.h"

/* client */
#include "voteinfo.h"

/* gui-gtk-2.0 */
#include "chatline.h"
#include "pages.h"

#include "voteinfo_bar.h"

/* A set of widgets. */
struct voteinfo_bar {
  GtkWidget *box;
  GtkWidget *next_button;
  GtkWidget *label;
  GtkWidget *yes_button;
  GtkWidget *no_button;
  GtkWidget *abstain_button;
  GtkWidget *yes_count_label;
  GtkWidget *no_count_label;
  GtkWidget *abstain_count_label;
  GtkWidget *voter_count_label;
};

GtkWidget *pregame_votebar = NULL;      /* PAGE_START voteinfo bar. */
GtkWidget *ingame_votebar = NULL;       /* PAGE_GAME voteinfo bar. */

/**************************************************************************
  Called after a click on a vote button.
**************************************************************************/
static void voteinfo_bar_do_vote_callback(GtkWidget *w, gpointer userdata)
{
  enum client_vote_type vote;
  struct voteinfo *vi;
  
  vote = GPOINTER_TO_INT(userdata);
  vi = voteinfo_queue_get_current(NULL);

  if (vi == NULL) {
    return;
  }

  voteinfo_do_vote(vi->vote_no, vote);
}

/**************************************************************************
  Switch to the next vote.
**************************************************************************/
static void voteinfo_bar_next_callback(GtkWidget *w, gpointer userdata)
{
  voteinfo_queue_next();
  voteinfo_gui_update();
}

/**************************************************************************
  Destroy the voteinfo_bar data structure.
**************************************************************************/
static void voteinfo_bar_destroy(GtkWidget *w, gpointer userdata)
{
  free((struct voteinfo_bar *) userdata);
}

/**************************************************************************
  Create a voteinfo_bar structure.
**************************************************************************/
GtkWidget *voteinfo_bar_new(void)
{
  GtkWidget *label, *button, *hbox, *evbox, *spacer, *arrow;
  struct voteinfo_bar *vib;
  const int BUTTON_HEIGHT = 12;

  vib = fc_calloc(1, sizeof(struct voteinfo_bar));

  hbox = gtk_hbox_new(FALSE, 4);
  g_object_set_data(G_OBJECT(hbox), "voteinfo_bar", vib);
  g_signal_connect(hbox, "destroy", G_CALLBACK(voteinfo_bar_destroy), vib);
  vib->box = hbox;

  label = gtk_label_new("");
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_misc_set_padding(GTK_MISC(label), 8, 4);
  gtk_label_set_max_width_chars(GTK_LABEL(label), 80);
  evbox = gtk_event_box_new();
  gtk_container_add(GTK_CONTAINER(evbox), label);
  gtk_box_pack_start(GTK_BOX(hbox), evbox, TRUE, TRUE, 0);
  gtk_widget_set_name(evbox, "vote label");
  vib->label = label;

  arrow = gtk_image_new_from_stock(GTK_STOCK_MEDIA_REWIND,
                                   GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_misc_set_alignment(GTK_MISC(arrow), 0.5, 0.25);

  button = gtk_button_new();
  g_signal_connect(button, "clicked",
                   G_CALLBACK(voteinfo_bar_next_callback), NULL);
  gtk_button_set_image(GTK_BUTTON(button), arrow);
  gtk_widget_set_size_request(button, -1, BUTTON_HEIGHT);
  gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
  gtk_button_set_focus_on_click(GTK_BUTTON(button), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
  vib->next_button = button;

  spacer = gtk_alignment_new(0, 0, 0, 0);
  gtk_widget_set_size_request(spacer, 16, -1);
  gtk_box_pack_start(GTK_BOX(hbox), spacer, FALSE, FALSE, 0);

  button = gtk_button_new_with_mnemonic(_("_YES"));
  g_signal_connect(button, "clicked",
                   G_CALLBACK(voteinfo_bar_do_vote_callback),
                   GINT_TO_POINTER(CVT_YES));
  gtk_widget_set_size_request(button, 70, BUTTON_HEIGHT);
  gtk_button_set_focus_on_click(GTK_BUTTON(button), FALSE);
  evbox = gtk_event_box_new();
  gtk_container_add(GTK_CONTAINER(evbox), button);
  gtk_box_pack_start(GTK_BOX(hbox), evbox, FALSE, FALSE, 0);
  gtk_widget_set_name(button, "vote yes button");
  gtk_widget_set_name(evbox, "vote yes button");
  gtk_widget_set_name(gtk_bin_get_child(GTK_BIN(button)),
                      "vote yes button");
  vib->yes_button = button;

  label = gtk_label_new("0");
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_widget_set_size_request(label, 24, -1);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  vib->yes_count_label = label;

  button = gtk_button_new_with_mnemonic(_("_NO"));
  g_signal_connect(button, "clicked",
                   G_CALLBACK(voteinfo_bar_do_vote_callback),
                   GINT_TO_POINTER(CVT_NO));
  gtk_widget_set_size_request(button, 70, BUTTON_HEIGHT);
  gtk_button_set_focus_on_click(GTK_BUTTON(button), FALSE);
  evbox = gtk_event_box_new();
  gtk_container_add(GTK_CONTAINER(evbox), button);
  gtk_box_pack_start(GTK_BOX(hbox), evbox, FALSE, FALSE, 0);
  gtk_widget_set_name(button, "vote no button");
  gtk_widget_set_name(evbox, "vote no button");
  gtk_widget_set_name(gtk_bin_get_child(GTK_BIN(button)),
                      "vote no button");
  vib->no_button = button;

  label = gtk_label_new("0");
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_widget_set_size_request(label, 24, -1);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  vib->no_count_label = label;

  button = gtk_button_new_with_mnemonic(_("_ABSTAIN"));
  g_signal_connect(button, "clicked",
                   G_CALLBACK(voteinfo_bar_do_vote_callback),
                   GINT_TO_POINTER(CVT_ABSTAIN));
  gtk_widget_set_size_request(button, 70, BUTTON_HEIGHT);
  gtk_button_set_focus_on_click(GTK_BUTTON(button), FALSE);
  evbox = gtk_event_box_new();
  gtk_container_add(GTK_CONTAINER(evbox), button);
  gtk_box_pack_start(GTK_BOX(hbox), evbox, FALSE, FALSE, 0);
  gtk_widget_set_name(button, "vote abstain button");
  gtk_widget_set_name(evbox, "vote abstain button");
  gtk_widget_set_name(gtk_bin_get_child(GTK_BIN(button)),
                      "vote abstain button");
  vib->abstain_button = button;

  label = gtk_label_new("0");
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_widget_set_size_request(label, 24, -1);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  vib->abstain_count_label = label;

  label = gtk_label_new("/0");
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_widget_set_size_request(label, 24, -1);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  vib->voter_count_label = label;

  return vib->box;
}

/****************************************************************************
  Refresh all vote related GUI widgets. Called by the voteinfo module when
  the client receives new vote information from the server.
****************************************************************************/
void voteinfo_gui_update(void)
{
  int vote_count, index;
  struct voteinfo_bar *vib = NULL;
  struct voteinfo *vi = NULL;
  char buf[512], status[128], ordstr[128], color[32];
  bool running, need_scroll;
  gchar *escaped_desc, *escaped_user;

  if (get_client_page() == PAGE_START && NULL != pregame_votebar) {
    vib = g_object_get_data(G_OBJECT(pregame_votebar), "voteinfo_bar");
  } else if (get_client_page() == PAGE_GAME && NULL != ingame_votebar) {
    vib = g_object_get_data(G_OBJECT(ingame_votebar), "voteinfo_bar");
  }

  if (vib == NULL) {
    return;
  }

  if (voteinfo_queue == NULL) {
    gtk_widget_hide_all(vib->box);
    return;
  }

  vote_count = voteinfo_list_size(voteinfo_queue);
  vi = voteinfo_queue_get_current(&index);

  if (vote_count <= 0 || vi == NULL) {
    gtk_widget_hide_all(vib->box);
    return;
  }

  if (vi != NULL && vi->resolved && vi->passed) {
    /* TRANS: Describing a vote that passed. */
    my_snprintf(status, sizeof(status), _("[passed]"));
    sz_strlcpy(color, "green");
  } else if (vi != NULL && vi->resolved && !vi->passed) {
    /* TRANS: Describing a vote that failed. */
    my_snprintf(status, sizeof(status), _("[failed]"));
    sz_strlcpy(color, "red");
  } else if (vi != NULL && vi->remove_time > 0) {
    /* TRANS: Describing a vote that was removed. */
    my_snprintf(status, sizeof(status), _("[removed]"));
    sz_strlcpy(color, "grey");
  } else {
    status[0] = '\0';
  }

  if (vote_count > 1) {
    my_snprintf(ordstr, sizeof(ordstr),
                "<span weight=\"bold\">(%d/%d)</span> ",
                index + 1, vote_count);
  } else {
    ordstr[0] = '\0';
  }

  if (status[0] != '\0') {
    my_snprintf(buf, sizeof(buf),
        "<span weight=\"bold\" background=\"%s\">%s</span> ",
        color, status);
    sz_strlcpy(status, buf);
  }

  if (vi != NULL) {
    escaped_desc = g_markup_escape_text(vi->desc, -1);
    escaped_user = g_markup_escape_text(vi->user, -1);
    my_snprintf(buf, sizeof(buf), _("%sVote %d by %s: %s%s"),
                ordstr, vi->vote_no, escaped_user, status,
                escaped_desc);
    g_free(escaped_desc);
    g_free(escaped_user);
  } else {
    buf[0] = '\0';
  }
  gtk_label_set_markup(GTK_LABEL(vib->label), buf);

  if (vi != NULL)  {
    my_snprintf(buf, sizeof(buf), "%d", vi->yes);
    gtk_label_set_text(GTK_LABEL(vib->yes_count_label), buf);
    my_snprintf(buf, sizeof(buf), "%d", vi->no);
    gtk_label_set_text(GTK_LABEL(vib->no_count_label), buf);
    my_snprintf(buf, sizeof(buf), "%d", vi->abstain);
    gtk_label_set_text(GTK_LABEL(vib->abstain_count_label), buf);
    my_snprintf(buf, sizeof(buf), "/%d", vi->num_voters);
    gtk_label_set_text(GTK_LABEL(vib->voter_count_label), buf);
  } else {
    gtk_label_set_text(GTK_LABEL(vib->yes_count_label), "-");
    gtk_label_set_text(GTK_LABEL(vib->no_count_label), "-");
    gtk_label_set_text(GTK_LABEL(vib->abstain_count_label), "-");
    gtk_label_set_text(GTK_LABEL(vib->voter_count_label), "/-");
  }

  running = vi != NULL && !vi->resolved && vi->remove_time == 0;

  gtk_widget_set_sensitive(vib->yes_button, running);
  gtk_widget_set_sensitive(vib->no_button, running);
  gtk_widget_set_sensitive(vib->abstain_button, running);

  need_scroll = !GTK_WIDGET_VISIBLE(vib->box)
    && chatline_is_scrolled_to_bottom();

  gtk_widget_show_all(vib->box);

  if (vote_count <= 1) {
    gtk_widget_hide(vib->next_button);
  }

  if (need_scroll) {
    /* Showing the votebar when it was hidden
     * previously makes the chatline scroll up. */
    chatline_scroll_to_bottom(TRUE);
  }
}
