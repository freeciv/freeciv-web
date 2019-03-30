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
#ifndef FC__RSSANITY_H
#define FC__RSSANITY_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* common */
#include "fc_types.h"

bool autoadjust_ruleset_data(void);
bool autolock_settings(void);
bool sanity_check_ruleset_data(bool ignore_retired);

bool sanity_check_server_setting_value_in_req(ssetv ssetval);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__RSSANITY_H */
