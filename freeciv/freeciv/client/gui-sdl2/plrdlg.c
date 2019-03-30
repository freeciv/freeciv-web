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

/* SDL2 */
#ifdef SDL2_PLAIN_INCLUDE
#include <SDL.h>
#else  /* SDL2_PLAIN_INCLUDE */
#include <SDL2/SDL.h>
#endif /* SDL2_PLAIN_INCLUDE */

/* utility */
#include "astring.h"
#include "fcintl.h"

/* client */
#include "client_main.h"
#include "climisc.h"

/* gui-sdl2 */
#include "colors.h"
#include "diplodlg.h"
#include "graphics.h"
#include "gui_id.h"
#include "gui_main.h"
#include "gui_tilespec.h"
#include "inteldlg.h"
#include "mapview.h"
#include "sprite.h"
#include "themespec.h"
#include "widget.h"

#include "plrdlg.h"

#ifndef M_PI
#define M_PI		3.14159265358979323846	/* pi */
#endif
#ifndef M_PI_2
#define M_PI_2		1.57079632679489661923	/* pi/2 */
#endif

static struct SMALL_DLG  *pPlayers_Dlg = NULL;

/**********************************************************************//**
  User interacted with player dialog close button.
**************************************************************************/
static int exit_players_dlg_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    popdown_players_dialog();
    flush_dirty();
  }

  return -1;
}

/**********************************************************************//**
  User interacted with player widget.
**************************************************************************/
static int player_callback(struct widget *pWidget)
{
  struct player *pPlayer = pWidget->data.player;

  switch(Main.event.button.button) {
#if 0
      case SDL_BUTTON_LEFT:

        break;
      case SDL_BUTTON_MIDDLE:

        break;
#endif /* 0 */
    case SDL_BUTTON_RIGHT:
      if (can_intel_with_player(pPlayer)) {
	popdown_players_dialog();
        popup_intel_dialog(pPlayer);
	return -1;
      }
    break;
    default:
      popdown_players_dialog();
      popup_diplomacy_dialog(pPlayer);
      return -1;
    break;
  }

  return -1;
}

/**********************************************************************//**
  User interacted with player dialog window.
**************************************************************************/
static int players_window_dlg_callback(struct widget *pWindow)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (move_window_group_dialog(pPlayers_Dlg->pBeginWidgetList, pWindow)) {
      select_window_group_dialog(pPlayers_Dlg->pBeginWidgetList, pWindow);
      players_dialog_update();
    } else {
      if (select_window_group_dialog(pPlayers_Dlg->pBeginWidgetList, pWindow)) {
        widget_flush(pWindow);
      }
    }
  }

  return -1;
}

/**********************************************************************//**
  User interacted with 'draw war status' toggle.
**************************************************************************/
static int toggle_draw_war_status_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    /* exit button -> neutral -> war -> casefire -> peace -> alliance */
    struct widget *pPlayer = pPlayers_Dlg->pEndWidgetList->prev->prev->prev->prev->prev->prev;

    SDL_Client_Flags ^= CF_DRAW_PLAYERS_WAR_STATUS;
    do {
      pPlayer = pPlayer->prev;
      FREESURFACE(pPlayer->gfx);
    } while (pPlayer != pPlayers_Dlg->pBeginWidgetList);

    players_dialog_update();
  }

  return -1;
}

/**********************************************************************//**
  User interacted with 'draw cease-fire status' toggle.
**************************************************************************/
static int toggle_draw_ceasefire_status_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    /* exit button -> neutral -> war -> casefire -> peace -> alliance */
    struct widget *pPlayer = pPlayers_Dlg->pEndWidgetList->prev->prev->prev->prev->prev->prev;

    SDL_Client_Flags ^= CF_DRAW_PLAYERS_CEASEFIRE_STATUS;
    do {
      pPlayer = pPlayer->prev;
      FREESURFACE(pPlayer->gfx);
    } while (pPlayer != pPlayers_Dlg->pBeginWidgetList);

    players_dialog_update();
  }

  return -1;
}

