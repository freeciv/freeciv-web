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
#include <QGraphicsDropShadowEffect>
#include <QGroupBox>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QStack>
#include <QStringList>
#include <QTextBrowser>
#include <QTreeWidget>
#include <QVBoxLayout>

// utility
#include "fcintl.h"

// common
#include "movement.h"
#include "nation.h"
#include "specialist.h"
#include "terrain.h"
#include "unit.h"

// client
#include "helpdata.h"

// gui-qt
#include "fc_client.h"
#include "fonts.h"
#include "helpdlg.h"
#include "sprite.h"

#define MAX_HELP_TEXT_SIZE 8192
#define REQ_LABEL_NEVER _("(Never)")
#define REQ_LABEL_NONE _("?tech:None")
static help_dialog *help_dlg = NULL;
canvas *terrain_canvas(struct terrain *terrain,
                       const struct extra_type *resource = NULL,
                       enum extra_cause cause = EC_COUNT);

/**********************************************************************//**
  Popup the help dialog to get help on the given string topic.  Note
  that the topic may appear in multiple sections of the help (it may
  be both an improvement and a unit, for example).

  The given string should be untranslated.
**************************************************************************/
void popup_help_dialog_string(const char *item)
{
  popup_help_dialog_typed(Q_(item), HELP_ANY);
}

/**********************************************************************//**
  Popup the help dialog to display help on the given string topic from
  the given section.

  The string will be translated.
**************************************************************************/
void popup_help_dialog_typed(const char *item, enum help_page_type htype)
{
  int pos;
  const help_item *topic;

  if (!help_dlg) {
    help_dlg = new help_dialog();
  }
  topic = get_help_item_spec(item, htype, &pos);
  if (pos >= 0) {
    help_dlg->set_topic(topic);
  }
  help_dlg->setVisible(true);
  help_dlg->activateWindow();
}

/**********************************************************************//**
  Close the help dialog.
**************************************************************************/
void popdown_help_dialog(void)
{
  if (help_dlg) {
    help_dlg->setVisible(false);
    help_dlg->deleteLater();
    help_dlg = NULL;
  }
}

/**********************************************************************//**
  Updates fonts
**************************************************************************/
void update_help_fonts()
{
  if (help_dlg) {
    help_dlg->update_fonts();
  }
}

/**********************************************************************//**
  Constructor for help dialog
**************************************************************************/
help_dialog::help_dialog(QWidget *parent) : qfc_dialog(parent)
{
  QHBoxLayout *hbox;
  QPushButton *but;
  QTreeWidgetItem *first;
  QVBoxLayout *layout;
  QWidget *buttons;

  setWindowTitle(_("Freeciv Help Browser"));
  history_pos = -1;
  update_history = true;
  layout = new QVBoxLayout(this);

  splitter = new QSplitter(this);
  layout->addWidget(splitter, 10);

  tree_wdg = new QTreeWidget();
  tree_wdg->setHeaderHidden(true);
  make_tree();
  splitter->addWidget(tree_wdg);

  help_wdg = new help_widget(splitter);
  connect(
    tree_wdg,
    &QTreeWidget::currentItemChanged,
    this, &help_dialog::item_changed
  );
  help_wdg->layout()->setContentsMargins(0, 0, 0, 0);
  splitter->addWidget(help_wdg);

  buttons = new QWidget;
  hbox = new QHBoxLayout;
  prev_butt = new QPushButton(style()->standardIcon(
                              QStyle::QStyle::SP_ArrowLeft), (""));
  connect(prev_butt, &QAbstractButton::clicked, this,
          &help_dialog::history_back);
  hbox->addWidget(prev_butt);
  next_butt = new QPushButton(style()->standardIcon(
                              QStyle::QStyle::SP_ArrowRight), (""));
  connect(next_butt, &QAbstractButton::clicked, this,
          &help_dialog::history_forward);
  hbox->addWidget(next_butt);
  hbox->addStretch(20);
  but = new QPushButton(style()->standardIcon(
                        QStyle::SP_DialogHelpButton), _("About Qt"));
  connect(but, &QPushButton::pressed, &QApplication::aboutQt);
  hbox->addWidget(but, Qt::AlignRight);
  but = new QPushButton(style()->standardIcon(
                        QStyle::SP_DialogDiscardButton), _("Close"));
  connect(but, &QPushButton::pressed, this, &QWidget::close);
  hbox->addWidget(but, Qt::AlignRight);
  buttons->setLayout(hbox);
  layout->addWidget(buttons, 0, Qt::AlignBottom);


  first = tree_wdg->topLevelItem(0);
  if (first) {
    tree_wdg->setCurrentItem(first);
  }
}

/**********************************************************************//**
  Update fonts for help_wdg
**************************************************************************/
void help_dialog::update_fonts()
{
  help_wdg->update_fonts();
}

/****************************************************************************
  Hide event
****************************************************************************/
void help_dialog::hideEvent(QHideEvent *event)
{
  gui()->qt_settings.help_geometry = saveGeometry();
  gui()->qt_settings.help_splitter1 = splitter->saveState();
}

/****************************************************************************
  Show event
****************************************************************************/
void help_dialog::showEvent(QShowEvent *event)
{
  QList<int> sizes;

  if (gui()->qt_settings.help_geometry.isNull() == false) {
    restoreGeometry(gui()->qt_settings.help_geometry);
    splitter->restoreState(gui()->qt_settings.help_splitter1);
  } else {
    QRect rect = QApplication::desktop()->screenGeometry();
    resize((rect.width() * 3) / 5, (rect.height() * 3) / 6);
    sizes << rect.width() / 10 << rect.width() / 3;
    splitter->setSizes(sizes);
  }
}

/****************************************************************************
  Close event
****************************************************************************/
void help_dialog::closeEvent(QCloseEvent *event)
{
  gui()->qt_settings.help_geometry = saveGeometry();
  gui()->qt_settings.help_splitter1 = splitter->saveState();
}



