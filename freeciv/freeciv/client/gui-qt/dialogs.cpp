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
#include <QComboBox>
#include <QDir>
#include <QGroupBox>
#include <QHeaderView>
#include <QImage>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QRadioButton>
#include <QRect>
#include <QSignalMapper>
#include <QTableWidgetItem>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QtMath>

// utility
#include "astring.h"

// common
#include "actions.h"
#include "city.h"
#include "game.h"
#include "government.h"
#include "improvement.h"
#include "movement.h"
#include "nation.h"
#include "research.h"

// client
#include "audio.h"
#include "chatline_common.h"
#include "client_main.h"
#include "control.h"
#include "helpdata.h"
#include "packhand.h"
#include "text.h"
#include "tilespec.h"

// gui-qt
#include "dialogs.h"
#include "fc_client.h"
#include "hudwidget.h"
#include "qtg_cxxside.h"
#include "sprite.h"

/* Locations for non action enabler controlled buttons. */
#define BUTTON_MOVE ACTION_COUNT
#define BUTTON_WAIT BUTTON_MOVE + 1
#define BUTTON_CANCEL BUTTON_MOVE + 2
#define BUTTON_COUNT BUTTON_MOVE + 3

extern void popdown_all_spaceships_dialogs();
extern void popdown_players_report();
extern void popdown_economy_report();
extern void popdown_units_report();
extern void popdown_science_report();
extern void popdown_city_report();
extern void popdown_endgame_report();

static void act_sel_keep_moving(QVariant data1, QVariant data2);
static void diplomat_incite(QVariant data1, QVariant data2);
static void diplomat_incite_escape(QVariant data1, QVariant data2);
static void spy_request_sabotage_list(QVariant data1, QVariant data2);
static void spy_request_sabotage_esc_list(QVariant data1, QVariant data2);
static void spy_sabotage(QVariant data1, QVariant data2);
static void spy_steal(QVariant data1, QVariant data2);
static void spy_steal_esc(QVariant data1, QVariant data2);
static void spy_steal_something(QVariant data1, QVariant data2);
static void diplomat_steal(QVariant data1, QVariant data2);
static void diplomat_steal_esc(QVariant data1, QVariant data2);
static void spy_poison(QVariant data1, QVariant data2);
static void spy_poison_esc(QVariant data1, QVariant data2);
static void spy_steal_gold(QVariant data1, QVariant data2);
static void spy_steal_gold_esc(QVariant data1, QVariant data2);
static void spy_steal_maps(QVariant data1, QVariant data2);
static void spy_steal_maps_esc(QVariant data1, QVariant data2);
static void spy_nuke_city(QVariant data1, QVariant data2);
static void spy_nuke_city_esc(QVariant data1, QVariant data2);
static void destroy_city(QVariant data1, QVariant data2);
static void diplomat_embassy(QVariant data1, QVariant data2);
static void spy_embassy(QVariant data1, QVariant data2);
static void spy_sabotage_unit(QVariant data1, QVariant data2);
static void spy_sabotage_unit_esc(QVariant data1, QVariant data2);
static void spy_investigate(QVariant data1, QVariant data2);
static void diplomat_investigate(QVariant data1, QVariant data2);
static void diplomat_sabotage(QVariant data1, QVariant data2);
static void diplomat_sabotage_esc(QVariant data1, QVariant data2);
static void diplomat_bribe(QVariant data1, QVariant data2);
static void caravan_marketplace(QVariant data1, QVariant data2);
static void caravan_establish_trade(QVariant data1, QVariant data2);
static void caravan_help_build(QVariant data1, QVariant data2);
static void unit_recycle(QVariant data1, QVariant data2);
static void capture_units(QVariant data1, QVariant data2);
static void expel_unit(QVariant data1, QVariant data2);
static void bombard(QVariant data1, QVariant data2);
static void found_city(QVariant data1, QVariant data2);
static void transform_terrain(QVariant data1, QVariant data2);
static void irrigate_tf(QVariant data1, QVariant data2);
static void mine_tf(QVariant data1, QVariant data2);
static void pillage(QVariant data1, QVariant data2);
static void road(QVariant data1, QVariant data2);
static void base(QVariant data1, QVariant data2);
static void mine(QVariant data1, QVariant data2);
static void irrigate(QVariant data1, QVariant data2);
static void nuke(QVariant data1, QVariant data2);
static void attack(QVariant data1, QVariant data2);
static void suicide_attack(QVariant data1, QVariant data2);
static void paradrop(QVariant data1, QVariant data2);
static void convert_unit(QVariant data1, QVariant data2);
static void fortify(QVariant data1, QVariant data2);
static void disband_unit(QVariant data1, QVariant data2);
static void join_city(QVariant data1, QVariant data2);
static void unit_home_city(QVariant data1, QVariant data2);
static void unit_upgrade(QVariant data1, QVariant data2);
static void airlift(QVariant data1, QVariant data2);
static void conquer_city(QVariant data1, QVariant data2);
static void heal_unit(QVariant data1, QVariant data2);
static void keep_moving(QVariant data1, QVariant data2);
static void pillage_something(QVariant data1, QVariant data2);
static void action_entry(choice_dialog *cd,
                         action_id act,
                         const struct act_prob *act_probs,
                         QString custom,
                         QVariant data1, QVariant data2);


static bool is_showing_pillage_dialog = false;
static races_dialog* race_dialog;
static bool is_race_dialog_open = false;

/* Information used in action selection follow up questions. Can't be
 * stored in the action selection dialog since it is closed before the
 * follow up question is asked. */
static bool is_more_user_input_needed = FALSE;

/* Don't remove a unit's action decision want or move on to the next actor
 unit that wants a decision in the current unit selection. */
static bool did_not_decide = false;

extern char forced_tileset_name[512];
qdef_act* qdef_act::m_instance = 0;

/***********************************************************************//**
  Initialize a mapping between an action and the function to call if
  the action's button is pushed.
***************************************************************************/
static const QHash<action_id, pfcn_void> af_map_init(void)
{
  QHash<action_id, pfcn_void> action_function;

  /* Unit acting against a city target. */
  action_function[ACTION_ESTABLISH_EMBASSY] = spy_embassy;
  action_function[ACTION_ESTABLISH_EMBASSY_STAY] = diplomat_embassy;
  action_function[ACTION_SPY_INVESTIGATE_CITY] = spy_investigate;
  action_function[ACTION_INV_CITY_SPEND] = diplomat_investigate;
  action_function[ACTION_SPY_POISON] = spy_poison;
  action_function[ACTION_SPY_POISON_ESC] = spy_poison_esc;
  action_function[ACTION_SPY_STEAL_GOLD] = spy_steal_gold;
  action_function[ACTION_SPY_STEAL_GOLD_ESC] = spy_steal_gold_esc;
  action_function[ACTION_SPY_SABOTAGE_CITY] = diplomat_sabotage;
  action_function[ACTION_SPY_SABOTAGE_CITY_ESC] = diplomat_sabotage_esc;
  action_function[ACTION_SPY_TARGETED_SABOTAGE_CITY] =
      spy_request_sabotage_list;
  action_function[ACTION_SPY_TARGETED_SABOTAGE_CITY_ESC] =
      spy_request_sabotage_esc_list;
  action_function[ACTION_SPY_STEAL_TECH] = diplomat_steal;
  action_function[ACTION_SPY_STEAL_TECH_ESC] = diplomat_steal_esc;
  action_function[ACTION_SPY_TARGETED_STEAL_TECH] = spy_steal;
  action_function[ACTION_SPY_TARGETED_STEAL_TECH_ESC] = spy_steal_esc;
  action_function[ACTION_SPY_INCITE_CITY] = diplomat_incite;
  action_function[ACTION_SPY_INCITE_CITY_ESC] = diplomat_incite_escape;
  action_function[ACTION_TRADE_ROUTE] = caravan_establish_trade;
  action_function[ACTION_MARKETPLACE] = caravan_marketplace;
  action_function[ACTION_HELP_WONDER] = caravan_help_build;
  action_function[ACTION_JOIN_CITY] = join_city;
  action_function[ACTION_STEAL_MAPS] = spy_steal_maps;
  action_function[ACTION_STEAL_MAPS_ESC] = spy_steal_maps_esc;
  action_function[ACTION_SPY_NUKE] = spy_nuke_city;
  action_function[ACTION_SPY_NUKE_ESC] = spy_nuke_city_esc;
  action_function[ACTION_DESTROY_CITY] = destroy_city;
  action_function[ACTION_RECYCLE_UNIT] = unit_recycle;
  action_function[ACTION_HOME_CITY] = unit_home_city;
  action_function[ACTION_UPGRADE_UNIT] = unit_upgrade;
  action_function[ACTION_AIRLIFT] = airlift;
  action_function[ACTION_CONQUER_CITY] = conquer_city;

  /* Unit acting against a unit target. */
  action_function[ACTION_SPY_BRIBE_UNIT] = diplomat_bribe;
  action_function[ACTION_SPY_SABOTAGE_UNIT] = spy_sabotage_unit;
  action_function[ACTION_SPY_SABOTAGE_UNIT_ESC] = spy_sabotage_unit_esc;
  action_function[ACTION_EXPEL_UNIT] = expel_unit;
  action_function[ACTION_HEAL_UNIT] = heal_unit;

  /* Unit acting against all units at a tile. */
  action_function[ACTION_CAPTURE_UNITS] = capture_units;
  action_function[ACTION_BOMBARD] = bombard;

  /* Unit acting against a tile. */
  action_function[ACTION_FOUND_CITY] = found_city;
  action_function[ACTION_NUKE] = nuke;
  action_function[ACTION_PARADROP] = paradrop;
  action_function[ACTION_ATTACK] = attack;
  action_function[ACTION_SUICIDE_ATTACK] = suicide_attack;
  action_function[ACTION_TRANSFORM_TERRAIN] = transform_terrain;
  action_function[ACTION_IRRIGATE_TF] = irrigate_tf;
  action_function[ACTION_MINE_TF] = mine_tf;
  action_function[ACTION_PILLAGE] = pillage;
  action_function[ACTION_ROAD] = road;
  action_function[ACTION_BASE] = base;
  action_function[ACTION_MINE] = mine;
  action_function[ACTION_IRRIGATE] = irrigate;

  /* Unit acting with no target except itself. */
  action_function[ACTION_DISBAND_UNIT] = disband_unit;
  action_function[ACTION_FORTIFY] = fortify;
  action_function[ACTION_CONVERT] = convert_unit;

  return action_function;
}

/* Mapping from an action to the function to call when its button is
 * pushed. */
static const QHash<action_id, pfcn_void> af_map = af_map_init();

/***********************************************************************//**
  Constructor for custom dialog with themed titlebar
***************************************************************************/
qfc_dialog::qfc_dialog(QWidget* parent) : QDialog(parent,
                                                  Qt::FramelessWindowHint)
{
  titlebar_height = 0;
  moving_now = false;
  setSizeGripEnabled(true);
  close_pix = *fc_icons::instance()->get_pixmap("cclose");
}

/***********************************************************************//**
  Paint event for themed dialog
***************************************************************************/
void qfc_dialog::paintEvent(QPaintEvent *event)
{
  Q_UNUSED(event)

  QPainter p(this);
  QStyleOptionTitleBar tbar_opt;
  QStyleOption win_opt;
  QStyle *style = this->style();
  QRect active_area = this->rect();
  QPalette qpal;
  QRect close_rect, text_rect;

  qpal.setColor(QPalette::Active, QPalette::ToolTipText, Qt::white);
  tbar_opt.initFrom(this);
  titlebar_height = style->pixelMetric(QStyle::PM_TitleBarHeight,
                                       &tbar_opt, this) + 2;
  close_pix = close_pix.scaledToHeight(titlebar_height);
  tbar_opt.rect = QRect(0, 0, this->width(), titlebar_height);
  text_rect = QRect(0, 0, this->width() - close_pix.width() , titlebar_height);
  close_rect = QRect(this->width() - close_pix.width(), 0, this->width(),
                     titlebar_height);
  tbar_opt.titleBarState = this->windowState();
  tbar_opt.text = tbar_opt.fontMetrics.elidedText(this->windowTitle(),
                  Qt::ElideRight,
                  text_rect.width());
  style->drawComplexControl(QStyle::CC_TitleBar, &tbar_opt, &p, this);
  style->drawItemText(&p, text_rect, Qt::AlignCenter, qpal,
                      true, tbar_opt.text, QPalette::ToolTipText);
  style->drawItemPixmap(&p, close_rect, Qt::AlignLeft,
                        close_pix.scaledToHeight(titlebar_height));

  active_area.setTopLeft(QPoint(0, titlebar_height));
  this->setContentsMargins(0, titlebar_height, 0, 0);
  win_opt.initFrom(this);
  win_opt.rect = active_area;
  style->drawPrimitive(QStyle::PE_Widget, &win_opt, &p, this);
}

/***********************************************************************//**
  Mouse move event for themed titlebar (moves dialog with left mouse)
***************************************************************************/
void qfc_dialog::mouseMoveEvent(QMouseEvent *event)
{
  if (moving_now == true) {
    move(event->globalPos() - point);
  }
}

/***********************************************************************//**
  Mouse press event - catches left click
***************************************************************************/
void qfc_dialog::mousePressEvent(QMouseEvent *event)
{
  if (event->pos().y() <= titlebar_height
      && event->pos().x() <= width() - close_pix.width()) {
    point = event->globalPos() - geometry().topLeft();
    moving_now = true;
    setCursor(Qt::SizeAllCursor);
  } else if (event->pos().y() <= titlebar_height
      && event->pos().x() > width() - close_pix.width()) {
    close();
  }
}

/***********************************************************************//**
  Mouse release event for themed dialog
***************************************************************************/
void qfc_dialog::mouseReleaseEvent(QMouseEvent *event)
{
  moving_now = false;
  setCursor(Qt::ArrowCursor);
}