/**********************************************************************//**
  User interacted with 'draw peace status' toggle.
**************************************************************************/
static int toggle_draw_peace_status_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    /* exit button -> neutral -> war -> casefire -> peace -> alliance */
    struct widget *pPlayer = pPlayers_Dlg->pEndWidgetList->prev->prev->prev->prev->prev->prev;

    SDL_Client_Flags ^= CF_DRAW_PLAYERS_PEACE_STATUS;
    do {
      pPlayer = pPlayer->prev;
      FREESURFACE(pPlayer->gfx);
    } while (pPlayer != pPlayers_Dlg->pBeginWidgetList);

    players_dialog_update();
  }

  return -1;
}

/**********************************************************************//**
  User interacted with 'draw alliance status' toggle.
**************************************************************************/
static int toggle_draw_alliance_status_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    /* exit button -> neutral -> war -> casefire -> peace -> alliance */
    struct widget *pPlayer = pPlayers_Dlg->pEndWidgetList->prev->prev->prev->prev->prev->prev;

    SDL_Client_Flags ^= CF_DRAW_PLAYERS_ALLIANCE_STATUS;
    do {
      pPlayer = pPlayer->prev;
      FREESURFACE(pPlayer->gfx);
    } while (pPlayer != pPlayers_Dlg->pBeginWidgetList);

    players_dialog_update();
  }

  return -1;
}

/**********************************************************************//**
  User interacted with 'draw neutral status' toggle.
**************************************************************************/
static int toggle_draw_neutral_status_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    /* exit button -> neutral -> war -> casefire -> peace -> alliance */
    struct widget *pPlayer = pPlayers_Dlg->pEndWidgetList->prev->prev->prev->prev->prev->prev;

    SDL_Client_Flags ^= CF_DRAW_PLAYERS_NEUTRAL_STATUS;
    do {
      pPlayer = pPlayer->prev;
      FREESURFACE(pPlayer->gfx);
    } while (pPlayer != pPlayers_Dlg->pBeginWidgetList);

    players_dialog_update();
  }

  return -1;
}

/**********************************************************************//**
  Does the attached player have embassy-level information about the player.
**************************************************************************/
static bool have_diplomat_info_about(struct player *pPlayer)
{
  return (pPlayer == client.conn.playing
          || (pPlayer != client.conn.playing
              && player_has_embassy(client.conn.playing, pPlayer)));
}

