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
#ifndef FC__MUSIC_H
#define FC__MUSIC_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void start_style_music(void);
void stop_style_music(void);
void start_menu_music(const char *const tag, char *const alt_tag);
void stop_menu_music(void);
void play_single_track(const char *const tag);

void musicspec_reread_callback(struct option *poption);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FC__MUSIC_H */
