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
#ifndef FC__REQTEXT_H
#define FC__REQTEXT_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

enum rt_verbosity { VERB_DEFAULT, VERB_ACTUAL };

bool req_text_insert(char *buf, size_t bufsz, struct player *pplayer,
                     const struct requirement *preq,
                     enum rt_verbosity verb, const char *prefix);

bool req_text_insert_nl(char *buf, size_t bufsz, struct player *pplayer,
                        const struct requirement *preq,
                        enum rt_verbosity verb, const char *prefix);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__REQTEXT_H */
