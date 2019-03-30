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
#ifndef FC__VOTEBAR_H
#define FC__VOTEBAR_H

extern "C" {
#include "voteinfo_bar_g.h"
}

// Qt
#include <QWidget>

class QGridLayout;
class QLabel;
class QPushButton;

/***************************************************************************
  pregamevote class used for displaying vote bar in PAGE START
***************************************************************************/
class pregamevote : public QWidget
{
  Q_OBJECT
public:
  explicit pregamevote(QWidget *parent = NULL);
  ~pregamevote();
  void update_vote();
  QLabel *label_text;
  QLabel *label_vote_text;
  QPushButton *vote_yes;
  QPushButton *vote_no;
  QPushButton *vote_abstain;
  QLabel *lab_yes;
  QLabel *lab_no;
  QLabel *lab_abstain;
  QLabel *voters;
  QGridLayout *layout;
public slots:
  void v_yes();
  void v_no();
  void v_abstain();
};

/***************************************************************************
  xvote class used for displaying vote bar in PAGE GAME
***************************************************************************/
class xvote : public pregamevote
{
  Q_OBJECT
public:
  xvote(QWidget *parent);
protected:
  void paint(QPainter *painter, QPaintEvent *event);
  void paintEvent(QPaintEvent *event);
};

#endif  /* FC__VOTEBAR_H */