/***********************************************************************//**
 Constructor for selecting nations
***************************************************************************/
races_dialog::races_dialog(struct player *pplayer,
                           QWidget *parent) : qfc_dialog(parent)
{
  int i;
  QGridLayout *qgroupbox_layout;
  QGroupBox *no_name;
  QTableWidgetItem *item;
  QPixmap *pix;
  QHeaderView *header;
  QSize size;
  QString title;
  QLabel *ns_label;

  setAttribute(Qt::WA_DeleteOnClose);
  is_race_dialog_open = true;
  main_layout = new QGridLayout;
  selected_nation_tabs = new QTableWidget;
  nation_tabs = new QTableWidget();
  styles = new QTableWidget;
  ok_button = new QPushButton;
  qnation_set =  new QComboBox;
  ns_label = new QLabel;
  tplayer = pplayer;

  selected_nation = -1;
  selected_style = -1;
  selected_sex = -1;
  setWindowTitle(_("Select Nation"));
  selected_nation_tabs->setRowCount(0);
  selected_nation_tabs->setColumnCount(1);
  selected_nation_tabs->setSelectionMode(QAbstractItemView::SingleSelection);
  selected_nation_tabs->verticalHeader()->setVisible(false);
  selected_nation_tabs->horizontalHeader()->setVisible(false);
  selected_nation_tabs->setProperty("showGrid", "true");
  selected_nation_tabs->setEditTriggers(QAbstractItemView::NoEditTriggers);
  selected_nation_tabs->setShowGrid(false);
  selected_nation_tabs->setAlternatingRowColors(true);
  
  nation_tabs->setRowCount(0);
  nation_tabs->setColumnCount(1);
  nation_tabs->setSelectionMode(QAbstractItemView::SingleSelection);
  nation_tabs->verticalHeader()->setVisible(false);
  nation_tabs->horizontalHeader()->setVisible(false);
  nation_tabs->setProperty("showGrid", "true");
  nation_tabs->setEditTriggers(QAbstractItemView::NoEditTriggers);
  nation_tabs->setShowGrid(false);
  ns_label->setText(_("Nation Set:"));
  styles->setRowCount(0);
  styles->setColumnCount(2);
  styles->setSelectionMode(QAbstractItemView::SingleSelection);
  styles->verticalHeader()->setVisible(false);
  styles->horizontalHeader()->setVisible(false);
  styles->setProperty("showGrid", "false");
  styles->setProperty("selectionBehavior", "SelectRows");
  styles->setEditTriggers(QAbstractItemView::NoEditTriggers);
  styles->setShowGrid(false);

  qgroupbox_layout = new QGridLayout;
  no_name = new QGroupBox(parent);
  leader_name = new QComboBox(no_name);
  is_male = new QRadioButton(no_name);
  is_female = new QRadioButton(no_name);

  leader_name->setEditable(true);
  qgroupbox_layout->addWidget(leader_name, 1, 0, 1, 2);
  qgroupbox_layout->addWidget(is_male, 2, 1);
  qgroupbox_layout->addWidget(is_female, 2, 0);
  is_female->setText(_("Female"));
  is_male->setText(_("Male"));
  no_name->setLayout(qgroupbox_layout);

  description = new QTextEdit;
  description->setReadOnly(true);
  description->setText(_("Choose nation"));
  no_name->setTitle(_("Your leader name"));

  /**
   * Fill styles, no need to update them later
   */

  styles_iterate(pstyle) {
    i = basic_city_style_for_style(pstyle);

    if (i >= 0) {
      item = new QTableWidgetItem;
      styles->insertRow(i);
      pix = get_sample_city_sprite(tileset, i)->pm;
      item->setData(Qt::DecorationRole, *pix);
      item->setData(Qt::UserRole, style_number(pstyle));
      size.setWidth(pix->width());
      size.setHeight(pix->height());
      item->setSizeHint(size);
      styles->setItem(i, 0, item);
      item = new QTableWidgetItem;
      item->setText(style_name_translation(pstyle));
      styles->setItem(i, 1, item);
    }
  } styles_iterate_end;

  header = styles->horizontalHeader();
  header->setSectionResizeMode(QHeaderView::Stretch);
  header->resizeSections(QHeaderView::ResizeToContents);
  header = styles->verticalHeader();
  header->resizeSections(QHeaderView::ResizeToContents);
  nation_sets_iterate(pset) {
    qnation_set->addItem(nation_set_name_translation(pset),
                         nation_set_rule_name(pset));
  } nation_sets_iterate_end;
  /* create nation sets */
  refresh();

  connect(styles->selectionModel(),
          SIGNAL(selectionChanged(const QItemSelection &,
                                  const QItemSelection &)),
          SLOT(style_selected(const QItemSelection &,
                              const QItemSelection &)));
  connect(selected_nation_tabs->selectionModel(),
          SIGNAL(selectionChanged(const QItemSelection &,
                                  const QItemSelection &)),
          SLOT(nation_selected(const QItemSelection &,
                              const QItemSelection &)));
  connect(leader_name, SIGNAL(currentIndexChanged(int)),
          SLOT(leader_selected(int)));
  connect(leader_name->lineEdit(), &QLineEdit::returnPressed,
          this, &races_dialog::ok_pressed);
  connect(qnation_set, SIGNAL(currentIndexChanged(int)),
          SLOT(nationset_changed(int)));
  connect(nation_tabs->selectionModel(),
          SIGNAL(selectionChanged(const QItemSelection &,
                                  const QItemSelection &)),
          SLOT(group_selected(const QItemSelection &,
                              const QItemSelection &)));

  ok_button = new QPushButton;
  ok_button->setText(_("Cancel"));
  connect(ok_button, &QAbstractButton::pressed, this, &races_dialog::cancel_pressed);
  main_layout->addWidget(ok_button, 8, 2, 1, 1);
  random_button = new QPushButton;
  random_button->setText(_("Random"));
  connect(random_button, &QAbstractButton::pressed, this, &races_dialog::random_pressed);
  main_layout->addWidget(random_button, 8, 0, 1, 1);
  ok_button = new QPushButton;
  ok_button->setText(_("Ok"));
  connect(ok_button, &QAbstractButton::pressed, this, &races_dialog::ok_pressed);
  main_layout->addWidget(ok_button, 8, 3, 1, 1);
  main_layout->addWidget(no_name, 0, 3, 2, 1);
  if (nation_set_count() > 1) {
    main_layout->addWidget(ns_label, 0, 0, 1, 1);
    main_layout->addWidget(qnation_set, 0, 1, 1, 1);
    main_layout->addWidget(nation_tabs, 1, 0, 5, 2);
  } else {
    main_layout->addWidget(nation_tabs, 0, 0, 6, 2);
  }
  main_layout->addWidget(styles, 2, 3, 4, 1);
  main_layout->addWidget(description, 6, 0, 2, 4);
  main_layout->addWidget(selected_nation_tabs, 0, 2, 6, 1);

  setLayout(main_layout);
  set_index(-99);

  if (C_S_RUNNING == client_state()) {
    title = _("Edit Nation");
  } else if (NULL != pplayer && pplayer == client.conn.playing) {
    title = _("What Nation Will You Be?");
  } else {
    title = _("Pick Nation");
  }

  update_nationset_combo();
  setWindowTitle(title);
}

/***********************************************************************//**
  Destructor for races dialog
***************************************************************************/
races_dialog::~races_dialog()
{
  ::is_race_dialog_open = false;
}

/***********************************************************************//**
  Sets first index to call update of nation list
***************************************************************************/
void races_dialog::refresh()
{
  struct nation_group *group;
  QTableWidgetItem *item;
  QHeaderView *header;
  int i;
  int count;

  nation_tabs->clearContents();
  nation_tabs->setRowCount(0);
  nation_tabs->insertRow(0);
  item =  new QTableWidgetItem;
  item->setText(_("All nations"));
  item->setData(Qt::UserRole, -99);
  nation_tabs->setItem(0, 0, item);

  for (i = 1; i < nation_group_count() + 1; i++) {
    group = nation_group_by_number(i - 1);
    if (is_nation_group_hidden(group)) {
      continue;
    }
    count = 0;
    /* checking if group is empty */
    nations_iterate(pnation) {
      if (!is_nation_playable(pnation)
          || !is_nation_pickable(pnation)
          || !nation_is_in_group(pnation, group)) {
        continue;
      }
      count ++;
    } nations_iterate_end;
    if (count == 0) {
      continue;
    }
    nation_tabs->insertRow(i);
    item =  new QTableWidgetItem;
    item->setData(Qt::UserRole, i - 1);
    item->setText(nation_group_name_translation(group));
    nation_tabs->setItem(i, 0, item);
  }
  header = nation_tabs->horizontalHeader();
  header->resizeSections(QHeaderView::Stretch);
  header = nation_tabs->verticalHeader();
  header->resizeSections(QHeaderView::ResizeToContents);
  set_index(-99);
}

/***********************************************************************//**
  Updates nation_set combo ( usually called from option change )
***************************************************************************/
void races_dialog::update_nationset_combo()
{
  struct option *popt;
  struct nation_set *s;

  popt = optset_option_by_name(server_optset, "nationset");
  if (popt) {
    s = nation_set_by_setting_value(option_str_get(popt));
    qnation_set->setCurrentIndex(nation_set_index(s));
    qnation_set->setToolTip(nation_set_description(s));
  }
}

/***********************************************************************//**
  Selected group of nation
***************************************************************************/
void races_dialog::group_selected(const QItemSelection &sl,
                                  const QItemSelection &ds)
{
  QModelIndexList indexes = sl.indexes();
  QModelIndex index ;

  if (indexes.isEmpty()) {
    return;
  }
  index = indexes.at(0);
  set_index(index.row());
}


/***********************************************************************//**
  Sets new nations' group by current current selection,
  index is used only when there is no current selection.
***************************************************************************/
void races_dialog::set_index(int index)
{
  QTableWidgetItem *item;
  QPixmap *pix;
  QFont f;
  struct nation_group *group;
  int i;
  struct sprite *s;
  QHeaderView *header;
  selected_nation_tabs->clearContents();
  selected_nation_tabs->setRowCount(0);

  last_index = 0;
  i = nation_tabs->currentRow();
  if (i != -1) {
    item = nation_tabs->item(i, 0);
    index = item->data(Qt::UserRole).toInt();
  }

  group = nation_group_by_number(index);
  i = 0;
  nations_iterate(pnation) {
    if (!is_nation_playable(pnation)
        || !is_nation_pickable(pnation)) {
      continue;
    }
    if (!nation_is_in_group(pnation, group) && index != -99) {
      continue;
    }
    item = new QTableWidgetItem;
    selected_nation_tabs->insertRow(i);
    s = get_nation_flag_sprite(tileset, pnation);
    if (pnation->player) {
      f = item->font();
      f.setStrikeOut(true);
      item->setFont(f);
    }
    pix = s->pm;
    item->setData(Qt::DecorationRole, *pix);
    item->setData(Qt::UserRole, nation_number(pnation));
    item->setText(nation_adjective_translation(pnation));
    selected_nation_tabs->setItem(i, 0, item);
  } nations_iterate_end;

  selected_nation_tabs->sortByColumn(0, Qt::AscendingOrder);
  header = selected_nation_tabs->horizontalHeader();
  header->resizeSections(QHeaderView::Stretch);
  header = selected_nation_tabs->verticalHeader();
  header->resizeSections(QHeaderView::ResizeToContents);
}

/***********************************************************************//**
  Sets selected nation and updates style and leaders selector
***************************************************************************/
void races_dialog::nation_selected(const QItemSelection &selected,
                                   const QItemSelection &deselcted)
{
  char buf[4096];
  QModelIndex index ;
  QVariant qvar;
  QModelIndexList indexes = selected.indexes();
  QString str;
  QTableWidgetItem *item;
  int style, ind;

  if (indexes.isEmpty()) {
    return;
  }

  index = indexes.at(0);
  if (indexes.isEmpty()){
    return;
  }
  qvar = index.data(Qt::UserRole);
  selected_nation = qvar.toInt();

  helptext_nation(buf, sizeof(buf), nation_by_number(selected_nation), NULL);
  description->setText(buf);
  leader_name->clear();
  if (client.conn.playing == tplayer) {
    leader_name->addItem(client.conn.playing->name, true);
  }
  nation_leader_list_iterate(nation_leaders(nation_by_number
                                            (selected_nation)), pleader) {
    str = QString::fromUtf8(nation_leader_name(pleader));
    leader_name->addItem(str, nation_leader_is_male(pleader));
  } nation_leader_list_iterate_end;

  /**
   * select style for nation
   */

  style = style_number(style_of_nation(nation_by_number(selected_nation)));
  qvar = qvar.fromValue<int>(style);

  for (ind = 0; ind < styles->rowCount(); ind++) {
    item = styles->item(ind, 0);

    if (item->data(Qt::UserRole) == qvar) {
      styles->selectRow(ind);
    }
  }
}

/***********************************************************************//**
  Sets selected style
***************************************************************************/
void races_dialog::style_selected(const QItemSelection &selected,
                                   const QItemSelection &deselcted)
{
  QModelIndex index ;
  QVariant qvar;
  QModelIndexList indexes = selected.indexes();

  if (indexes.isEmpty()) {
    return;
  }

  index = indexes.at(0);
  qvar = index.data(Qt::UserRole);
  selected_style = qvar.toInt();
}

/***********************************************************************//**
  Sets selected leader
***************************************************************************/
void races_dialog::leader_selected(int index)
{
  if (leader_name->itemData(index).toBool()) {
    is_male->setChecked(true);
    is_female->setChecked(false);
    selected_sex=0;
  } else {
    is_male->setChecked(false);
    is_female->setChecked(true);
    selected_sex=1;
  }
}

/***********************************************************************//**
  Button accepting all selection has been pressed, closes dialog if
  everything is ok
***************************************************************************/
void races_dialog::ok_pressed()
{

  if (selected_nation == -1) {
    return;
  }

  if (selected_sex == -1) {
    output_window_append(ftc_client, _("You must select your sex."));
    return;
  }

  if (selected_style == -1) {
    output_window_append(ftc_client, _("You must select your style."));
    return;
  }

  if (leader_name->currentText().length() == 0) {
    output_window_append(ftc_client, _("You must type a legal name."));
    return;
  }

  if (nation_by_number(selected_nation)->player != NULL) {
    output_window_append(ftc_client,
                         _("Nation has been chosen by other player"));
    return;
  }
  dsend_packet_nation_select_req(&client.conn, player_number(tplayer),
                                 selected_nation, selected_sex,
                                 leader_name->currentText().toUtf8().data(),
                                 selected_style);
  close();
  deleteLater();
}

/***********************************************************************//**
  Constructor for notify dialog
***************************************************************************/
notify_dialog::notify_dialog(const char *caption, const char *headline,
                             const char *lines, QWidget *parent)
                             : fcwidget()
{
  int x, y;
  QString qlines;

  setAttribute(Qt::WA_DeleteOnClose);
  setCursor(Qt::ArrowCursor);
  setParent(parent);
  setFrameStyle(QFrame::Box);
  cw = new close_widget(this);
  cw->put_to_corner();

  qcaption = QString(caption);
  qheadline = QString(headline);
  qlines = QString(lines);
  qlist = qlines.split("\n");
  small_font = *fc_font::instance()->get_font("gui_qt_font_notify_label");
  x = 0;
  y = 0;
  calc_size(x, y);
  resize(x, y);
  gui()->mapview_wdg->find_place(gui()->mapview_wdg->width() - x - 4, 4,
                                 x, y, x, y, 0);
  move(x, y);
  was_destroyed = false;

}

/***********************************************************************//**
  Starts new copy of notify dialog and closes current one
***************************************************************************/
void notify_dialog::restart()
{
  QString s, q;
  int i;

  for (i = 0; i < qlist.size(); ++i) {
    s = qlist.at(i);
    q = q + s;
    if (i < qlist.size() - 1) {
      q = q + QChar('\n');
    }
  }
  popup_notify_dialog(qcaption.toLocal8Bit().data(),
                      qheadline.toLocal8Bit().data(),
                      q.toLocal8Bit().data());
  close();
  destroy();
}

/***********************************************************************//**
  Calculates size of notify dialog
***************************************************************************/
void notify_dialog::calc_size(int &x, int &y)
{
  QFontMetrics fm(small_font);
  int i;
  QStringList str_list;

  str_list = qlist;
  str_list << qcaption << qheadline;

  for (i = 0; i < str_list.count(); i++) {
    x = qMax(x, fm.width(str_list.at(i)));
    y = y + 3 + fm.height();
  }
  x = x + 15;
}

/***********************************************************************//**
  Paint Event for notify dialog
***************************************************************************/
void notify_dialog::paintEvent(QPaintEvent * paint_event)
{
  QPainter painter(this);
  QPainterPath path;
  QPen pen;
  QFontMetrics fm(small_font);
  int i;

  pen.setWidth(1);
  pen.setColor(palette().color(QPalette::Text));
  painter.setFont(small_font);
  painter.setPen(pen);
  painter.drawText(10, fm.height() + 3, qcaption);
  painter.drawText(10, 2 * fm.height() + 6, qheadline);
  for (i = 0; i < qlist.count(); i++) {
    painter.drawText(10, 3 + (fm.height() + 3) * (i + 3), qlist[i]);
  }
  cw->put_to_corner();
}

/***********************************************************************//**
  Called when mouse button was pressed, just to close on right click
***************************************************************************/
void notify_dialog::mousePressEvent(QMouseEvent *event)
{
  cursor = event->globalPos() - geometry().topLeft();
  if (event->button() == Qt::RightButton){
    was_destroyed = true;
    close();
  }
}

/***********************************************************************//**
  Called when mouse button was pressed and moving around
***************************************************************************/
void notify_dialog::mouseMoveEvent(QMouseEvent *event)
{
  move(event->globalPos() - cursor);
  setCursor(Qt::SizeAllCursor);
}

/***********************************************************************//**
  Called when mouse button unpressed. Restores cursor.
***************************************************************************/
void notify_dialog::mouseReleaseEvent(QMouseEvent *event)
{
  setCursor(Qt::ArrowCursor);
}

/***********************************************************************//**
  Called when close button was pressed
***************************************************************************/
void notify_dialog::update_menu()
{
  was_destroyed = true;
  destroy();
}

