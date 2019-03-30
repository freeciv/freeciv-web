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

/***********************************************************************
                          gui_main.c  -  description
                             -------------------
    begin                : Sun Jun 30 2002
    copyright            : (C) 2002 by Rafał Bursig
    email                : Rafał Bursig <bursig@poczta.fm>
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include "fc_prehdrs.h"

#include <errno.h>

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* SDL2 */
#ifdef SDL2_PLAIN_INCLUDE
#include <SDL.h>
#else  /* SDL2_PLAIN_INCLUDE */
#include <SDL2/SDL.h>
#endif /* SDL2_PLAIN_INCLUDE */

/* utility */
#include "fc_cmdline.h"
#include "fciconv.h"
#include "fcintl.h"
#include "log.h"
#include "netintf.h"

/* common */
#include "unitlist.h"

/* client */
#include "client_main.h"
#include "climisc.h"
#include "clinet.h"
#include "editgui_g.h"
#include "tilespec.h"
#include "update_queue.h"

/* gui-sdl2 */
#include "chatline.h"
#include "citydlg.h"
#include "cityrep.h"
#include "diplodlg.h"
#include "graphics.h"
#include "gui_id.h"
#include "gui_mouse.h"
#include "gui_tilespec.h"
#include "inteldlg.h"
#include "mapctrl.h"
#include "mapview.h"
#include "menu.h"
#include "messagewin.h"
#include "optiondlg.h"
#include "repodlgs.h"
#include "themespec.h"
#include "spaceshipdlg.h"
#include "widget.h"

#include "gui_main.h"

#define UNITS_TIMER_INTERVAL 128	/* milliseconds */
#define MAP_SCROLL_TIMER_INTERVAL 500

const char *client_string = "gui-sdl2";

/* The real GUI character encoding is UTF-16 which is not supported by
 * fciconv code at this time. Conversion between UTF-8 and UTF-16 is done
 * in gui_iconv.c */
const char * const gui_character_encoding = "UTF-8";
const bool gui_use_transliteration = FALSE;

Uint32 SDL_Client_Flags = 0;

Uint32 widget_info_counter = 0;
int MOVE_STEP_X = DEFAULT_MOVE_STEP;
int MOVE_STEP_Y = DEFAULT_MOVE_STEP;
extern bool draw_goto_patrol_lines;
SDL_Event *flush_event = NULL;
bool is_unit_move_blocked;
bool LSHIFT;
bool RSHIFT;
bool LCTRL;
bool RCTRL;
bool LALT;
static int city_names_font_size = 10;
static int city_productions_font_size = 10;
int *client_font_sizes[FONT_COUNT] = {
  &city_names_font_size,       /* FONT_CITY_NAME */
  &city_productions_font_size, /* FONT_CITY_PROD */
  &city_productions_font_size  /* FONT_REQTREE_TEXT; not used yet */
};

/* ================================ Private ============================ */
static int net_socket = -1;
static bool autoconnect = FALSE;
static bool is_map_scrolling = FALSE;
static enum direction8 scroll_dir;

static struct mouse_button_behavior button_behavior;

static SDL_Event *pNet_User_Event = NULL;
static SDL_Event *pAnim_User_Event = NULL;
static SDL_Event *pInfo_User_Event = NULL;
static SDL_Event *pMap_Scroll_User_Event = NULL;

static void print_usage(void);
static void parse_options(int argc, char **argv);
static int check_scroll_area(int x, int y);

int user_event_type;

enum USER_EVENT_ID {
  EVENT_ERROR = 0,
  NET,
  ANIM,
  TRY_AUTO_CONNECT,
  SHOW_WIDGET_INFO_LABEL,
  FLUSH,
  MAP_SCROLL,
  EXIT_FROM_EVENT_LOOP
};

struct callback {
  void (*callback)(void *data);
  void *data;
};

#define SPECLIST_TAG callback
#define SPECLIST_TYPE struct callback
#include "speclist.h"

struct callback_list *callbacks;

/* =========================================================== */

/**********************************************************************//**
  Print extra usage information, including one line help on each option,
  to stderr.
**************************************************************************/
static void print_usage(void)
{
  /* add client-specific usage information here */
  fc_fprintf(stderr,
             _("  -f,  --fullscreen\tStart Client in Fullscreen mode\n"));
  fc_fprintf(stderr, _("  -t,  --theme THEME\tUse GUI theme THEME\n"));

  /* TRANS: No full stop after the URL, could cause confusion. */
  fc_fprintf(stderr, _("Report bugs at %s\n"), BUG_URL);
}

