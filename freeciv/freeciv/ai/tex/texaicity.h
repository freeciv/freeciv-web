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
#ifndef FC__TEXAICITY_H
#define FC__TEXAICITY_H

#include "aicity.h"

struct city;
struct texai_req;

struct texai_city
{
  struct ai_city defai; /* Keep this first so default ai finds it */
  int unit_wants[U_LAST];
};

void texai_city_alloc(struct ai_type *ait, struct city *pcity);
void texai_city_free(struct ai_type *ait, struct city *pcity);

void texai_city_worker_requests_create(struct ai_type *ait, struct player *pplayer,
                                       struct city *pcity);
void texai_city_worker_wants(struct ai_type *ait,
                             struct player *pplayer, struct city *pcity);
void texai_req_worker_task_rcv(struct texai_req *req);

static inline struct texai_city *texai_city_data(struct ai_type *ait,
                                                 const struct city *pcity)
{
  return (struct texai_city *)city_ai_data(pcity, ait);
}

#endif /* FC__TEXAICITY_H */
