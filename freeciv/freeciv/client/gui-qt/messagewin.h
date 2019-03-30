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

#ifndef FC__MESSAGEWIN_H
#define FC__MESSAGEWIN_H

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

extern "C" {
#include "messagewin_g.h"
}

//Qt
#include <QWidget>

//qt-client
#include "chatline.h"
#include "mapview.h"

class QEvent;
class QGridLayout;
class QItemSelection;
class QMouseEvent;
class QPixmap;
class QTableWidget;

/***************************************************************************
  Class representing message output
***************************************************************************/
class messagewdg : public QWidget
{
  Q_OBJECT
public:
  messagewdg(QWidget *parent);
  void msg_update();
  void clr();
  void msg(const struct message *pmsg);
private:
  QTableWidget *mesg_table;
  QGridLayout *layout;
  QPixmap *pix;
protected:
  void enterEvent(QEvent *event);
  void leaveEvent(QEvent *event);
  void paint(QPainter *painter, QPaintEvent *event);
  void paintEvent(QPaintEvent *event);
  void resizeEvent(QResizeEvent *event);
public slots:
  void item_selected(const QItemSelection &sl, const QItemSelection &ds);
};

/***************************************************************************
  Class which manages chat and messages
***************************************************************************/
class info_tab : public fcwidget
{
  Q_OBJECT
public:
  info_tab(QWidget *parent);
  void max_chat_size();
  QGridLayout *layout;
  messagewdg *msgwdg;
  chatwdg *chtwdg;
  void maximize_chat();
  void restore_chat();
  bool chat_maximized;
private:
  void update_menu();
  QPoint cursor;
  QSize last_size;
  move_widget *mw;
  bool hidden_state;
  bool resize_mode;
  bool resxy;
  bool resx;
  bool resy;
protected:
  void mousePressEvent(QMouseEvent *event);
  void mouseMoveEvent(QMouseEvent *event);
  void mouseReleaseEvent(QMouseEvent *event);
  int& e_pos();
};

#endif /* FC__MESSAGEWIN_H */