/**********************************************************************//**
  Update all information in the player list dialog.
**************************************************************************/
void real_players_dialog_update(void *unused)
{
  if (pPlayers_Dlg) {
    struct widget *pPlayer0, *pPlayer1;
    struct player *pPlayer;
    int i;
    struct astring astr = ASTRING_INIT;

    /* redraw window */
    widget_redraw(pPlayers_Dlg->pEndWidgetList);

    /* exit button -> neutral -> war -> casefire -> peace -> alliance */
    pPlayer0 = pPlayers_Dlg->pEndWidgetList->prev->prev->prev->prev->prev->prev;
    do {
      pPlayer0 = pPlayer0->prev;
      pPlayer1 = pPlayer0;
      pPlayer = pPlayer0->data.player;

      for (i = 0; i < num_player_dlg_columns; i++) {
        if (player_dlg_columns[i].show) {
          switch (player_dlg_columns[i].type) {
            case COL_TEXT:
            case COL_RIGHT_TEXT:
              astr_add_line(&astr, "%s: %s", player_dlg_columns[i].title,
                                             player_dlg_columns[i].func(pPlayer));
              break;
            case COL_BOOLEAN:
              astr_add_line(&astr, "%s: %s", player_dlg_columns[i].title,
                            player_dlg_columns[i].bool_func(pPlayer) ?
                              _("Yes") : _("No"));
              break;
            default:
              break;
          }
        }
      }

      copy_chars_to_utf8_str(pPlayer0->info_label, astr_str(&astr));

      astr_free(&astr);

      /* now add some eye candy ... */
      if (pPlayer1 != pPlayers_Dlg->pBeginWidgetList) {
        SDL_Rect dst0, dst1;

        dst0.x = pPlayer0->size.x + pPlayer0->size.w / 2;
        dst0.y = pPlayer0->size.y + pPlayer0->size.h / 2;

        do {
          pPlayer1 = pPlayer1->prev;
	  if (have_diplomat_info_about(pPlayer)
              || have_diplomat_info_about(pPlayer1->data.player)) {

            dst1.x = pPlayer1->size.x + pPlayer1->size.w / 2;
            dst1.y = pPlayer1->size.y + pPlayer1->size.h / 2;

            switch (player_diplstate_get(pPlayer,
                                         pPlayer1->data.player)->type) {
	      case DS_ARMISTICE:
	        if (SDL_Client_Flags & CF_DRAW_PLAYERS_NEUTRAL_STATUS) {
	          create_line(pPlayer1->dst->surface,
                              dst0.x, dst0.y, dst1.x, dst1.y,
                              get_theme_color(COLOR_THEME_PLRDLG_ARMISTICE));
	        }
	      break;
              case DS_WAR:
	        if (SDL_Client_Flags & CF_DRAW_PLAYERS_WAR_STATUS) {
	          create_line(pPlayer1->dst->surface,
                              dst0.x, dst0.y, dst1.x, dst1.y,
                              get_theme_color(COLOR_THEME_PLRDLG_WAR));
	        }
              break;
	      case DS_CEASEFIRE:
	        if (SDL_Client_Flags & CF_DRAW_PLAYERS_CEASEFIRE_STATUS) {
	          create_line(pPlayer1->dst->surface,
                              dst0.x, dst0.y, dst1.x, dst1.y,
                              get_theme_color(COLOR_THEME_PLRDLG_CEASEFIRE));
	        }
              break;
              case DS_PEACE:
	        if (SDL_Client_Flags & CF_DRAW_PLAYERS_PEACE_STATUS) {
	          create_line(pPlayer1->dst->surface,
                              dst0.x, dst0.y, dst1.x, dst1.y,
                              get_theme_color(COLOR_THEME_PLRDLG_PEACE));
	        }
              break;
	      case DS_ALLIANCE:
	        if (SDL_Client_Flags & CF_DRAW_PLAYERS_ALLIANCE_STATUS) {
	          create_line(pPlayer1->dst->surface,
                              dst0.x, dst0.y, dst1.x, dst1.y,
                              get_theme_color(COLOR_THEME_PLRDLG_ALLIANCE));
	        }
              break;
              default:
	        /* no contact */
              break;
	    }
	  }
        } while (pPlayer1 != pPlayers_Dlg->pBeginWidgetList);
      }
    } while (pPlayer0 != pPlayers_Dlg->pBeginWidgetList);

    /* -------------------- */
    /* redraw */
    redraw_group(pPlayers_Dlg->pBeginWidgetList,
                 pPlayers_Dlg->pEndWidgetList->prev, 0);
    widget_mark_dirty(pPlayers_Dlg->pEndWidgetList);

    flush_dirty();
  }
}

