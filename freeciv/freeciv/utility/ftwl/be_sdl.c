/**********************************************************************
 Freeciv - Copyright (C) 2005 - The Freeciv Project
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
#include <config.h>
#endif

#include <assert.h>
#include <errno.h>
#include <stdio.h>

#include "SDL.h"

#include "back_end.h"

#include "shared.h"

#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "netintf.h"

static int other_fd = -1;

/*************************************************************************
  ...
*************************************************************************/
static inline bool copy_event(struct be_event *event, SDL_Event *sdl_event)
{
  switch (sdl_event->type) {
  case SDL_VIDEOEXPOSE:
    event->type = BE_EXPOSE;
    break;

  case SDL_MOUSEMOTION:
    event->type = BE_MOUSE_MOTION;
    event->position.x = sdl_event->motion.x;
    event->position.y = sdl_event->motion.y;

    break;
  case SDL_MOUSEBUTTONUP:
  case SDL_MOUSEBUTTONDOWN:
    event->type =
	sdl_event->type ==
	SDL_MOUSEBUTTONDOWN ? BE_MOUSE_PRESSED : BE_MOUSE_RELEASED;
    event->position.x = sdl_event->button.x;
    event->position.y = sdl_event->button.y;
    switch (sdl_event->button.button) {
    case SDL_BUTTON_LEFT:
      event->button = BE_MB_LEFT;
      break;
    case SDL_BUTTON_MIDDLE:
      event->button = BE_MB_MIDDLE;
      break;
    case SDL_BUTTON_RIGHT:
      event->button = BE_MB_RIGHT;
      break;
    default:
      assert(0);
    }
    break;
  case SDL_KEYDOWN:
    {
      SDLKey key = sdl_event->key.keysym.sym;

      event->key.alt = sdl_event->key.keysym.mod & KMOD_ALT;
      event->key.control = sdl_event->key.keysym.mod & KMOD_CTRL;
      event->key.shift = sdl_event->key.keysym.mod & KMOD_SHIFT;

      if (key == SDLK_BACKSPACE) {
	event->key.type = BE_KEY_BACKSPACE;
      } else if (key == SDLK_RETURN) {
	event->key.type = BE_KEY_RETURN;
      } else if (key == SDLK_KP_ENTER) {
	event->key.type = BE_KEY_ENTER;
      } else if (key == SDLK_DELETE) {
	event->key.type = BE_KEY_DELETE;
      } else if (key == SDLK_LEFT) {
	event->key.type = BE_KEY_LEFT;
      } else if (key == SDLK_RIGHT) {
	event->key.type = BE_KEY_RIGHT;
      } else if (key == SDLK_DOWN) {
	event->key.type = BE_KEY_DOWN;
      } else if (key == SDLK_UP) {
	event->key.type = BE_KEY_UP;
      } else if (key == SDLK_ESCAPE) {
	event->key.type = BE_KEY_ESCAPE;
      } else if (key == SDLK_KP0) {
	event->key.type = BE_KEY_KP_0;
      } else if (key == SDLK_KP1) {
	event->key.type = BE_KEY_KP_1;
      } else if (key == SDLK_KP2) {
	event->key.type = BE_KEY_KP_2;
      } else if (key == SDLK_KP3) {
	event->key.type = BE_KEY_KP_3;
      } else if (key == SDLK_KP4) {
	event->key.type = BE_KEY_KP_4;
      } else if (key == SDLK_KP5) {
	event->key.type = BE_KEY_KP_5;
      } else if (key == SDLK_KP6) {
	event->key.type = BE_KEY_KP_6;
      } else if (key == SDLK_KP7) {
	event->key.type = BE_KEY_KP_7;
      } else if (key == SDLK_KP8) {
	event->key.type = BE_KEY_KP_8;
      } else if (key == SDLK_KP9) {
	event->key.type = BE_KEY_KP_9;
      } else {
	Uint16 unicode = sdl_event->key.keysym.unicode;

	if (unicode == 0) {
          freelog(LOG_TEST, "unicode == 0");
	  return FALSE;
	}
	if ((unicode & 0xFF80) != 0) {
	  printf("An International Character '%c' %d.\n", (char) unicode,
		 (char) unicode);
	  return FALSE;
	}

	event->key.type = BE_KEY_NORMAL;
	event->key.key = unicode & 0x7F;
	event->key.shift = FALSE;
      }
      event->type = BE_KEY_PRESSED;
      SDL_GetMouseState(&event->position.x, &event->position.y);
    }
    break;
  case SDL_QUIT:
    exit(EXIT_SUCCESS);

  default:
    // freelog(LOG_TEST, "ignored event %d\n", sdl_event->type);
    return FALSE;
  }
  return TRUE;
}

/*************************************************************************
  ...
*************************************************************************/
void be_next_non_blocking_event(struct be_event *event)
{
  SDL_Event sdl_event;

  event->type = BE_NO_EVENT;

  while (SDL_PollEvent(&sdl_event)) {
    if (copy_event(event, &sdl_event)) {
      break;
    }
  }
}

/*************************************************************************
  ...
*************************************************************************/
void be_next_blocking_event(struct be_event *event, struct timeval *timeout)
{
  Uint32 timeout_end =
      SDL_GetTicks() + timeout->tv_sec * 1000 + timeout->tv_usec / 1000;

  for (;;) {

    /* Test for already queued events like mouse and keyboard */
    be_next_non_blocking_event(event);
    if (event->type != BE_NO_EVENT) {
      return;
    }

    /* Test the network socket. */
    if (other_fd != -1) {
      fd_set readfds, exceptfds;
      int ret;
      struct timeval zero_timeout;

      FD_ZERO(&readfds);
      FD_ZERO(&exceptfds);

      FD_SET(other_fd, &readfds);
      FD_SET(other_fd, &exceptfds);

      zero_timeout.tv_sec = 0;
      zero_timeout.tv_usec = 0;

      ret = fc_select(other_fd + 1, &readfds, NULL, &exceptfds, &zero_timeout);
      if (ret > 0 && (FD_ISSET(other_fd, &readfds) ||
		      FD_ISSET(other_fd, &exceptfds))) {
	event->type = BE_DATA_OTHER_FD;
	event->socket = other_fd;
	return;
      }
    }

    /* Test for the timeout */
    if (SDL_GetTicks() >= timeout_end) {
      event->type = BE_TIMEOUT;
      return;
    }

    /* Wait 10ms and do polling */
    SDL_Delay(10);
    assert(event->type == BE_NO_EVENT);
  }
}

/*************************************************************************
  ...
*************************************************************************/
void be_add_net_input(int sock)
{
  other_fd = sock;
}

/*************************************************************************
  ...
*************************************************************************/
void be_remove_net_input(void)
{
  other_fd = -1;
}

/*************************************************************************
  ...
*************************************************************************/
bool be_supports_fullscreen(void)
{
  return TRUE;
}
