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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

/* utility */
#include "shared.h"
#include "support.h"

/* common */
#include "events.h"
#include "fcintl.h"
#include "government.h"
#include "packets.h"
#include "player.h"

/* client */
#include "client_main.h"
#include "options.h"

#include "chatline.h"
#include "cityrep.h"
#include "dialogs.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "ratesdlg.h"
#include "optiondlg.h"

/******************************************************************/
static GtkWidget *rates_dialog_shell;
static GtkWidget *rates_gov_label;
static GtkWidget *rates_tax_toggle, *rates_lux_toggle, *rates_sci_toggle;
static GtkWidget *rates_tax_label, *rates_lux_label, *rates_sci_label;
static GtkObject *rates_tax_adj, *rates_lux_adj, *rates_sci_adj;

static gulong     rates_tax_sig, rates_lux_sig, rates_sci_sig;
/******************************************************************/

static int rates_tax_value, rates_lux_value, rates_sci_value;


static void rates_changed_callback(GtkAdjustment *adj);


/**************************************************************************
...
**************************************************************************/
static void rates_set_values(int tax, int no_tax_scroll, 
			     int lux, int no_lux_scroll,
			     int sci, int no_sci_scroll)
{
  char buf[64];
  int tax_lock, lux_lock, sci_lock;
  int maxrate;
  
  tax_lock	= GTK_TOGGLE_BUTTON(rates_tax_toggle)->active;
  lux_lock	= GTK_TOGGLE_BUTTON(rates_lux_toggle)->active;
  sci_lock	= GTK_TOGGLE_BUTTON(rates_sci_toggle)->active;

  if (NULL != client.conn.playing) {
    maxrate = get_player_bonus(client.conn.playing, EFT_MAX_RATES);
  } else {
    maxrate = 100;
  }
  /* This's quite a simple-minded "double check".. */
  tax=MIN(tax, maxrate);
  lux=MIN(lux, maxrate);
  sci=MIN(sci, maxrate);
  
  if(tax+sci+lux!=100)
  {
    if((tax!=rates_tax_value))
    {
      if(!lux_lock)
	lux=MIN(MAX(100-tax-sci, 0), maxrate);
      if(!sci_lock)
	sci=MIN(MAX(100-tax-lux, 0), maxrate);
    }
    else if((lux!=rates_lux_value))
    {
      if(!tax_lock)
	tax=MIN(MAX(100-lux-sci, 0), maxrate);
      if(!sci_lock)
	sci=MIN(MAX(100-lux-tax, 0), maxrate);
    }
    else if((sci!=rates_sci_value))
    {
      if(!lux_lock)
	lux=MIN(MAX(100-tax-sci, 0), maxrate);
      if(!tax_lock)
	tax=MIN(MAX(100-lux-sci, 0), maxrate);
    }
    
    if(tax+sci+lux!=100) {
      tax=rates_tax_value;
      lux=rates_lux_value;
      sci=rates_sci_value;

      rates_tax_value=-1;
      rates_lux_value=-1;
      rates_sci_value=-1;

      no_tax_scroll=0;
      no_lux_scroll=0;
      no_sci_scroll=0;
    }

  }

  if(tax!=rates_tax_value) {
    my_snprintf(buf, sizeof(buf), "%3d%%", tax);
    if (strcmp(buf, GTK_LABEL(rates_tax_label)->label) != 0)
	gtk_label_set_text(GTK_LABEL(rates_tax_label), buf);
    if(!no_tax_scroll)
    {
	g_signal_handler_block(rates_tax_adj, rates_tax_sig);
	gtk_adjustment_set_value(GTK_ADJUSTMENT(rates_tax_adj), tax/10 );
	g_signal_handler_unblock(rates_tax_adj, rates_tax_sig);
    }
    rates_tax_value=tax;
  }

  if(lux!=rates_lux_value) {
    my_snprintf(buf, sizeof(buf), "%3d%%", lux);
    if (strcmp(buf, GTK_LABEL(rates_lux_label)->label) != 0)
	gtk_label_set_text(GTK_LABEL(rates_lux_label), buf);
    if(!no_lux_scroll)
    {
	g_signal_handler_block(rates_lux_adj, rates_lux_sig);
	gtk_adjustment_set_value(GTK_ADJUSTMENT(rates_lux_adj), lux/10 );
	g_signal_handler_unblock(rates_lux_adj, rates_lux_sig);
    }
    rates_lux_value=lux;
  }

  if(sci!=rates_sci_value) {
    my_snprintf(buf, sizeof(buf), "%3d%%", sci);
    if (strcmp(buf, GTK_LABEL(rates_sci_label)->label) != 0)
	gtk_label_set_text(GTK_LABEL(rates_sci_label),buf);
    if(!no_sci_scroll)
    {
	g_signal_handler_block(rates_sci_adj, rates_sci_sig);
	gtk_adjustment_set_value(GTK_ADJUSTMENT(rates_sci_adj), sci/10 );
	g_signal_handler_unblock(rates_sci_adj, rates_sci_sig);
    }
    rates_sci_value=sci;
  }
}