/**********************************************************************//**
  Create the help tree.
**************************************************************************/
void help_dialog::make_tree()
{
  char *title;
  int dep;
  int i;
  QHash<int, QTreeWidgetItem *> hash;
  QIcon icon;
  QTreeWidgetItem *item;
  sprite *spite;
  struct advance *padvance;
  struct canvas *pcan;
  struct extra_type *pextra;
  struct government *gov;
  struct impr_type *imp;
  struct nation_type *nation;
  struct terrain *pterrain;
  struct unit_type *f_type;
  struct drawn_sprite sprs[80];

  help_items_iterate(pitem) {
    const char *s;
    int last;
    title = pitem->topic;

    for (s = pitem->topic; *s == ' '; s++) {
      /* nothing */
    }

    item = new QTreeWidgetItem(QStringList(title));
    topics_map[item] = pitem;
    dep = s - pitem->topic;
    hash.insert(dep, item);

    if (dep == 0) {
      tree_wdg->addTopLevelItem(item);
    } else {
      last = dep - 1;
      spite = nullptr;

      switch (pitem->type) {
      case HELP_EXTRA:
        pextra = extra_type_by_translated_name(s);
        fill_basic_extra_sprite_array(tileset, sprs, pextra);
        icon = QIcon(*sprs->sprite->pm);
        break;

      case HELP_GOVERNMENT:
        gov = government_by_translated_name(s);
        spite = get_government_sprite(tileset, gov);
        if (spite) {
          icon = QIcon(*spite->pm);
        }
        break;

      case HELP_IMPROVEMENT:
      case HELP_WONDER:
        imp = improvement_by_translated_name(s);
        spite = get_building_sprite(tileset, imp);
        if (spite) {
          icon = QIcon(*spite->pm);
        }
        break;
      case HELP_NATIONS:
        nation = nation_by_translated_plural(s);
        spite = get_nation_flag_sprite(tileset, nation);
        if (spite) {
            icon = QIcon(*spite->pm);
        }
        break;
      case HELP_TECH:
        padvance  = advance_by_translated_name(s);
        if (padvance && !is_future_tech(i = advance_number(padvance))) {
          spite = get_tech_sprite(tileset, i);
          if (spite) {
            icon = QIcon(*spite->pm);
          }
        }
        break;

      case HELP_TERRAIN:
        pterrain = terrain_by_translated_name(s);
        pcan = terrain_canvas(pterrain);
        if (pcan) {
          icon = QIcon(pcan->map_pixmap);
          delete pcan;
        }
        break;

      case HELP_UNIT:
        f_type = unit_type_by_translated_name(s);
        if (f_type) {
          spite = get_unittype_sprite(tileset, f_type, direction8_invalid());
        }
        if (spite) {
          icon = QIcon(*spite->pm);
        }
        break;

      default:
        break;
      }

      if (icon.isNull() == false) {
        item->setIcon(0, icon);
      }

      hash.value(last)->addChild(item);
    }
  } help_items_iterate_end;
}

/**********************************************************************//**
  Changes the displayed topic.
**************************************************************************/
void help_dialog::set_topic(const help_item *topic)
{
  help_wdg->set_topic(topic);
  // Reverse search of the item to select.
  QHash<QTreeWidgetItem *, const help_item *>::const_iterator
      i = topics_map.cbegin();
  for ( ; i != topics_map.cend(); ++i) {
    if (i.value() == topic) {
      tree_wdg->setCurrentItem(i.key());
      break;
    }
  }
}

/**********************************************************************//**
  Goes to next topic in history
**************************************************************************/
void help_dialog::history_forward()
{
  QTreeWidgetItem *i;

  update_history = false;
  if (history_pos < item_history.count()) {
    history_pos++;
  }
  i = item_history.value(history_pos);
  if (i != nullptr) {
    tree_wdg->setCurrentItem(i);
  }
}

/**********************************************************************//**
  Backs in history to previous topic
**************************************************************************/
void help_dialog::history_back()
{
  QTreeWidgetItem *i;

  update_history = false;
  if (history_pos > 0) {
    history_pos--;
  }
  i = item_history.value(history_pos);
  if (i != nullptr) {
    tree_wdg->setCurrentItem(i);
  }
}

/**********************************************************************//**
  Update buttons (back and next)
**************************************************************************/
void help_dialog::update_buttons()
{
  if (history_pos == 0) {
    prev_butt->setEnabled(false);
  } else {
    prev_butt->setEnabled(true);
  }
  if (history_pos >= item_history.size() - 1) {
    next_butt->setEnabled(false);
  } else {
    next_butt->setEnabled(true);
  }
}

/**********************************************************************//**
  Called when a tree item is activated.
**************************************************************************/
void help_dialog::item_changed(QTreeWidgetItem *item, QTreeWidgetItem *prev)
{

  if (prev == item) {
    return;
  }

  help_wdg->set_topic(topics_map[item]);

  if (update_history == true) {
    history_pos++;
    item_history.append(item);
  } else {
    update_history = true;
  }
  update_buttons();
}

/**********************************************************************//**
  Creates a new, empty help widget.
**************************************************************************/
help_widget::help_widget(QWidget *parent) :
  QWidget(parent),
  main_widget(NULL), text_browser(NULL), bottom_panel(NULL),
  info_panel(NULL), splitter(NULL), info_layout(NULL)
{
  setup_ui();
}

/**********************************************************************//**
  Creates a new help widget displaying the specified topic.
**************************************************************************/
help_widget::help_widget(const help_item *topic, QWidget *parent) :
  QWidget(parent),
  main_widget(NULL), text_browser(NULL), bottom_panel(NULL),
  info_panel(NULL), splitter(NULL), info_layout(NULL)
{
  setup_ui();
  set_topic(topic);
}

/**********************************************************************//**
  Destructor.
**************************************************************************/
help_widget::~help_widget() {
  // Nothing to do here
}

/**********************************************************************//**
  Creates the UI.
**************************************************************************/
void help_widget::setup_ui()
{
  QVBoxLayout *layout;
  QHBoxLayout *group_layout;

  layout = new QVBoxLayout();
  setLayout(layout);

  box_wdg = new QFrame(this);
  layout->addWidget(box_wdg);
  group_layout = new QHBoxLayout(box_wdg);
  box_wdg->setLayout(group_layout);
  box_wdg->setFrameShape(QFrame::StyledPanel);
  box_wdg->setFrameShadow(QFrame::Raised);

  title_label = new QLabel(box_wdg);
  title_label->setProperty(fonts::help_title, "true");
  group_layout->addWidget(title_label);

  text_browser = new QTextBrowser(this);
  text_browser->setProperty(fonts::help_text, "true");
  layout->addWidget(text_browser);
  main_widget = text_browser;

  update_fonts();
  splitter_sizes << 200 << 400;
}