/**********************************************************************//**
  Search for command line options. right now, it's just help
  semi-useless until we have options that aren't the same across all clients.
**************************************************************************/
static void parse_options(int argc, char **argv)
{
  int i = 1;
  char *option = NULL;

  while (i < argc) {
    if (is_option("--help", argv[i])) {
      print_usage();
      exit(EXIT_SUCCESS);
    } else if (is_option("--fullscreen", argv[i])) {
      gui_options.gui_sdl2_fullscreen = TRUE;
    } else if ((option = get_option_malloc("--theme", argv, &i, argc, FALSE))) {
      sz_strlcpy(gui_options.gui_sdl2_default_theme_name, option);
      free(option);
    } else {
      fc_fprintf(stderr, _("Unrecognized option: \"%s\"\n"), argv[i]);
      exit(EXIT_FAILURE);
    }

    i++;
  }
}

/**********************************************************************//**
  Main handler for key presses
**************************************************************************/
static Uint16 main_key_down_handler(SDL_Keysym key, void *data)
{
  static struct widget *pWidget;

  if ((pWidget = find_next_widget_for_key(NULL, key)) != NULL) {
    return widget_pressed_action(pWidget);
  } else {
    if (key.sym == SDLK_TAB) {
      /* input */
      popup_input_line();
    } else {
      if (map_event_handler(key)
          && C_S_RUNNING == client_state()) {
        switch (key.sym) {
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
          if (LSHIFT || RSHIFT) {
            disable_focus_animation();
            key_end_turn();
          } else {
            struct unit *pUnit;
            struct city *pCity;

            if (NULL != (pUnit = head_of_units_in_focus())
                && (pCity = tile_city(unit_tile(pUnit))) != NULL
                && city_owner(pCity) == client.conn.playing) {
              popup_city_dialog(pCity);
            }
          }
          return ID_ERROR;

        case SDLK_F2:
          units_report_dialog_popup(FALSE);
          return ID_ERROR;

        case SDLK_F4:
          city_report_dialog_popup(FALSE);
          return ID_ERROR;

        case SDLK_F7:
          send_report_request(REPORT_WONDERS_OF_THE_WORLD);
          return ID_ERROR;

        case SDLK_F8:
          send_report_request(REPORT_TOP_5_CITIES);
          return ID_ERROR;

        case SDLK_F9:
          if (meswin_dialog_is_open()) {
            meswin_dialog_popdown();
          } else {
            meswin_dialog_popup(TRUE);
          }
          flush_dirty();
          return ID_ERROR;

        case SDLK_F11:
          send_report_request(REPORT_DEMOGRAPHIC);
          return ID_ERROR;

        case SDLK_F12:
          popup_spaceship_dialog(client.conn.playing);
          return ID_ERROR;

        case SDLK_ASTERISK:
          send_report_request(REPORT_ACHIEVEMENTS);
          return ID_ERROR;

        default:
          return ID_ERROR;
        }
      }
    }
  }

  return ID_ERROR;
}

/**********************************************************************//**
  Main key release handler.
**************************************************************************/
static Uint16 main_key_up_handler(SDL_Keysym Key, void *pData)
{
  if (selected_widget) {
    unselect_widget_action();
  }

  return ID_ERROR;
}

/**********************************************************************//**
  Main mouse click handler.
**************************************************************************/
static Uint16 main_mouse_button_down_handler(SDL_MouseButtonEvent *pButtonEvent,
                                             void *pData)
{
  struct widget *pWidget;

  if ((pWidget = find_next_widget_at_pos(NULL,
                                         pButtonEvent->x,
                                         pButtonEvent->y)) != NULL) {
    if (get_wstate(pWidget) != FC_WS_DISABLED) {
      return widget_pressed_action(pWidget);
    }
  } else {
    /* no visible widget at this position -> map click */
#ifdef UNDER_CE
    if (!check_scroll_area(pButtonEvent->x, pButtonEvent->y)) {
#endif
    if (!button_behavior.counting) {
      /* start counting */
      button_behavior.counting = TRUE;
      button_behavior.button_down_ticks = SDL_GetTicks();
      *button_behavior.event = *pButtonEvent;
      button_behavior.hold_state = MB_HOLD_SHORT;
      button_behavior.ptile = canvas_pos_to_tile(pButtonEvent->x, pButtonEvent->y);
    }
#ifdef UNDER_CE
    }
#endif
  }
  return ID_ERROR;
}

