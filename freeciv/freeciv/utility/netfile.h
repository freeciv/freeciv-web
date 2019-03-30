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

#ifndef FC__NETFILE_H
#define FC__NETFILE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct netfile_post;

struct netfile_write_cb_data
{
  char *mem;
  int size;
};

typedef void (*nf_errmsg)(const char *msg, void *data);

struct section_file *netfile_get_section_file(const char *URL,
                                              nf_errmsg cb, void *data);

bool netfile_download_file(const char *URL, const char *filename,
                           nf_errmsg cb, void *data);

struct netfile_post *netfile_start_post(void);
void netfile_add_form_str(struct netfile_post *post,
                          const char *name, const char *val);
void netfile_add_form_int(struct netfile_post *post,
                          const char *name, const int val);
void netfile_close_post(struct netfile_post *post);

bool netfile_send_post(const char *URL, struct netfile_post *post,
                       FILE *reply_fp, struct netfile_write_cb_data *mem_data,
                       const char *addr);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__NETFILE_H */
