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

// Qt
#include <QApplication>
#include <QDesktopWidget>
#include <QGroupBox>
#include <QMouseEvent>
#include <QPainter>
#include <QVBoxLayout>

// common
#include "multipliers.h"

// gui-qt
#include "fc_client.h"
#include "dialogs.h"
#include "qtg_cxxside.h"
#include "sprite.h"

#include "ratesdlg.h"

static int scale_to_mult(const struct multiplier *pmul, int scale);
static int mult_to_scale(const struct multiplier *pmul, int val);

/**********************************************************************//**
  Dialog constructor for changing rates with sliders.
  Automatic destructor will clean qobjects, so there is no one
**************************************************************************/
tax_rates_dialog::tax_rates_dialog(QWidget *parent)
  : qfc_dialog(parent)
{
  QHBoxLayout *some_layout;
  QVBoxLayout *main_layout;
  QPushButton *cancel_button;
  QPushButton *ok_button;
  QPushButton *apply_button;
  QLabel *l1, *l2;
  QString str;
  int max;

  setWindowTitle(_("Tax rates"));
  main_layout = new QVBoxLayout;

  if (client.conn.playing != nullptr) {
    max = get_player_bonus(client.conn.playing, EFT_MAX_RATES);
  } else {
    max = 100;
  }

  /* Trans: Government - max rate (of taxes) x% */
  str = QString(_("%1 - max rate: %2%")).
        arg(government_name_for_player(client.conn.playing),
            QString::number(max));

  l2 = new QLabel(_("Select tax, luxury and science rates"));
  l1 = new QLabel(str);
  l1->setAlignment(Qt::AlignHCenter);
  l2->setAlignment(Qt::AlignHCenter);
  main_layout->addWidget(l2);
  main_layout->addWidget(l1);
  main_layout->addSpacing(20);

  cancel_button = new QPushButton(_("Cancel"));
  ok_button = new QPushButton(_("Ok"));
  apply_button = new QPushButton(_("Apply"));
  some_layout = new QHBoxLayout;
  connect(cancel_button, &QAbstractButton::pressed,
          this, &tax_rates_dialog::slot_cancel_button_pressed);
  connect(apply_button, &QAbstractButton::pressed,
          this, &tax_rates_dialog::slot_apply_button_pressed);
  connect(ok_button, &QAbstractButton::pressed,
          this, &tax_rates_dialog::slot_ok_button_pressed);
  some_layout->addWidget(cancel_button);
  some_layout->addWidget(apply_button);
  some_layout->addWidget(ok_button);
  fcde = new fc_double_edge(this);
  main_layout->addWidget(fcde);
  main_layout->addSpacing(20);
  main_layout->addLayout(some_layout);
  setLayout(main_layout);
}

/**********************************************************************//**
  When cancel in qtpushbutton pressed selfdestruction :D.
**************************************************************************/
void tax_rates_dialog::slot_cancel_button_pressed()
{
  delete this;
}

/**********************************************************************//**
  When ok in qpushbutton pressed send info to server and selfdestroy :D.
**************************************************************************/
void tax_rates_dialog::slot_ok_button_pressed()
{
  dsend_packet_player_rates(&client.conn, 10 * fcde->current_min,
                            10 * (10 - fcde->current_max),
                            10 * (fcde->current_max - fcde->current_min));
  delete this;
}

/**********************************************************************//**
  Pressed "apply" in tax rates dialog.
**************************************************************************/
void tax_rates_dialog::slot_apply_button_pressed()
{
  dsend_packet_player_rates(&client.conn, 10 * fcde->current_min,
                            10 * (10 - fcde->current_max),
                            10 * (fcde->current_max - fcde->current_min));
}

/**********************************************************************//**
  Multipler rates dialog constructor
  Inheriting from qfc_dialog will cause crash in Qt5.2
**************************************************************************/
multipler_rates_dialog::multipler_rates_dialog(QWidget *parent,
                                               Qt::WindowFlags f)
  : QDialog(parent)
{
  QGroupBox *group_box;
  QHBoxLayout *some_layout;
  QLabel *label;
  QSlider *slider;
  QVBoxLayout *main_layout;
  struct player *pplayer = client_player();

  cancel_button = new QPushButton;
  ok_button = new QPushButton;
  setWindowTitle(_("Change policies"));
  main_layout = new QVBoxLayout;

  multipliers_iterate(pmul) {
    QHBoxLayout *hb = new QHBoxLayout;
    int val = player_multiplier_target_value(pplayer, pmul);

    group_box = new QGroupBox(multiplier_name_translation(pmul));
    slider = new QSlider(Qt::Horizontal, this);
    slider->setMinimum(mult_to_scale(pmul, pmul->start));
    slider->setMaximum(mult_to_scale(pmul, pmul->stop));
    slider->setValue(val);
    connect(slider, &QAbstractSlider::valueChanged,
            this, &multipler_rates_dialog::slot_set_value);
    slider_list.append(slider);
    label = new QLabel(QString::number(val));
    hb->addWidget(slider);
    slider->setEnabled(multiplier_can_be_changed(pmul, client_player()));
    hb->addWidget(label);
    group_box->setLayout(hb);
    slider->setProperty("lab", QVariant::fromValue((void *) label));
    main_layout->addWidget(group_box);

  } multipliers_iterate_end;
  some_layout = new QHBoxLayout;
  cancel_button->setText(_("Cancel"));
  ok_button->setText(_("Ok"));
  connect(cancel_button, &QAbstractButton::pressed,
          this, &multipler_rates_dialog::slot_cancel_button_pressed);
  connect(ok_button, &QAbstractButton::pressed,
          this, &multipler_rates_dialog::slot_ok_button_pressed);
  some_layout->addWidget(cancel_button);
  some_layout->addWidget(ok_button);
  main_layout->addSpacing(20);
  main_layout->addLayout(some_layout);
  setLayout(main_layout);
}