/**********************************************************************//**
  Main mouse button release handler.
**************************************************************************/
static Uint16 main_mouse_button_up_handler(SDL_MouseButtonEvent *pButtonEvent,
                                           void *pData)
{
  if (button_behavior.button_down_ticks /* button wasn't pressed over a widget */
      && !find_next_widget_at_pos(NULL, pButtonEvent->x, pButtonEvent->y)) {
    *button_behavior.event = *pButtonEvent;
    button_up_on_map(&button_behavior);
  }

  button_behavior.counting = FALSE;
  button_behavior.button_down_ticks = 0;

  is_map_scrolling = FALSE;

  return ID_ERROR;
}

#ifdef UNDER_CE
  #define SCROLL_MAP_AREA       8
#else
  #define SCROLL_MAP_AREA       1 
#endif

/**********************************************************************//**
  Main handler for mouse movement handling.
**************************************************************************/
static Uint16 main_mouse_motion_handler(SDL_MouseMotionEvent *pMotionEvent,
                                        void *pData)
{
  static struct widget *pWidget;
  struct tile *ptile;

  /* stop evaluating button hold time when moving to another tile in medium
   * hold state or above */
  if (button_behavior.counting && (button_behavior.hold_state >= MB_HOLD_MEDIUM)) {
    ptile = canvas_pos_to_tile(pMotionEvent->x, pMotionEvent->y);
    if (tile_index(ptile) != tile_index(button_behavior.ptile)) {
      button_behavior.counting = FALSE;
    }
  }

  if (draw_goto_patrol_lines) {
    update_line(pMotionEvent->x, pMotionEvent->y);
  }

#ifndef UNDER_CE
  if (gui_options.gui_sdl2_fullscreen) {
    check_scroll_area(pMotionEvent->x, pMotionEvent->y);
  }
#endif /* UNDER_CE */

  if ((pWidget = find_next_widget_at_pos(NULL,
                                         pMotionEvent->x,
                                         pMotionEvent->y)) != NULL) {
    update_mouse_cursor(CURSOR_DEFAULT);
    if (get_wstate(pWidget) != FC_WS_DISABLED) {
      widget_selected_action(pWidget);
    }
  } else {
    if (selected_widget) {
      unselect_widget_action();
    } else {
      control_mouse_cursor(canvas_pos_to_tile(pMotionEvent->x, pMotionEvent->y));
    }
  }

  draw_mouse_cursor();

  return ID_ERROR;
}

/**********************************************************************//**
  This is called every TIMER_INTERVAL milliseconds whilst we are in
  gui_main_loop() (which is all of the time) TIMER_INTERVAL needs to be .5s
**************************************************************************/
static void update_button_hold_state(void)
{
  /* button pressed */
  if (button_behavior.counting) {
    if (((SDL_GetTicks() - button_behavior.button_down_ticks) >= MB_MEDIUM_HOLD_DELAY)
        && ((SDL_GetTicks() - button_behavior.button_down_ticks) < MB_LONG_HOLD_DELAY)) {

      if (button_behavior.hold_state != MB_HOLD_MEDIUM) {
        button_behavior.hold_state = MB_HOLD_MEDIUM;
        button_down_on_map(&button_behavior);
      }

    } else if (((SDL_GetTicks() - button_behavior.button_down_ticks)
                                                    >= MB_LONG_HOLD_DELAY)) {

      if (button_behavior.hold_state != MB_HOLD_LONG) {
        button_behavior.hold_state = MB_HOLD_LONG;
        button_down_on_map(&button_behavior);
      }
    }  
  }

  return;
}

