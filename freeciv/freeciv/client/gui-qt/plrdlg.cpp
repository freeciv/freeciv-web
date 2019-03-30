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
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QSplitter>
#include <QVBoxLayout>

// gui-qt
#include "fc_client.h"
#include "plrdlg.h"

/**********************************************************************//**
  Help function to draw checkbox inside delegate
**************************************************************************/
static QRect check_box_rect(const QStyleOptionViewItem
                            &view_item_style_options)
{
  QStyleOptionButton cbso;
  QRect check_box_rect = QApplication::style()->subElementRect(
                           QStyle::SE_CheckBoxIndicator, &cbso);
  QPoint check_box_point(view_item_style_options.rect.x() +
                         view_item_style_options.rect.width() / 2 -
                         check_box_rect.width() / 2,
                         view_item_style_options.rect.y() +
                         view_item_style_options.rect.height() / 2 -
                         check_box_rect.height() / 2);
  return QRect(check_box_point, check_box_rect.size());
}

/**********************************************************************//**
  Slighty increase deafult cell height
**************************************************************************/
QSize plr_item_delegate::sizeHint(const QStyleOptionViewItem &option,
                                  const QModelIndex &index) const
{
  QSize r;

  r =  QItemDelegate::sizeHint(option, index);
  r.setHeight(r.height() + 4);
  return r;
}

/**********************************************************************//**
  Paint evenet for custom player item delegation
**************************************************************************/
void plr_item_delegate::paint(QPainter *painter, const QStyleOptionViewItem
                              &option, const QModelIndex &index) const
{
  QStyleOptionButton but;
  QStyleOptionButton cbso;
  bool b;
  QString str;
  QRect rct;
  QPixmap pix(16, 16);

  QStyleOptionViewItem opt = QItemDelegate::setOptions(index, option);
  painter->save();
  switch (player_dlg_columns[index.column()].type) {
  case COL_FLAG:
    QItemDelegate::drawBackground(painter, opt, index);
    QItemDelegate::drawDecoration(painter, opt, option.rect,
                                  index.data().value<QPixmap>());
    break;
  case COL_COLOR:
    pix.fill(index.data().value <QColor> ());
    QItemDelegate::drawBackground(painter, opt, index);
    QItemDelegate::drawDecoration(painter, opt, option.rect, pix);
    break;
  case COL_BOOLEAN:
    b = index.data().toBool();
    QItemDelegate::drawBackground(painter, opt, index);
    cbso.state |= QStyle::State_Enabled;
    if (b) {
      cbso.state |= QStyle::State_On;
    } else {
      cbso.state |= QStyle::State_Off;
    }
    cbso.rect = check_box_rect(option);

    QApplication::style()->drawControl(QStyle::CE_CheckBox, &cbso, painter);
    break;
  case COL_TEXT:
    QItemDelegate::paint(painter, option, index);
    break;
  case COL_RIGHT_TEXT:
    QItemDelegate::drawBackground(painter, opt, index);
    opt.displayAlignment = Qt::AlignRight;
    rct = option.rect;
    rct.setTop((rct.top() + rct.bottom()) / 2
               - opt.fontMetrics.height() / 2);
    rct.setBottom((rct.top()+rct.bottom()) / 2
                  + opt.fontMetrics.height() / 2);
    if (index.data().toInt() == -1){
      str = "?";
    } else {
      str = index.data().toString();
    }
    QItemDelegate::drawDisplay(painter, opt, rct, str);
    break;
  default:
    QItemDelegate::paint(painter, option, index);
  }
  painter->restore();
}

/**********************************************************************//**
  Constructor for plr_item
**************************************************************************/
plr_item::plr_item(struct player *pplayer): QObject()
{
  ipplayer = pplayer;
}

/**********************************************************************//**
  Sets data for plr_item (not used)
**************************************************************************/
bool plr_item::setData(int column, const QVariant &value, int role)
{
  return false;
}