/**********************************************************************//**
  Popup (or raise) the player list dialog.
**************************************************************************/
void popup_players_dialog(bool raise)
{
  struct widget *pWindow = NULL, *pBuf = NULL;
  SDL_Surface *pLogo = NULL, *pZoomed = NULL;
  utf8_str *pstr;
  SDL_Rect dst;
  int i, n, h;
  double a, b, r;
  SDL_Rect area;

  if (pPlayers_Dlg) {
    return;
  }

  n = 0;
  players_iterate(pPlayer) {
    if (is_barbarian(pPlayer)) {
      continue;
    }
    n++;
  } players_iterate_end;

  if (n < 2) {
    return;
  }

  pPlayers_Dlg = fc_calloc(1, sizeof(struct SMALL_DLG));

  pstr = create_utf8_from_char(Q_("?header:Players"), adj_font(12));
  pstr->style |= TTF_STYLE_BOLD;

  pWindow = create_window_skeleton(NULL, pstr, 0);

  pWindow->action = players_window_dlg_callback;
  set_wstate(pWindow, FC_WS_NORMAL);

  add_to_gui_list(ID_WINDOW, pWindow);
  pPlayers_Dlg->pEndWidgetList = pWindow;
  /* ---------- */
  /* exit button */
  pBuf = create_themeicon(current_theme->Small_CANCEL_Icon, pWindow->dst,
                          WF_WIDGET_HAS_INFO_LABEL | WF_RESTORE_BACKGROUND);
  pBuf->info_label = create_utf8_from_char(_("Close Dialog (Esc)"),
                                           adj_font(12));
  pBuf->action = exit_players_dlg_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->key = SDLK_ESCAPE;

  add_to_gui_list(ID_BUTTON, pBuf);
  /* ---------- */

  for (i = 0; i < DS_LAST; i++) {
    switch (i) {
      case DS_ARMISTICE:
	pBuf = create_checkbox(pWindow->dst,
                               (SDL_Client_Flags & CF_DRAW_PLAYERS_NEUTRAL_STATUS),
                               WF_RESTORE_BACKGROUND);
	pBuf->action = toggle_draw_neutral_status_callback;
	pBuf->key = SDLK_n;
      break;
      case DS_WAR:
	pBuf = create_checkbox(pWindow->dst,
                               (SDL_Client_Flags & CF_DRAW_PLAYERS_WAR_STATUS),
                               WF_RESTORE_BACKGROUND);
	pBuf->action = toggle_draw_war_status_callback;
	pBuf->key = SDLK_w;
      break;
      case DS_CEASEFIRE:
	pBuf = create_checkbox(pWindow->dst,
                               (SDL_Client_Flags & CF_DRAW_PLAYERS_CEASEFIRE_STATUS),
                               WF_RESTORE_BACKGROUND);
	pBuf->action = toggle_draw_ceasefire_status_callback;
	pBuf->key = SDLK_c;
      break;
      case DS_PEACE:
	pBuf = create_checkbox(pWindow->dst,
                               (SDL_Client_Flags & CF_DRAW_PLAYERS_PEACE_STATUS),
                               WF_RESTORE_BACKGROUND);
	pBuf->action = toggle_draw_peace_status_callback;
	pBuf->key = SDLK_p;
      break;
      case DS_ALLIANCE:
	pBuf = create_checkbox(pWindow->dst,
                               (SDL_Client_Flags & CF_DRAW_PLAYERS_ALLIANCE_STATUS),
                               WF_RESTORE_BACKGROUND);
	pBuf->action = toggle_draw_alliance_status_callback;
	pBuf->key = SDLK_a;
      break;
      default:
	 /* no contact */
	 continue;
      break;
    }
    set_wstate(pBuf, FC_WS_NORMAL);
    add_to_gui_list(ID_CHECKBOX, pBuf);
  } 
  /* ---------- */

  players_iterate(pPlayer) {
    if (is_barbarian(pPlayer)) {
      continue;
    }

    pstr = create_utf8_str(NULL, 0, adj_font(10));
    pstr->style |= (TTF_STYLE_BOLD|SF_CENTER);

    pLogo = get_nation_flag_surface(nation_of_player(pPlayer));
    {
      /* Aim for a flag height of 60 pixels, but draw smaller flags if there
       * are more players */
      double zoom = DEFAULT_ZOOM * (60.0 - n) / pLogo->h;

      pZoomed = zoomSurface(pLogo, zoom, zoom, 1);
    }

    pBuf = create_icon2(pZoomed, pWindow->dst,
                        WF_RESTORE_BACKGROUND | WF_WIDGET_HAS_INFO_LABEL
                        | WF_FREE_THEME);
    pBuf->info_label = pstr;

    if (!pPlayer->is_alive) {
      pstr = create_utf8_from_char(_("R.I.P.") , adj_font(10));
      pstr->style |= TTF_STYLE_BOLD;
      pstr->fgcol = *get_theme_color(COLOR_THEME_PLRDLG_TEXT);
      pLogo = create_text_surf_from_utf8(pstr);
      FREEUTF8STR(pstr);

      dst.x = (pZoomed->w - pLogo->w) / 2;
      dst.y = (pZoomed->h - pLogo->h) / 2;
      alphablit(pLogo, NULL, pZoomed, &dst, 255);
      FREESURFACE(pLogo);
    }

    if (pPlayer->is_alive) {
      set_wstate(pBuf, FC_WS_NORMAL);
    }

    pBuf->data.player = pPlayer;

    pBuf->action = player_callback;

    add_to_gui_list(ID_LABEL, pBuf);

  } players_iterate_end;

  pPlayers_Dlg->pBeginWidgetList = pBuf;

  resize_window(pWindow, NULL, NULL, adj_size(500), adj_size(400));

  area = pWindow->area;

  r = MIN(area.w, area.h);
  r -= ((MAX(pBuf->size.w, pBuf->size.h) * 2));
  r /= 2;
  a = (2.0 * M_PI) / n;

  widget_set_position(pWindow,
                      (main_window_width() - pWindow->size.w) / 2,
                      (main_window_height() - pWindow->size.h) / 2);

  /* exit button */
  pBuf = pWindow->prev;

  pBuf->size.x = area.x + area.w - pBuf->size.w - 1;
  pBuf->size.y = pWindow->size.y + adj_size(2);

  n = area.y;
  pstr = create_utf8_str(NULL, 0, adj_font(10));
  pstr->style |= TTF_STYLE_BOLD;
  pstr->bgcol = (SDL_Color) {0, 0, 0, 0};

  for (i = 0; i < DS_LAST; i++) {
    switch (i) {
    case DS_ARMISTICE:
      pstr->fgcol = *get_theme_color(COLOR_THEME_PLRDLG_ARMISTICE);
      break;
    case DS_WAR:
      pstr->fgcol = *get_theme_color(COLOR_THEME_PLRDLG_WAR);
      break;
    case DS_CEASEFIRE:
      pstr->fgcol = *get_theme_color(COLOR_THEME_PLRDLG_CEASEFIRE);
      break;
    case DS_PEACE:
      pstr->fgcol = *get_theme_color(COLOR_THEME_PLRDLG_PEACE);
      break;
    case DS_ALLIANCE:
      pstr->fgcol = *get_theme_color(COLOR_THEME_PLRDLG_ALLIANCE);
      break;
    default:
      /* no contact */
      continue;
      break;
    }

    copy_chars_to_utf8_str(pstr, diplstate_type_translated_name(i));
    pLogo = create_text_surf_from_utf8(pstr);

    pBuf = pBuf->prev;
    h = MAX(pBuf->size.h, pLogo->h);
    pBuf->size.x = area.x + adj_size(5);
    pBuf->size.y = n + (h - pBuf->size.h) / 2;

    dst.x = adj_size(5) + pBuf->size.w + adj_size(6);
    dst.y = n + (h - pLogo->h) / 2;
    alphablit(pLogo, NULL, pWindow->theme, &dst, 255);
    n += h;
    FREESURFACE(pLogo);
  }
  FREEUTF8STR(pstr);

  /* first player shield */
  pBuf = pBuf->prev;
  pBuf->size.x = area.x + area.w / 2 - pBuf->size.w / 2;
  pBuf->size.y = area.y + area.h / 2 - r - pBuf->size.h / 2;

  n = 1;
  if (pBuf != pPlayers_Dlg->pBeginWidgetList) {
    do {
      pBuf = pBuf->prev;
      b = M_PI_2 + n * a;
      pBuf->size.x = area.x + area.w / 2 - r * cos(b) - pBuf->size.w / 2;
      pBuf->size.y = area.y + area.h / 2 - r * sin(b) - pBuf->size.h / 2;
      n++;
    } while (pBuf != pPlayers_Dlg->pBeginWidgetList);
  }

  players_dialog_update();
}

