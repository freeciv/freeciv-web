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
#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QLineEdit>
#include <QScrollArea>
#include <QSettings>
#include <QSignalMapper>
#include <QVBoxLayout>
#include <QWidget>

// client
#include "options.h"

// gui-qt
#include "fc_client.h"
#include "shortcuts.h"

extern "C" void real_menus_init();

static QHash<int, const char *> key_map;
static QString button_name(Qt::MouseButton bt);
static QMap<shortcut_id, fc_shortcut*>* hash_copy(QMap<shortcut_id, fc_shortcut*> *h);
fc_shortcuts *fc_shortcuts::m_instance = 0;
QMap<shortcut_id, fc_shortcut*> fc_shortcuts::hash = QMap<shortcut_id, fc_shortcut*>();

enum {
  RESPONSE_CANCEL,
  RESPONSE_OK,
  RESPONSE_APPLY,
  RESPONSE_RESET,
  RESPONSE_SAVE
};

static int num_shortcuts = 57;
fc_shortcut default_shortcuts[] = {
  {SC_SCROLL_MAP, 0, Qt::RightButton, Qt::NoModifier, "Scroll map" },
  {SC_CENTER_VIEW, Qt::Key_C, Qt::AllButtons, Qt::NoModifier,
    _("Center View") },
  {SC_FULLSCREEN, Qt::Key_Return, Qt::AllButtons, Qt::AltModifier,
    _("Fullscreen") },
  {SC_MINIMAP, Qt::Key_M, Qt::AllButtons, Qt::ControlModifier,
    _("Show minimap") },
  {SC_CITY_OUTPUT, Qt::Key_W, Qt::AllButtons, Qt::ControlModifier,
    _("City Output") },
  {SC_MAP_GRID, Qt::Key_G, Qt::AllButtons, Qt::ControlModifier,
    _("Map Grid") },
  {SC_NAT_BORDERS, Qt::Key_B, Qt::AllButtons, Qt::ControlModifier,
    _("National Borders") },
  {SC_QUICK_BUY, 0, Qt::LeftButton, Qt::ControlModifier | Qt::ShiftModifier,
    _("Quick buy from map") },
  {SC_QUICK_SELECT, 0, Qt::LeftButton, Qt::ControlModifier,
    _("Quick production select from map") },
  {SC_SELECT_BUTTON, 0, Qt::LeftButton, Qt::NoModifier,
    _("Select button") },
  {SC_ADJUST_WORKERS, 0, Qt::LeftButton,
    Qt::MetaModifier | Qt::ControlModifier, _("Adjust workers") },
  {SC_APPEND_FOCUS, 0, Qt::LeftButton, Qt::MetaModifier,
    _("Append focus") },
  {SC_POPUP_INFO, 0, Qt::MiddleButton, Qt::NoModifier,
    _("Popup tile info") },
  {SC_WAKEUP_SENTRIES, 0, Qt::MiddleButton, Qt::ControlModifier,
    _("Wakeup sentries") },
  {SC_MAKE_LINK, 0, Qt::RightButton, Qt::ControlModifier | Qt::AltModifier,
    _("Show link to tile") },
  {SC_PASTE_PROD, 0, Qt::RightButton,
    Qt::ShiftModifier | Qt::ControlModifier, _("Paste production") },
  {SC_COPY_PROD, 0, Qt::RightButton,
    Qt::ShiftModifier, _("Copy production") },
  {SC_HIDE_WORKERS, 0, Qt::RightButton,
    Qt::ShiftModifier | Qt::AltModifier, _("Show/hide workers") },
  {SC_SHOW_UNITS, Qt::Key_Space, Qt::AllButtons, Qt::ControlModifier,
    _("Units selection (for tile under mouse position)") },
  {SC_TRADE_ROUTES, Qt::Key_D, Qt::AllButtons, Qt::ControlModifier,
    _("City Traderoutes") },
  {SC_CITY_PROD, Qt::Key_P, Qt::AllButtons, Qt::ControlModifier,
    _("City Production Levels") },
  {SC_CITY_NAMES, Qt::Key_N, Qt::AllButtons, Qt::ControlModifier,
    _("City Names") },
  {SC_DONE_MOVING, Qt::Key_Space, Qt::AllButtons, Qt::NoModifier,
    _("Done Moving") },
  {SC_GOTOAIRLIFT, Qt::Key_T, Qt::AllButtons, Qt::NoModifier,
    _("Go to/Airlift to City...") },
  {SC_AUTOEXPLORE, Qt::Key_X, Qt::AllButtons, Qt::NoModifier,
    _("Auto Explore") },
  {SC_PATROL, Qt::Key_Q, Qt::AllButtons, Qt::NoModifier,
    _("Patrol") },
  {SC_UNSENTRY_TILE, Qt::Key_D, Qt::AllButtons,
    Qt::ShiftModifier | Qt::ControlModifier, _("Unsentry All On Tile") },
  {SC_DO, Qt::Key_D, Qt::AllButtons, Qt::NoModifier, _("Do...") },
  {SC_UPGRADE_UNIT, Qt::Key_U, Qt::AllButtons, Qt::ControlModifier,
    _("Upgrade") },
  {SC_SETHOME, Qt::Key_H, Qt::AllButtons, Qt::NoModifier,
    _("Set Home City") },
  {SC_BUILDMINE, Qt::Key_M, Qt::AllButtons, Qt::NoModifier,
    _("Build Mine") },
  {SC_BUILDIRRIGATION, Qt::Key_I, Qt::AllButtons, Qt::NoModifier,
    _("Build Irrigation") },
  {SC_BUILDROAD, Qt::Key_R, Qt::AllButtons, Qt::NoModifier,
    _("Build Road") },
  {SC_BUILDCITY, Qt::Key_B, Qt::AllButtons, Qt::NoModifier,
    _("Build City") },
  {SC_SENTRY, Qt::Key_S, Qt::AllButtons, Qt::NoModifier,
    _("Sentry") },
  {SC_FORTIFY, Qt::Key_F, Qt::AllButtons, Qt::NoModifier,
    _("Fortify") },
  {SC_GOTO, Qt::Key_G, Qt::AllButtons, Qt::NoModifier,
    _("Go to Tile") },
  {SC_WAIT, Qt::Key_W, Qt::AllButtons, Qt::NoModifier,
    _("Wait") },
  {SC_TRANSFORM, Qt::Key_O, Qt::AllButtons, Qt::NoModifier,
    _("Transform") },
  {SC_NUKE, Qt::Key_N, Qt::AllButtons, Qt::ShiftModifier,
    _("Explode Nuclear") },
  {SC_LOAD, Qt::Key_L, Qt::AllButtons, Qt::NoModifier,
    _("Load") },
  {SC_UNLOAD, Qt::Key_U, Qt::AllButtons, Qt::NoModifier,
    _("Unload") },
  {SC_BUY_MAP, 0, Qt::BackButton, Qt::NoModifier,
    _("Quick buy current production from map") },
  {SC_IFACE_LOCK, Qt::Key_L, Qt::AllButtons, Qt::ControlModifier
    | Qt::ShiftModifier, _("Lock/unlock interface") },
  {SC_AUTOMATE, Qt::Key_A, Qt::AllButtons, Qt::NoModifier,
    _("Auto worker") },
  {SC_PARADROP, Qt::Key_P, Qt::AllButtons, Qt::NoModifier,
    _("Paradrop/clean pollution") },
  {SC_POPUP_COMB_INF, Qt::Key_F1, Qt::AllButtons, Qt::ControlModifier,
    _("Popup combat information") },
  {SC_RELOAD_THEME, Qt::Key_F5, Qt::AllButtons, Qt::ControlModifier
     | Qt::ShiftModifier, _("Reload theme") },
  {SC_RELOAD_TILESET, Qt::Key_F6, Qt::AllButtons, Qt::ControlModifier
    | Qt::ShiftModifier, _("Reload tileset") },
  {SC_SHOW_FULLBAR, Qt::Key_F, Qt::AllButtons, Qt::ControlModifier,
    _("Toggle city full bar visibility") },
  {SC_ZOOM_IN, Qt::Key_Plus, Qt::AllButtons, Qt::NoModifier,
    _("Reload zoomed in tileset") },
  {SC_ZOOM_OUT, Qt::Key_Minus, Qt::AllButtons, Qt::NoModifier,
    _("Reload zoomed out tileset") },
  {SC_LOAD_LUA, Qt::Key_J, Qt::AllButtons, Qt::ControlModifier
    | Qt::ShiftModifier, _("Load Lua script") },
  {SC_RELOAD_LUA, Qt::Key_K, Qt::AllButtons, Qt::ControlModifier
    | Qt::ShiftModifier, _("Load last loaded Lua script") },
  {SC_ZOOM_RESET, Qt::Key_Backspace, Qt::AllButtons, Qt::ControlModifier,
    _("Reload tileset with default scale") },
  {SC_GOBUILDCITY, Qt::Key_B, Qt::AllButtons, Qt::ShiftModifier,
    _("Go And Build City") },
  {SC_GOJOINCITY, Qt::Key_J, Qt::AllButtons, Qt::ShiftModifier,
    _("Go And Join City") }
};