/**********************************************************************//**
  Check if coordinate is in scroll area.
**************************************************************************/
static int check_scroll_area(int x, int y)
{
  SDL_Rect rect_north = {0, 0, Main.map->w, SCROLL_MAP_AREA};
  SDL_Rect rect_east = {Main.map->w - SCROLL_MAP_AREA, 0, SCROLL_MAP_AREA, Main.map->h};
  SDL_Rect rect_south = {0, Main.map->h - SCROLL_MAP_AREA, Main.map->w, SCROLL_MAP_AREA};
  SDL_Rect rect_west = {0, 0, SCROLL_MAP_AREA, Main.map->h};

  if (is_in_rect_area(x, y, rect_north)) {
    is_map_scrolling = TRUE;
    if (is_in_rect_area(x, y, rect_west)) {
      scroll_dir = DIR8_NORTHWEST;
    } else if (is_in_rect_area(x, y, rect_east)) {
      scroll_dir = DIR8_NORTHEAST;
    } else {
      scroll_dir = DIR8_NORTH;
    }
  } else if (is_in_rect_area(x, y, rect_south)) {
    is_map_scrolling = TRUE;
    if (is_in_rect_area(x, y, rect_west)) {
      scroll_dir = DIR8_SOUTHWEST;
    } else if (is_in_rect_area(x, y, rect_east)) {
      scroll_dir = DIR8_SOUTHEAST;
    } else {
      scroll_dir = DIR8_SOUTH;
    }
  } else if (is_in_rect_area(x, y, rect_east)) {
    is_map_scrolling = TRUE;
    scroll_dir = DIR8_EAST;
  } else if (is_in_rect_area(x, y, rect_west)) {
    is_map_scrolling = TRUE;
    scroll_dir = DIR8_WEST;
  } else {
    is_map_scrolling = FALSE;
  }

  return is_map_scrolling;
}

/* ============================ Public ========================== */

/**********************************************************************//**
  Instruct event loop to exit.
**************************************************************************/
void force_exit_from_event_loop(void)
{
  SDL_Event Event;

  Event.type = user_event_type;
  Event.user.code = EXIT_FROM_EVENT_LOOP;
  Event.user.data1 = NULL;
  Event.user.data2 = NULL;

  SDL_PushEvent(&Event);
}

/**********************************************************************//**
  Filter out mouse motion events for too small movement to react to.
  This function may run in a separate event thread.
**************************************************************************/
int FilterMouseMotionEvents(void *data, SDL_Event *event)
{
  if (event->type == SDL_MOUSEMOTION) {
    static int x = 0, y = 0;

    if (((MOVE_STEP_X > 0) && (abs(event->motion.x - x) >= MOVE_STEP_X))
        || ((MOVE_STEP_Y > 0) && (abs(event->motion.y - y) >= MOVE_STEP_Y)) ) {
      x = event->motion.x;
      y = event->motion.y;
      return 1;    /* Catch it */
    } else {
      return 0;    /* Drop it, we've handled it */
    }
  }
  return 1;
}

