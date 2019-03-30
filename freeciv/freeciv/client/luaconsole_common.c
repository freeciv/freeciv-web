/*****************************************************************************
 Freeciv - Copyright (C) 2002 - R. Falke
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*****************************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <stdarg.h>
#include <string.h>

/* utility */
#include "fcintl.h"
#include "mem.h"

/* common */
#include "featured_text.h"

/* include */
#include "luaconsole_g.h"

/* client */
#include "luaconsole_common.h"



/*************************************************************************//**
  Add a line of text to the output ("chatline") window, like puts() would
  do it in the console.
*****************************************************************************/
void luaconsole_append(const struct ft_color color,
                       const char *featured_text)
{
  char plain_text[MAX_LEN_MSG];
  struct text_tag_list *tags;

  /* Separate the text and the tags. */
  featured_text_to_plain_text(featured_text, plain_text,
                              sizeof(plain_text), &tags, TRUE);

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

  real_luaconsole_append(plain_text, tags);
  text_tag_list_destroy(tags);
}

/*************************************************************************//**
  Add a line of text to the output ("chatline") window.  The text is
  constructed in printf style.
*****************************************************************************/
void luaconsole_vprintf(const struct ft_color color,
                        const char *format, va_list args)
{
  char featured_text[MAX_LEN_MSG];

  fc_vsnprintf(featured_text, sizeof(featured_text), format, args);
  luaconsole_append(color, featured_text);
}


/*************************************************************************//**
  Add a line of text to the output ("chatline") window.  The text is
  constructed in printf style.
*****************************************************************************/
void luaconsole_printf(const struct ft_color color,
                       const char *format, ...)
{
  va_list args;

  va_start(args, format);
  luaconsole_vprintf(color, format, args);
  va_end(args);
}

/*************************************************************************//**
  Add a line of text to the output ("chatline") window from server event.
*****************************************************************************/
void luaconsole_event(const char *plain_text,
                      const struct text_tag_list *tags)
{
  real_luaconsole_append(plain_text, tags);
}

/*************************************************************************//**
  Standard welcome message.
*****************************************************************************/
void luaconsole_welcome_message(void)
{
  luaconsole_append(ftc_any, _("This is the Client Lua Console."));
}