/***********************************************************************//**
  Destructor for notify dialog, notice that somehow object is not destroyed
  immediately, so it can be still visible for parent, check boolean
  was_destroyed if u suspect it could not be destroyed yet
***************************************************************************/
notify_dialog::~notify_dialog()
{
  was_destroyed = true;
  destroy();
}

/***********************************************************************//**
  Default actions provider constructor
***************************************************************************/
qdef_act::qdef_act()
{
  vs_city = -1;
  vs_unit = -1;
}

/***********************************************************************//**
  Returns instance of qdef_act
***************************************************************************/
qdef_act *qdef_act::action()
{
  if (!m_instance) {
    m_instance = new qdef_act;
  }
  return m_instance;
}

/***********************************************************************//**
  Deletes qdef_act instance
***************************************************************************/
void qdef_act::drop()
{
  if (m_instance) {
    delete m_instance;
    m_instance = 0;
  }
}

/***********************************************************************//**
  Sets default action vs city
***************************************************************************/
void qdef_act::vs_city_set(int i)
{
  vs_city = i;
}

/***********************************************************************//**
  Sets default action vs unit
***************************************************************************/
void qdef_act::vs_unit_set(int i)
{
  vs_unit = i;
}

/***********************************************************************//**
  Returns default action vs city
***************************************************************************/
action_id qdef_act::vs_city_get()
{
  return vs_city;
}

/***********************************************************************//**
  Returns default action vs unit
***************************************************************************/
action_id qdef_act::vs_unit_get()
{
  return vs_unit;
}

/***********************************************************************//**
  Button canceling all selections has been pressed.
***************************************************************************/
void races_dialog::cancel_pressed()
{
  delete this;
}

/***********************************************************************//**
  Sets random nation
***************************************************************************/
void races_dialog::random_pressed()
{
  dsend_packet_nation_select_req(&client.conn,player_number(tplayer),-1,
                                 false, "", 0);
  delete this;
}

/***********************************************************************//**
  Notify goto dialog constructor
***************************************************************************/
notify_goto::notify_goto(const char *headline, const char *lines,
                         const struct text_tag_list *tags, tile *ptile,
                         QWidget *parent): QMessageBox(parent)
{
  QString qlines;
  setAttribute(Qt::WA_DeleteOnClose);
  goto_but = this->addButton(_("Goto Location"), QMessageBox::ActionRole);
  goto_but->setIcon(fc_icons::instance()->get_icon("go-up"));
  inspect_but = this->addButton(_("Inspect City"), QMessageBox::ActionRole);
  inspect_but->setIcon(fc_icons::instance()->get_icon("plus"));

  close_but = this->addButton(QMessageBox::Close);
  gtile = ptile;
  if (!gtile) {
    goto_but->setVisible(false);
    inspect_but->setVisible(false);
  } else {
    struct city *pcity = tile_city(gtile);
    inspect_but->setVisible(NULL != pcity
                            && city_owner(pcity) == client.conn.playing);
  }
  setWindowTitle(headline);
  qlines = lines;
  qlines.replace("\n", " ");
  setText(qlines);
  connect(goto_but, &QAbstractButton::pressed, this, &notify_goto::goto_tile);
  connect(inspect_but, &QAbstractButton::pressed, this, &notify_goto::inspect_city);
  connect(close_but, &QAbstractButton::pressed, this, &QWidget::close);
  show();
}

/***********************************************************************//**
  Clicked goto tile in notify goto dialog
***************************************************************************/
void notify_goto::goto_tile()
{
  center_tile_mapcanvas(gtile);
  close();
}

/***********************************************************************//**
  Clicked inspect city in notify goto dialog
***************************************************************************/
void notify_goto::inspect_city()
{
  struct city *pcity = tile_city(gtile);
  if (pcity) {
    qtg_real_city_dialog_popup(pcity);
  }
  close();
}

/***********************************************************************//**
  User changed nation_set
***************************************************************************/
void races_dialog::nationset_changed(int index)
{
  QString rule_name;
  char *rn;
  struct option *poption = optset_option_by_name(server_optset, "nationset");
  rule_name = qnation_set->currentData().toString();
  rn = rule_name.toLocal8Bit().data();
  if (nation_set_by_setting_value(option_str_get(poption))
      != nation_set_by_rule_name(rn)) {
    option_str_set(poption, rn);
  }
}

/***********************************************************************//**
  Popup a dialog to display information about an event that has a
  specific location.  The user should be given the option to goto that
  location.
***************************************************************************/
void popup_notify_goto_dialog(const char *headline, const char *lines,
                              const struct text_tag_list *tags,
                              struct tile *ptile)
{
  notify_goto *ask = new notify_goto(headline, lines, tags, ptile,
                                     gui()->central_wdg);
  ask->show();
}

/***********************************************************************//**
  Popup a dialog to display connection message from server.
***************************************************************************/
void popup_connect_msg(const char *headline, const char *message)
{
  QMessageBox msg(gui()->central_wdg);

  msg.setText(message);
  msg.setStandardButtons(QMessageBox::Ok);
  msg.setWindowTitle(headline);
  msg.exec();

}

/***********************************************************************//**
  Popup a generic dialog to display some generic information.
***************************************************************************/
void popup_notify_dialog(const char *caption, const char *headline,
                         const char *lines)
{
  notify_dialog *nd = new notify_dialog(caption, headline, lines,
                                       gui()->mapview_wdg);
  nd->show();
}

/***********************************************************************//**
  Popup the nation selection dialog.
***************************************************************************/
void popup_races_dialog(struct player *pplayer)
{
  if (!is_race_dialog_open) {
    race_dialog = new races_dialog(pplayer, gui()->central_wdg);
    is_race_dialog_open = true;
    race_dialog->show();
  }
  race_dialog->showNormal();
}

/***********************************************************************//**
  Close the nation selection dialog.  This should allow the user to
  (at least) select a unit to activate.
***************************************************************************/
void popdown_races_dialog(void)
{
  if (is_race_dialog_open){
    race_dialog->close();
    is_race_dialog_open = false;
  }
}

/***********************************************************************//**
  Popup a dialog window to select units on a particular tile.
***************************************************************************/
void unit_select_dialog_popup(struct tile *ptile)
{
  if (ptile != NULL
      && (unit_list_size(ptile->units) > 1
          || (unit_list_size(ptile->units) == 1 && tile_city(ptile)))) {
    gui()->toggle_unit_sel_widget(ptile);
  }
}

/***********************************************************************//**
  Update the dialog window to select units on a particular tile.
***************************************************************************/
void unit_select_dialog_update_real(void *unused)
{
  gui()->update_unit_sel();
}

/***********************************************************************//**
  Updates nationset combobox
***************************************************************************/
void update_nationset_combo()
{
  if (is_race_dialog_open) {
    race_dialog->update_nationset_combo();
  }
}

/***********************************************************************//**
  The server has changed the set of selectable nations.
***************************************************************************/
void races_update_pickable(bool nationset_change)
{
  if (is_race_dialog_open){
    race_dialog->refresh();
  }
}

/***********************************************************************//**
  In the nation selection dialog, make already-taken nations unavailable.
  This information is contained in the packet_nations_used packet.
***************************************************************************/
void races_toggles_set_sensitive(void)
{
  if (is_race_dialog_open) {
    race_dialog->refresh();
  }
}

/***********************************************************************//**
  Popup a dialog asking if the player wants to start a revolution.
***************************************************************************/
void popup_revolution_dialog(struct government *government)
{
  hud_message_box ask(gui()->central_wdg);
  int ret;

  if (0 > client.conn.playing->revolution_finishes) {
    ask.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
    ask.setDefaultButton(QMessageBox::Cancel);
    ask.set_text_title(_("You say you wanna revolution?"), 
                       _("Revolution!"));
    ret = ask.exec();

    switch (ret) {
    case QMessageBox::Cancel:
      break;
    case QMessageBox::Ok:
      revolution_response(government);
      break;
    }
  } else {
    revolution_response(government);
  }
}

/***********************************************************************//**
  Constructor for choice_dialog_button_data
***************************************************************************/
Choice_dialog_button::Choice_dialog_button(const QString title,
                                           pfcn_void func_in,
                                           QVariant data1_in,
                                           QVariant data2_in)
  : QPushButton(title)
{
  func = func_in;
  data1 = data1_in;
  data2 = data2_in;
}

/***********************************************************************//**
  Get the function to call when the button is pressed.
***************************************************************************/
pfcn_void Choice_dialog_button::getFunc()
{
  return func;
}

/***********************************************************************//**
  Get the first piece of data to feed the function when the button is
  pressed.
***************************************************************************/
QVariant Choice_dialog_button::getData1()
{
  return data1;
}

/***********************************************************************//**
  Get the second piece of data to feed the function when the button is
  pressed.
***************************************************************************/
QVariant Choice_dialog_button::getData2()
{
  return data2;
}

/***********************************************************************//**
  Sets the first piece of data
***************************************************************************/
void Choice_dialog_button::setData1(QVariant wariat)
{
  data1 = wariat;
}

/***********************************************************************//**
  Sets the second piece of data
***************************************************************************/
void Choice_dialog_button::setData2(QVariant wariat)
{
  data2 = wariat;
}

/***********************************************************************//**
  Constructor for choice_dialog
***************************************************************************/
choice_dialog::choice_dialog(const QString title, const QString text,
                             QWidget *parent,
                             void (*run_on_close_in)(int)): QWidget(parent)
{
  QLabel *l = new QLabel(text);

  setProperty("themed_choice", true);
  signal_mapper = new QSignalMapper(this);
  layout = new QVBoxLayout(this);
  run_on_close = run_on_close_in;

  layout->addWidget(l);
  setWindowFlags(Qt::Dialog);
  setWindowTitle(title);
  setAttribute(Qt::WA_DeleteOnClose);
  gui()->set_diplo_dialog(this);

  unit_id = IDENTITY_NUMBER_ZERO;
  target_id[ATK_SELF] = unit_id;
  target_id[ATK_CITY] = IDENTITY_NUMBER_ZERO;
  target_id[ATK_UNIT] = IDENTITY_NUMBER_ZERO;
  target_id[ATK_UNITS] = TILE_INDEX_NONE;
  target_id[ATK_TILE] = TILE_INDEX_NONE;
  target_extra_id = EXTRA_NONE;

  targeted_unit = nullptr;
  /* No buttons are added yet. */
  for (int i = 0; i < BUTTON_COUNT; i++) {
    action_button_map << NULL;
  }
}

/***********************************************************************//**
  Destructor for choice dialog
***************************************************************************/
choice_dialog::~choice_dialog()
{
  buttons_list.clear();
  action_button_map.clear();
  delete signal_mapper;
  gui()->set_diplo_dialog(NULL);

  if (run_on_close) {
    run_on_close(unit_id);
    run_on_close = NULL;
  }
}

/***********************************************************************//**
  Sets layout for choice dialog
***************************************************************************/
void choice_dialog::set_layout()
{

  targeted_unit = game_unit_by_number(target_id[ATK_UNIT]);

  if ((game_unit_by_number(unit_id)) && targeted_unit
      && unit_list_size(targeted_unit->tile->units) > 1) {
    struct canvas *pix;
    QPushButton *next, *prev;
    unit_skip = new QHBoxLayout;
    next = new QPushButton();
    next->setIcon(fc_icons::instance()->get_icon("city-right"));
    next->setIconSize(QSize(32, 32));
    next->setFixedSize(QSize(36, 36));
    prev = new QPushButton();
    prev->setIcon(fc_icons::instance()->get_icon("city-left"));
    prev->setIconSize(QSize(32, 32));
    prev->setFixedSize(QSize(36, 36));
    target_unit_button = new QPushButton;
    pix = qtg_canvas_create(tileset_unit_width(tileset),
                            tileset_unit_height(tileset));
    pix->map_pixmap.fill(Qt::transparent);
    put_unit(targeted_unit, pix, 1.0, 0, 0);
    target_unit_button->setIcon(QIcon(pix->map_pixmap));
    qtg_canvas_free(pix);
    target_unit_button->setIconSize(QSize(96, 96));
    target_unit_button->setFixedSize(QSize(100, 100));
    unit_skip->addStretch(100);
    unit_skip->addWidget(prev, Qt::AlignCenter);
    unit_skip->addWidget(target_unit_button, Qt::AlignCenter);
    unit_skip->addWidget(next, Qt::AlignCenter);
    layout->addLayout(unit_skip);
    unit_skip->addStretch(100);
    connect(prev, &QAbstractButton::clicked, this, &choice_dialog::prev_unit);
    connect(next, &QAbstractButton::clicked, this, &choice_dialog::next_unit);
  }

  connect(signal_mapper, SIGNAL(mapped(const int &)),
           this, SLOT(execute_action(const int &)));
  setLayout(layout);
}

/***********************************************************************//**
  Adds new action for choice dialog
***************************************************************************/
void choice_dialog::add_item(QString title, pfcn_void func, QVariant data1,
                             QVariant data2, QString tool_tip = "",
                             const int button_id = -1)
{
   Choice_dialog_button *button = new Choice_dialog_button(title, func,
                                                           data1, data2);
   connect(button, SIGNAL(clicked()), signal_mapper, SLOT(map()));
   signal_mapper->setMapping(button, buttons_list.count());
   buttons_list.append(button);

   if (!tool_tip.isEmpty()) {
     button->setToolTip(tool_tip);
   }

   if (0 <= button_id) {
     /* The id is valid. */
     action_button_map[button_id] = button;
   }

   layout->addWidget(button);
}

/***********************************************************************//**
  Shows choice dialog
***************************************************************************/
void choice_dialog::show_me()
{
  QPoint p;

  p = mapFromGlobal(QCursor::pos());
  p.setY(p.y()-this->height());
  p.setX(p.x()-this->width());
  move(p);
  show();
}

/***********************************************************************//**
  Returns layout in choice dialog
***************************************************************************/
QVBoxLayout *choice_dialog::get_layout()
{
  return layout;
}

/***********************************************************************//**
  Get the button with the given identity.
***************************************************************************/
Choice_dialog_button *choice_dialog::get_identified_button(const int id)
{
  if (id < 0) {
    fc_assert_msg(0 <= id, "Invalid button ID.");
    return NULL;
  }

  return action_button_map[id];
}

/***********************************************************************//**
  Try to pick up default unit action
***************************************************************************/
bool try_default_unit_action(QVariant q1, QVariant q2)
{
  action_id action;
  pfcn_void func;

  action = qdef_act::action()->vs_unit_get();
  if (action == -1) {
    return false;
  }
  func = af_map[action];

  func(q1, q2);
  return true;
}

/***********************************************************************//**
  Try to pick up default city action
***************************************************************************/
bool try_default_city_action(QVariant q1, QVariant q2)
{
  action_id action;
  pfcn_void func;

  action = qdef_act::action()->vs_city_get();
  if (action == -1) {
    return false;
  }
  func = af_map[action];
  func(q1, q2);
  return true;
}

/***********************************************************************//**
  Focus next target
***************************************************************************/
void choice_dialog::next_unit()
{
  struct tile *ptile;
  struct unit *new_target = nullptr;
  bool break_next = false;
  bool first = true;
  struct canvas *pix;

  if (targeted_unit == nullptr) {
    return;
  }

  ptile = targeted_unit->tile;

  unit_list_iterate(ptile->units, ptgt) {
    if (first) {
      new_target = ptgt;
      first = false;
    }
    if (break_next == true) {
      new_target = ptgt;
      break;
    }
    if (ptgt == targeted_unit) {
       break_next = true;
    }
  } unit_list_iterate_end;
  targeted_unit = new_target;
  pix = qtg_canvas_create(tileset_unit_width(tileset),
                          tileset_unit_height(tileset));
  pix->map_pixmap.fill(Qt::transparent);
  put_unit(targeted_unit, pix, 1.0, 0, 0);
  target_unit_button->setIcon(QIcon(pix->map_pixmap));
  qtg_canvas_free(pix);
  switch_target();
}

