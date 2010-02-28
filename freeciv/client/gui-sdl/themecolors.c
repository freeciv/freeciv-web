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

/* gui-sdl */
#include "colors.h"
#include "themespec.h"

#include "themecolors.h"

/* An RGBAcolor contains the R,G,B,A bitvalues for a color.  The color itself
 * holds the color structure for this color but may be NULL (it's allocated
 * on demand at runtime). */
struct rgbacolor {
  int r, g, b, a;
  struct color *color;
};

struct theme_color_system {
  struct rgbacolor colors[(THEME_COLOR_LAST - COLOR_LAST)];
};

static char *color_names[] = {
  "background",
  "checkbox_label_text",
  "custom_widget_normal_text",
  "custom_widget_selected_frame",  
  "custom_widget_selected_text",
  "custom_widget_pressed_frame",  
  "custom_widget_pressed_text",
  "custom_widget_disabled_text",
  "editfield_caret",
  "label_bar",
  "quick_info_bg",
  "quick_info_frame",  
  "quick_info_text",
  "selectionrectangle",
  "text",
  "themelabel2_bg",  
  "widget_normal_text",
  "widget_selected_text",
  "widget_pressed_text",
  "widget_disabled_text",
  "window_titlebar_separator",
  
  "advancedterraindlg_text",
  

  "citydlg_buy",
  "citydlg_celeb",  
  "citydlg_corruption",  
  "citydlg_foodperturn",
  "citydlg_foodstock",
  "citydlg_food_surplus",  
  "citydlg_frame",  
  "citydlg_gold",    
  "citydlg_granary",  
  "citydlg_growth",
  "citydlg_happy",
  "citydlg_impr",
  "citydlg_infopanel",  
  "citydlg_lux",    
  "citydlg_panel",
  "citydlg_prod",
  "citydlg_science",
  "citydlg_sell",
  "citydlg_shieldstock",
  "citydlg_stocks",  
  "citydlg_support",
  "citydlg_trade",
  "citydlg_upkeep",
  "cityrep_foodstock",  
  "cityrep_frame",
  "cityrep_prod",
  "cityrep_text",  
  "cityrep_trade",
  "cma_frame",
  "cma_text",
  "connectdlg_frame",  
  "connectdlg_innerframe",
  "connectdlg_labelframe",
  "connlistdlg_frame",
  "diplodlg_meeting_heading_text",  
  "diplodlg_meeting_text",
  "diplodlg_text",
  "economydlg_frame",
  "economydlg_neg_text",  
  "economydlg_text",
  "helpdlg_frame",
  "helpdlg_line",
  "helpdlg_line2",
  "helpdlg_line3",
  "helpdlg_text",
  "joingamedlg_frame",  
  "joingamedlg_text",
  "mapview_info_frame",
  "mapview_info_text",
  "mapview_unitinfo_text",
  "mapview_unitinfo_veteran_text",
  "meswin_active_text",
  "meswin_active_text2",
  "meswin_frame",
  "nationdlg_frame",
  "nationdlg_legend",
  "nationdlg_text",
  "newcitydlg_text",
  "optiondlg_worklistlist_frame",
  "optiondlg_worklistlist_text",
  "plrdlg_alliance",
  "plrdlg_armistice",
  "plrdlg_ceasefire",
  "plrdlg_frame",  
  "plrdlg_peace",
  "plrdlg_text",
  "plrdlg_war",
  "plrdlg_war_restricted",
  "revolutiondlg_text",
  "sabotagedlg_separator",
  "sciencedlg_frame",
  "sciencedlg_med_techicon_bg",
  "sciencedlg_text",
  "sellimpr_text",
  "unitsrep_frame",
  "unitsrep_text",
  "unitupgrade_text",
  "userpasswddlg_frame",  
  "userpasswddlg_text",
  "wardlg_text",
  "wldlg_frame",
};

struct theme_color_system *theme_color_system_read(struct section_file *file)
{
  int i;
  struct theme_color_system *colors = fc_malloc(sizeof(*colors));

  assert(ARRAY_SIZE(color_names) == (THEME_COLOR_LAST - COLOR_LAST));
  for (i = 0; i < (THEME_COLOR_LAST - COLOR_LAST); i++) {
    colors->colors[i].r
      = secfile_lookup_int(file, "colors.%s0.r", color_names[i]);
    colors->colors[i].g
      = secfile_lookup_int(file, "colors.%s0.g", color_names[i]);
    colors->colors[i].b
      = secfile_lookup_int(file, "colors.%s0.b", color_names[i]);
    colors->colors[i].a
      = secfile_lookup_int(file, "colors.%s0.a", color_names[i]);
    colors->colors[i].color = NULL;
  }
  
  return colors;
}

/****************************************************************************
  Called when the client first starts to free any allocated colors.
****************************************************************************/
void theme_color_system_free(struct theme_color_system *colors)
{
  int i;

  for (i = 0; i < (THEME_COLOR_LAST - COLOR_LAST); i++) {
    if (colors->colors[i].color) {
      color_free(colors->colors[i].color);
    }
  }
  
  free(colors);
}

/****************************************************************************
  Return the RGBA color, allocating it if necessary.
****************************************************************************/
static struct color *ensure_color(struct rgbacolor *rgba)
{
  if (!rgba->color) {
    rgba->color = color_alloc_rgba(rgba->r, rgba->g, rgba->b, rgba->a);
  }
  return rgba->color;
}

/****************************************************************************
  Return a pointer to the given "theme" color.
****************************************************************************/
struct color *theme_get_color(const struct theme *t, enum theme_color color)
{
  return ensure_color(&theme_get_color_system(t)->colors[color]);
}
