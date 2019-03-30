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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "support.h"            /* bool type */

#define MAX_AUDIO_NAME_LEN		20
#define MAX_AUDIO_DESCR_LEN		200

#define MAX_ALT_AUDIO_FILES             5

typedef void (*audio_finished_callback)(void);

struct audio_plugin {
  char name[MAX_AUDIO_NAME_LEN];
  char descr[MAX_AUDIO_DESCR_LEN];
  bool (*init) (void);
  void (*shutdown) (void);
  void (*stop) (void);
  void (*wait) (void);
  double (*get_volume) (void);
  void (*set_volume) (double volume);
  bool (*play) (const char *const tag, const char *const path, bool repeat,
                audio_finished_callback cb);
};

enum music_usage { MU_SINGLE, MU_MENU, MU_INGAME };

struct strvec;
struct option;
const struct strvec *get_soundplugin_list(const struct option *poption);
const struct strvec *get_soundset_list(const struct option *poption);
const struct strvec *get_musicset_list(const struct option *poption);

void audio_init(void);
void audio_real_init(const char *const soundspec_name,
                     const char *const musicset_name,
                     const char *const preferred_plugin_name);
void audio_add_plugin(struct audio_plugin *p);
void audio_shutdown(void);
void audio_stop(void);
void audio_stop_usage(void);
void audio_restart(const char *soundset_name, const char *musicset_name);

void audio_play_sound(const char *const tag, char *const alt_tag);
void audio_play_music(const char *const tag, char *const alt_tag,
                      enum music_usage usage);
void audio_play_track(const char *const tag, char *const alt_tag);
bool audio_play_from_path(const char *path, audio_finished_callback cb);

double audio_get_volume(void);
void audio_set_volume(double volume);

bool audio_select_plugin(const char *const name);
const char *audio_get_all_plugin_names(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FC__AUDIO_H */