/***********************************************************************//**
  Focus previous target
***************************************************************************/
void choice_dialog::prev_unit()
{
  struct tile *ptile;
  struct unit *new_target = nullptr;
  struct canvas *pix;
  if (targeted_unit == nullptr) {
    return;
  }

  ptile = targeted_unit->tile;
  unit_list_iterate(ptile->units, ptgt) {
    if ((ptgt == targeted_unit) && new_target != nullptr) {
       break;
    }
    new_target = ptgt;
  } unit_list_iterate_end;
  targeted_unit = new_target;
  pix = qtg_canvas_create(tileset_unit_width(tileset),
                          tileset_unit_height(tileset));
  pix->map_pixmap.fill(Qt::transparent);
  put_unit(targeted_unit, pix, 1.0, 0, 0);
  target_unit_button->setIcon(QIcon(pix->map_pixmap));
  qtg_canvas_free(pix);
  switch_target();
}

/***********************************************************************//**
  Update dialog for new target (targeted_unit)
***************************************************************************/
void choice_dialog::update_dialog(const struct act_prob *act_probs)
{
  if (targeted_unit == nullptr) {
    return;
  }
  unit_skip->setParent(nullptr);
  action_selection_refresh(game_unit_by_number(unit_id), nullptr,
                           targeted_unit, targeted_unit->tile,
                           (target_extra_id != EXTRA_NONE
                              ? extra_by_number(target_extra_id)
                              : NULL),
                           act_probs);
  layout->addLayout(unit_skip);
}

/***********************************************************************//**
  Switches target unit
***************************************************************************/
void choice_dialog::switch_target()
{
  if (targeted_unit == nullptr) {
    return;
  }

  unit_skip->setParent(nullptr);
  dsend_packet_unit_get_actions(&client.conn,
                                unit_id,
                                targeted_unit->id,
                                targeted_unit->tile->index,
                                action_selection_target_extra(),
                                TRUE);
  layout->addLayout(unit_skip);
}

/***********************************************************************//**
  Run chosen action and close dialog
***************************************************************************/
void choice_dialog::execute_action(const int action)
{
  Choice_dialog_button *button = buttons_list.at(action);
  pfcn_void func = button->getFunc();

  func(button->getData1(), button->getData2());
  close();
}

/***********************************************************************//**
  Put the button in the stack and temporarily remove it. When
  unstack_all_buttons() is called all buttons in the stack will be added
  to the end of the dialog.

  Can be used to place a button below existing buttons or below buttons
  added while it was in the stack.
***************************************************************************/
void choice_dialog::stack_button(Choice_dialog_button *button)
{
  /* Store the data in the stack. */
  last_buttons_stack.append(button);

  /* Temporary remove the button so it will end up below buttons added
   * before unstack_all_buttons() is called. */
  layout->removeWidget(button);

  /* The old mappings may not be valid after reinsertion. */
  signal_mapper->removeMappings(button);

  /* Synchronize the list with the layout. */
  buttons_list.removeAll(button);
}

/***********************************************************************//**
  Put all the buttons in the stack back to the dialog. They will appear
  after any other buttons. See stack_button()
***************************************************************************/
void choice_dialog::unstack_all_buttons()
{
  while (!last_buttons_stack.isEmpty()) {
    Choice_dialog_button *button = last_buttons_stack.takeLast();

    /* Restore mapping. */
    signal_mapper->setMapping(button, buttons_list.count());

    /* Reinsert the button below the other buttons. */
    buttons_list.append(button);
    layout->addWidget(button);
  }
}

/***********************************************************************//**
  Action enter market place for choice dialog
***************************************************************************/
static void caravan_marketplace(QVariant data1, QVariant data2)
{
  int actor_unit_id = data1.toInt();
  int target_city_id = data2.toInt();

  if (NULL != game_unit_by_number(actor_unit_id)
      && NULL != game_city_by_number(target_city_id)) {
    request_do_action(ACTION_MARKETPLACE, actor_unit_id,
                      target_city_id, 0, "");
  }
}

/***********************************************************************//**
  Action establish trade for choice dialog
***************************************************************************/
static void caravan_establish_trade(QVariant data1, QVariant data2)
{
  int actor_unit_id = data1.toInt();
  int target_city_id = data2.toInt();

  if (NULL != game_unit_by_number(actor_unit_id)
      && NULL != game_city_by_number(target_city_id)) {
    request_do_action(ACTION_TRADE_ROUTE, actor_unit_id,
                      target_city_id, 0, "");
  }
}

/***********************************************************************//**
  Action help build wonder for choice dialog
***************************************************************************/
static void caravan_help_build(QVariant data1, QVariant data2)
{
  int caravan_id = data1.toInt();
  int caravan_target_id = data2.toInt();

  if (NULL != game_unit_by_number(caravan_id)
      && NULL != game_city_by_number(caravan_target_id)) {
    request_do_action(ACTION_HELP_WONDER,
                      caravan_id, caravan_target_id, 0, "");
  }
}

/***********************************************************************//**
  Action Recycle Unit for choice dialog
***************************************************************************/
static void unit_recycle(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int tgt_city_id = data2.toInt();

  if (NULL != game_unit_by_number(actor_id)
      && NULL != game_city_by_number(tgt_city_id)) {
    request_do_action(ACTION_RECYCLE_UNIT,
                      actor_id, tgt_city_id, 0, "");
  }
}

/***********************************************************************//**
  Action Home City for choice dialog
***************************************************************************/
static void unit_home_city(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int tgt_city_id = data2.toInt();

  if (NULL != game_unit_by_number(actor_id)
      && NULL != game_city_by_number(tgt_city_id)) {
    request_do_action(ACTION_HOME_CITY,
                      actor_id, tgt_city_id, 0, "");
  }
}

/***********************************************************************//**
  Action "Upgrade Unit" for choice dialog
***************************************************************************/
static void unit_upgrade(QVariant data1, QVariant data2)
{
  struct unit *punit;

  int actor_id = data1.toInt();
  int tgt_city_id = data2.toInt();

  if ((punit = game_unit_by_number(actor_id))
      && NULL != game_city_by_number(tgt_city_id)) {
    struct unit_list *as_list;

    as_list = unit_list_new();
    unit_list_append(as_list, punit);
    popup_upgrade_dialog(as_list);
    unit_list_destroy(as_list);
  }
}

/***********************************************************************//**
  Action "Airlift Unit" for choice dialog
***************************************************************************/
static void airlift(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int tgt_city_id = data2.toInt();

  if (NULL != game_unit_by_number(actor_id)
      && NULL != game_city_by_number(tgt_city_id)) {
    request_do_action(ACTION_AIRLIFT,
                      actor_id, tgt_city_id, 0, "");
  }
}

/***********************************************************************//**
  Action "Conquer City" for choice dialog
***************************************************************************/
static void conquer_city(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int tgt_city_id = data2.toInt();

  if (NULL != game_unit_by_number(actor_id)
      && NULL != game_city_by_number(tgt_city_id)) {
    request_do_action(ACTION_CONQUER_CITY,
                      actor_id, tgt_city_id, 0, "");
  }
}

/***********************************************************************//**
  Delay selection of what action to take.
***************************************************************************/
static void act_sel_wait(QVariant data1, QVariant data2)
{
  key_unit_wait();
}

/***********************************************************************//**
  Empty action for choice dialog (just do nothing)
***************************************************************************/
static void keep_moving(QVariant data1, QVariant data2)
{
}

/***********************************************************************//**
  Starts revolution with targeted government as target or anarchy otherwise
***************************************************************************/
void revolution_response(struct government *government)
{
  if (!government) {
    start_revolution();
  } else {
    set_government_choice(government);
  }
}

/***********************************************************************//**
  Move the queue of diplomats that need user input forward unless the
  current diplomat will need more input.
***************************************************************************/
static void diplomat_queue_handle_primary(int actor_unit_id)
{
  if (!is_more_user_input_needed) {
    /* The client isn't waiting for information for any unanswered follow
     * up questions. */

    struct unit *actor_unit;

    if ((actor_unit = game_unit_by_number(actor_unit_id))) {
      /* The action selection dialog wasn't closed because the actor unit
       * was lost. */

      /* The probabilities didn't just disappear, right? */
      fc_assert_action(actor_unit->client.act_prob_cache,
                       client_unit_init_act_prob_cache(actor_unit));

      FC_FREE(actor_unit->client.act_prob_cache);
    }

     /* The action selection process is over, at least for now. */
    action_selection_no_longer_in_progress(actor_unit_id);

    if (did_not_decide) {
      /* The action selection dialog was closed but the player didn't
       * decide what the unit should do. */

      /* Reset so the next action selection dialog does the right thing. */
      did_not_decide = false;
    } else {
      /* An action, or no action at all, was selected. */
      action_decision_clear_want(actor_unit_id);
      action_selection_next_in_focus(actor_unit_id);
    }
  }
}

/***********************************************************************//**
  Move the queue of diplomats that need user input forward since the
  current diplomat got the extra input that was required.
***************************************************************************/
static void diplomat_queue_handle_secondary(int actor_id)
{
  /* Stop waiting. Move on to the next queued diplomat. */
  is_more_user_input_needed = FALSE;
  diplomat_queue_handle_primary(actor_id);
}

/**********************************************************************//**
  Let the non shared client code know that the action selection process
  no longer is in progress for the specified unit.

  This allows the client to clean up any client specific assumptions.
**************************************************************************/
void action_selection_no_longer_in_progress_gui_specific(int actor_id)
{
  /* Stop assuming the answer to a follow up question will arrive. */
  is_more_user_input_needed = FALSE;
}

/***********************************************************************//**
  Returns a string with how many shields remains of the current production.
  This is useful as custom information on the help build wonder button.
***************************************************************************/
static QString city_prod_remaining(struct city *target_city)
{
  if (target_city == nullptr
      || city_owner(target_city) != client.conn.playing) {
    /* Can't give remaining production for a foreign or non existing
     * city. */
    return "";
  }

  return QString(_("%1 remaining")).arg(
        impr_build_shield_cost(target_city,
                               target_city->production.value.building)
        - target_city->shield_stock);
}

/***********************************************************************//**
  Popup a dialog that allows the player to select what action a unit
  should take.
***************************************************************************/
void popup_action_selection(struct unit *actor_unit,
                            struct city *target_city,
                            struct unit *target_unit,
                            struct tile *target_tile,
                            struct extra_type *target_extra,
                            const struct act_prob *act_probs)
{
  struct astring title = ASTRING_INIT, text = ASTRING_INIT;
  choice_dialog *cd;
  qtiles caras;
  QVariant qv1, qv2;
  pfcn_void func;
  struct city *actor_homecity;
  action_id unit_act;
  action_id city_act;

  unit_act = qdef_act::action()->vs_unit_get();
  city_act = qdef_act::action()->vs_city_get();

  foreach (caras, gui()->trade_gen.lines) {
    if (caras.autocaravan == actor_unit) {
      int i;
      if (nullptr != game_unit_by_number(actor_unit->id)
          && nullptr != game_city_by_number(target_city->id)) {
        request_do_action(ACTION_TRADE_ROUTE, actor_unit->id,
                          target_city->id, 0, "");
        client_unit_init_act_prob_cache(actor_unit);
        diplomat_queue_handle_primary(actor_unit->id);
        i = gui()->trade_gen.lines.indexOf(caras);
        gui()->trade_gen.lines.takeAt(i);
        return;
      }
    }
  }
  if (target_city
      && try_default_city_action(actor_unit->id, target_city->id)
      && action_prob_possible(act_probs[unit_act])) {
    diplomat_queue_handle_primary(actor_unit->id);
    return;
  }

  if (target_unit
      && try_default_unit_action(actor_unit->id, target_unit->id)
      && action_prob_possible(act_probs[city_act])) {
    diplomat_queue_handle_primary(actor_unit->id);
    return;
  }
  /* Could be caused by the server failing to reply to a request for more
   * information or a bug in the client code. */
  fc_assert_msg(!is_more_user_input_needed,
                "Diplomat queue problem. Is another diplomat window open?");

  /* No extra input is required as no action has been chosen yet. */
  is_more_user_input_needed = FALSE;

  actor_homecity = game_city_by_number(actor_unit->homecity);

  astr_set(&title,
           /* TRANS: %s is a unit name, e.g., Spy */
           _("Choose Your %s's Strategy"),
           unit_name_translation(actor_unit));

  if (target_city && actor_homecity) {
    astr_set(&text,
             _("Your %s from %s reaches the city of %s.\nWhat now?"),
             unit_name_translation(actor_unit),
             city_name_get(actor_homecity),
             city_name_get(target_city));
  } else if (target_city) {
    astr_set(&text,
             _("Your %s has arrived at %s.\nWhat is your command?"),
             unit_name_translation(actor_unit),
             city_name_get(target_city));
  } else if (target_unit) {
    astr_set(&text,
             /* TRANS: Your Spy is ready to act against Roman Freight. */
             _("Your %s is ready to act against %s %s."),
             unit_name_translation(actor_unit),
             nation_adjective_for_player(unit_owner(target_unit)),
             unit_name_translation(target_unit));
  } else {
    fc_assert_msg(target_unit || target_city || target_tile,
                  "No target specified.");

    astr_set(&text,
             /* TRANS: %s is a unit name, e.g., Diplomat, Spy */
             _("Your %s is waiting for your command."),
             unit_name_translation(actor_unit));
  }

  cd = gui()->get_diplo_dialog();
  if ((cd != nullptr) && cd->targeted_unit != nullptr) {
    cd->update_dialog(act_probs);
    return;
  }
  cd = new choice_dialog(astr_str(&title), astr_str(&text),
                         gui()->game_tab_widget,
                         diplomat_queue_handle_primary);
  qv1 = actor_unit->id;

  cd->unit_id = actor_unit->id;

  cd->target_id[ATK_SELF] = cd->unit_id;

  if (target_city) {
    cd->target_id[ATK_CITY] = target_city->id;
  } else {
    cd->target_id[ATK_CITY] = IDENTITY_NUMBER_ZERO;
  }

  if (target_unit) {
    cd->target_id[ATK_UNIT] = target_unit->id;
  } else {
    cd->target_id[ATK_UNIT] = IDENTITY_NUMBER_ZERO;
  }

  if (target_tile) {
    cd->target_id[ATK_UNITS] = tile_index(target_tile);
  } else {
    cd->target_id[ATK_UNITS] = TILE_INDEX_NONE;
  }

  if (target_tile) {
    cd->target_id[ATK_TILE] = tile_index(target_tile);
  } else {
    cd->target_id[ATK_TILE] = TILE_INDEX_NONE;
  }

  if (target_extra) {
    cd->target_extra_id = extra_number(target_extra);
  } else {
    cd->target_extra_id = EXTRA_NONE;
  }

  /* Unit acting against a city */

  /* Set the correct target for the following actions. */
  qv2 = cd->target_id[ATK_CITY];

  action_iterate(act) {
    if (action_id_get_actor_kind(act) == AAK_UNIT
        && action_id_get_target_kind(act) == ATK_CITY) {
      action_entry(cd, act, act_probs,
                   act == ACTION_HELP_WONDER ?
                     city_prod_remaining(target_city) : "",
                   qv1, qv2);
    }
  } action_iterate_end;

  /* Unit acting against another unit */

  /* Set the correct target for the following actions. */
  qv2 = cd->target_id[ATK_UNIT];

  action_iterate(act) {
    if (action_id_get_actor_kind(act) == AAK_UNIT
        && action_id_get_target_kind(act) == ATK_UNIT) {
      action_entry(cd, act, act_probs, "", qv1, qv2);
    }
  } action_iterate_end;

  /* Unit acting against all units at a tile */

  /* Set the correct target for the following actions. */
  qv2 = cd->target_id[ATK_UNITS];

  action_iterate(act) {
    if (action_id_get_actor_kind(act) == AAK_UNIT
        && action_id_get_target_kind(act) == ATK_UNITS) {
      action_entry(cd, act, act_probs, "", qv1, qv2);
    }
  } action_iterate_end;

  /* Unit acting against a tile. */

  /* Set the correct target for the following actions. */
  qv2 = cd->target_id[ATK_TILE];

  action_iterate(act) {
    if (action_id_get_actor_kind(act) == AAK_UNIT
        && action_id_get_target_kind(act) == ATK_TILE) {
      action_entry(cd, act, act_probs, "", qv1, qv2);
    }
  } action_iterate_end;

  /* Unit acting against itself */

  /* Set the correct target for the following actions. */
  qv2 = cd->target_id[ATK_SELF];

  action_iterate(act) {
    if (action_id_get_actor_kind(act) == AAK_UNIT
        && action_id_get_target_kind(act) == ATK_SELF) {
      action_entry(cd, act, act_probs, "", qv1, qv2);
    }
  } action_iterate_end;

  if (unit_can_move_to_tile(&(wld.map), actor_unit, target_tile,
                            FALSE, FALSE)) {
    qv2 = target_tile->index;

    func = act_sel_keep_moving;
    cd->add_item(QString(_("Keep moving")), func, qv1, qv2,
                 "", BUTTON_MOVE);
  }

  func = act_sel_wait;
  cd->add_item(QString(_("&Wait")), func, qv1, qv2,
               "", BUTTON_WAIT);

  func = keep_moving;
  cd->add_item(QString(_("Do nothing")), func, qv1, qv2,
               "", BUTTON_CANCEL);

  cd->set_layout();
  cd->show_me();

  /* Give follow up questions access to action probabilities. */
  client_unit_init_act_prob_cache(actor_unit);
  action_iterate(act) {
    actor_unit->client.act_prob_cache[act] = act_probs[act];
  } action_iterate_end;

  astr_free(&title);
  astr_free(&text);
}

