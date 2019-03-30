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
#include <QCloseEvent>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

// utility
#include "fc_cmdline.h"
#include "fciconv.h"
#include "fcintl.h"
#include "log.h"

// common
#include "version.h"

// tools
#include "download.h"
#include "mpcmdline.h"
#include "mpdb.h"
#include "mpgui_qt_worker.h"
#include "modinst.h"

#include "mpgui_qt.h"

struct fcmp_params fcmp = { MODPACK_LIST_URL, NULL, NULL };

static mpgui *gui;

static mpqt_worker *worker = nullptr;

static int mpcount = 0;

#define ML_COL_NAME    0
#define ML_COL_VER     1
#define ML_COL_INST    2
#define ML_COL_TYPE    3
#define ML_COL_SUBTYPE 4
#define ML_COL_LIC     5
#define ML_COL_URL     6

#define ML_TYPE        7

#define ML_COL_COUNT   8

static void setup_modpack_list(const char *name, const char *URL,
                               const char *version, const char *license,
                               enum modpack_type type, const char *subtype,
                               const char *notes);
static void msg_callback(const char *msg);
static void msg_callback_thr(const char *msg);
static void progress_callback_thr(int downloaded, int max);

static void gui_download_modpack(QString URL);

/**********************************************************************//**
  Entry point for whole freeciv-mp-qt program.
**************************************************************************/
int main(int argc, char **argv)
{
  int ui_options;

  fcmp_init();

  /* This modifies argv! */
  ui_options = fcmp_parse_cmdline(argc, argv);

  if (ui_options != -1) {
    int i;

    for (i = 1; i <= ui_options; i++) {
      if (is_option("--help", argv[i])) {
        fc_fprintf(stderr,
             _("This modpack installer accepts the standard Qt command-line options\n"
               "after '--'. See the Qt documentation.\n\n"));

        /* TRANS: No full stop after the URL, could cause confusion. */
        fc_fprintf(stderr, _("Report bugs at %s\n"), BUG_URL);

        ui_options = -1;
      }
    }
  }

  if (ui_options != -1) {
    QApplication *qapp;
    mpgui_main *main_window;
    QWidget *central;
    const char *errmsg;

    load_install_info_lists(&fcmp);

    qapp = new QApplication(ui_options, argv);
    central = new QWidget;
    main_window = new mpgui_main(qapp, central);

    main_window->setGeometry(0, 30, 640, 60);
    main_window->setWindowTitle(QString::fromUtf8(_("Freeciv modpack installer (Qt)")));

    gui = new mpgui;

    gui->setup(central, &fcmp);

    main_window->setCentralWidget(central);
    main_window->setVisible(true);

    errmsg = download_modpack_list(&fcmp, setup_modpack_list, msg_callback);
    if (errmsg != nullptr) {
      gui->display_msg(errmsg);
    }

    qapp->exec();

    if (worker != nullptr) {
      if (worker->isRunning()) {
        worker->wait();
      }
      delete worker;
    }

    delete gui;
    delete qapp;

    close_mpdbs();
  }

  fcmp_deinit();
  cmdline_option_values_free();

  return EXIT_SUCCESS;
}

/**********************************************************************//**
  Progress indications from downloader
**************************************************************************/
static void msg_callback(const char *msg)
{
  gui->display_msg(msg);
}

/**********************************************************************//**
  Progress indications from downloader thread
**************************************************************************/
static void msg_callback_thr(const char *msg)
{
  gui->display_msg_thr(msg);
}

/**********************************************************************//**
  Progress indications from downloader
**************************************************************************/
static void progress_callback_thr(int downloaded, int max)
{
  gui->progress_thr(downloaded, max);
}

