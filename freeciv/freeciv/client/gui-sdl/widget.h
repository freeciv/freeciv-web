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

/**********************************************************************
                          gui_stuff.h  -  description
                             -------------------
    begin                : June 30 2002
    copyright            : (C) 2002 by Rafał Bursig
    email                : Rafał Bursig <bursig@poczta.fm>
 **********************************************************************/

#ifndef FC__WIDGET_H
#define FC__WIDGET_H

/* utility */
#include "fc_types.h"

/* gui-sdl */
#include "gui_main.h"
#include "gui_string.h"

#ifdef SMALL_SCREEN
#define	WINDOW_TITLE_HEIGHT	10
#else
#define	WINDOW_TITLE_HEIGHT	20
#endif

#define MAX_ID			0xFFFF

/* Widget Types */
enum widget_type {			/* allow 64 widgets type */
  WT_BUTTON	 = 4,		/* Button with Text (not use !!!)
				   ( can be transparent ) */
  WT_I_BUTTON	 = 8,		/* Button with TEXT and ICON
				   ( static; can't be transp. ) */
  WT_TI_BUTTON	= 12,		/* Button with TEXT and ICON
				   (themed; can't be transp. ) */
  WT_EDIT	= 16,		/* edit field   */
  WT_ICON	= 20,		/* flat Button from 4 - state icon */
  WT_VSCROLLBAR = 24,		/* bugy */
  WT_HSCROLLBAR = 28,		/* bugy */
  WT_WINDOW	= 32,
  WT_T_LABEL	= 36,		/* text label with theme backgroud */
  WT_I_LABEL	= 40,		/* text label with icon */
  WT_TI_LABEL	= 44,		/* text label with icon and theme backgroud.
				   NOTE: Not DEFINED- don't use
				   ( can't be transp. ) */
  WT_CHECKBOX	= 48,		/* checkbox. */
  WT_TCHECKBOX	= 52,		/* text label with checkbox. */
  WT_ICON2	= 56,		/* flat Button from 1 - state icon */
  WT_T2_LABEL	= 60
};

/* Widget FLAGS -> allowed 20 flags */
/* default: ICON_CENTER_Y, ICON_ON_LEFT */
enum widget_flag {
  WF_HIDDEN				= (1<<10),
  /* widget->gfx may be freed together with the widget */
  WF_FREE_GFX	 			= (1<<11),
  /* widget->theme may be freed  together with the widget*/
  WF_FREE_THEME				= (1<<12),
  /* widget->theme2 may be freed  together with the widget*/
  WF_FREE_THEME2                        = (1<<13),
  /* widget->string may be freed  together with the widget*/
  WF_FREE_STRING			= (1<<14),
  /* widget->data may be freed  together with the widget*/
  WF_FREE_DATA			 	= (1<<15),
  /* widget->private_data may be freed  together with the widget*/
  WF_FREE_PRIVATE_DATA			= (1<<16),
  WF_ICON_ABOVE_TEXT			= (1<<17),
  WF_ICON_UNDER_TEXT			= (1<<18),
  WF_ICON_CENTER			= (1<<19),
  WF_ICON_CENTER_RIGHT			= (1<<20),
  WF_RESTORE_BACKGROUND	                = (1<<21),
  WF_DRAW_FRAME_AROUND_WIDGET	 	= (1<<22),
  WF_DRAW_TEXT_LABEL_WITH_SPACE		= (1<<23),
  WF_WIDGET_HAS_INFO_LABEL		= (1<<24),
  WF_SELLECT_WITHOUT_BAR		= (1<<25),
  WF_PASSWD_EDIT			= (1<<26),
  WF_EDIT_LOOP				= (1<<27)
};

/* Widget states */
enum widget_state {
  FC_WS_NORMAL		= 0,
  FC_WS_SELLECTED	= 1,
  FC_WS_PRESSED		= 2,
  FC_WS_DISABLED	= 3
};

