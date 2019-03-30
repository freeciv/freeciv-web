/***********************************************************************
 Freeciv - Copyright (C) 1996-2005 - Freeciv Development Team
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
#include <QAction>
#include <QApplication>
#include <QDesktopWidget>
#include <QHBoxLayout>
#include <QMenu>
#include <QPainter>
#include <QPaintEvent>
#include <QPixmap>
#include <QTimer>

// common
#include "research.h"

// client
#include "client_main.h"

// gui-qt
#include "fc_client.h"
#include "repodlgs.h"
#include "sidebar.h"
#include "sprite.h"

extern void pixmap_copy(QPixmap *dest, QPixmap *src, int src_x, int src_y,
                        int dest_x, int dest_y, int width, int height);

static void reduce_mod(int &val,  int &mod);

/***********************************************************************//**
  Helper function to fit tax sprites, reduces modulo, increasing value
***************************************************************************/
void reduce_mod(int &mod,  int &val)
{
  if (mod > 0) {
    val++;
    mod--;
  }

  return;
}

/***********************************************************************//**
  Sidewidget constructor
***************************************************************************/
fc_sidewidget::fc_sidewidget(QPixmap *pix, QString label, QString pg,
                             pfcn_bool func, int type): QWidget()
{
  if (pix == nullptr) {
    pix = new QPixmap(12,12);
    pix->fill(Qt::black);
  }
  blink = false;
  disabled = false;
  def_pixmap = pix;
  scaled_pixmap = new QPixmap;
  final_pixmap = new QPixmap;
  sfont = new QFont(*fc_font::instance()->get_font(fonts::notify_label));
  left_click = func;
  desc = label;
  standard = type;
  hover = false;
  right_click = nullptr;
  wheel_down = nullptr;
  wheel_up = nullptr;
  page = pg;
  setContextMenuPolicy(Qt::CustomContextMenu);
  timer = new QTimer;
  timer->setSingleShot(false);
  timer->setInterval(700);
  sfont->setCapitalization(QFont::SmallCaps);
  sfont->setItalic(true);
  info_font = new  QFont(*sfont);
  info_font->setBold(true);
  info_font->setItalic(false);
  connect(timer, &QTimer::timeout, this, &fc_sidewidget::sblink);
}

/***********************************************************************//**
  Sidewidget destructor
***************************************************************************/
fc_sidewidget::~fc_sidewidget()
{
  if (scaled_pixmap) {
    delete scaled_pixmap;
  }

  if (def_pixmap) {
    delete def_pixmap;
  }

  if (final_pixmap) {
    delete final_pixmap;
  }
  delete timer;
  delete sfont;
  delete info_font;
}

/***********************************************************************//**
  Sets default pixmap for sidewidget
***************************************************************************/
void fc_sidewidget::set_pixmap(QPixmap *pm)
{
  if (def_pixmap) {
    delete def_pixmap;
  }

  def_pixmap = pm;
}

/***********************************************************************//**
  Sets custom text visible on top of sidewidget
***************************************************************************/
void fc_sidewidget::set_custom_labels(QString l)
{
  custom_label = l;
}

/***********************************************************************//**
  Sets tooltip for sidewidget
***************************************************************************/
void fc_sidewidget::set_tooltip(QString tooltip)
{
  setToolTip(tooltip);
}

/***********************************************************************//**
  Returns scaled (not default) pixmap for sidewidget
***************************************************************************/
QPixmap *fc_sidewidget::get_pixmap()
{
  return scaled_pixmap;
}

/***********************************************************************//**
  Sets default label on bottom of sidewidget
***************************************************************************/
void fc_sidewidget::set_label(QString str)
{
  desc = str;
}

/***********************************************************************//**
  Resizes default_pixmap to scaled_pixmap to fit current width,
  leaves default_pixmap unchanged
***************************************************************************/
void fc_sidewidget::resize_pixmap(int width, int height)
{
  if (standard == SW_TAX) {
    height = get_tax_sprite(tileset, O_LUXURY)->pm->height() + 8;
  }

  if (standard == SW_INDICATORS) {
    height = client_government_sprite()->pm->height() + 8;
  }

  if (def_pixmap) {
    *scaled_pixmap = def_pixmap->scaled(width, height, Qt::IgnoreAspectRatio,
                                        Qt::SmoothTransformation);
  }
}