/**********************************************************************//**
  Returns data from item
**************************************************************************/
QVariant plr_item::data(int column, int role) const
{
  QFont f;
  QFontMetrics *fm;
  QPixmap *pix;
  QString str;
  struct player_dlg_column *pdc;

  if (role == Qt::UserRole) {
    return QVariant::fromValue((void *)ipplayer);
  }
  if (role != Qt::DisplayRole) {
    return QVariant();
  }
  pdc = &player_dlg_columns[column];
  switch (player_dlg_columns[column].type) {
  case COL_FLAG:
    pix = get_nation_flag_sprite(tileset, nation_of_player(ipplayer))->pm;
    f = *fc_font::instance()->get_font(fonts::default_font);
    fm = new QFontMetrics(f);
    *pix = pix->scaledToHeight(fm->height());
    delete fm;
    return *pix;
    break;
  case COL_COLOR:
    return get_player_color(tileset, ipplayer)->qcolor;
    break;
  case COL_BOOLEAN:
    return pdc->bool_func(ipplayer);
    break;
  case COL_TEXT:
    return pdc->func(ipplayer);
    break;
  case COL_RIGHT_TEXT:
    str = pdc->func(ipplayer);
    if (str.toInt() != 0){
      return str.toInt();
    } else if (str == "?") {
      return -1;
    }
    return str;
  default:
    return QVariant();
  }
}

/**********************************************************************//**
  Constructor for player model
**************************************************************************/
plr_model::plr_model(QObject *parent): QAbstractListModel(parent)
{
  populate();
}

/**********************************************************************//**
  Destructor for player model
**************************************************************************/
plr_model::~plr_model()
{
  qDeleteAll(plr_list);
  plr_list.clear();
}

/**********************************************************************//**
  Returns data from model
**************************************************************************/
QVariant plr_model::data(const QModelIndex &index, int role) const
{
  if (!index.isValid()) return QVariant();
  if (index.row() >= 0 && index.row() < rowCount() && index.column() >= 0
      && index.column() < columnCount())
    return plr_list[index.row()]->data(index.column(), role);
  return QVariant();
}

/**********************************************************************//**
  Returns header data from model
**************************************************************************/
QVariant plr_model::headerData(int section, Qt::Orientation orientation, 
                               int role) const
{
  struct player_dlg_column *pcol;
  if (orientation == Qt::Horizontal && section < num_player_dlg_columns) {
    if (role == Qt::DisplayRole) {
      pcol = &player_dlg_columns[section];
      return pcol->title;
    }
  }
  return QVariant();
}

/**********************************************************************//**
  Sets data in model
**************************************************************************/
bool plr_model::setData(const QModelIndex &index, const QVariant &value, 
                        int role)
{
  if (!index.isValid() || role != Qt::DisplayRole) return false;
  if (index.row() >= 0 && index.row() < rowCount() && index.column() >= 0 
    && index.column() < columnCount()) {
    bool change = plr_list[index.row()]->setData(index.column(), value, role);
    if (change) {
      notify_plr_changed(index.row());
    }
    return change;
  }
  return false;
}

/**********************************************************************//**
  Notifies that row has been changed
**************************************************************************/
void plr_model::notify_plr_changed(int row)
{
  emit dataChanged(index(row, 0), index(row, columnCount() - 1));
}

/**********************************************************************//**
  Fills model with data
**************************************************************************/
void plr_model::populate()
{
  plr_item *pi;

  qDeleteAll(plr_list);
  plr_list.clear();
  beginResetModel();
  players_iterate(pplayer) {
    if ((is_barbarian(pplayer))){
      continue;
    }
    pi = new plr_item(pplayer);
    plr_list << pi;
  } players_iterate_end;
  endResetModel();
}

/**********************************************************************//**
  Constructor for plr_widget
**************************************************************************/
plr_widget::plr_widget(plr_report *pr): QTreeView()
{
  plr = pr;
  other_player = NULL;
  selected_player = nullptr;
  pid = new plr_item_delegate(this);
  setItemDelegate(pid);
  list_model = new plr_model(this);
  filter_model = new QSortFilterProxyModel();
  filter_model->setDynamicSortFilter(true);
  filter_model->setSourceModel(list_model);
  filter_model->setFilterRole(Qt::DisplayRole);
  setModel(filter_model);
  setRootIsDecorated(false);
  setAllColumnsShowFocus(true);
  setSortingEnabled(true);
  setSelectionMode(QAbstractItemView::SingleSelection);
  setItemsExpandable(false);
  setAutoScroll(true);
  setAlternatingRowColors(true);
  header()->setContextMenuPolicy(Qt::CustomContextMenu);
  hide_columns();
  connect(header(), &QWidget::customContextMenuRequested,
          this, &plr_widget::display_header_menu);
  connect(selectionModel(),
          SIGNAL(selectionChanged(const QItemSelection &,
                                  const QItemSelection &)),
          SLOT(nation_selected(const QItemSelection &,
                               const QItemSelection &)));
}

