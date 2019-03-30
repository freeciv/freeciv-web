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

//Qt
#include <QApplication>
#include <QCheckBox>
#include <QCompleter>
#include <QGridLayout>
#include <QKeyEvent>
#include <QPainter>
#include <QScrollBar>
#include <QStyleFactory>
#include <QTextBlock>
#include <QTextLayout>
#include <QTextBrowser>

// common
#include "chat.h"

// client
#include "audio.h"
#include "climap.h"
#include "climisc.h"      /* for write_chatline_content */
#include "connectdlg_common.h"
#include "control.h"
#include "game.h"

// gui-qt
#include "chatline.h"
#include "colors.h"
#include "fc_client.h"
#include "gui_main.h"
#include "qtg_cxxside.h"

extern QApplication *qapp;
static bool is_plain_public_message(QString s);

FC_CPP_DECLARE_LISTENER(chat_listener)
QStringList chat_listener::history = QStringList();
QStringList chat_listener::word_list = QStringList();

/***********************************************************************//**
  Updates the chat completion word list.
***************************************************************************/
void chat_listener::update_word_list()
{
  QString str;

  conn_list_iterate(game.est_connections, pconn) {
    if (pconn->playing) {
      word_list << pconn->playing->name;
      word_list << pconn->playing->username;
    } else {
      word_list << pconn->username;
    }
  } conn_list_iterate_end;

  players_iterate (pplayer){
    str = pplayer->name;
    if (!word_list.contains(str)){
      word_list << str;
    }
  } players_iterate_end

  invoke(&chat_listener::chat_word_list_changed, word_list);
}

/***********************************************************************//**
  Constructor.
***************************************************************************/
chat_listener::chat_listener() :
    position(HISTORY_END)
{}

/***********************************************************************//**
  Called whenever a message is received. Default implementation does
  nothing.
***************************************************************************/
void chat_listener::chat_message_received(const QString &,
                                          const struct text_tag_list *)
{}

/***********************************************************************//**
  Called whenever the completion word list changes. Default implementation
  does nothing.
***************************************************************************/
void chat_listener::chat_word_list_changed(const QStringList &)
{}

/***********************************************************************//**
  Sends commands to server, but first searches for custom keys, if it finds
  then it makes custom action.

  The history position is reset to HISTORY_END.
***************************************************************************/
void chat_listener::send_chat_message(const QString &message)
{
  int index;
  QString splayer, s;

  /* FIXME
   * Key == PICK: used for picking nation, it was put here cause those
   * Qt slots are a bit limited ...I'm unable to pass custom player pointer
   * or idk how to do that
   */
  s = message;
  index = message.indexOf("PICK:");

  if (index != -1) {
    s = s.remove("PICK:");
    /* now should be playername left in string */
    players_iterate(pplayer) {
      splayer = QString(pplayer->name);

      if (!splayer.compare(s)) {
        popup_races_dialog(pplayer);
      }
    } players_iterate_end;
    return;
  }

  history << message;
  reset_history_position();

  /*
   * If client send commands to take ai, set /away to disable AI
   */
  if (message.startsWith("/take ")) {
    s = s.remove("/take ");
    players_iterate(pplayer) {
      splayer = QString(pplayer->name);
      splayer = "\"" + splayer + "\"";

      if (!splayer.compare(s)) {
        if (is_ai(pplayer)) {
          send_chat(message.toLocal8Bit());
          send_chat("/away");
          return;
        }
      }
    } players_iterate_end;
  }

  /*
   * Option to send to allies by default
   */
  if (!message.isEmpty()) {
    if (client_state() >= C_S_RUNNING && gui_options.gui_qt_allied_chat_only
        && is_plain_public_message(message)) {
      send_chat((QString(CHAT_ALLIES_PREFIX)
                 + " " + message).toLocal8Bit());
    } else {
      send_chat(message.toLocal8Bit());
    }
  }
  // Empty messages aren't sent
  // FIXME Inconsistent behavior: "." will send an empty message to allies
}

/***********************************************************************//**
  Goes back one position in history, and returns the message at the new
  position.
***************************************************************************/
QString chat_listener::back_in_history()
{
  if (!history.empty() && position == HISTORY_END) {
    position = history.size() - 1;
  } else if (position > 0) {
    position--;
  }
  return history.empty() ? "" : history.at(position);
}

