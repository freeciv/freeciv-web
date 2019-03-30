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

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <string.h>

#ifdef SDL2_PLAIN_INCLUDE
#include <SDL.h>
#include <SDL_mixer.h>
#else  /* SDL2_PLAIN_INCLUDE */
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#endif /* SDL2_PLAIN_INCLUDE */

/* utility */
#include "log.h"
#include "support.h"

/* client */
#include "audio.h"

#include "audio_sdl.h"

struct sample {
  Mix_Chunk *wave;
  const char *tag;
};

/* Sounds don't sound good on Windows unless the buffer size is 4k,
 * but this seems to cause strange behaviour on other systems,
 * such as a delay before playing the sound. */
#ifdef FREECIV_MSWINDOWS
const size_t buf_size = 4096;
#else
const size_t buf_size = 1024;
#endif

static Mix_Music *mus = NULL;
static struct sample samples[MIX_CHANNELS];
static double sdl_audio_volume;

/**********************************************************************//**
  Set the volume.
**************************************************************************/
static void sdl_audio_set_volume(double volume)
{
  Mix_VolumeMusic(volume * MIX_MAX_VOLUME);
  Mix_Volume(-1, volume * MIX_MAX_VOLUME);
  sdl_audio_volume = volume;
}

/**********************************************************************//**
  Get the volume.
**************************************************************************/
static double sdl_audio_get_volume(void)
{
  return sdl_audio_volume;
}

/**********************************************************************//**
  Play sound
**************************************************************************/
static bool sdl_audio_play(const char *const tag, const char *const fullpath,
                           bool repeat, audio_finished_callback cb)
{
  int i, j;
  Mix_Chunk *wave = NULL;

  if (!fullpath) {
    return FALSE;
  }

  if (repeat) {
    /* unload previous */
    Mix_HaltMusic();
    Mix_FreeMusic(mus);

    /* load music file */
    mus = Mix_LoadMUS(fullpath);
    if (mus == NULL) {
      log_error("Can't open file \"%s\"", fullpath);
    }

    if (cb == NULL) {
      Mix_PlayMusic(mus, -1);	/* -1 means loop forever */
    } else {
      Mix_PlayMusic(mus, 0);
      Mix_HookMusicFinished(cb);
    }
    log_verbose("Playing file \"%s\" on music channel", fullpath);
    /* in case we did a sdl_audio_stop() recently; add volume controls later */
    Mix_VolumeMusic(MIX_MAX_VOLUME);

  } else {

    /* see if we can cache on this one */
    for (j = 0; j < MIX_CHANNELS; j++) {
      if (samples[j].tag && (strcmp(samples[j].tag, tag) == 0)) {
        log_debug("Playing file \"%s\" from cache (slot %d)", fullpath, j);
        Mix_PlayChannel(-1, samples[j].wave, 0);
        return TRUE;
      }
    }                           /* guess not */

    /* load wave */
    wave = Mix_LoadWAV(fullpath);
    if (wave == NULL) {
      log_error("Can't open file \"%s\"", fullpath);
    }

    /* play sound sample on first available channel, returns -1 if no
       channel found */
    i = Mix_PlayChannel(-1, wave, 0);
    if (i < 0) {
      log_verbose("No available sound channel to play %s.", tag);
      Mix_FreeChunk(wave);
      return FALSE;
    }
    log_verbose("Playing file \"%s\" on channel %d", fullpath, i);
    /* free previous sample on this channel. it will by definition no
       longer be playing by the time we get here */
    if (samples[i].wave) {
      Mix_FreeChunk(samples[i].wave);
      samples[i].wave = NULL;
    }
    /* remember for cacheing */
    samples[i].wave = wave;
    samples[i].tag = tag;

  }
  return TRUE;
}

/**********************************************************************//**
  Stop music
**************************************************************************/
static void sdl_audio_stop(void)
{
  /* fade out over 2 sec */
  Mix_FadeOutMusic(2000);
}

/**********************************************************************//**
  Wait for audio to die on all channels.
  WARNING: If a channel is looping, it will NEVER exit! Always call
  music_stop() first!
**************************************************************************/
static void sdl_audio_wait(void)
{
  while (Mix_Playing(-1) != 0) {
    SDL_Delay(100);
  }
}

/**********************************************************************//**
  Quit SDL.  If the video is still in use (by gui-sdl), just quit the
  subsystem.

  This will need to be changed if SDL is used elsewhere.
**************************************************************************/
static void quit_sdl_audio(void)
{
  if (SDL_WasInit(SDL_INIT_VIDEO)) {
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
  } else {
    SDL_Quit();
  }
}

/**********************************************************************//**
  Init SDL.  If the video is already in use (by gui-sdl), just init the
  subsystem.

  This will need to be changed if SDL is used elsewhere.
**************************************************************************/
static int init_sdl_audio(void)
{
  if (SDL_WasInit(SDL_INIT_VIDEO)) {
    return SDL_InitSubSystem(SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE);
  } else {
    return SDL_Init(SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE);
  }
}

/**********************************************************************//**
  Clean up.
**************************************************************************/
static void sdl_audio_shutdown(void)
{
  int i;

  sdl_audio_stop();
  sdl_audio_wait();

  /* remove all buffers */
  for (i = 0; i < MIX_CHANNELS; i++) {
    if (samples[i].wave) {
      Mix_FreeChunk(samples[i].wave);
    }
  }
  Mix_HaltMusic();
  Mix_FreeMusic(mus);

  Mix_CloseAudio();
  quit_sdl_audio();
}

/**********************************************************************//**
  Initialize.
**************************************************************************/
static bool sdl_audio_init(void)
{
  /* Initialize variables */
  const int audio_rate = MIX_DEFAULT_FREQUENCY;
  const int audio_format = MIX_DEFAULT_FORMAT;
  const int audio_channels = 2;
  int i;

  if (init_sdl_audio() < 0) {
    return FALSE;
  }

  if (Mix_OpenAudio(audio_rate, audio_format, audio_channels, buf_size) < 0) {
    log_error("Error calling Mix_OpenAudio");
    /* try something else */
    quit_sdl_audio();
    return FALSE;
  }

  Mix_AllocateChannels(MIX_CHANNELS);
  for (i = 0; i < MIX_CHANNELS; i++) {
    samples[i].wave = NULL;
  }
  /* sanity check, for now; add volume controls later */
  sdl_audio_set_volume(sdl_audio_volume);
  return TRUE;
}

/**********************************************************************//**
  Initialize. Note that this function is called very early at the
  client startup. So for example logging isn't available.
**************************************************************************/
void audio_sdl_init(void)
{
  struct audio_plugin self;

  sz_strlcpy(self.name, "sdl");
  sz_strlcpy(self.descr, "Simple DirectMedia Library (SDL) mixer plugin");
  self.init = sdl_audio_init;
  self.shutdown = sdl_audio_shutdown;
  self.stop = sdl_audio_stop;
  self.wait = sdl_audio_wait;
  self.play = sdl_audio_play;
  self.set_volume = sdl_audio_set_volume;
  self.get_volume = sdl_audio_get_volume;
  audio_add_plugin(&self);
  sdl_audio_volume = 1.0;
}
