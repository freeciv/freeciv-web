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
#ifndef FC__ZOOM_H
#define FC__ZOOM_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void zoom_set(float new_zoom);
void zoom_1_0(void);

#define zoom_get_level() map_zoom
#define zoom_is_enabled() zoom_enabled

void zoom_step_up(void);
void zoom_step_down(void);

void zoom_start(float tgt, bool tgt_1_0, float factor, float interval);
bool zoom_update(double time_until_next_call);

extern bool zoom_enabled;
extern float map_zoom;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FC__ZOOM_H */