/**********************************************************************//**
  Returns shortcut as string (eg. for menu)
**************************************************************************/
QString shortcut_to_string(fc_shortcut *sc)
{
  QString ret, m, bn, k;

  if (sc == nullptr) {
    return "";
  }
  if (sc->mod != Qt::NoModifier) {
    m = QKeySequence(sc->mod).toString(QKeySequence::NativeText);
  }
  if (sc->mouse != Qt::AllButtons) {
    bn = button_name(sc->mouse);
  }
  if (sc->key != 0) {
    k = QKeySequence(sc->key).toString(QKeySequence::NativeText);
  }
  ret = m + bn + k;

  return ret;
}

/**********************************************************************//**
  fc_shortcuts contructor
**************************************************************************/
fc_shortcuts::fc_shortcuts()
{
  init_default(true);
}

/**********************************************************************//**
  fc_shortcuts destructor
**************************************************************************/
fc_shortcuts::~fc_shortcuts()
{
  qDeleteAll(hash.begin(), hash.end());
  hash.clear();
}


/**********************************************************************//**
  Returns description for given shortcut
**************************************************************************/
QString fc_shortcuts::get_desc(shortcut_id id)
{
  fc_shortcut *s;
  s = hash.value(id);
  return s->str;
}

/**********************************************************************//**
  Returns shortcut for given id
**************************************************************************/
fc_shortcut *fc_shortcuts::get_shortcut(shortcut_id id)
{
  fc_shortcut *s;
  s = hash.value(id);
  return s;
}