/**************************************************************************
...
**************************************************************************/
static void rates_changed_callback(GtkAdjustment *adj)
{
  int percent=adj->value;

  if(adj==GTK_ADJUSTMENT(rates_tax_adj)) {
    int tax_value;

    tax_value=10*percent;
    tax_value=MIN(tax_value, 100);
    rates_set_values(tax_value,1, rates_lux_value,0, rates_sci_value,0);
  }
  else if(adj==GTK_ADJUSTMENT(rates_lux_adj)) {
    int lux_value;

    lux_value=10*percent;
    lux_value=MIN(lux_value, 100);
    rates_set_values(rates_tax_value,0, lux_value,1, rates_sci_value,0);
  }
  else {
    int sci_value;

    sci_value=10*percent;
    sci_value=MIN(sci_value, 100);
    rates_set_values(rates_tax_value,0, rates_lux_value,0, sci_value,1);
  }
}


/**************************************************************************
...
**************************************************************************/
static void rates_command_callback(GtkWidget *w, gint response_id)
{
  if (response_id == GTK_RESPONSE_OK) {
    dsend_packet_player_rates(&client.conn, rates_tax_value, rates_lux_value,
			      rates_sci_value);
  }
  gtk_widget_destroy(rates_dialog_shell);
}


/****************************************************************
... 
*****************************************************************/
static GtkWidget *create_rates_dialog(void)
{
  GtkWidget     *shell;
  GtkWidget	*frame, *hbox;

  GtkWidget	*scale;

  if (!can_client_issue_orders()) {
    return NULL;
  }
  
  shell = gtk_dialog_new_with_buttons(_("Select tax, luxury and science rates"),
  	NULL,
	0,
	GTK_STOCK_CANCEL,
	GTK_RESPONSE_CANCEL,
	GTK_STOCK_OK,
	GTK_RESPONSE_OK,
	NULL);
  setup_dialog(shell, toplevel);
  gtk_dialog_set_default_response(GTK_DIALOG(shell), GTK_RESPONSE_OK);
  gtk_window_set_position(GTK_WINDOW(shell), GTK_WIN_POS_MOUSE);

  rates_gov_label = gtk_label_new("");
  gtk_box_pack_start( GTK_BOX( GTK_DIALOG( shell )->vbox ), rates_gov_label, TRUE, TRUE, 5 );

  frame = gtk_frame_new( _("Tax") );
  gtk_box_pack_start( GTK_BOX( GTK_DIALOG( shell )->vbox ), frame, TRUE, TRUE, 5 );

  hbox = gtk_hbox_new( FALSE, 10 );
  gtk_container_add( GTK_CONTAINER( frame ), hbox );

  rates_tax_adj = gtk_adjustment_new( 0.0, 0.0, 11.0, 1.0, 1.0, 1.0 );
  scale = gtk_hscale_new( GTK_ADJUSTMENT( rates_tax_adj ) );
  gtk_widget_set_size_request(scale, 300, 40);
  gtk_scale_set_digits( GTK_SCALE( scale ), 0 );
  gtk_scale_set_draw_value( GTK_SCALE( scale ), FALSE );
  gtk_box_pack_start( GTK_BOX( hbox ), scale, TRUE, TRUE, 0 );

  rates_tax_label = gtk_label_new("  0%");
  gtk_box_pack_start( GTK_BOX( hbox ), rates_tax_label, TRUE, TRUE, 0 );
  gtk_widget_set_size_request(rates_tax_label, 40, -1);

  rates_tax_toggle = gtk_check_button_new_with_label( _("Lock") );
  gtk_box_pack_start( GTK_BOX( hbox ), rates_tax_toggle, TRUE, TRUE, 0 );

  frame = gtk_frame_new( _("Luxury") );
  gtk_box_pack_start( GTK_BOX( GTK_DIALOG( shell )->vbox ), frame, TRUE, TRUE, 5 );

  hbox = gtk_hbox_new( FALSE, 10 );
  gtk_container_add( GTK_CONTAINER( frame ), hbox );

  rates_lux_adj = gtk_adjustment_new( 0.0, 0.0, 11.0, 1.0, 1.0, 1.0 );
  scale = gtk_hscale_new( GTK_ADJUSTMENT( rates_lux_adj ) );
  gtk_widget_set_size_request(scale, 300, 40);
  gtk_scale_set_digits( GTK_SCALE( scale ), 0 );
  gtk_scale_set_draw_value( GTK_SCALE( scale ), FALSE );
  gtk_box_pack_start( GTK_BOX( hbox ), scale, TRUE, TRUE, 0 );

  rates_lux_label = gtk_label_new("  0%");
  gtk_box_pack_start( GTK_BOX( hbox ), rates_lux_label, TRUE, TRUE, 0 );
  gtk_widget_set_size_request(rates_lux_label, 40, -1);

  rates_lux_toggle = gtk_check_button_new_with_label( _("Lock") );
  gtk_box_pack_start( GTK_BOX( hbox ), rates_lux_toggle, TRUE, TRUE, 0 );

  frame = gtk_frame_new( _("Science") );
  gtk_box_pack_start( GTK_BOX( GTK_DIALOG( shell )->vbox ), frame, TRUE, TRUE, 5 );

  hbox = gtk_hbox_new( FALSE, 10 );
  gtk_container_add( GTK_CONTAINER( frame ), hbox );

  rates_sci_adj = gtk_adjustment_new( 0.0, 0.0, 11.0, 1.0, 1.0, 1.0 );
  scale = gtk_hscale_new( GTK_ADJUSTMENT( rates_sci_adj ) );
  gtk_widget_set_size_request(scale, 300, 40);
  gtk_scale_set_digits( GTK_SCALE( scale ), 0 );
  gtk_scale_set_draw_value( GTK_SCALE( scale ), FALSE );
  gtk_box_pack_start( GTK_BOX( hbox ), scale, TRUE, TRUE, 0 );

  rates_sci_label = gtk_label_new("  0%");
  gtk_box_pack_start( GTK_BOX( hbox ), rates_sci_label, TRUE, TRUE, 0 );
  gtk_widget_set_size_request(rates_sci_label, 40, -1);

  rates_sci_toggle = gtk_check_button_new_with_label( _("Lock") );
  gtk_box_pack_start( GTK_BOX( hbox ), rates_sci_toggle, TRUE, TRUE, 0 );


  g_signal_connect(shell, "response",
		   G_CALLBACK(rates_command_callback), NULL);
  g_signal_connect(shell, "destroy",
		   G_CALLBACK(gtk_widget_destroyed), &rates_dialog_shell);

  gtk_widget_show_all( GTK_DIALOG( shell )->vbox );
  gtk_widget_show_all( GTK_DIALOG( shell )->action_area );

  rates_tax_value=-1;
  rates_lux_value=-1;
  rates_sci_value=-1;

  rates_tax_sig =
    g_signal_connect_after(rates_tax_adj, "value_changed",
			   G_CALLBACK(rates_changed_callback), NULL);

  rates_lux_sig =
    g_signal_connect_after(rates_lux_adj, "value_changed",
			   G_CALLBACK(rates_changed_callback), NULL);

  rates_sci_sig =
    g_signal_connect_after(rates_sci_adj, "value_changed",
			   G_CALLBACK(rates_changed_callback), NULL);

  rates_set_values(client.conn.playing->economic.tax, 0,
		   client.conn.playing->economic.luxury, 0,
		   client.conn.playing->economic.science, 0);
  return shell;
}