/**********************************************************************//**
  Restores selection of previously selected nation
**************************************************************************/
void plr_widget::restore_selection()
{
  QItemSelection selection;
  QModelIndex i;
  struct player *pplayer;
  QVariant qvar;

  if (selected_player == nullptr) {
    return;
  }
  for (int j = 0; j < filter_model->rowCount(); j++) {
    i = filter_model->index(j, 0);
    qvar = i.data(Qt::UserRole);
    if (qvar.isNull()) {
      continue;
    }
    pplayer = reinterpret_cast<struct player *>(qvar.value<void *>());
    if (selected_player == pplayer) {
      selection.append(QItemSelectionRange(i));
    }
  }
  selectionModel()->select(selection, QItemSelectionModel::Rows
                           | QItemSelectionModel::SelectCurrent);
}

/**********************************************************************//**
  Displays menu on header by right clicking
**************************************************************************/
void plr_widget::display_header_menu(const QPoint &)
{
  struct player_dlg_column *pcol;
  QMenu hideshowColumn(this);
  hideshowColumn.setTitle(_("Column visibility"));
  QList<QAction *> actions;
  for (int i = 0; i < list_model->columnCount(); ++i) {
    QAction *myAct = hideshowColumn.addAction(
                       list_model->headerData(i, Qt::Horizontal, 
                                              Qt::DisplayRole).toString());
    myAct->setCheckable(true);
    myAct->setChecked(!isColumnHidden(i));
    actions.append(myAct);
  }
  QAction *act = hideshowColumn.exec(QCursor::pos());
  if (act) {
    int col = actions.indexOf(act);
    Q_ASSERT(col >= 0);
    pcol = &player_dlg_columns[col];
    pcol->show = !pcol->show;
    setColumnHidden(col, !isColumnHidden(col));
    if (!isColumnHidden(col) && columnWidth(col) <= 5)
      setColumnWidth(col, 100);
  }
}

/**********************************************************************//**
  Returns information about column if hidden
**************************************************************************/
QVariant plr_model::hide_data(int section) const
{
  struct player_dlg_column *pcol;
  pcol = &player_dlg_columns[section];
  return pcol->show;
}

/**********************************************************************//**
  Hides columns in plr widget, depending on info from plr_list
**************************************************************************/
void plr_widget::hide_columns()
{
  int col;

  for (col = 0; col < list_model->columnCount(); col++) {
    if (!list_model->hide_data(col).toBool()){
      setColumnHidden(col, !isColumnHidden(col));
    }
  }
}

