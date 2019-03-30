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
#include <QGridLayout>
#include <QPainter>

// client
#include "voteinfo.h"

// gui-qt
#include "fc_client.h"

#include "voteinfo_bar.h"

/***********************************************************************//**
  Constructor for pregamevote
***************************************************************************/
pregamevote::pregamevote(QWidget *parent)
{
  setParent(parent);
  layout = new QGridLayout;
  label_text = new QLabel;
  label_vote_text = new QLabel;
  vote_yes = new QPushButton(_("Yes"));
  vote_no = new QPushButton(_("No"));
  vote_abstain = new QPushButton(_("Abstain"));
  lab_yes = new QLabel;
  lab_no = new QLabel;
  lab_abstain = new QLabel;
  voters = new QLabel;
  label_text->setAlignment(Qt::AlignHCenter);
  label_vote_text->setAlignment(Qt::AlignHCenter);
  label_text->setTextFormat(Qt::RichText);
  label_vote_text->setTextFormat(Qt::RichText);
  layout->addWidget(label_text, 0, 0, 1, 7);
  layout->addWidget(label_vote_text, 1, 0, 1, 7);
  layout->addWidget(vote_yes, 2, 0, 1, 1);
  layout->addWidget(vote_no, 2, 2, 1, 1);
  layout->addWidget(vote_abstain, 2, 4, 1, 1);
  layout->addWidget(lab_yes, 2, 1, 1, 1);
  layout->addWidget(lab_no, 2, 3, 1, 1);
  layout->addWidget(lab_abstain, 2, 5, 1, 1);
  layout->addWidget(voters, 2, 6, 1, 1);
  setLayout(layout);
  connect(vote_yes, &QAbstractButton::clicked, this, &pregamevote::v_yes);
  connect(vote_no, &QAbstractButton::clicked, this, &pregamevote::v_no);
  connect(vote_abstain, &QAbstractButton::clicked, this, &pregamevote::v_abstain);

}

/***********************************************************************//**
  Slot vote abstain
***************************************************************************/
void pregamevote::v_abstain()
{
  struct voteinfo *vi;

  vi = voteinfo_queue_get_current(NULL);
  if (vi == NULL) {
    return;
  }
  voteinfo_do_vote(vi->vote_no, CVT_ABSTAIN);
}

/***********************************************************************//**
  Slot vote no
***************************************************************************/
void pregamevote::v_no()
{
  struct voteinfo *vi;

  vi = voteinfo_queue_get_current(NULL);
  if (vi == NULL) {
    return;
  }
  voteinfo_do_vote(vi->vote_no, CVT_NO);
}

/***********************************************************************//**
  Slot vote yes
***************************************************************************/
void pregamevote::v_yes()
{
  struct voteinfo *vi;

  vi = voteinfo_queue_get_current(NULL);
  if (vi == NULL) {
    return;
  }
  voteinfo_do_vote(vi->vote_no, CVT_YES);
}

/***********************************************************************//**
  Updates text on vote
***************************************************************************/
void pregamevote::update_vote()
{
  int vote_count, index;
  struct voteinfo *vi = NULL;
  char buf[1024], status[1024], color[32];
  bool running;

  show();
  vote_count = voteinfo_queue_size();
  vi = voteinfo_queue_get_current(&index);
  if (vi != NULL && vi->resolved && vi->passed) {
    /* TRANS: Describing a vote that passed. */
    fc_snprintf(status, sizeof(status), _("[passed]"));
    sz_strlcpy(color, "green");
  } else if (vi != NULL && vi->resolved && !vi->passed) {
    /* TRANS: Describing a vote that failed. */
    fc_snprintf(status, sizeof(status), _("[failed]"));
    sz_strlcpy(color, "red");
  } else if (vi != NULL && vi->remove_time > 0) {
    /* TRANS: Describing a vote that was removed. */
    fc_snprintf(status, sizeof(status), _("[removed]"));
    sz_strlcpy(color, "grey");
  } else {
    status[0] = '\0';
  }
  if (status[0] != '\0') {
    fc_snprintf(buf, sizeof(buf),
                "<b><p style=\"background-color: %s\"> %s</p></b> ",
                color, status);
    sz_strlcpy(status, buf);
  } else {
    buf[0] = '\0';
  }
  if (vi != NULL)  {
    lab_yes->setText(QString::number(vi->yes));
    lab_no->setText(QString::number(vi->no));
    lab_abstain->setText(QString::number(vi->abstain));
    if (buf[0] != '\0') {
      label_text->setText(buf);
    } else {
      label_text->setText(QString(_("<b>%1 called a vote for:</b>")).
                          arg(vi->user));
    }
    label_vote_text->setText(QString("</b><p style=\"color:"
                                     " red\"> %1</p></b>").arg(vi->desc));
    voters->setText(QString(" /%1").arg(vi->num_voters));
  } else {
    label_text->setText("");
    lab_yes->setText("-");
    lab_no->setText("-");
    lab_abstain->setText("-");
  }
  running = vi != NULL && !vi->resolved && vi->remove_time == 0;
  vote_yes->setEnabled(running);
  vote_no->setEnabled(running);
  vote_abstain->setEnabled(running);

  if (vote_count < 1) {
    hide();
  }
  update();
}

/***********************************************************************//**
  Destructor for pregamevote
***************************************************************************/
pregamevote::~pregamevote()
{
}

/***********************************************************************//**
  pregamevote class used for displaying vote bar in PAGE START
***************************************************************************/
xvote::xvote(QWidget *parent) : pregamevote(parent)
{
  QPalette palette;

  setParent(parent);
  palette.setColor(QPalette::WindowText, QColor(0, 255, 255));
  label_text->setPalette(palette);
  label_vote_text->setPalette(palette);
  palette.setColor(QPalette::WindowText, QColor(255, 255, 0));
  lab_yes->setPalette(palette);
  lab_no->setPalette(palette);
  lab_abstain->setPalette(palette);
  voters->setPalette(palette);
}

/***********************************************************************//**
  Paints frames for xvote
***************************************************************************/
void xvote::paint(QPainter *painter, QPaintEvent *event)
{
  painter->setBrush(QColor(90, 90, 192, 185));
  painter->drawRect(0, 0, width(), height());
  painter->setBrush(QColor(90, 90, 192, 185));
  painter->drawRect(5, 5, width() - 10, height() - 10);
}

/***********************************************************************//**
  Paint event for xvote
***************************************************************************/
void xvote::paintEvent(QPaintEvent *event)
{
  QPainter painter;

  painter.begin(this);
  paint(&painter, event);
  painter.end();
}

/***********************************************************************//**
  Refresh all vote related GUI widgets. Called by the voteinfo module when
  the client receives new vote information from the server.
***************************************************************************/
void voteinfo_gui_update(void)
{
  if (gui()->current_page() == PAGE_START) {
    gui()->pre_vote->update_vote();
  }
  if (gui()->current_page() == PAGE_GAME) {

    if (gui()->x_vote != NULL) {
      gui()->x_vote->show();
      gui()->x_vote->update_vote();
    }
  }
}