/**********************************************************************//**
  Setup GUI object
**************************************************************************/
void mpgui::setup(QWidget *central, struct fcmp_params *params)
{
#define URL_LABEL_TEXT N_("Modpack URL")
  QVBoxLayout *main_layout = new QVBoxLayout();
  QHBoxLayout *hl = new QHBoxLayout();
  QPushButton *install_button = new QPushButton(QString::fromUtf8(_("Install modpack")));
  QStringList headers;
  QLabel *URL_label;
  QLabel *version_label;
  char verbuf[2048];
  const char *rev_ver;

  rev_ver = fc_git_revision();

  if (rev_ver == nullptr) {
    fc_snprintf(verbuf, sizeof(verbuf), "%s%s", word_version(), VERSION_STRING);
  } else {
    fc_snprintf(verbuf, sizeof(verbuf), _("%s%s\ncommit: %s"),
                word_version(), VERSION_STRING, rev_ver);
  }

  version_label = new QLabel(QString::fromUtf8(verbuf));
  version_label->setAlignment(Qt::AlignHCenter);
  version_label->setParent(central);
  version_label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
  main_layout->addWidget(version_label);

  mplist_table = new QTableWidget();
  mplist_table->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
  mplist_table->setColumnCount(ML_COL_COUNT);
  headers << QString::fromUtf8(_("Name")) << QString::fromUtf8(_("Version"));
  headers << QString::fromUtf8(_("Installed")) << QString::fromUtf8(Q_("?modpack:Type"));
  headers << QString::fromUtf8(_("Subtype")) << QString::fromUtf8(_("License"));
  headers << QString::fromUtf8(_("URL")) << "typeint";
  mplist_table->setHorizontalHeaderLabels(headers);
  mplist_table->verticalHeader()->setVisible(false);
  mplist_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  mplist_table->setSelectionBehavior(QAbstractItemView::SelectRows);
  mplist_table->setSelectionMode(QAbstractItemView::SingleSelection);
  mplist_table->hideColumn(ML_TYPE);

  connect(mplist_table, SIGNAL(cellClicked(int, int)), this, SLOT(row_selected(int, int)));
  connect(mplist_table, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(row_download(QModelIndex)));
  connect(this, SIGNAL(display_msg_thr_signal(const char *)), this, SLOT(display_msg(const char *)));
  connect(this, SIGNAL(progress_thr_signal(int, int)), this, SLOT(progress(int, int)));
  connect(this, SIGNAL(refresh_list_versions_thr_signal()), this, SLOT(refresh_list_versions()));

  main_layout->addWidget(mplist_table);

  URL_label = new QLabel(QString::fromUtf8(_(URL_LABEL_TEXT)));
  URL_label->setParent(central);
  hl->addWidget(URL_label);

  URLedit = new QLineEdit(central);
  if (params->autoinstall == nullptr) {
    URLedit->setText(DEFAULT_URL_START);
  } else {
    URLedit->setText(QString::fromUtf8(params->autoinstall));
  }
  URLedit->setFocus();

  connect(URLedit, SIGNAL(returnPressed()), this, SLOT(URL_given()));

  hl->addWidget(URLedit);
  main_layout->addLayout(hl);

  connect(install_button, SIGNAL(pressed()), this, SLOT(URL_given()));
  main_layout->addWidget(install_button);

  bar = new QProgressBar(central);
  main_layout->addWidget(bar);

  msg_dspl = new QLabel(QString::fromUtf8(_("Select modpack to install")));
  msg_dspl->setParent(central);
  main_layout->addWidget(msg_dspl);

  msg_dspl->setAlignment(Qt::AlignHCenter);

  central->setLayout(main_layout); 
}

/**********************************************************************//**
  Display status message
**************************************************************************/
void mpgui::display_msg(const char *msg)
{
  log_verbose("%s", msg);
  msg_dspl->setText(QString::fromUtf8(msg));
}

/**********************************************************************//**
  Display status message from another thread
**************************************************************************/
void mpgui::display_msg_thr(const char *msg)
{
  emit display_msg_thr_signal(msg);
}

/**********************************************************************//**
  Update progress bar
**************************************************************************/
void mpgui::progress(int downloaded, int max)
{
  bar->setMaximum(max);
  bar->setValue(downloaded);
}

/**********************************************************************//**
  Update progress bar from another thread
**************************************************************************/
void mpgui::progress_thr(int downloaded, int max)
{
  emit progress_thr_signal(downloaded, max);
}

/**********************************************************************//**
  Download modpack from given URL
**************************************************************************/
static void gui_download_modpack(QString URL)
{
  if (worker != nullptr) {
    if (worker->isRunning()) {
      gui->display_msg(_("Another download already active"));
      return;
    }
  } else {
    worker = new mpqt_worker;
  }

  worker->download(URL, gui, &fcmp, msg_callback_thr, progress_callback_thr);
}

/**********************************************************************//**
  User entered URL
**************************************************************************/
void mpgui::URL_given()
{
  gui_download_modpack(URLedit->text());
}

/**********************************************************************//**
  Refresh display of modpack list modpack versions
**************************************************************************/
void mpgui::refresh_list_versions()
{
  for (int i = 0; i < mpcount; i++) {
    QString name_str;
    int type_int;
    const char *new_inst;
    enum modpack_type type;

    name_str = mplist_table->item(i, ML_COL_NAME)->text();
    type_int = mplist_table->item(i, ML_TYPE)->text().toInt();
    type = (enum modpack_type) type_int;
    new_inst = mpdb_installed_version(name_str.toUtf8().data(), type);

    if (new_inst == nullptr) {
      new_inst = _("Not installed");
    }

    mplist_table->item(i, ML_COL_INST)->setText(QString::fromUtf8(new_inst));
  }

  mplist_table->resizeColumnsToContents();
}