/**********************************************************************//**
  Slot for selecting player/nation
**************************************************************************/
void plr_widget::nation_selected(const QItemSelection &sl,
                                 const QItemSelection &ds)
{
  QModelIndex index;
  QVariant qvar;
  QModelIndexList indexes = sl.indexes();
  struct city *pcity;
  const struct player_diplstate *state;
  struct research *my_research, *research;
  char tbuf[256];
  QString res;
  QString sp = " ";
  QString etax, esci, elux, egold, egov;
  QString nl = "<br>";
  QStringList sorted_list_a;
  QStringList sorted_list_b;
  struct player *pplayer;
  int a , b;
  bool added;
  bool entry_exist = false;
  struct player *me;
  Tech_type_id tech_id;

  other_player = NULL;
  intel_str.clear();
  tech_str.clear();
  ally_str.clear();
  if (indexes.isEmpty()) {
    selected_player = nullptr;
    plr->update_report(false);
    return;
  }
  index = indexes.at(0);
  qvar = index.data(Qt::UserRole);
  pplayer = reinterpret_cast<player *>(qvar.value<void *>());
  selected_player = pplayer;
  other_player = pplayer;
  if (pplayer->is_alive == false) {
    plr->update_report(false);
    return;
  }
  me = client_player();
  pcity = player_capital(pplayer);
  research = research_get(pplayer);

  switch (research->researching) {
  case A_UNKNOWN:
    res = _("(Unknown)");
    break;
  case A_UNSET:
      if (player_has_embassy(me, pplayer)) {
        res = _("(none)");
      } else {
        res = _("(Unknown)");
      }
    break;
  default:
    res = QString(research_advance_name_translation(research,
                                                    research->researching))
          + sp + "(" + QString::number(research->bulbs_researched) + "/"
          + QString::number(research->client.researching_cost) + ")";
    break;
  }
  if (player_has_embassy(me, pplayer)) {
    etax = QString::number(pplayer->economic.tax) + "%";
    esci = QString::number(pplayer->economic.science) + "%";
    elux = QString::number(pplayer->economic.science) + "%";
  } else {
    etax = _("(Unknown)");
    esci = _("(Unknown)");
    elux = _("(Unknown)");
  }
  if (could_intel_with_player(me, pplayer)) {
    egold = QString::number(pplayer->economic.gold);
    egov = QString(government_name_for_player(pplayer));
  } else {
    egold = _("(Unknown)");
    egov = _("(Unknown)");
  }
  /** Formatting rich text */
  intel_str =
    QString("<table><tr><td><b>") + _("Nation") + QString("</b></td><td>")
    + QString(nation_adjective_for_player(pplayer))
    + QString("</td><tr><td><b>") + _("Ruler:") + QString("</b></td><td>")
    + QString(ruler_title_for_player(pplayer, tbuf, sizeof(tbuf)))
    + QString("</td></tr><tr><td><b>") + _("Government:")
    + QString("</b></td><td>") + egov
    + QString("</td></tr><tr><td><b>") + _("Capital:")
    + QString("</b></td><td>")
    + QString(((!pcity) ? _("(Unknown)") : city_name_get(pcity)))
    + QString("</td></tr><tr><td><b>") + _("Gold:")
    + QString("</b></td><td>") + egold
    + QString("</td></tr><tr><td><b>") + _("Tax:")
    + QString("</b></td><td>") + etax
    + QString("</td></tr><tr><td><b>") + _("Science:")
    + QString("</b></td><td>") + esci
    + QString("</td></tr><tr><td><b>") + _("Luxury:")
    + QString("</b></td><td>") + elux
    + QString("</td></tr><tr><td><b>") + _("Researching:")
    + QString("</b></td><td>") + res + QString("</td></table>");

  for (int i = 0; i < static_cast<int>(DS_LAST); i++) {
    added = false;
    if (entry_exist) {
      ally_str += "<br>";
    }
    entry_exist = false;
    players_iterate_alive(other) {
      if (other == pplayer || is_barbarian(other)) {
        continue;
      }
      state = player_diplstate_get(pplayer, other);
      if (static_cast<int>(state->type) == i
          && could_intel_with_player(me, pplayer)) {
        if (added == false) {
          ally_str = ally_str  + QString("<b>")
                     + QString(diplstate_type_translated_name(
                                 static_cast<diplstate_type>(i)))
                     + ": "  + QString("</b>") + nl;
          added = true;
        }
        if (gives_shared_vision(pplayer, other)) {
          ally_str = ally_str + "(◐‿◑)";
        }
        ally_str = ally_str + nation_plural_for_player(other) + ", ";
        entry_exist = true;
      }
    } players_iterate_alive_end;
    if (entry_exist) {
      ally_str.replace(ally_str.lastIndexOf(","), 1, ".");
    }
  }
  my_research = research_get(me);
  if (!client_is_global_observer()) {
    if (player_has_embassy(me, pplayer) && me != pplayer) {
      a = 0;
      b = 0;
      techs_known = QString(_("<b>Techs unknown by %1:</b>")).
                    arg(nation_plural_for_player(pplayer));
      techs_unknown = QString(_("<b>Techs unknown by you :</b>"));

      advance_iterate(A_FIRST, padvance) {
        tech_id = advance_number(padvance);
        if (research_invention_state(my_research, tech_id) == TECH_KNOWN
            && (research_invention_state(research, tech_id) 
                != TECH_KNOWN)) {
          a++;
          sorted_list_a << research_advance_name_translation(research,
                                                             tech_id);
        }
        if (research_invention_state(my_research, tech_id) != TECH_KNOWN
            && (research_invention_state(research, tech_id) == TECH_KNOWN)) {
          b++;
          sorted_list_b << research_advance_name_translation(research,
                                                             tech_id);
        }
      } advance_iterate_end;
      sorted_list_a.sort(Qt::CaseInsensitive);
      sorted_list_b.sort(Qt::CaseInsensitive);
      foreach (res, sorted_list_a) {
        techs_known = techs_known + QString("<i>") + res + ","
                      + QString("</i>") + sp;
      }
      foreach (res, sorted_list_b) {
        techs_unknown = techs_unknown + QString("<i>") + res + ","
                        + QString("</i>") + sp;
      }
      if (a == 0) {
        techs_known = techs_known + QString("<i>") + sp
                      + QString(Q_("?tech:None")) + QString("</i>");
      } else {
        techs_known.replace(techs_known.lastIndexOf(","), 1, ".");
      }
      if (b == 0) {
        techs_unknown = techs_unknown + QString("<i>") + sp
                        + QString(Q_("?tech:None")) + QString("</i>");
      } else {
        techs_unknown.replace(techs_unknown.lastIndexOf(","), 1, ".");
      }
      tech_str = techs_known + nl + techs_unknown;
    }
  } else {
    tech_str = QString(_("<b>Techs known by %1:</b>")).
               arg(nation_plural_for_player(pplayer));
    advance_iterate(A_FIRST, padvance) {
      tech_id = advance_number(padvance);
      if (research_invention_state(research, tech_id) == TECH_KNOWN) {
        sorted_list_a << research_advance_name_translation(research, tech_id);
      }
    } advance_iterate_end;
    sorted_list_a.sort(Qt::CaseInsensitive);
    foreach (res, sorted_list_a) {
      tech_str = tech_str + QString("<i>") + res + ","
                    + QString("</i>") + sp;
    }
  }
  plr->update_report(false);
}