/**********************************************************************//**
  Lays things out. The widget is organized as follows, with the additional
  complexity that info_ and/or bottom_panel may be absent.

    +---------------------------------+
    | title_label                     |
    +---------------------------------+
    |+-main_widget-------------------+|
    ||+------------+ +--------------+||
    |||            | |              |||
    ||| info_panel | | text_browser |||
    |||            | |              |||
    |||            |.+--------------+||
    |||            |.      ...       ||
    |||            |.+--------------+||
    |||            | |              |||
    |||            | | bottom_panel |||
    |||            | |              |||
    ||+------------+ +--------------+||
    |+-------------------------------+|
    +---------------------------------+
**************************************************************************/
void help_widget::do_layout()
{
  QWidget *right;

  layout()->removeWidget(main_widget);
  main_widget->setParent(NULL);

  if (bottom_panel) {
    splitter = new QSplitter(Qt::Vertical);
    splitter->addWidget(text_browser);
    splitter->setStretchFactor(0, 100);
    splitter->addWidget(bottom_panel);
    splitter->setStretchFactor(1, 0);
    right = splitter;
  } else {
    right = text_browser;
  }

  if (info_panel) {
    splitter = new QSplitter();
    splitter->addWidget(info_panel);
    splitter->setStretchFactor(0, 25);
    splitter->addWidget(right);
    splitter->setStretchFactor(1, 75);
    splitter->setSizes(splitter_sizes);
    main_widget = splitter;
    info_panel->setLayout(info_layout);
  } else {
    main_widget = right;
  }

  layout()->addWidget(main_widget);
  qobject_cast<QVBoxLayout *>(layout())->setStretchFactor(main_widget, 100);
}

/**********************************************************************//**
  Updates fonts for manual
**************************************************************************/
void help_widget::update_fonts()
{
  QList<QWidget *> l;
  QFont *f;

  l = findChildren<QWidget *>();

  f = fc_font::instance()->get_font(fonts::help_label);
  for (int i = 0; i < l.size(); ++i) {
    if (l.at(i)->property(fonts::help_label).isValid()) {
      l.at(i)->setFont(*f);
    }
  }
  f = fc_font::instance()->get_font(fonts::help_text);
  for (int i = 0; i < l.size(); ++i) {
    if (l.at(i)->property(fonts::help_text).isValid()) {
      l.at(i)->setFont(*f);
    }
  }
  f = fc_font::instance()->get_font(fonts::help_title);
  for (int i = 0; i < l.size(); ++i) {
    if (l.at(i)->property(fonts::help_title).isValid()) {
      l.at(i)->setFont(*f);
    }
  }
}

/**********************************************************************//**
  Deletes the widgets created by do_complex_layout().
**************************************************************************/
void help_widget::undo_layout()
{
  // Save the splitter sizes to avoid jumps
  if (info_panel) {
    splitter_sizes = splitter->sizes();
  }
  // Unparent the widget we want to keep
  text_browser->setParent(NULL);
  // Delete everything else
  if (text_browser != main_widget) {
    main_widget->deleteLater();
  }
  // Reset pointers to defaults
  main_widget = text_browser;
  bottom_panel = NULL;
  info_panel = NULL;
  splitter = NULL;
  info_layout = NULL;
}

/**********************************************************************//**
  Creates the information panel. It will be shown by do_complex_layout().
**************************************************************************/
void help_widget::show_info_panel()
{
  info_panel = new QWidget();
  info_layout = new QVBoxLayout();
}

/**********************************************************************//**
  Adds a pixmap to the information panel.
**************************************************************************/
void help_widget::add_info_pixmap(QPixmap *pm, bool shadow)
{
  QLabel *label = new QLabel();
  QGraphicsDropShadowEffect *effect;

  label->setAlignment(Qt::AlignHCenter);
  label->setPixmap(*pm);

  if (shadow) {
    effect = new QGraphicsDropShadowEffect(label);
    effect->setBlurRadius(3);
    effect->setOffset(0, 2);
    label->setGraphicsEffect(effect);
  }

  info_layout->addWidget(label);
}

/**********************************************************************//**
  Adds a text label to the information panel.
**************************************************************************/
void help_widget::add_info_label(const QString &text)
{
  QLabel *label = new QLabel(text);
  label->setWordWrap(true);
  label->setProperty(fonts::help_label, "true");
  info_layout->addWidget(label);
}

/**********************************************************************//**
  Adds a widget indicating a progress to the information panel.
  Arguments:
    text: A descriptive text
    progress: The progress to display
    [min,max]: The interval progress is in
    value: Use this to display a non-numeral value
**************************************************************************/
void help_widget::add_info_progress(const QString &text, int progress,
                                    int min, int max, const QString &value)
{
  QGridLayout *layout;
  QLabel *label;
  QProgressBar *bar;
  QWidget *wdg;

  wdg = new QWidget();
  layout = new QGridLayout(wdg);
  layout->setMargin(0);
  layout->setVerticalSpacing(0);

  label = new QLabel(text, wdg);
  layout->addWidget(label, 0, 0);
  label->setProperty(fonts::help_label, "true");
  label = new QLabel(wdg);
  if (value.isEmpty()) {
    label->setNum(progress);
  } else {
    label->setText(value);
  }
  label->setProperty(fonts::help_label, "true");
  layout->addWidget(label, 0, 1, Qt::AlignRight);

  bar = new QProgressBar(wdg);
  bar->setMaximumHeight(4);
  bar->setRange(min, max != min ? max : min + 1);
  bar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
  bar->setTextVisible(false);
  bar->setValue(progress);
  layout->addWidget(bar, 1, 0, 1, 2);

  info_layout->addWidget(wdg);
}

/**********************************************************************//**
  Create labels about all extras of one cause buildable to the terrain.
**************************************************************************/
void help_widget::add_extras_of_act_for_terrain(struct terrain *pterr,
                                                enum unit_activity act,
                                                const char *label)
{
  struct universal for_terr;
  enum extra_cause cause = activity_to_extra_cause(act);

  for_terr.kind = VUT_TERRAIN;
  for_terr.value.terrain = pterr;

  extra_type_by_cause_iterate(cause, pextra) {
    if (pextra->buildable
        && universal_fulfills_requirements(FALSE, &(pextra->reqs),
                                           &for_terr)) {
      QLabel *tb;
      QString str;

      tb = new QLabel(this);
      tb->setProperty(fonts::help_label, "true");
      tb->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
      tb->setTextFormat(Qt::RichText);

      str = str + QString(label) 
            + link_me(extra_name_translation(pextra), HELP_EXTRA)
            + QString(helptext_extra_for_terrain_str(pextra, pterr, act))
            + "\n";
            tb->setText(str.trimmed());
            connect(tb, &QLabel::linkActivated,
                    this, &help_widget::anchor_clicked);
            info_layout->addWidget(tb);
    }
  } extra_type_by_cause_iterate_end;
}