/***********************************************************************//**
  Paint event for sidewidget
***************************************************************************/
void fc_sidewidget::paintEvent(QPaintEvent *event)
{
  QPainter painter;

  painter.begin(this);
  paint(&painter, event);
  painter.end();
}

/***********************************************************************//**
  Paints final pixmap on screeen
***************************************************************************/
void fc_sidewidget::paint(QPainter *painter, QPaintEvent *event)
{
  if (final_pixmap) {
    painter->drawPixmap(event->rect(), *final_pixmap,
                        event->rect());
  }
}

/***********************************************************************//**
  Mouse entered on widget area
***************************************************************************/
void fc_sidewidget::enterEvent(QEvent *event)
{
  if (hover == false) {
    hover = true;
    update_final_pixmap();
    QWidget::enterEvent(event);
    update();
  }
}

/***********************************************************************//**
  Mouse left widget area
***************************************************************************/
void fc_sidewidget::leaveEvent(QEvent *event)
{
  if (hover) {
    hover = false;
    update_final_pixmap();
    QWidget::leaveEvent(event);
    update();

  }
}

/***********************************************************************//**
  Context menu requested
***************************************************************************/
void fc_sidewidget::contextMenuEvent(QContextMenuEvent *event)
{
  if (hover) {
    hover = false;
    update_final_pixmap();
    QWidget::contextMenuEvent(event);
    update();
  }
}

/***********************************************************************//**
  Sets callback for mouse left click
***************************************************************************/
void fc_sidewidget::set_left_click(pfcn_bool func)
{
  left_click = func;
}

/***********************************************************************//**
  Sets callback for mouse right click
***************************************************************************/
void fc_sidewidget::set_right_click(pfcn func)
{
  right_click = func;
}

/***********************************************************************//**
  Sets callback for mouse wheel down
***************************************************************************/
void fc_sidewidget::set_wheel_down(pfcn func)
{
  wheel_down = func;
}

/***********************************************************************//**
  Sets callback for mouse wheel up
***************************************************************************/
void fc_sidewidget::set_wheel_up(pfcn func)
{
  wheel_up = func;
}

/***********************************************************************//**
  Mouse press event for sidewidget
***************************************************************************/
void fc_sidewidget::mousePressEvent(QMouseEvent *event)
{
  if (event->button() == Qt::LeftButton && left_click != nullptr) {
    left_click(true);
  }
  if (event->button() == Qt::RightButton && right_click != nullptr) {
    right_click();
  }
  if (event->button() == Qt::RightButton && right_click == nullptr) {
    gui()->game_tab_widget->setCurrentIndex(0);
  }
}

/***********************************************************************//**
  Mouse wheel event
***************************************************************************/
void fc_sidewidget::wheelEvent(QWheelEvent *event)
{
  if (event->delta() < -90 && wheel_down) {
    wheel_down();
  } else if (event->delta() > 90 && wheel_up) {
    wheel_up();
  }

  event->accept();
}

/***********************************************************************//**
  Blinks current sidebar widget
***************************************************************************/
void fc_sidewidget::sblink()
{
  if (keep_blinking) {
    if (timer->isActive() == false) {
      timer->start();
    }
    blink = !blink;
  } else {
    blink = false;
    if (timer->isActive()) {
      timer->stop();
    }
  }
  update_final_pixmap();
}

/***********************************************************************//**
  Miscelanous slot, helping observe players currently, and changing science
  extra functionality might be added,
  eg by setting properties
***************************************************************************/
void fc_sidewidget::some_slot()
{
  QVariant qvar;
  struct player *obs_player;
  QAction *act;

  act = qobject_cast<QAction *>(sender());
  qvar = act->data();

  if (qvar.isValid() == false) {
    return;
  }

  if (act->property("scimenu") == true) {
    dsend_packet_player_research(&client.conn, qvar.toInt());
    return;
  }

  if (qvar.toInt() == -1) {
    send_chat("/observe");
    return;
  }

  obs_player = reinterpret_cast<struct player *>(qvar.value<void *>());
  if (obs_player != nullptr) {
    QString s;
    s = QString("/observe \"%1\"").arg(obs_player->name);
    send_chat(s.toLocal8Bit().data());
  }
}