/**********************************************************************//**
  Slider value changed
**************************************************************************/
void multipler_rates_dialog::slot_set_value(int i)
{
  QSlider *qo;
  qo = (QSlider *) QObject::sender();
  QVariant qvar;
  QLabel *lab;

  qvar = qo->property("lab");
  lab =  reinterpret_cast<QLabel *>(qvar.value<void *>());
  lab->setText(QString::number(qo->value()));
}

/**********************************************************************//**
  Cancel pressed
**************************************************************************/
void multipler_rates_dialog::slot_cancel_button_pressed()
{
  close();
  deleteLater();
}

/**********************************************************************//**
  Ok pressed - send mulipliers value.
**************************************************************************/
void multipler_rates_dialog::slot_ok_button_pressed()
{
  int j = 0;
  int value;
  struct packet_player_multiplier mul;

  multipliers_iterate(pmul) {
    Multiplier_type_id i = multiplier_index(pmul);

    value = slider_list.at(j)->value();
    mul.multipliers[i] = scale_to_mult(pmul, value);
    j++;
  } multipliers_iterate_end;
  mul.count = multiplier_count();
  send_packet_player_multiplier(&client.conn, &mul);
  close();
  deleteLater();
}

/**********************************************************************//**
  Convert real multiplier display value to scale value
**************************************************************************/
int mult_to_scale(const struct multiplier *pmul, int val)
{
  return (val - pmul->start) / pmul->step;
}

/**********************************************************************//**
  Convert scale units to real multiplier display value
**************************************************************************/
int scale_to_mult(const struct multiplier *pmul, int scale)
{
  return scale * pmul->step + pmul->start;
}

/**********************************************************************//**
  Popup (or raise) the (tax/science/luxury) rates selection dialog.
**************************************************************************/
void popup_rates_dialog(void)
{
  QPoint p;
  QRect rect;

  p = QCursor::pos();
  rect = QApplication::desktop()->availableGeometry();
  tax_rates_dialog *trd = new tax_rates_dialog(gui()->central_wdg);
  p.setY(p.y() - trd->height() / 2);
  if (p.y() < 50) {
    p.setY(50);
  }
  if (p.y() + trd->height() > rect.bottom()) {
    p.setY(rect.bottom() - trd->height());
  }
  if (p.x() + trd->width() > rect.right()) {
    p.setX(rect.right() - trd->width());
  }
  trd->move(p);
  trd->show();
}

/**********************************************************************//**
  Update multipliers (policies) dialog.
**************************************************************************/
void real_multipliers_dialog_update(void *unused)
{
  /* PORTME */
}

/**********************************************************************//**
  Popups multiplier dialog
**************************************************************************/
void popup_multiplier_dialog(void)
{
  multipler_rates_dialog *mrd;

  if (!can_client_issue_orders()) {
    return;
  }
  mrd = new multipler_rates_dialog(gui()->central_wdg);
  mrd->show();
}

/**********************************************************************//**
  Double edged slider constructor
**************************************************************************/
fc_double_edge::fc_double_edge(QWidget *parent)
  : QWidget(parent)
{

  current_min = client.conn.playing->economic.tax / 10;
  current_max = 10 - (client.conn.playing->economic.luxury / 10);
  mouse_x = 0.;
  moved = 0;
  on_min = false;
  on_max = false;

  if (client.conn.playing !=  nullptr) {
    max_rates = get_player_bonus(client.conn.playing, EFT_MAX_RATES) / 10;
  } else {
    max_rates = 10;
  }
  cursor_pix = *fc_icons::instance()->get_pixmap("control");
  setMouseTracking(true);
}

/**********************************************************************//**
  Double edged slider destructor
**************************************************************************/
fc_double_edge::~fc_double_edge()
{
}

/**********************************************************************//**
  Default size for double edge slider
**************************************************************************/
QSize fc_double_edge::sizeHint() const
{
  return QSize(30 * get_tax_sprite(tileset, O_LUXURY)->pm->width(),
               3 * get_tax_sprite(tileset, O_LUXURY)->pm->height());
}