/**********************************************************************//**
  Creates link to given help page
**************************************************************************/
QString help_widget::link_me(const char *str, help_page_type hpt)
{
  QString s;
  s = QString(str).replace(" ", "&nbsp;");
  return " <a href=" + QString::number(hpt)
            + "," + s + ">" + s + "</a> ";
}

/**********************************************************************//**
  Adds a separator to the information panel.
**************************************************************************/
void help_widget::add_info_separator()
{
  info_layout->addSpacing(2 * info_layout->spacing());
}

/**********************************************************************//**
  Called when everything needed has been added to the information panel.
**************************************************************************/
void help_widget::info_panel_done()
{
  info_layout->addStretch();
}

/**********************************************************************//**
  Hyperlink clicked, link has 2 variables, string(name of given help)
  and int(help_page_type)
**************************************************************************/
void help_widget::anchor_clicked(const QString &link)
{
  QStringList sl;
  int n;
  QString st;
  enum help_page_type type;

  sl = link.split(",");
  n = sl.at(0).toInt();
  type = static_cast<help_page_type>(n);
  st = sl.at(1);
  st = st.replace("\u00A0", " ");

  if (strcmp(qPrintable(st), REQ_LABEL_NEVER) != 0
      && strcmp(qPrintable(st),
                skip_intl_qualifier_prefix(REQ_LABEL_NONE)) != 0
      && strcmp(qPrintable(st),
                advance_name_translation(advance_by_number(A_NONE))) != 0) {
    popup_help_dialog_typed(qPrintable(st), type);
  }
}

/**********************************************************************//**
  Shows the given help page.
**************************************************************************/
void help_widget::set_topic(const help_item *topic)
{
  char *title = topic->topic;
  for ( ; *title == ' '; ++title) {
    // Do nothing
  }
  title_label->setText(title);

  undo_layout();

  switch (topic->type) {
    case HELP_ANY:
    case HELP_MULTIPLIER:
    case HELP_RULESET:
    case HELP_TILESET:
    case HELP_TEXT:
      set_topic_other(topic, title);
      break;
    case HELP_EXTRA:
      set_topic_extra(topic, title);
      break;
    case HELP_GOODS:
      set_topic_goods(topic, title);
      break;
    case HELP_GOVERNMENT:
      set_topic_government(topic, title);
      break;
    case HELP_IMPROVEMENT:
    case HELP_WONDER:
      set_topic_building(topic, title);
      break;
    case HELP_NATIONS:
      set_topic_nation(topic, title);
      break;
    case HELP_SPECIALIST:
      set_topic_specialist(topic, title);
      break;
    case HELP_TECH:
      set_topic_tech(topic, title);
      break;
    case HELP_TERRAIN:
      set_topic_terrain(topic, title);
      break;
    case HELP_UNIT:
      set_topic_unit(topic, title);
      break;
    case HELP_LAST: // Just to avoid warning
      break;
  }

  do_layout();
}

/**********************************************************************//**
  Sets the bottom panel.
**************************************************************************/
void help_widget::set_bottom_panel(QWidget *widget) {
  bottom_panel = widget;
}

/**********************************************************************//**
  Creates help pages with no special widgets.
**************************************************************************/
void help_widget::set_topic_other(const help_item *topic,
                                    const char *title)
{
  if (topic->text) {
    text_browser->setText(topic->text);
  } else {
    text_browser->setText(""); // Something better to do ?
  }
}

/**********************************************************************//**
  Creates unit help pages.
**************************************************************************/
void help_widget::set_topic_unit(const help_item *topic,
                                   const char *title)
{
  char buffer[MAX_HELP_TEXT_SIZE];
  int upkeep, max_upkeep;
  struct advance *tech;
  struct canvas *canvas;
  struct unit_type *obsolete, *utype, *max_utype;
  QList<int> list;
  QString str;

  utype = unit_type_by_translated_name(title);
  if (utype) {
    helptext_unit(buffer, sizeof(buffer), client.conn.playing,
                  topic->text, utype);
    text_browser->setText(buffer);

    // Create information panel
    show_info_panel();
    max_utype = uclass_max_values(utype->uclass);

    // Unit icon
    canvas = qtg_canvas_create(
               tileset_full_tile_width(tileset),
               tileset_full_tile_height(tileset)
             );
    canvas->map_pixmap.fill(Qt::transparent);
    put_unittype(utype, canvas, 1.0f, 0, 0);
    add_info_pixmap(&canvas->map_pixmap);
    qtg_canvas_free(canvas);

    add_info_progress(_("Attack:"), utype->attack_strength, 0,
                      max_utype->attack_strength);
    add_info_progress(_("Defense:"), utype->defense_strength,
                      0, max_utype->defense_strength);
    add_info_progress(_("Moves:"), utype->move_rate, 0, max_utype->move_rate,
                      move_points_text(utype->move_rate, true));

    add_info_separator();

    add_info_progress(_("Hitpoints:"), utype->hp, 0, max_utype->hp);
    add_info_progress(_("Cost:"), utype_build_shield_cost_base(utype),
                      0, utype_build_shield_cost_base(max_utype));
    add_info_progress(_("Firepower:"), utype->firepower, 0,
                      max_utype->firepower);

    // Upkeep
    upkeep = utype->upkeep[O_FOOD] + utype->upkeep[O_GOLD]
             + utype->upkeep[O_LUXURY] + utype->upkeep[O_SCIENCE]
             + utype->upkeep[O_SHIELD] + utype->upkeep[O_TRADE]
             + utype->happy_cost;
    max_upkeep = max_utype->upkeep[O_FOOD] + max_utype->upkeep[O_GOLD]
                 + max_utype->upkeep[O_LUXURY] + max_utype->upkeep[O_SCIENCE]
                 + max_utype->upkeep[O_SHIELD] + max_utype->upkeep[O_TRADE]
                 + max_utype->happy_cost;
    add_info_progress(_("Basic upkeep:"), upkeep, 0, max_upkeep,
                      helptext_unit_upkeep_str(utype));

    add_info_separator();

    // Tech requirement
    tech = utype->require_advance;
    if (tech && tech != advance_by_number(0)) {
      QLabel *tb;

      tb = new QLabel(this);
      str = _("Requires");
      str = "<b>" + str + "</b> "
            + link_me(advance_name_translation(tech), HELP_TECH);

      tb->setProperty(fonts::help_label, "true");
      tb->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
      tb->setTextFormat(Qt::RichText);
      tb->setText(str.trimmed());
      connect(tb, &QLabel::linkActivated,
              this, &help_widget::anchor_clicked);
      info_layout->addWidget(tb);
    } else {
      add_info_label(_("No technology required."));
    }

    // Obsolescence
    obsolete = utype->obsoleted_by;
    if (obsolete) {
      tech = obsolete->require_advance;
      if (tech && tech != advance_by_number(0)) {
        QLabel *tb;

        tb = new QLabel(this);
        str = _("Obsoleted by");
        str = "<b>" + str + "</b> "
              + link_me(utype_name_translation(obsolete), HELP_UNIT)
              + "("  + link_me(advance_name_translation(tech), HELP_TECH)
              + ")";
        tb->setProperty(fonts::help_label, "true");
        tb->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
        tb->setTextFormat(Qt::RichText);
        tb->setText(str.trimmed());
        connect(tb, &QLabel::linkActivated,
                this, &help_widget::anchor_clicked);
        info_layout->addWidget(tb);
      } else {
        add_info_label(
          // TRANS: Current unit obsoleted by other unit
          QString(_("Obsoleted by %1."))
          .arg(utype_name_translation(obsolete)));
      }
    } else {
      add_info_label(_("Never obsolete."));
    }

    info_panel_done();

    delete max_utype;
  } else {
    set_topic_other(topic, title);
  }
}

