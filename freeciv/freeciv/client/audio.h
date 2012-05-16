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
#ifndef FC__AUDIO_H
#define FC__AUDIO_H

#include "shared.h"		/* bool type */

#define MAX_AUDIO_NAME_LEN		20
#define MAX_AUDIO_DESCR_LEN		200

struct audio_plugin {
  char name[MAX_AUDIO_NAME_LEN];
  char descr[MAX_AUDIO_DESCR_LEN];
  bool (*init) (void);
  void (*shutdown) (void);
  void (*stop) (void);
  void (*wait) (void);
  double (*get_volume) (void);
  void (*set_volume) (double volume);
  bool (*play) (const char *const tag, const char *const path, bool repeat);
};

const char **get_soundplugin_list(void);
const char **get_soundset_list(void);

void audio_init(void);
void audio_real_init(const char *const spec_name,
		     const char *const prefered_plugin_name);
void audio_add_plugin(struct audio_plugin *p);
void audio_shutdown(void);
void audio_stop(void);

void audio_play_sound(const char *const tag, char *const alt_tag);
void audio_play_music(const char *const tag, char *const alt_tag);

double audio_get_volume(void);
void audio_set_volume(double volume);

bool audio_select_plugin(const char *const name);
const char *audio_get_all_plugin_names(void);

#endif				/* FC__AUDIO_H */
