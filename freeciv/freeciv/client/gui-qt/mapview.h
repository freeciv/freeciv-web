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

#ifndef FC__MAPVIEW_H
#define FC__MAPVIEW_H

// In this case we have to include fc_config.h from header file.
// Some other headers we include demand that fc_config.h must be
// included also. Usually all source files include fc_config.h, but
// there's moc generated meta_mapview.cpp file which does not.
#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

extern "C" {
#include "mapview_g.h"
}

// gui-qt
#include "fonts.h"

// Qt
#include <QFrame>
#include <QLabel>
#include <QMutex>
#include <QQueue>
#include <QThread>
#include <QTimer>

// Forward declarations
class QMutex;
class QPixmap;

class minimap_view;

bool is_point_in_area(int x, int y, int px, int py, int pxe, int pye);
void unscale_point(double scale_factor, int &x, int &y);
void draw_calculated_trade_routes(QPainter *painter);

/**************************************************************************
  Struct used for idle callback to execute some callbacks later
**************************************************************************/
struct call_me_back {
  void (*callback) (void *data);
  void *data;
};

/**************************************************************************
  Class to handle idle callbacks
**************************************************************************/
class mr_idle : public QObject
{
  Q_OBJECT
public:
  mr_idle();
  ~mr_idle();
  void add_callback(call_me_back* cb);
  QQueue<call_me_back*> callback_list;
private slots:
  void idling();
private:
  QTimer timer;
};

/**************************************************************************
  QWidget used for displaying map
**************************************************************************/
class map_view : public QWidget
{
  Q_OBJECT
  void shortcut_pressed(int key);
  void shortcut_released(Qt::MouseButton mb);
public:
  map_view();
  void paint(QPainter *painter, QPaintEvent *event);
  void find_place(int pos_x, int pos_y, int &w, int &h, int wdth, int hght, 
                  int recursive_nr);
  void resume_searching(int pos_x,int pos_y,int &w, int &h,
                        int wdtht, int hght, int recursive_nr);
  void update_cursor(enum cursor_type);
  bool menu_click;

protected:
  void paintEvent(QPaintEvent *event);
  void keyPressEvent(QKeyEvent *event);
  void mousePressEvent(QMouseEvent *event);
  void mouseReleaseEvent(QMouseEvent *event);
  void mouseMoveEvent(QMouseEvent *event);
  void focusOutEvent(QFocusEvent *event);
  void leaveEvent(QEvent *event);
private slots:
  void timer_event();
private:
  void update_font(const QString &name, const QFont &font);

  bool stored_autocenter;
  int cursor_frame;
  int cursor;

};

/**************************************************************************
  Information label about clicked tile
**************************************************************************/
class info_tile: public QLabel
{
  Q_OBJECT
  QFont info_font;
public:
  info_tile(struct tile *ptile, QWidget *parent = 0);
  struct tile *itile;
protected:
  void paintEvent(QPaintEvent *event);
  void paint(QPainter *painter, QPaintEvent *event);
private:
  QStringList str_list;
  void calc_size();
  void update_font(const QString &name, const QFont &font);
};

/**************************************************************************
  Widget allowing resizing other widgets
**************************************************************************/
class resize_widget : public QLabel
{
  Q_OBJECT
public:
  resize_widget(QWidget* parent);
  void put_to_corner();

protected:
  void mouseMoveEvent(QMouseEvent *event);
  void mousePressEvent(QMouseEvent *event);
private:
  QPoint point;
};


/**************************************************************************
  Widget allowing moving other widgets
**************************************************************************/
class move_widget : public QLabel
{
  Q_OBJECT
public:
  move_widget(QWidget* parent);
  void put_to_corner();

protected:
  void mouseMoveEvent(QMouseEvent *event);
  void mousePressEvent(QMouseEvent *event);
private:
  QPoint point;
};

/**************************************************************************
  Abstract class for widgets wanting to do custom action
  when closing widgets is called (eg. update menu)
**************************************************************************/
class fcwidget : public QFrame
{
  Q_OBJECT
public:
  virtual void update_menu() = 0;
  bool was_destroyed;
};

/**************************************************************************
  Widget allowing closing other widgets
**************************************************************************/
class close_widget : public QLabel
{
  Q_OBJECT
public:
  close_widget(QWidget *parent);
  void put_to_corner();
protected:
  void mousePressEvent(QMouseEvent *event);
  void notify_parent();
};

/**************************************************************************
  Thread helper for drawing minimap
**************************************************************************/
class minimap_thread : public QThread
{
  Q_OBJECT
public:
  minimap_thread(QObject *parent = 0);
  ~minimap_thread();
  void render(double scale_factor, int width, int height);

signals:
  void rendered_image(const QImage &image);
protected:
  void run() Q_DECL_OVERRIDE;

private:
  int mini_width, mini_height;
  double scale;
  QMutex mutex;
};

/**************************************************************************
  QLabel used for displaying overview (minimap)
**************************************************************************/
class minimap_view: public fcwidget
{
  Q_OBJECT
public:
  minimap_view(QWidget *parent);
  ~minimap_view();
  void paint(QPainter *painter, QPaintEvent *event);
  virtual void update_menu();
  void update_image();
  void reset();

protected:
  void paintEvent(QPaintEvent *event);
  void resizeEvent(QResizeEvent *event);
  void mousePressEvent(QMouseEvent *event);
  void mouseMoveEvent(QMouseEvent *event);
  void mouseReleaseEvent(QMouseEvent *event);
  void wheelEvent(QWheelEvent *event);
  void moveEvent(QMoveEvent *event);
  void showEvent(QShowEvent *event);

private slots:
  void update_pixmap(const QImage &image);
  void zoom_in();
  void zoom_out();

private:
  void draw_viewport(QPainter *painter);
  void scale(double factor);
  void scale_point(int &x, int &y);
  double scale_factor;
  float w_ratio, h_ratio;
  minimap_thread thread;
  QBrush background;
  QPixmap *pix;
  QPoint cursor;
  QPoint position;
  resize_widget *rw;
};


void mapview_freeze(void);
void mapview_thaw(void);
bool mapview_is_frozen(void);
void pixmap_put_overlay_tile(int canvas_x, int  canvas_y,
                             struct sprite *ssprite);

#endif /* FC__MAPVIEW_H */