/***********************************************************************//**
  Goes forward one position in history, and returns the message at the new
  position. An empty string is returned if the new position is HISTORY_END.
***************************************************************************/
QString chat_listener::forward_in_history()
{
  if (position == HISTORY_END) {
    return "";
  }
  position++;
  if (position >= history.size()) {
    position = HISTORY_END;
    return "";
  } else {
    return history.at(position);
  }
}

/***********************************************************************//**
  Go to the end of the history.
***************************************************************************/
void chat_listener::reset_history_position()
{
  position = HISTORY_END;
}

/***********************************************************************//**
  Constructor
***************************************************************************/
chat_input::chat_input(QWidget *parent) :
    QLineEdit(parent)
{
  connect(this, &QLineEdit::returnPressed, this, &chat_input::send);
  chat_word_list_changed(current_word_list());
  chat_listener::listen();
}

/***********************************************************************//**
  Sends the content of the input box
***************************************************************************/
void chat_input::send()
{
  send_chat_message(text());
  clear();
}

/***********************************************************************//**
  Called whenever the completion word list changes.
***************************************************************************/
void chat_input::chat_word_list_changed(const QStringList &word_list)
{
  QCompleter *cmplt = completer();

  if (cmplt != nullptr) {
    delete cmplt;
  }
  cmplt = new QCompleter(word_list);
  cmplt->setCaseSensitivity(Qt::CaseInsensitive);
  cmplt->setCompletionMode(QCompleter::InlineCompletion);
  setCompleter(cmplt);
}

/***********************************************************************//**
  Event handler for chat_input, used for history
***************************************************************************/
bool chat_input::event(QEvent *event)
{
  if (event->type() == QEvent::KeyPress) {
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
    switch (keyEvent->key()) {
    case Qt::Key_Up:
      setText(back_in_history());
      return true;
    case Qt::Key_Down:
      setText(forward_in_history());
      return true;
    }
  }
  return QLineEdit::event(event);
}

/***********************************************************************//**
  Constructor for chatwdg
***************************************************************************/
chatwdg::chatwdg(QWidget *parent)
{
  QGridLayout *gl;

  setParent(parent);
  cb = new QCheckBox("");
  cb->setToolTip(_("Allies only"));
  cb->setChecked(gui_options.gui_qt_allied_chat_only);
  gl = new QGridLayout;
  chat_line = new chat_input;
  chat_output = new text_browser_dblclck(this);
  chat_output->setFont(*fc_font::instance()->get_font(fonts::chatline));
  remove_links = new QPushButton("");
  remove_links->setIcon(style()->standardPixmap(QStyle::SP_DialogCancelButton));
  remove_links->setToolTip(_("Clear links"));
  gl->setVerticalSpacing(0);
  gl->addWidget(chat_output,0 , 0, 1 ,3);
  gl->addWidget(chat_line,1, 0);
  gl->addWidget(cb,1,1);
  gl->addWidget(remove_links,1,2);
  gl->setContentsMargins(0, 0, 6, 0);
  setLayout(gl);
  chat_output->setReadOnly(true);
  chat_line->installEventFilter(this);
  chat_output->setVisible(true);
  chat_output->setAcceptRichText(true);
  chat_output->setOpenLinks(false);
  chat_output->setReadOnly(true);
  connect(chat_output, &QTextBrowser::anchorClicked,
          this, &chatwdg::anchor_clicked);
  connect(chat_output, &QTextBrowser::anchorClicked,
          this, &chatwdg::anchor_clicked);
  connect(chat_output, &text_browser_dblclck::dbl_clicked,
          this, &chatwdg::toggle_size);
  connect(remove_links, &QAbstractButton::clicked, this, &chatwdg::rm_links);
  connect(cb, &QCheckBox::stateChanged, this, &chatwdg::state_changed);
  setMouseTracking(true);

  chat_listener::listen();
}

/***********************************************************************//**
  Manages "To allies" chat button state
***************************************************************************/
void chatwdg::state_changed(int state)
{
  if (state > 0) {
    gui_options.gui_qt_allied_chat_only = true;
  } else {
    gui_options.gui_qt_allied_chat_only = false;
  }
}