/***********************************************************************//**
  Get the non targeted version of an action so it, if enabled, can appear
  in the target selection dialog.
***************************************************************************/
static action_id get_non_targeted_action_id(action_id tgt_action_id)
{
  /* Don't add an action mapping here unless the non targeted version is
   * selectable in the targeted version's target selection dialog. */
  switch ((enum gen_action)tgt_action_id) {
  case ACTION_SPY_TARGETED_SABOTAGE_CITY:
    return ACTION_SPY_SABOTAGE_CITY;
  case ACTION_SPY_TARGETED_SABOTAGE_CITY_ESC:
    return ACTION_SPY_SABOTAGE_CITY_ESC;
  case ACTION_SPY_TARGETED_STEAL_TECH:
    return ACTION_SPY_STEAL_TECH;
  case ACTION_SPY_TARGETED_STEAL_TECH_ESC:
    return ACTION_SPY_STEAL_TECH_ESC;
  default:
    /* No non targeted version found. */
    return ACTION_NONE;
  }
}

/***********************************************************************//**
  Get the targeted version of an action so it, if enabled, can hide the
  non targeted action in the action selection dialog.
***************************************************************************/
static action_id get_targeted_action_id(action_id non_tgt_action_id)
{
  /* Don't add an action mapping here unless the non targeted version is
   * selectable in the targeted version's target selection dialog. */
  switch ((enum gen_action)non_tgt_action_id) {
  case ACTION_SPY_SABOTAGE_CITY:
    return ACTION_SPY_TARGETED_SABOTAGE_CITY;
  case ACTION_SPY_SABOTAGE_CITY_ESC:
    return ACTION_SPY_TARGETED_SABOTAGE_CITY_ESC;
  case ACTION_SPY_STEAL_TECH:
    return ACTION_SPY_TARGETED_STEAL_TECH;
  case ACTION_SPY_STEAL_TECH_ESC:
    return ACTION_SPY_TARGETED_STEAL_TECH_ESC;
  default:
    /* No targeted version found. */
    return ACTION_NONE;
  }
}

/***********************************************************************//**
  Show the user the action if it is enabled.
***************************************************************************/
static void action_entry(choice_dialog *cd,
                         action_id act,
                         const struct act_prob *act_probs,
                         QString custom,
                         QVariant data1, QVariant data2)
{
  QString title;
  QString tool_tip;

  if (get_targeted_action_id(act) != ACTION_NONE
      && action_prob_possible(act_probs[get_targeted_action_id(act)])) {
    /* The player can select the untargeted version from the target
     * selection dialog. */
    return;
  }

  if (!af_map.contains(act)) {
    /* The Qt client doesn't support ordering this action from the
     * action selection dialog. */
    return;
  }

  /* Don't show disabled actions. */
  if (!action_prob_possible(act_probs[act])) {
    return;
  }

  title = QString(action_prepare_ui_name(act, "&",
                                         act_probs[act],
                                         custom != "" ?
                                             custom.toUtf8().data() :
                                             NULL));

  tool_tip = QString(action_get_tool_tip(act, act_probs[act]));

  cd->add_item(title, af_map[act], data1, data2, tool_tip, act);
}

/***********************************************************************//**
  Update an existing button.
***************************************************************************/
static void action_entry_update(Choice_dialog_button *button,
                                action_id act,
                                const struct act_prob *act_probs,
                                QString custom,
                                QVariant data1, QVariant data2)
{
  QString title;
  QString tool_tip;

  /* An action that just became impossible has its button disabled.
   * An action that became possible again must be reenabled. */
  button->setEnabled(action_prob_possible(act_probs[act]));
  button->setData1(data1);
  button->setData2(data2);
  /* The probability may have changed. */
  title = QString(action_prepare_ui_name(act, "&",
                                         act_probs[act],
                                         custom != "" ?
                                             custom.toUtf8().data() :
                                             NULL));

  tool_tip = QString(action_get_tool_tip(act, act_probs[act]));

  button->setText(title);
  button->setToolTip(tool_tip);
}

/***********************************************************************//**
  Action Disband Unit for choice dialog
***************************************************************************/
static void disband_unit(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  request_do_action(ACTION_DISBAND_UNIT, actor_id,
                    target_id, 0, "");
}

/***********************************************************************//**
  Action "Fortify" for choice dialog
***************************************************************************/
static void fortify(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  if (NULL != game_unit_by_number(actor_id)) {
    request_do_action(ACTION_FORTIFY, actor_id,
                      target_id, 0, "");
  }
}

/***********************************************************************//**
  Action Convert Unit for choice dialog
***************************************************************************/
static void convert_unit(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  request_do_action(ACTION_CONVERT, actor_id,
                    target_id, 0, "");
}

/***********************************************************************//**
  Action bribe unit for choice dialog
***************************************************************************/
static void diplomat_bribe(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (NULL != game_unit_by_number(diplomat_id)
      && NULL != game_unit_by_number(diplomat_target_id)) {
    /* Wait for the server's reply before moving on to the next queued diplomat. */
    is_more_user_input_needed = TRUE;

    request_action_details(ACTION_SPY_BRIBE_UNIT, diplomat_id,
                           diplomat_target_id);
  }
}

/***********************************************************************//**
  Action sabotage unit for choice dialog
***************************************************************************/
static void spy_sabotage_unit(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  request_do_action(ACTION_SPY_SABOTAGE_UNIT, diplomat_id,
                    diplomat_target_id, 0, "");
}

/***********************************************************************//**
  Action Sabotage Unit Escape for choice dialog
***************************************************************************/
static void spy_sabotage_unit_esc(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  request_do_action(ACTION_SPY_SABOTAGE_UNIT_ESC, diplomat_id,
                    diplomat_target_id, 0, "");
}

/***********************************************************************//**
  Action "Heal Unit" for choice dialog
***************************************************************************/
static void heal_unit(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  request_do_action(ACTION_HEAL_UNIT, actor_id, target_id, 0, "");
}

/***********************************************************************//**
  Action capture units for choice dialog
***************************************************************************/
static void capture_units(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  request_do_action(ACTION_CAPTURE_UNITS, actor_id,
                    target_id, 0, "");
}

/***********************************************************************//**
  Action expel unit for choice dialog
***************************************************************************/
static void expel_unit(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  request_do_action(ACTION_EXPEL_UNIT, actor_id,
                    target_id, 0, "");
}

/***********************************************************************//**
  Action bombard for choice dialog
***************************************************************************/
static void bombard(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  request_do_action(ACTION_BOMBARD, actor_id,
                    target_id, 0, "");
}

/***********************************************************************//**
  Action build city for choice dialog
***************************************************************************/
static void found_city(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();

  dsend_packet_city_name_suggestion_req(&client.conn,
                                        actor_id);
}

/***********************************************************************//**
  Action "Transform Terrain" for choice dialog
***************************************************************************/
static void transform_terrain(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  if (NULL != game_unit_by_number(actor_id)
      && NULL != index_to_tile(&(wld.map), target_id)) {
    request_do_action(ACTION_TRANSFORM_TERRAIN,
                      actor_id, target_id, 0, "");
  }
}

/***********************************************************************//**
  Action "Irrigate TF" for choice dialog
***************************************************************************/
static void irrigate_tf(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  if (NULL != game_unit_by_number(actor_id)
      && NULL != index_to_tile(&(wld.map), target_id)) {
    request_do_action(ACTION_IRRIGATE_TF,
                      actor_id, target_id, 0, "");
  }
}

/***********************************************************************//**
  Action "Mine TF" for choice dialog
***************************************************************************/
static void mine_tf(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  if (NULL != game_unit_by_number(actor_id)
      && NULL != index_to_tile(&(wld.map), target_id)) {
    request_do_action(ACTION_MINE_TF,
                      actor_id, target_id, 0, "");
  }
}

/***********************************************************************//**
  Action "Pillage" for choice dialog
***************************************************************************/
static void pillage(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  if (NULL != game_unit_by_number(actor_id)
      && NULL != index_to_tile(&(wld.map), target_id)) {
    request_do_action(ACTION_PILLAGE,
                      actor_id, target_id,
                      /* FIXME: will cause problems if more than
                       * one action selection dialog at a time
                       * becomes supported. */
                      action_selection_target_extra(),
                      "");
  }
}

/***********************************************************************//**
  Action "Road" for choice dialog
***************************************************************************/
static void road(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  if (NULL != game_unit_by_number(actor_id)
      && NULL != index_to_tile(&(wld.map), target_id)
      && NULL != extra_by_number(action_selection_target_extra())) {
    request_do_action(ACTION_ROAD,
                      actor_id, target_id,
                      /* FIXME: will cause problems if more than
                       * one action selection dialog at a time
                       * becomes supported. */
                      action_selection_target_extra(),
                      "");
  }
}

/***********************************************************************//**
  Action "Build Base" for choice dialog
***************************************************************************/
static void base(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  if (NULL != game_unit_by_number(actor_id)
      && NULL != index_to_tile(&(wld.map), target_id)
      && NULL != extra_by_number(action_selection_target_extra())) {
    request_do_action(ACTION_BASE,
                      actor_id, target_id,
                      /* FIXME: will cause problems if more than
                       * one action selection dialog at a time
                       * becomes supported. */
                      action_selection_target_extra(),
                      "");
  }
}

/***********************************************************************//**
  Action "Build Mine" for choice dialog
***************************************************************************/
static void mine(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  if (NULL != game_unit_by_number(actor_id)
      && NULL != index_to_tile(&(wld.map), target_id)
      && NULL != extra_by_number(action_selection_target_extra())) {
    request_do_action(ACTION_MINE,
                      actor_id, target_id,
                      /* FIXME: will cause problems if more than
                       * one action selection dialog at a time
                       * becomes supported. */
                      action_selection_target_extra(),
                      "");
  }
}

/***********************************************************************//**
  Action "Build Irrigation" for choice dialog
***************************************************************************/
static void irrigate(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  if (NULL != game_unit_by_number(actor_id)
      && NULL != index_to_tile(&(wld.map), target_id)
      && NULL != extra_by_number(action_selection_target_extra())) {
    request_do_action(ACTION_IRRIGATE,
                      actor_id, target_id,
                      /* FIXME: will cause problems if more than
                       * one action selection dialog at a time
                       * becomes supported. */
                      action_selection_target_extra(),
                      "");
  }
}

/***********************************************************************//**
  Action "Explode Nuclear" for choice dialog
***************************************************************************/
static void nuke(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (NULL != game_unit_by_number(diplomat_id)
      && NULL != index_to_tile(&(wld.map), diplomat_target_id)) {
    request_do_action(ACTION_NUKE,
                      diplomat_id, diplomat_target_id, 0, "");
  }
}

/***********************************************************************//**
  Action "Attack" for choice dialog
***************************************************************************/
static void attack(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (NULL != game_unit_by_number(diplomat_id)
      && NULL != index_to_tile(&(wld.map), diplomat_target_id)) {
    request_do_action(ACTION_ATTACK,
                      diplomat_id, diplomat_target_id, 0, "");
  }
}

/***********************************************************************//**
  Action "Suicide Attack" for choice dialog
***************************************************************************/
static void suicide_attack(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (NULL != game_unit_by_number(diplomat_id)
      && NULL != index_to_tile(&(wld.map), diplomat_target_id)) {
    request_do_action(ACTION_SUICIDE_ATTACK,
                      diplomat_id, diplomat_target_id, 0, "");
  }
}

/***********************************************************************//**
  Action "Paradrop Unit" for choice dialog
***************************************************************************/
static void paradrop(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  if (NULL != game_unit_by_number(actor_id)
      && NULL != index_to_tile(&(wld.map), target_id)) {
    request_do_action(ACTION_PARADROP,
                      actor_id, target_id, 0, "");
  }
}

/***********************************************************************//**
  Action join city for choice dialog
***************************************************************************/
static void join_city(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  if (NULL != game_unit_by_number(actor_id)
      && NULL != game_city_by_number(target_id)) {
    request_do_action(ACTION_JOIN_CITY,
                      actor_id, target_id, 0, "");
  }
}

/***********************************************************************//**
  Action steal tech with spy for choice dialog
***************************************************************************/
static void spy_steal_shared(QVariant data1, QVariant data2,
                             action_id act_id)
{
  QString str;
  QVariant qv1;
  pfcn_void func;
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();
  struct unit *actor_unit = game_unit_by_number(diplomat_id);
  struct city *pvcity = game_city_by_number(diplomat_target_id);
  struct player *pvictim = NULL;
  choice_dialog *cd;
  QList<QVariant> actor_and_target;

  /* Wait for the player's reply before moving on to the next queued diplomat. */
  is_more_user_input_needed = TRUE;

  if (pvcity) {
    pvictim = city_owner(pvcity);
  }
  cd = gui()->get_diplo_dialog();
  if (cd != NULL) {
    cd->close();
  }
  struct astring stra = ASTRING_INIT;
  cd = new choice_dialog(_("Steal"), _("Steal Technology"),
                         gui()->game_tab_widget,
                         diplomat_queue_handle_secondary);

  /* Put both actor and target city in qv1 since qv2 is taken */
  actor_and_target.append(diplomat_id);
  actor_and_target.append(diplomat_target_id);
  actor_and_target.append(act_id);
  qv1 = QVariant::fromValue(actor_and_target);

  struct player *pplayer = client.conn.playing;
  if (pvictim) {
    const struct research *presearch = research_get(pplayer);
    const struct research *vresearch = research_get(pvictim);

    advance_index_iterate(A_FIRST, i) {
      if (research_invention_gettable(presearch, i,
                                      game.info.tech_steal_allow_holes)
          && research_invention_state(vresearch, i) == TECH_KNOWN
          && research_invention_state(presearch, i) != TECH_KNOWN) {
        func = spy_steal_something;
        str = research_advance_name_translation(presearch, i);
        cd->add_item(str, func, qv1, i);
      }
    } advance_index_iterate_end;

    if (action_prob_possible(actor_unit->client.act_prob_cache[
                             get_non_targeted_action_id(act_id)])) {
      astr_set(&stra, _("At %s's Discretion"),
               unit_name_translation(actor_unit));
      func = spy_steal_something;
      str = astr_str(&stra);
      cd->add_item(str, func, qv1, A_UNSET);
    }

    cd->set_layout();
    cd->show_me();
  }
  astr_free(&stra);
}

/***********************************************************************//**
  Action "Targeted Steal Tech" for choice dialog
***************************************************************************/
static void spy_steal(QVariant data1, QVariant data2)
{
  spy_steal_shared(data1, data2, ACTION_SPY_TARGETED_STEAL_TECH);
}