/**********************************************************************//**
  Returns id for given shortcut
**************************************************************************/
shortcut_id fc_shortcuts::get_id(fc_shortcut *sc)
{
  return hash.key(sc);
}

/**********************************************************************//**
  Sets given shortcut
**************************************************************************/
void fc_shortcuts::set_shortcut(fc_shortcut *s)
{
  fc_shortcut *sc;
  sc = hash.value(s->id);
  sc->key = s->key;
  sc->mod = s->mod;
  sc->mouse = s->mouse;
}

/**********************************************************************//**
  Deletes current instance
**************************************************************************/
void fc_shortcuts::drop()
{
  if (m_instance) {
    delete m_instance;
    m_instance = 0;
  }
}

/**********************************************************************//**
  Returns given instance
**************************************************************************/
fc_shortcuts *fc_shortcuts::sc()
{
  if (!m_instance)
    m_instance = new fc_shortcuts;
  return m_instance;
}

/**********************************************************************//**
  Inits defaults shortcuts or reads from settings
**************************************************************************/
void fc_shortcuts::init_default(bool read)
{
  int i;
  fc_shortcut *s;
  bool suc = false;
  hash.clear();

  if (read) {
    suc = read_shortcuts();
  }
  if (suc == false) {
    for (i = 0 ; i < num_shortcuts; i++) {
      s = new fc_shortcut();
      s->id = default_shortcuts[i].id;
      s->key = default_shortcuts[i].key;
      s->mouse = default_shortcuts[i].mouse;
      s->mod = default_shortcuts[i].mod;
      s->str = default_shortcuts[i].str;
      hash.insert(default_shortcuts[i].id, s);
    }
  }
}