struct CONTAINER {
  int id0;
  int id1;
  int value;
};

struct SMALL_DLG;
struct ADVANCED_DLG;
struct CHECKBOX;
  
struct widget {
  struct widget *next;
  struct widget *prev;
    
  struct gui_layer *dst;      /* destination buffer layer */

  SDL_Surface *theme;
  SDL_Surface *theme2;        /* Icon or theme2 */
  SDL_Surface *gfx;           /* saved background */
  SDL_String16 *string16;
  
  /* data/information/transport pointers */
  union {
    struct CONTAINER *cont;
    struct city *city;
    struct unit *unit;
    struct player *player;
    void *ptr;
  } data;
  
  union {
    struct CHECKBOX *cbox;
    struct SMALL_DLG *small_dlg;
    struct ADVANCED_DLG *adv_dlg;
    void *ptr;
  } private_data;
  
  Uint32 state_types_flags;	/* "packed" widget info */

  SDL_Rect size;		/* size.w and size.h are the size of widget   
				   size.x and size.y are the draw pozitions. */
  
  SDL_Rect area;                /* position and size of the area the widget resides in */
  
  SDLKey key;			/* key alliased with this widget */
  Uint16 mod;			/* SHIFT, CTRL, ALT, etc */
  Uint16 ID;			/* ID in widget list */

  int (*action) (struct widget *);	/* default callback action */  

  void (*set_area) (struct widget *pwidget, SDL_Rect area);  
  void (*set_position) (struct widget *pwidget, int x, int y);
  void (*resize) (struct widget *pwidget, int w, int h);
  void (*draw_frame) (struct widget *pwidget);
  int (*redraw) (struct widget *pwidget);
  void (*mark_dirty) (struct widget *pwidget);
  void (*flush) (struct widget *pwidget);
  void (*undraw) (struct widget *pwidget);
  void (*select) (struct widget *pwidget);
  void (*unselect) (struct widget *pwidget);
  void (*press) (struct widget *pwidget);
};

/* Struct of basic window group dialog ( without scrollbar ) */
struct SMALL_DLG {
  struct widget *pBeginWidgetList;
  struct widget *pEndWidgetList;	/* window */
};

/* Struct of advenced window group dialog ( with scrollbar ) */
struct ADVANCED_DLG {
  struct widget *pBeginWidgetList;
  struct widget *pEndWidgetList;/* window */
    
  struct widget *pBeginActiveWidgetList;
  struct widget *pEndActiveWidgetList;
  struct widget *pActiveWidgetList; /* first seen widget */
  struct ScrollBar *pScroll;
};

enum scan_direction {
  SCAN_FORWARD,
  SCAN_BACKWARD
};

void add_to_gui_list(Uint16 ID, struct widget *pGUI);
void del_widget_pointer_from_gui_list(struct widget *pGUI);
void DownAdd(struct widget *pNew_Widget, struct widget *pAdd_Dock);
  
bool is_this_widget_first_on_list(struct widget *pGUI);
void move_widget_to_front_of_gui_list(struct widget *pGUI);

void del_gui_list(struct widget *pGUI_List);
void del_main_list(void);

struct widget *find_next_widget_at_pos(struct widget *pStartWidget, int x, int y);
struct widget *find_next_widget_for_key(struct widget *pStartWidget, SDL_keysym key);

struct widget *get_widget_pointer_form_ID(const struct widget *pGUI_List, Uint16 ID,
                                       enum scan_direction direction);

struct widget *get_widget_pointer_form_main_list(Uint16 ID);

#define set_action(ID, action_callback)	\
	get_widget_pointer_form_main_list(ID)->action = action_callback

#define set_key(ID, keyb)	\
	get_widget_pointer_form_main_list(ID)->key = keyb

#define set_mod(ID, mod)	\
	get_widget_pointer_form_main_list(ID)->mod = mod