/***********************************************************************//**
  Updates final pixmap and draws it on screen
***************************************************************************/
void fc_sidewidget::update_final_pixmap()
{
  const struct sprite *sprite;
  int w, h, pos, i;
  QPainter p;
  QPen pen;
  bool current = false;

  if (final_pixmap) {
    delete final_pixmap;
  }

  i = gui()->gimme_index_of(page);
  if (i == gui()->game_tab_widget->currentIndex()) {
    current = true;
  }
  final_pixmap = new QPixmap(scaled_pixmap->width(), scaled_pixmap->height());
  final_pixmap->fill(Qt::transparent);

  if (scaled_pixmap->width() == 0 || scaled_pixmap->height() == 0) {
    return;
  }

  p.begin(final_pixmap);
  p.setFont(*sfont);
  pen.setColor(QColor(232, 255, 0));
  p.setPen(pen);

  if (standard == SW_TAX && client_is_global_observer() == false) {
    pos = 0;
    int d, modulo;
    sprite = get_tax_sprite(tileset, O_GOLD);
    if (sprite == nullptr) {
      return;
    }
    w = width() / 10;
    modulo = width() % 10;
    h = sprite->pm->height();
    reduce_mod(modulo, pos);
    if (client.conn.playing == nullptr) {
      return;
    }
    for (d = 0; d < client.conn.playing->economic.tax / 10; ++d) {
      p.drawPixmap(pos, 5, sprite->pm->scaled(w, h), 0, 0, w, h);
      pos = pos + w;
      reduce_mod(modulo, pos);
    }

    sprite = get_tax_sprite(tileset, O_SCIENCE);

    for (; d < (client.conn.playing->economic.tax
                + client.conn.playing->economic.science) / 10; ++d) {
      p.drawPixmap(pos, 5, sprite->pm->scaled(w, h), 0, 0, w, h);
      pos = pos + w;
      reduce_mod(modulo, pos);
    }

    sprite = get_tax_sprite(tileset, O_LUXURY);

    for (; d < 10 ; ++d) {
      p.drawPixmap(pos, 5, sprite->pm->scaled(w, h), 0, 0, w, h);
      pos = pos + w;
      reduce_mod(modulo, pos);
    }
  } else if (standard == SW_INDICATORS) {
    sprite = client_research_sprite();
    w = sprite->pm->width();
    pos = scaled_pixmap->width() / 2 - 2 * w;
    p.drawPixmap(pos, 5, *sprite->pm);
    pos = pos + w;
    sprite = client_warming_sprite();
    p.drawPixmap(pos, 5, *sprite->pm);
    pos = pos + w;
    sprite = client_cooling_sprite();
    p.drawPixmap(pos, 5, *sprite->pm);
    pos = pos + w;
    sprite = client_government_sprite();
    p.drawPixmap(pos, 5, *sprite->pm);

  } else {
    p.drawPixmap(0, 0 , *scaled_pixmap);
    p.drawText(0, height() - 6 , desc);
  }

  p.setPen(palette().color(QPalette::Text));
  if (custom_label.isEmpty() == false) {
    p.setFont(*info_font);
    p.drawText(0, 0, width(), height(), Qt::AlignLeft | Qt::TextWordWrap,
               custom_label);
  }

  if (current) {
    p.setPen(palette().color(QPalette::Highlight));
    p.drawRect(0 , 0, width() - 1 , height() - 1);
  }

  if (hover && !disabled) {
    p.setCompositionMode(QPainter::CompositionMode_ColorDodge);
    p.setPen(palette().color(QPalette::Highlight));
    p.setBrush(palette().color(QPalette::AlternateBase));
    p.drawRect(0 , 0, width() - 1 , height() - 1);
  }

  if (disabled) {
    p.setCompositionMode(QPainter::CompositionMode_Darken);
    p.setPen(QColor(0, 0, 0));
    p.setBrush(QColor(0, 0, 50, 95));
    p.drawRect(0 , 0, width(), height());
  }

  if (blink) {
    p.setCompositionMode(QPainter::CompositionMode_ColorDodge);
    p.setPen(QColor(0, 0, 0));
    p.setBrush(palette().color(QPalette::HighlightedText));
    p.drawRect(0 , 0, width(), height());
  }

  p.end();
  update();
}