/**********************************************************************//**
  SDL2-client main loop.
**************************************************************************/
Uint16 gui_event_loop(void *pData,
                      void (*loop_action)(void *pData),
                      Uint16 (*key_down_handler)(SDL_Keysym Key, void *pData),
                      Uint16 (*key_up_handler)(SDL_Keysym Key, void *pData),
                      Uint16 (*textinput_handler)(char *text, void *pData),
                      Uint16 (*mouse_button_down_handler)(SDL_MouseButtonEvent *pButtonEvent,
                                                          void *pData),
                      Uint16 (*mouse_button_up_handler)(SDL_MouseButtonEvent *pButtonEvent,
                                                        void *pData),
                      Uint16 (*mouse_motion_handler)(SDL_MouseMotionEvent *pMotionEvent,
                                                     void *pData))
{
  Uint16 ID;
  static fc_timeval tv;
  static fd_set civfdset;
  Uint32 t_current, t_last_unit_anim, t_last_map_scrolling;
  Uint32 real_timer_next_call;
  static int result;

  ID = ID_ERROR;
  t_last_map_scrolling = t_last_unit_anim = real_timer_next_call = SDL_GetTicks();
  while (ID == ID_ERROR) {
    /* ========================================= */
    /* net check with 10ms delay event loop */
    if (net_socket >= 0) {
      FD_ZERO(&civfdset);

      if (net_socket >= 0) {
        FD_SET(net_socket, &civfdset);
      }

      tv.tv_sec = 0;
      tv.tv_usec = 10000; /* 10ms*/

      result = fc_select(net_socket + 1, &civfdset, NULL, NULL, &tv);
      if (result < 0) {
        if (errno != EINTR) {
          break;
        } else {
          continue;
        }
      } else {
        if (result > 0) {
          if ((net_socket >= 0) && FD_ISSET(net_socket, &civfdset)) {
            SDL_PushEvent(pNet_User_Event);
          }
        }
      }
    } else { /* if connection is not establish */
      SDL_Delay(10);
    }
    /* ========================================= */

    t_current = SDL_GetTicks();

    if (t_current > real_timer_next_call) {
      real_timer_next_call = t_current + (real_timer_callback() * 1000);
    }

    if ((t_current - t_last_unit_anim) > UNITS_TIMER_INTERVAL) {
      if (autoconnect) {
        widget_info_counter++;
        SDL_PushEvent(pAnim_User_Event);
      } else {
        SDL_PushEvent(pAnim_User_Event);
      }

      t_last_unit_anim = SDL_GetTicks();
    }

    if (is_map_scrolling) {
      if ((t_current - t_last_map_scrolling) > MAP_SCROLL_TIMER_INTERVAL) {
        SDL_PushEvent(pMap_Scroll_User_Event);
        t_last_map_scrolling = SDL_GetTicks();
      }
    } else {
      t_last_map_scrolling = SDL_GetTicks();
    }

    if (widget_info_counter > 0) {
      SDL_PushEvent(pInfo_User_Event);
      widget_info_counter = 0;
    }

    /* ========================================= */

    if (loop_action) {
      loop_action(pData);
    }

    /* ========================================= */

    while (SDL_PollEvent(&Main.event) == 1) {

      if (Main.event.type == user_event_type) {
        switch(Main.event.user.code) {
        case NET:
          input_from_server(net_socket);
          break;
        case ANIM:
          update_button_hold_state();
          animate_mouse_cursor();
          draw_mouse_cursor();
          break;
        case SHOW_WIDGET_INFO_LABEL:
          draw_widget_info_label();
          break;
        case TRY_AUTO_CONNECT:
          if (try_to_autoconnect()) {
            pInfo_User_Event->user.code = SHOW_WIDGET_INFO_LABEL;
            autoconnect = FALSE;
          }
          break;
        case FLUSH:
          unqueue_flush();
          break;
        case MAP_SCROLL:
          scroll_mapview(scroll_dir);
          break;
        case EXIT_FROM_EVENT_LOOP:
          return MAX_ID;
          break;
        default:
          break;
        }

      } else {

        switch (Main.event.type) {

        case SDL_QUIT:
          return MAX_ID;
          break;

        case SDL_KEYUP:
          switch (Main.event.key.keysym.sym) {
            /* find if Shifts are released */
            case SDLK_RSHIFT:
              RSHIFT = FALSE;
            break;
            case SDLK_LSHIFT:
              LSHIFT = FALSE;
            break;
            case SDLK_LCTRL:
              LCTRL = FALSE;
            break;
            case SDLK_RCTRL:
              RCTRL = FALSE;
            break;
            case SDLK_LALT:
              LALT = FALSE;
            break;
            default:
              if (key_up_handler) {
                ID = key_up_handler(Main.event.key.keysym, pData);
              }
            break;
          }
          break;

        case SDL_KEYDOWN:
          switch(Main.event.key.keysym.sym) {
#if 0
            case SDLK_PRINT:
              fc_snprintf(schot, sizeof(schot), "fc_%05d.bmp", schot_nr++);
              log_normal(_("Making screenshot %s"), schot);
              SDL_SaveBMP(Main.screen, schot);
            break;
#endif

            case SDLK_RSHIFT:
              /* Right Shift is Pressed */
              RSHIFT = TRUE;
            break;

            case SDLK_LSHIFT:
              /* Left Shift is Pressed */
              LSHIFT = TRUE;
            break;

            case SDLK_LCTRL:
              /* Left CTRL is Pressed */
              LCTRL = TRUE;
            break;

            case SDLK_RCTRL:
              /* Right CTRL is Pressed */
              RCTRL = TRUE;
            break;

            case SDLK_LALT:
              /* Left ALT is Pressed */
              LALT = TRUE;
            break;

            default:
              if (key_down_handler) {
                ID = key_down_handler(Main.event.key.keysym, pData);
              }
            break;
          }
          break;

        case SDL_TEXTINPUT:
          if (textinput_handler) {
            ID = textinput_handler(Main.event.text.text, pData);
          }
          break;

        case SDL_MOUSEBUTTONDOWN:
          if (mouse_button_down_handler) {
            ID = mouse_button_down_handler(&Main.event.button, pData);
          }
          break;

        case SDL_MOUSEBUTTONUP:
          if (mouse_button_up_handler) {
            ID = mouse_button_up_handler(&Main.event.button, pData);
          }
          break;

        case SDL_MOUSEMOTION:
          if (mouse_motion_handler) {
            ID = mouse_motion_handler(&Main.event.motion, pData);
          }	
          break;
        }
      }
    }

    if (ID == ID_ERROR) {
      if (callbacks && callback_list_size(callbacks) > 0) {
        struct callback *cb = callback_list_get(callbacks, 0);

        callback_list_remove(callbacks, cb);
        (cb->callback)(cb->data);
        free(cb);
      }
    }

    update_main_screen();
  }

  return ID;
}