/**********************************************************************//**
  Constructor for setting shortcuts
**************************************************************************/
fc_shortcut_popup::fc_shortcut_popup(QWidget *parent): QDialog()
{
  QVBoxLayout *vb = new QVBoxLayout;
  setParent(parent);
  setAttribute(Qt::WA_DeleteOnClose);
  setWindowFlags(Qt::Popup);
  vb->addWidget(&edit);
  setLayout(vb);
  sc = nullptr;
}

/**********************************************************************//**
  Custom line edit contructor
**************************************************************************/
line_edit::line_edit(): QLineEdit()
{
  setContextMenuPolicy(Qt::NoContextMenu);
  setWindowModality(Qt::WindowModal);
}

/**********************************************************************//**
  Mouse press event for line edit
**************************************************************************/
void line_edit::mousePressEvent(QMouseEvent *event)
{
  fc_shortcut_popup *fcp;
  shc.mouse =  event->button();
  shc.mod = event->modifiers();
  fcp = reinterpret_cast<fc_shortcut_popup *>(parentWidget());
  fcp->sc->mouse = shc.mouse;
  fcp->sc->mod = shc.mod;
  fcp->sc->key = 0;
  parentWidget()->close();
}

/**********************************************************************//**
  Key release event for line edit
**************************************************************************/
void line_edit::keyReleaseEvent(QKeyEvent *event)
{
  fc_shortcut_popup *fcp;

  shc.key = event->key();
  shc.mod = event->modifiers();
  fcp = reinterpret_cast<fc_shortcut_popup *>(parentWidget());
  fcp->sc->mouse = Qt::AllButtons;
  fcp->sc->key = shc.key;
  fcp->sc->mod = shc.mod;
  parentWidget()->close();
}

/**********************************************************************//**
  Popups line edit for setting shortcut
**************************************************************************/
void fc_shortcut_popup::run(fc_shortcut *s)
{
  QPoint p(50, 20);
  edit.setReadOnly(true);
  edit.setFocus();
  setWindowModality(Qt::WindowModal);
  sc = s;
  move(QCursor::pos() - p);
  show();
}

/**********************************************************************//**
  Closes given popup and sets shortcut
**************************************************************************/
void fc_shortcut_popup::closeEvent(QCloseEvent *ev)
{
  fc_sc_button *scp;

  if (sc != nullptr) {
    if (check_if_exist() == false) {
      scp = reinterpret_cast<fc_sc_button *>(parentWidget());
      scp->setText(shortcut_to_string(scp->sc));
      fc_shortcuts::sc()->set_shortcut(sc);
    }
  }

  QDialog::closeEvent(ev);
}

/**********************************************************************//**
  Checks is shortcut is already assigned and popups hud box then
  or return false if is not assigned
**************************************************************************/
bool fc_shortcut_popup::check_if_exist()
{
  fc_shortcut *fsc;
  QString desc;
  int id = 0;

  desc = "";
  if (sc != nullptr) {
    foreach (fsc, fc_shortcuts::sc()->hash) {
      if (id == 0) {
        id++;
        continue;
      }
      if (*sc == *fsc) {
        desc = fc_shortcuts::sc()->get_desc(static_cast<shortcut_id>(id + 1));
      }
      id++;
    }
    if (desc.isEmpty() == true) {
      desc = gui()->menu_bar->shortcut_exist(sc);
    }
    if (desc.isEmpty() == false) {
      fc_sc_button *fsb;
      fsb = qobject_cast<fc_sc_button*>(parentWidget());
      fsb->show_info(desc);
      return true;
    }
  }

  return false;
}