/***********************************************************************//**
  Toggle chat size
***************************************************************************/
void chatwdg::toggle_size()
{
  if (gui()->infotab->chat_maximized == true) {
    gui()->infotab->restore_chat();
    return;
  }
  if (gui()->infotab->chat_maximized == false) {
    gui()->infotab->maximize_chat();
    chat_line->setFocus();
  }
}

/***********************************************************************//**
  Scrolls chat to bottom
***************************************************************************/
void chatwdg::scroll_to_bottom()
{
   chat_output->verticalScrollBar()->setSliderPosition(
                                 chat_output->verticalScrollBar()->maximum());
}


/***********************************************************************//**
  Updates font for chatwdg
***************************************************************************/
void chatwdg::update_font()
{
  QFont *qf;
  qf = fc_font::instance()->get_font(fonts::chatline);
  chat_output->setFont(*qf);
}

/***********************************************************************//**
  User clicked clear links button
***************************************************************************/
void chatwdg::rm_links()
{
  link_marks_clear_all();
}

/***********************************************************************//**
  User clicked some custom link
***************************************************************************/
void chatwdg::anchor_clicked(const QUrl &link)
{
  int n;
  QStringList sl;
  int id;
  enum text_link_type type;
  sl = link.toString().split(",");
  n = sl.at(0).toInt();
  id = sl.at(1).toInt();

  type = static_cast<text_link_type>(n);
  struct tile *ptile = NULL;
  switch (type) {
  case TLT_CITY: {
    struct city *pcity = game_city_by_number(id);

    if (pcity) {
      ptile = client_city_tile(pcity);
    } else {
      output_window_append(ftc_client, _("This city isn't known!"));
    }
  }
  break;
  case TLT_TILE:
    ptile = index_to_tile(&(wld.map), id);

    if (!ptile) {
      output_window_append(ftc_client,
                           _("This tile doesn't exist in this game!"));
    }
    break;
  case TLT_UNIT: {
    struct unit *punit = game_unit_by_number(id);

    if (punit) {
      ptile = unit_tile(punit);
    } else {
      output_window_append(ftc_client, _("This unit isn't known!"));
    }
  }
  break;
  }
  if (ptile) {
    center_tile_mapcanvas(ptile);
    link_mark_restore(type, id);
  }
}

/***********************************************************************//**
  Adds news string to chatwdg (from chat_listener interface)
***************************************************************************/
void chatwdg::chat_message_received(const QString& message,
                                    const struct text_tag_list *tags)
{
  QColor col = chat_output->palette().color(QPalette::Text);
  append(apply_tags(message, tags, col));
}

/***********************************************************************//**
  Adds news string to chatwdg
***************************************************************************/
void chatwdg::append(const QString &str)
{
  QTextCursor cursor;

  chat_output->append(str);
  chat_output->verticalScrollBar()->setSliderPosition(
                              chat_output->verticalScrollBar()->maximum());
}

/***********************************************************************//**
  Draws semi-transparent background
***************************************************************************/
void chatwdg::paint(QPainter *painter, QPaintEvent *event)
{
  painter->setBrush(QColor(0, 0, 0, 35));
  painter->drawRect(0, 0, width(), height());
}

/***********************************************************************//**
  Paint event for chatwdg
***************************************************************************/
void chatwdg::paintEvent(QPaintEvent *event)
{
  QPainter painter;

  painter.begin(this);
  paint(&painter, event);
  painter.end();
}

/***********************************************************************//**
  Processess history for chat
***************************************************************************/
bool chatwdg::eventFilter(QObject *obj, QEvent *event)
{
  if (obj == chat_line) {
    if (event->type() == QEvent::KeyPress) {
      QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
      if (keyEvent->key() == Qt::Key_Escape) {
        gui()->infotab->restore_chat();
        gui()->mapview_wdg->setFocus();
        return true;
      }
    }
    if (event->type() == QEvent::ShortcutOverride) {
      event->setAccepted(true);
    }
  }
  return QObject::eventFilter(obj, event);
}

/***********************************************************************//**
  Hides allies and links button for local game
***************************************************************************/
void chatwdg::update_widgets()
{
  if (is_server_running()) {
    cb->hide();
    remove_links->hide();
  } else {
    cb->show();
    remove_links->show();
  }
}