/***********************************************************************//**
  Action "Targeted Steal Tech Escape Expected" for choice dialog
***************************************************************************/
static void spy_steal_esc(QVariant data1, QVariant data2)
{
  spy_steal_shared(data1, data2, ACTION_SPY_TARGETED_STEAL_TECH_ESC);
}

/***********************************************************************//**
  Action steal given tech for choice dialog
***************************************************************************/
static void spy_steal_something(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toList().at(0).toInt();
  int diplomat_target_id = data1.toList().at(1).toInt();
  action_id act_id = data1.toList().at(2).toInt();

  if (NULL != game_unit_by_number(diplomat_id)
      && NULL != game_city_by_number(diplomat_target_id)) {
    if (data2.toInt() == A_UNSET) {
      /* This is the untargeted version. */
      request_do_action(get_non_targeted_action_id(act_id),
                        diplomat_id, diplomat_target_id, data2.toInt(), "");
    } else {
      /* This is the targeted version. */
      request_do_action(act_id, diplomat_id,
                        diplomat_target_id, data2.toInt(), "");
    }
  }
}

/***********************************************************************//**
  Action  request sabotage list for choice dialog
***************************************************************************/
static void spy_request_sabotage_list(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (NULL != game_unit_by_number(diplomat_id)
      && NULL != game_city_by_number(diplomat_target_id)) {
    /* Wait for the server's reply before moving on to the next queued diplomat. */
    is_more_user_input_needed = TRUE;

    request_action_details(ACTION_SPY_TARGETED_SABOTAGE_CITY, diplomat_id,
                           diplomat_target_id);
  }
}

/***********************************************************************//**
  Action request sabotage (and escape) list for choice dialog
***************************************************************************/
static void spy_request_sabotage_esc_list(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (NULL != game_unit_by_number(diplomat_id)
      && NULL != game_city_by_number(diplomat_target_id)) {
    /* Wait for the server's reply before moving on to the next queued
     * diplomat. */
    is_more_user_input_needed = TRUE;

    request_action_details(ACTION_SPY_TARGETED_SABOTAGE_CITY_ESC,
                           diplomat_id, diplomat_target_id);
  }
}

/***********************************************************************//**
  Action Poison City for choice dialog
***************************************************************************/
static void spy_poison(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (NULL != game_unit_by_number(diplomat_id)
      && NULL != game_city_by_number(diplomat_target_id)) {
    request_do_action(ACTION_SPY_POISON,
                      diplomat_id, diplomat_target_id, 0, "");
  }
}

/***********************************************************************//**
  Action Poison City Escape for choice dialog
***************************************************************************/
static void spy_poison_esc(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (NULL != game_unit_by_number(diplomat_id)
      && NULL != game_city_by_number(diplomat_target_id)) {
    request_do_action(ACTION_SPY_POISON_ESC,
                      diplomat_id, diplomat_target_id, 0, "");
  }
}

/***********************************************************************//**
  Action suitcase nuke for choice dialog
***************************************************************************/
static void spy_nuke_city(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (NULL != game_unit_by_number(diplomat_id)
      && NULL != game_city_by_number(diplomat_target_id)) {
    request_do_action(ACTION_SPY_NUKE,
                      diplomat_id, diplomat_target_id, 0, "");
  }
}

/***********************************************************************//**
  Action suitcase nuke escape for choice dialog
***************************************************************************/
static void spy_nuke_city_esc(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (NULL != game_unit_by_number(diplomat_id)
      && NULL != game_city_by_number(diplomat_target_id)) {
    request_do_action(ACTION_SPY_NUKE_ESC,
                      diplomat_id, diplomat_target_id, 0, "");
  }
}

/***********************************************************************//**
  Action destroy city for choice dialog
***************************************************************************/
static void destroy_city(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (NULL != game_unit_by_number(diplomat_id)
      && NULL != game_city_by_number(diplomat_target_id)) {
    request_do_action(ACTION_DESTROY_CITY,
                      diplomat_id, diplomat_target_id, 0, "");
  }
}

/***********************************************************************//**
  Action steal gold for choice dialog
***************************************************************************/
static void spy_steal_gold(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (NULL != game_unit_by_number(diplomat_id)
      && NULL != game_city_by_number(diplomat_target_id)) {
    request_do_action(ACTION_SPY_STEAL_GOLD,
                      diplomat_id, diplomat_target_id, 0, "");
  }
}

/***********************************************************************//**
  Action steal gold escape for choice dialog
***************************************************************************/
static void spy_steal_gold_esc(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (NULL != game_unit_by_number(diplomat_id)
      && NULL != game_city_by_number(diplomat_target_id)) {
    request_do_action(ACTION_SPY_STEAL_GOLD_ESC,
                      diplomat_id, diplomat_target_id, 0, "");
  }
}

/***********************************************************************//**
  Action steal maps for choice dialog
***************************************************************************/
static void spy_steal_maps(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (NULL != game_unit_by_number(diplomat_id)
      && NULL != game_city_by_number(diplomat_target_id)) {
    request_do_action(ACTION_STEAL_MAPS,
                      diplomat_id, diplomat_target_id, 0, "");
  }
}

/***********************************************************************//**
  Action steal maps escape for choice dialog
***************************************************************************/
static void spy_steal_maps_esc(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (NULL != game_unit_by_number(diplomat_id)
      && NULL != game_city_by_number(diplomat_target_id)) {
    request_do_action(ACTION_STEAL_MAPS_ESC,
                      diplomat_id, diplomat_target_id, 0, "");
  }
}

/***********************************************************************//**
  Action establish embassy for choice dialog
***************************************************************************/
static void spy_embassy(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (NULL != game_unit_by_number(diplomat_id)
      && NULL != game_city_by_number(diplomat_target_id)) {
    request_do_action(ACTION_ESTABLISH_EMBASSY, diplomat_id,
                      diplomat_target_id, 0, "");
  }
}

/***********************************************************************//**
  Action establish embassy for choice dialog
***************************************************************************/
static void diplomat_embassy(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (NULL != game_unit_by_number(diplomat_id)
      && NULL != game_city_by_number(diplomat_target_id)) {
    request_do_action(ACTION_ESTABLISH_EMBASSY_STAY, diplomat_id,
                      diplomat_target_id, 0, "");
  }
}

/***********************************************************************//**
  Action investigate city for choice dialog
***************************************************************************/
static void spy_investigate(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (NULL != game_city_by_number(diplomat_target_id)
      && NULL != game_unit_by_number(diplomat_id)) {
    request_do_action(ACTION_SPY_INVESTIGATE_CITY, diplomat_id,
                      diplomat_target_id, 0, "");
  }
}

/***********************************************************************//**
  Action Investigate City Spend Unit for choice dialog
***************************************************************************/
static void diplomat_investigate(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (NULL != game_city_by_number(diplomat_target_id)
      && NULL != game_unit_by_number(diplomat_id)) {
    request_do_action(ACTION_INV_CITY_SPEND, diplomat_id,
                      diplomat_target_id, 0, "");
  }
}

/***********************************************************************//**
  Action sabotage for choice dialog
***************************************************************************/
static void diplomat_sabotage(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (NULL != game_unit_by_number(diplomat_id)
      && NULL != game_city_by_number(diplomat_target_id)) {
    request_do_action(ACTION_SPY_SABOTAGE_CITY, diplomat_id,
                      diplomat_target_id, B_LAST + 1, "");
  }
}

/***********************************************************************//**
  Action sabotage and escape for choice dialog
***************************************************************************/
static void diplomat_sabotage_esc(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (NULL != game_unit_by_number(diplomat_id)
      && NULL != game_city_by_number(diplomat_target_id)) {
    request_do_action(ACTION_SPY_SABOTAGE_CITY_ESC, diplomat_id,
                      diplomat_target_id, B_LAST + 1, "");
  }
}

/***********************************************************************//**
  Action steal with diplomat for choice dialog
***************************************************************************/
static void diplomat_steal(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (NULL != game_unit_by_number(diplomat_id)
      && NULL != game_city_by_number(diplomat_target_id)) {
    request_do_action(ACTION_SPY_STEAL_TECH, diplomat_id,
                      diplomat_target_id, A_UNSET, "");
  }
}

/***********************************************************************//**
  Action "Steal Tech Escape Expected" for choice dialog
***************************************************************************/
static void diplomat_steal_esc(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (NULL != game_unit_by_number(diplomat_id)
      && NULL != game_city_by_number(diplomat_target_id)) {
    request_do_action(ACTION_SPY_STEAL_TECH_ESC, diplomat_id,
                      diplomat_target_id, A_UNSET, "");
  }
}

/***********************************************************************//**
  Action incite revolt for choice dialog
***************************************************************************/
static void diplomat_incite(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (NULL != game_unit_by_number(diplomat_id)
      && NULL != game_city_by_number(diplomat_target_id)) {
    /* Wait for the server's reply before moving on to the next queued diplomat. */
    is_more_user_input_needed = TRUE;

    request_action_details(ACTION_SPY_INCITE_CITY, diplomat_id,
                           diplomat_target_id);
  }
}

/***********************************************************************//**
  Action incite revolt and escape for choice dialog
***************************************************************************/
static void diplomat_incite_escape(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (NULL != game_unit_by_number(diplomat_id)
      && NULL != game_city_by_number(diplomat_target_id)) {
    /* Wait for the server's reply before moving on to the next queued diplomat. */
    is_more_user_input_needed = TRUE;

    request_action_details(ACTION_SPY_INCITE_CITY_ESC, diplomat_id,
                           diplomat_target_id);
  }
}

/***********************************************************************//**
  Action keep moving with actor unit for choice dialog
***************************************************************************/
static void act_sel_keep_moving(QVariant data1, QVariant data2)
{
  struct unit *punit;
  struct tile *ptile;
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if ((punit = game_unit_by_number(diplomat_id))
      && (ptile = index_to_tile(&(wld.map), diplomat_target_id))
      && !same_pos(unit_tile(punit), ptile)) {
    request_unit_non_action_move(punit, ptile);
  }
}

/***********************************************************************//**
  Popup a window asking a diplomatic unit if it wishes to incite the
  given enemy city.
***************************************************************************/
void popup_incite_dialog(struct unit *actor, struct city *tcity, int cost,
                         const struct action *paction)
{
  char buf[1024];
  char buf2[1024];
  int ret;
  int diplomat_id = actor->id;
  int diplomat_target_id = tcity->id;

  /* Should be set before sending request to the server. */
  fc_assert(is_more_user_input_needed);

  fc_snprintf(buf, ARRAY_SIZE(buf), PL_("Treasury contains %d gold.",
                                        "Treasury contains %d gold.",
                                        client_player()->economic.gold),
              client_player()->economic.gold);

  if (INCITE_IMPOSSIBLE_COST == cost) {
    hud_message_box incite_impossible(gui()->central_wdg);

    fc_snprintf(buf2, ARRAY_SIZE(buf2),
                _("You can't incite a revolt in %s."), city_name_get(tcity));
    incite_impossible.set_text_title(QString(buf2), "!");
    incite_impossible.setStandardButtons(QMessageBox::Ok);
    incite_impossible.exec();
  } else if (cost <= client_player()->economic.gold) {
    hud_message_box ask(gui()->central_wdg);

    fc_snprintf(buf2, ARRAY_SIZE(buf2),
                PL_("Incite a revolt for %d gold?\n%s",
                    "Incite a revolt for %d gold?\n%s", cost), cost, buf);
    ask.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
    ask.setDefaultButton(QMessageBox::Cancel);
    ask.set_text_title(QString(buf2), _("Incite a Revolt!"));
    ret = ask.exec();
    switch (ret) {
    case QMessageBox::Cancel:
      break;
    case QMessageBox::Ok:
      request_do_action(paction->id, diplomat_id,
                        diplomat_target_id, 0, "");
      break;
    }
  } else {
    hud_message_box too_much(gui()->central_wdg);

    fc_snprintf(buf2, ARRAY_SIZE(buf2),
                PL_("Inciting a revolt costs %d gold.\n%s",
                    "Inciting a revolt costs %d gold.\n%s", cost), cost,
                buf);
    too_much.set_text_title(QString(buf2), "!");
    too_much.setStandardButtons(QMessageBox::Ok);
    too_much.exec();
  }

  diplomat_queue_handle_secondary(diplomat_id);
}

/***********************************************************************//**
  Popup a dialog asking a diplomatic unit if it wishes to bribe the
  given enemy unit.
***************************************************************************/
void popup_bribe_dialog(struct unit *actor, struct unit *tunit, int cost,
                        const struct action *paction)
{
  hud_message_box ask(gui()->central_wdg);
  int ret;
  char buf[1024];
  char buf2[1024];
  int diplomat_id = actor->id;
  int diplomat_target_id = tunit->id;

  /* Should be set before sending request to the server. */
  fc_assert(is_more_user_input_needed);

  fc_snprintf(buf, ARRAY_SIZE(buf), PL_("Treasury contains %d gold.",
                                        "Treasury contains %d gold.",
                                        client_player()->economic.gold),
              client_player()->economic.gold);

  if (cost <= client_player()->economic.gold) {
    fc_snprintf(buf2, ARRAY_SIZE(buf2), PL_("Bribe unit for %d gold?\n%s",
                                            "Bribe unit for %d gold?\n%s",
                                            cost), cost, buf);
    ask.set_text_title(QString(buf2), QString(_("Bribe Enemy Unit")));
    ask.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
    ask.setDefaultButton(QMessageBox::Cancel);
    ret = ask.exec();
    switch (ret) {
    case QMessageBox::Cancel:
      break;
    case QMessageBox::Ok:
      request_do_action(paction->id, diplomat_id,
                        diplomat_target_id, 0, "");
      break;
    default:
      break;
    }
  } else {
    fc_snprintf(buf2, ARRAY_SIZE(buf2),
                PL_("Bribing the unit costs %d gold.\n%s",
                    "Bribing the unit costs %d gold.\n%s", cost), cost, buf);
    ask.set_text_title(QString(buf2), _("Traitors Demand Too Much!"));
    ask.exec();
  }

  diplomat_queue_handle_secondary(diplomat_id);
}

/***********************************************************************//**
  Action pillage for choice dialog
***************************************************************************/
static void pillage_something(QVariant data1, QVariant data2)
{
  int punit_id;
  int what;
  struct unit *punit;
  struct extra_type *target;

  what = data1.toInt();
  punit_id = data2.toInt();
  punit = game_unit_by_number(punit_id);
  if (punit) {
    target = extra_by_number(what);
    request_new_unit_activity_targeted(punit, ACTIVITY_PILLAGE, target);
  }
  ::is_showing_pillage_dialog = false;
}

/***********************************************************************//**
  Action sabotage with spy for choice dialog
***************************************************************************/
static void spy_sabotage(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toList().at(0).toInt();
  int diplomat_target_id = data1.toList().at(1).toInt();
  action_id act_id = data1.toList().at(2).toInt();

  if (NULL != game_unit_by_number(diplomat_id)
      && NULL != game_city_by_number(diplomat_target_id)) {
    if (data2.toInt() == B_LAST) {
      /* This is the untargeted version. */
      request_do_action(get_non_targeted_action_id(act_id),
                        diplomat_id, diplomat_target_id, data2.toInt() + 1,
                        "");
    } else {
      /* This is the targeted version. */
      request_do_action(act_id, diplomat_id,
                        diplomat_target_id, data2.toInt() + 1, "");
    }
  }
}