/**********************************************************************//**
  Returns mouse button name
**************************************************************************/
QString button_name(Qt::MouseButton bt)
{
  switch (bt) {
  case Qt::NoButton:
    return _("NoButton");
  case Qt::LeftButton:
    return _("LeftButton");
  case Qt::RightButton:
    return _("RightButton");
  case Qt::MiddleButton:
    return _("MiddleButton");
  case Qt::BackButton:
    return _("BackButton");
  case Qt::ForwardButton:
    return _("ForwardButton");
  case Qt::TaskButton:
    return _("TaskButton");
  case Qt::ExtraButton4:
    return _("ExtraButton4");
  case Qt::ExtraButton5:
    return _("ExtraButton5");
  case Qt::ExtraButton6:
    return _("ExtraButton6");
  case Qt::ExtraButton7:
    return _("ExtraButton7");
  case Qt::ExtraButton8:
    return _("ExtraButton8");
  case Qt::ExtraButton9:
    return _("ExtraButton9");
  case Qt::ExtraButton10:
    return _("ExtraButton10");
  case Qt::ExtraButton11:
    return _("ExtraButton11");
  case Qt::ExtraButton12:
    return _("ExtraButton12");
  case Qt::ExtraButton13:
    return _("ExtraButton13");
  case Qt::ExtraButton14:
    return _("ExtraButton14");
  case Qt::ExtraButton15:
    return _("ExtraButton15");
  case Qt::ExtraButton16:
    return _("ExtraButton16");
  case Qt::ExtraButton17:
    return _("ExtraButton17");
  case Qt::ExtraButton18:
    return _("ExtraButton18");
  case Qt::ExtraButton19:
    return _("ExtraButton19");
  case Qt::ExtraButton20:
    return _("ExtraButton20");
  case Qt::ExtraButton21:
    return _("ExtraButton21");
  case Qt::ExtraButton22:
    return _("ExtraButton22");
  case Qt::ExtraButton23:
    return _("ExtraButton23");
  case Qt::ExtraButton24:
    return _("ExtraButton24");
  default:
    return "";
  }
}

/**********************************************************************//**
  Constructor for button setting shortcut
**************************************************************************/
fc_sc_button::fc_sc_button(): QPushButton()
{
  sc = new fc_shortcut;
}

/**********************************************************************//**
  Constructor setting given shortcut
**************************************************************************/
fc_sc_button::fc_sc_button(fc_shortcut *s): QPushButton()
{
  sc_orig = s;
  sc = new fc_shortcut;
  sc->id = sc_orig->id;
  sc->key = sc_orig->key;
  sc->mouse = sc_orig->mouse;
  sc->mod = sc_orig->mod;
  sc->str = sc_orig->str;
  setText(shortcut_to_string(sc));
}

/**********************************************************************//**
  Executes slot to show information about assigned shortcut
**************************************************************************/
void fc_sc_button::show_info(QString str)
{
  err_message = str;
  popup_error();
}

/**********************************************************************//**
  Shows information about assigned shortcut
**************************************************************************/
void fc_sc_button::popup_error()
{
  hud_message_box scinfo(gui()->central_wdg);
  QList<fc_shortcut_popup *> fsb_list;
  QString title;

  /* wait until shortcut popup is destroyed */
  fsb_list = findChildren<fc_shortcut_popup *>();
  if (fsb_list.count() > 0) {
    QTimer::singleShot(20, this, SLOT(popup_error()));
    return;
  }

  /* TRANS: Given shortcut(%1) is already assigned */
  title = QString(_("%1 is already assigned to"))
                  .arg(shortcut_to_string(sc));
  scinfo.setStandardButtons(QMessageBox::Ok);
  scinfo.setDefaultButton(QMessageBox::Ok);
  scinfo.set_text_title(err_message, title);
  scinfo.exec();
}

/**********************************************************************//**
  Contructor for shortcut dialog
**************************************************************************/
fc_shortcuts_dialog::fc_shortcuts_dialog(QWidget *parent)
  : QDialog(parent)
{
  setWindowTitle(_("Shortcuts options"));
  hashcopy = hash_copy(&fc_shortcuts::hash);
  init();
}

/**********************************************************************//**
  Destructor for shortcut dialog
**************************************************************************/
fc_shortcuts_dialog::~fc_shortcuts_dialog()
{
}