/***********************************************************************//**
  Sets chat to show only X(lines) lines
***************************************************************************/
int chatwdg::default_size(int lines)
{
  int line_count = 0;
  int line_height;
  int size;
  QTextBlock qtb;

  qtb = chat_output->document()->firstBlock();
  /* Count all lines in all text blocks layouts
   * document()->lineCount returns numer of lines without wordwrap */

  while (qtb.isValid()) {
    line_count = line_count + qtb.layout()->lineCount();
    qtb = qtb.next();
  }
  line_height = (chat_output->document()->size().height()
                 - 2 * chat_output->document()->documentMargin())
                / line_count;

  size = lines * line_height
         + chat_line->size().height() + chat_output->document()->documentMargin();
  size = qMax(0, size);
  return size;
}


/***********************************************************************//**
  Makes link to tile/unit or city
***************************************************************************/
void chatwdg::make_link(struct tile *ptile)
{
  struct unit *punit;
  char buf[MAX_LEN_MSG];

  punit = find_visible_unit(ptile);
  if (tile_city(ptile)) {
    sz_strlcpy(buf, city_link(tile_city(ptile)));
  } else if (punit) {
    sz_strlcpy(buf, unit_link(punit));
  } else {
    sz_strlcpy(buf, tile_link(ptile));
  }
  chat_line->insert(QString(buf));
  chat_line->setFocus();
}


/***********************************************************************//**
  Applies tags to text
***************************************************************************/
QString apply_tags(QString str, const struct text_tag_list *tags,
                   QColor bg_color)
{
  int start, stop, last_i;
  QString str_col;
  QString color;
  QString final_string;
  QByteArray qba;
  QColor qc;
  QMultiMap <int, QString> mm;
  if (tags == NULL) {
    return str;
  }
  qba = str.toLocal8Bit().data();
  text_tag_list_iterate(tags, ptag) {
    if ((text_tag_stop_offset(ptag) == FT_OFFSET_UNSET)) {
      stop = qba.count();
    } else {
      stop = text_tag_stop_offset(ptag);
    }

    if ((text_tag_start_offset(ptag) == FT_OFFSET_UNSET)) {
      start = 0;
    } else {
      start = text_tag_start_offset(ptag);
    }
    switch (text_tag_type(ptag)) {
    case TTT_BOLD:
      mm.insert(stop, "</b>");
      mm.insert(start, "<b>");
      break;
    case TTT_ITALIC:
      mm.insert(stop, "</i>");
      mm.insert(start, "<i>");
      break;
    case TTT_STRIKE:
      mm.insert(stop, "</s>");
      mm.insert(start, "<s>");
      break;
    case TTT_UNDERLINE:
      mm.insert(stop, "</u>");
      mm.insert(start, "<u>");
      break;
    case TTT_COLOR:
      if (text_tag_color_foreground(ptag)) {
        color = text_tag_color_foreground(ptag);
          if (color == "#00008B") {
            color = bg_color.name();
          } else {
            qc.setNamedColor(color);
            qc = qc.lighter(200);
            color = qc.name();
          }
          str_col = QString("<span style=color:%1>").arg(color);
          mm.insert(stop, "</span>");
          mm.insert(start, str_col);
      }
      if (text_tag_color_background(ptag)) {
        color = text_tag_color_background(ptag);
        if (QColor::isValidColor(color)) {
          str_col = QString("<span style= background-color:%1;>").arg(color);
          mm.insert(stop, "</span>");
          mm.insert(start, str_col);
        }
      }
      break;
    case TTT_LINK: {
      struct color *pcolor = NULL;

      switch (text_tag_link_type(ptag)) {
      case TLT_CITY:
        pcolor = get_color(tileset, COLOR_MAPVIEW_CITY_LINK);
        break;
      case TLT_TILE:
        pcolor = get_color(tileset, COLOR_MAPVIEW_TILE_LINK);
        break;
      case TLT_UNIT:
        pcolor = get_color(tileset, COLOR_MAPVIEW_UNIT_LINK);
        break;
      }

      if (!pcolor) {
        break; /* Not a valid link type case. */
      }
      color = pcolor->qcolor.name(QColor::HexRgb);
      str_col = QString("<font color=\"%1\">").arg(color);
      mm.insert(stop, "</a></font>");

      color = QString(str_col + "<a href=%1,%2>").
              arg(QString::number(text_tag_link_type(ptag)),
                  QString::number(text_tag_link_id(ptag)));
      mm.insert(start, color);
    }
    }
  } text_tag_list_iterate_end;

  /* insert html starting from last items */
  last_i = str.count();
  QMultiMap<int, QString>::const_iterator i = mm.constEnd();
  QMultiMap<int, QString>::const_iterator j = mm.constEnd();
  while (i != mm.constBegin()) {
    --i;
    if (i.key() < last_i) {
      final_string = final_string.prepend(QString(qba.mid(i.key(),
                                                          last_i - i.key()))
                                          .toHtmlEscaped());
    }
    last_i = i.key();
    j = i;
    if (i != mm.constBegin()) {
      j--;
    }
    if (j.key() == i.key() && i != j) {
      final_string = final_string.prepend(j.value());
      final_string = final_string.prepend(i.value());
      i--;
    } else {
      final_string = final_string.prepend(i.value());
    }
  }
  if (last_i == str.count()) {
    return str;
  }

  return final_string;
}