/**********************************************************************//**
  Returns model used in widget
**************************************************************************/
plr_model *plr_widget::get_model() const
{
  return list_model;
}

/**********************************************************************//**
  Destructor for player widget
**************************************************************************/
plr_widget::~plr_widget()
{
  delete pid;
  delete list_model;
  delete filter_model;
  gui()->qt_settings.player_repo_sort_col = header()->sortIndicatorSection();
  gui()->qt_settings.player_report_sort = header()->sortIndicatorOrder();
}

/**********************************************************************//**
  Constructor for plr_report
**************************************************************************/
plr_report::plr_report():QWidget()
{
  v_splitter = new QSplitter(Qt::Vertical);
  h_splitter = new QSplitter(Qt::Horizontal);
  layout = new QVBoxLayout;
  hlayout = new QHBoxLayout;
  plr_wdg = new plr_widget(this);
  plr_label = new QLabel;
  plr_label->setFrameStyle(QFrame::StyledPanel);
  plr_label->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  plr_label->setWordWrap(true);
  plr_label->setTextFormat(Qt::RichText);
  ally_label = new QLabel;
  ally_label->setFrameStyle(QFrame::StyledPanel);
  ally_label->setTextFormat(Qt::RichText);
  ally_label->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  ally_label->setWordWrap(true);
  tech_label = new QLabel;
  tech_label->setTextFormat(Qt::RichText);
  tech_label->setFrameStyle(QFrame::StyledPanel);
  tech_label->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  tech_label->setWordWrap(true);
  meet_but = new QPushButton;
  meet_but->setText(_("Meet"));
  cancel_but = new QPushButton;
  cancel_but->setText(_("Cancel Treaty"));
  withdraw_but = new QPushButton;
  withdraw_but->setText(_("Withdraw Vision"));
  toggle_ai_but = new QPushButton;
  toggle_ai_but->setText(_("Toggle AI Mode"));
  meet_but->setDisabled(true);
  cancel_but->setDisabled(true);
  withdraw_but->setDisabled(true);
  toggle_ai_but->setDisabled(true);
  v_splitter->addWidget(plr_wdg);
  h_splitter->addWidget(plr_label);
  h_splitter->addWidget(ally_label);
  h_splitter->addWidget(tech_label);
  v_splitter->addWidget(h_splitter);
  layout->addWidget(v_splitter);
  hlayout->addWidget(meet_but);
  hlayout->addWidget(cancel_but);
  hlayout->addWidget(withdraw_but);
  hlayout->addWidget(toggle_ai_but);
  hlayout->addStretch();
  layout->addLayout(hlayout);
  connect(meet_but, &QAbstractButton::pressed, this, &plr_report::req_meeeting);
  connect(cancel_but, &QAbstractButton::pressed, this, &plr_report::req_caancel_threaty);
  connect(withdraw_but, &QAbstractButton::pressed, this, &plr_report::req_wiithdrw_vision);
  connect(toggle_ai_but, &QAbstractButton::pressed, this, &plr_report::toggle_ai_mode);
  setLayout(layout);
  if (gui()->qt_settings.player_repo_sort_col != -1) {
    plr_wdg->sortByColumn(gui()->qt_settings.player_repo_sort_col,
                          gui()->qt_settings.player_report_sort);
  }
}