/**********************************************************************//**
  Popdown the player list dialog.
**************************************************************************/
void popdown_players_dialog(void)
{
  if (pPlayers_Dlg) {
    popdown_window_group_dialog(pPlayers_Dlg->pBeginWidgetList,
                                pPlayers_Dlg->pEndWidgetList);
    FC_FREE(pPlayers_Dlg);
  }
}


/* ============================== SHORT =============================== */
static struct ADVANCED_DLG  *pShort_Players_Dlg = NULL;

/**********************************************************************//**
  User interacted with nations window.
**************************************************************************/
static int players_nations_window_dlg_callback(struct widget *pWindow)
{
  return -1;
}

/**********************************************************************//**
  User interacted with nations window close button.
**************************************************************************/
static int exit_players_nations_dlg_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    popdown_players_nations_dialog();
    flush_dirty();
  }

  return -1;
}

/**********************************************************************//**
  User interacted with widget of a single nation/player.
**************************************************************************/
static int player_nation_callback(struct widget *pWidget)
{
  struct player *pPlayer = pWidget->data.player;

  popdown_players_nations_dialog();
  switch(Main.event.button.button) {
#if 0
  case SDL_BUTTON_LEFT:

    break;
  case SDL_BUTTON_MIDDLE:

    break;
#endif /* 0 */
  case SDL_BUTTON_RIGHT:
    if (can_intel_with_player(pPlayer)) {
      popup_intel_dialog(pPlayer);
    } else {
      flush_dirty();
    }
    break;
  default:
    if (pPlayer != client.conn.playing) {
      popup_diplomacy_dialog(pPlayer);
    }
    break;
  }

  return -1;
}