/***********************************************************************//**
  Helper function to determine if a given client input line is intended as
  a "plain" public message.
***************************************************************************/
static bool is_plain_public_message(QString s)
{
  QString s1, str;
  int i;

  str = s.trimmed();
  if (str.at(0) == SERVER_COMMAND_PREFIX
      || str.at(0) == CHAT_ALLIES_PREFIX
      || str.at(0) == CHAT_DIRECT_PREFIX) {
    return false;
  }

  /* Search for private message */
  if (!str.contains(CHAT_DIRECT_PREFIX)) {
    return true;
  }
  i = str.indexOf(CHAT_DIRECT_PREFIX);
  str = str.left(i);

  /* Compare all players and connections looking for match */
  conn_list_iterate(game.all_connections, pconn) {
    s1 = pconn->username;
    if (s1.length() < i) {
      continue;
    }
    if (!QString::compare(s1.left(i), str, Qt::CaseInsensitive)) {
      return false;
    }
  } conn_list_iterate_end;
  players_iterate(pplayer) {
    s1 = pplayer->name;
    if (s1.length() < i) {
      continue;
    }
    if (!QString::compare(s1.left(i), str, Qt::CaseInsensitive)) {
      return false;
    }
  } players_iterate_end;

  return true;
}

/***********************************************************************//**
  Appends the string to the chat output window.  The string should be
  inserted on its own line, although it will have no newline.
***************************************************************************/
void qtg_real_output_window_append(const char *astring,
                                   const struct text_tag_list *tags,
                                   int conn_id)
{
  QString str;
  QString wakeup;

  str = QString::fromUtf8(astring);
  gui()->set_status_bar(str);

  wakeup = gui_options.gui_qt_wakeup_text;

  /* Format wakeup string if needed */
  if (wakeup.contains("%1")) {
    wakeup = wakeup.arg(client.conn.username);
  }

  if (str.contains(client.conn.username)) {
    qapp->alert(gui()->central_wdg);
  }

  /* Play sound if we encountered wakeup string */
  if (str.contains(wakeup) && client_state() < C_S_RUNNING
      && !wakeup.isEmpty()) {
    qapp->alert(gui()->central_wdg);
    audio_play_sound("e_player_wake", NULL);
  }

  chat_listener::update_word_list();
  chat_listener::invoke(&chat_listener::chat_message_received,
                        str, tags);
}

/***********************************************************************//**
  Get the text of the output window, and call write_chatline_content() to
  log it.
***************************************************************************/
void log_output_window(void)
{
  /* PORTME */
  write_chatline_content(NULL);
}

/***********************************************************************//**
  Clear all text from the output window.
***************************************************************************/
void clear_output_window(void)
{
  /* PORTME */
#if 0
  set_output_window_text(_("Cleared output window."));
#endif
}

/***********************************************************************//**
  Version message event constructor.
***************************************************************************/
version_message_event::version_message_event(const QString &msg) :
  QEvent(QEvent::User),
  message(msg)
{}

/***********************************************************************//**
  Got version message from metaserver thread.
***************************************************************************/
void qtg_version_message(const char *vertext)
{
  current_app()->postEvent(gui(), new version_message_event(vertext));
}