/****************************************************************
... 
*****************************************************************/
void popup_rates_dialog(void)
{
  char buf[64];

  if (!can_client_issue_orders()) {
    return;
  }

  if (!rates_dialog_shell) {
    rates_dialog_shell = create_rates_dialog();
  }
  if (!rates_dialog_shell) {
    return;
  }

  my_snprintf(buf, sizeof(buf), _("%s max rate: %d%%"),
      government_name_for_player(client.conn.playing),
      get_player_bonus(client.conn.playing, EFT_MAX_RATES));
  gtk_label_set_text(GTK_LABEL(rates_gov_label), buf);
  
  gtk_window_present(GTK_WINDOW(rates_dialog_shell));
}


/**************************************************************************
  Option dialog 
**************************************************************************/

static GtkWidget *option_dialog_shell;

/**************************************************************************
...
**************************************************************************/
static void option_command_processing(void)
{
  const char *dp;
  bool b;
  int val;

    client_options_iterate(o) {
      switch (o->type) {
      case COT_BOOLEAN:
	b = *(o->boolean.pvalue);
	*(o->boolean.pvalue) = GTK_TOGGLE_BUTTON(o->gui_data)->active;
	if (b != *(o->boolean.pvalue) && o->change_callback) {
	  (o->change_callback)(o);
	}
	break;
      case COT_INTEGER:
	val = *(o->integer.pvalue);
	dp = gtk_entry_get_text(GTK_ENTRY(o->gui_data));
	sscanf(dp, "%d", o->integer.pvalue);
	if (val != *(o->integer.pvalue) && o->change_callback) {
	  (o->change_callback)(o);
	}
	break;
      case COT_STRING:
	if (o->string.val_accessor) {
	  const char *new_value =
	    gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(o->gui_data)->entry));
	  if (strcmp(o->string.pvalue, new_value)) {
	    mystrlcpy(o->string.pvalue, new_value, o->string.size);
	    if (o->change_callback) {
	      (o->change_callback)(o);
	    }
	  }
	} else {
	  mystrlcpy(o->string.pvalue,
		    gtk_entry_get_text(GTK_ENTRY(o->gui_data)),
		    o->string.size);
	}
	break;
      case COT_FONT:
	{
	  const char *new_value =
	    gtk_font_button_get_font_name(GTK_FONT_BUTTON(o->gui_data));
	  if (strcmp(o->string.pvalue, new_value)) {
	    mystrlcpy(o->string.pvalue, new_value, o->string.size);
	    gui_update_font_from_option(o);
	  }
	}
        break;
      }
    } client_options_iterate_end;

    if (gui_gtk2_map_scrollbars) {
      gtk_widget_show(map_horizontal_scrollbar);
      gtk_widget_show(map_vertical_scrollbar);
    } else {
      gtk_widget_hide(map_horizontal_scrollbar);
      gtk_widget_hide(map_vertical_scrollbar);
    }
    if (fullscreen_mode) {
      gtk_window_fullscreen(GTK_WINDOW(toplevel));
    } else {
      gtk_window_unfullscreen(GTK_WINDOW(toplevel));
    }

    gtk_rc_reset_styles(gtk_settings_get_default());
}