/**********************************************************************//**
  Inits shortut dialog layout
**************************************************************************/
void fc_shortcuts_dialog::init()
{
  fc_shortcut *sc;
  QPushButton *but;
  QScrollArea *scroll;
  QSize size;
  QString desc;
  QWidget *widget;
  shortcut_id id;

  widget = new QWidget(this);
  scroll = new QScrollArea(this);
  scroll->setWidgetResizable(true);
  scroll_layout = new QVBoxLayout;
  main_layout = new QVBoxLayout;
  foreach (sc, fc_shortcuts::sc()->hash) {
    id = fc_shortcuts::sc()->get_id(sc);
    desc = fc_shortcuts::sc()->get_desc(id);
    add_option(sc);
  }
  widget->setProperty("doomed", true);
  widget->setLayout(scroll_layout);
  scroll->setWidget(widget);
  main_layout->addWidget(scroll);

  signal_map = new QSignalMapper;
  button_box = new QDialogButtonBox();
  but = new QPushButton(style()->standardIcon(QStyle::SP_DialogCancelButton),
                        _("Cancel"));
  button_box->addButton(but, QDialogButtonBox::ActionRole);
  connect(but, SIGNAL(clicked()), signal_map, SLOT(map()));
  signal_map->setMapping(but, RESPONSE_CANCEL);

  but = new QPushButton(style()->standardIcon(QStyle::SP_DialogResetButton),
                        _("Reset"));
  button_box->addButton(but, QDialogButtonBox::ResetRole);
  connect(but, SIGNAL(clicked()), signal_map, SLOT(map()));
  signal_map->setMapping(but, RESPONSE_RESET);

  but = new QPushButton(style()->standardIcon(QStyle::SP_DialogApplyButton),
                        _("Apply"));
  button_box->addButton(but, QDialogButtonBox::ActionRole);
  connect(but, SIGNAL(clicked()), signal_map, SLOT(map()));
  signal_map->setMapping(but, RESPONSE_APPLY);

  but = new QPushButton(style()->standardIcon(QStyle::SP_DialogSaveButton),
                        _("Save"));
  button_box->addButton(but, QDialogButtonBox::ActionRole);
  connect(but, SIGNAL(clicked()), signal_map, SLOT(map()));
  signal_map->setMapping(but, RESPONSE_SAVE);

  but = new QPushButton(style()->standardIcon(QStyle::SP_DialogOkButton),
                        _("Ok"));
  button_box->addButton(but, QDialogButtonBox::ActionRole);
  connect(but, SIGNAL(clicked()), signal_map, SLOT(map()));
  signal_map->setMapping(but, RESPONSE_OK);

  main_layout->addWidget(button_box);
  setLayout(main_layout);
  size = sizeHint();
  size.setWidth(size.width() + 10
                + style()->pixelMetric(QStyle::PM_ScrollBarExtent));
  resize(size);
  connect(signal_map, SIGNAL(mapped(int)), this, SLOT(apply_option(int)));
  setAttribute(Qt::WA_DeleteOnClose);
}

/**********************************************************************//**
  Adds shortcut option for dialog
**************************************************************************/
void fc_shortcuts_dialog::add_option(fc_shortcut *sc)
{
  QHBoxLayout *hb;
  QLabel *l;

  l = new QLabel(sc->str);
  hb = new QHBoxLayout();

  fc_sc_button *fb = new fc_sc_button(sc);
  connect(fb, &QAbstractButton::clicked, this, &fc_shortcuts_dialog::edit_shortcut);

  hb->addWidget(l, 1, Qt::AlignLeft);
  hb->addStretch();
  hb->addWidget(fb, 1, Qt::AlignRight);

  scroll_layout->addLayout(hb);
}

/**********************************************************************//**
  Slot for editing shortcut
**************************************************************************/
void fc_shortcuts_dialog::edit_shortcut()
{
  fc_sc_button *pb;
  pb = qobject_cast<fc_sc_button *>(sender());
  fc_shortcut_popup *sb = new fc_shortcut_popup(pb);
  sb->run(pb->sc);
}