/***********************************************************************//**
  Sidebar constructor
***************************************************************************/
fc_sidebar::fc_sidebar()
{
  setAttribute(Qt::WA_OpaquePaintEvent, true);
  layout = new QVBoxLayout;
  layout->setContentsMargins(0, 0, 0, 0);
  setLayout(layout);
  setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Ignored);
}

/***********************************************************************//**
  Sidebar destructor
***************************************************************************/
fc_sidebar::~fc_sidebar()
{
}

/***********************************************************************//**
  Adds new sidebar widget
***************************************************************************/
void fc_sidebar::add_widget(fc_sidewidget *fsw)
{
  objects.append(fsw);
  layout->addWidget(fsw);
  return;
}

/***********************************************************************//**
  Paint event for sidebar
***************************************************************************/
void fc_sidebar::paintEvent(QPaintEvent *event)
{
  QPainter painter;

  painter.begin(this);
  paint(&painter, event);
  painter.end();
}

/***********************************************************************//**
  Paints dark rectangle as background for sidebar
***************************************************************************/
void fc_sidebar::paint(QPainter *painter, QPaintEvent *event)
{
  painter->setBrush(QBrush(QColor(40, 40, 40)));
  painter->drawRect(event->rect());
}

/**************************************************************************
  Resize sidebar to take at least 80 pixels width and 100 pixels for FullHD
  desktop and scaled accordingly for bigger resolutions eg 200 pixels for 4k
  desktop.
**************************************************************************/
void fc_sidebar::resize_me(int hght, bool force)
{
  int w, h, non_std, non_std_count, screen_hres;
  QDesktopWidget *qdp;

  h = hght;
  qdp = QApplication::desktop();
  screen_hres = qdp->availableGeometry(gui()->central_wdg).width();
  w = (100 * screen_hres) / 1920;
  w = qMax(w, 80);

  if (force == false && w == width() && h == height()) {
    return;
  }

  non_std = 0;
  non_std_count = 0;

  /* resize all non standard sidewidgets first*/
  foreach (fc_sidewidget * sw,  objects) {
    if (sw->standard != SW_STD) {
      sw->resize_pixmap(w, 0);
      sw->setFixedSize(w, sw->get_pixmap()->height());
      sw->update_final_pixmap();
      non_std = non_std + sw->get_pixmap()->height();
      non_std_count++;
    }
  }

  h = h - non_std;
  h = h / (objects.count() - non_std_count) - 2;
  /* resize all standard sidewidgets */
  foreach (fc_sidewidget * sw,  objects) {
    if (sw->standard == SW_STD) {
      sw->resize_pixmap(w, h);
      sw->setFixedSize(w, h);
      sw->update_final_pixmap();
    }
  }
}


/***********************************************************************//**
  Callback to show map
***************************************************************************/
void side_show_map(bool nothing)
{
  gui()->game_tab_widget->setCurrentIndex(0);
}

/***********************************************************************//**
  Callback for finishing turn
***************************************************************************/
void side_finish_turn(bool nothing)
{
  key_end_turn();
}

/***********************************************************************//**
  Callback to popup rates dialog
***************************************************************************/
void side_rates_wdg(bool nothing)
{
  if (client_is_observer() == false) {
    popup_rates_dialog();
  }
}

/***********************************************************************//**
  Callback to center on current unit
***************************************************************************/
void side_center_unit()
{
  gui()->game_tab_widget->setCurrentIndex(0);
  request_center_focus_unit();
}

/***********************************************************************//**
  Disables end turn button if asked
***************************************************************************/
void side_disable_endturn(bool do_restore)
{
  if (gui()->current_page() != PAGE_GAME) {
    return;
  }
  gui()->sw_endturn->disabled = !do_restore;
  gui()->sw_endturn->update_final_pixmap();
}

/***********************************************************************//**
  Changes background of endturn widget if asked
***************************************************************************/
void side_blink_endturn(bool do_restore)
{
  if (gui()->current_page() != PAGE_GAME) {
    return;
  }
  gui()->sw_endturn->blink = !do_restore;
  gui()->sw_endturn->update_final_pixmap();
}

