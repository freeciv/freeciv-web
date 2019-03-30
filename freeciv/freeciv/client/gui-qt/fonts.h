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

#ifndef FC__FONTS_H
#define FC__FONTS_H

// Qt
#include <QMap>

// gui-qt
#include "listener.h"

class QFont;
class QStringList;

namespace fonts
{
  const char * const city_label       = "gui_qt_font_city_label";
  const char * const default_font     = "gui_qt_font_default";
  const char * const notify_label     = "gui_qt_font_notify_label";
  const char * const spaceship_label  = "gui_qt_font_spaceship_label";
  const char * const help_label       = "gui_qt_font_help_label";
  const char * const help_link        = "gui_qt_font_help_link";
  const char * const help_text        = "gui_qt_font_help_text";
  const char * const help_title       = "gui_qt_font_help_title";
  const char * const chatline         = "gui_qt_font_chatline";
  const char * const beta_label       = "gui_qt_font_beta_label";
  const char * const comment_label    = "gui_qt_font_comment_label";
  const char * const city_names       = "gui_qt_font_city_names";
  const char * const city_productions = "gui_qt_font_city_productions";
  const char * const reqtree_text     = "gui_qt_font_reqtree_text";
}


class fc_font
{
  Q_DISABLE_COPY(fc_font);
private:
  QMap <QString, QFont *> font_map;
  static fc_font* m_instance;
  explicit fc_font();
public:
  static fc_font* instance();
  static void drop();
  void set_font(QString name, QFont *qf);
  QFont* get_font(QString name);
  void init_fonts();
  void release_fonts();
  void get_mapfont_size();
  int city_fontsize;
  int prod_fontsize;
};

void configure_fonts();
QString configure_font(QString font_name, QStringList sl, int size, bool bold = false);

#endif // FC__FONTS_H