/**********************************************************************//**
  Reinitializes layout
**************************************************************************/
void fc_shortcuts_dialog::refresh()
{
  QLayout *layout;
  QLayout *sublayout;
  QLayoutItem *item;
  QWidget *widget;

  layout = main_layout;
  while ((item = layout->takeAt(0))) {
    if ((sublayout = item->layout()) != 0) {
    } else if ((widget = item->widget()) != 0) {
      widget->hide();
      delete widget;
    } else {
      delete item;
    }
  }
  delete main_layout;
  init();
}

/**********************************************************************//**
  Slot for buttons on bottom of shortcut dialog
**************************************************************************/
void fc_shortcuts_dialog::apply_option(int response)
{
  switch (response) {
  case RESPONSE_APPLY:
    real_menus_init();
    gui()->menuBar()->setVisible(true);
    break;
  case RESPONSE_CANCEL:
    fc_shortcuts::hash = *hashcopy;
    fc_shortcuts::hash.detach();
    close();
    break;
  case RESPONSE_OK:
    real_menus_init();
    gui()->menuBar()->setVisible(true);
    close();
    break;
  case RESPONSE_SAVE:
    write_shortcuts();
    break;
  case RESPONSE_RESET:
    fc_shortcuts::sc()->init_default(false);
    refresh();
    break;
  }
}

/**********************************************************************//**
  Popups shortcut dialog
**************************************************************************/
void popup_shortcuts_dialog()
{
  fc_shortcuts_dialog *sh = new fc_shortcuts_dialog(gui());
  sh->show();
}

/**********************************************************************//**
  Make deep copy of shortcut map
**************************************************************************/
QMap<shortcut_id, fc_shortcut *> *hash_copy(QMap<shortcut_id, fc_shortcut *> *h)
{
  QMap<shortcut_id, fc_shortcut*> *new_hash;
  fc_shortcut *s;
  fc_shortcut *sc;
  int i;
  shortcut_id id;
  new_hash = new QMap<shortcut_id, fc_shortcut*>;

  for (i = 1 ; i < num_shortcuts + 1; i++) {
      sc = new fc_shortcut();
      id = static_cast<shortcut_id>(i);
      s = h->value(id);
      sc->id = id;
      sc->key = s->key;
      sc->mouse = s->mouse;
      sc->mod = s->mod;
      sc->str = s->str;
      new_hash->insert(sc->id, sc);
  }

  return new_hash;
}

/**********************************************************************//**
  Writes shortcuts to file
**************************************************************************/
void write_shortcuts()
{
  fc_shortcut *sc;
  QMap<shortcut_id, fc_shortcut*> h = fc_shortcuts::hash;
  QSettings s(QSettings::IniFormat, QSettings::UserScope,
              "freeciv-qt-client");
  s.beginWriteArray("Shortcuts");
  for (int i = 0; i < num_shortcuts; ++i) {
    s.setArrayIndex(i);
    sc = h.value(static_cast<shortcut_id>(i + 1));
    s.setValue("id", sc->id);
    s.setValue("key", sc->key);
    s.setValue("mouse", sc->mouse);
    s.setValue("mod", QVariant(sc->mod));
  }
  s.endArray();
}

/**********************************************************************//**
  Reads shortcuts from file. Returns false if failed.
**************************************************************************/
bool read_shortcuts()
{
  int num, i;
  fc_shortcut *sc;
  QSettings s(QSettings::IniFormat, QSettings::UserScope,
              "freeciv-qt-client");
  num = s.beginReadArray("Shortcuts");
  if (num == num_shortcuts) {
    for (i = 0; i < num_shortcuts; ++i) {
      s.setArrayIndex(i);
      sc = new fc_shortcut();
      sc->id = static_cast<shortcut_id>(s.value("id").toInt());
      sc->key = s.value("key").toInt();
      sc->mouse = static_cast<Qt::MouseButton>(s.value("mouse").toInt());
      sc->mod = static_cast<Qt::KeyboardModifiers>(s.value("mod").toInt());
      sc->str = default_shortcuts[i].str;
      fc_shortcuts::hash.insert(sc->id, sc);
    }
  } else {
    s.endArray();
    return false;
  }
  s.endArray();
  return true;
}