/* ============ Freeciv native game function =========== */

/**********************************************************************//**
  Do any necessary pre-initialization of the UI, if necessary.
**************************************************************************/
void ui_init(void)
{
  Uint32 iSDL_Flags;

  button_behavior.counting = FALSE;
  button_behavior.button_down_ticks = 0;
  button_behavior.hold_state = MB_HOLD_SHORT;
  button_behavior.event = fc_calloc(1, sizeof(SDL_MouseButtonEvent));

  SDL_Client_Flags = 0;
  iSDL_Flags = SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE;

  /* auto center new windows in X enviroment */
  putenv((char *)"SDL_VIDEO_CENTERED=yes");

  init_sdl(iSDL_Flags);
}

/**********************************************************************//**
  Really resize the main window.
**************************************************************************/
static void real_resize_window_callback(void *data)
{
  struct widget *widget;

  if (C_S_RUNNING == client_state()) {
    /* Move units window to botton-right corner. */
    set_new_unitinfo_window_pos();
    /* Move minimap window to botton-left corner. */
    set_new_minimap_window_pos();

    /* Move cooling/warming icons to botton-right corner. */
    widget = get_widget_pointer_form_main_list(ID_WARMING_ICON);
    widget_set_position(widget, (main_window_width() - adj_size(10)
                                 - (widget->size.w * 2)), widget->size.y);

    widget = get_widget_pointer_form_main_list(ID_COOLING_ICON);
    widget_set_position(widget, (main_window_width() - adj_size(10)
                                 - widget->size.w), widget->size.y);

    map_canvas_resized(main_window_width(), main_window_height());
    update_info_label();
    update_unit_info_label(get_units_in_focus());
    center_on_something();      /* With redrawing full map. */
    update_order_widgets();
  } else {
    draw_intro_gfx();
    dirty_all();
  }
  flush_all();
}

/**********************************************************************//**
  Resize the main window after option changed.
**************************************************************************/
static void resize_window_callback(struct option *poption)
{
  update_queue_add(real_resize_window_callback, NULL);
}