/**********************************************************************//**
  Refresh display of modpack list modpack versions from another thread
**************************************************************************/
void mpgui::refresh_list_versions_thr()
{
  emit refresh_list_versions_thr_signal();
}

/**********************************************************************//**
  Build main modpack list view
**************************************************************************/
void mpgui::setup_list(const char *name, const char *URL,
                       const char *version, const char *license,
                       enum modpack_type type, const char *subtype,
                       const char *notes)
{
  const char *type_str;
  const char *lic_str;
  const char *inst_str;
  QString type_nbr;
  QTableWidgetItem *item;

  if (modpack_type_is_valid(type)) {
    type_str = _(modpack_type_name(type));
  } else {
    /* TRANS: Unknown modpack type */
    type_str = _("?");
  }

  if (license != nullptr) {
    lic_str = license;
  } else {
    /* TRANS: License of modpack is not known */
    lic_str = Q_("?license:Unknown");
  }

  inst_str = mpdb_installed_version(name, type);
  if (inst_str == nullptr) {
    inst_str = _("Not installed");
  }

  mplist_table->setRowCount(mpcount+1);

  item = new QTableWidgetItem(QString::fromUtf8(name));
  item->setToolTip(QString::fromUtf8(notes));
  mplist_table->setItem(mpcount, ML_COL_NAME, item);
  item = new QTableWidgetItem(QString::fromUtf8(version));
  item->setToolTip(QString::fromUtf8(notes));
  mplist_table->setItem(mpcount, ML_COL_VER, item);
  item = new QTableWidgetItem(QString::fromUtf8(inst_str));
  item->setToolTip(QString::fromUtf8(notes));
  mplist_table->setItem(mpcount, ML_COL_INST, item);
  item = new QTableWidgetItem(QString::fromUtf8(type_str));
  item->setToolTip(QString::fromUtf8(notes));
  mplist_table->setItem(mpcount, ML_COL_TYPE, item);
  item = new QTableWidgetItem(QString::fromUtf8(subtype));
  item->setToolTip(QString::fromUtf8(notes));
  mplist_table->setItem(mpcount, ML_COL_SUBTYPE, item);
  item = new QTableWidgetItem(QString::fromUtf8(lic_str));
  item->setToolTip(QString::fromUtf8(notes));
  mplist_table->setItem(mpcount, ML_COL_LIC, item);
  item = new QTableWidgetItem(QString::fromUtf8(URL));
  item->setToolTip(QString::fromUtf8(notes));
  mplist_table->setItem(mpcount, ML_COL_URL, item);

  type_nbr.setNum(type);
  item = new QTableWidgetItem(type_nbr);
  item->setToolTip(notes);
  mplist_table->setItem(mpcount, ML_TYPE, item);

  mplist_table->resizeColumnsToContents();
  mpcount++;
}

/**********************************************************************//**
  Build main modpack list view
**************************************************************************/
static void setup_modpack_list(const char *name, const char *URL,
                               const char *version, const char *license,
                               enum modpack_type type, const char *subtype,
                               const char *notes)
{
  // Just call setup_list for gui singleton
  gui->setup_list(name, URL, version, license, type, subtype, notes);
}

/**********************************************************************//**
  User activated another table row
**************************************************************************/
void mpgui::row_selected(int row, int column)
{
  QString URL = mplist_table->item(row, ML_COL_URL)->text();

  URLedit->setText(URL);
}

/**********************************************************************//**
  User activated another table row
**************************************************************************/
void mpgui::row_download(const QModelIndex &index)
{
  QString URL = mplist_table->item(index.row(), ML_COL_URL)->text();

  URLedit->setText(URL);

  URL_given();
}

/**********************************************************************//**
  Main window constructor
**************************************************************************/
mpgui_main::mpgui_main(QApplication *qapp_in, QWidget *central_in) : QMainWindow()
{
  qapp = qapp_in;
  central = central_in;
}

/**********************************************************************//**
  Open dialog to confirm that user wants to quit modpack installer.
**************************************************************************/
void mpgui_main::popup_quit_dialog()
{
  QMessageBox ask(central);
  int ret;

  ask.setText(_("Modpack installation in progress.\nAre you sure you want to quit?"));
  ask.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
  ask.setDefaultButton(QMessageBox::Cancel);
  ask.setIcon(QMessageBox::Warning);
  ask.setWindowTitle(_("Quit?"));
  ret = ask.exec();

  switch (ret) {
  case QMessageBox::Cancel:
    return;
    break;
  case QMessageBox::Ok:
    qapp->quit();
    break;
  }
}

/**********************************************************************//**
  User clicked windows close button.
**************************************************************************/
void mpgui_main::closeEvent(QCloseEvent *event)
{
  if (worker != nullptr && worker->isRunning()) {
    // Download in progress. Confirm quit from user.
    popup_quit_dialog();
    event->ignore();
  }
}
