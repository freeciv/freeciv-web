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
#ifndef FC__CHATLINE_COMMON_H
#define FC__CHATLINE_COMMON_H

/* utility */
#include "shared.h"		/* bool type */

/* common */
#include "featured_text.h"      /* struct text_tag_list */

void send_chat(const char *message);
void send_chat_printf(const char *format, ...)
  fc__attribute((__format__ (__printf__, 1, 2)));

void chatline_common_init(void);
void chatline_common_done(void);

void output_window_append(const char *fg_color, const char *bg_color,
                          const char *featured_text);
void output_window_vprintf(const char *fg_color, const char *bg_color,
                           const char *format, va_list args);
void output_window_printf(const char *fg_color, const char *bg_color,
                          const char *format, ...)
                          fc__attribute((__format__ (__printf__, 3, 4)));
void output_window_event(const char *plain_text,
                         const struct text_tag_list *tags, int conn_id);

void output_window_freeze(void);
void output_window_thaw(void);
void output_window_force_thaw(void);
bool is_output_window_frozen(void);

void chat_welcome_message(void);

#endif  /* FC__CHATLINE_COMMON_H */