/**********************************************************************//**
  Popup (or raise) the short player list dialog version.
**************************************************************************/
void popup_players_nations_dialog(void)
{
  struct widget *pWindow = NULL, *pBuf = NULL;
  SDL_Surface *pLogo = NULL;
  utf8_str *pstr;
  char cBuf[128], *state;
  int n = 0, w = 0, units_h = 0;
  const struct player_diplstate *pDS;
  SDL_Rect area;

  if (pShort_Players_Dlg) {
    return;
  }

  pShort_Players_Dlg = fc_calloc(1, sizeof(struct ADVANCED_DLG));

  /* TRANS: Nations report title */
  pstr = create_utf8_from_char(_("Nations") , adj_font(12));
  pstr->style |= TTF_STYLE_BOLD;

  pWindow = create_window_skeleton(NULL, pstr, 0);

  pWindow->action = players_nations_window_dlg_callback;
  set_wstate(pWindow, FC_WS_NORMAL);

  add_to_gui_list(ID_WINDOW, pWindow);
  pShort_Players_Dlg->pEndWidgetList = pWindow;

  area = pWindow->area;

  /* ---------- */
  /* exit button */
  pBuf = create_themeicon(current_theme->Small_CANCEL_Icon, pWindow->dst,
                          WF_WIDGET_HAS_INFO_LABEL | WF_RESTORE_BACKGROUND);
  pBuf->info_label = create_utf8_from_char(_("Close Dialog (Esc)"),
                                           adj_font(12));
  area.w = MAX(area.w, pBuf->size.w + adj_size(10));
  pBuf->action = exit_players_nations_dlg_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->key = SDLK_ESCAPE;

  add_to_gui_list(ID_BUTTON, pBuf);
  /* ---------- */

  players_iterate(pPlayer) {
    if (pPlayer != client.conn.playing) {
      if (!pPlayer->is_alive || is_barbarian(pPlayer)) {
        continue;
      }

      pDS = player_diplstate_get(client.conn.playing, pPlayer);

      if (is_ai(pPlayer)) {
	state = _("AI");
      } else {
        if (pPlayer->is_connected) {
          if (pPlayer->phase_done) {
      	    state = _("done");
          } else {
      	    state = _("moving");
          }
        } else {
          state = _("disconnected");
        }
      }

      if (pDS->type == DS_CEASEFIRE) {
	fc_snprintf(cBuf, sizeof(cBuf), "%s(%s) - %d %s",
                    nation_adjective_for_player(pPlayer),
                    state,
                    pDS->turns_left, PL_("turn", "turns", pDS->turns_left));
      } else {
	fc_snprintf(cBuf, sizeof(cBuf), "%s(%s)",
                    nation_adjective_for_player(pPlayer),
                    state);
      }

      pstr = create_utf8_from_char(cBuf, adj_font(10));
      pstr->style |= TTF_STYLE_BOLD;

      pLogo = get_nation_flag_surface(nation_of_player(pPlayer));

      pBuf = create_iconlabel(pLogo, pWindow->dst, pstr,
                              (WF_RESTORE_BACKGROUND|WF_DRAW_TEXT_LABEL_WITH_SPACE));

      /* now add some eye candy ... */
      switch (pDS->type) {
      case DS_ARMISTICE:
        pBuf->string_utf8->fgcol = *get_theme_color(COLOR_THEME_PLRDLG_ARMISTICE);
        set_wstate(pBuf, FC_WS_NORMAL);
        break;
      case DS_WAR:
        if (can_meet_with_player(pPlayer) || can_intel_with_player(pPlayer)) {
          set_wstate(pBuf, FC_WS_NORMAL);
          pBuf->string_utf8->fgcol = *get_theme_color(COLOR_THEME_PLRDLG_WAR);
        } else {
          pBuf->string_utf8->fgcol = *(get_theme_color(COLOR_THEME_PLRDLG_WAR_RESTRICTED));
        }
        break;
      case DS_CEASEFIRE:
        pBuf->string_utf8->fgcol = *get_theme_color(COLOR_THEME_PLRDLG_CEASEFIRE);
        set_wstate(pBuf, FC_WS_NORMAL);
        break;
      case DS_PEACE:
        pBuf->string_utf8->fgcol = *get_theme_color(COLOR_THEME_PLRDLG_PEACE);
        set_wstate(pBuf, FC_WS_NORMAL);
        break;
      case DS_ALLIANCE:
        pBuf->string_utf8->fgcol = *get_theme_color(COLOR_THEME_PLRDLG_ALLIANCE);
        set_wstate(pBuf, FC_WS_NORMAL);
        break;
      case DS_NO_CONTACT:
        pBuf->string_utf8->fgcol = *(get_theme_color(COLOR_THEME_WIDGET_DISABLED_TEXT));
	break;
      default:
        set_wstate(pBuf, FC_WS_NORMAL);
        break;
      }

      pBuf->string_utf8->bgcol = (SDL_Color) {0, 0, 0, 0};

      pBuf->data.player = pPlayer;

      pBuf->action = player_nation_callback;

      add_to_gui_list(ID_LABEL, pBuf);

      area.w = MAX(w, pBuf->size.w);
      area.h += pBuf->size.h;

      if (n > 19) {
        set_wflag(pBuf, WF_HIDDEN);
      }

      n++;
    }
  } players_iterate_end;
  pShort_Players_Dlg->pBeginWidgetList = pBuf;
  pShort_Players_Dlg->pBeginActiveWidgetList = pShort_Players_Dlg->pBeginWidgetList;
  pShort_Players_Dlg->pEndActiveWidgetList = pWindow->prev->prev;
  pShort_Players_Dlg->pActiveWidgetList = pShort_Players_Dlg->pEndActiveWidgetList;


  /* ---------- */
  if (n > 20) {
    units_h = create_vertical_scrollbar(pShort_Players_Dlg, 1, 20, TRUE, TRUE);
    pShort_Players_Dlg->pScroll->count = n;

    n = units_h;
    area.w += n;

    units_h = 20 * pBuf->size.h;

  } else {
    units_h = area.h;
  }

  /* ---------- */

  area.h = units_h;

  resize_window(pWindow, NULL, NULL,
                (pWindow->size.w - pWindow->area.w) + area.w,
                (pWindow->size.h + pWindow->area.h) + area.h);

  area = pWindow->area;

  widget_set_position(pWindow,
    ((Main.event.motion.x + pWindow->size.w + adj_size(10) < main_window_width()) ?
      (Main.event.motion.x + adj_size(10)) :
      (main_window_width() - pWindow->size.w - adj_size(10))),
    ((Main.event.motion.y - adj_size(2) + pWindow->size.h < main_window_height()) ?
      (Main.event.motion.y - adj_size(2)) :
      (main_window_height() - pWindow->size.h - adj_size(10))));

  w = area.w;

  if (pShort_Players_Dlg->pScroll) {
    w -= n;
  }

  /* exit button */
  pBuf = pWindow->prev;
  pBuf->size.x = area.x + area.w - pBuf->size.w - 1;
  pBuf->size.y = pWindow->size.y + adj_size(2);

  /* cities */
  pBuf = pBuf->prev;
  setup_vertical_widgets_position(1, area.x, area.y, w, 0,
                                  pShort_Players_Dlg->pBeginActiveWidgetList,
                                  pBuf);

  if (pShort_Players_Dlg->pScroll) {
    setup_vertical_scrollbar_area(pShort_Players_Dlg->pScroll,
                                  area.x + area.w, area.y,
                                  area.h, TRUE);
  }

  /* -------------------- */
  /* redraw */
  redraw_group(pShort_Players_Dlg->pBeginWidgetList, pWindow, 0);
  widget_mark_dirty(pWindow);

  flush_dirty();
}

/**********************************************************************//**
  Popdown the short player list dialog version.
**************************************************************************/
void popdown_players_nations_dialog(void)
{
  if (pShort_Players_Dlg) {
    popdown_window_group_dialog(pShort_Players_Dlg->pBeginWidgetList,
                                pShort_Players_Dlg->pEndWidgetList);
    FC_FREE(pShort_Players_Dlg->pScroll);
    FC_FREE(pShort_Players_Dlg);
  }
}