/**********************************************************************//**
  Destructor for plr_report
**************************************************************************/
plr_report::~plr_report()
{
  gui()->remove_repo_dlg("PLR");
}

/**********************************************************************//**
  Adds plr_report to tab widget
**************************************************************************/
void plr_report::init()
{
  gui()->gimme_place(this, "PLR");
  index = gui()->add_game_tab(this);
  gui()->game_tab_widget->setCurrentIndex(index);
}


/**********************************************************************//**
  Public function to call meeting
**************************************************************************/
void plr_report::call_meeting()
{
  if (meet_but->isEnabled() == true) {
    req_meeeting();
  }
}

/**********************************************************************//**
  Slot for canceling threaty (name changed to cheat autoconnect, and
  doubled execution)
**************************************************************************/
void plr_report::req_caancel_threaty()
{
  dsend_packet_diplomacy_cancel_pact(&client.conn, 
                                     player_number(other_player),
                                     CLAUSE_CEASEFIRE);
}

/**********************************************************************//**
  Slot for meeting request
**************************************************************************/
void plr_report::req_meeeting()
{
  dsend_packet_diplomacy_init_meeting_req(&client.conn,
                                          player_number(other_player));
}

/**********************************************************************//**
  Slot for withdrawing vision
**************************************************************************/
void plr_report::req_wiithdrw_vision()
{
  dsend_packet_diplomacy_cancel_pact(&client.conn, 
                                     player_number(other_player),
                                     CLAUSE_VISION);
}

/**********************************************************************//**
  Slot for changing AI mode
**************************************************************************/
void plr_report::toggle_ai_mode()
{
  QAction *act;
  QAction *toggle_ai_act;
  QAction *ai_level_act;
  QMenu ai_menu(this);
  int level;

  toggle_ai_act = new QAction(_("Toggle AI Mode"), nullptr);
  ai_menu.addAction(toggle_ai_act);
  ai_menu.addSeparator();
  for (level = 0; level < AI_LEVEL_COUNT; level++) {
    if (is_settable_ai_level(static_cast<ai_level>(level))) {
      QString ln = ai_level_translated_name(static_cast<ai_level>(level));
      ai_level_act = new QAction(ln, nullptr);
      ai_level_act->setData(QVariant::fromValue(level));
      ai_menu.addAction(ai_level_act);
    }
  }
  act = 0;
  act = ai_menu.exec(QCursor::pos());
  if (act == toggle_ai_act) {
    send_chat_printf("/aitoggle \"%s\"", player_name(plr_wdg->other_player));
    return;
  }
  if (act && act->isVisible()) {
    level = act->data().toInt();
    if (is_human(plr_wdg->other_player)) {
      send_chat_printf("/aitoggle \"%s\"", player_name(plr_wdg->other_player));
    }
    send_chat_printf("/%s %s", ai_level_cmd(static_cast<ai_level>(level)),
                     player_name(plr_wdg->other_player));
  }

}