/***********************************************************************//**
  Popups menu on indicators widget
***************************************************************************/
void side_indicators_menu()
{
  gov_menu *menu = new gov_menu(gui()->sidebar_wdg);

  menu->create();
  menu->update();
  menu->popup(QCursor::pos());
}

/***********************************************************************//**
  Right click for diplomacy 
  Opens diplomacy meeting for player
  For observer popups menu
***************************************************************************/
void side_right_click_diplomacy(void)
{
  if (client_is_observer()) {
    QMenu *menu = new QMenu(gui()->central_wdg);
    QAction *eiskalt;
    QString erwischt;

    players_iterate(pplayer) {
      if (pplayer == client.conn.playing) {
        continue;
      }
      erwischt = QString(_("Observe %1")).arg(pplayer->name);
      erwischt = erwischt + " ("
                 + nation_plural_translation(pplayer->nation) + ")";
      eiskalt = new QAction(erwischt, gui()->mapview_wdg);
      eiskalt->setData(QVariant::fromValue((void *)pplayer));
      QObject::connect(eiskalt, &QAction::triggered, gui()->sw_diplo,
                       &fc_sidewidget::some_slot);
      menu->addAction(eiskalt);
    } players_iterate_end

    if (client_is_global_observer() == false) {
      eiskalt = new QAction(_("Observe globally"), gui()->mapview_wdg);
      eiskalt->setData(-1);
      menu->addAction(eiskalt);
      QObject::connect(eiskalt, &QAction::triggered, gui()->sw_diplo,
                       &fc_sidewidget::some_slot);
    }

    menu->exec(QCursor::pos());
  } else {
    int i;
    i = gui()->gimme_index_of("DDI");
    if (i < 0) {
      return;
    }
    gui()->game_tab_widget->setCurrentIndex(i);
  }
}

/***********************************************************************//**
  Right click for science, allowing to choose current tech
***************************************************************************/
void side_right_click_science(void)
{
  QMenu *menu;
  QAction *act;
  QVariant qvar;
  QList<qlist_item> curr_list;
  qlist_item item;

  if (client_is_observer() == false) {
    struct research *research = research_get(client_player());

    advance_index_iterate(A_FIRST, i) {
      if (TECH_PREREQS_KNOWN == research->inventions[i].state
        && research->researching != i) {
        item.tech_str =
          QString::fromUtf8(advance_name_translation(advance_by_number(i)));
        item.id = i;
        curr_list.append(item);
      }
    } advance_index_iterate_end;
    if (curr_list.isEmpty()) {
      return;
    }
    std::sort(curr_list.begin(), curr_list.end(), comp_less_than);
    menu = new QMenu(gui()->central_wdg);
    for (int i = 0; i < curr_list.count(); i++) {
      QIcon ic;
      struct sprite *sp;

      qvar = curr_list.at(i).id;
      sp = get_tech_sprite(tileset, curr_list.at(i).id);
      if (sp) {
        ic = QIcon(*sp->pm);
      }
      act = new QAction(ic, curr_list.at(i).tech_str, gui()->mapview_wdg);
      act->setData(qvar);
      act->setProperty("scimenu", true);
      menu->addAction(act);
      QObject::connect(act, &QAction::triggered, gui()->sw_science,
                       &fc_sidewidget::some_slot);
    }
    menu->exec(QCursor::pos());
  }
}

/***********************************************************************//**
  Left click for science, allowing to close/open
***************************************************************************/
void side_left_click_science(bool nothing)
{
  science_report *sci_rep;
  int i;
  QWidget *w;

  if (client_is_global_observer()) {
    return;
  }
  if (!gui()->is_repo_dlg_open("SCI")) {
    sci_rep = new science_report;
    sci_rep->init(true);
  } else {
    i = gui()->gimme_index_of("SCI");
    w = gui()->game_tab_widget->widget(i);
    if (w->isVisible() == true) {
      gui()->game_tab_widget->setCurrentIndex(0);
      return;
    }
    sci_rep = reinterpret_cast<science_report*>(w);
    gui()->game_tab_widget->setCurrentWidget(sci_rep);
  }
}
