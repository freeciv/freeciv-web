/********************************************************************** 
 Freeciv - Copyright (C) 2003 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#ifndef FC__RATESDLG_H
#define FC__RATESDLG_H

// In this case we have to include fc_config.h from header file.
// Some other headers we include demand that fc_config.h must be
// included also. Usually all source files include fc_config.h, but
// there's moc generated meta_ratesdlg.cpp file which does not.
#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

// Qt
#include <QDialog>
# include <QList>

// common
#include "government.h"

// client
#include "client_main.h"

// gui-qt
#include "dialogs.h"

extern "C" {
#include "ratesdlg_g.h"
}

class QMouseEvent;
class QPaintEvent;
class QPixmap;
class QPushButton;
class QSize;
class QSlider;

/**************************************************************************
 * Custom slider with two settable values
 *************************************************************************/
class fc_double_edge : public QWidget
{
  Q_OBJECT

private:
  double cursor_size;
  double mouse_x;
  int moved;
  bool on_min;
  bool on_max;
  int max_rates;
  QPixmap cursor_pix;
public:
  fc_double_edge(QWidget *parent = NULL);
  ~fc_double_edge();
  int current_min;
  int current_max;
  QSize sizeHint() const;
protected:
  void paintEvent(QPaintEvent *event);
  void mousePressEvent(QMouseEvent *event);
  void mouseMoveEvent(QMouseEvent *event);

};

/**************************************************************************
 * Dialog used to change tax rates
 *************************************************************************/
class tax_rates_dialog: public qfc_dialog
{
Q_OBJECT

  public:
  tax_rates_dialog(QWidget *parent = 0);

private:
  fc_double_edge *fcde;
private slots:
  void slot_ok_button_pressed();
  void slot_cancel_button_pressed();
  void slot_apply_button_pressed();
};

/**************************************************************************
 * Dialog used to change policies
 *************************************************************************/
class multipler_rates_dialog: public QDialog
{
  Q_OBJECT

public:
  explicit multipler_rates_dialog(QWidget* parent = 0, Qt::WindowFlags f = 0);
private:
  QList<QSlider*> slider_list;
  QPushButton *cancel_button;
  QPushButton *ok_button;
private slots:
  void slot_set_value(int i);
  void slot_ok_button_pressed();
  void slot_cancel_button_pressed();
};


void popup_multiplier_dialog(void);

#endif /* FC__RATESDLG_H */