/***********************************************************************//**
  Popup a dialog asking a diplomatic unit if it wishes to sabotage the
  given enemy city.
***************************************************************************/
void popup_sabotage_dialog(struct unit *actor, struct city *tcity,
                           const struct action *paction)
{
  QString str;
  QVariant qv1, qv2;
  int diplomat_id = actor->id;
  int diplomat_target_id = tcity->id;
  pfcn_void func;
  choice_dialog *cd = new choice_dialog(_("Sabotage"),
                                        _("Select Improvement to Sabotage"),
                                        gui()->game_tab_widget,
                                        diplomat_queue_handle_secondary);
  int nr = 0;
  struct astring stra = ASTRING_INIT;
  QList<QVariant> actor_and_target;

  /* Should be set before sending request to the server. */
  fc_assert(is_more_user_input_needed);

  /* Put both actor, target city and action in qv1 since qv2 is taken */
  actor_and_target.append(diplomat_id);
  actor_and_target.append(diplomat_target_id);
  actor_and_target.append(paction->id);
  qv1 = QVariant::fromValue(actor_and_target);

  func = spy_sabotage;
  cd->add_item(QString(_("City Production")), func, qv1, -1);
  city_built_iterate(tcity, pimprove) {
    if (pimprove->sabotage > 0) {
      func = spy_sabotage;
      str = city_improvement_name_translation(tcity, pimprove);
      qv2 = nr;
      cd->add_item(str, func, qv1, improvement_number(pimprove));
      nr++;
    }
  } city_built_iterate_end;

  if (action_prob_possible(actor->client.act_prob_cache[
                           get_non_targeted_action_id(paction->id)])) {
    astr_set(&stra, _("At %s's Discretion"),
             unit_name_translation(actor));
    func = spy_sabotage;
    str = astr_str(&stra);
    cd->add_item(str, func, qv1, B_LAST);
  }

  cd->set_layout();
  cd->show_me();
  astr_free(&stra);
}

/***********************************************************************//**
  Popup a dialog asking the unit which improvement they would like to
  pillage.
***************************************************************************/
void popup_pillage_dialog(struct unit *punit, bv_extras extras)
{
  QString str;
  QVariant qv1, qv2;
  pfcn_void func;
  choice_dialog *cd;
  struct extra_type *tgt;

  if (is_showing_pillage_dialog){
    return;
  }
  cd = new choice_dialog(_("What To Pillage"), _("Select what to pillage:"),
                         gui()->game_tab_widget);
  qv2 = punit->id;
  while ((tgt = get_preferred_pillage(extras))) {
    int what;

    what = extra_index(tgt);
    BV_CLR(extras, what);

    func = pillage_something;
    str = extra_name_translation(tgt);
    qv1 = what;
    cd->add_item(str, func, qv1, qv2);
  }
  cd->set_layout();
  cd->show_me();
}

/***********************************************************************//**
  Disband Message box contructor
***************************************************************************/
disband_box::disband_box(struct unit_list *punits,
                         QWidget *parent) : hud_message_box(parent)
{
  QString str;
  QPushButton *pb;

  setAttribute(Qt::WA_DeleteOnClose);
  setModal(false);
  cpunits = unit_list_new();
  unit_list_iterate(punits, punit) {
    unit_list_append(cpunits, punit);
  } unit_list_iterate_end;

  str = QString(PL_("Are you sure you want to disband that %1 unit?",
                  "Are you sure you want to disband those %1 units?",
                  unit_list_size(punits))).arg(unit_list_size(punits));
  pb = addButton(_("Yes"), QMessageBox::AcceptRole);
  addButton(_("No"), QMessageBox::RejectRole);
  set_text_title(str, _("Disband units"));
  setDefaultButton(pb);
  connect(pb, &QAbstractButton::clicked, this, &disband_box::disband_clicked);
}

/***********************************************************************//**
  Clicked Yes in disband box
***************************************************************************/
void disband_box::disband_clicked()
{
  unit_list_iterate(cpunits, punit) {
    if (unit_can_do_action(punit, ACTION_DISBAND_UNIT)) {
      request_unit_disband(punit);
    }
  } unit_list_iterate_end;
}

/***********************************************************************//**
  Destructor for disband box
***************************************************************************/
disband_box::~disband_box()
{
  unit_list_destroy(cpunits);
}

/***********************************************************************//**
  Pops up a dialog to confirm disband of the unit(s).
***************************************************************************/
void popup_disband_dialog(struct unit_list *punits)
{
  disband_box *ask = new disband_box(punits, gui()->central_wdg);
  ask->show();
}

/***********************************************************************//**
  Ruleset (modpack) has suggested loading certain tileset. Confirm from
  user and load.
***************************************************************************/
void popup_tileset_suggestion_dialog(void)
{
  hud_message_box ask(gui()->central_wdg);
  QString text;
  QString title;
  QPushButton *ok_button;

  title = QString(_("Modpack suggests using %1 tileset."))
          .arg(game.control.preferred_tileset);
  text = QString("It might not work with other tilesets.\n"
                 "You are currently using tileset %1.")
         .arg(tileset_basename(tileset));
  ask.addButton(_("Keep current tileset"), QMessageBox::ActionRole);
  ok_button = ask.addButton(_("Load tileset"), QMessageBox::ActionRole);
  ask.set_text_title(text, title);
  ask.exec();
  if (ask.clickedButton() == ok_button) {
    sz_strlcpy(forced_tileset_name, game.control.preferred_tileset);
    if (!tilespec_reread(game.control.preferred_tileset, FALSE, gui()->map_scale)) {
      tileset_error(LOG_ERROR, _("Can't load requested tileset %s."),
                    game.control.preferred_tileset);
    }
  }
}

/***********************************************************************//**
  Ruleset (modpack) has suggested loading certain soundset. Confirm from
  user and load.
***************************************************************************/
void popup_soundset_suggestion_dialog(void)
{
  hud_message_box ask(gui()->central_wdg);
  QString text;
  QString title;
  QPushButton *ok_button;

  title = QString(_("Modpack suggests using %1 soundset."))
          .arg(game.control.preferred_soundset);
  text = QString("It might not work with other tilesets.\n"
                 "You are currently using soundset %1.")
         .arg(sound_set_name);
  ask.addButton(_("Keep current soundset"), QMessageBox::ActionRole);
  ok_button = ask.addButton(_("Load soundset"), QMessageBox::ActionRole);
  ask.set_text_title(text, title);
  ask.exec();
  if (ask.clickedButton() == ok_button) {
    audio_restart(game.control.preferred_soundset, music_set_name);
  }
}

/***********************************************************************//**
  Ruleset (modpack) has suggested loading certain musicset. Confirm from
  user and load.
***************************************************************************/
void popup_musicset_suggestion_dialog(void)
{
  hud_message_box ask(gui()->central_wdg);
  QString text;
  QString title;
  QPushButton *ok_button;

  title = QString(_("Modpack suggests using %1 musicset."))
          .arg(game.control.preferred_musicset);
  text = QString("It might not work with other tilesets.\n"
                 "You are currently using musicset %1.")
         .arg(music_set_name);
  ask.addButton(_("Keep current musicset"), QMessageBox::ActionRole);
  ok_button = ask.addButton(_("Load musicset"), QMessageBox::ActionRole);
  ask.set_text_title(text, title);
  ask.exec();
  if (ask.clickedButton() == ok_button) {
    audio_restart(sound_set_name, game.control.preferred_musicset);
  }
}

/***********************************************************************//**
  Tileset (modpack) has suggested loading certain theme. Confirm from
  user and load.
***************************************************************************/
bool popup_theme_suggestion_dialog(const char *theme_name)
{
  /* PORTME */
  return false;
}

/***********************************************************************//**
  Restarts all notify dialogs
***************************************************************************/
void restart_notify_dialogs()
{
  QList<notify_dialog *> nd_list;
  int i;

  nd_list = gui()->mapview_wdg->findChildren<notify_dialog *>();
  for (i = 0; i < nd_list.count(); i++) {
    nd_list[i]->restart();
    delete nd_list[i];
  }
}

/***********************************************************************//**
  This function is called when the client disconnects or the game is
  over.  It should close all dialog windows for that game.
***************************************************************************/
void popdown_all_game_dialogs(void)
{
  int i;
  QList <choice_dialog *> cd_list;
  QList <notify_dialog *> nd_list;

  QApplication::alert(gui()->central_wdg);
  cd_list = gui()->game_tab_widget->findChildren <choice_dialog *>();
  for (i = 0; i < cd_list.count(); i++) {
      cd_list[i]->close();
  }
  nd_list = gui()->game_tab_widget->findChildren <notify_dialog *>();
  for (i = 0; i < nd_list.count(); i++) {
      nd_list[i]->close();
  }

  popdown_help_dialog();
  popdown_players_report();
  popdown_all_spaceships_dialogs();
  popdown_economy_report();
  popdown_units_report();
  popdown_science_report();
  popdown_city_report();
  popdown_endgame_report();
  gui()->popdown_unit_sel();
}

/***********************************************************************//**
  Returns the id of the actor unit currently handled in action selection
  dialog when the action selection dialog is open.
  Returns IDENTITY_NUMBER_ZERO if no action selection dialog is open.
***************************************************************************/
int action_selection_actor_unit(void)
{
  choice_dialog *cd = gui()->get_diplo_dialog();

  if (cd != NULL) {
    return cd->unit_id;
  } else {
    return IDENTITY_NUMBER_ZERO;
  }
}

/***********************************************************************//**
  Returns id of the target city of the actions currently handled in action
  selection dialog when the action selection dialog is open and it has a
  city target. Returns IDENTITY_NUMBER_ZERO if no action selection dialog
  is open or no city target is present in the action selection dialog.
***************************************************************************/
int action_selection_target_city(void)
{
  choice_dialog *cd = gui()->get_diplo_dialog();

  if (cd != NULL) {
    return cd->target_id[ATK_CITY];
  } else {
    return IDENTITY_NUMBER_ZERO;
  }
}

/***********************************************************************//**
  Returns id of the target tile of the actions currently handled in action
  selection dialog when the action selection dialog is open and it has a
  tile target. Returns TILE_INDEX_NONE if no action selection dialog is
  open.
***************************************************************************/
int action_selection_target_tile(void)
{
  choice_dialog *cd = gui()->get_diplo_dialog();

  if (cd != NULL) {
    return cd->target_id[ATK_TILE];
  } else {
    return TILE_INDEX_NONE;
  }
}

/**********************************************************************//**
  Returns id of the target extra of the actions currently handled in action
  selection dialog when the action selection dialog is open and it has an
  extra target. Returns EXTRA_NONE if no action selection dialog is open
  or no extra target is present in the action selection dialog.
**************************************************************************/
int action_selection_target_extra(void)
{
  choice_dialog *cd = gui()->get_diplo_dialog();

  if (cd != NULL) {
    return cd->target_extra_id;
  } else {
    return EXTRA_NONE;
  }
}

/***********************************************************************//**
  Returns id of the target unit of the actions currently handled in action
  selection dialog when the action selection dialog is open and it has a
  unit target. Returns IDENTITY_NUMBER_ZERO if no action selection dialog
  is open or no unit target is present in the action selection dialog.
***************************************************************************/
int action_selection_target_unit(void)
{
  choice_dialog *cd = gui()->get_diplo_dialog();

  if (cd != NULL) {
    return cd->target_id[ATK_UNIT];
  } else {
    return IDENTITY_NUMBER_ZERO;
  }
}

/***********************************************************************//**
  Updates the action selection dialog with new information.
***************************************************************************/
void action_selection_refresh(struct unit *actor_unit,
                              struct city *target_city,
                              struct unit *target_unit,
                              struct tile *target_tile,
                              struct extra_type *target_extra,
                              const struct act_prob *act_probs)
{
  choice_dialog *asd;
  Choice_dialog_button *keep_moving_button;
  Choice_dialog_button *wait_button;
  Choice_dialog_button *cancel_button;
  QVariant qv1, qv2;

  asd = gui()->get_diplo_dialog();
  if (asd == NULL) {
    fc_assert_msg(asd != NULL,
                  "The action selection dialog should have been open");
    return;
  }

  if (actor_unit->id != action_selection_actor_unit()) {
    fc_assert_msg(actor_unit->id == action_selection_actor_unit(),
                  "The action selection dialog is for another actor unit.");
  }

  /* Put the actor id in qv1. */
  qv1 = actor_unit->id;

  cancel_button = asd->get_identified_button(BUTTON_CANCEL);
  if (cancel_button != NULL) {
    /* Temporary remove the Cancel button so it won't end up above
     * any added buttons. */
    asd->stack_button(cancel_button);
  }

  wait_button = asd->get_identified_button(BUTTON_WAIT);
  if (wait_button != NULL) {
    /* Temporary remove the Wait button so it won't end up above
     * any added buttons. */
    asd->stack_button(wait_button);
  }

  keep_moving_button = asd->get_identified_button(BUTTON_MOVE);
  if (keep_moving_button != NULL) {
    /* Temporary remove the Keep moving button so it won't end up above
     * any added buttons. */
    asd->stack_button(keep_moving_button);
  }

  action_iterate(act) {
    QString custom;

    if (action_id_get_actor_kind(act) != AAK_UNIT) {
      /* Not relevant. */
      continue;
    }

    if (action_prob_possible(act_probs[act])
        && act == ACTION_HELP_WONDER) {
      /* Add information about how far along the wonder is. */
      custom = city_prod_remaining(target_city);
    } else {
      custom = "";
    }

    /* Put the target id in qv2. */
    switch (action_id_get_target_kind(act)) {
    case ATK_UNIT:
      if (target_unit != NULL) {
        qv2 = target_unit->id;
      } else {
        fc_assert_msg(!action_prob_possible(act_probs[act])
                      || target_unit != NULL,
                      "Action enabled against non existing unit!");

        qv2 = IDENTITY_NUMBER_ZERO;
      }
      break;
    case ATK_CITY:
      if (target_city != NULL) {
        qv2 = target_city->id;
      } else {
        fc_assert_msg(!action_prob_possible(act_probs[act])
                      || target_city != NULL,
                      "Action enabled against non existing city!");

        qv2 = IDENTITY_NUMBER_ZERO;
      }
      break;
    case ATK_TILE:
    case ATK_UNITS:
      if (target_tile != NULL) {
        qv2 = tile_index(target_tile);
      } else {
        fc_assert_msg(!action_prob_possible(act_probs[act])
                      || target_tile != NULL,
                      "Action enabled against all units on "
                      "non existing tile!");

        qv2 = TILE_INDEX_NONE;
      }
      break;
    case ATK_SELF:
      qv2 = actor_unit->id;
      break;
    case ATK_COUNT:
      fc_assert_msg(ATK_COUNT != action_id_get_target_kind(act),
                    "Bad target kind");
      continue;
    }

    if (asd->get_identified_button(act)) {
      /* Update the existing button. */
      action_entry_update(asd->get_identified_button(act),
                          act, act_probs, custom,
                          qv1, qv2);
    } else {
      /* Add the button (unless its probability is 0). */
      action_entry(asd, act, act_probs, custom,
                   qv1, qv2);
    }
  } action_iterate_end;

  if (keep_moving_button != NULL
      || wait_button != NULL || cancel_button != NULL) {
    /* Reinsert the "Keep moving" button below any potential
     * buttons recently added. */
    asd->unstack_all_buttons();
  }
}

/***********************************************************************//**
  Closes the action selection dialog
***************************************************************************/
void action_selection_close(void)
{
  choice_dialog *cd;

  cd = gui()->get_diplo_dialog();
  if (cd != NULL){
    did_not_decide = true;
    cd->close();
  }
}

/***********************************************************************//**
  Player has gained a new tech.
***************************************************************************/
void show_tech_gained_dialog(Tech_type_id tech)
{
}

/***********************************************************************//**
  Show tileset error dialog.
***************************************************************************/
void show_tileset_error(const char *msg)
{
  char buf[1024];

  fc_snprintf(buf, sizeof(buf),
              _("Tileset problem, it's probably incompatible with the"
              " ruleset:\n%s"), msg);

  if (QCoreApplication::instance() != nullptr) {
    QMessageBox ask(gui()->central_wdg);

    ask.setText(buf);
    ask.setStandardButtons(QMessageBox::Ok);
    ask.setWindowTitle(_("Tileset error"));
    ask.exec();
  }
}