/**********************************************************************//**
  Creates improvement help pages.
**************************************************************************/
void help_widget::set_topic_building(const help_item *topic,
                                     const char *title)
{
  char buffer[MAX_HELP_TEXT_SIZE];
  int type, value;
  struct sprite *spr;
  struct impr_type *itype = improvement_by_translated_name(title);
  char req_buf[512];
  QString str, s1, s2;
  QLabel *tb;

  if (itype) {
    helptext_building(buffer, sizeof(buffer), client.conn.playing,
                      topic->text, itype);
    text_browser->setText(buffer);
    show_info_panel();
    spr = get_building_sprite(tileset, itype);
    if (spr) {
      add_info_pixmap(spr->pm);
    }
    str = _("Cost:");
    str = "<b>" + str + "</b>" + " "
          + QString::number(impr_build_shield_cost(NULL, itype));
    add_info_label(str);
    if (!is_great_wonder(itype)) {
      str = _("Upkeep:");
      str = "<b>" + str + "</b>" + " "
            + QString::number(itype->upkeep);
      add_info_label(str);
    }

    requirement_vector_iterate(&itype->reqs, preq) {
      if (!preq->present) {
        continue;
      }
      universal_extraction(&preq->source, &type, &value);
      if (type == VUT_ADVANCE) {
        s1 = link_me(universal_name_translation(&preq->source, req_buf,
                                       sizeof(req_buf)), HELP_TECH);
      } else if (type == VUT_GOVERNMENT) {
        s1 = link_me(universal_name_translation(&preq->source, req_buf,
                                       sizeof(req_buf)), HELP_GOVERNMENT);
      } else if (type == VUT_TERRAIN) {
        s1 = link_me(universal_name_translation(&preq->source, req_buf,
                                       sizeof(req_buf)), HELP_TERRAIN);
      }
      break;
    } requirement_vector_iterate_end;

    if (s1.isEmpty() == false) {
      tb = new QLabel(this);
      str = _("Requirement:");
      str = "<b>" + str + "</b> " + s1;
      tb->setProperty(fonts::help_label, "true");
      tb->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
      tb->setTextFormat(Qt::RichText);
      tb->setText(str.trimmed());
      connect(tb, &QLabel::linkActivated,
              this, &help_widget::anchor_clicked);
      info_layout->addWidget(tb);
    }

    requirement_vector_iterate(&itype->obsolete_by, pobs) {
      if (pobs->source.kind == VUT_ADVANCE) {
        s2 = link_me(advance_name_translation(pobs->source.value.advance),
                     HELP_TECH);
        break;
      }
    } requirement_vector_iterate_end;

    str = _("Obsolete by:");
    str = "<b>" + str + "</b> " + s2;
    if (s2.isEmpty() == false) {
      tb = new QLabel(this);
      tb->setProperty(fonts::help_label, "true");
      tb->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
      tb->setTextFormat(Qt::RichText);
      tb->setText(str.trimmed());
      connect(tb, &QLabel::linkActivated,
              this, &help_widget::anchor_clicked);
      info_layout->addWidget(tb);
    }
    info_panel_done();
  } else {
    set_topic_other(topic, title);
  }
}

/**********************************************************************//**
  Creates technology help pages.
**************************************************************************/
void help_widget::set_topic_tech(const help_item *topic,
                                 const char *title)
{
  char buffer[MAX_HELP_TEXT_SIZE];
  struct sprite *spr;
  QLabel *tb;
  struct advance *padvance = advance_by_translated_name(title);
  QString str;

  if (padvance) {
    int n = advance_number(padvance);
    if (!is_future_tech(n)) {

      show_info_panel();
      spr = get_tech_sprite(tileset, n);
      if (spr) {
        add_info_pixmap(spr->pm);
      }

      governments_iterate(pgov) {
        requirement_vector_iterate(&pgov->reqs, preq) {
          if (VUT_ADVANCE == preq->source.kind
              && preq->source.value.advance == padvance) {
            tb = new QLabel(this);
            tb->setProperty(fonts::help_label, "true");
            tb->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
            tb->setTextFormat(Qt::RichText);
            str = _("Allows");
            str = "<b>" + str + "</b> "
               + link_me(government_name_translation(pgov), HELP_GOVERNMENT);
            tb->setText(str.trimmed());
            connect(tb, &QLabel::linkActivated,
                    this, &help_widget::anchor_clicked);
            info_layout->addWidget(tb);
          }
        } requirement_vector_iterate_end;
      } governments_iterate_end;

      improvement_iterate(pimprove) {
        requirement_vector_iterate(&pimprove->reqs, preq) {
          if (VUT_ADVANCE == preq->source.kind
              && preq->source.value.advance == padvance) {
            str = _("Allows");
            str = "<b>" + str + "</b> "
                  + link_me(improvement_name_translation(pimprove),
                            is_great_wonder(pimprove) ? HELP_WONDER
                             : HELP_IMPROVEMENT);
            tb = new QLabel(this);
            tb->setProperty(fonts::help_label, "true");
            tb->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
            tb->setTextFormat(Qt::RichText);
            tb->setText(str.trimmed());
            connect(tb, &QLabel::linkActivated,
                    this, &help_widget::anchor_clicked);
            info_layout->addWidget(tb);
          }
        } requirement_vector_iterate_end;

        requirement_vector_iterate(&pimprove->obsolete_by, pobs) {
          if (pobs->source.kind == VUT_ADVANCE
              && pobs->source.value.advance == padvance) {
            str = _("Obsoletes");
            str = "<b>" + str + "</b> "
                  + link_me(improvement_name_translation(pimprove),
                            is_great_wonder(pimprove) ? HELP_WONDER
                            : HELP_IMPROVEMENT);
            tb = new QLabel(this);
            tb->setProperty(fonts::help_label, "true");
            tb->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
            tb->setTextFormat(Qt::RichText);
            tb->setText(str.trimmed());
            connect(tb, &QLabel::linkActivated,
                    this, &help_widget::anchor_clicked);
            info_layout->addWidget(tb);
          }
        } requirement_vector_iterate_end;
      } improvement_iterate_end;

      unit_type_iterate(punittype) {
        if (padvance != punittype->require_advance) {
          continue;
        }
        str = _("Allows");
        str = "<b>" + str + "</b> "
              + link_me(utype_name_translation(punittype), HELP_UNIT);
        tb = new QLabel(this);
        tb->setProperty(fonts::help_label, "true");
        tb->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
        tb->setTextFormat(Qt::RichText);
        tb->setText(str.trimmed());
        connect(tb, &QLabel::linkActivated,
                this, &help_widget::anchor_clicked);
        info_layout->addWidget(tb);
      } unit_type_iterate_end;

      info_panel_done();
      helptext_advance(buffer, sizeof(buffer), client.conn.playing,
                       topic->text, n);
      text_browser->setText(buffer);

    }
  } else {
    set_topic_other(topic, title);
  }
}