#define enable(ID)						\
do {								\
  struct widget *____pGUI = get_widget_pointer_form_main_list(ID);	\
  set_wstate(____pGUI, FC_WS_NORMAL);				\
} while(0)

#define disable(ID)						\
do {								\
  struct widget *____pGUI = get_widget_pointer_form_main_list(ID);	\
  set_wstate(____pGUI , FC_WS_DISABLED);				\
} while(0)

#define show(ID)	\
  clear_wflag( get_widget_pointer_form_main_list(ID), WF_HIDDEN)

#define hide(ID)	\
  set_wflag(get_widget_pointer_form_main_list(ID), WF_HIDDEN)

void widget_sellected_action(struct widget *pWidget);
Uint16 widget_pressed_action(struct widget *pWidget);

void unsellect_widget_action(void);

#define draw_widget_info_label() redraw_widget_info_label(NULL);
void redraw_widget_info_label(SDL_Rect *area);

/* Widget */
void set_wstate(struct widget *pWidget, enum widget_state state);
enum widget_state get_wstate(const struct widget *pWidget);
  
void set_wtype(struct widget *pWidget, enum widget_type type);
enum widget_type get_wtype(const struct widget *pWidget);
  
void set_wflag(struct widget *pWidget, enum widget_flag flag);
void clear_wflag(struct widget *pWidget, enum widget_flag flag);
  
enum widget_flag get_wflags(const struct widget *pWidget);

static inline void widget_set_area(struct widget *pwidget, SDL_Rect area) {
  pwidget->set_area(pwidget, area);
}

static inline void widget_set_position(struct widget *pwidget, int x, int y) {
  pwidget->set_position(pwidget, x, y);
}

static inline void widget_resize(struct widget *pwidget, int w, int h) {
  pwidget->resize(pwidget, w, h);
}

static inline int widget_redraw(struct widget *pWidget) {
  return pWidget->redraw(pWidget);
}

static inline void widget_draw_frame(struct widget *pwidget) {
  pwidget->draw_frame(pwidget);
}

static inline void widget_mark_dirty(struct widget *pwidget) {
  pwidget->mark_dirty(pwidget);
}

static inline void widget_flush(struct widget *pwidget) {
  pwidget->flush(pwidget);
}

static inline void widget_undraw(struct widget *pwidget) {
  pwidget->undraw(pwidget);
}

void widget_free(struct widget **pGUI);

void refresh_widget_background(struct widget *pWidget);

#define FREEWIDGET(pWidget)					\
do {								\
  widget_free(&pWidget);                                        \
} while(0)

#define draw_frame_inside_widget_on_surface(pWidget , pDest)		\
do {                                                                    \
  draw_frame(pDest, pWidget->size.x, pWidget->size.y, pWidget->size.w, pWidget->size.h);  \
} while(0);

#define draw_frame_inside_widget(pWidget)				\
	draw_frame_inside_widget_on_surface(pWidget , pWidget->dst->surface)

#define draw_frame_around_widget_on_surface(pWidget , pDest)		\
do {                                                                    \
  draw_frame(pDest, pWidget->size.x - pTheme->FR_Left->w, pWidget->size.y - pTheme->FR_Top->h, \
             pWidget->size.w + pTheme->FR_Left->w + pTheme->FR_Right->w,\
             pWidget->size.h + pTheme->FR_Top->h + pTheme->FR_Bottom->h);  \
} while(0);

#define draw_frame_around_widget(pWidget)				\
	draw_frame_around_widget_on_surface(pWidget , pWidget->dst->surface)

/* Group */
Uint16 redraw_group(const struct widget *pBeginGroupWidgetList,
		    const struct widget *pEndGroupWidgetList,
		    int add_to_update);

void undraw_group(struct widget *pBeginGroupWidgetList,
                  struct widget *pEndGroupWidgetList);
		    
void set_new_group_start_pos(const struct widget *pBeginGroupWidgetList,
			     const struct widget *pEndGroupWidgetList,
			     Sint16 Xrel, Sint16 Yrel);