/**************************************************************************
...
**************************************************************************/
static void option_command_callback(GtkWidget *win, gint rid)
{
  switch (rid) {
  case GTK_RESPONSE_ACCEPT:
    option_command_processing();
    save_options();
    break;
  case GTK_RESPONSE_APPLY:
    option_command_processing();
    break;
  default:
    break;
  };
  gtk_widget_destroy(win);
}

/****************************************************************
... 
*****************************************************************/
static void create_option_dialog(void)
{
  GtkWidget *ebox, *label, *notebook, *align, *vbox[COC_MAX], *sw;
  int i, len[COC_MAX];
  GtkSizeGroup *group[2][COC_MAX];
  GtkTooltips *tips;

  option_dialog_shell = gtk_dialog_new_with_buttons(_("Set local options"),
  	NULL,
	0,
	GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	GTK_STOCK_APPLY, GTK_RESPONSE_APPLY,
	GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
	NULL);
  setup_dialog(option_dialog_shell, toplevel);
  gtk_window_set_position(GTK_WINDOW(option_dialog_shell),
                          GTK_WIN_POS_MOUSE);
  gtk_window_set_default_size(GTK_WINDOW(option_dialog_shell), -1, 400);

  notebook = gtk_notebook_new();
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(option_dialog_shell)->vbox),
                     notebook, TRUE, TRUE, 0);

  for (i = 0; i < COC_MAX; i++) {
    label = gtk_label_new_with_mnemonic(_(client_option_class_names[i]));

    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                   GTK_POLICY_NEVER,
                                   GTK_POLICY_AUTOMATIC);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), sw, label);

    align = gtk_alignment_new(0.0, 0.0, 1.0, 0.0);
    gtk_container_set_border_width(GTK_CONTAINER(align), 8);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(sw), align);

    vbox[i] = gtk_vbox_new(TRUE, 0);
    gtk_container_add(GTK_CONTAINER(align), vbox[i]);

    group[0][i] = gtk_size_group_new(GTK_SIZE_GROUP_BOTH);
    group[1][i] = gtk_size_group_new(GTK_SIZE_GROUP_BOTH);

    len[i] = 0;
  }

  tips = gtk_tooltips_new();

  client_options_iterate(o) {
    GtkWidget *hbox;

    if (o->category == COC_MAX) {
      continue;
    }

    i = len[o->category];

    hbox = gtk_hbox_new(FALSE, 2);
    gtk_container_add(GTK_CONTAINER(vbox[o->category]), hbox);

    ebox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(hbox), ebox);
    gtk_size_group_add_widget(group[0][o->category], ebox);

    label = gtk_label_new(_(o->description));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_container_add(GTK_CONTAINER(ebox), label);

    gtk_tooltips_set_tip(tips, ebox, _(o->help_text), NULL);

    switch (o->type) {
    case COT_BOOLEAN:
      o->gui_data = gtk_check_button_new();
      break;
    case COT_INTEGER:
      o->gui_data = gtk_entry_new();
      gtk_entry_set_max_length(GTK_ENTRY(o->gui_data), 5);
      gtk_widget_set_size_request(GTK_WIDGET(o->gui_data), 45, -1);
      break;
    case COT_STRING:
      if (o->string.val_accessor) {
        o->gui_data = gtk_combo_new();
      } else {
        o->gui_data = gtk_entry_new();
      }
      gtk_widget_set_size_request(GTK_WIDGET(o->gui_data), 150, -1);
      break;
    case COT_FONT:
      o->gui_data = gtk_font_button_new();
      g_object_set(o->gui_data,
	  	   "use-font", TRUE,
		   NULL);
      break;
    }
    gtk_container_add(GTK_CONTAINER(hbox), o->gui_data);
    gtk_size_group_add_widget(group[1][o->category], o->gui_data);

    len[o->category]++;
  } client_options_iterate_end;

  g_signal_connect(option_dialog_shell, "response",
		   G_CALLBACK(option_command_callback), NULL);
  g_signal_connect(option_dialog_shell, "destroy",
		   G_CALLBACK(gtk_widget_destroyed), &option_dialog_shell);

  gtk_widget_show_all(GTK_DIALOG(option_dialog_shell)->vbox);
}