/**********************************************************************//**
  Creates a terrain image on the given canvas.
**************************************************************************/
canvas *terrain_canvas(struct terrain *terrain,
                       const struct extra_type *resource,
                       enum extra_cause cause)
{
  struct canvas *canvas;
  struct drawn_sprite sprs[80];
  int canvas_y, count, i, width, height;
  struct extra_type *pextra;

  width = tileset_full_tile_width(tileset);
  height = tileset_full_tile_height(tileset);
  canvas_y = height - tileset_tile_height(tileset);

  canvas = qtg_canvas_create(width, height);
  canvas->map_pixmap.fill(Qt::transparent);
  for (i = 0; i < 3; ++i) {
    count = fill_basic_terrain_layer_sprite_array(tileset, sprs,
                                                  i, terrain);
    put_drawn_sprites(canvas, 1.0f, 0, canvas_y, count, sprs, false);
  }

  pextra = NULL;
  if (cause != EC_COUNT) {
    extra_type_by_cause_iterate(cause, e) {
      pextra = e;
      break;
    } extra_type_by_cause_iterate_end;

    count = fill_basic_extra_sprite_array(tileset, sprs, pextra);
    put_drawn_sprites(canvas, 1.0f, 0, canvas_y, count, sprs, false);
  }

  if (resource != NULL) {
    count = fill_basic_extra_sprite_array(tileset, sprs, resource);
    put_drawn_sprites(canvas, 1.0f, 0, canvas_y, count, sprs, false);
  }

  return canvas;
}

/**********************************************************************//**
  Creates a terrain widget with title, terrain image, legend. An optional
  tooltip can be given to explain the legend.
**************************************************************************/
QLayout *help_widget::create_terrain_widget(const QString &title,
                                            const struct canvas *image,
                                            const QString &legend,
                                            const QString &tooltip)
{
  QGraphicsDropShadowEffect *effect;
  QLabel *label;
  QGridLayout *layout = new QGridLayout();

  label = new QLabel();
  effect = new QGraphicsDropShadowEffect(label);
  effect->setBlurRadius(3);
  effect->setOffset(0, 2);
  label->setGraphicsEffect(effect);
  label->setPixmap(image->map_pixmap);
  layout->addWidget(label, 0, 0, 2, 1);

  label = new QLabel(title);
  layout->addWidget(label, 0, 1, Qt::AlignBottom);
  label->setProperty(fonts::help_title, "true");

  label = new QLabel(legend);
  layout->addWidget(label, 1, 1, Qt::AlignTop);
  label->setProperty(fonts::help_label, "true");

  if (!tooltip.isEmpty()) {
    label->setToolTip(tooltip);
    label->setCursor(Qt::WhatsThisCursor);
  }

  layout->setColumnStretch(0, 0);
  layout->setColumnStretch(1, 100);

  return layout;
}

