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

#ifndef FC__OPTIONDLG_H
#define FC__OPTIONDLG_H

extern "C" {
#include "optiondlg_g.h"
}

// Qt
#include <QDialog>
#include <QMap>

// qt-client
#include <dialogs.h>

class Qdialog;
class QVBoxLayout;
class QTabWidget;
class QDialogButtonBox;
class QSignalMapper;
class QWidget;
class QString;

QString split_text(QString text, bool cut);
QString cut_helptext(QString text);
/****************************************************************************
  Dialog for client/server options
****************************************************************************/
class option_dialog : public qfc_dialog
{
  Q_OBJECT
  QVBoxLayout * main_layout;
  QTabWidget *tab_widget;
  QDialogButtonBox *button_box;
  QList <QString> categories;
  QMap <QString, QWidget *> widget_map;
  QSignalMapper *signal_map;

public:
   option_dialog(const QString &name, const option_set *options,
                 QWidget *parent = 0);
  ~option_dialog();
  void fill(const struct option_set *poptset);
  void add_option(struct option *poption);
  void option_dialog_refresh(struct option *poption);
  void option_dialog_reset(struct option *poption);
  void full_refresh();
  void apply_options();
private:
  const option_set *curr_options;
  void set_bool(struct option *poption, bool value);
  void set_int(struct option *poption, int value);
  void set_string(struct option *poption, const char *string);
  void set_enum(struct option *poption, int index);
  void set_bitwise(struct option *poption, unsigned value);
  void set_color(struct option *poption, struct ft_color color);
  void set_font(struct option *poption, QString s);
  void get_color(struct option *poption, QByteArray &a1, QByteArray &a2);
  bool get_bool(struct option *poption);
  int get_int(struct option *poption);
  QFont get_font(struct option *poption);
  QByteArray get_button_font(struct option *poption);
  QByteArray get_string(struct option *poption);
  int get_enum(struct option *poption);
  struct option* get_color_option();
  unsigned get_bitwise(struct option *poption);
  void full_reset();
private slots:
  void apply_option(int response);
  void set_color();
  void set_font();
};

#endif /* FC__OPTIONDLG_H */