/**********************************************************************//**
  Extra initializers for client options. Here we make set the callback
  for the specific gui-sdl2 options.
**************************************************************************/
void options_extra_init(void)
{
  struct option *poption;

#define option_var_set_callback(var, callback)                              \
  if ((poption = optset_option_by_name(client_optset, #var))) {             \
    option_set_changed_callback(poption, callback);                         \
  } else {                                                                  \
    log_error("Didn't find option %s!", #var);                              \
  }

  option_var_set_callback(gui_sdl2_fullscreen, resize_window_callback);
  option_var_set_callback(gui_sdl2_screen, resize_window_callback);
#undef option_var_set_callback
}

/**********************************************************************//**
  Remove double messages caused by message configured to both MW_MESSAGES
  and MW_OUTPUT.
**************************************************************************/
static void clear_double_messages_call(void)
{
  int i;

  /* clear double call */
  for (i = 0; i <= event_type_max(); i++) {
    if (messages_where[i] & MW_MESSAGES) {
      messages_where[i] &= ~MW_OUTPUT;
    }
  }
}

/**********************************************************************//**
  Entry point for freeciv client program. SDL has macro magic to turn
  this in to function named SDL_main() and it provides actual main()
  itself.
**************************************************************************/
int main(int argc, char **argv)
{
  return client_main(argc, argv);
}

/**********************************************************************//**
  Migrate sdl2 client specific options from sdl client options.
**************************************************************************/
static void migrate_options_from_sdl(void)
{
  log_normal(_("Migrating options from sdl to sdl2 client"));

#define MIGRATE_OPTION(opt) gui_options.gui_sdl2_##opt = gui_options.gui_sdl_##opt;

  /* Default theme name is never migrated */
  MIGRATE_OPTION(fullscreen);
  MIGRATE_OPTION(screen);
  MIGRATE_OPTION(do_cursor_animation);
  MIGRATE_OPTION(use_color_cursors);

#undef MIGRATE_OPTION

  gui_options.gui_sdl2_migrated_from_sdl = TRUE;
}

/**********************************************************************//**
  The main loop for the UI.  This is called from main(), and when it
  exits the client will exit.
**************************************************************************/
void ui_main(int argc, char *argv[])
{
  SDL_Event __Net_User_Event;
  SDL_Event __Anim_User_Event;
  SDL_Event __Info_User_Event;
  SDL_Event __Flush_User_Event;
  SDL_Event __pMap_Scroll_User_Event;
  Uint32 flags = 0;

  parse_options(argc, argv);

  if (!gui_options.gui_sdl2_migrated_from_sdl) {
    migrate_options_from_sdl();
  }

  if (gui_options.gui_sdl2_fullscreen) {
    flags |= SDL_WINDOW_FULLSCREEN;
  } else {
    flags &= ~SDL_WINDOW_FULLSCREEN;
  }
  log_normal(_("Using Video Output: %s"), SDL_GetCurrentVideoDriver());
  set_video_mode(gui_options.gui_sdl2_screen.width,
                 gui_options.gui_sdl2_screen.height,
                 flags);

  user_event_type = SDL_RegisterEvents(1);

  SDL_zero(__Net_User_Event);
  __Net_User_Event.type = user_event_type;
  __Net_User_Event.user.code = NET;
  __Net_User_Event.user.data1 = NULL;
  __Net_User_Event.user.data2 = NULL;
  pNet_User_Event = &__Net_User_Event;

  SDL_zero(__Anim_User_Event);
  __Anim_User_Event.type = user_event_type;
  __Anim_User_Event.user.code = EVENT_ERROR;
  __Anim_User_Event.user.data1 = NULL;
  __Anim_User_Event.user.data2 = NULL;
  pAnim_User_Event = &__Anim_User_Event;

  SDL_zero(__Info_User_Event);
  __Info_User_Event.type = user_event_type;
  __Info_User_Event.user.code = SHOW_WIDGET_INFO_LABEL;
  __Info_User_Event.user.data1 = NULL;
  __Info_User_Event.user.data2 = NULL;
  pInfo_User_Event = &__Info_User_Event;

  SDL_zero(__Flush_User_Event);
  __Flush_User_Event.type = user_event_type;
  __Flush_User_Event.user.code = FLUSH;
  __Flush_User_Event.user.data1 = NULL;
  __Flush_User_Event.user.data2 = NULL;
  flush_event = &__Flush_User_Event;

  SDL_zero(__pMap_Scroll_User_Event);
  __pMap_Scroll_User_Event.type = user_event_type;
  __pMap_Scroll_User_Event.user.code = MAP_SCROLL;
  __pMap_Scroll_User_Event.user.data1 = NULL;
  __pMap_Scroll_User_Event.user.data2 = NULL;
  pMap_Scroll_User_Event = &__pMap_Scroll_User_Event;

  is_unit_move_blocked = FALSE;

  SDL_Client_Flags |= (CF_DRAW_PLAYERS_NEUTRAL_STATUS
                       |CF_DRAW_PLAYERS_WAR_STATUS
                       |CF_DRAW_PLAYERS_CEASEFIRE_STATUS
                       |CF_DRAW_PLAYERS_PEACE_STATUS
                       |CF_DRAW_PLAYERS_ALLIANCE_STATUS);

  tileset_init(tileset);
  tileset_load_tiles(tileset);
  tileset_use_preferred_theme(tileset);

  load_cursors();

  callbacks = callback_list_new();

  diplomacy_dialog_init();
  intel_dialog_init();

  clear_double_messages_call();

  setup_auxiliary_tech_icons();

  /* this need correct Main.screen size */
  init_mapcanvas_and_overview();

  set_client_state(C_S_DISCONNECTED);

  /* Main game loop */
  gui_event_loop(NULL, NULL, main_key_down_handler, main_key_up_handler, NULL,
                 main_mouse_button_down_handler, main_mouse_button_up_handler,
                 main_mouse_motion_handler);
  start_quitting();
}

/**********************************************************************//**
  Do any necessary UI-specific cleanup
**************************************************************************/
void ui_exit()
{

#if defined UNDER_CE && defined SMALL_SCREEN
  /* change back to window mode to restore the title bar */
  set_video_mode(320, 240, SDL_SWSURFACE | SDL_ANYFORMAT);
#endif

  free_mapcanvas_and_overview();

  free_auxiliary_tech_icons();
  free_intro_radar_sprites();

  diplomacy_dialog_done();
  intel_dialog_done();

  callback_list_destroy(callbacks);

  unload_cursors();

  FC_FREE(button_behavior.event);

  meswin_dialog_popdown();

  del_main_list();

  free_font_system();
  theme_free(theme);
  theme = NULL;

  quit_sdl();
}

/**********************************************************************//**
  Return our GUI type
**************************************************************************/
enum gui_type get_gui_type(void)
{
  return GUI_SDL2;
}

/**********************************************************************//**
  Make a bell noise (beep).  This provides low-level sound alerts even
  if there is no real sound support.
**************************************************************************/
void sound_bell(void)
{
  log_debug("sound_bell : PORT ME");
}

/**********************************************************************//**
  Show Focused Unit Animation.
**************************************************************************/
void enable_focus_animation(void)
{
  pAnim_User_Event->user.code = ANIM;
  SDL_Client_Flags |= CF_FOCUS_ANIMATION;
}

/**********************************************************************//**
  Don't show Focused Unit Animation.
**************************************************************************/
void disable_focus_animation(void)
{
  SDL_Client_Flags &= ~CF_FOCUS_ANIMATION;
}

/**********************************************************************//**
  Wait for data on the given socket.  Call input_from_server() when data
  is ready to be read.
**************************************************************************/
void add_net_input(int sock)
{
  log_debug("Connection UP (%d)", sock);
  net_socket = sock;
  autoconnect = FALSE;
  enable_focus_animation();
}

/**********************************************************************//**
  Stop waiting for any server network data.  See add_net_input().
**************************************************************************/
void remove_net_input(void)
{
  log_debug("Connection DOWN... ");
  net_socket = (-1);
  disable_focus_animation();
  draw_goto_patrol_lines = FALSE;
  update_mouse_cursor(CURSOR_DEFAULT);
}

/**********************************************************************//**
  Enqueue a callback to be called during an idle moment.  The 'callback'
  function should be called sometimes soon, and passed the 'data' pointer
  as its data.
**************************************************************************/
void add_idle_callback(void (callback)(void *), void *data)
{
  struct callback *cb = fc_malloc(sizeof(*cb));

  cb->callback = callback;
  cb->data = data;

  callback_list_prepend(callbacks, cb);
}

/**********************************************************************//**
  Stub for editor function
**************************************************************************/
void editgui_tileset_changed(void)
{}

/**********************************************************************//**
  Stub for editor function
**************************************************************************/
void editgui_refresh(void)
{}

/**********************************************************************//**
  Stub for editor function
**************************************************************************/
void editgui_popup_properties(const struct tile_list *tiles, int objtype)
{}

/**********************************************************************//**
  Stub for editor function
**************************************************************************/
void editgui_popdown_all(void)
{}

/**********************************************************************//**
  Stub for editor function
**************************************************************************/
void editgui_notify_object_changed(int objtype, int object_id, bool removal)
{}

/**********************************************************************//**
  Stub for editor function
**************************************************************************/
void editgui_notify_object_created(int tag, int id)
{}

/**********************************************************************//**
  Updates a gui font style.
**************************************************************************/
void gui_update_font(const char *font_name, const char *font_value)
{
#define CHECK_FONT(client_font, action) \
  do { \
    if (strcmp(#client_font, font_name) == 0) { \
      char *end; \
      long size = strtol(font_value, &end, 10); \
      if (end && *end == '\0' && size > 0) { \
        *client_font_sizes[client_font] = size; \
        action; \
      } \
    } \
  } while(0)

  CHECK_FONT(FONT_CITY_NAME, update_city_descriptions());
  CHECK_FONT(FONT_CITY_PROD, update_city_descriptions());
  /* FONT_REQTREE_TEXT not used yet */

#undef CHECK_FONT
}

/**********************************************************************//**
  Insert build information to help
**************************************************************************/
void insert_client_build_info(char *outbuf, size_t outlen)
{
  /* PORTME */
}

/**********************************************************************//**
  Make dynamic adjustments to first-launch default options.
**************************************************************************/
void adjust_default_options(void)
{
  /* Nothing in case of this gui */
}