/**********************************************************************//**
  Double edge paint event
**************************************************************************/
void fc_double_edge::paintEvent(QPaintEvent *event)
{
  QPainter p;
  int i, j, pos;
  QPixmap *pix;
  QPixmap pix_scaled;
  QSize s;
  double x_min,  x_max;

  cursor_pix = cursor_pix.scaled(width() / 20,  height());
  cursor_size = cursor_pix.width();
  p.begin(this);
  p.setRenderHint(QPainter::TextAntialiasing);
  p.setBrush(Qt::SolidPattern);
  p.setPen(Qt::SolidLine);

  x_min = static_cast<double>(current_min) / 10 *
          ((width() - 1)  - 2 * cursor_size) + cursor_size;
  x_max = static_cast<double>(current_max) / 10 *
          ((width() - 1) - 2 * cursor_size) + cursor_size;

  pos = cursor_size;
  pix = get_tax_sprite(tileset, O_GOLD)->pm;
  s.setWidth((width() - 2 * cursor_size) / 10);
  s.setHeight(height());
  pix_scaled = pix->scaled(s, Qt::IgnoreAspectRatio,
                           Qt::SmoothTransformation);
  for (i = 0; i < current_min; i++) {
    p.drawPixmap(pos, 0, pix_scaled);
    pos = pos + pix_scaled.width();
  }
  j = i;
  pix = get_tax_sprite(tileset, O_SCIENCE)->pm;
  pix_scaled = pix->scaled(s, Qt::IgnoreAspectRatio,
                           Qt::SmoothTransformation);
  for (i = j; i < current_max; i++) {
    p.drawPixmap(pos, 0, pix_scaled);
    pos = pos + pix_scaled.width();
  }
  j = i;
  pix = get_tax_sprite(tileset, O_LUXURY)->pm;
  pix_scaled = pix->scaled(s, Qt::IgnoreAspectRatio,
                           Qt::SmoothTransformation);
  for (i = j; i < 10; i++) {
    p.drawPixmap(pos, 0, pix_scaled);
    pos = pos + pix_scaled.width();
  }
  p.drawPixmap(x_max - cursor_size / 2, 0,  cursor_pix);
  p.drawPixmap(x_min - cursor_size / 2, 0,  cursor_pix);
  p.end();
}

/**********************************************************************//**
  Double edged slider mouse press event
**************************************************************************/
void fc_double_edge::mousePressEvent(QMouseEvent *event)
{
  if (event->buttons() & Qt::LeftButton) {
    mouse_x = static_cast<double>(event->x());

    if (mouse_x <= current_max * width() / 10 - 2 * cursor_size) {
      moved = 1;
    } else {
      moved = 2;
    }
  } else {
    moved = 0;
  }
  mouseMoveEvent(event);
  update();
}

/**********************************************************************//**
  Double edged slider mouse move event
**************************************************************************/
void fc_double_edge::mouseMoveEvent(QMouseEvent *event)
{
  float x_min, x_max, x_mouse;

  if (on_max || on_min) {
    setCursor(Qt::SizeHorCursor);
  } else {
    setCursor(Qt::ArrowCursor);
  }

  x_mouse = static_cast<float>(event->x());
  x_min = static_cast<float>(current_min) / 10 *
          ((width() - 1)  - 2 * cursor_size) + cursor_size;
  x_max = static_cast<float>(current_max) / 10 *
          ((width() - 1) - 2 * cursor_size) + cursor_size;

  on_min = (((x_mouse > (x_min - cursor_size * 1.1)) &&
             (x_mouse < (x_min + cursor_size * 1.1)))
            && (!on_max))
           || (moved == 1);
  on_max = (((x_mouse > (x_max - cursor_size * 1.1)) &&
             (x_mouse < (x_max + cursor_size * 1.1)))
            && !on_min)
           || (moved == 2);
  if (event->buttons() & Qt::LeftButton) {
    if ((moved != 2) && on_min) {
      x_min = x_mouse * width() /
              ((width() - 1)  - 2 * cursor_size) - cursor_size;
      if (x_min < 0) x_min = 0;
      if (x_min > width()) x_min = width();
      current_min = (x_min * 10 / (width() - 1));
      if (current_min > max_rates) {
        current_min = max_rates;
      }
      if (current_max < current_min) {
        current_max = current_min;
      }
      if (current_max - current_min > max_rates) {
        current_min = current_max - max_rates;
      }
      moved = 1;
    } else if ((moved != 1) && on_max) {
      x_max = x_mouse * width() /
              ((width() - 1)  - 2 * cursor_size) - cursor_size;
      if (x_max < 0) {
        x_max = 0;
      }
      if (x_max > width()) {
        x_max = width();
      }
      current_max = (x_max * 10 / (width() - 1));
      if (current_max > max_rates + current_min) {
        current_max = max_rates + current_min;
      }
      if (current_max < 10 - max_rates) {
        current_max = 10 - max_rates;
      }
      if (current_min > current_max) {
        current_min = current_max;
      }
      moved = 2;
    }
    update();
  } else {
    moved = 0;
  }

  mouse_x = x_mouse;
}