/**********************************************************************//**
  Creates terrain help pages.
**************************************************************************/
void help_widget::set_topic_terrain(const help_item *topic,
                                      const char *title)
{
  char buffer[MAX_HELP_TEXT_SIZE];
  struct terrain *pterrain, *max;
  canvas *canvas;
  QVBoxLayout *vbox;
  bool show_panel = false;
  QScrollArea *area;
  QWidget *panel;
  QString str;

  pterrain = terrain_by_translated_name(title);
  if (pterrain) {
    struct universal for_terr;

    for_terr.kind = VUT_TERRAIN;
    for_terr.value.terrain = pterrain;

    helptext_terrain(buffer, sizeof(buffer), client.conn.playing,
                     topic->text, pterrain);
    text_browser->setText(buffer);

    // Create information panel
    show_info_panel();
    max = terrain_max_values();

    // Create terrain icon. Use shadow to help distinguish terrain.
    canvas = terrain_canvas(pterrain);
    add_info_pixmap(&canvas->map_pixmap, true);
    qtg_canvas_free(canvas);

    add_info_progress(_("Food:"), pterrain->output[O_FOOD],
                      0, max->output[O_FOOD]);
    add_info_progress(_("Production:"), pterrain->output[O_SHIELD],
                      0, max->output[O_SHIELD]);
    add_info_progress(_("Trade:"), pterrain->output[O_TRADE],
                      0, max->output[O_TRADE]);

    add_info_separator();

    add_info_progress(_("Move cost:"), pterrain->movement_cost,
                      0, max->movement_cost);
    add_info_progress(_("Defense bonus:"), MIN(100, pterrain->defense_bonus),
                      0, 100,
                      // TRANS: Display a percentage, eg "50%".
                      QString(_("%1%")).arg(pterrain->defense_bonus));

    add_info_separator();

    if (pterrain->irrigation_result != pterrain
        && pterrain->irrigation_result != T_NONE
        && pterrain->irrigation_time != 0
        && univs_have_action_enabler(ACTION_IRRIGATE_TF, NULL, &for_terr)) {
      QLabel *tb;
      char buffer[1024];

      tb = new QLabel(this);
      fc_snprintf(buffer, sizeof(buffer), PL_("%d turn", "%d turns",
                                              pterrain->irrigation_time),
                  pterrain->irrigation_time);
      str = N_("Irrig. Rslt/Time:");;
      str = str + link_me(terrain_name_translation(pterrain->irrigation_result),
                          HELP_TERRAIN)
            + QString(buffer);
      tb = new QLabel(this);
      tb->setProperty(fonts::help_label, "true");
      tb->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
      tb->setTextFormat(Qt::RichText);
      tb->setText(str.trimmed());
      connect(tb, &QLabel::linkActivated,
              this, &help_widget::anchor_clicked);
      info_layout->addWidget(tb);
    }

    if (pterrain->mining_result != pterrain
        && pterrain->mining_result != T_NONE
        && pterrain->mining_time != 0
        && univs_have_action_enabler(ACTION_MINE_TF, NULL, &for_terr)) {
      QLabel *tb;
      char buffer[1024];

      tb = new QLabel(this);
      fc_snprintf(buffer, sizeof(buffer), PL_("%d turn", "%d turns",
                                              pterrain->mining_time),
                  pterrain->mining_time);
      str = N_("Mine Rslt/Time:");;
      str = str + link_me(terrain_name_translation(pterrain->mining_result),
                          HELP_TERRAIN)
            + QString(buffer);
      tb = new QLabel(this);
      tb->setProperty(fonts::help_label, "true");
      tb->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
      tb->setTextFormat(Qt::RichText);
      tb->setText(str.trimmed());
      connect(tb, &QLabel::linkActivated,
              this, &help_widget::anchor_clicked);
      info_layout->addWidget(tb);
    }

    if (pterrain->transform_result != T_NONE
        && pterrain->transform_time != 0
        && univs_have_action_enabler(ACTION_TRANSFORM_TERRAIN, NULL, &for_terr)) {
      QLabel *tb;
      char buffer[1024];

      tb = new QLabel(this);
      fc_snprintf(buffer, sizeof(buffer), PL_("%d turn", "%d turns",
                                              pterrain->transform_time),
                  pterrain->transform_time);
      str = N_("Trans. Rslt/Time:");
      str = str + link_me(terrain_name_translation(pterrain->transform_result),
                          HELP_TERRAIN)
            + QString(buffer);
      tb = new QLabel(this);
      tb->setProperty(fonts::help_label, "true");
      tb->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
      tb->setTextFormat(Qt::RichText);
      tb->setText(str.trimmed());
      connect(tb, &QLabel::linkActivated,
              this, &help_widget::anchor_clicked);
      info_layout->addWidget(tb);
    }

    if (pterrain->irrigation_result == pterrain
        && pterrain->irrigation_time != 0
        && univs_have_action_enabler(ACTION_IRRIGATE, NULL, &for_terr)) {
      add_extras_of_act_for_terrain(pterrain, ACTIVITY_IRRIGATE, _("Build as irrigation"));
    }
    if (pterrain->mining_result == pterrain
        && pterrain->mining_time != 0
        && univs_have_action_enabler(ACTION_MINE, NULL, &for_terr)) {
      add_extras_of_act_for_terrain(pterrain, ACTIVITY_MINE, _("Build as mine"));
    }
    if (pterrain->road_time != 0) {
      add_extras_of_act_for_terrain(pterrain, ACTIVITY_GEN_ROAD, _("Build as road"));
    }
    if (pterrain->base_time != 0) {
      add_extras_of_act_for_terrain(pterrain, ACTIVITY_BASE, _("Build as base"));
    }

    info_panel_done();

    // Create bottom widget
    panel = new QWidget();
    vbox = new QVBoxLayout(panel);

    if (*(pterrain->resources)) {
      struct extra_type **r;

      for (r = pterrain->resources; *r; r++) {
        canvas = terrain_canvas(pterrain, *r);
        vbox->addLayout(create_terrain_widget(
          extra_name_translation(*r),
          canvas,
          // TRANS: %1 food, %2 shields, %3 trade
          QString(_("Tile output becomes %1, %2, %3."))
            .arg(pterrain->output[O_FOOD]   + (*r)->data.resource->output[O_FOOD])
            .arg(pterrain->output[O_SHIELD] + (*r)->data.resource->output[O_SHIELD])
            .arg(pterrain->output[O_TRADE]  + (*r)->data.resource->output[O_TRADE]),
          // TRANS: Tooltip decorating strings like "1, 2, 3".
          _("Output (Food, Shields, Trade) of a tile where the resource is "
            "present.")));
        qtg_canvas_free(canvas);
        show_panel = true;
      }
    }

    vbox->addStretch(100);
    vbox->setSizeConstraint(QLayout::SetMinimumSize);
    if (show_panel) {
      area = new QScrollArea();
      area->setWidget(panel);
      set_bottom_panel(area);
    } else {
      panel->deleteLater();
    }

    delete max;
  } else {
    set_topic_other(topic, title);
  }
}

/**********************************************************************//**
  Creates extra help pages.
**************************************************************************/
void help_widget::set_topic_extra(const help_item *topic,
                                    const char *title)
{
  char buffer[MAX_HELP_TEXT_SIZE];
  struct extra_type *pextra = extra_type_by_translated_name(title);
  if (pextra) {
    helptext_extra(buffer, sizeof(buffer), client.conn.playing,
                  topic->text, pextra);
    text_browser->setText(buffer);
  } else {
    set_topic_other(topic, title);
  }
}

/**********************************************************************//**
  Creates specialist help pages.
**************************************************************************/
void help_widget::set_topic_specialist(const help_item *topic,
                                         const char *title)
{
  char buffer[MAX_HELP_TEXT_SIZE];
  struct specialist *pspec = specialist_by_translated_name(title);
  if (pspec) {
    helptext_specialist(buffer, sizeof(buffer), client.conn.playing,
                        topic->text, pspec);
    text_browser->setText(buffer);
  } else {
    set_topic_other(topic, title);
  }
}

/**********************************************************************//**
  Creates government help pages.
**************************************************************************/
void help_widget::set_topic_government(const help_item *topic,
                                         const char *title)
{
  char buffer[MAX_HELP_TEXT_SIZE];
  struct government *pgov = government_by_translated_name(title);
  if (pgov) {
    helptext_government(buffer, sizeof(buffer), client.conn.playing,
                        topic->text, pgov);
    text_browser->setText(buffer);
  } else {
    set_topic_other(topic, title);
  }
}