/***********************************************************************//**
  Popup dialog for upgrade units
***************************************************************************/
void popup_upgrade_dialog(struct unit_list *punits)
{
  char buf[512];
  hud_message_box ask(gui()->central_wdg);
  int ret;
  QString title;

  if (!punits || unit_list_size(punits) == 0) {
    return;
  }
  if (!get_units_upgrade_info(buf, sizeof(buf), punits)) {
    title = _("Upgrade Unit!");
    ask.setStandardButtons(QMessageBox::Ok);
  } else {
    title = _("Upgrade Obsolete Units");
    ask.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
    ask.setDefaultButton(QMessageBox::Cancel);
  }
  ask.set_text_title(QString(buf), title);
  ret = ask.exec();

  switch (ret) {
  case QMessageBox::Cancel:
    return;
    break;
  case QMessageBox::Ok:
    unit_list_iterate(punits, punit) {
      request_unit_upgrade(punit);
    }
    unit_list_iterate_end;
    break;
  }
}

/***********************************************************************//**
  Contructor for units_select
***************************************************************************/
units_select::units_select(tile *ptile, QWidget *parent)
{
  QPoint p, final_p;

  setParent(parent);
  utile = ptile;
  pix = NULL;
  show_line = 0;
  highligh_num = -1;
  ufont.setItalic(true);
  info_font = *fc_font::instance()->get_font(fonts::notify_label);
  update_units();
  h_pix = NULL;
  create_pixmap();
  p = mapFromGlobal(QCursor::pos());
  cw = new close_widget(this);
  setMouseTracking(true);
  final_p.setX(p.x());
  final_p.setY(p.y());
  if (p.x() + width() > parentWidget()->width()) {
    final_p.setX(parentWidget()->width() - width());
  }
  if (p.y() - height() < 0) {
    final_p.setY(height());
  }
  move(final_p.x(), final_p.y() - height());
  setFocus();
  /* Build fails with qt5 connect style for static functions
   * Qt5.2 so dont update */
  QTimer::singleShot(10, this, SLOT(update_img()));
}

/***********************************************************************//**
  Destructor for unit select
***************************************************************************/
units_select::~units_select()
{
    delete h_pix;
    delete pix;
    delete cw;
}

/***********************************************************************//**
  Create pixmap of whole widget except borders (pix)
***************************************************************************/
void units_select::create_pixmap()
{
  int a;
  int rate, f;
  int x, y, i;
  QFontMetrics fm(info_font);
  QImage cropped_img;
  QImage img;
  QList <QPixmap*>pix_list;
  QPainter p;
  QPen pen;
  QPixmap pixc;
  QPixmap *pixp;
  QPixmap *tmp_pix;
  QRect crop;
  QString str;
  struct canvas *unit_pixmap;
  struct unit *punit;
  float isosize;

  if (pix != NULL) {
    delete pix;
    pix = NULL;
  };
  isosize = 0.7;
  if (tileset_hex_height(tileset) > 0 || tileset_hex_width(tileset) > 0) {
    isosize = 0.5;
  }

  update_units();
  if (unit_list.count() > 0) {
  if (tileset_is_isometric(tileset) == false) {
    item_size.setWidth(tileset_unit_width(tileset));
    item_size.setHeight(tileset_unit_width(tileset));
  } else {
    item_size.setWidth(tileset_unit_width(tileset) * isosize);
    item_size.setHeight(tileset_unit_width(tileset) * isosize);
  }
  more = false;
  if (h_pix != nullptr) {
    delete h_pix;
  }
  h_pix = new QPixmap(item_size.width(), item_size.height());
  h_pix->fill(palette().color(QPalette::HighlightedText));
  if (unit_count < 5) {
    row_count = 1;
    pix = new QPixmap((unit_list.size()) * item_size.width(),
                      item_size.height());
  } else if (unit_count < 9) {
    row_count = 2;
    pix = new QPixmap(4 * item_size.width(), 2 * item_size.height());
  } else {
    row_count = 3;
    if (unit_count > 12) {
      more = true;
    }
    pix = new QPixmap(4 * item_size.width(), 3 * item_size.height());
  }
  pix->fill(Qt::transparent);
  foreach(punit, unit_list) {
    unit_pixmap = qtg_canvas_create(tileset_unit_width(tileset),
                                    tileset_unit_height(tileset));
    unit_pixmap->map_pixmap.fill(Qt::transparent);
    put_unit(punit, unit_pixmap, 1.0, 0, 0);
    img = unit_pixmap->map_pixmap.toImage();
    crop = zealous_crop_rect(img);
    cropped_img = img.copy(crop);
    if (tileset_is_isometric(tileset) == false) {
      img = cropped_img.scaled(tileset_unit_width(tileset),
                               tileset_unit_width(tileset),
                               Qt::KeepAspectRatio,
                               Qt::SmoothTransformation);
    } else {
      img = cropped_img.scaled(tileset_unit_width(tileset) * isosize,
                               tileset_unit_width(tileset) * isosize,
                               Qt::KeepAspectRatio,
                               Qt::SmoothTransformation);
    }
    pixc = QPixmap::fromImage(img);
    pixp = new QPixmap(pixc);
    pix_list.push_back(pixp);
    qtg_canvas_free(unit_pixmap);
  }
  a = qMin(item_size.width() / 4, 12);
  x = 0, y = -item_size.height(), i = -1;
  p.begin(pix);
  ufont.setPixelSize(a);
  p.setFont(ufont);
  pen.setColor(palette().color(QPalette::Text));
  p.setPen(pen);

  while (!pix_list.isEmpty()) {
    tmp_pix = pix_list.takeFirst();
    i++;
    if (i % 4 == 0) {
      x = 0;
      y = y + item_size.height();
    }
    punit = unit_list.at(i);
    Q_ASSERT(punit != NULL);
    rate = unit_type_get(punit)->move_rate;
    f = ((punit->fuel) - 1);
    if (i == highligh_num) {
      p.drawPixmap(x, y, *h_pix);
      p.drawPixmap(x, y, *tmp_pix);
    } else {
      p.drawPixmap(x, y, *tmp_pix);
    }
    str = QString(move_points_text(punit->moves_left, false));
    if (utype_fuel(unit_type_get(punit))) {
      str = str + "(" + QString(move_points_text((rate * f)
            + punit->moves_left, false)) + ")";
    }
    /* TRANS: MP = Movement points */
    str = QString(_("MP:")) + str;
    p.drawText(x, y + item_size.height() - 4, str);

    x = x + item_size.width();
    delete tmp_pix;
  }
  p.end();
  setFixedWidth(pix->width() + 20);
  setFixedHeight(pix->height() + 2 * (fm.height() + 6));
  qDeleteAll(pix_list.begin(), pix_list.end());
  }
}

/***********************************************************************//**
  Event for mouse moving around units_select
***************************************************************************/
void units_select::mouseMoveEvent(QMouseEvent *event)
{
  int a, b;
  int old_h;
  QFontMetrics fm(info_font);

  old_h = highligh_num;
  highligh_num = -1;
  if (event->x() > width() - 11
      || event->y() > height() - fm.height() - 5
      || event->y() < fm.height() + 3 || event->x() < 11) {
    /** do nothing if mouse is on border, just skip next if */
  } else if (row_count > 0) {
    a = (event->x() - 10) / item_size.width();
    b = (event->y() - fm.height() - 3) / item_size.height();
    highligh_num = b * 4 + a;
  }
  if (old_h != highligh_num) {
    create_pixmap();
    update();
  }
}

/***********************************************************************//**
  Mouse pressed event for units_select.
  Left Button - chooses units
  Right Button - closes widget
***************************************************************************/
void units_select::mousePressEvent(QMouseEvent *event)
{
  struct unit *punit;
  if (event->button() == Qt::RightButton) {
    was_destroyed = true;
    close();
    destroy();
  }
  if (event->button() == Qt::LeftButton && highligh_num != -1) {
    update_units();
    if (highligh_num >= unit_list.count()) {
      return;
    }
    punit = unit_list.at(highligh_num);
    unit_focus_set(punit);
    was_destroyed = true;
    close();
    destroy();
  }
}

/***********************************************************************//**
  Update image, because in constructor theme colors
  are uninitialized in QPainter
***************************************************************************/
void units_select::update_img()
{
  create_pixmap();
  update();
}

/***********************************************************************//**
  Redirected paint event
***************************************************************************/
void units_select::paint(QPainter *painter, QPaintEvent *event)
{
  QFontMetrics fm(info_font);
  int h, i;
  int *f_size;
  QPen pen;
  QString str, str2;
  struct unit *punit;
  int point_size = info_font.pointSize();
  int pixel_size = info_font.pixelSize();

  if (point_size < 0) {
    f_size = &pixel_size;
  } else {
    f_size = &point_size;
  }
  if (highligh_num != -1 && highligh_num < unit_list.count()) {
    punit = unit_list.at(highligh_num);
    /* TRANS: HP - hit points */
    str2 = QString(_("%1 HP:%2/%3")).arg(QString(unit_activity_text(punit)),
                                      QString::number(punit->hp),
                                      QString::number(unit_type_get(punit)->hp));
  }
  str = QString(PL_("%1 unit", "%1 units",
                    unit_list_size(utile->units)))
                .arg(unit_list_size(utile->units));
  for (i = *f_size; i > 4; i--) {
    if (point_size < 0) {
      info_font.setPixelSize(i);
    } else  {
      info_font.setPointSize(i);
    }
    QFontMetrics qfm(info_font);
    if (10 + qfm.width(str2) < width()) {
      break;
    }
  }
  h = fm.height();
  if (pix != NULL) {
    painter->drawPixmap(10, h + 3, *pix);
    pen.setColor(palette().color(QPalette::Text));
    painter->setPen(pen);
    painter->setFont(info_font);
    painter->drawText(10, h, str);
    if (highligh_num != -1 && highligh_num < unit_list.count()) {
      painter->drawText(10, height() - 5, str2);
    }
    /* draw scroll */
    if (more == true) {
      int maxl = ((unit_count - 1) / 4) + 1;
      float page_height = 3.0f / maxl;
      float page_start = (static_cast<float>(show_line)) / maxl;
      pen.setColor(palette().color(QPalette::HighlightedText));
      painter->setBrush(palette().color(QPalette::HighlightedText).darker());
      painter->setPen(palette().color(QPalette::HighlightedText).darker());
      painter->drawRect(pix->width() + 10, h, 8, h + pix->height());
      painter->setPen(pen);
      painter->drawRoundedRect(pix->width() + 10,
                               h + page_start * pix->height(),
                               8, h + page_height * pix->height(), 2, 2);
    }
  }
  if (point_size < 0) {
    info_font.setPixelSize(*f_size);
  } else {
    info_font.setPointSize(*f_size);
  }
  cw->put_to_corner();
}

/***********************************************************************//**
  Paint event, redirects to paint(...)
***************************************************************************/
void units_select::paintEvent(QPaintEvent *event)
{
  QPainter painter;

  painter.begin(this);
  paint(&painter, event);
  painter.end();
}

/***********************************************************************//**
  Function from abstract fcwidget to update menu, its not needed
  cause widget is easy closable via right mouse click
***************************************************************************/
void units_select::update_menu()
{
  was_destroyed = true;
  close();
  destroy();
}

/***********************************************************************//**
  Updates unit list on tile
***************************************************************************/
void units_select::update_units()
{
  int i = 1;
  struct unit_list *punit_list;

  unit_count = 0;
  if (utile == NULL) {
    struct unit *punit = head_of_units_in_focus();
    if (punit) {
      utile = unit_tile(punit);
    }
  }
  unit_list.clear();
  if (utile != nullptr) {
    punit_list = utile->units;
    if (punit_list != nullptr) {
      unit_list_iterate(utile->units, punit) {
        unit_count++;
        if (i > show_line * 4)
          unit_list.push_back(punit);
        i++;
      } unit_list_iterate_end;
    }
  }
  if (unit_list.count() == 0) {
    close();
  }
}

/***********************************************************************//**
  Close event for units_select, restores focus to map
***************************************************************************/
void units_select::closeEvent(QCloseEvent* event)
{
  gui()->mapview_wdg->setFocus();
  QWidget::closeEvent(event);
}

/***********************************************************************//**
  Mouse wheel event for units_select
***************************************************************************/
void units_select::wheelEvent(QWheelEvent *event)
{
  int nr;

  if (more == false && utile == NULL) {
    return;
  }
  nr = qCeil(static_cast<qreal>(unit_list_size(utile->units)) / 4) - 3;
  if (event->delta() < 0) {
    show_line++;
    show_line = qMin(show_line, nr);
  } else {
    show_line--;
    show_line = qMax(0, show_line);
  }
  update_units();
  create_pixmap();
  update();
  event->accept();
}

/***********************************************************************//**
  Keyboard handler for units_select
***************************************************************************/
void units_select::keyPressEvent(QKeyEvent *event)
{
  if (event->key() == Qt::Key_Escape) {
    was_destroyed = true;
    close();
    destroy();
  }
  QWidget::keyPressEvent(event);
}

/***********************************************************************//**
  Set current diplo dialog
***************************************************************************/
void fc_client::set_diplo_dialog(choice_dialog *widget)
{
  opened_dialog = widget;
}

/***********************************************************************//**
  Get current diplo dialog
***************************************************************************/
choice_dialog *fc_client::get_diplo_dialog()
{
  return opened_dialog;
}

/***********************************************************************//**
  Give a warning when user is about to edit scenario with manually
  set properties.
***************************************************************************/
bool qtg_handmade_scenario_warning()
{
  /* Just tell the client common code to handle this. */
  return false;
}

/***********************************************************************//**
  Unit wants to get into some transport on given tile.
***************************************************************************/
bool qtg_request_transport(struct unit *pcargo, struct tile *ptile)
{
  int tcount;
  hud_unit_loader *hul;
  struct unit_list *potential_transports = unit_list_new();
  struct unit *best_transport = transporter_for_unit_at(pcargo, ptile);

  unit_list_iterate(ptile->units, ptransport) {
    if (can_unit_transport(ptransport, pcargo)
        && get_transporter_occupancy(ptransport) < get_transporter_capacity(ptransport)) {
      unit_list_append(potential_transports, ptransport);
    }
  } unit_list_iterate_end;

  tcount = unit_list_size(potential_transports);

  if (tcount == 0) {
    fc_assert(best_transport == NULL);
    unit_list_destroy(potential_transports);

    return false; /* Unit was not handled here. */
  } else if (tcount == 1) {
    /* There's exactly one potential transport - use it automatically */
    fc_assert(unit_list_get(potential_transports, 0) == best_transport);
    request_unit_load(pcargo, unit_list_get(potential_transports, 0), ptile);

    unit_list_destroy(potential_transports);

    return true;
  }

  hul = new hud_unit_loader(pcargo, ptile);
  hul->show_me();
  return true;
}

/***********************************************************************//**
  Popup detailed information about battle or save information for
  some kind of statistics
***************************************************************************/
void qtg_popup_combat_info(int attacker_unit_id, int defender_unit_id,
                           int attacker_hp, int defender_hp,
                           bool make_att_veteran, bool make_def_veteran)
{
  if (gui()->qt_settings.show_battle_log == true) {
    hud_unit_combat* huc = new hud_unit_combat(attacker_unit_id,
                                               defender_unit_id,
                                               attacker_hp, defender_hp,
                                               make_att_veteran,
                                               make_def_veteran,
                                               gui()->battlelog_wdg->scale,
                                               gui()->battlelog_wdg);

    gui()->battlelog_wdg->add_combat_info(huc);
    gui()->battlelog_wdg->show();
  }
}

/***********************************************************************//**
  Popup dialog showing given image and text,
  start playing given sound, stop playing sound when popup is closed.
  Take all space available to show image if fullsize is set.
  If there are other the same popups show them in queue.
***************************************************************************/
void show_img_play_snd(const char *img_path, const char *snd_path,
                       const char *desc, bool fullsize)
{
  QPixmap *pix = new QPixmap;
  QString img, snd;
  hud_img *hi;
  QDir dir;

  img = dir.absolutePath() + QString("/data/") + QString(img_path);
  pix->load(img);
  if (pix->isNull()) {
    delete pix;
    return;
  }
  snd = dir.absolutePath() + QString("/data/") + QString(snd_path);
  hi = new hud_img(pix, snd, desc, fullsize,  gui()->mapview_wdg);
  hi->init();

}
