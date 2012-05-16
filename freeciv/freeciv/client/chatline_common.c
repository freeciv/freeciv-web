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

#include <assert.h>
#include <stdarg.h>
#include <string.h>

/* utility */
#include "astring.h"
#include "fcintl.h"
#include "log.h"
#include "support.h"

/* common */
#include "featured_text.h"
#include "packets.h"

/* include */
#include "chatline_g.h"

/* client */
#include "chatline_common.h"
#include "client_main.h"


/* Stored up buffer of lines for the chatline */
struct remaining {
  char *text;
  struct text_tag_list *tags;
  int conn_id;
};
#define SPECLIST_TAG remaining
#include "speclist.h"
#define remaining_list_iterate(rlist, pline) \
  TYPED_LIST_ITERATE(struct remaining, rlist, pline)
#define remaining_list_iterate_end LIST_ITERATE_END

static struct remaining_list *remains;

/**************************************************************************
  Initialize data structures.
**************************************************************************/
void chatline_common_init(void)
{
  remains = remaining_list_new();
}

/**************************************************************************
  Clean up.
**************************************************************************/
void chatline_common_done(void)
{
  remaining_list_free(remains);
}

/**************************************************************************
  Send the message as a chat to the server.
**************************************************************************/
void send_chat(const char *message)
{
  dsend_packet_chat_msg_req(&client.conn, message);
}

/**************************************************************************
  Send the message as a chat to the server. Message is constructed
  in printf style.
**************************************************************************/
void send_chat_printf(const char *format, ...)
{
  char msg[250];
  int maxlen = sizeof(msg);

  va_list ap;
  va_start(ap, format);
  /* FIXME: terminating like this can lead to invalid utf-8, a major no-no. */
  my_vsnprintf(msg, maxlen, format, ap);
  msg[maxlen - 1] = '\0'; /* Make sure there is always ending zero */
  send_chat(msg);
  va_end(ap);
}


static int frozen_level = 0;

/**************************************************************************
  Turn on buffering, using a counter so that calls may be nested.
**************************************************************************/
void output_window_freeze(void)
{
  frozen_level++;

  if (frozen_level == 1) {
    assert(remaining_list_size(remains) == 0);
  }
}

/**************************************************************************
  Turn off buffering if internal counter of number of times buffering
  was turned on falls to zero, to handle nested freeze/thaw pairs.
  When counter is zero, append the picked up data.
**************************************************************************/
void output_window_thaw(void)
{
  frozen_level--;
  assert(frozen_level >= 0);

  if (frozen_level == 0) {
    remaining_list_iterate(remains, pline) {
      real_output_window_append(pline->text, pline->tags, pline->conn_id);
      free(pline->text);
      text_tag_list_clear_all(pline->tags);
      text_tag_list_free(pline->tags);
      free(pline);
    } remaining_list_iterate_end;
    remaining_list_clear(remains);
  }
}

/**************************************************************************
  Turn off buffering and append the picked up data.
**************************************************************************/
void output_window_force_thaw(void)
{
  if (frozen_level > 0) {
    frozen_level = 1;
    output_window_thaw();
  }
}

/**************************************************************************
  Add a line of text to the output ("chatline") window, like puts() would
  do it in the console.
**************************************************************************/
void output_window_append(const char *fg_color, const char *bg_color,
                          const char *featured_text)
{
  char plain_text[MAX_LEN_MSG];
  struct text_tag_list *tags = text_tag_list_new();

  /* Separate the text and the tags. */
  featured_text_to_plain_text(featured_text, plain_text,
                              sizeof(plain_text), tags);

  if ((fg_color && fg_color[0] != '\0')
      || (bg_color && bg_color[0] != '\0')) {
    /* A color is requested. */
    struct text_tag *ptag = text_tag_new(TTT_COLOR, 0, OFFSET_UNSET,
                                         fg_color, bg_color);

    if (ptag) {
      /* Prepends to the list, to avoid to overwrite inside colors. */
      text_tag_list_prepend(tags, ptag);
    } else {
      freelog(LOG_ERROR,
              "Failed to create a color text tag (fg = %s, bg = %s).",
              fg_color ? fg_color : "NULL",
              bg_color ? bg_color : "NULL");
    }
  }

  if (frozen_level == 0) {
    real_output_window_append(plain_text, tags, -1);
    text_tag_list_clear_all(tags);
    text_tag_list_free(tags);
  } else {
    struct remaining *premain = fc_malloc(sizeof(*premain));

    remaining_list_append(remains, premain);
    premain->text = mystrdup(plain_text);
    premain->tags = tags;
    premain->conn_id = -1;
  }
}

/**************************************************************************
  Add a line of text to the output ("chatline") window.  The text is
  constructed in printf style.
**************************************************************************/
void output_window_vprintf(const char *fg_color, const char *bg_color,
                           const char *format, va_list args)
{
  char featured_text[MAX_LEN_MSG];

  my_vsnprintf(featured_text, sizeof(featured_text), format, args);
  output_window_append(fg_color, bg_color, featured_text);
}


/**************************************************************************
  Add a line of text to the output ("chatline") window.  The text is
  constructed in printf style.
**************************************************************************/
void output_window_printf(const char *fg_color, const char *bg_color,
                          const char *format, ...)
{
  va_list args;

  va_start(args, format);
  output_window_vprintf(fg_color, bg_color, format, args);
  va_end(args);
}

/**************************************************************************
  Add a line of text to the output ("chatline") window from server event.
**************************************************************************/
void output_window_event(const char *plain_text,
                         const struct text_tag_list *tags, int conn_id)
{
  if (frozen_level == 0) {
    real_output_window_append(plain_text, tags, conn_id);
  } else {
    struct remaining *premain = fc_malloc(sizeof(*premain));

    remaining_list_append(remains, premain);
    premain->text = mystrdup(plain_text);
    premain->tags = text_tag_list_dup(tags);
    premain->conn_id = conn_id;
  }
}

/****************************************************************************
  Standard welcome message.
****************************************************************************/
void chat_welcome_message(void)
{
  output_window_append(NULL, NULL,
                       _("Freeciv is free software and you are welcome to "
                         "distribute copies of it under certain conditions;"));
  output_window_append(NULL, NULL,
                       _("See the \"Copying\" item on the Help menu."));
  output_window_append(NULL, NULL,
                       _("Now ... Go give 'em hell!"));
}