/**********************************************************************//**
  Creates nation help pages.
**************************************************************************/
void help_widget::set_topic_nation(const help_item *topic,
                                     const char *title)
{
  char buffer[MAX_HELP_TEXT_SIZE];
  struct nation_type *pnation = nation_by_translated_plural(title);
  if (pnation) {
    helptext_nation(buffer, sizeof(buffer), pnation, topic->text);
    text_browser->setText(buffer);
  } else {
    set_topic_other(topic, title);
  }
}

/**********************************************************************//**
  Creates goods help page.
**************************************************************************/
void help_widget::set_topic_goods(const help_item* topic,
                                  const char* title)
{
  char buffer[MAX_HELP_TEXT_SIZE];
  struct goods_type *pgood = goods_by_translated_name(title);
  if (pgood) {
    helptext_goods(buffer, sizeof(buffer), client.conn.playing,
                   topic->text, pgood);
    text_browser->setText(buffer);
  } else {
    set_topic_other(topic, title);
  }
}

/**********************************************************************//**
  Retrieves the maximum values any terrain will ever have.
  Supported fields:
    base_time, clean_fallout_time, clean_pollution_time, defense_bonus,
    irrigation_food_incr, irrigation_time, mining_shield_incr, mining_time,
    movement_cost, output, pillage_time, road_output_incr_pct, road_time,
    transform_time
  Other fields in returned value are undefined. Especially, all pointers are
  invalid.
**************************************************************************/
struct terrain *help_widget::terrain_max_values()
{
  Terrain_type_id i, count;
  struct terrain *terrain;
  struct terrain *max = new struct terrain;
  max->base_time = 0;
  max->clean_fallout_time = 0;
  max->clean_pollution_time = 0;
  max->defense_bonus = 0;
  max->irrigation_food_incr = 0;
  max->irrigation_time = 0;
  max->mining_shield_incr = 0;
  max->mining_time = 0;
  max->movement_cost = 0;
  max->output[O_FOOD] = 0;
  max->output[O_GOLD] = 0;
  max->output[O_LUXURY] = 0;
  max->output[O_SCIENCE] = 0;
  max->output[O_SHIELD] = 0;
  max->output[O_TRADE] = 0;
  max->pillage_time = 0;
  max->road_output_incr_pct[O_FOOD] = 0;
  max->road_output_incr_pct[O_GOLD] = 0;
  max->road_output_incr_pct[O_LUXURY] = 0;
  max->road_output_incr_pct[O_SCIENCE] = 0;
  max->road_output_incr_pct[O_SHIELD] = 0;
  max->road_output_incr_pct[O_TRADE] = 0;
  max->road_time = 0;
  max->transform_time = 0;
  count = terrain_count();
  for (i = 0; i < count; ++i) {
    terrain = terrain_by_number(i);
#define SET_MAX(v) \
    max->v = max->v > terrain->v ? max->v : terrain->v
    SET_MAX(base_time);
    SET_MAX(clean_fallout_time);
    SET_MAX(clean_pollution_time);
    SET_MAX(defense_bonus);
    SET_MAX(irrigation_food_incr);
    SET_MAX(irrigation_time);
    SET_MAX(mining_shield_incr);
    SET_MAX(mining_time);
    SET_MAX(movement_cost);
    SET_MAX(output[O_FOOD]);
    SET_MAX(output[O_GOLD]);
    SET_MAX(output[O_LUXURY]);
    SET_MAX(output[O_SCIENCE]);
    SET_MAX(output[O_SHIELD]);
    SET_MAX(output[O_TRADE]);
    SET_MAX(pillage_time);
    SET_MAX(road_output_incr_pct[O_FOOD]);
    SET_MAX(road_output_incr_pct[O_GOLD]);
    SET_MAX(road_output_incr_pct[O_LUXURY]);
    SET_MAX(road_output_incr_pct[O_SCIENCE]);
    SET_MAX(road_output_incr_pct[O_SHIELD]);
    SET_MAX(road_output_incr_pct[O_TRADE]);
    SET_MAX(road_time);
    SET_MAX(transform_time);
#undef SET_MAX
  }
  return max;
}

/**********************************************************************//**
  Retrieves the maximum values any unit of uclass will ever have.
  Supported fields:
    attack_strength, bombard_rate, build_cost, city_size, convert_time,
    defense_strength, firepower, fuel, happy_cost, hp, move_rate, pop_cost,
    upkeep, vision_radius_sq
  Other fiels in returned value are undefined. Especially, all pointers are
  invalid except uclass.
**************************************************************************/
struct unit_type *help_widget::uclass_max_values(struct unit_class *uclass)
{
  struct unit_type *max = new struct unit_type;
  max->uclass = uclass;
  max->attack_strength = 0;
  max->bombard_rate = 0;
  max->build_cost = 0;
  max->city_size = 0;
  max->defense_strength = 0;
  max->firepower = 0;
  max->fuel = 0;
  max->happy_cost = 0;
  max->hp = 0;
  max->move_rate = 0;
  max->pop_cost = 0;
  max->upkeep[O_FOOD] = 0;
  max->upkeep[O_GOLD] = 0;
  max->upkeep[O_LUXURY] = 0;
  max->upkeep[O_SCIENCE] = 0;
  max->upkeep[O_SHIELD] = 0;
  max->upkeep[O_TRADE] = 0;
  max->vision_radius_sq = 0;
  unit_type_iterate(utype) {
    if (utype->uclass == uclass) {
#define SET_MAX(v) \
      max->v = max->v > utype->v ? max->v : utype->v
      SET_MAX(attack_strength);
      SET_MAX(bombard_rate);
      SET_MAX(build_cost);
      SET_MAX(city_size);
      SET_MAX(convert_time);
      SET_MAX(defense_strength);
      SET_MAX(firepower);
      SET_MAX(fuel);
      SET_MAX(happy_cost);
      SET_MAX(hp);
      SET_MAX(move_rate);
      SET_MAX(pop_cost);
      SET_MAX(upkeep[O_FOOD]);
      SET_MAX(upkeep[O_GOLD]);
      SET_MAX(upkeep[O_LUXURY]);
      SET_MAX(upkeep[O_SCIENCE]);
      SET_MAX(upkeep[O_SHIELD]);
      SET_MAX(upkeep[O_TRADE]);
      SET_MAX(vision_radius_sq);
#undef SET_MAX
    }
  } unit_type_iterate_end
  return max;
}
