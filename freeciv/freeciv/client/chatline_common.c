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

#include <stdarg.h>
#include <string.h>

/* utility */
#include "astring.h"
#include "fcintl.h"
#include "fcthread.h"
#include "fc_utf8.h"
#include "log.h"

/* common */
#include "featured_text.h"
#include "packets.h"

/* include */
#include "chatline_g.h"

/* client */
#include "client_main.h"
#include "options.h"

#include "chatline_common.h"

static fc_mutex ow_mutex;

/**********************************************************************//**
  Send the message as a chat to the server.
**************************************************************************/
int send_chat(const char *message)
{
  return dsend_packet_chat_msg_req(&client.conn, message);
}

/**********************************************************************//**
  Send the message as a chat to the server. Message is constructed
  in printf style.
**************************************************************************/
int send_chat_printf(const char *format, ...)
{
  struct packet_chat_msg_req packet;
  va_list args;

  va_start(args, format);
  fc_utf8_vsnprintf_trunc(packet.message, sizeof(packet.message),
                          format, args);
  va_end(args);

  return send_packet_chat_msg_req(&client.conn, &packet);
}

/**********************************************************************//**
  Allocate output window mutex
**************************************************************************/
void fc_allocate_ow_mutex(void)
{
  fc_allocate_mutex(&ow_mutex);
}

/**********************************************************************//**
  Release output window mutex
**************************************************************************/
void fc_release_ow_mutex(void)
{
  fc_release_mutex(&ow_mutex);
}

/**********************************************************************//**
  Initialize output window mutex
**************************************************************************/
void fc_init_ow_mutex(void)
{
  fc_init_mutex(&ow_mutex);
}

/**********************************************************************//**
  Destroy output window mutex
**************************************************************************/
void fc_destroy_ow_mutex(void)
{
  fc_destroy_mutex(&ow_mutex);
}

/**********************************************************************//**
  Add a line of text to the output ("chatline") window, like puts() would
  do it in the console.
**************************************************************************/
void output_window_append(const struct ft_color color,
                          const char *featured_text)
{
  char plain_text[MAX_LEN_MSG];
  struct text_tag_list *tags;

  /* Separate the text and the tags. */
  featured_text_to_plain_text(featured_text, plain_text,
                              sizeof(plain_text), &tags, FALSE);

  if (ft_color_requested(color)) {
    /* A color is requested. */
    struct text_tag *ptag = text_tag_new(TTT_COLOR, 0, FT_OFFSET_UNSET,
                                         color);

    if (ptag) {
      /* Prepends to the list, to avoid to overwrite inside colors. */
      text_tag_list_prepend(tags, ptag);
    } else {
      log_error("Failed to create a color text tag (fg = %s, bg = %s).",
                (NULL != color.foreground ? color.foreground : "NULL"),
                (NULL != color.background ? color.background : "NULL"));
    }
  }

  fc_allocate_ow_mutex();
  real_output_window_append(plain_text, tags, -1);
  fc_release_ow_mutex();
  text_tag_list_destroy(tags);
}

/**********************************************************************//**
  Add a line of text to the output ("chatline") window.  The text is
  constructed in printf style.
**************************************************************************/
void output_window_vprintf(const struct ft_color color,
                           const char *format, va_list args)
{
  char featured_text[MAX_LEN_MSG];

  fc_vsnprintf(featured_text, sizeof(featured_text), format, args);
  output_window_append(color, featured_text);
}


/**********************************************************************//**
  Add a line of text to the output ("chatline") window.  The text is
  constructed in printf style.
**************************************************************************/
void output_window_printf(const struct ft_color color,
                          const char *format, ...)
{
  va_list args;

  va_start(args, format);
  output_window_vprintf(color, format, args);
  va_end(args);
}

/**********************************************************************//**
  Add a line of text to the output ("chatline") window from server event.
**************************************************************************/
void output_window_event(const char *plain_text,
                         const struct text_tag_list *tags, int conn_id)
{
  fc_allocate_ow_mutex();
  real_output_window_append(plain_text, tags, conn_id);
  fc_release_ow_mutex();
}

/**********************************************************************//**
  Standard welcome message.
**************************************************************************/
void chat_welcome_message(bool gui_has_copying_mitem)
{
  output_window_append(ftc_any, _("Freeciv is free software and you are "
                                  "welcome to distribute copies of it "
                                  "under certain conditions;"));
  if (gui_has_copying_mitem) {
    output_window_append(ftc_any, _("See the \"Copying\" item on the "
                                    "Help menu."));
  } else {
    output_window_append(ftc_any, _("See COPYING file distributed with "
                                    "this program."));
  }
  output_window_append(ftc_any, _("Now ... Go give 'em hell!"));
}

/**********************************************************************//**
  Writes the supplied string into the file defined by the variable
  'default_chat_logfile'.
**************************************************************************/
void write_chatline_content(const char *txt)
{
  FILE *fp = fc_fopen(gui_options.default_chat_logfile, "w");
  char buf[512];

  fc_snprintf(buf, sizeof(buf), _("Exporting output window to '%s' ..."),
              gui_options.default_chat_logfile);
  output_window_append(ftc_client, buf);

  if (fp) {
    fputs(txt, fp);
    fclose(fp);
    output_window_append(ftc_client, _("Export complete."));
  } else {
    output_window_append(ftc_client,
                         _("Export failed, couldn't write to file."));
  }
}