/**********************************************************************//**
  Handle mouse click
**************************************************************************/
void plr_widget::mousePressEvent(QMouseEvent *event)
{
  QModelIndex index =  this->indexAt(event->pos());
  if (index.isValid() &&  event->button() == Qt::RightButton
      && can_client_issue_orders()) {
     plr->call_meeting();
  }
  QTreeView::mousePressEvent(event);
}

/**********************************************************************//**
  Updates widget
**************************************************************************/
void plr_report::update_report(bool update_selection)
{
  QModelIndex qmi;
  int player_count = 0;
  
  /* Force updating selected player information */
  if (update_selection == true) {
    qmi = plr_wdg->currentIndex();
    if (qmi.isValid()) {
      plr_wdg->clearSelection();
      plr_wdg->setCurrentIndex(qmi);
    }
  }

  players_iterate(pplayer) {
    if ((is_barbarian(pplayer))){
      continue;
    }
    player_count++;
  } players_iterate_end;

  if (player_count != plr_wdg->get_model()->rowCount()) {
    plr_wdg->get_model()->populate();
  }

  plr_wdg->header()->resizeSections(QHeaderView::ResizeToContents);
  meet_but->setDisabled(true);
  cancel_but->setDisabled(true);
  withdraw_but->setDisabled(true);
  toggle_ai_but->setDisabled(true);
  plr_label->setText(plr_wdg->intel_str);
  ally_label->setText(plr_wdg->ally_str);
  tech_label->setText(plr_wdg->tech_str);
  other_player = plr_wdg->other_player;
  if (other_player == NULL || !can_client_issue_orders()) {
    return;
  }
  if (NULL != client.conn.playing
      && other_player != client.conn.playing) {

    // We keep button sensitive in case of DIPL_SENATE_BLOCKING, so that player
    // can request server side to check requirements of those effects with omniscience
    if (pplayer_can_cancel_treaty(client_player(), other_player) != DIPL_ERROR) {
      cancel_but->setEnabled(true);
    }
    toggle_ai_but->setEnabled(true);
  }
  if (gives_shared_vision(client_player(), other_player)
      && !players_on_same_team(client_player(), other_player)) {
    withdraw_but->setEnabled(true);
  }
  if (can_meet_with_player(other_player) == true) {
    meet_but->setEnabled(true);
  }
  plr_wdg->restore_selection();
}

/**********************************************************************//**
  Display the player list dialog.  Optionally raise it.
**************************************************************************/
void popup_players_dialog(bool raise)
{
  int i;
  QWidget *w;

  if (!gui()->is_repo_dlg_open("PLR")) {
    plr_report *pr = new plr_report;

    pr->init();
    pr->update_report();
  } else {
    plr_report *pr;

    i = gui()->gimme_index_of("PLR");
    w = gui()->game_tab_widget->widget(i);
    if (w->isVisible() == true) {
      gui()->game_tab_widget->setCurrentIndex(0);
      return;
    }
    pr = reinterpret_cast<plr_report*>(w);
    gui()->game_tab_widget->setCurrentWidget(pr);
    pr->update_report();
  }
}

/**********************************************************************//**
  Update all information in the player list dialog.
**************************************************************************/
void real_players_dialog_update(void *unused)
{
  int i;
  plr_report *pr;
  QWidget *w;

  if (gui()->is_repo_dlg_open("PLR")) {
    i = gui()->gimme_index_of("PLR");
    if (gui()->game_tab_widget->currentIndex() == i) {
      w = gui()->game_tab_widget->widget(i);
      pr = reinterpret_cast<plr_report *>(w);
      pr->update_report();
    }
  }
}

/**********************************************************************//**
  Closes players report
**************************************************************************/
void popdown_players_report()
{
  int i;
  plr_report *pr;
  QWidget *w;

  if (gui()->is_repo_dlg_open("PLR")) {
    i = gui()->gimme_index_of("PLR");
    fc_assert(i != -1);
    w = gui()->game_tab_widget->widget(i);
    pr = reinterpret_cast<plr_report *>(w);
    pr->deleteLater();
  }
}