void del_group_of_widgets_from_gui_list(struct widget *pBeginGroupWidgetList,
					struct widget *pEndGroupWidgetList);
void move_group_to_front_of_gui_list(struct widget *pBeginGroupWidgetList,
				     struct widget *pEndGroupWidgetList);

void set_group_state(struct widget *pBeginGroupWidgetList,
		     struct widget *pEndGroupWidgetList, enum widget_state state);

void show_group(struct widget *pBeginGroupWidgetList,
		struct widget *pEndGroupWidgetList);

void hide_group(struct widget *pBeginGroupWidgetList,
		struct widget *pEndGroupWidgetList);

void group_set_area(struct widget *pBeginGroupWidgetList,
		    struct widget *pEndGroupWidgetList,
                    SDL_Rect area);

/* Window Group */
void popdown_window_group_dialog(struct widget *pBeginGroupWidgetList,
				 struct widget *pEndGroupWidgetList);

bool sellect_window_group_dialog(struct widget *pBeginWidgetList,
				struct widget *pWindow);
bool move_window_group_dialog(struct widget *pBeginGroupWidgetList,
			     struct widget *pEndGroupWidgetList);
void move_window_group(struct widget *pBeginWidgetList, struct widget *pWindow);
				      
int setup_vertical_widgets_position(int step,
	Sint16 start_x, Sint16 start_y, Uint16 w, Uint16 h,
				struct widget *pBegin, struct widget *pEnd);

#define del_widget_from_gui_list(__pGUI)	\
do {						\
  del_widget_pointer_from_gui_list(__pGUI);	\
  FREEWIDGET(__pGUI);				\
} while(0)

#define del_ID_from_gui_list(ID)				\
do {								\
  struct widget *___pTmp = get_widget_pointer_form_main_list(ID);	\
  del_widget_pointer_from_gui_list(___pTmp);			\
  FREEWIDGET(___pTmp);						\
} while(0)

#define move_ID_to_front_of_gui_list(ID)	\
	move_widget_to_front_of_gui_list(       \
          get_widget_pointer_form_main_list(ID))

#define del_group(pBeginGroupWidgetList, pEndGroupWidgetList)		\
do {									\
  del_group_of_widgets_from_gui_list(pBeginGroupWidgetList,		\
					   pEndGroupWidgetList);	\
  pBeginGroupWidgetList = NULL;						\
  pEndGroupWidgetList = NULL;						\
} while(0)

#define enable_group(pBeginGroupWidgetList, pEndGroupWidgetList)	\
	set_group_state(pBeginGroupWidgetList, 				\
			pEndGroupWidgetList, FC_WS_NORMAL)

#define disable_group(pBeginGroupWidgetList, pEndGroupWidgetList)	\
	set_group_state(pBeginGroupWidgetList,	pEndGroupWidgetList,	\
			FC_WS_DISABLED)

/* Advanced Dialog */
bool add_widget_to_vertical_scroll_widget_list(struct ADVANCED_DLG *pDlg,
				      struct widget *pNew_Widget,
				      struct widget *pAdd_Dock, bool dir,
					Sint16 start_x, Sint16 start_y);
				      
bool del_widget_from_vertical_scroll_widget_list(struct ADVANCED_DLG *pDlg, 
  						struct widget *pWidget);

/* misc */
SDL_Surface *create_bcgnd_surf(SDL_Surface *pTheme, Uint8 state,
                               Uint16 Width, Uint16 High);
void draw_frame(SDL_Surface *pDest, Sint16 start_x, Sint16 start_y,
		Uint16 w, Uint16 h);

#include "widget_button.h"
#include "widget_checkbox.h"
#include "widget_edit.h"
#include "widget_icon.h"
#include "widget_label.h"
#include "widget_scrollbar.h"
#include "widget_window.h"

#endif	/* FC__WIDGET_H */