/****************************************************************
... 
*****************************************************************/
void popup_option_dialog(void)
{
  char valstr[64];

  if (!option_dialog_shell) {
    create_option_dialog();
  }

  client_options_iterate(o) {
    switch (o->type) {
    case COT_BOOLEAN:
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(o->gui_data),
				   *(o->boolean.pvalue));
      break;
    case COT_INTEGER:
      my_snprintf(valstr, sizeof(valstr), "%d", *(o->integer.pvalue));
      gtk_entry_set_text(GTK_ENTRY(o->gui_data), valstr);
      break;
    case COT_STRING:
      if (o->string.val_accessor) {
	int i;
	GList *items = NULL;
	const char **vals = (*o->string.val_accessor) ();

	for (i = 0; vals[i]; i++) {
	  if (strcmp(vals[i], o->string.pvalue) == 0) {
	    continue;
	  }
	  items = g_list_prepend(items, (gpointer) vals[i]);
	}
	items = g_list_prepend(items, (gpointer) o->string.pvalue);
	gtk_combo_set_popdown_strings(GTK_COMBO(o->gui_data), items);
      } else {
	gtk_entry_set_text(GTK_ENTRY(o->gui_data), o->string.pvalue);
      }
      break;
    case COT_FONT:
      gtk_font_button_set_font_name(GTK_FONT_BUTTON(o->gui_data),
	  			    o->string.pvalue);
      break;
    }
  } client_options_iterate_end;

  gtk_widget_show(option_dialog_shell);
}
