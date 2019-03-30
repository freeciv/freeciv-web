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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* utility */
#include "support.h"            /* bool type */

/* common */
#include "featured_text.h"      /* struct text_tag_list, struct ft_color */

int send_chat(const char *message);
int send_chat_printf(const char *format, ...)
    fc__attribute((__format__ (__printf__, 1, 2)));

void output_window_append(const struct ft_color color,
                          const char *featured_text);
void output_window_vprintf(const struct ft_color color,
                           const char *format, va_list args);
void output_window_printf(const struct ft_color color,
                          const char *format, ...)
                          fc__attribute((__format__ (__printf__, 2, 3)));
void output_window_event(const char *plain_text,
                         const struct text_tag_list *tags, int conn_id);

void chat_welcome_message(bool gui_has_copying_mitem);
void write_chatline_content(const char *txt);

void fc_allocate_ow_mutex(void);
void fc_release_ow_mutex(void);
void fc_init_ow_mutex(void);
void fc_destroy_ow_mutex(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__CHATLINE_COMMON_H */
