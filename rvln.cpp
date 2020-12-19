/***************************************************************************
 *   2007-2013 by Peter Semiletov                            *
 *   tea@list.ru                                             *

started at 08 November 2007
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/




#include "rvln.h"

#include "utils.h"
#include "gui_utils.h"
#include "libretta_calc.h"

#include "textproc.h"
#include "logmemo.h"
#include "tzipper.h"
#include "wavinfo.h"
#include <math.h>

#include <QMimeData>
#include <QStyleFactory>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QToolBar>
#include <QClipboard>
#include <QFileDialog>
#include <QMenuBar>
#include <QGroupBox>
#include <QImageWriter>
#include <QDesktopServices>
#include <QColorDialog>


#include <QPainter>
#include <QInputDialog>
#include <QSettings>
#include <QLibraryInfo>
#include <QCryptographicHash>


#ifdef PRINTER_ENABLE

#include <QPrinter>
#include <QPrintDialog>
#include <QAbstractPrintDialog>

#endif



#ifdef SPELLCHECKER_ENABLE

#include "spellchecker.h"

#endif




class CFSizeFName: public QObject
{
public:

  qint64 size;
  QString fname;

  CFSizeFName (qint64 sz, const QString &fn):
               size (sz),
               fname (fn)
               {
               }
};


extern QSettings *settings;
QFontDatabase *font_database;

enum {
      FM_ENTRY_MODE_NONE = 0,
      FM_ENTRY_MODE_OPEN,
      FM_ENTRY_MODE_SAVE
     };


QStringList bytearray_to_stringlist (QList<QByteArray> a)
{
  QStringList r;
  foreach (QByteArray i, a)
          r.append (i.data());

  return r;
}


//for the further compat.
QTabWidget::TabPosition int_to_tabpos (int i)
{
  QTabWidget::TabPosition p = QTabWidget::North;

  switch (i)
         {
          case 0:
                 p = QTabWidget::North;
                 break;
          case 1:
                 p = QTabWidget::South;
                 break;
          case 2:
                 p = QTabWidget::West;
                 break;
          case 3:
                 p = QTabWidget::East;
                 break;
         }

  return p;
}


void rvln::create_paths()
{
  portable_mode = false;

  QStringList l = qApp->arguments();
  if (l.contains ("--p"))
     portable_mode = true;

  QDir dr;
  if (! portable_mode)
     dir_config = dr.homePath();
  else
      dir_config = QCoreApplication::applicationDirPath();

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)

  dir_config.append ("/tea");

#else

  dir_config.append ("/.config/tea");

#endif

  dr.setPath (dir_config);
  if (! dr.exists())
     dr.mkpath (dir_config);


  fname_userfonts.append (dir_config).append ("/userfonts.txt");
  fname_crapbook.append (dir_config).append ("/crapbook.txt");
  fname_hls_cache.append (dir_config).append ("/hls_cache");
  fname_fif.append (dir_config).append ("/fif");
  fname_bookmarks.append (dir_config).append ("/tea_bmx");
  fname_programs.append (dir_config).append ("/programs");
  fname_places_bookmarks.append (dir_config).append ("/places_bookmarks");
  fname_tempfile.append (QDir::tempPath()).append ("/tea.tmp");
  fname_tempparamfile.append (QDir::tempPath()).append ("/teaparam.tmp");

  
  dir_tables.append (dir_config).append ("/tables");

  dr.setPath (dir_tables);
  if (! dr.exists())
     dr.mkpath (dir_tables);

  dir_user_dict.append (dir_config).append ("/dictionaries");

  dr.setPath (dir_user_dict);
  if (! dr.exists())
     dr.mkpath (dir_user_dict);

  dir_profiles.append (dir_config).append ("/profiles");

  dr.setPath (dir_profiles);
  if (! dr.exists())
     dr.mkpath (dir_profiles);

  dir_templates.append (dir_config).append ("/templates");

  dr.setPath (dir_templates);
  if (! dr.exists())
     dr.mkpath (dir_templates);

  dir_snippets.append (dir_config).append ("/snippets");

  dr.setPath (dir_snippets);
  if (! dr.exists())
     dr.mkpath (dir_snippets);

  dir_scripts.append (dir_config).append ("/scripts");

  dr.setPath (dir_scripts);
  if (! dr.exists())
     dr.mkpath (dir_scripts);

  dir_days.append (dir_config).append ("/days");

  dr.setPath (dir_days);
  if (! dr.exists())
     dr.mkpath (dir_days);


  dir_sessions.append (dir_config).append ("/sessions");

  dr.setPath (dir_sessions);
  if (! dr.exists())
     dr.mkpath (dir_sessions);

  dir_hls.append (dir_config).append ("/hls");

  dr.setPath (dir_hls);
  if (! dr.exists())
     dr.mkpath (dir_hls);

  dir_palettes.append (dir_config).append ("/palettes");

  dr.setPath (dir_palettes);
  if (! dr.exists())
     dr.mkpath (dir_palettes);
}


void rvln::update_bookmarks()
{
  if (! file_exists (fname_bookmarks))
     return;

  bookmarks = qstring_load (fname_bookmarks);
  if (bookmarks.isEmpty())
     return;

  menu_file_bookmarks->clear();
  create_menu_from_list (this, menu_file_bookmarks,
                         bookmarks.split ("\n"),
                         SLOT (file_open_bookmark()));
}


void rvln::readSettings()
{
  int ui_tab_align = settings->value ("ui_tabs_align", "0").toInt();
  main_tab_widget->setTabPosition (int_to_tabpos (ui_tab_align));

  int docs_tab_align = settings->value ("docs_tabs_align", "0").toInt();
  tab_widget->setTabPosition (int_to_tabpos (docs_tab_align));

  markup_mode = settings->value ("markup_mode", "HTML").toString();
  charset = settings->value ("charset", "UTF-8").toString();
  fname_def_palette = settings->value ("fname_def_palette", ":/palettes/TEA").toString();
  QPoint pos = settings->value ("pos", QPoint (1, 200)).toPoint();
  QSize size = settings->value ("size", QSize (600, 420)).toSize();
  mainSplitter->restoreState (settings->value ("splitterSizes").toByteArray());

#if QT_VERSION >= 0x050000

  QString default_style = qApp->style()->objectName();

  if (default_style == "GTK+") //can be buggy
     default_style = "Fusion";

#else

  QString default_style = qApp->style()->objectName();

  if (default_style == "GTK+") //can be buggy
     default_style = "Cleanlooks";


#endif


  QApplication::setStyle (QStyleFactory::create (settings->value ("ui_style", default_style).toString()));

  resize (size);
  move (pos);
}


void rvln::writeSettings()
{
  settings->setValue ("pos", pos());
  settings->setValue ("size", size());
  settings->setValue ("charset", charset);
  settings->setValue ("splitterSizes", mainSplitter->saveState());
  settings->setValue ("spl_fman", spl_fman->saveState());

  settings->setValue ("dir_last", dir_last);
  settings->setValue ("fname_def_palette", fname_def_palette);
  settings->setValue ("markup_mode", markup_mode);
  settings->setValue ("VERSION_NUMBER", QString (VERSION_NUMBER));
  settings->setValue ("state", saveState());

  delete settings;
}


void rvln::create_main_widget()
{
  QWidget *main_widget = new QWidget;
  QVBoxLayout *v_box = new QVBoxLayout;
  main_widget->setLayout (v_box);

  main_tab_widget = new QTabWidget;
  
  main_tab_widget->setTabShape (QTabWidget::Triangular);

  tab_widget = new QTabWidget;
  
#if QT_VERSION >= 0x040500
  tab_widget->setMovable (true);
#endif
  
  QPushButton *bt_close = new QPushButton ("X", this);
  connect (bt_close, SIGNAL(clicked()), this, SLOT(close_current()));
  tab_widget->setCornerWidget (bt_close);
  
  log_memo = new QPlainTextEdit;
  log = new CLogMemo (log_memo);

  mainSplitter = new QSplitter (Qt::Vertical);
  v_box->addWidget (mainSplitter);

  cmb_fif = new QComboBox;
  cmb_fif->setInsertPolicy (QComboBox::InsertAtTop); 

  cmb_fif->setEditable (true);
  fif = cmb_fif->lineEdit();
  connect (fif, SIGNAL(returnPressed()), this, SLOT(search_find()));

  main_tab_widget->setMinimumHeight (10);
  log_memo->setMinimumHeight (10);

  mainSplitter->addWidget (main_tab_widget);
  mainSplitter->addWidget (log_memo);

  QHBoxLayout *lt_fte = new QHBoxLayout;
  v_box->addLayout (lt_fte);

  QToolButton *bt_find = new QToolButton (this);
  QToolButton *bt_prev = new QToolButton (this);
  QToolButton *bt_next = new QToolButton (this);
  bt_next->setArrowType (Qt::RightArrow);
  bt_prev->setArrowType (Qt::LeftArrow);

  connect (bt_find, SIGNAL(clicked()), this, SLOT(search_find()));
  connect (bt_next, SIGNAL(clicked()), this, SLOT(search_find_next()));
  connect (bt_prev, SIGNAL(clicked()), this, SLOT(search_find_prev()));

  bt_find->setIcon (QIcon (":/icons/search_find.png"));

  lt_fte->addWidget (cmb_fif);
  lt_fte->addWidget (bt_find);
  lt_fte->addWidget (bt_prev);
  lt_fte->addWidget (bt_next);

  mainSplitter->setStretchFactor (1, 1);

  idx_tab_edit = main_tab_widget->addTab (tab_widget, tr ("edit"));
  setCentralWidget (main_widget);

  //tab_widget->resize (tab_widget->width(), width() - 100);
  connect (tab_widget, SIGNAL(currentChanged(int)), this, SLOT(pageChanged(int)));
}


#ifdef SPELLCHECK_ENABLE  

void rvln::setup_spellcheckers()
{
 
#ifdef ASPELL_ENABLE
  spellcheckers.append ("Aspell"); 
#endif

  
#ifdef HUNSPELL_ENABLE
  spellcheckers.append ("Hunspell"); 
#endif

  cur_spellchecker = settings->value ("cur_spellchecker", "Hunspell").toString();
  if (! spellcheckers.contains (cur_spellchecker))
     cur_spellchecker = spellcheckers[0]; 

  
#ifdef ASPELL_ENABLE
  if (cur_spellchecker == "Aspell")
     {
      QString lang = settings->value ("spell_lang", QLocale::system().name().left(2)).toString();

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)

      QString win32_aspell_path = settings->value ("win32_aspell_path", "C:\\Program Files\\Aspell").toString();
      spellchecker = new CSpellchecker (lang, win32_aspell_path);

#else

      spellchecker = new CSpellchecker (lang);

#endif

     }

#endif

  
#ifdef HUNSPELL_ENABLE
   if (cur_spellchecker == "Hunspell")
      spellchecker = new CHunspellChecker (settings->value ("spell_lang", QLocale::system().name().left(2)).toString(), settings->value ("hunspell_dic_path", QDir::homePath()).toString(), dir_user_dict);  
#endif

  
 create_spellcheck_menu();
}

#endif  


rvln::rvln()
{
  ui_update = true;

  lv_menuitems = NULL;
  fm_entry_mode = FM_ENTRY_MODE_NONE;
  
  date1 = QDate::currentDate();
  date2 = QDate::currentDate();
  
  idx_tab_edit = 0;
  idx_tab_tune = 0;
  idx_tab_fman = 0;
  idx_tab_learn = 0;
  idx_tab_calendar = 0;
  idx_tab_keyboard = 0;

  calendar = 0;

  capture_to_storage_file = false;

  font_database = new QFontDatabase();

  create_paths();
  load_userfonts();
  
  QString sfilename = dir_config + "/tea.conf";
  settings = new QSettings (sfilename, QSettings::IniFormat);

  if (settings->value ("override_locale", 0).toBool())
     {
      QString ts = settings->value ("override_locale_val", "en").toString();
      if (ts.length() != 2)
         ts = "en";
      
      qtTranslator.load (QString ("qt_%1").arg (ts),
                         QLibraryInfo::location (QLibraryInfo::TranslationsPath));
      qApp->installTranslator (&qtTranslator);

      myappTranslator.load (":/translations/tea_" + ts);
      qApp->installTranslator (&myappTranslator);
     }
  else
      {
       qtTranslator.load (QString ("qt_%1").arg (QLocale::system().name()),
                          QLibraryInfo::location (QLibraryInfo::TranslationsPath));
       qApp->installTranslator (&qtTranslator);

       myappTranslator.load (":/translations/tea_" + QLocale::system().name());
       qApp->installTranslator (&myappTranslator);
      }

    
  createActions();
  createMenus();
  createToolBars();
  createStatusBar();
  update_bookmarks();
  update_templates();
  update_tables();
  update_snippets();
  update_sessions();
  update_scripts();
  update_programs();
  update_palettes();
  update_charsets();
  update_profiles();
  create_markup_hash();
  create_moon_phase_algos();

  setMinimumSize (12, 12);
    

  create_main_widget();

  idx_prev = 0;
  connect (main_tab_widget, SIGNAL(currentChanged(int)), this, SLOT(main_tab_page_changed(int)));

  readSettings();
  read_search_options();
  
  documents = new document_holder();
  documents->parent_wnd = this;
  documents->tab_widget = tab_widget;
  documents->recent_menu = menu_file_recent;
  documents->recent_list_fname.append (dir_config).append ("/tea_recent");
  documents->reload_recent_list();
  documents->update_recent_menu();
  documents->log = log;
  documents->status_bar = statusBar();
  documents->markup_mode = markup_mode;
  documents->dir_config = dir_config;
  documents->todo.dir_days = dir_days;
  documents->fname_crapbook = fname_crapbook;
  documents->load_palette (fname_def_palette);
  documents->todo.load_dayfile();

  update_hls();
  update_view_hls();

#ifdef SPELLCHECK_ENABLE
  setup_spellcheckers();
#endif  
  
  shortcuts = new CShortcuts (this);
  shortcuts->fname.append (dir_config).append ("/shortcuts");
  shortcuts->load_from_file (shortcuts->fname);

  sl_fif_history = qstring_load (fname_fif).split ("\n");
  cmb_fif->addItems (sl_fif_history);
  cmb_fif->clearEditText(); 

  createFman();
  createOptions();
  createCalendar();
  createManual();
  
  updateFonts();
  update_logmemo_palette();

  dir_last = settings->value ("dir_last", QDir::homePath()).toString();
  b_preview = settings->value ("b_preview", false).toBool(); 
  
  l_status = new QLabel;
  pb_status = new QProgressBar;
  pb_status->setRange (0, 0);

  statusBar()->insertPermanentWidget (0, pb_status);
  statusBar()->insertPermanentWidget (1, l_status);

  pb_status->hide();
  documents->l_status_bar = l_status;

  img_viewer = new CImgViewer;

  restoreState (settings->value ("state", QByteArray()).toByteArray());

  QString vn = settings->value ("VERSION_NUMBER", "").toString();
  if (vn.isEmpty() || vn != QString (VERSION_NUMBER))
     {
      update_hls (true);
      help_show_news();
     }

  if (settings->value ("session_restore", false).toBool())
     {
      QString fname_session (dir_sessions);
      fname_session.append ("/def-session-777");
      documents->load_from_session (fname_session);
     }
  
  handle_args();
  ui_update = false;
  
  setIconSize (QSize (icon_size, icon_size));
  tb_fman_dir->setIconSize (QSize (icon_size, icon_size));

  QClipboard *clipboard = QApplication::clipboard();
  connect (clipboard , SIGNAL(dataChanged()), this, SLOT(clipboard_dataChanged()));


  setAcceptDrops (true);
  qApp->setWindowIcon (QIcon (":/icons/tea_icon_v2.png"));
  log->log (tr ("<b>TEA %1 @ http://semiletov.org/tea</b><br>by Peter Semiletov (e-mail: tea@list.ru, site: semiletov.org)<br>read the Manual under the <i>Learn</i> tab!").arg (QString (VERSION_NUMBER)));
  
  idx_tab_edit_activate();
}


void rvln::closeEvent (QCloseEvent *event)
{
  if (main_tab_widget->currentIndex() == idx_tab_tune)
     leaving_tune();

  QString fname (dir_config);
  fname.append ("/last_used_charsets");

  qstring_save (fname, sl_last_used_charsets.join ("\n").trimmed());

  if (settings->value("session_restore", false).toBool())
     {
      QString fname_session (dir_sessions);
      fname_session.append ("/def-session-777");
      documents->save_to_session (fname_session);
     }

  write_search_options();
  writeSettings();

  qstring_save (fname_fif, sl_fif_history.join ("\n"));

  delete documents;
  delete log;
  delete img_viewer;

#ifdef SPELLCHECK_ENABLE

  delete spellchecker;

#endif

  delete shortcuts;

  foreach (CMarkupPair *p, hs_markup)
          delete p;

  event->accept();
}


void rvln::newFile()
{
  documents->create_new();
  main_tab_widget->setCurrentIndex (idx_tab_edit);
}


void rvln::open()
{
  if (! settings->value ("use_trad_dialogs", "0").toBool())
     {
      CDocument *d = documents->get_current();
      if (d)
         {
          if (file_exists (d->file_name))
              fman->nav (get_file_path (d->file_name));
          else
              fman->nav (dir_last);
         }
      else
          fman->nav (dir_last);

      main_tab_widget->setCurrentIndex (idx_tab_fman);
      fm_entry_mode = FM_ENTRY_MODE_OPEN;

      return;
     }

  QFileDialog dialog (this);
  QSize size = settings->value ("dialog_size", QSize (width(), height())).toSize();
  dialog.resize (size);

  dialog.setFilter (QDir::AllEntries | QDir::Hidden);
  dialog.setOption (QFileDialog::DontUseNativeDialog, true);

  QList<QUrl> sidebarUrls = dialog.sidebarUrls();
  QList<QUrl> sidebarUrls_old = dialog.sidebarUrls();

  sidebarUrls.append (QUrl::fromLocalFile (dir_templates));
  sidebarUrls.append (QUrl::fromLocalFile (dir_snippets));
  sidebarUrls.append (QUrl::fromLocalFile (dir_sessions));
  sidebarUrls.append (QUrl::fromLocalFile (dir_scripts));
  sidebarUrls.append (QUrl::fromLocalFile (dir_tables));

#ifdef Q_OS_LINUX

  QDir volDir ("/mnt");
  QStringList volumes (volDir.entryList (volDir.filter() | QDir::NoDotAndDotDot));

  QDir volDir2 ("/media");
  QStringList volumes2 (volDir2.entryList (volDir.filter() | QDir::NoDotAndDotDot));

  foreach (QString v, volumes)  
          sidebarUrls.append (QUrl::fromLocalFile ("/mnt/" + v));

  foreach (QString v, volumes2)
          sidebarUrls.append (QUrl::fromLocalFile ("/media/" + v));


#endif

  dialog.setSidebarUrls (sidebarUrls);

  dialog.setFileMode (QFileDialog::ExistingFiles);
  dialog.setAcceptMode (QFileDialog::AcceptOpen);


  CDocument *d = documents->get_current();
  if (d)
     {
      if (file_exists (d->file_name))
          dialog.setDirectory (get_file_path (d->file_name));
      else
          dialog.setDirectory (dir_last);
     }
  else
      dialog.setDirectory (dir_last);

  dialog.setNameFilter (tr ("All (*);;Text files (*.txt);;Markup files (*.xml *.html *.htm *.);;C/C++ (*.c *.h *.cpp *.hh *.c++ *.h++ *.cxx)"));

  QLabel *l = new QLabel (tr ("Charset"));
  QComboBox *cb_codecs = new QComboBox (&dialog);
  dialog.layout()->addWidget (l);
  dialog.layout()->addWidget (cb_codecs);

  if (sl_last_used_charsets.size () > 0)
     cb_codecs->addItems (sl_last_used_charsets + sl_charsets);
  else
     {
      cb_codecs->addItems (sl_charsets);
      cb_codecs->setCurrentIndex (sl_charsets.indexOf ("UTF-8"));
     }

  QStringList fileNames;
  
  if (dialog.exec())
     {
      dialog.setSidebarUrls (sidebarUrls_old);

      fileNames = dialog.selectedFiles();
      
      foreach (QString fn, fileNames)
              {
               CDocument *d = documents->open_file (fn, cb_codecs->currentText());
               if (d)
                  {
                   dir_last = get_file_path (d->file_name);
                   charset = d->charset;
                  }

               add_to_last_used_charsets (cb_codecs->currentText());
              }
     }
  else
      dialog.setSidebarUrls (sidebarUrls_old);

  settings->setValue ("dialog_size", dialog.size());
  update_dyn_menus();
}


bool rvln::save()
{
  CDocument *d = documents->get_current();
  if (! d)
     return false;

  if (d->textEdit->isReadOnly())
     {
      log->log (tr ("This file is open in the read-only mode. You can save it with another name using <b>Save as</b>"));
      return false;
     }

  if (file_exists (d->file_name))
     d->save_with_name (d->file_name, d->charset);
  else
      return saveAs();

  if (d->file_name == fname_bookmarks)
     update_bookmarks();

  if (d->file_name == fname_programs)
     update_programs();

  return true;
}


bool rvln::saveAs()
{
  CDocument *d = documents->get_current();
  if (! d)
     return false;

  if (! settings->value ("use_trad_dialogs", "0").toBool())
     {
      main_tab_widget->setCurrentIndex (idx_tab_fman);
      fm_entry_mode = FM_ENTRY_MODE_SAVE;

      if (file_exists (d->file_name))
         fman->nav (get_file_path (d->file_name));
          else
              fman->nav (dir_last);

      ed_fman_fname->setFocus();   
          
      return true;
     }

  QFileDialog dialog (this);
  QSize size = settings->value ("dialog_size", QSize (width(), height())).toSize();
  dialog.resize (size);

  dialog.setFilter(QDir::AllEntries | QDir::Hidden);
  dialog.setOption (QFileDialog::DontUseNativeDialog, true);

  QList<QUrl> sidebarUrls = dialog.sidebarUrls();
  QList<QUrl> sidebarUrls_old = dialog.sidebarUrls();

  sidebarUrls.append(QUrl::fromLocalFile(dir_templates));
  sidebarUrls.append(QUrl::fromLocalFile(dir_snippets));
  sidebarUrls.append(QUrl::fromLocalFile(dir_sessions));
  sidebarUrls.append(QUrl::fromLocalFile(dir_scripts));
  sidebarUrls.append(QUrl::fromLocalFile(dir_tables));

#ifdef Q_OS_LINUX

  QDir volDir ("/mnt");
  QStringList volumes (volDir.entryList (volDir.filter() | QDir::NoDotAndDotDot));
  
  foreach (QString v, volumes)  
          sidebarUrls.append (QUrl::fromLocalFile ("/mnt/" + v));


  QDir volDir2 ("/media");
  QStringList volumes2 (volDir2.entryList (volDir2.filter() | QDir::NoDotAndDotDot));

  foreach (QString v, volumes2)
          sidebarUrls.append (QUrl::fromLocalFile ("/media/" + v));


#endif

  dialog.setSidebarUrls (sidebarUrls);

  dialog.setFileMode (QFileDialog::AnyFile);
  dialog.setAcceptMode (QFileDialog::AcceptSave);
  dialog.setConfirmOverwrite (false);
  dialog.setDirectory (dir_last);

  QLabel *l = new QLabel (tr ("Charset"));
  QComboBox *cb_codecs = new QComboBox (&dialog);
  dialog.layout()->addWidget (l);
  dialog.layout()->addWidget (cb_codecs);

  if (sl_last_used_charsets.size () > 0)
     cb_codecs->addItems (sl_last_used_charsets + sl_charsets);
  else
     {
      cb_codecs->addItems (sl_charsets);
      cb_codecs->setCurrentIndex (sl_charsets.indexOf ("UTF-8"));
     }

  if (dialog.exec())
     {
      dialog.setSidebarUrls (sidebarUrls_old);

      QString fileName = dialog.selectedFiles().at(0);

      if (file_exists (fileName))
         {
          int ret = QMessageBox::warning (this, "TEA",
                                          tr ("%1 already exists\n"
                                          "Do you want to overwrite?")
                                           .arg (fileName),
                                          QMessageBox::Yes | QMessageBox::Default,
                                          QMessageBox::Cancel | QMessageBox::Escape);

          if (ret == QMessageBox::Cancel)
             return false;
         }

      d->save_with_name (fileName, cb_codecs->currentText());
      d->set_markup_mode();
      d->set_hl();

      add_to_last_used_charsets (cb_codecs->currentText());
      update_dyn_menus();

      QFileInfo f (d->file_name);
      dir_last = f.path();
     }
   else
       dialog.setSidebarUrls (sidebarUrls_old);

  settings->setValue ("dialog_size", dialog.size());
  return true;
}


void rvln::about()
{
  CAboutWindow *a = new CAboutWindow();
  a->move (x() + 20, y() + 20);
  a->show();
}


void rvln::createActions()
{
  icon_size = settings->value ("icon_size", "32").toInt();
  act_test = new QAction (QIcon (":/icons/file-save.png"), tr ("Test"), this);

  connect (act_test, SIGNAL(triggered()), this, SLOT(test()));

  act_labels = new QAction (QIcon (":/icons/labels.png"), tr ("Labels"), this);
  connect (act_labels, SIGNAL(triggered()), this, SLOT(update_labels_list()));

  newAct = new QAction (QIcon (":/icons/file-new.png"), tr ("New"), this);

  newAct->setShortcut (QKeySequence ("Ctrl+N"));
  newAct->setStatusTip (tr ("Create a new file"));
  connect (newAct, SIGNAL(triggered()), this, SLOT(newFile()));

  openAct = new QAction (QIcon (":/icons/file-open.png"), tr ("Open file"), this);
  openAct->setStatusTip (tr ("Open an existing file"));
  connect (openAct, SIGNAL(triggered()), this, SLOT(open()));

  saveAct = new QAction (QIcon (":/icons/file-save.png"), tr ("Save"), this);
  saveAct->setShortcut (QKeySequence ("Ctrl+S"));
  saveAct->setStatusTip (tr ("Save the document to disk"));
  connect (saveAct, SIGNAL(triggered()), this, SLOT(save()));

  saveAsAct = new QAction (QIcon (":/icons/file-save-as.png"), tr ("Save As"), this);
  saveAsAct->setStatusTip (tr ("Save the document under a new name"));
  connect (saveAsAct, SIGNAL(triggered()), this, SLOT(saveAs()));

  exitAct = new QAction (tr ("Exit"), this);
  exitAct->setShortcut (QKeySequence ("Ctrl+Q"));
  exitAct->setStatusTip (tr ("Exit the application"));
  connect (exitAct, SIGNAL(triggered()), this, SLOT(close()));

  cutAct = new QAction (QIcon(":/icons/edit-cut.png"), tr ("Cut"), this);
  cutAct->setShortcut (QKeySequence ("Ctrl+X"));
  cutAct->setStatusTip (tr ("Cut the current selection's contents to the clipboard"));
  connect (cutAct, SIGNAL(triggered()), this, SLOT(ed_cut()));

  copyAct = new QAction (QIcon (":/icons/edit-copy.png"), tr("Copy"), this);
  copyAct->setShortcut (QKeySequence ("Ctrl+C"));
  copyAct->setStatusTip (tr ("Copy the current selection's contents to the clipboard"));
  connect (copyAct, SIGNAL(triggered()), this, SLOT(ed_copy()));

  pasteAct = new QAction (QIcon (":/icons/edit-paste.png"), tr("Paste"), this);
  pasteAct->setShortcut (QKeySequence ("Ctrl+V"));
  pasteAct->setStatusTip (tr ("Paste the clipboard's contents into the current selection"));
  connect (pasteAct, SIGNAL(triggered()), this, SLOT(ed_paste()));

  undoAct = new QAction (tr ("Undo"), this);
  undoAct->setShortcut (QKeySequence ("Ctrl+Z"));
  connect (undoAct, SIGNAL(triggered()), this, SLOT(ed_undo()));

  redoAct = new QAction (tr ("Redo"), this);
  connect (redoAct, SIGNAL(triggered()), this, SLOT(ed_redo()));

  aboutAct = new QAction (tr ("About"), this);
  connect (aboutAct, SIGNAL(triggered()), this, SLOT(about()));

  aboutQtAct = new QAction (tr ("About Qt"), this);
  connect (aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
}


void rvln::createMenus()
{
  fileMenu = menuBar()->addMenu (tr ("File"));
  fileMenu->setTearOffEnabled (true);

//  fileMenu->addAction (act_test);

  fileMenu->addAction (newAct);
  add_to_menu (fileMenu, tr ("Open"), SLOT(open()), "Ctrl+O", ":/icons/file-open.png");
  add_to_menu (fileMenu, tr ("Last closed file"), SLOT(file_last_opened()));
  add_to_menu (fileMenu, tr ("Open at cursor"), SLOT(open_at_cursor()), "F2");
  add_to_menu (fileMenu, tr ("Crapbook"), SLOT(file_crapbook()), "Alt+M");

  fileMenu->addSeparator();

  fileMenu->addAction (saveAct);
  fileMenu->addAction (saveAsAct);

  QMenu *tm = fileMenu->addMenu (tr ("Save as different"));
  tm->setTearOffEnabled (true);

  add_to_menu (tm, tr ("Save .bak"), SLOT(file_save_bak()), "Ctrl+B");
  add_to_menu (tm, tr ("Save timestamped version"), SLOT(file_save_version()), "Alt+S");
  add_to_menu (tm, tr ("Save session"), SLOT(session_save_as()));

  fileMenu->addSeparator();

  menu_file_actions = fileMenu->addMenu (tr ("File actions"));
  add_to_menu (menu_file_actions, tr ("Reload"), SLOT(file_reload()));
  add_to_menu (menu_file_actions, tr ("Reload with encoding"), SLOT(file_reload_enc()));
  menu_file_actions->addSeparator();
  add_to_menu (menu_file_actions, tr ("Set UNIX end of line"), SLOT(set_eol_unix()));
  add_to_menu (menu_file_actions, tr ("Set Windows end of line"), SLOT(set_eol_win()));
  add_to_menu (menu_file_actions, tr ("Set traditional Mac end of line"), SLOT(set_eol_mac()));
  
      
  menu_file_recent = fileMenu->addMenu (tr ("Recent files"));

  menu_file_bookmarks = fileMenu->addMenu (tr ("Bookmarks"));

  menu_file_edit_bookmarks = fileMenu->addMenu (tr ("Edit bookmarks"));
  add_to_menu (menu_file_edit_bookmarks, tr ("Add to bookmarks"), SLOT(file_add_to_bookmarks()));
  add_to_menu (menu_file_edit_bookmarks, tr ("Find obsolete paths"), SLOT(file_find_obsolete_paths()));

  menu_file_templates = fileMenu->addMenu (tr ("Templates"));
  menu_file_sessions = fileMenu->addMenu (tr ("Sessions"));

  menu_file_configs = fileMenu->addMenu (tr ("Configs"));
  add_to_menu (menu_file_configs, tr ("Bookmarks list"), SLOT(file_open_bookmarks_file()));
  add_to_menu (menu_file_configs, tr ("Programs list"), SLOT(file_open_programs_file()));

  fileMenu->addSeparator();

  add_to_menu (fileMenu, tr ("Print"), SLOT(file_print()));
  add_to_menu (fileMenu, tr ("Close current"), SLOT(close_current()), "Ctrl+W");

  fileMenu->addAction (exitAct);


  editMenu = menuBar()->addMenu (tr ("Edit"));
  editMenu->setTearOffEnabled (true);

  editMenu->addAction (cutAct);
  editMenu->addAction (copyAct);
  editMenu->addAction (pasteAct);

  editMenu->addSeparator();

  add_to_menu (editMenu, tr ("Copy current file name"), SLOT(edit_copy_current_fname()));

  editMenu->addSeparator();

  editMenu->addAction (undoAct);
  editMenu->addAction (redoAct);

  editMenu->addSeparator();
  
  add_to_menu (editMenu, tr ("Indent (tab)"), SLOT(ed_indent()));
  add_to_menu (editMenu, tr ("Un-indent (shift+tab)"), SLOT(ed_unindent()));
  add_to_menu (editMenu, tr ("Indent by first line"), SLOT(indent_by_first_line()));

  editMenu->addSeparator();

  add_to_menu (editMenu, tr ("Comment selection"), SLOT(ed_comment()));

  editMenu->addSeparator();

  add_to_menu (editMenu, tr ("Set as storage file"), SLOT(set_as_storage_file()));
  add_to_menu (editMenu, tr ("Copy to storage file"), SLOT(copy_to_storage_file()));
  add_to_menu (editMenu, tr ("Start/stop capture clipboard to storage file"), SLOT(capture_clipboard_to_storage_file()));



  menu_markup = menuBar()->addMenu (tr ("Markup"));
  menu_markup->setTearOffEnabled (true);

  tm = menu_markup->addMenu (tr ("Mode"));
  tm->setTearOffEnabled (true);

  create_menu_from_list (this, tm,
                         QString ("HTML XHTML Docbook LaTeX Lout DokuWiki MediaWiki").split (" "),
                         SLOT (mrkup_mode_choosed()));

  tm = menu_markup->addMenu (tr ("Header"));
  tm->setTearOffEnabled (true);

  create_menu_from_list (this, tm,
                         QString ("H1 H2 H3 H4 H5 H6").split (" "),
                         SLOT (mrkup_header()));

  tm = menu_markup->addMenu (tr ("Align"));
  tm->setTearOffEnabled (true);

  create_menu_from_list (this, tm,
                         QString ("Center Left Right Justify").split (" "),
                         SLOT (mrkup_align()));

  add_to_menu (menu_markup, tr ("Bold"), SLOT(mrkup_bold()), "Alt+B");
  add_to_menu (menu_markup, tr ("Italic"), SLOT(mrkup_italic()), "Alt+I");
  add_to_menu (menu_markup, tr ("Underline"), SLOT(mrkup_underline()));

  add_to_menu (menu_markup, tr ("Link"), SLOT(mrkup_link()), "Alt+L");
  add_to_menu (menu_markup, tr ("Paragraph"), SLOT(mrkup_para()), "Alt+P");
  add_to_menu (menu_markup, tr ("Color"), SLOT(mrkup_color()), "Alt+C");

  add_to_menu (menu_markup, tr ("Break line"), SLOT(mrkup_br()), "Ctrl+Return");
  add_to_menu (menu_markup, tr ("Non-breaking space"), SLOT(mrkup_nbsp()), "Ctrl+Space");
  add_to_menu (menu_markup, tr ("Insert image"), SLOT(markup_ins_image()));

  tm = menu_markup->addMenu (tr ("[X]HTML tools"));
  tm->setTearOffEnabled (true);

  add_to_menu (tm, tr ("Text to [X]HTML"), SLOT(mrkup_text_to_html()));
  add_to_menu (tm, tr ("Convert tags to entities"), SLOT(mrkup_tags_to_entities()));
  add_to_menu (tm, tr ("Antispam e-mail"), SLOT(fn_antispam_email()));
  add_to_menu (tm, tr ("Document weight"), SLOT(mrkup_document_weight()));
  add_to_menu (tm, tr ("Preview selected color"), SLOT(mrkup_preview_color()));
  add_to_menu (tm, tr ("Strip HTML tags"), SLOT(fn_strip_html_tags()));
  add_to_menu (tm, tr ("Rename selected file"), SLOT(rename_selected()));

  
  menu_search = menuBar()->addMenu (tr ("Search"));
  menu_search->setTearOffEnabled (true);

  add_to_menu (menu_search, tr ("Find"), SLOT(search_find()));
  add_to_menu (menu_search, tr ("Find next"), SLOT(search_find_next()),"F3");
  add_to_menu (menu_search, tr ("Find previous"), SLOT(search_find_prev()),"Ctrl+F3");

  menu_search->addSeparator();

  add_to_menu (menu_search, tr ("Find in files"), SLOT(search_in_files()));

  menu_search->addSeparator();

  add_to_menu (menu_search, tr ("Replace with"), SLOT(search_replace_with()));
  add_to_menu (menu_search, tr ("Replace all"), SLOT(search_replace_all()));
  add_to_menu (menu_search, tr ("Replace all in opened files"), SLOT(search_replace_all_at_ofiles()));

  menu_search->addSeparator();
  
  
  menu_find_case = new QAction (tr ("Case sensitive"), this);
  menu_search->addAction (menu_find_case);
  menu_find_case->setCheckable (true);

  menu_find_whole_words = new QAction (tr ("Whole words"), this);
  menu_search->addAction (menu_find_whole_words);
  menu_find_whole_words->setCheckable (true);
  connect (menu_find_whole_words, SIGNAL(triggered()), this, SLOT(search_whole_words_mode()));

  menu_find_regexp = new QAction (tr ("Regexp mode"), this);
  menu_search->addAction (menu_find_regexp);
  menu_find_regexp->setCheckable (true);
  connect (menu_find_regexp, SIGNAL(triggered()), this, SLOT(search_regexp_mode()));


  menu_find_fuzzy = new QAction (tr ("Fuzzy mode"), this);
  menu_search->addAction (menu_find_fuzzy);
  menu_find_fuzzy->setCheckable (true);
  connect (menu_find_fuzzy, SIGNAL(triggered()), this, SLOT(search_fuzzy_mode()));

  menu_functions = menuBar()->addMenu (tr ("Functions"));
  menu_functions->setTearOffEnabled (true);

  menu_fn_snippets = menu_functions->addMenu (tr ("Snippets"));
  menu_fn_scripts = menu_functions->addMenu (tr ("Scripts"));
  menu_fn_tables = menu_functions->addMenu (tr ("Tables"));

  tm = menu_functions->addMenu (tr ("Place"));
  tm->setTearOffEnabled (true);

  add_to_menu (tm, "Lorem ipsum", SLOT(fn_insert_loremipsum()));
  add_to_menu (tm, tr ("HTML template"), SLOT(fn_insert_template_html()));
  add_to_menu (tm, tr ("HTML5 template"), SLOT(fn_insert_template_html5()));

  add_to_menu (tm, tr ("C++ template"), SLOT(fn_insert_cpp()));
  add_to_menu (tm, tr ("C template"), SLOT(fn_insert_c()));


  add_to_menu (tm, tr ("Date"), SLOT(fn_insert_date()));
  add_to_menu (tm, tr ("Time"), SLOT(fn_insert_time()));


  tm = menu_functions->addMenu (tr ("Case"));
  tm->setTearOffEnabled (true);

  add_to_menu (tm, tr ("UPCASE"), SLOT(upCase()),"Ctrl+Up");
  add_to_menu (tm, tr ("lower case"), SLOT(dnCase()),"Ctrl+Down");


  tm = menu_functions->addMenu (tr ("Sort"));
  tm->setTearOffEnabled (true);

  add_to_menu (tm, tr ("Sort case sensitively"), SLOT(fn_sort_casecare()));
  add_to_menu (tm, tr ("Sort case insensitively"), SLOT(fn_sort_casecareless()));
  add_to_menu (tm, tr ("Flip a list"), SLOT(fn_flip_a_list()));


  tm = menu_functions->addMenu (tr ("Filter"));
  tm->setTearOffEnabled (true);

  add_to_menu (tm, tr ("Remove duplicates"), SLOT(fn_filter_rm_duplicates()));
  add_to_menu (tm, tr ("Remove empty lines"), SLOT(fn_filter_rm_empty()));
  add_to_menu (tm, tr ("Remove lines < N size"), SLOT(fn_filter_rm_less_than()));
  add_to_menu (tm, tr ("Remove lines > N size"), SLOT(fn_filter_rm_greater_than()));
  add_to_menu (tm, tr ("Remove before delimeter at each line"), SLOT(fn_filter_delete_before_sep()));
  add_to_menu (tm, tr ("Remove after delimeter at each line"), SLOT(fn_filter_delete_after_sep()));

  add_to_menu (tm, tr ("Filter with regexp"), SLOT(fn_filter_with_regexp()));



  tm = menu_functions->addMenu (tr ("Math"));
  tm->setTearOffEnabled (true);

  add_to_menu (tm, tr ("Evaluate"), SLOT(fn_evaluate()), "F4");
  add_to_menu (tm, tr ("Arabic to Roman"), SLOT(fn_number_arabic_to_roman()));
  add_to_menu (tm, tr ("Roman to Arabic"), SLOT(fn_number_roman_to_arabic()));
  add_to_menu (tm, tr ("Decimal to binary"), SLOT(fn_number_decimal_to_binary()));
  add_to_menu (tm, tr ("Binary to decimal"), SLOT(fn_binary_to_decimal()));
  add_to_menu (tm, tr ("Flip bits (bitwise complement)"), SLOT(fn_number_flip_bits()));
  add_to_menu (tm, tr ("Enumerate"), SLOT(fn_enum()));


  tm = menu_functions->addMenu (tr ("Morse code"));
  tm->setTearOffEnabled (true);

  add_to_menu (tm, tr ("From Russian to Morse"), SLOT(fn_morse_from_ru()));
  add_to_menu (tm, tr ("From Morse To Russian"), SLOT(fn_morse_to_ru()));

  add_to_menu (tm, tr ("From English to Morse"), SLOT(fn_morse_from_en()));
  add_to_menu (tm, tr ("From Morse To English"), SLOT(fn_morse_to_en()));


  tm = menu_functions->addMenu (tr ("Analyze"));
  tm->setTearOffEnabled (true);

  add_to_menu (tm, tr ("Text statistics"), SLOT(fn_text_stat()));
  add_to_menu (tm, tr ("Extract words"), SLOT(fn_extract_words()));
  add_to_menu (tm, tr ("UNITAZ quantity sorting"), SLOT(fn_get_words_count()));
  add_to_menu (tm, tr ("UNITAZ sorting alphabet"), SLOT(fn_unitaz_abc()));

  add_to_menu (tm, tr ("Count the substring"), SLOT(fn_count()));
  add_to_menu (tm, tr ("Count the substring (regexp)"), SLOT(fn_count_rx()));

  
  tm = menu_functions->addMenu (tr ("Text"));
  tm->setTearOffEnabled (true);

  add_to_menu (tm, tr ("Apply to each line"), SLOT(fn_apply_to_each_line()),"Alt+E");
  add_to_menu (tm, tr ("Remove formatting"), SLOT(fn_rm_formatting()));
  add_to_menu (tm, tr ("Remove formatting at each line"), SLOT(fn_rm_formatting_at_each_line()));
  add_to_menu (tm, tr ("Remove trailing spaces"), SLOT(fn_rm_trailing_spaces()));

  add_to_menu (tm, tr ("Escape regexp"), SLOT(fn_escape()));

  add_to_menu (tm, tr ("Reverse"), SLOT(fn_reverse()));
  add_to_menu (tm, tr ("Quotes to facing quotes"), SLOT(fn_convert_quotes()));


#ifdef SPELLCHECK_ENABLE

  menu_functions->addSeparator();

  menu_spell_langs = menu_functions->addMenu (tr ("Spell-checker languages"));
  menu_spell_langs->setTearOffEnabled (true);

  add_to_menu (menu_functions, tr ("Spell check"), SLOT(fn_spell_check()), "", ":/icons/fn-spell-check.png");
  add_to_menu (menu_functions, tr ("Suggest"), SLOT(fn_spell_suggest()));
  add_to_menu (menu_functions, tr ("Add to dictionary"), SLOT(fn_spell_add_to_dict()));
  add_to_menu (menu_functions, tr ("Remove from dictionary"), SLOT(fn_remove_from_dict()));

  menu_functions->addSeparator();

#endif


  menu_cal = menuBar()->addMenu (tr ("Calendar"));  
  menu_cal->setTearOffEnabled (true);

  add_to_menu (menu_cal, tr ("Moon mode on/off"), SLOT(cal_moon_mode()));
  add_to_menu (menu_cal, tr ("Mark first date"), SLOT(cal_set_date_a()));
  add_to_menu (menu_cal, tr ("Mark last date"), SLOT(cal_set_date_b()));


  menu_cal_add = menu_cal->addMenu (tr ("Add or subtract"));
  menu_cal_add->setTearOffEnabled (true);

  add_to_menu (menu_cal_add, tr ("Days"), SLOT(cal_add_days()));
  add_to_menu (menu_cal_add, tr ("Months"), SLOT(cal_add_months()));
  add_to_menu (menu_cal_add, tr ("Years"), SLOT(cal_add_years()));


  menu_cal->addSeparator();


  add_to_menu (menu_cal, tr ("Go to current date"), SLOT(cal_set_to_current()));
  add_to_menu (menu_cal, tr ("Calculate moon days between dates"), SLOT(cal_gen_mooncal()));
  add_to_menu (menu_cal, tr ("Number of days between two dates"), SLOT(cal_diff_days()));

  add_to_menu (menu_cal, tr ("Remove day record"), SLOT(cal_remove()));
  

  menu_programs = menuBar()->addMenu (tr ("Run"));

  menu_nav = menuBar()->addMenu (tr ("Nav"));
  menu_nav->setTearOffEnabled (true);

  add_to_menu (menu_nav, tr ("Save position"), SLOT(nav_save_pos()));
  add_to_menu (menu_nav, tr ("Go to saved position"), SLOT(nav_goto_pos()));
  add_to_menu (menu_nav, tr ("Go to line"), SLOT(nav_goto_line()),"Alt+G");
  add_to_menu (menu_nav, tr ("Next tab"), SLOT(nav_goto_right_tab()));
  add_to_menu (menu_nav, tr ("Prev tab"), SLOT(nav_goto_left_tab()));
  add_to_menu (menu_nav, tr ("Toggle header/source"), SLOT(nav_toggle_hs()));
  add_to_menu (menu_nav, tr ("Focus the Famous input field"), SLOT(nav_focus_to_fif()), "Ctrl+F");
  add_to_menu (menu_nav, tr ("Focus the editor"), SLOT(nav_focus_to_editor()));

  menu_labels = menu_nav->addMenu (tr ("Labels"));
  add_to_menu (menu_nav, tr ("Refresh labels"), SLOT(update_labels_list()));

  menu_fm = menuBar()->addMenu (tr ("Fm"));
  menu_fm->setTearOffEnabled (true);

  menu_fm_file_ops = menu_fm->addMenu (tr ("File operations"));
  menu_fm_file_ops->setTearOffEnabled (true);

  add_to_menu (menu_fm_file_ops, tr ("Create new directory"), SLOT(fman_create_dir()));
  add_to_menu (menu_fm_file_ops, tr ("Rename"), SLOT(fman_rename()));
  add_to_menu (menu_fm_file_ops, tr ("Delete file"), SLOT(fman_delete()));

  menu_fm_file_infos = menu_fm->addMenu (tr ("File information"));
  menu_fm_file_infos->setTearOffEnabled (true);


  add_to_menu (menu_fm_file_infos, tr ("Count lines in selected files"), SLOT(fman_count_lines_in_selected_files()));

  add_to_menu (menu_fm_file_infos, tr ("Full info"), SLOT(fm_full_info()));

  add_to_menu (menu_fm_file_infos, tr ("MD5 checksum"), SLOT(fm_hashsum_md5()));
  add_to_menu (menu_fm_file_infos, tr ("MD4 checksum"), SLOT(fm_hashsum_md4()));
  add_to_menu (menu_fm_file_infos, tr ("SHA1 checksum"), SLOT(fm_hashsum_sha1()));

  menu_fm_zip = menu_fm->addMenu (tr ("ZIP"));
  menu_fm_zip->setTearOffEnabled (true);

  add_to_menu (menu_fm_zip, tr ("Create new ZIP"), SLOT(fman_create_zip()));
  add_to_menu (menu_fm_zip, tr ("Add to ZIP"), SLOT(fman_add_to_zip()));
  add_to_menu (menu_fm_zip, tr ("Save ZIP"), SLOT(fman_save_zip()));


  add_to_menu (menu_fm_zip, tr ("List ZIP content"), SLOT(fman_zip_info()));
  add_to_menu (menu_fm_zip, tr ("Unpack ZIP to current directory"), SLOT(fman_unpack_zip()));

  
  menu_fm_img_conv = menu_fm->addMenu (tr ("Images"));
  menu_fm_img_conv->setTearOffEnabled (true);

  add_to_menu (menu_fm_img_conv, tr ("Scale by side"), SLOT(fman_img_conv_by_side()));
  add_to_menu (menu_fm_img_conv, tr ("Scale by percentages"), SLOT(fman_img_conv_by_percent()));
  add_to_menu (menu_fm_img_conv, tr ("Create web gallery"), SLOT(fman_mk_gallery()));

  
  add_to_menu (menu_fm, tr ("Go to home dir"), SLOT(fman_home()));
  add_to_menu (menu_fm, tr ("Refresh current dir"), SLOT(fman_refresh()));

  add_to_menu (menu_fm, tr ("Preview image"), SLOT(fman_preview_image()));

  add_to_menu (menu_fm, tr ("Select by regexp"), SLOT(fman_select_by_regexp()));
  add_to_menu (menu_fm, tr ("Deselect by regexp"), SLOT(fman_deselect_by_regexp()));
  
  
  menu_view = menuBar()->addMenu (tr ("View"));
  menu_view->setTearOffEnabled (true);

  menu_view_palettes = menu_view->addMenu (tr ("Palettes"));
  menu_view_palettes->setTearOffEnabled (true);

  menu_view_hl = menu_view->addMenu (tr ("Highlighting mode"));
  menu_view_hl->setTearOffEnabled (true);

  menu_view_profiles = menu_view->addMenu (tr ("Profiles"));
  menu_view_profiles->setTearOffEnabled (true);

  add_to_menu (menu_view, tr ("Save profile"), SLOT(profile_save_as()));
  
  add_to_menu (menu_view, tr ("Toggle word wrap"), SLOT(toggle_wrap()));
  add_to_menu (menu_view, tr ("Hide error marks"), SLOT(view_hide_error_marks()));
  add_to_menu (menu_view, tr ("Toggle fullscreen"), SLOT(view_toggle_fs()));
  add_to_menu (menu_view, tr ("Stay on top"), SLOT(view_stay_on_top()));
  add_to_menu (menu_view, tr ("Preview with default browser"), SLOT(view_preview_in_bro()));


  helpMenu = menuBar()->addMenu (tr ("Help"));
  helpMenu->setTearOffEnabled (true);

  helpMenu->addAction (aboutAct);
  helpMenu->addAction (aboutQtAct);
  add_to_menu (helpMenu, tr ("NEWS"), SLOT(help_show_news()));
  add_to_menu (helpMenu, "TODO", SLOT(help_show_todo()));
  add_to_menu (helpMenu, "ChangeLog", SLOT(help_show_changelog()));
  add_to_menu (helpMenu, tr ("License"), SLOT(help_show_gpl()));
}


void rvln::createToolBars()
{
  openAct->setMenu (menu_file_recent);
  act_labels->setMenu (menu_labels);

  fileToolBar = addToolBar (tr ("File"));
  fileToolBar->setObjectName ("fileToolBar");
  fileToolBar->addAction (newAct);
  fileToolBar->addAction (openAct);
  fileToolBar->addAction (saveAct);

  editToolBar = addToolBar (tr ("Edit"));
  editToolBar->setObjectName ("editToolBar");
  editToolBar->addAction (cutAct);
  editToolBar->addAction (copyAct);
  editToolBar->addAction (pasteAct);

  editToolBar->addSeparator();

  editToolBar->addAction (act_labels);
}


void rvln::createStatusBar()
{
  statusBar()->showMessage (tr ("Ready"));
}


rvln::~rvln()
{
  qDebug() << "~rvln()";
}


void rvln::pageChanged (int index)
{
  if (index == -1)
      return;  

  CDocument *d = documents->list[index];
  if (! d)
     return;

  d->update_title();
  d->update_status();

  update_labels_menu();
}


void rvln::close_current()
{
  documents->close_current();
}


void rvln::ed_copy()
{
  if (main_tab_widget->currentIndex() == idx_tab_edit)
     {
      CDocument *d = documents->get_current();
      if (d)
         d->textEdit->copy();
     }
  else
      if (main_tab_widget->currentIndex() == idx_tab_learn)      
         man->copy();
}


void rvln::ed_cut()
{
  CDocument *d = documents->get_current();
  if (d)
     d->textEdit->cut();
}


void rvln::ed_paste()
{
  CDocument *d = documents->get_current();
  if (d)
     d->textEdit->paste();
}


void rvln::ed_undo()
{
  CDocument *d = documents->get_current();
  if (d)
     d->textEdit->undo();
}


void rvln::ed_redo()
{
  CDocument *d = documents->get_current();
  if (d)
     d->textEdit->redo();
}


void rvln::ed_clear()
{
  CDocument *d = documents->get_current();
  if (d)
     d->textEdit->clear();
}


void rvln::upCase()
{
  CDocument *d = documents->get_current();
  if (d)
     d->textEdit->textCursor().insertText (d->textEdit->textCursor().selectedText().toUpper());
}


void rvln::dnCase()
{
  CDocument *d = documents->get_current();
  if (d)
      d->textEdit->textCursor().insertText (d->textEdit->textCursor().selectedText().toLower());
}


void rvln::remove_formatting()
{
  CDocument *d = documents->get_current();
  if (d)
     d->textEdit->textCursor().insertText (d->textEdit->textCursor().selectedText().simplified());
}

void rvln::mrkup_bold()
{
  CDocument *d = documents->get_current();
  if (d)
      d->textEdit->textCursor().insertText (hs_markup["bold"]->tags_a[d->markup_mode] +
                                            d->textEdit->textCursor().selectedText() +
                                            hs_markup["bold"]->tags_b[d->markup_mode]
                                           );
}


void rvln::mrkup_italic()
{
  CDocument *d = documents->get_current();
  if (d)
     d->textEdit->textCursor().insertText (hs_markup["italic"]->tags_a[d->markup_mode] +
                                           d->textEdit->textCursor().selectedText() +
                                           hs_markup["italic"]->tags_b[d->markup_mode]
                                          );
}


void rvln::mrkup_underline()
{
  CDocument *d = documents->get_current();
  if (d)
     d->textEdit->textCursor().insertText (hs_markup["underline"]->tags_a[d->markup_mode] +
                                           d->textEdit->textCursor().selectedText() +
                                           hs_markup["underline"]->tags_b[d->markup_mode]
                                          );
}


void rvln::mrkup_para()
{
  CDocument *d = documents->get_current();
  if (d)
     d->textEdit->textCursor().insertText (hs_markup["para"]->tags_a[d->markup_mode] +
                                           d->textEdit->textCursor().selectedText() +
                                           hs_markup["para"]->tags_b[d->markup_mode]
                                          );
}


void rvln::mrkup_link()
{
  CDocument *d = documents->get_current();
  if (d)
     d->textEdit->textCursor().insertText (hs_markup["link"]->tags_a[d->markup_mode] +
                                           d->textEdit->textCursor().selectedText() +
                                           hs_markup["link"]->tags_b[d->markup_mode]
                                          );
}


void rvln::mrkup_br()
{
  CDocument *d = documents->get_current();
  if (d)
     d->textEdit->textCursor().insertText (hs_markup["newline"]->tags_a[d->markup_mode] +
                                           d->textEdit->textCursor().selectedText() +
                                           hs_markup["newline"]->tags_b[d->markup_mode]
                                          );
}


void rvln::mrkup_nbsp()
{
  CDocument *d = documents->get_current();
  if (d)
     d->textEdit->textCursor().insertText ("&nbsp;");
}


QTextDocument::FindFlags rvln::get_search_options()
{
  QTextDocument::FindFlags flags = 0;

  if (menu_find_whole_words->isChecked())
     flags = flags | QTextDocument::FindWholeWords;

  if (menu_find_case->isChecked())
     flags = flags | QTextDocument::FindCaseSensitively;
  
  return flags;
}


void rvln::search_find()
{
  if (main_tab_widget->currentIndex() == idx_tab_edit)
     {
      CDocument *d = documents->get_current();
      if (! d)
        return;

      QTextCursor cr;

      int from = 0;

      QString fiftxt = fif_get_text();
      if (d->text_to_search == fiftxt)
         from = d->textEdit->textCursor().position();
      else
          from = 0;

      d->text_to_search = fiftxt;

      if (menu_find_regexp->isChecked())
         cr = d->textEdit->document()->find (QRegExp (d->text_to_search), from, get_search_options());
      else
          if (menu_find_fuzzy->isChecked())
             {
              int pos = str_fuzzy_search (d->textEdit->toPlainText(), d->text_to_search, from, settings->value ("fuzzy_q", "60").toInt());
              if (pos != -1)
                 {
                  from = pos + d->text_to_search.length() - 1;
                  //set selection:
                  cr = d->textEdit->textCursor();
                  cr.setPosition (pos, QTextCursor::MoveAnchor);
                  cr.movePosition (QTextCursor::Right, QTextCursor::KeepAnchor, d->text_to_search.length());

                  if (! cr.isNull())
                      d->textEdit->setTextCursor (cr);

                 }
              return;
             }
      else //normal search
          cr = d->textEdit->document()->find (d->text_to_search, from, get_search_options());

      if (! cr.isNull())  
          d->textEdit->setTextCursor (cr);
     }
  else
  if (main_tab_widget->currentIndex() == idx_tab_learn)
      man_find_find();
  else
  if (main_tab_widget->currentIndex() == idx_tab_tune)
     opt_shortcuts_find();
  else
  if (main_tab_widget->currentIndex() == idx_tab_fman)
     fman_find();
}


void rvln::fman_find()
{
  QString ft = fif_get_text();
  if (ft.isEmpty()) 
      return;  
     
  l_fman_find = fman->mymodel->findItems (ft, Qt::MatchStartsWith);
  
  if (l_fman_find.size() < 1)
     return; 
  
  fman_find_idx = 0;
  
  fman->setCurrentIndex (fman->mymodel->indexFromItem (l_fman_find[fman_find_idx]));
}


void rvln::fman_find_next()
{
  QString ft = fif_get_text();
  if (ft.isEmpty()) 
      return;  
   
  if (l_fman_find.size() < 1)
     return; 
  
  if (fman_find_idx < (l_fman_find.size() - 1))
     fman_find_idx++;
  
  fman->setCurrentIndex (fman->mymodel->indexFromItem (l_fman_find[fman_find_idx]));
}


void rvln::fman_find_prev()
{
  QString ft = fif_get_text();
  if (ft.isEmpty()) 
      return;  
   
  if (l_fman_find.size() < 1)
     return; 
  
  if (fman_find_idx != 0)
     fman_find_idx--;
  
  fman->setCurrentIndex (fman->mymodel->indexFromItem (l_fman_find[fman_find_idx]));
}


void rvln::search_find_next()
{
  if (main_tab_widget->currentIndex() == idx_tab_edit)
     {
      CDocument *d = documents->get_current();
      if (! d)
         return;

      QTextCursor cr;
      if (menu_find_regexp->isChecked())
         cr = d->textEdit->document()->find (QRegExp (d->text_to_search), d->textEdit->textCursor().position(), get_search_options());
      if (menu_find_fuzzy->isChecked())
         {
          int pos = str_fuzzy_search (d->textEdit->toPlainText(), d->text_to_search, d->textEdit->textCursor().position(), settings->value ("fuzzy_q", "60").toInt());
          if (pos != -1)
             {
              cr = d->textEdit->textCursor();
              cr.setPosition (pos, QTextCursor::MoveAnchor);
              cr.movePosition (QTextCursor::Right, QTextCursor::KeepAnchor, d->text_to_search.length());

              if (! cr.isNull())
                  d->textEdit->setTextCursor (cr);

             }
          return;

         }
      else
          cr = d->textEdit->document()->find (d->text_to_search, d->textEdit->textCursor().position(), get_search_options());
          
      if (! cr.isNull())  
          d->textEdit->setTextCursor (cr);
     }
   else
   if (main_tab_widget->currentIndex() == idx_tab_learn)
      man_find_next();
   else
   if (main_tab_widget->currentIndex() == idx_tab_tune)
      opt_shortcuts_find_next();
   else
   if (main_tab_widget->currentIndex() == idx_tab_fman)
      fman_find_next();
}


void rvln::search_find_prev()
{
  if (main_tab_widget->currentIndex() == idx_tab_edit)
     {
      CDocument *d = documents->get_current();
      if (! d)
         return;
      
      QTextCursor cr;
      
      if (menu_find_regexp->isChecked())
         cr = d->textEdit->document()->find (QRegExp (d->text_to_search),
                                             d->textEdit->textCursor(),
                                             get_search_options() | QTextDocument::FindBackward);
      else
          cr = d->textEdit->document()->find (d->text_to_search,
                                              d->textEdit->textCursor(),
                                              get_search_options() | QTextDocument::FindBackward);
      
      if (! cr.isNull())  
          d->textEdit->setTextCursor (cr);
     }
  else
  if (main_tab_widget->currentIndex() == idx_tab_learn)
      man_find_prev();
  else
  if (main_tab_widget->currentIndex() == idx_tab_tune)
      opt_shortcuts_find_prev();
  else
  if (main_tab_widget->currentIndex() == idx_tab_fman)
      fman_find_prev();
}


void rvln::opt_shortcuts_find()
{
   int from = 0;

   QString fiftxt = fif_get_text();

   if  (opt_shortcuts_string_to_find == fiftxt)
       from = lv_menuitems->currentRow();

   opt_shortcuts_string_to_find = fiftxt;
    
   if (from == -1)
      from = 0;

  int index = shortcuts->captions.indexOf (QRegExp (opt_shortcuts_string_to_find + ".*",
                                                    Qt::CaseInsensitive), from);
  if (index != -1) 
     lv_menuitems->setCurrentRow (index);
}


void rvln::opt_shortcuts_find_next()
{
  int from = lv_menuitems->currentRow();
  if (from == -1)
     from = 0;
  
  int index = shortcuts->captions.indexOf (QRegExp (opt_shortcuts_string_to_find + ".*",
                                                    Qt::CaseInsensitive), from + 1);
  if (index != -1) 
    lv_menuitems->setCurrentRow (index);
}


void rvln::opt_shortcuts_find_prev()
{
  int from = lv_menuitems->currentRow();
  if (from == -1)
     from = 0;
  
  int index = shortcuts->captions.lastIndexOf (QRegExp (opt_shortcuts_string_to_find + ".*",
                                                        Qt::CaseInsensitive), from - 1);
  if (index != -1) 
     lv_menuitems->setCurrentRow (index); 
}


void rvln::file_save_bak()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  QString fname (d->file_name);
  fname.append (".bak");
  d->save_with_name_plain (fname);
  log->log (tr ("%1 is saved").arg (fname));
}


void rvln::markup_ins_image()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  main_tab_widget->setCurrentIndex (idx_tab_fman);

  if (file_exists (d->file_name))
     fman->nav (get_file_path (d->file_name));
}


void rvln::slot_lv_menuitems_currentItemChanged (QListWidgetItem *current, QListWidgetItem *previous)
{
  if (! current)
     return;

  QAction *a = shortcuts->find_by_caption (current->text());
  if (a)
     ent_shtcut->setText (a->shortcut().toString());
}


void rvln::pb_assign_hotkey_clicked()
{
  if (! lv_menuitems->currentItem())
     return;

  if (ent_shtcut->text().isEmpty())
     return;

  shortcuts->set_new_shortcut (lv_menuitems->currentItem()->text(), ent_shtcut->text());
  shortcuts->save_to_file (shortcuts->fname);
}


void rvln::pb_remove_hotkey_clicked ()
{
  if (! lv_menuitems->currentItem())
     return;

  shortcuts->set_new_shortcut (lv_menuitems->currentItem()->text(), "");
  ent_shtcut->setText ("");
  shortcuts->save_to_file (shortcuts->fname);
}


void rvln::createOptions()
{
  tab_options = new QTabWidget;

  idx_tab_tune = main_tab_widget->addTab (tab_options, tr ("tune"));
  
  QWidget *page_interface = new QWidget (tab_options);

  QVBoxLayout *page_interface_layout = new QVBoxLayout;
  page_interface_layout->setAlignment (Qt::AlignTop);


  QString default_style = qApp->style()->objectName();
  if (default_style == "GTK+") //can be buggy
     default_style = "Cleanlooks";


  cmb_styles = new_combobox (page_interface_layout,
                             tr ("UI style"),
                             QStyleFactory::keys(),
                             settings->value ("ui_style", default_style).toString());


  connect (cmb_styles, SIGNAL(currentIndexChanged (const QString &)),
           this, SLOT(slot_style_currentIndexChanged (const QString &)));


  QHBoxLayout *lt_h = new QHBoxLayout;

  QLabel *l_font = new QLabel (tr ("Editor font"));

  cmb_font_name = new QFontComboBox (page_interface);
  cmb_font_name->setCurrentFont (QFont (settings->value ("editor_font_name", "Monospace").toString()));


  spb_font_size = new QSpinBox (page_interface);
  spb_font_size->setRange (6, 64);
  spb_font_size->setValue (settings->value ("editor_font_size", "16").toInt());


  connect (cmb_font_name, SIGNAL(currentIndexChanged (const QString &)),
           this, SLOT(slot_editor_fontname_changed(const QString &)));

  connect (spb_font_size, SIGNAL(valueChanged (int)), this, SLOT(slot_font_size_changed (int )));


  QLabel *l_app_font = new QLabel (tr ("Interface font"));

  cmb_app_font_name = new QFontComboBox (page_interface);
  cmb_app_font_name->setCurrentFont (QFont (settings->value ("app_font_name", qApp->font().family()).toString()));
  spb_app_font_size = new QSpinBox (page_interface);
  spb_app_font_size->setRange (6, 64);
  QFontInfo fi = QFontInfo (qApp->font());

  spb_app_font_size->setValue (settings->value ("app_font_size", fi.pointSize()).toInt());
  connect (spb_app_font_size, SIGNAL(valueChanged (int)), this, SLOT(slot_app_font_size_changed (int )));

  connect (cmb_app_font_name, SIGNAL(currentIndexChanged ( const QString & )),
           this, SLOT(slot_app_fontname_changed(const QString & )));

  QPushButton *bt_add_user_font = new QPushButton (tr ("Add user font"), this);
  connect (bt_add_user_font, SIGNAL(clicked()), this, SLOT(add_user_font()));


  lt_h->addWidget (l_font);
  lt_h->addWidget (cmb_font_name);
  lt_h->addWidget (spb_font_size);

  page_interface_layout->addLayout (lt_h);

  lt_h = new QHBoxLayout;

  lt_h->addWidget (l_app_font);
  lt_h->addWidget (cmb_app_font_name);
  lt_h->addWidget (spb_app_font_size);

  page_interface_layout->addLayout (lt_h);
  page_interface_layout->addWidget (bt_add_user_font);


  QStringList sl_icon_sizes;
  sl_icon_sizes << "16" << "24" << "32" << "48" << "64";

  cmb_icon_size = new_combobox (page_interface_layout,
                                tr ("Icons size"),
                                sl_icon_sizes,
                                settings->value ("icon_size", "32").toString());

  connect (cmb_icon_size, SIGNAL(currentIndexChanged (const QString &)),
           this, SLOT(cmb_icon_sizes_currentIndexChanged (const QString &)));

  cb_show_linenums = new QCheckBox (tr ("Show line numbers"), tab_options);
  cb_show_linenums->setCheckState (Qt::CheckState (settings->value ("show_linenums", "0").toInt()));
  page_interface_layout->addWidget (cb_show_linenums);


  cb_wordwrap = new QCheckBox (tr ("Word wrap"), tab_options);
  cb_wordwrap->setCheckState (Qt::CheckState (settings->value ("word_wrap", "2").toInt()));
  page_interface_layout->addWidget (cb_wordwrap);


  cb_use_hl_wrap = new QCheckBox (tr ("Use wrap setting from highlighting module"), tab_options);
  cb_use_hl_wrap->setCheckState (Qt::CheckState (settings->value ("use_hl_wrap", "2").toInt()));
  page_interface_layout->addWidget (cb_use_hl_wrap);


  cb_hl_current_line = new QCheckBox (tr ("Highlight current line"), tab_options);
  cb_hl_current_line->setCheckState (Qt::CheckState (settings->value ("additional_hl", "0").toInt()));
  page_interface_layout->addWidget (cb_hl_current_line);


  cb_hl_brackets = new QCheckBox (tr ("Highlight paired brackets"), tab_options);
  cb_hl_brackets->setCheckState (Qt::CheckState (settings->value ("hl_brackets", "0").toInt()));
  page_interface_layout->addWidget (cb_hl_brackets);


  cb_auto_indent = new QCheckBox (tr ("Automatic indent"), tab_options);
  cb_auto_indent->setCheckState (Qt::CheckState (settings->value ("auto_indent", "0").toInt()));
  page_interface_layout->addWidget (cb_auto_indent);


  cb_spaces_instead_of_tabs = new QCheckBox (tr ("Use spaces instead of tabs"), tab_options);
  cb_spaces_instead_of_tabs->setCheckState (Qt::CheckState (settings->value ("spaces_instead_of_tabs", "2").toInt()));
  page_interface_layout->addWidget (cb_spaces_instead_of_tabs);


  cb_cursor_xy_visible = new QCheckBox (tr ("Show cursor position"), tab_options);
  cb_cursor_xy_visible->setCheckState (Qt::CheckState (settings->value ("cursor_xy_visible", "2").toInt()));
  page_interface_layout->addWidget (cb_cursor_xy_visible);



  spb_tab_sp_width = new_spin_box (page_interface_layout,
                                   tr ("Tab width in spaces"), 1, 64,
                                   settings->value ("tab_sp_width", 8).toInt());
  
  cb_center_on_cursor = new QCheckBox (tr ("Cursor center on scroll"), tab_options);
  cb_center_on_cursor->setCheckState (Qt::CheckState (settings->value ("center_on_scroll", "2").toInt()));
  page_interface_layout->addWidget (cb_center_on_cursor);

  cb_show_margin = new QCheckBox (tr ("Show margin at"), tab_options);
  cb_show_margin->setCheckState (Qt::CheckState (settings->value ("show_margin", "0").toInt()));

  spb_margin_pos = new QSpinBox;
  spb_margin_pos->setValue (settings->value ("margin_pos", 72).toInt());
   
  QHBoxLayout *lt_margin = new QHBoxLayout;
  lt_margin->addWidget (cb_show_margin);
  lt_margin->addWidget (spb_margin_pos);

  page_interface_layout->addLayout (lt_margin);
  
  page_interface->setLayout (page_interface_layout);
  page_interface->show();

  tab_options->addTab (page_interface, tr ("Interface"));

  //////////
  
  QWidget *page_common = new QWidget (tab_options);
  QVBoxLayout *page_common_layout = new QVBoxLayout;
  page_common_layout->setAlignment (Qt::AlignTop);

  cb_auto_img_preview = new QCheckBox (tr ("Automatic preview images at file manager"), tab_options);
  cb_auto_img_preview->setCheckState (Qt::CheckState (settings->value ("b_preview", "0").toInt()));

  cb_session_restore = new QCheckBox (tr ("Restore the last session on start-up"), tab_options);
  cb_session_restore->setCheckState (Qt::CheckState (settings->value ("session_restore", "0").toInt()));
  
  cb_override_locale = new QCheckBox (tr ("Override locale"), tab_options);
  cb_override_locale->setCheckState (Qt::CheckState (settings->value ("override_locale", 0).toInt()));


  ed_locale_override = new QLineEdit (this);
  ed_locale_override->setText (settings->value ("override_locale_val", "en").toString());

  QHBoxLayout *hb_locovr = new QHBoxLayout;
  
  hb_locovr->addWidget (cb_override_locale);
  hb_locovr->addWidget (ed_locale_override);


/*
  QStringList sl_ui_langs;
  sl_ui_langs << "en" << "ru" << "de" << "fr";

  cmb_ui_langs = new_combobox (page_common_layout,
                                tr ("UI language"),
                                sl_ui_langs,
                                settings->value ("ui_lang", QLocale::system().name()).toString());

  connect (cmb_ui_langs, SIGNAL(currentIndexChanged (const QString &)),
           this, SLOT(cmb_ui_langs_currentIndexChanged (const QString &)));

*/

  
  cb_use_trad_dialogs = new QCheckBox (tr ("Use traditional File Save/Open dialogs"), tab_options);
  cb_use_trad_dialogs->setCheckState (Qt::CheckState (settings->value ("use_trad_dialogs", "0").toInt()));

  cb_start_on_sunday = new QCheckBox (tr ("Start week on Sunday"), tab_options);
  cb_start_on_sunday->setCheckState (Qt::CheckState (settings->value ("start_week_on_sunday", "0").toInt()));


  cb_northern_hemisphere = new QCheckBox (tr ("Northern hemisphere"), this);
  cb_northern_hemisphere->setCheckState (Qt::CheckState (settings->value ("northern_hemisphere", "2").toInt()));

//
  QHBoxLayout *hb_moon_phase_algo = new QHBoxLayout;
  QLabel *l_moon_phase_algos = new QLabel (tr ("Moon phase algorithm"));

  cmb_moon_phase_algos = new QComboBox;
  cmb_moon_phase_algos->addItems (moon_phase_algos.values());

  cmb_moon_phase_algos->setCurrentIndex (settings->value ("moon_phase_algo", MOON_PHASE_TRIG2).toInt());


  hb_moon_phase_algo->addWidget (l_moon_phase_algos);
  hb_moon_phase_algo->addWidget (cmb_moon_phase_algos);


///
  QHBoxLayout *hb_ui_tabs_align = new QHBoxLayout;

  QLabel *l_tabs_align = new QLabel (tr ("UI tabs align"));

  QStringList sl_tabs_align;

  sl_tabs_align.append (tr ("Up"));
  sl_tabs_align.append (tr ("Bottom"));
  sl_tabs_align.append (tr ("Left"));
  sl_tabs_align.append (tr ("Right"));

  QComboBox *cmb_ui_tabs_align = new QComboBox;
  cmb_ui_tabs_align->addItems (sl_tabs_align);

  int ui_tab_align = settings->value ("ui_tabs_align", "3").toInt();
  main_tab_widget->setTabPosition (int_to_tabpos (ui_tab_align ));

  cmb_ui_tabs_align->setCurrentIndex (ui_tab_align);

  connect (cmb_ui_tabs_align, SIGNAL(currentIndexChanged (int)),
           this, SLOT(cmb_ui_tabs_currentIndexChanged (int)));


  hb_ui_tabs_align->addWidget (l_tabs_align);
  hb_ui_tabs_align->addWidget (cmb_ui_tabs_align);


  ///
  QHBoxLayout *hb_docs_tabs_align = new QHBoxLayout;

  l_tabs_align = new QLabel (tr ("Documents tabs align"));


  QComboBox *cmb_docs_tabs_align = new QComboBox;
  cmb_docs_tabs_align->addItems (sl_tabs_align);

  connect (cmb_docs_tabs_align, SIGNAL(currentIndexChanged (int)),
           this, SLOT(cmb_docs_tabs_currentIndexChanged (int)));

  int docs_tab_align = settings->value ("docs_tabs_align", "0").toInt();
  tab_widget->setTabPosition (int_to_tabpos (docs_tab_align));
  cmb_docs_tabs_align->setCurrentIndex (docs_tab_align);


  hb_docs_tabs_align->addWidget (l_tabs_align);
  hb_docs_tabs_align->addWidget (cmb_docs_tabs_align);
  ///


  QHBoxLayout *hb_zip_charset_in = new QHBoxLayout;

  QLabel *zl = new QLabel (tr ("ZIP unpacking: file names charset"));
  cmb_zip_charset_in = new QComboBox;
  cmb_zip_charset_in->addItems (sl_charsets);

  cmb_zip_charset_in->setCurrentIndex (sl_charsets.indexOf (settings->value ("zip_charset_in", "UTF-8").toString()));

  hb_zip_charset_in->addWidget (zl);
  hb_zip_charset_in->addWidget (cmb_zip_charset_in);


  QHBoxLayout *hb_zip_charset_out = new QHBoxLayout;

  zl = new QLabel (tr ("ZIP packing: file names charset"));
  cmb_zip_charset_out = new QComboBox;
  cmb_zip_charset_out->addItems (sl_charsets);

  cmb_zip_charset_out->setCurrentIndex (sl_charsets.indexOf (settings->value ("zip_charset_in", "UTF-8").toString()));

  hb_zip_charset_out->addWidget (zl);
  hb_zip_charset_out->addWidget (cmb_zip_charset_out);


  page_common_layout->addLayout (hb_ui_tabs_align);
  page_common_layout->addLayout (hb_docs_tabs_align);


  page_common_layout->addWidget (cb_auto_img_preview);
  page_common_layout->addWidget (cb_session_restore);
  page_common_layout->addWidget (cb_use_trad_dialogs);
  page_common_layout->addWidget (cb_start_on_sunday);
  page_common_layout->addWidget (cb_northern_hemisphere);

  page_common_layout->addLayout (hb_moon_phase_algo);


  page_common_layout->addLayout (hb_locovr);
  page_common_layout->addLayout (hb_zip_charset_in);
  page_common_layout->addLayout (hb_zip_charset_out);


  page_common->setLayout (page_common_layout);
  page_common->show();

  tab_options->addTab (page_common, tr ("Common"));

  
  /////////////

  QWidget *page_functions = new QWidget (tab_options);
  QVBoxLayout *page_functions_layout = new QVBoxLayout;
  page_functions_layout->setAlignment (Qt::AlignTop);

  QGroupBox *gb_labels = new QGroupBox (tr ("Labels"));
  QVBoxLayout *vb_labels = new QVBoxLayout;
  gb_labels->setLayout (vb_labels);

  ed_label_start = new_line_edit (vb_labels, tr ("Label starts with: "), settings->value ("label_start", "[?").toString());
  ed_label_end = new_line_edit (vb_labels, tr ("Label ends with: "), settings->value ("label_end", "?]").toString());


  page_functions_layout->addWidget (gb_labels);




  QGroupBox *gb_datetime = new QGroupBox (tr ("Date and time"));
  QVBoxLayout *vb_datetime = new QVBoxLayout;
  gb_datetime->setLayout (vb_datetime);


  ed_date_format  = new_line_edit (vb_datetime, tr ("Date format"), settings->value ("date_format", "dd/MM/yyyy").toString());
  ed_time_format  = new_line_edit (vb_datetime, tr ("Time format"), settings->value ("time_format", "hh:mm:ss").toString());


  page_functions_layout->addWidget (gb_datetime);

  QLabel *l_t = 0;

#ifdef SPELLCHECK_ENABLE

  QGroupBox *gb_spell = new QGroupBox (tr ("Spell checking"));
  QVBoxLayout *vb_spell = new QVBoxLayout;
  gb_spell->setLayout(vb_spell);


  QHBoxLayout *hb_spellcheck_engine = new QHBoxLayout;



  cmb_spellcheckers = new_combobox (hb_spellcheck_engine,
                                    tr ("Spell checker engine"),
                                    spellcheckers,
                                    cur_spellchecker);

  vb_spell->addLayout (hb_spellcheck_engine);


#ifdef HUNSPELL_ENABLE

  QHBoxLayout *hb_spellcheck_path = new QHBoxLayout;
  l_t = new QLabel (tr ("Hunspell dictionaries directory"));

  ed_spellcheck_path = new QLineEdit (this);
  ed_spellcheck_path->setText (settings->value ("hunspell_dic_path", QDir::homePath ()).toString());
  ed_spellcheck_path->setReadOnly (true);

  QPushButton *pb_choose_path = new QPushButton (tr ("Select"), this);

  connect (pb_choose_path, SIGNAL(clicked()), this, SLOT(pb_choose_hunspell_path_clicked()));
  
  hb_spellcheck_path->addWidget (l_t);
  hb_spellcheck_path->addWidget (ed_spellcheck_path);
  hb_spellcheck_path->addWidget (pb_choose_path);
   
  vb_spell->addLayout (hb_spellcheck_path);

#endif


#ifdef ASPELL_ENABLE


#if defined(Q_OS_WIN) || defined(Q_OS_OS2)

  QHBoxLayout *hb_aspellcheck_path = new QHBoxLayout;
  l_t = new QLabel (tr ("Aspell directory"));

  ed_aspellcheck_path = new QLineEdit (this);
  ed_aspellcheck_path->setText (settings->value ("win32_aspell_path", "C:\\Program Files\\Aspell").toString());
  ed_aspellcheck_path->setReadOnly (true);

  QPushButton *pb_choose_path2 = new QPushButton (tr ("Select"), this);

  connect (pb_choose_path2, SIGNAL(clicked()), this, SLOT(pb_choose_aspell_path_clicked()));

  hb_aspellcheck_path->addWidget (l_t);
  hb_aspellcheck_path->addWidget (ed_aspellcheck_path);
  hb_aspellcheck_path->addWidget (pb_choose_path2);

  vb_spell->addLayout (hb_aspellcheck_path);

#endif

#endif

  connect (cmb_spellcheckers, SIGNAL(currentIndexChanged (const QString&)),
           this, SLOT(cmb_spellchecker_currentIndexChanged (const QString &)));


  page_functions_layout->addWidget (gb_spell);


#endif


  QGroupBox *gb_func_misc = new QGroupBox (tr ("Miscellaneous"));
  QVBoxLayout *vb_func_misc = new QVBoxLayout;
  gb_func_misc->setLayout (vb_func_misc);

  spb_fuzzy_q = new_spin_box (vb_func_misc, tr ("Fuzzy search factor"), 10, 100, settings->value ("fuzzy_q", "60").toInt());

  page_functions_layout->addWidget (gb_func_misc);


  page_functions->setLayout (page_functions_layout);
  page_functions->show();

  tab_options->addTab (page_functions, tr ("Functions"));

/////////////


  QWidget *page_images = new QWidget (tab_options);
  QVBoxLayout *page_images_layout = new QVBoxLayout;
  page_images_layout->setAlignment (Qt::AlignTop);

  QGroupBox *gb_images = new QGroupBox (tr("Miscellaneous"));
  QVBoxLayout *vb_images = new QVBoxLayout;
  gb_images->setLayout(vb_images);


  cmb_output_image_fmt = new_combobox (vb_images,
                                       tr ("Image conversion output format"),
                                       bytearray_to_stringlist (QImageWriter::supportedImageFormats()),
                                       settings->value ("output_image_fmt", "jpg").toString());

  cb_output_image_flt = new QCheckBox (tr ("Scale images with bilinear filtering"), this);
  cb_output_image_flt->setCheckState (Qt::CheckState (settings->value ("img_filter", 0).toInt()));

  vb_images->addWidget (cb_output_image_flt);

   spb_img_quality = new_spin_box (vb_images, tr ("Output images quality"), -1, 100, settings->value ("img_quality", "-1").toInt());


  cb_zip_after_scale = new QCheckBox (tr ("Zip directory with processed images"), this);
  cb_zip_after_scale->setCheckState (Qt::CheckState (settings->value ("img_post_proc", 0).toInt()));

  vb_images->addWidget (cb_zip_after_scale);

  page_images_layout->addWidget (gb_images);


  QGroupBox *gb_webgallery = new QGroupBox (tr ("Web gallery options"));
  QVBoxLayout *vb_webgal = new QVBoxLayout;

  ed_side_size = new_line_edit (vb_webgal, tr ("Size of the side"), settings->value ("ed_side_size", "110").toString());
  ed_link_options = new_line_edit (vb_webgal, tr ("Link options"), settings->value ("ed_link_options", "target=\"_blank\"").toString());
  ed_cols_per_row = new_line_edit (vb_webgal, tr ("Columns per row"), settings->value ("ed_cols_per_row", "4").toString());

  gb_webgallery->setLayout(vb_webgal);
  page_images_layout->addWidget (gb_webgallery);

  page_images->setLayout (page_images_layout);
  page_images->show();

  tab_options->addTab (page_images, tr ("Images"));
////////////////////////////

  QWidget *page_keyboard = new QWidget (tab_options);

  lt_h = new QHBoxLayout;

  QHBoxLayout *lt_shortcut = new QHBoxLayout;

  QVBoxLayout *lt_vkeys = new QVBoxLayout;
  QVBoxLayout *lt_vbuttons = new QVBoxLayout;

  lv_menuitems = new QListWidget;
  
  lt_vkeys->addWidget (lv_menuitems);

  connect (lv_menuitems, SIGNAL(currentItemChanged (QListWidgetItem *, QListWidgetItem *)),
           this, SLOT(slot_lv_menuitems_currentItemChanged (QListWidgetItem *, QListWidgetItem *)));

  ent_shtcut = new CShortcutEntry;
  QLabel *l_shortcut = new QLabel (tr ("Shortcut"));

  lt_shortcut->addWidget (l_shortcut);
  lt_shortcut->addWidget (ent_shtcut);

  lt_vbuttons->addLayout (lt_shortcut);

  QPushButton *pb_assign_hotkey = new QPushButton (tr ("Assign"), this);
  QPushButton *pb_remove_hotkey = new QPushButton (tr ("Remove"), this);

  connect (pb_assign_hotkey, SIGNAL(clicked()), this, SLOT(pb_assign_hotkey_clicked()));
  connect (pb_remove_hotkey, SIGNAL(clicked()), this, SLOT(pb_remove_hotkey_clicked()));

  lt_vbuttons->addWidget (pb_assign_hotkey);
  lt_vbuttons->addWidget (pb_remove_hotkey, 0, Qt::AlignTop);

  lt_h->addLayout (lt_vkeys);
  lt_h->addLayout (lt_vbuttons);

  page_keyboard->setLayout (lt_h);
  page_keyboard->show();

  idx_tab_keyboard = tab_options->addTab (page_keyboard, tr ("Keyboard"));


  connect (tab_options, SIGNAL(currentChanged(int)), this, SLOT(tab_options_pageChanged(int)));
}


void rvln::opt_update_keyb()
{
//  qDebug() << "opt_update_keyb()";

  if (! lv_menuitems)
     return;

  lv_menuitems->clear();

  shortcuts->captions_iterate();

  lv_menuitems->addItems (shortcuts->captions);
}


void rvln::slot_style_currentIndexChanged (const QString &text)
{
  if (text == "GTK+") //because it is buggy with some Qt versions. sorry!
     return;

  QApplication::setStyle (QStyleFactory::create (text));
  settings->setValue ("ui_style", text);
}


void rvln::open_at_cursor()
{
  if (main_tab_widget->currentIndex() == idx_tab_fman)
     {
      fman_preview_image();
      return;
     }
  
  CDocument *d = documents->get_current();
  if (! d)
     return;

  QString fname = d->get_filename_at_cursor();

  if (fname.isEmpty())
     return;

  if (is_image (fname))
     {
      if (! img_viewer->window_full.isVisible())
         {
          img_viewer->window_full.show();
          activateWindow();
         }
      
      img_viewer->set_image_full (fname);
      return;
     }

  if (fname.startsWith ("#"))
     {
      fname.remove (0, 1);
      fname.prepend ("name=\"");
      d->textEdit->find (fname);
      return;
     }

  documents->open_file (fname, d->charset);
}


void rvln::toggle_wrap()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  if (d->textEdit->lineWrapMode() == QPlainTextEdit::NoWrap)
     d->textEdit->setLineWrapMode (QPlainTextEdit::WidgetWidth);
  else
      d->textEdit->setLineWrapMode (QPlainTextEdit::NoWrap);
}


void rvln::view_preview_in_bro()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  QString cm ("file:///");
  cm.append (d->file_name);
  QDesktopServices::openUrl (cm);
}


void rvln::nav_save_pos()
{
  CDocument *d = documents->get_current();
  if (d)
     d->position = d->textEdit->textCursor().position();
}


void rvln::nav_goto_pos()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  QTextCursor cr = d->textEdit->textCursor();
  cr.setPosition (d->position);
  d->textEdit->setTextCursor (cr);
}


void rvln::slot_editor_fontname_changed (const QString &text)
{
  settings->setValue ("editor_font_name", text);
  updateFonts();
}


void rvln::slot_app_fontname_changed (const QString &text)
{
  settings->setValue ("app_font_name", text);
  updateFonts();
}


void rvln::slot_app_font_size_changed (int i)
{
  settings->setValue("app_font_size", i);
  updateFonts();
}


void rvln::slot_font_size_changed (int i)
{
  settings->setValue("editor_font_size", i);
  updateFonts();
}


void rvln::mrkup_color()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  QColor color = QColorDialog::getColor (Qt::green, this);
  if (! color.isValid())
     return;

  QString s;

  if (d->textEdit->textCursor().hasSelection())
      s = QString ("<span style=\"color:%1;\">%2</span>")
                   .arg (color.name())
                   .arg (d->textEdit->textCursor().selectedText());
  else
      s = color.name();

  d->textEdit->textCursor().insertText (s);
}


void rvln::nav_goto_line()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  QTextCursor cr = d->textEdit->textCursor();
  cr.movePosition (QTextCursor::Start);
  cr.movePosition (QTextCursor::Down, QTextCursor::MoveAnchor, fif_get_text().toInt());
  d->textEdit->setTextCursor (cr);
}


void rvln::updateFonts()
{
  documents->apply_settings();

  QFont f;
  f.fromString (settings->value ("editor_font_name", "Monospace").toString());
  f.setPointSize (settings->value ("editor_font_size", "16").toInt());
  man->setFont (f);

  QFont fapp;
  QFontInfo fi = QFontInfo (qApp->font());

  fapp.setPointSize (settings->value ("app_font_size", fi.pointSize()).toInt());
  fapp.fromString (settings->value ("app_font_name", qApp->font().family()).toString());
  qApp->setFont (fapp);
}


void rvln::man_find_find()
{
  QString fiftxt = fif_get_text();

  if (man_search_value == fiftxt)
      man->find (fiftxt);
   else
      man->find (fiftxt, 0);

  man_search_value = fiftxt;
}


void rvln::man_find_next()
{
  man->find (man_search_value, get_search_options());
}


void rvln::man_find_prev()
{
  man->find (man_search_value, get_search_options() | QTextDocument::FindBackward);
}


void rvln::createManual()
{
  QWidget *wd_man = new QWidget (this);

  QVBoxLayout *lv_t = new QVBoxLayout;

  QString loc = QLocale::system().name().left (2);

  if (settings->value ("override_locale", 0).toBool())
     {
      QString ts = settings->value ("override_locale_val", "en").toString();
      if (ts.length() != 2)
          ts = "en";
      loc = ts;
     }

  QString filename (":/manuals/");
  filename.append (loc).append (".html");
  
  if (! file_exists (filename))
      filename = ":/manuals/en.html";

  man_search_value = "";

  QHBoxLayout *lh_controls = new QHBoxLayout();

  QPushButton *bt_back = new QPushButton ("<");
  QPushButton *bt_forw = new QPushButton (">");

  lh_controls->addWidget (bt_back);
  lh_controls->addWidget (bt_forw);

  man = new QTextBrowser;
  man->setOpenExternalLinks (true);
  man->setSource (QUrl ("qrc" + filename));

  connect (bt_back, SIGNAL(clicked()), man, SLOT(backward()));
  connect (bt_forw, SIGNAL(clicked()), man, SLOT(forward()));

  lv_t->addLayout (lh_controls);
  lv_t->addWidget (man);

  wd_man->setLayout (lv_t);

  idx_tab_learn = main_tab_widget->addTab (wd_man, tr ("learn"));
}


void rvln::file_crapbook()
{
  if (! QFile::exists (fname_crapbook))
      qstring_save (fname_crapbook, tr ("you can put here notes, etc"));

  documents->open_file (fname_crapbook, "UTF-8");
}


void rvln::fn_apply_to_each_line()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  QStringList sl = d->textEdit->textCursor().selectedText().split (QChar::ParagraphSeparator);
  QString t = fif_get_text();

  if (t.startsWith ("@@"))
     {
      QString fname = dir_snippets + QDir::separator() + t;

      if (file_exists (fname))
         {
          log->log (tr("snippet %1 is not exists").arg (fname));
          return;
         }

      t = t.remove (0, 2);
      t = qstring_load (fname);
     }


  QMutableListIterator <QString> i (sl);

  while (i.hasNext())
        {
         QString ts (t);
         QString s = i.next();
         i.setValue (ts.replace ("%s", s));
        }

   QString x = sl.join ("\n");

   d->textEdit->textCursor().insertText (x);
}


void rvln::fn_filter_with_regexp()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;
  
  d->textEdit->textCursor().insertText (qstringlist_process (
                                        d->textEdit->textCursor().selectedText(),
                                        fif_get_text(), 
                                        QSTRL_PROC_FLT_WITH_REGEXP));
}


void rvln::fn_reverse()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  QString s = d->textEdit->textCursor().selectedText();
  if (s.isEmpty())
     return;
  
  d->textEdit->textCursor().insertText (string_reverse (s));
}


void rvln::file_print()
{

#ifdef PRINTER_ENABLE

  CDocument *d = documents->get_current();
  if (! d)
     return;

  QPrintDialog *dialog = new QPrintDialog (&printer, this);

  dialog->setWindowTitle (tr ("Print document"));

  if (d->textEdit->textCursor().hasSelection())
      dialog->addEnabledOption (QAbstractPrintDialog::PrintSelection);

  if (dialog->exec() != QDialog::Accepted)
      return;

  d->textEdit->print (&printer);

#endif
}


void rvln::file_last_opened()
{
  if (documents->recent_files.size() > 0)
     {
      documents->open_file_triplex (documents->recent_files[0]);
      documents->recent_files.removeAt (0);
      documents->update_recent_menu();
      main_tab_widget->setCurrentIndex (idx_tab_edit); 
     }
}


#ifdef SPELLCHECK_ENABLE

void rvln::fn_change_spell_lang()
{
  QAction *Act = qobject_cast<QAction *>(sender());
  settings->setValue ("spell_lang", Act->text());
  spellchecker->change_lang (Act->text());
  fn_spell_check();
}


void rvln::create_spellcheck_menu()
{
  menu_spell_langs->clear();
  create_menu_from_list (this, menu_spell_langs, spellchecker->get_speller_modules_list(), SLOT(fn_change_spell_lang()));
}


void rvln::fn_spell_check()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  QTime time_start;
  time_start.start();

  pb_status->show();
  pb_status->setRange (0, d->textEdit->toPlainText().size() - 1);
  pb_status->setFormat (tr ("%p% completed"));
  pb_status->setTextVisible (true);

  int i = 0;

  QTextCursor cr = d->textEdit->textCursor();
  int pos = cr.position();

  QString text = d->textEdit->toPlainText();

//delete all underlines
  cr.setPosition (0);
  cr.movePosition (QTextCursor::End, QTextCursor::KeepAnchor);
  QTextCharFormat f = cr.blockCharFormat();
  f.setFontUnderline (false);
  cr.mergeCharFormat (f);

  cr.setPosition (0);
  cr.movePosition (QTextCursor::Start, QTextCursor::MoveAnchor);

  do
    {
//     if (i % 100 == 0)
  //      qApp->processEvents();
        
     QChar c = text.at (cr.position());
     if (char_is_shit (c))
        while (char_is_shit (c))
              {
               cr.movePosition (QTextCursor::NextCharacter);
               c = text.at (cr.position());
              }

     cr.movePosition (QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
     c = text.at (cr.position());

     QString stext = cr.selectedText();

     if (! stext.isNull() && stext.endsWith ("\""))
        {
         cr.movePosition (QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
         stext = cr.selectedText();
        }  
           
     if (! stext.isNull() || ! stext.isEmpty())
     if (! spellchecker->check (cr.selectedText()))
        {
         f = cr.blockCharFormat();
         f.setUnderlineStyle (QTextCharFormat::SpellCheckUnderline);
         f.setUnderlineColor (QColor (hash_get_val (documents->palette, "error", "red")));
         cr.mergeCharFormat (f);
        }

      pb_status->setValue (i++);
     }
  while (cr.movePosition (QTextCursor::NextWord));
  
  cr.setPosition (pos);
  d->textEdit->setTextCursor (cr);
  d->textEdit->document()->setModified (false);

  pb_status->hide();
  
  log->log (tr("elapsed milliseconds: %1").arg (time_start.elapsed()));
}


void rvln::fn_spell_add_to_dict()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  QTextCursor cr = d->textEdit->textCursor();
  cr.select (QTextCursor::WordUnderCursor);
  QString s = cr.selectedText();

  if (! s.isEmpty() || ! s.isNull())
     spellchecker->add_to_user_dict (s);
}


void rvln::fn_remove_from_dict()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  QTextCursor cr = d->textEdit->textCursor();
  cr.select (QTextCursor::WordUnderCursor);
  QString s = cr.selectedText();

  if (! s.isEmpty())
     spellchecker->remove_from_user_dict (s);
}


void rvln::fn_spell_suggest_callback()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  QAction *Act = qobject_cast<QAction *>(sender());
  QString new_text = Act->text();

  QTextCursor cr = d->textEdit->textCursor();

  cr.select (QTextCursor::WordUnderCursor);
  QString s = cr.selectedText();
  if (s.isEmpty())
     return; 
  
  if (s[0].isUpper())
     new_text[0] = new_text[0].toUpper();

  cr.insertText (new_text);
  d->textEdit->setTextCursor (cr);
}


void rvln::fn_spell_suggest()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  QTextCursor cr = d->textEdit->textCursor();
  cr.select (QTextCursor::WordUnderCursor);
  QString s = cr.selectedText();
  if (s.isEmpty())
     return;

  QStringList l = spellchecker->get_suggestions_list (s);

  QMenu *m = new QMenu (this);
  create_menu_from_list (this, m, l, SLOT (fn_spell_suggest_callback()));
  m->popup (mapToGlobal(d->textEdit->cursorRect().topLeft()));
}

#endif


void rvln::file_open_bookmark()
{
  QAction *a = qobject_cast<QAction *>(sender());
  documents->open_file_triplex (a->text());
  main_tab_widget->setCurrentIndex (idx_tab_edit); 
}


void rvln::file_add_to_bookmarks()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  if (! file_exists (d->file_name))
     return;

  bool found = false;
  QStringList l_bookmarks = qstring_load (fname_bookmarks).split("\n");

  for (int i = 0; i < l_bookmarks.size(); i++)
      {
       if (l_bookmarks[i].contains (d->file_name))
          {
           l_bookmarks[i] = d->get_triplex();
           found = true;
           break;
          }
      }

  if (! found)
     l_bookmarks.prepend (d->get_triplex());

  bookmarks = l_bookmarks.join ("\n").trimmed();

  qstring_save (fname_bookmarks, bookmarks);
  update_bookmarks();
}


void rvln::file_use_template()
{
  QAction *a = qobject_cast<QAction *>(sender());

  QString txt = qstring_load (a->data().toString());

  CDocument *d = documents->create_new();
  if (d)
     d->textEdit->textCursor().insertText (txt);
}


void rvln::file_use_snippet()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  QAction *a = qobject_cast<QAction *>(sender());
  QString s = qstring_load (a->data().toString());

  if (s.contains ("%s"))
     s = s.replace ("%s", d->textEdit->textCursor().selectedText());

  d->textEdit->textCursor().insertText (s);
}


void rvln::update_templates()
{
  menu_file_templates->clear();

  create_menu_from_dir (this,
                        menu_file_templates,
                        dir_templates,
                        SLOT (file_use_template())
                        );
}


void rvln::update_snippets()
{
   menu_fn_snippets->clear();
   create_menu_from_dir (this,
                        menu_fn_snippets,
                        dir_snippets,
                        SLOT (file_use_snippet())
                        );
}


void rvln::file_save_version()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  if (! file_exists (d->file_name))
     return;

  QDate date = QDate::currentDate();
  QFileInfo fi;
  fi.setFile (d->file_name);

  QString version_timestamp_fmt = settings->value ("version_timestamp_fmt", "yyyy-MM-dd").toString();
  QTime t = QTime::currentTime();

  QString fname = fi.absoluteDir().absolutePath() +
                  "/" +
                  fi.baseName() +
                  "-" +
                  date.toString (version_timestamp_fmt) +
                  "-" +
                  t.toString ("hh-mm-ss") +
                  "." +
                  fi.suffix();


  if (d->save_with_name_plain (fname))
     log->log (tr ("%1 - saved").arg (fname));
  else
     log->log (tr ("Cannot save %1").arg (fname));
}


void rvln::dragEnterEvent (QDragEnterEvent *event)
{
  if (event->mimeData()->hasFormat ("text/uri-list"))
     event->acceptProposedAction();
}


void rvln::dropEvent (QDropEvent *event)
{
  QString fName;
  QFileInfo info;
  
  if (! event->mimeData()->hasUrls())
     return;
     
  foreach (QUrl u, event->mimeData()->urls())     
          {
           fName = u.toLocalFile();
           info.setFile (fName);
           if (info.isFile())
               documents->open_file (fName, cb_fman_codecs->currentText());
           }
              
 event->accept();
}


void rvln::fn_evaluate()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  QString s = d->textEdit->textCursor().selectedText();

  std::string utf8_text = s.toUtf8().constData();

  double f = calculate (utf8_text);

  QString fs = s.setNum (f);

  log->log (fs);
}


void rvln::fn_sort_casecare()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  d->textEdit->textCursor().insertText (qstringlist_process (
                                        d->textEdit->textCursor().selectedText(),
                                        fif_get_text(), 
                                        QSTRL_PROC_FLT_WITH_SORTCASECARE));
}


void rvln::mrkup_text_to_html()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  QStringList l;

  if (d->textEdit->textCursor().hasSelection())
     l = d->textEdit->textCursor().selectedText().split (QChar::ParagraphSeparator);
  else
      l = d->textEdit->toPlainText().split("\n");

  QString result;

  if (d->markup_mode == "HTML")
     result += "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\n";
  else
     result += "<!DOCTYPE html PUBLIC \"-//W3C//DTD  1.0 Transitional//EN\" \"http://www.w3.org/TR/1/DTD/1-transitional.dtd\">\n";

  result += "<html>\n"
            "<head>\n"
            "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">\n"
            "<style type=\"text/css\">\n"
            ".p1\n"
            "{\n"
            "margin: 0px 0px 0px 0px;\n"
            "padding: 0px 0px 0px 0px;\n"
            "text-indent: 1.5em;\n"
            "text-align: justify;\n"
            "}\n"
            "</style>\n"
            "<title></title>\n"
            "</head>\n"
            "<body>\n";
  
  foreach (QString s, l)
          {
           QString t = s.simplified();

           if (t.isNull() || t.isEmpty())
              {
               if (d->markup_mode == "HTML")
                  result += "<br>\n";
               else
                   result += "<br />\n";
               }
           else
               result.append ("<p class=\"p1\">").append (s).append ("</p>\n");
          }

  result += "</body>\n</html>";

  CDocument *doc = documents->create_new();

  if (doc)
     doc->textEdit->textCursor().insertText (result);
}


void rvln::fn_text_stat()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  QString s;

  bool b_sel = d->textEdit->textCursor().hasSelection();
  
  if (b_sel)
     s = d->textEdit->textCursor().selectedText();
  else
      s = d->textEdit->toPlainText();

  int c = s.length();
  int purechars = 0;
  int lines = 1;

  for (int i = 0; i < c; ++ i)
      {
       QChar ch = s.at(i);

       if (ch.isLetterOrNumber() || ch.isPunct())
          purechars++;
       
       if (! b_sel)
          {
           if (ch == '\n')
              lines++;
          }
       else
          if (ch == QChar::ParagraphSeparator)
             lines++;
      }


  QString result = tr ("chars: %1<br>chars without spaces: %2<br>lines: %3<br>author's sheets: %4")
                       .arg (QString::number (c))
                       .arg (QString::number (purechars))
                       .arg (QString::number (lines))
                       .arg (QString::number (c / 40000));

  documents->log->log (result);
}


void rvln::fn_antispam_email()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  QString s = d->textEdit->textCursor().selectedText();
  QString result;

  int c = s.size();
  for (int i = 0; i < c; i++)
      result.append ("&#").append (QString::number(s.at(i).unicode())).append(";");

  d->textEdit->textCursor().insertText (result);
}


void rvln::search_replace_with()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  d->textEdit->textCursor().insertText (fif_get_text());
}


void rvln::search_replace_all_at_ofiles()
{
  QStringList l = fif_get_text().split ("~");
  if (l.size() < 2)
     return;

  int c = documents->list.size();
  if (c < 0)
     return;

  Qt::CaseSensitivity cs = Qt::CaseInsensitive;
  if (menu_find_case->isChecked())
     cs = Qt::CaseSensitive; 

  foreach (CDocument *d, documents->list)
          {
           QString s; 
    
           if (menu_find_regexp->isChecked())  
              s = d->textEdit->toPlainText().replace (QRegExp (l[0]), l[1]);
           else
               s = d->textEdit->toPlainText().replace (l[0], l[1], cs);
  
           d->textEdit->selectAll();
           d->textEdit->textCursor().insertText (s);
          }
}


void rvln::search_replace_all()
{
  Qt::CaseSensitivity cs = Qt::CaseInsensitive;
  if (menu_find_case->isChecked())
     cs = Qt::CaseSensitive;

  QStringList l = fif_get_text().split ("~");
  if (l.size() < 2)
     return;

  if (main_tab_widget->currentIndex() == idx_tab_edit)
     {
      CDocument *d = documents->get_current();
      if (! d)
         return;
  
      QString s;

      if (menu_find_regexp->isChecked())
         s = d->textEdit->toPlainText().replace (QRegExp (l[0]), l[1]);
      else
          s = d->textEdit->toPlainText().replace (l[0], l[1], cs);
  
      d->textEdit->selectAll();
      d->textEdit->textCursor().insertText (s);
     }
  else
      if (main_tab_widget->currentIndex() == idx_tab_fman)
        {
          QStringList sl = fman->get_sel_fnames();

          if (sl.size() < 1)
             return;

          char *charset = cb_fman_codecs->currentText().toLatin1().data();

          foreach (QString fname, sl)
                 {
                  QString f = qstring_load (fname, charset);
                  QString r;

                  if (menu_find_regexp->isChecked())
                     r = f.replace (QRegExp (l[0]), l[1]);
                  else
                      r = f.replace (l[0], l[1], cs);

                  qstring_save (fname, r, charset);
                  log->log (tr ("%1 is processed and saved").arg (fname));
                 }
        }
}


void CApplication::saveState (QSessionManager &manager)
{
  manager.setRestartHint (QSessionManager::RestartIfRunning);
}


void rvln::update_charsets()
{
  QString fname (dir_config);
  fname.append ("/last_used_charsets");

  if (! file_exists (fname))
     qstring_save (fname, "UTF-8");

  sl_last_used_charsets = qstring_load (fname).split ("\n");

  foreach (QByteArray codec, QTextCodec::availableCodecs())
          sl_charsets.prepend (codec);
  
  sl_charsets.sort();
}


void rvln::add_to_last_used_charsets (const QString &s)
{
  int i = sl_last_used_charsets.indexOf (s);
  if (i == -1)
     sl_last_used_charsets.prepend (s);
  else
      sl_last_used_charsets.move (i, 0);

  if (sl_last_used_charsets.size() > 3)
     sl_last_used_charsets.removeLast();
}


void rvln::fn_flip_a_list()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;
  
  d->textEdit->textCursor().insertText (qstringlist_process (
                                        d->textEdit->textCursor().selectedText(),
                                        fif_get_text(), 
                                        QSTRL_PROC_LIST_FLIP));
}


QString str_to_entities (const QString &s)
{
  QString t = s;
  t = t.replace ("&", "&amp;");

  t = t.replace ("\"", "&quot;");
  t = t.replace ("'", "&apos;");

  t = t.replace ("<", "&lt;");
  t = t.replace (">", "&gt;");

  return t;
}


void rvln::mrkup_tags_to_entities()
{
  CDocument *d = documents->get_current();
  if (d)
     d->textEdit->textCursor().insertText (str_to_entities (d->textEdit->textCursor().selectedText()));

  //QT5 QString::toHtmlEscaped
}


void rvln::fn_insert_loremipsum()
{
  CDocument *d = documents->get_current();
  if (d)
     d->textEdit->textCursor().insertText (qstring_load (":/text-data/lorem-ipsum"));
}


void rvln::mrkup_mode_choosed()
{
  QAction *a = qobject_cast<QAction *>(sender());
  markup_mode = a->text();
  documents->markup_mode = markup_mode;

  CDocument *d = documents->get_current();
  if (d)
     d->markup_mode = markup_mode;
}


void rvln::mrkup_header()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  QAction *a = qobject_cast<QAction *>(sender());

  QString r = QString ("<%1>%2</%1>").arg (
                       a->text().toLower()).arg (
                       d->textEdit->textCursor().selectedText());

  d->textEdit->textCursor().insertText (r);
}


void rvln::mrkup_align()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  QAction *a = qobject_cast<QAction *>(sender());

  QString r;

  if (d->markup_mode == "LaTeX")
      r = QString ("\\begin{%1}\n%2\\end{%1}").arg (a->text().toLower()).arg (d->textEdit->textCursor().selectedText());
  if (d->markup_mode == "MediaWiki")
     {
//FIXME write code here
     }
  else
       {
        r.append ("<p style=\"text-align:").append (
                  a->text().toLower()).append (
                  ";\">").append (
                  d->textEdit->textCursor().selectedText()).append (
                  "</p>");
       }

  d->textEdit->textCursor().insertText (r);
}


void rvln::nav_goto_right_tab()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  if (tab_widget->currentIndex() > (tab_widget->count() - 1))
    return;

  tab_widget->setCurrentIndex (tab_widget->currentIndex() + 1);
}


void rvln::nav_goto_left_tab()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  if (tab_widget->currentIndex() == 0)
     return;

  tab_widget->setCurrentIndex (tab_widget->currentIndex() -1);
}


void rvln::fn_filter_rm_less_than()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;
  
  d->textEdit->textCursor().insertText (qstringlist_process (
                                        d->textEdit->textCursor().selectedText(),
                                        fif_get_text(), 
                                        QSTRL_PROC_FLT_LESS));
}


void rvln::fn_filter_rm_greater_than()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  d->textEdit->textCursor().insertText (qstringlist_process (
                                        d->textEdit->textCursor().selectedText(),
                                        fif_get_text(), 
                                        QSTRL_PROC_FLT_GREATER));
}


void rvln::fn_filter_rm_duplicates()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  d->textEdit->textCursor().insertText (qstringlist_process (
                                        d->textEdit->textCursor().selectedText(),
                                        fif_get_text(), 
                                        QSTRL_PROC_FLT_REMOVE_DUPS));
}


void rvln::fn_filter_rm_empty()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  d->textEdit->textCursor().insertText (qstringlist_process (
                                        d->textEdit->textCursor().selectedText(),
                                        fif_get_text(), 
                                        QSTRL_PROC_FLT_REMOVE_EMPTY));
}


void rvln::fn_extract_words()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  QStringList w = d->get_words();

  CDocument *nd = documents->create_new();
  if (nd)
     nd->textEdit->textCursor().insertText (w.join("\n"));
}


QString toggle_fname_header_source (const QString &fileName)
{
  QFileInfo f (fileName);

  QString ext = f.suffix();

  if (ext == "c" || ext == "cpp" || ext == "cxx" || ext == "cc" || ext == "c++")
     {
      if (file_exists (f.absolutePath() + "/" + f.baseName () + ".h"))
        return f.absolutePath() + "/" + f.baseName () + ".h";
      else
      if (file_exists (f.absolutePath() + "/" + f.baseName () + ".hxx"))
        return f.absolutePath() + "/" + f.baseName () + ".hxx";
      else
      if (file_exists (f.absolutePath() + "/" + f.baseName () + ".h++"))
        return f.absolutePath() + "/" + f.baseName () + ".h++";
      else
      if (file_exists (f.absolutePath() + "/" + f.baseName () + ".hh"))
        return f.absolutePath() + "/" + f.baseName () + ".hh";
      else
      if (file_exists (f.absolutePath() + "/" + f.baseName () + ".hpp"))
        return f.absolutePath() + "/" + f.baseName () + ".hpp";
     }
  else
  if (ext == "h" || ext == "h++" || ext == "hxx" || ext == "hh" || ext == "hpp")
     {
      if (file_exists (f.absolutePath() + "/" + f.baseName () + ".c"))
        return f.absolutePath() + "/" + f.baseName () + ".c";
      else
      if (file_exists (f.absolutePath() + "/" + f.baseName () + ".cpp"))
        return f.absolutePath() + "/" + f.baseName () + ".cpp";
      else
      if (file_exists (f.absolutePath() + "/" + f.baseName () + ".cxx"))
        return f.absolutePath() + "/" + f.baseName () + ".cxx";
      else
      if (file_exists (f.absolutePath() + "/" + f.baseName () + ".c++"))
        return f.absolutePath() + "/" + f.baseName () + ".c++";
      else
      if (file_exists (f.absolutePath() + "/" + f.baseName () + ".cc"))
        return f.absolutePath() + "/" + f.baseName () + ".cc";
     }

  return fileName;
}


void rvln::nav_toggle_hs()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  if (! file_exists (d->file_name))
      return;

  documents->open_file (toggle_fname_header_source (d->file_name), d->charset);
}


QString morse_from_lang (const QString &s, const QString &lang)
{
  QHash<QString, QString> h = hash_load_keyval (":/text-data/morse-" + lang);

  QString result;
  QString x = s.toUpper();

  int c = x.size();
  for (int i = 0; i < c; i++)
      {
       QString t = h.value (QString (x[i]));
       if (! t.isNull() || ! t.isEmpty())
          result.append (t).append (" ");
      }

  return result;
}


QString morse_to_lang (const QString &s, const QString &lang)
{
  QHash<QString, QString> h = hash_load_keyval (":/text-data/morse-" + lang);

  QStringList sl = s.toUpper().split (" ");

  QString result;

  int c = sl.size();
  for (int i = 0; i < c; i++)
      {
       QString t = h.key (sl[i]);
       if (! t.isNull() || ! t.isEmpty())
          result.append (t);
      }

  return result;
}


void rvln::fn_morse_from_en()
{
  CDocument *d = documents->get_current();
  if (d)
     d->textEdit->textCursor().insertText (morse_from_lang (d->textEdit->textCursor().selectedText().toUpper(), "en"));
}


void rvln::fn_morse_to_en()
{
  CDocument *d = documents->get_current();
  if (d)
     d->textEdit->textCursor().insertText (morse_to_lang (d->textEdit->textCursor().selectedText(), "en"));
}


void rvln::fn_morse_from_ru()
{
  CDocument *d = documents->get_current();
  if (d)
     d->textEdit->textCursor().insertText (morse_from_lang (d->textEdit->textCursor().selectedText().toUpper(), "ru"));
}


void rvln::fn_morse_to_ru()
{
  CDocument *d = documents->get_current();
  if (d)
     d->textEdit->textCursor().insertText (morse_to_lang (d->textEdit->textCursor().selectedText(), "ru"));
}


void rvln::nav_focus_to_fif()
{
  fif->setFocus (Qt::OtherFocusReason);
}


void rvln::nav_focus_to_editor()
{
  CDocument *d = documents->get_current();
  if (d)
     d->textEdit->setFocus (Qt::OtherFocusReason);
}


void rvln::edit_copy_current_fname()
{
  CDocument *d = documents->get_current();
  if (d)
     QApplication::clipboard()->setText (d->file_name);
}


void rvln::fn_insert_date()
{
  CDocument *d = documents->get_current();
  if (d)
     d->textEdit->textCursor().insertText (QDate::currentDate ().toString (settings->value("date_format", "dd/MM/yyyy").toString()));
}


void rvln::fn_insert_time()
{
  CDocument *d = documents->get_current();
  if (d)
     d->textEdit->textCursor().insertText (QTime::currentTime ().toString (settings->value("time_format", "hh:mm:ss").toString()));
}


void rvln::fn_rm_formatting_at_each_line()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  d->textEdit->textCursor().insertText (qstringlist_process (d->textEdit->textCursor().selectedText(), "", QSTRL_PROC_REMOVE_FORMATTING));
}


void rvln::fn_number_arabic_to_roman()
{
  CDocument *d = documents->get_current();
  if (d)
     d->textEdit->textCursor().insertText (arabicToRoman (d->textEdit->textCursor().selectedText().toUInt()));
}


void rvln::fn_number_roman_to_arabic()
{
  CDocument *d = documents->get_current();
  if (d)
     d->textEdit->textCursor().insertText (QString::number(romanToDecimal (d->textEdit->textCursor().selectedText().toUpper().toUtf8().data())));
}


void rvln::help_show_gpl()
{
  CDocument *d = documents->open_file (":/COPYING", "UTF-8");
  if (d)
     d->textEdit->setReadOnly (true);
}


void rvln::update_dyn_menus()
{
  update_templates();
  update_snippets();
  update_scripts();
  update_palettes();
  //opt_update_keyb();
  update_view_hls();
  update_tables();
  update_profiles();
  update_labels_menu();
}


void rvln::file_open_bookmarks_file()
{
  documents->open_file (fname_bookmarks, "UTF-8");
}


void rvln::file_open_programs_file()
{
//#if defined(Q_WS_WIN) || defined(Q_OS_OS2)
#if defined(Q_OS_WIN) || defined(Q_OS_OS2)


  if (! file_exists (fname_programs))
     qstring_save (fname_programs, tr ("#external programs list. example:\nopera=\"C:\\Program Files\\Opera\\opera.exe \" \"%s\""));

#else

  if (! file_exists (fname_programs))
     qstring_save (fname_programs, tr ("#external programs list. example:\nopera=opera %s"));

#endif

  documents->open_file (fname_programs, "UTF-8");
}


void rvln::process_readyReadStandardOutput()
{
  QProcess *p = qobject_cast<QProcess *>(sender());
  QByteArray a = p->readAllStandardOutput().data();  
  QTextCodec *c = QTextCodec::codecForLocale(); 
  QString t = c->toUnicode (a); 
  
  log->log (t);
}


void rvln::file_open_program()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  QAction *a = qobject_cast<QAction *>(sender());
  QString command = programs.value(a->text());
  if (command.isNull() || command.isEmpty())
     return;

  command = command.replace ("%s", d->file_name);
  
  QProcess *process  = new QProcess (this);
  
  connect (process, SIGNAL(readyReadStandardOutput()), this, SLOT(process_readyReadStandardOutput()));
  process->setProcessChannelMode(QProcess::MergedChannels) ;
  
  process->start (command, QIODevice::ReadWrite);
}


void rvln::update_programs()
{
  if (! file_exists (fname_programs))
     return;

  programs = hash_load_keyval (fname_programs);
  if (programs.count() < 0)
     return;

  menu_programs->clear();

  create_menu_from_list (this, menu_programs,
                         programs.keys(),
                         SLOT (file_open_program()));
}


void rvln::fn_insert_template_html()
{
  CDocument *d = documents->get_current();
  if (d)
     d->textEdit->textCursor().insertText (qstring_load (":/text-data/template-html"));
}


void rvln::fn_insert_template_html5()
{
  CDocument *d = documents->get_current();
  if (d)
     d->textEdit->textCursor().insertText (qstring_load (":/text-data/template-html5"));
}


void rvln::view_hide_error_marks()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  QTextCursor cr = d->textEdit->textCursor();

//delete all underlines
  cr.setPosition (0);
  cr.movePosition (QTextCursor::End, QTextCursor::KeepAnchor);
  QTextCharFormat f = cr.blockCharFormat();
  f.setFontUnderline (false);
  cr.mergeCharFormat (f);
  d->textEdit->document()->setModified (false);
}


void rvln::fn_rm_formatting()
{
  CDocument *d = documents->get_current();
  if (d)
     d->textEdit->textCursor().insertText (d->textEdit->textCursor().selectedText().simplified());
}


void rvln::view_toggle_fs()
{
  setWindowState(windowState() ^ Qt::WindowFullScreen);
}


void rvln::help_show_news()
{
  QString fname = ":/NEWS";
  if (QLocale::system().name().left(2) == "ru")
     fname = ":/NEWS-RU";

  CDocument *d = documents->open_file (fname, "UTF-8");
  if (d)
     d->textEdit->setReadOnly (true);
}


void rvln::help_show_changelog()
{
  CDocument *d = documents->open_file (":/ChangeLog", "UTF-8");
  if (d)
     d->textEdit->setReadOnly (true);
}


void rvln::help_show_todo()
{
  CDocument *d = documents->open_file (":/TODO", "UTF-8");
  if (d)
     d->textEdit->setReadOnly (true);
}


void CAboutWindow::closeEvent (QCloseEvent *event)
{
  event->accept();
}


void CTextListWindow::closeEvent (QCloseEvent *event)
{
  event->accept();
}


void CAboutWindow::update_image()
{
  QImage img (400, 100, QImage::Format_ARGB32);

  QPainter painter (&img);
  QFont f;
  f.setPixelSize (25);
  painter.setPen (Qt::gray);
  painter.setFont (f);

  for (int y = 1; y < 100; y += 25)
  for (int x = 1; x < 400; x += 25)
      {
       QColor color;
       
       int i = qrand() % 5;
       
       switch (i)
              {
               case 0: color = 0xfff3f9ff;
                       break;

               case 1: color = 0xffbfffb0;
                       break;

               case 2: color = 0xffa5a5a6;
                       break;
                       
               case 3: color = 0xffebffe9;
                       break;
               
               case 4: color = 0xffbbf6ff;
                       break;
              }
                  
       painter.fillRect (x, y, 25, 25, QBrush (color));
       
       if (i == 0) 
           painter.drawText (x, y + 25, "0");

       if (i == 1)
           painter.drawText (x, y + 25, "1");
     } 

  QString txt = "TEA";
  
  QFont f2 ("Monospace");
  f2.setPixelSize (75);
  painter.setFont (f2);
  
  painter.setPen (Qt::black);
  painter.drawText (4, 80, txt);
  
  painter.setPen (Qt::red);
  painter.drawText (2, 76, txt);

  logo->setPixmap (QPixmap::fromImage (img));
}
  

CAboutWindow::CAboutWindow()
{
  setAttribute (Qt::WA_DeleteOnClose);

  QStringList sl_t = qstring_load (":/AUTHORS").split ("##");

  logo = new QLabel;
  update_image();
    
  QTimer *timer = new QTimer(this);
  connect(timer, SIGNAL(timeout()), this, SLOT(update_image()));
  timer->start (1000);
  
  QTabWidget *tw = new QTabWidget (this);

  QPlainTextEdit *page_code = new QPlainTextEdit();
  QPlainTextEdit *page_thanks = new QPlainTextEdit();
  QPlainTextEdit *page_translators = new QPlainTextEdit();
  QPlainTextEdit *page_maintainers = new QPlainTextEdit();

  if (sl_t.size() == 4)
     {
      page_code->setPlainText (sl_t[0].trimmed());
      page_thanks->setPlainText (sl_t[3].trimmed());
      page_translators->setPlainText (sl_t[1].trimmed());
      page_maintainers->setPlainText (sl_t[2].trimmed());
     }

  tw->addTab (page_code, tr ("Code"));
  tw->addTab (page_thanks, tr ("Acknowledgements"));
  tw->addTab (page_translators, tr ("Translations"));
  tw->addTab (page_maintainers, tr ("Packages"));

  QVBoxLayout *layout = new QVBoxLayout();

  layout->addWidget(logo);
  layout->addWidget(tw);

  setLayout (layout);
  setWindowTitle (tr ("About"));
}


void rvln::cb_script_finished (int exitCode, QProcess::ExitStatus exitStatus)
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  QString s = qstring_load (fname_tempfile);
  if (! s.isNull() || ! s.isEmpty())
     d->textEdit->textCursor().insertText(s);

  QFile f (fname_tempfile);
  f.remove();
  f.setFileName (fname_tempparamfile);
  f.remove();
}


void rvln::update_scripts()
{
  menu_fn_scripts->clear();

  create_menu_from_dir (this,
                        menu_fn_scripts,
                        dir_scripts,
                        SLOT (fn_run_script())
                        );
}


void rvln::fn_run_script()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  QAction *a = qobject_cast<QAction *>(sender());

  QString fname = a->data().toString();
  QString ext = file_get_ext (fname);

  if (! d->textEdit->textCursor().hasSelection())
     return;

  QString intrp;

  if (ext == "rb")
     intrp = "ruby";
  else
  if (ext == "py")
     intrp = "python";
  else
  if (ext == "pl")
     intrp = "perl";
  else
  if (ext == "sh")
     intrp = "sh";

  if (intrp.isEmpty())
      return;

  qstring_save (fname_tempfile, d->textEdit->textCursor().selectedText());
  qstring_save (fname_tempparamfile, fif_get_text());

  QString command = QString ("%1 %2 %3 %4").arg (
                             intrp).arg (
                             fname).arg (
                             fname_tempfile).arg (
                             fname_tempparamfile);

  QProcess *process = new QProcess (this);
  connect(process, SIGNAL(finished ( int, QProcess::ExitStatus )), this, SLOT(cb_script_finished (int, QProcess::ExitStatus )));

  process->start (command);
}


void rvln::cb_button_saves_as()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  if (ed_fman_fname->text().isEmpty())
     return;

  QString filename (fman->dir.path());
  
  filename.append ("/").append (ed_fman_fname->text());

  if (file_exists (filename))
     if (QMessageBox::warning (this, "TEA",
                               tr ("%1 already exists\n"
                               "Do you want to overwrite?")
                               .arg (filename),
                               QMessageBox::Yes | QMessageBox::Default,
                               QMessageBox::Cancel | QMessageBox::Escape) == QMessageBox::Cancel)
         return;


   d->save_with_name (filename, cb_fman_codecs->currentText());
   d->set_markup_mode();

   add_to_last_used_charsets (cb_fman_codecs->currentText());

   d->set_hl();
   QFileInfo f (d->file_name);
   dir_last = f.path();
   update_dyn_menus();

   shortcuts->load_from_file (shortcuts->fname);

   fman->refresh();
   main_tab_widget->setCurrentIndex (idx_tab_edit);
}


void rvln::fman_home()
{
//#if defined(Q_WS_WIN) || defined(Q_OS_OS2)
#if defined(Q_OS_WIN) || defined(Q_OS_OS2)


  fman->nav ("c:/");

#else

  fman->nav (QDir::homePath());

#endif
}


void rvln::fman_add_bmk()
{
  sl_places_bmx.prepend (ed_fman_path->text());
  qstring_save (fname_places_bookmarks, sl_places_bmx.join ("\n"));
  update_places_bookmarks();
}


void rvln::fman_del_bmk()
{
  int i = lv_places->currentRow();
  if (i < 5)
     return;

  QString s = lv_places->item(i)->text();
  if (s.isNull() || s.isEmpty())
     return;

  i = sl_places_bmx.indexOf (s);
  sl_places_bmx.removeAt (i);
  qstring_save (fname_places_bookmarks, sl_places_bmx.join ("\n"));
  update_places_bookmarks();
}


void rvln::fman_naventry_confirm()
{
  fman->nav (ed_fman_path->text());
}


void rvln::fman_places_itemActivated (QListWidgetItem *item)
{
  int index = lv_places->currentRow();
  
  if (index == 0)
     {
      fman->nav (dir_templates);
      return;
     }
  else
  if (index == 1)
     {
      fman->nav (dir_snippets);
      return;
     }
  else
  if (index == 2)
     {
      fman->nav (dir_scripts);
      return;
     }
  else
  if (index == 3)
     {
      fman->nav (dir_tables);
      return;
     }
  else
  if (index == 4)
     {
      fman->nav (dir_config);
      return;
     }
  
  
  fman->nav (item->text());
}


void rvln::update_places_bookmarks()
{
  lv_places->clear();
  QStringList sl_items;
  sl_items << tr ("templates");
  sl_items << tr ("snippets");
  sl_items << tr ("scripts");
  sl_items << tr ("tables");
  sl_items << tr ("configs");
  
  lv_places->addItems (sl_items);

  if (! file_exists (fname_places_bookmarks))
     return;

  sl_places_bmx = qstring_load (fname_places_bookmarks).split ("\n");
  if (sl_places_bmx.size() != 0)
     lv_places->addItems (sl_places_bmx);
}


void rvln::fman_open()
{
  QString f = ed_fman_fname->text().trimmed();
  QStringList li = fman->get_sel_fnames();

  if (! f.isNull() || ! f.isEmpty())
  if (f[0] == '/')
     {
      CDocument *d = documents->open_file (f, cb_fman_codecs->currentText());
      if (d)
         {
          dir_last = get_file_path (d->file_name);
          charset = d->charset;
          add_to_last_used_charsets (cb_fman_codecs->currentText());
         }

      main_tab_widget->setCurrentIndex (idx_tab_edit);
      return;
     }

  if (li.size() == 0)
     {
      QString fname (fman->dir.path());
      fname.append ("/").append (f);
      CDocument *d = documents->open_file (fname, cb_fman_codecs->currentText());
      if (d)
         {
          dir_last = get_file_path (d->file_name);
          charset = d->charset;
          add_to_last_used_charsets (cb_fman_codecs->currentText());
         }

      main_tab_widget->setCurrentIndex (idx_tab_edit);
      return;
     }

  foreach (QString fname, li)
          {
           CDocument *d = 0;
           d = documents->open_file (fname, cb_fman_codecs->currentText());
           if (d)
              {
               dir_last = get_file_path (d->file_name);
               charset = d->charset;
              }
          }

  add_to_last_used_charsets (cb_fman_codecs->currentText());
  main_tab_widget->setCurrentIndex (idx_tab_edit);
}


void rvln::fman_create_dir()
{
  bool ok;
  QString newdir = QInputDialog::getText (this, tr ("Enter the name"),
                                                tr ("Name:"), QLineEdit::Normal,
                                                tr ("new_directory"), &ok);
  if (! ok || newdir.isEmpty())
     return;

  QString dname = fman->dir.path() + "/" + newdir;

  QDir d;
  if (! d.mkpath (dname))
     return;

  fman->nav (dname);
}


void rvln::fn_convert_quotes()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  QString source = d->textEdit->textCursor().selectedText();
  if (source.isEmpty())
     return;

  QString dest = conv_quotes (source);

  d->textEdit->textCursor().insertText (dest);
}


void rvln::fn_enum()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  QStringList source = d->textEdit->textCursor().selectedText().split (QChar::ParagraphSeparator);

  int pad = 0;
  int end = source.size() - 1;
  int step = 1;
  QString result;
  QString prefix;

  QStringList params = fif_get_text().split ("~");

  if (params.size() > 0)
     step = params[0].toInt();

  if (params.size() > 1)
     pad = params[1].toInt();

  if (params.size() > 2)
     prefix = params[2];

  for (int c = 0; c <= end; c++)
      {
       QString n;
       n = n.setNum (((c + 1) * step));
       if (pad != 0)
          n = n.rightJustified (pad, '0');

       result.append (n).append (prefix).append (source.at(c)).append ('\n');
      }

  d->textEdit->textCursor().insertText (result);
}


void rvln::view_stay_on_top()
{
  Qt::WindowFlags flags = windowFlags();
  flags ^= Qt::WindowStaysOnTopHint;
  setWindowFlags (flags );
  show();
  activateWindow();
}


void rvln::update_sessions()
{
  menu_file_sessions->clear();
  create_menu_from_dir (this,
                        menu_file_sessions,
                        dir_sessions,
                        SLOT (file_open_session())
                       );
}


void rvln::file_open_session()
{
  QAction *a = qobject_cast<QAction *>(sender());
  documents->load_from_session (a->data().toString());
}


void rvln::session_save_as()
{
  if (documents->list.size() < 0)
     return;

  bool ok;
  QString name = QInputDialog::getText (this, tr ("Enter the name"),
                                              tr ("Name:"), QLineEdit::Normal,
                                              tr ("new_session"), &ok);
  if (! ok || name.isEmpty())
     return;

  QString fname (dir_sessions);
  fname.append ("/").append (name);
  documents->save_to_session (fname);
  update_sessions();
}


void rvln::file_use_palette()
{
  QAction *a = qobject_cast<QAction *>(sender());
  QString fname (dir_palettes);
  fname.append ("/").append (a->text());

  if (! file_exists (fname))
     fname = ":/palettes/" + a->text();

  fname_def_palette = fname;
  documents->load_palette (fname);
  documents->apply_settings();

  update_logmemo_palette();
}


void rvln::update_logmemo_palette()
{
  QString text_color = hash_get_val (documents->palette, "text", "black");
  QString back_color = hash_get_val (documents->palette, "background", "white");
  QString sel_back_color = hash_get_val (documents->palette, "sel-background", "black");
  QString sel_text_color = hash_get_val (documents->palette, "sel-text", "white");

  QString sheet = QString ("QPlainTextEdit { color: %1; background-color: %2; selection-color: %3; selection-background-color: %4;}").arg (
                            text_color).arg (
                            back_color).arg (
                            sel_text_color).arg (
                            sel_back_color);

  log->memo->setStyleSheet (sheet);

  sheet = QString ("QLineEdit { color: %1; background-color: %2; selection-color: %3; selection-background-color: %4;}").arg (
                            text_color).arg (
                            back_color).arg (
                            sel_text_color).arg (
                            sel_back_color);

  fif->setStyleSheet (sheet);
}


void rvln::update_palettes()
{
  menu_view_palettes->clear();

  QStringList l1 = read_dir_entries (dir_palettes);
  QStringList l2 = read_dir_entries (":/palettes");
  l1 += l2;

  create_menu_from_list (this, menu_view_palettes,
                         l1,
                         SLOT (file_use_palette()));
}


void rvln::update_hls (bool force)
{
  documents->hls.clear();

  QStringList l1 = read_dir_entries (":/hls");
  l1 << read_dir_entries (dir_hls);
  QString newlist = l1.join("\n").trimmed();

  QString fname_hls_flist (dir_config);
  fname_hls_flist.append ("/fname_hls_flist");

  if (force)
     {
      QFile::remove (fname_hls_flist);
      qDebug() << "new version, hls cache is updated";
     }

  if (! file_exists (fname_hls_flist))
     {
      qstring_save (fname_hls_flist, l1.join ("\n").trimmed());
      QFile::remove (fname_hls_cache);
     }
  else
      {
       QString oldlist = qstring_load (fname_hls_flist);
       if (newlist != oldlist)
          {
           qstring_save (fname_hls_flist, newlist);
           QFile::remove (fname_hls_cache);
          }
      }

  
  if (! file_exists (fname_hls_cache))
     {
      for (int i = 0; i < l1.size(); i++)
          {
           QString fname = ":/hls/" + l1[i];
           if (! file_exists (fname))
              fname = dir_hls + "/" + l1[i];

           QString buffer = qstring_load (fname);
           QString exts = string_between (buffer, "exts=\"", "\"");
           if (! exts.isNull() || ! exts.isEmpty())
              {
               QStringList l = exts.split (";");
               for (int i = 0; i < l.size(); i++)
                   documents->hls.insert (l[i], fname);
              }
          }

      qstring_save (fname_hls_cache, hash_keyval_to_string (documents->hls));
     }
  else
      documents->hls = hash_load_keyval (fname_hls_cache);
}


void rvln::mrkup_preview_color()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  if (! d->textEdit->textCursor().hasSelection())
     return;
  
  QString color = d->textEdit->textCursor().selectedText();

  if (QColor::colorNames().indexOf (color) == - 1)
     {
      color = color.remove (";");
      if (! color.startsWith ("#"))
          color = "#" + color;
     }
  else
     {
      QColor c (color);
      color = c.name();
     }

  QString style = QString ("color:%1; font-weight:bold;").arg (color);
  log->log (tr ("<span style=\"%1\">COLOR SAMPLE</span>").arg (style));
}


void rvln::fman_drives_changed (const QString & path)
{
  if (! ui_update)
     fman->nav (path);
}


void rvln::createFman()
{
  QWidget *wd_fman = new QWidget (this);

  QVBoxLayout *lav_main = new QVBoxLayout;
  QVBoxLayout *lah_controls = new QVBoxLayout;
  QHBoxLayout *lah_topbar = new QHBoxLayout;

  QLabel *l_t = new QLabel (tr ("Name"));
  ed_fman_fname = new QLineEdit;
  connect (ed_fman_fname, SIGNAL(returnPressed()), this, SLOT(fman_fname_entry_confirm()));

  
  ed_fman_path = new QLineEdit;
  connect (ed_fman_path, SIGNAL(returnPressed()), this, SLOT(fman_naventry_confirm()));

  tb_fman_dir = new QToolBar;
  tb_fman_dir->setObjectName ("tb_fman_dir");

  QAction *act_fman_go = new QAction (QIcon( ":/icons/go.png"), tr ("Go"), this);
  connect (act_fman_go, SIGNAL(triggered()), this, SLOT(fman_naventry_confirm()));

  QAction *act_fman_home = new QAction (QIcon (":/icons/home.png"), tr ("Home"), this);
  connect (act_fman_home, SIGNAL(triggered()), this, SLOT(fman_home()));

  QAction *act_fman_refresh = new QAction (QIcon (":/icons/refresh.png"), tr ("Refresh"), this);
  QAction *act_fman_ops = new QAction (QIcon (":/icons/create-dir.png"), tr ("Operations"), this);
  act_fman_ops->setMenu (menu_fm_file_ops);

  tb_fman_dir->addAction (act_fman_go);
  tb_fman_dir->addAction (act_fman_home);
  tb_fman_dir->addAction (act_fman_refresh);
  tb_fman_dir->addAction (act_fman_ops);

//#if defined(Q_WS_WIN) || defined(Q_OS_OS2)
#if defined(Q_OS_WIN) || defined(Q_OS_OS2)


  cb_fman_drives = new QComboBox;
  lah_topbar->addWidget (cb_fman_drives);

  QFileInfoList l_drives = QDir::drives();
  foreach (QFileInfo fi, l_drives)
           cb_fman_drives->addItem (fi.path());
 
#endif

  lah_topbar->addWidget (ed_fman_path);
  lah_topbar->addWidget (tb_fman_dir);

  lah_controls->addWidget (l_t);
  lah_controls->addWidget (ed_fman_fname);

  l_t = new QLabel (tr ("Charset"));
  
  QPushButton *bt_magicenc = new QPushButton ("?", this);
        
  bt_magicenc->setMaximumWidth (QApplication::fontMetrics().width ("???"));
  connect (bt_magicenc, SIGNAL(clicked()), this, SLOT(guess_enc()));
  
    
  cb_fman_codecs = new QComboBox;

  if (sl_last_used_charsets.size () > 0)
     cb_fman_codecs->addItems (sl_last_used_charsets + sl_charsets);
  else
     {
      cb_fman_codecs->addItems (sl_charsets);
      cb_fman_codecs->setCurrentIndex (sl_charsets.indexOf ("UTF-8"));
     }
  
  QPushButton *bt_fman_open = new QPushButton (tr ("Open"), this);
  connect (bt_fman_open, SIGNAL(clicked()), this, SLOT(fman_open()));

  QPushButton *bt_fman_save_as = new QPushButton (tr ("Save as"), this);
  connect (bt_fman_save_as, SIGNAL(clicked()), this, SLOT(cb_button_saves_as()));

  lah_controls->addWidget (l_t);
    
  
  QHBoxLayout *lt_hb = new QHBoxLayout;
  
  lt_hb->addWidget (cb_fman_codecs);
  lt_hb->addWidget (bt_magicenc);
  
  lah_controls->addLayout (lt_hb); 

  lah_controls->addWidget (bt_fman_open);
  lah_controls->addWidget (bt_fman_save_as);

  fman = new CFMan;

  connect (fman, SIGNAL(file_activated (const QString &)), this, SLOT(fman_file_activated (const QString &)));
  connect (fman, SIGNAL(dir_changed  (const QString &)), this, SLOT(fman_dir_changed  (const QString &)));
  connect (fman, SIGNAL(current_file_changed  (const QString &, const QString &)), this, SLOT(fman_current_file_changed  (const QString &, const QString &)));

  connect (act_fman_refresh, SIGNAL(triggered()), fman, SLOT(refresh()));

//#if defined(Q_WS_WIN) || defined(Q_OS_OS2)
#if defined(Q_OS_WIN) || defined(Q_OS_OS2)


  connect (cb_fman_drives, SIGNAL(currentIndexChanged ( const QString & )),
          this, SLOT(fman_drives_changed(const QString & )));

#endif

  w_right = new QWidget (this);
  QVBoxLayout *lw_right = new QVBoxLayout;
  w_right->setLayout (lw_right);  
  
  lw_right->addLayout (lah_controls);

  QFrame *vline = new QFrame;
  vline->setFrameStyle (QFrame::HLine);
  lw_right->addWidget (vline);

  QLabel *l_bookmarks = new QLabel (tr ("<b>Bookmarks</b>"));
  lw_right->addWidget (l_bookmarks);


  QHBoxLayout *lah_places_bar = new QHBoxLayout;
  QPushButton *bt_add_bmk = new QPushButton ("+");
  QPushButton *bt_del_bmk = new QPushButton ("-");
  lah_places_bar->addWidget (bt_add_bmk);
  lah_places_bar->addWidget (bt_del_bmk);

  connect (bt_add_bmk, SIGNAL(clicked()), this, SLOT(fman_add_bmk()));
  connect (bt_del_bmk, SIGNAL(clicked()), this, SLOT(fman_del_bmk()));

  lv_places = new QListWidget;
  update_places_bookmarks();
  connect (lv_places, SIGNAL(itemActivated (QListWidgetItem *)), this, SLOT(fman_places_itemActivated (QListWidgetItem *)));

  QVBoxLayout *vbox = new QVBoxLayout;
  vbox->addLayout (lah_places_bar);
  vbox->addWidget (lv_places);

  lw_right->addLayout (vbox);
  
  fman->setSizePolicy (QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);

  spl_fman = new QSplitter (this);
  spl_fman->setChildrenCollapsible (true);

  spl_fman->addWidget (fman);
  spl_fman->addWidget (w_right);
  
  spl_fman->restoreState (settings->value ("spl_fman").toByteArray());

  lav_main->addLayout (lah_topbar);
  lav_main->addWidget (spl_fman);

  wd_fman->setLayout (lav_main);

  fman_home();
  
  idx_tab_fman = main_tab_widget->addTab (wd_fman, tr ("manage"));
}


void rvln::fman_file_activated (const QString &full_path)
{
  if (is_image (full_path))
     {
      CDocument *d = documents->get_current();
      if (! d)
       return;

      d->insert_image (full_path);
      main_tab_widget->setCurrentIndex (idx_tab_edit);
      return;
     }

  CDocument *d = documents->open_file (full_path, cb_fman_codecs->currentText());
  if (d)
     {
      dir_last = get_file_path (d->file_name);
      charset = d->charset;
     }

  add_to_last_used_charsets (cb_fman_codecs->currentText());
  main_tab_widget->setCurrentIndex (idx_tab_edit);
}


void rvln::fman_dir_changed (const QString &full_path)
{
  ui_update = true;
  ed_fman_path->setText (full_path);

//#if defined(Q_WS_WIN) || defined(Q_OS_OS2)
#if defined(Q_OS_WIN) || defined(Q_OS_OS2)


  cb_fman_drives->setCurrentIndex (cb_fman_drives->findText (full_path.left(3).toUpper()));

#endif

  ui_update = false;
}


void rvln::fman_current_file_changed (const QString &full_path, const QString &just_name)
{
  ed_fman_fname->setText (just_name);

  if (b_preview)
  if (is_image (full_path))
     {
      if (! img_viewer->window_mini.isVisible())
         {
          img_viewer->window_mini.show();
          activateWindow();
          fman->setFocus();
         }
      img_viewer->set_image_mini (full_path);
     }
}


void rvln::fman_rename()
{
  QString fname = fman->get_sel_fname();
  if (fname.isNull() || fname.isEmpty())
     return;
  
  QFileInfo fi (fname);
  if (! fi.exists() && ! fi.isWritable())
     return;
  
  bool ok;
  QString newname = QInputDialog::getText (this, tr ("Enter the name"),
                                                 tr ("Name:"), QLineEdit::Normal,
                                                 tr ("new"), &ok);
  if (! ok || newname.isEmpty())
     return;
  
  QString newfpath (fi.path());
  newfpath.append ("/").append (newname);
  QFile::rename (fname, newfpath);
  update_dyn_menus();
  fman->refresh();

  QModelIndex index = fman->index_from_name (newname);
  fman->selectionModel()->setCurrentIndex (index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
  fman->scrollTo (index, QAbstractItemView::PositionAtCenter);
}


void rvln::fman_delete()
{
  QString fname = fman->get_sel_fname();
  if (fname.isNull() || fname.isEmpty())
     return;

  int i = fman->get_sel_index();

  QFileInfo fi (fname);
  if (! fi.exists() && ! fi.isWritable())
     return;
 
  if (QMessageBox::warning (this, "TEA",
                            tr ("Are you sure to delete\n"
                            "%1?").arg (fname),
                            QMessageBox::Yes | QMessageBox::Default,
                            QMessageBox::No | QMessageBox::Escape) == QMessageBox::No)
      return;
 
  QFile::remove (fname);
  update_dyn_menus();
  fman->refresh();

  if (i < fman->list.count())
     {
      QModelIndex index = fman->index_from_idx (i);
      fman->selectionModel()->setCurrentIndex (index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
      fman->scrollTo (index, QAbstractItemView::PositionAtCenter);
     }
}


void rvln::fm_hashsum_md5()
{
  QString filename = fman->get_sel_fname();
  if (! file_exists (filename))
      return;

  QCryptographicHash h (QCryptographicHash::Md5);

  h.addData (file_load (filename));
  QString sm = h.result().toHex();
  QString s = tr ("MD5 checksum for %1 is %2").arg (filename).arg (sm);

  log->log (s);
}


void rvln::fm_hashsum_md4()
{
  QCryptographicHash h (QCryptographicHash::Md4);
  QString filename = fman->get_sel_fname();

  if (! file_exists (filename))
      return;

  h.addData (file_load (filename));
  QString sm = h.result().toHex();
  QString s = tr ("MD4 checksum for %1 is %2").arg (filename).arg (sm);

  log->log (s);
}


void rvln::fm_hashsum_sha1()
{
  QString filename = fman->get_sel_fname();
  if (! file_exists (filename))
      return;

  QCryptographicHash h (QCryptographicHash::Sha1);

  h.addData (file_load (filename));
  QString sm = h.result().toHex();
  QString s = tr ("SHA1 checksum for %1 is %2").arg (filename).arg (sm);

  log->log (s);
}


void rvln::fman_refresh()
{
  fman->refresh();
}


void rvln::fm_full_info()
{
  QString fname;

  if (main_tab_widget->currentIndex() == idx_tab_fman)
     fname = fman->get_sel_fname();
  else
      {
       CDocument *d = documents->get_current();
       if (d)
          fname = d->file_name;
      }

  QFileInfo fi (fname);
  if (! fi.exists())
     return;

  QStringList l;

//detect EOL

  QFile f (fname);
  if (f.open (QIODevice::ReadOnly))
     {
      QString n (tr("End of line: "));     
      QByteArray barr = f.readAll();
      
      int nl = barr.count ('\n');
      int cr = barr.count ('\r');

      if (nl > 0 && cr == 0) 
         n += "UNIX";

      if (nl > 0 && cr > 0) 
         n += "Windows";

      if (nl == 0 && cr > 0) 
         n += "Mac";

      l.append (n);
     } 


  l.append (tr ("file name: %1").arg (fi.absoluteFilePath()));
  l.append (tr ("size: %1 kbytes").arg (QString::number (fi.size() / 1024)));
  l.append (tr ("created: %1").arg (fi.created().toString ("yyyy-MM-dd@hh:mm:ss")));
  l.append (tr ("modified: %1").arg (fi.lastModified().toString ("yyyy-MM-dd@hh:mm:ss")));

  if (file_get_ext (fname) == "wav")
     {
      CWavReader wr;
      wr.get_info (fname);
      l.append (tr ("bits per sample: %1").arg (wr.wav_chunk_fmt.bits_per_sample));
      l.append (tr ("number of channels: %1").arg (wr.wav_chunk_fmt.num_channels));
      l.append (tr ("sample rate: %1").arg (wr.wav_chunk_fmt.sample_rate));
      if (wr.wav_chunk_fmt.bits_per_sample == 16)
         l.append (tr ("RMS for all channels: %1 dB").arg (wr.rms));
     }
  
  log->log (l.join ("<br>"));
}


void rvln::file_reload()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  d->reload (d->charset);
}


CTextListWindow::CTextListWindow (const QString &title, const QString &label_text)
{
  setAttribute (Qt::WA_DeleteOnClose);
  QVBoxLayout *lt = new QVBoxLayout;

  QLabel *l = new QLabel (label_text);

  list = new QListWidget (this);
   
  lt->addWidget (l);
  lt->addWidget (list);
  
  setLayout (lt);
  setWindowTitle (title);
}


void rvln::file_reload_enc_itemDoubleClicked (QListWidgetItem *item)
{
  CDocument *d = documents->get_current();
  if (d)
     d->reload (item->text());
}


void rvln::file_reload_enc()
{
  CTextListWindow *w = new CTextListWindow (tr ("Reload with encoding"), tr ("Charset"));

  if (sl_last_used_charsets.size () > 0)
     w->list->addItems (sl_last_used_charsets + sl_charsets);
  else
      w->list->addItems (sl_charsets);
     
  connect (w->list, SIGNAL(itemDoubleClicked ( QListWidgetItem *)), 
           this, SLOT(file_reload_enc_itemDoubleClicked ( QListWidgetItem *)));

  w->show();
}


void rvln::handle_args()
{
  QStringList l = qApp->arguments();
  int size = l.size();
  if (size < 2)
     return;

  QString charset = "UTF-8";
  
  for (int i = 1; i < size; i++)
      {
       QString t = l.at(i);
       if (t.startsWith("--charset"))
          {
           QStringList pair = t.split ("=");
           if (pair.size() > 1)
              charset = pair[1];
          }
       else 
           {
            QFileInfo f (l.at(i));

            if (! f.isAbsolute())
               {
                QString fullname (QDir::currentPath());
                fullname.append ("/").append (l.at(i));
                documents->open_file (fullname, charset);
               }
            else
                documents->open_file (l.at(i), charset);
          }
      }
}


bool CStrIntPair_bigger_than (CStrIntPair *o1, CStrIntPair *o2)
{
   return o1->int_value > o2->int_value;
}


bool CStrIntPair_bigger_than_str (CStrIntPair *o1, CStrIntPair *o2)
{
   return o1->string_value < o2->string_value;
}


void rvln::run_unitaz (int mode)
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  pb_status->show();
  pb_status->setFormat (tr ("%p% completed"));
  pb_status->setTextVisible (true);

  int c = 0;

  QStringList total = d->get_words();
  QHash <QString, int> h;

  pb_status->setRange (0, total.size() - 1);

  foreach (QString w, total)
          {
           if (c % 100 == 0)
              qApp->processEvents();

           QHash<QString, int>::iterator i = h.find (w.toLower());
           if (i != h.end())
              i.value() += 1;
           else
                h.insert(w.toLower(), 1);

           pb_status->setValue (c++);
          }

  QStringList l;
  QList <CStrIntPair*> uwords;

  foreach (QString s, h.keys())
           uwords.append (new CStrIntPair (s, h.value (s)));

  if (mode == 0)
     qSort (uwords.begin(), uwords.end(), CStrIntPair_bigger_than);
  if (mode == 1)
     qSort (uwords.begin(), uwords.end(), CStrIntPair_bigger_than_str);

  QStringList outp;

  foreach (CStrIntPair *sp, uwords)
          outp.append (sp->string_value + " = " + QString::number (sp->int_value));

  double diff = static_cast <double> (total.size()) / static_cast <double> (uwords.size());
  double diff_per_cent = get_percent (static_cast <double> (total.size()), static_cast <double> (uwords.size()));

  outp.prepend (tr ("total to unique per cent diff: %1").arg (diff_per_cent, 0, 'f', 6));
  outp.prepend (tr ("total / unique: %1").arg (diff, 0, 'f', 6));
  outp.prepend (tr ("words unique: %1").arg (uwords.size()));
  outp.prepend (tr ("words total: %1").arg (total.size()));
  outp.prepend (tr ("text analysis of: %1").arg (d->file_name));
  outp.prepend (tr ("UNITAZ: UNIverlsal Text AnalyZer"));

  CDocument *nd = documents->create_new();
  nd->textEdit->textCursor().insertText (outp.join ("\n"));

  while (! uwords.isEmpty())
        delete uwords.takeFirst();

  pb_status->hide();
}


void rvln::fn_get_words_count()
{
  run_unitaz (0);
}


void rvln::fn_unitaz_abc()
{
  run_unitaz (1);
}


CStrIntPair::CStrIntPair (const QString &s, int i): QObject()
{
   string_value = s;
   int_value = i;
}


void rvln::create_markup_hash()
{
  CMarkupPair *p = new CMarkupPair;

  p->tags_a["Docbook"] = "<emphasis role=\"bold\">";
  p->tags_b["Docbook"] = "</emphasis>";

  p->tags_a["LaTeX"] = "\\textbf{";
  p->tags_b["LaTeX"] = "} ";

  p->tags_a["HTML"] = "<b>";
  p->tags_b["HTML"] = "</b>";

  p->tags_a["XHTML"] = "<b>";
  p->tags_b["XHTML"] = "</b>";

  p->tags_a["Lout"] = "@B{ ";
  p->tags_b["Lout"] = " }";

  p->tags_a["MediaWiki"] = "'''";
  p->tags_b["MediaWiki"] = "'''";

  p->tags_a["DokuWiki"] = "**";
  p->tags_b["DokuWiki"] = "**";

  
  hs_markup.insert("bold", p);


  p = new CMarkupPair;

  p->tags_a["Docbook"] = "<emphasis>";
  p->tags_b["Docbook"] = "</emphasis>";

  p->tags_a["LaTeX"] = "\\textit{";
  p->tags_b["LaTeX"] = "} ";

  p->tags_a["HTML"] = "<i>";
  p->tags_b["HTML"] = "</i>";

  p->tags_a["XHTML"] = "<i>";
  p->tags_b["XHTML"] = "</i>";

  p->tags_a["Lout"] = "@I{ ";
  p->tags_b["Lout"] = " }";
  

  p->tags_a["MediaWiki"] = "''";
  p->tags_b["MediaWiki"] = "''";

  p->tags_a["DokuWiki"] = "//";
  p->tags_b["DokuWiki"] = "//";



  hs_markup.insert("italic", p);

  
  p = new CMarkupPair;

  p->tags_a["Docbook"] = "<emphasis>";
  p->tags_b["Docbook"] = "</emphasis>";

  p->tags_a["LaTeX"] = "\\underline{";
  p->tags_b["LaTeX"] = "} ";

  p->tags_a["HTML"] = "<u>";
  p->tags_b["HTML"] = "</u>";

  p->tags_a["XHTML"] = "<u>";
  p->tags_b["XHTML"] = "</u>";

  p->tags_a["Lout"] = "@Underline{ ";
  p->tags_b["Lout"] = " }";
    
  p->tags_a["MediaWiki"] = "<u>";
  p->tags_b["MediaWiki"] = "</u>";

  p->tags_a["DokuWiki"] = "__";
  p->tags_b["DokuWiki"] = "__";



  hs_markup.insert("underline", p);


  p = new CMarkupPair;

  p->tags_a["Docbook"] = "<para>";
  p->tags_b["Docbook"] = "</para>";

  p->tags_a["HTML"] = "<p>";
  p->tags_b["HTML"] = "</p>";

  p->tags_a["XHTML"] = "<p>";
  p->tags_b["XHTML"] = "</p>";

  p->tags_a["Lout"] = "@PP";
  p->tags_b["Lout"] = "";
  
  hs_markup.insert("para", p);


  p = new CMarkupPair;

  p->tags_a["Docbook"] = "<ulink url=\"\">";
  p->tags_b["Docbook"] = "</ulink>";

  p->tags_a["HTML"] = "<a href=\"\">";
  p->tags_b["HTML"] = "</a>";

  p->tags_a["XHTML"] = "<a href=\"\">";
  p->tags_b["XHTML"] = "</a>";

  hs_markup.insert("link", p);


  p = new CMarkupPair;

  p->tags_a["LaTeX"] = "\\newline";
  p->tags_a["HTML"] = "<br>";
  p->tags_a["XHTML"] = "<br />";
  p->tags_a["Lout"] = "@LLP";
  p->tags_a["MediaWiki"] = "<br />";
  p->tags_a["DokuWiki"] = "\\\\ ";


  hs_markup.insert ("newline", p);
}


void rvln::count_substring (bool use_regexp)
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  QString text;

  if (d->textEdit->textCursor().hasSelection())
     text = d->textEdit->textCursor().selectedText();
  else
      text = d->textEdit->toPlainText();

  int count = 0;
  Qt::CaseSensitivity cs = Qt::CaseInsensitive;

  if (menu_find_case->isChecked())
     cs = Qt::CaseSensitive;

  if (use_regexp)
     count = text.count (QRegExp (fif_get_text()));
  else
      count = text.count (fif_get_text(), cs);

  log->log (tr ("%1 number of occurrences of %2 is found").arg (count).arg (fif->text()));
}


void rvln::fn_count()
{
  count_substring (false);
}


void rvln::fn_count_rx()
{
  count_substring (true);
}


void rvln::read_search_options()
{
  menu_find_whole_words->setChecked (settings->value ("find_whole_words", "0").toBool());
  menu_find_case->setChecked (settings->value ("find_case", "0").toBool());
  menu_find_regexp->setChecked (settings->value ("find_regexp", "0").toBool());
  menu_find_fuzzy->setChecked (settings->value ("find_fuzzy", "0").toBool());

}


void rvln::write_search_options()
{
  settings->setValue ("find_whole_words", menu_find_whole_words->isChecked());
  settings->setValue ("find_case", menu_find_case->isChecked());
  settings->setValue ("find_regexp", menu_find_regexp->isChecked());
  settings->setValue ("find_fuzzy", menu_find_fuzzy->isChecked());

}


void rvln::main_tab_page_changed (int index)
{
  if (idx_prev == idx_tab_fman)
     if (img_viewer && img_viewer->window_mini.isVisible())
         img_viewer->window_mini.close();

  if (idx_prev == idx_tab_tune)
      leaving_tune();

  idx_prev = index;
      
  if (index == idx_tab_fman)
     {
      fman->setFocus();
      fm_entry_mode = FM_ENTRY_MODE_NONE;
      idx_tab_fman_activate();
     }
  else       
  if (index == idx_tab_calendar)
    {
     calendar_update();
     idx_tab_calendar_activate();
    }
  else
  if (index == idx_tab_edit)
     idx_tab_edit_activate();
  else   
  if (index == idx_tab_tune)
     idx_tab_tune_activate();
  else   
  if (index == idx_tab_learn)
     idx_tab_learn_activate();
}


QString rvln::fif_get_text()
{
  QString t = fif->text();

  int i = sl_fif_history.indexOf (t);

  if (i != -1)
     {
      sl_fif_history.removeAt (i);
      sl_fif_history.prepend (t);
     }
  else
      sl_fif_history.prepend (t);

  if (sl_fif_history.count() > 77)
     sl_fif_history.removeLast();

  return t;
}


void rvln::fn_rm_trailing_spaces()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  QStringList sl = d->textEdit->textCursor().selectedText().split (QChar::ParagraphSeparator);

  QMutableListIterator <QString> i (sl);

  while (i.hasNext())
        {
         QString s = i.next();

         if (s.isEmpty())
            continue;

         if (s.at (s.size() - 1).isSpace())
            {
             int index = s.size() - 1;
             while (s.at (--index).isSpace())
                   ;

             s.truncate (index + 1);
             i.setValue (s);
            }
        }

  QString x = sl.join ("\n"); 
  
  d->textEdit->textCursor().insertText (x);
}


void rvln::fman_convert_images (bool by_side, int value)
{
  qsrand (QTime::currentTime().msec());

  QString dir_out ("images-out-");
  dir_out.append (QString::number (qrand() % 777));

  dir_out.prepend ("/");
  dir_out.prepend (fman->dir.absolutePath());

  if (! fman->dir.mkpath (dir_out))
     return;

  Qt::TransformationMode transformMode = Qt::FastTransformation;
  if (settings->value ("img_filter", 0).toBool())
     transformMode = Qt::SmoothTransformation;

  pb_status->show();
  pb_status->setFormat (tr ("%p% completed"));
  pb_status->setTextVisible (true);

  QStringList li = fman->get_sel_fnames();

  int quality = settings->value ("img_quality", "-1").toInt();

  pb_status->setRange (0, li.size() - 1 );
  int i = 0;

  foreach (QString fname, li)
          if (is_image (fname))
             {
              QImage source (fname);

              if (! source.isNull())
                 {
                  qApp->processEvents();
                  QImage dest = image_scale_by (source, by_side, value, transformMode);

                  QString fmt (settings->value ("output_image_fmt", "jpg").toString());

                  QFileInfo fi (fname);
                  QString dest_fname (dir_out);
                  dest_fname.append ("/");
                  dest_fname.append (fi.fileName());
                  dest_fname = change_file_ext (dest_fname, fmt);

                  if (! dest.save (dest_fname, fmt.toLatin1().constData(), quality))
                      qDebug() << "Cannot save " << dest_fname;

                  pb_status->setValue (i++);
                 }
             }

  pb_status->hide();

  if (settings->value ("img_post_proc", 0).toBool())
     {
      CZipper zipper;
      zipper.zip_directory (fman->dir.absolutePath(), dir_out);
     }

  fman->refresh();
}


void rvln::fman_img_conv_by_side()
{
  int side = fif_get_text().toInt();
  if (side == 0)
     return;

  fman_convert_images (true, side);
}


void rvln::fman_img_conv_by_percent()
{
  int percent = fif_get_text().toInt();
  if (percent == 0)
     return;

  fman_convert_images (false, percent);
}


void rvln::fn_escape()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  d->textEdit->textCursor().insertText (
     QRegExp::escape (d->textEdit->textCursor().selectedText()));
}


void rvln::fman_add_to_zip()
{
  QString f = ed_fman_fname->text().trimmed();
  QStringList li = fman->get_sel_fnames();

  if (! f.isNull() || ! f.isEmpty())
  if (f[0] == '/')
     {
      fman->zipper.files_list.append (f);
      return;
     }

  if (li.size() == 0)
     {
      QString fname (fman->dir.path());
      fname.append ("/").append (f);
      fman->zipper.files_list.append (fname);
      return;
     }

  foreach (QString fname, li)
          fman->zipper.files_list.append (fname);
}


void rvln::fman_create_zip()
{
  bool ok;
   
  QString name = QInputDialog::getText (this, tr ("Enter the archive name"),
                                              tr ("Name:"), QLineEdit::Normal,
                                              tr ("new_archive"), &ok);
 
  if (! ok)
     return;
  
  fman->zipper.files_list.clear();
  fman->zipper.archive_name = name;
  
  if (! name.endsWith (".zip"))
     name.append (".zip");
 
  fman->zipper.archive_fullpath = fman->dir.path() + "/" + name;
}


void rvln::fman_save_zip()
{
  fman->zipper.pack_prepared();
  fman->refresh();
}


void rvln::fman_preview_image()
{
  QString fname = fman->get_sel_fname();
  if (fname.isNull() || fname.isEmpty())
     return;
  
  if (is_image (fname))
     {
      img_viewer->window_full.show();
      img_viewer->set_image_full (fname);
     }
}


void rvln::ed_indent()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  d->textEdit->indent();
}  


void rvln::ed_unindent()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  d->textEdit->un_indent();
}  


void rvln::fn_sort_casecareless()
{
 
  CDocument *d = documents->get_current();
  if (! d)
     return;
  
  d->textEdit->textCursor().insertText (qstringlist_process (
                                        d->textEdit->textCursor().selectedText(),
                                        fif_get_text(), 
                                        QSTRL_PROC_FLT_WITH_SORTNOCASECARE));
}


void rvln::fman_fname_entry_confirm()
{

  if (fm_entry_mode == FM_ENTRY_MODE_OPEN)
     fman_open();     
   
  if (fm_entry_mode == FM_ENTRY_MODE_SAVE)
     cb_button_saves_as();
}


void rvln::update_view_hls()
{
  menu_view_hl->clear();
  
  QStringList l = documents->hls.uniqueKeys();
  l.sort();

  create_menu_from_list (this, menu_view_hl,
                         l,
                         SLOT (file_use_hl()));
}


void rvln::file_use_hl()
{
  QAction *a = qobject_cast<QAction *>(sender());

  CDocument *d = documents->get_current();
  if (! d)
     return;
  
  d->set_hl (false, a->text());
}


void rvln::fn_strip_html_tags()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  QString text;

  if (d->textEdit->textCursor().hasSelection())
     text = d->textEdit->textCursor().selectedText();
  else
      text = d->textEdit->toPlainText();

  if (d->textEdit->textCursor().hasSelection())
     d->textEdit->textCursor().insertText (strip_html (text));
  else
      d->textEdit->setPlainText (strip_html (text));
}


void rvln::fn_number_decimal_to_binary()
{
  CDocument *d = documents->get_current();
  if (d)
     d->textEdit->textCursor().insertText (int_to_binary (d->textEdit->textCursor().selectedText().toInt()));
}


void rvln::fn_number_flip_bits()
{
  CDocument *d = documents->get_current();
  if (!d)
     return;
  
  QString s = d->textEdit->textCursor().selectedText();
  for (int i = 0; i < s.size(); i++)
      {
       if (s[i] == '1')
          s[i] = '0';
       else          
       if (s[i] == '0')
          s[i] = '1';
      }   
  
  d->textEdit->textCursor().insertText (s);
}


void rvln::fn_use_table()
{
  QAction *a = qobject_cast<QAction *>(sender());

  if (main_tab_widget->currentIndex() == idx_tab_edit)
    {
     CDocument *d = documents->get_current();
     if (! d)
        return;
  
     QString text;

     if (d->textEdit->textCursor().hasSelection())
         text = d->textEdit->textCursor().selectedText();
     else
         text = d->textEdit->toPlainText();
  
     if (d->textEdit->textCursor().hasSelection())
        d->textEdit->textCursor().insertText (apply_table (text, a->data().toString(), menu_find_regexp->isChecked()));
     else
         d->textEdit->setPlainText (apply_table (text, a->data().toString(), menu_find_regexp->isChecked()));
     }
 else
     if (main_tab_widget->currentIndex() == idx_tab_fman)
        {
         QStringList sl = fman->get_sel_fnames();

         if (sl.size() < 1)
            return;

         char *charset = cb_fman_codecs->currentText().toLatin1().data();

         foreach (QString fname, sl)
                 {
                  QString f = qstring_load (fname, charset);
                  QString r = apply_table (f, a->data().toString(), menu_find_regexp->isChecked());
                  qstring_save (fname, r, charset);
                  log->log (tr ("%1 is processed and saved").arg (fname));
                 }
        }
}


void rvln::update_tables()
{
  menu_fn_tables->clear();

  create_menu_from_dir (this,
                        menu_fn_tables,
                        dir_tables,
                        SLOT (fn_use_table())
                        );
}


void rvln::fn_binary_to_decimal()
{
  CDocument *d = documents->get_current();
  if (d)
     d->textEdit->textCursor().insertText (QString::number (bin_to_decimal (d->textEdit->textCursor().selectedText())));
}


#ifdef SPELLCHECK_ENABLE

void rvln::cmb_spellchecker_currentIndexChanged (const QString &text)
{
  cur_spellchecker = text;


  qDebug() << "cur_spellchecker = " << cur_spellchecker;

  settings->setValue ("cur_spellchecker", cur_spellchecker);

  delete spellchecker;

  if (! spellcheckers.contains (cur_spellchecker))
     cur_spellchecker = spellcheckers[0]; 

  
#ifdef ASPELL_ENABLE
  if (cur_spellchecker == "Aspell")
     spellchecker = new CSpellchecker (settings->value ("spell_lang", QLocale::system().name().left(2)).toString());
#endif

  
#ifdef HUNSPELL_ENABLE
   if (cur_spellchecker == "Hunspell")
      spellchecker = new CHunspellChecker (settings->value ("spell_lang", QLocale::system().name().left(2)).toString(), "/home/rox/devel/test/hunspell");  
#endif
  
 create_spellcheck_menu();
}

#ifdef HUNSPELL_ENABLE

void rvln::pb_choose_hunspell_path_clicked()
{
 /* QFileDialog dialog;
  dialog.setOption (QFileDialog::ShowDirsOnly, true);
  dialog.setFileMode (QFileDialog::Directory);
  if (dialog.exec())
     {
      QString path = dialog.directory().absolutePath(); 
      settings->setValue ("hunspell_dic_path", path);     
      ed_spellcheck_path->setText (path);

      delete spellchecker;

      setup_spellcheckers();
      create_spellcheck_menu();
     }
*/

  QString path = QFileDialog::getExistingDirectory (this, tr ("Open Directory"), "/",
                                                    QFileDialog::ShowDirsOnly |
                                                QFileDialog::DontResolveSymlinks);
  if (! path.isEmpty())
  {
   settings->setValue ("hunspell_dic_path", path);
   ed_spellcheck_path->setText (path);

   delete spellchecker;

   setup_spellcheckers();
   create_spellcheck_menu();
  }
}

#endif


#ifdef ASPELL_ENABLE
//#if defined(Q_WS_WIN) || defined (Q_OS_OS2)
#if defined(Q_OS_WIN) || defined(Q_OS_OS2)


void rvln::pb_choose_aspell_path_clicked()
{
 /* QFileDialog dialog;
  dialog.setOption (QFileDialog::ShowDirsOnly, true);
  dialog.setFileMode (QFileDialog::Directory);
  if (dialog.exec())
     {
      QString path = dialog.directory().absolutePath();
      settings->setValue ("win32_aspell_path", path);
      ed_aspellcheck_path->setText (path);

      delete spellchecker;
      setup_spellcheckers();
	  create_spellcheck_menu();
     }*/

    QString path = QFileDialog::getExistingDirectory (this, tr ("Open Directory"), "/",
                                                      QFileDialog::ShowDirsOnly |
                                                  QFileDialog::DontResolveSymlinks);
    if (! path.isEmpty())
    {
     settings->setValue ("win32_aspell_path", path);
     ed_aspellcheck_path->setText (path);

     delete spellchecker;
     setup_spellcheckers();
     create_spellcheck_menu();
    }

}
#endif
#endif

#endif


void rvln::file_find_obsolete_paths()
{
  QStringList l_bookmarks = qstring_load (fname_bookmarks).split ("\n");

  for (int i = 0; i < l_bookmarks.size(); i++)
      {
       QStringList t = l_bookmarks[i].split (",");
       if (! file_exists (t[0]))
          l_bookmarks[i] = "#" + l_bookmarks[i];
      }

  bookmarks = l_bookmarks.join ("\n").trimmed();

  qstring_save (fname_bookmarks, bookmarks);
  update_bookmarks();
}


void rvln::fn_filter_delete_by_sep (bool mode)
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  QStringList sl = d->textEdit->textCursor().selectedText().split (QChar::ParagraphSeparator);

  QString t = fif_get_text();

  for (int i = 0; i < sl.size(); i++)
      {
       int n = sl[i].indexOf (t);
       if (n != -1)
          {
           QString s = sl[i];
           if (mode) 
               s = s.right (s.size() - n);
           else 
               s = s.left (n);
           sl[i] = s;
          }
      }

  QString x = sl.join ("\n"); 

  d->textEdit->textCursor().insertText (x);
}


void rvln::fn_filter_delete_before_sep()
{
  fn_filter_delete_by_sep (true);
}


void rvln::fn_filter_delete_after_sep()
{
  fn_filter_delete_by_sep (false);
}


void rvln::fman_mk_gallery()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  if (! file_exists (d->file_name))
     return;

  int side = settings->value ("ed_side_size", 110).toInt();
  int thumbs_per_row = settings->value ("ed_thumbs_per_row", 4).toInt();;
  QString link_options = settings->value ("ed_link_options", "target=\"_blank\"").toString();
  if (! link_options.startsWith (" "))
     link_options.prepend (" ");  

  QString dir_out (fman->dir.absolutePath());
    
  QString table ("<table>\n\n");

  Qt::TransformationMode transformMode = Qt::FastTransformation;

  pb_status->show();
  pb_status->setFormat (tr ("%p% completed"));
  pb_status->setTextVisible (true);

  QStringList li = fman->get_sel_fnames();
  int quality = settings->value ("img_quality", "-1").toInt();

  pb_status->setRange (0, li.size() - 1 );

  int x = 0;
  int col = 0;

  for (int i = 0; i < li.size(); i++)
     {
      QString fname = li[i]; 
      if (is_image (fname))
         {
          QFileInfo fi (fname);

          if (fi.baseName().startsWith ("tmb_"))
             continue;

          QImage source (fname);
          if (! source.isNull())
             {
              qApp->processEvents();
              
              QImage dest = image_scale_by (source, true, side, transformMode);
 
              QString dest_fname (dir_out);
              dest_fname.append ("/");
              dest_fname.append ("tmb_");

              dest_fname.append (fi.fileName());
              dest_fname = change_file_ext (dest_fname, "jpg");

              dest.save (dest_fname, 0, quality);

              QFileInfo inf (d->file_name);
              QDir dir (inf.absolutePath());
                         
              QString tmb = get_insert_image (d->file_name, dest_fname, d->markup_mode);
              QString cell = "<a href=\"%source\"" + link_options +">%thumb</a>";
              cell.replace ("%source", dir.relativeFilePath (fname));               
              cell.replace ("%thumb", tmb);               

              if (col == 0)
                 table += "<tr>\n\n";   
                
              table += "<td>\n";

              table += cell;

              table += "</td>\n";

              col++;
              if (col == thumbs_per_row) 
                 {
                  table += "</tr>\n\n";   
                  col = 0;
                 }  
                  
              pb_status->setValue (x++);
             }
          }
         }
  pb_status->hide();
  fman->refresh();

  if (! table.endsWith ("</tr>\n\n"))
     table += "</tr>\n\n";   

  table += "</table>\n";   


  if (d)
     d->textEdit->textCursor().insertText (table);
}


void rvln::indent_by_first_line()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  QStringList sl = d->textEdit->textCursor().selectedText().split (QChar::ParagraphSeparator);
  if (sl.size() == 0)
    return;

  QString x = sl[0];
  QChar c = x[0];
  int pos = 0;

  if (c == ' ' || c == '\t')
     for (int i = 0; i < x.size(); i++)
         if (x[i] != c)
            {
             pos = i;
             break;  
            }    

  QString fill_string;
  fill_string.fill (c, pos);

  for (int i = 0; i < sl.size(); i++)
      {
       QString s = sl[i].trimmed();
       s.prepend (fill_string);
       sl[i] = s;
      }

  QString t = sl.join ("\n"); 

  d->textEdit->textCursor().insertText (t);
}


void rvln::search_in_files_results_dclicked (QListWidgetItem *item)
{
  documents->open_file_triplex (item->text());
  main_tab_widget->setCurrentIndex (idx_tab_edit); 
}


void rvln::search_in_files()
{
  QStringList lresult;
  QString charset = cb_fman_codecs->currentText();
  QString path = fman->dir.path();
  QString text_to_search = fif_get_text();

  CFTypeChecker fc (":/text-data/cm-tf-names", ":/text-data/cm-tf-exts");
  QStringList l = documents->hls.uniqueKeys();
  fc.lexts.append (l);

  log->log (tr ("Getting files list..."));
  qApp->processEvents();

  CFilesList lf;
  lf.get (path);

  log->log (tr ("Searching..."));
  qApp->processEvents();

  pb_status->show();
  pb_status->setRange (0, lf.list.size());
  pb_status->setFormat (tr ("%p% completed"));
  pb_status->setTextVisible (true);


  for (int i = 0; i < lf.list.size(); i++)
      {
       if (i % 100 == 0)
           qApp->processEvents();

       pb_status->setValue (i); 

       QString fileName = lf.list[i];
       if (! fc.check (fileName))
          continue;

       CTio *tio = documents->tio_handler.get_for_fname (fileName);
       tio->charset = charset;

       if (! tio->load (fileName))
           log->log (tr ("cannot open %1 because of: %2")
                         .arg (fileName)
                         .arg (tio->error_string));

       Qt::CaseSensitivity cs = Qt::CaseInsensitive;
       if (menu_find_case->isChecked())
          cs = Qt::CaseSensitive; 

       int index = tio->data.indexOf (text_to_search, 0, cs);
       if (index != -1)
          lresult.append (fileName + "," + charset + "," + QString::number (index)); 
      }


  pb_status->hide();

  CTextListWindow *w = new CTextListWindow (tr ("Search results"), tr ("Files"));

  w->list->addItems (lresult);
     
  connect (w->list, SIGNAL(itemDoubleClicked ( QListWidgetItem *)), 
           this, SLOT(search_in_files_results_dclicked ( QListWidgetItem *)));

  w->resize (width() - 10, (int) height() / 2);
  w->show();
}


void rvln::view_use_profile()
{
  QAction *a = qobject_cast<QAction *>(sender());
  QSettings s (a->data().toString(), QSettings::IniFormat);

  QPoint pos = s.value ("pos", QPoint (1, 200)).toPoint();
  QSize size = s.value ("size", QSize (600, 420)).toSize();

  mainSplitter->restoreState (s.value ("splitterSizes").toByteArray());
  resize (size);
  move (pos);

  settings->setValue ("word_wrap", s.value ("word_wrap", "2").toInt());
  settings->setValue ("show_linenums", s.value ("show_linenums", "0").toInt());
  settings->setValue ("additional_hl", s.value ("additional_hl", "0").toInt()); 
  settings->setValue ("show_margin", s.value ("show_margin", "0").toInt()); 

  cb_wordwrap->setCheckState (Qt::CheckState (settings->value ("word_wrap", "2").toInt()));
  cb_show_linenums->setCheckState (Qt::CheckState (settings->value ("show_linenums", "0").toInt()));
  cb_hl_current_line->setCheckState (Qt::CheckState (settings->value ("additional_hl", "0").toInt()));
  cb_show_margin->setCheckState (Qt::CheckState (settings->value ("show_margin", "0").toInt()));

  settings->setValue ("editor_font_name", s.value ("editor_font_name", "Monospace").toString());
  settings->setValue ("editor_font_size", s.value ("editor_font_size", "12").toInt());

  documents->apply_settings();  
}


void rvln::profile_save_as()
{
  bool ok;
  QString name = QInputDialog::getText (this, tr ("Enter the name"),
                                              tr ("Name:"), QLineEdit::Normal,
                                              tr ("new_profile"), &ok);
  if (! ok || name.isEmpty())
     return;

  QString fname (dir_profiles);
  fname.append ("/").append (name);

  QSettings s (fname, QSettings::IniFormat);

  s.setValue ("word_wrap", settings->value ("word_wrap", "2").toInt());
  s.setValue ("show_linenums", settings->value ("show_linenums", "0").toInt());
  s.setValue ("additional_hl", settings->value ("additional_hl", "0").toInt()); 

  s.setValue ("pos", pos());
  s.setValue ("size", size());
  s.setValue ("splitterSizes", mainSplitter->saveState());

  s.setValue ("editor_font_name", settings->value ("editor_font_name", "Monospace").toString());
  s.setValue ("editor_font_size", settings->value ("editor_font_size", "12").toInt());

  s.sync();
  
  update_profiles();
  shortcuts->load_from_file (shortcuts->fname);
}


void rvln::update_profiles()
{
  menu_view_profiles->clear();
  create_menu_from_dir (this,
                        menu_view_profiles,
                        dir_profiles,
                        SLOT (view_use_profile())
                       );
}


void rvln::fman_items_select_by_regexp (bool mode)
{
  QString ft = fif_get_text();
  if (ft.isEmpty()) 
      return;  
     
  l_fman_find = fman->mymodel->findItems (ft, Qt::MatchRegExp);

  if (l_fman_find.size() < 1)
     return; 
   
  QItemSelectionModel *m = fman->selectionModel();
  for (int i = 0; i < l_fman_find.size(); i++)
      if (mode) 
         m->select (fman->mymodel->indexFromItem (l_fman_find[i]), QItemSelectionModel::Select | QItemSelectionModel::Rows);
      else   
          m->select (fman->mymodel->indexFromItem (l_fman_find[i]), QItemSelectionModel::Deselect | QItemSelectionModel::Rows);
}


void rvln::fman_select_by_regexp()
{
  fman_items_select_by_regexp (true);
}


void rvln::fman_deselect_by_regexp()
{
  fman_items_select_by_regexp (false);
}


void rvln::fman_count_lines_in_selected_files()
{
  QString ft = fif_get_text();
  if (ft.isEmpty()) 
      return;  
          
  QStringList sl = fman->get_sel_fnames();   
  
  if (sl.size() < 1)
     return; 
  
  long int sum = 0;
  
  foreach (QString fname, sl)
          {
           QByteArray f = file_load (fname);
           sum += f.count ('\n');
          }  
          
  log->log (tr ("There are %1 lines at %2 files").arg (sum).arg (sl.size()));        
}


void rvln::set_eol_unix()
{
  CDocument *d = documents->get_current();
  if (!d)
     return;

  d->eol = "\n";
}


void rvln::set_eol_win()
{
  CDocument *d = documents->get_current();
  if (!d)
     return;

  d->eol = "\r\n";
}


void rvln::set_eol_mac()
{
  CDocument *d = documents->get_current();
  if (!d)
     return;

  d->eol = "\r";
}


void rvln::guess_enc()
{
 CCharsetMagic cm;

 QString fn = fman->get_sel_fname();
 QString enc = cm.guess_for_file (fn); 
 cb_fman_codecs->setCurrentIndex (cb_fman_codecs->findText (enc));
}


void rvln::ed_comment()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;
  
  if (! d->highlighter)
     return;
    
  if (d->highlighter->cm_mult.isNull() && d->highlighter->cm_single.isNull())
     return;    
     
  QString t = d->get_selected_text();   
  QString result;   
    
  bool is_multiline = true;

  int sep_pos = t.indexOf (QChar::ParagraphSeparator);
  if (sep_pos == -1 || sep_pos == t.size() - 1)
     is_multiline = false; 
   
  if (is_multiline)
      result = d->highlighter->cm_mult;
  else
      result = d->highlighter->cm_single;
 
 
  if (is_multiline && result.isEmpty())
     {
      QStringList sl = t.split (QChar::ParagraphSeparator);
      for (int i = 0; i < sl.size(); i++)
          {
           QString x = d->highlighter->cm_single;
           sl[i] = x.replace ("%s", sl[i]);
          }

      QString z = sl.join("\n");
      d->textEdit->textCursor().insertText (z);
          
      return;
     }
 
  d->textEdit->textCursor().insertText (result.replace ("%s", t));
}


void rvln::calendar_clicked (const QDate &date)
{
/*  qDebug() << date.toString ("yyyy-MM-dd");
    
//  QDate d = QDate::currentDate();

  qDebug() << "lunar day by moon_phase_trig2: " << moon_phase_trig2 (date.year(), date.month(), date.day());
  qDebug() << "lunar day by moon_phase_trig1: " << moon_phase_trig1 (date.year(), date.month(), date.day());
  qDebug() << "lunar day by moon_phase_simple: " << moon_phase_simple (date.year(), date.month(), date.day());
  qDebug() << "lunar day by moon_phase_conway: " << moon_phase_conway (date.year(), date.month(), date.day());
  qDebug() << "lunar day by moon_phase_leueshkanov : " << moon_phase_leueshkanov (date.year(), date.month(), date.day());
  */

  QString fname = dir_days + "/" + date.toString ("yyyy-MM-dd");

  //bool fresh = false;

  if (file_exists (fname))
    {
     QString s = qstring_load (fname);
     log->log (s);
    }


}


void rvln::calendar_activated (const QDate &date)
{
  QString fname = dir_days + "/" + date.toString ("yyyy-MM-dd");

  bool fresh = false;
  
  if (settings->value ("cal_run_1st", true).toBool())
     {
      if (! file_exists (fname))
         qstring_save (fname, tr ("Enter your daily notes here.\nTo use time-based reminders, specify the time signature in 24-hour format [hh:mm], i.e.:\n[06:00]good morning!\n[20:10]go to theatre"));
     
      settings->setValue ("cal_run_1st", false);
      fresh = true;
     }
  else
  if (! file_exists (fname))
    {
     qstring_save (fname, tr ("Enter your daily notes here."));
     fresh = true;
    }  

  CDocument *d = documents->open_file (fname, "UTF-8");
  
  if (fresh)
     d->textEdit->selectAll();
  
  main_tab_widget->setCurrentIndex (idx_tab_edit); 
}


void rvln::createCalendar()
{
  calendar = new CCalendarWidget (this, dir_days);

  calendar->moon_mode = settings->value ("moon_mode", "0").toBool();
  calendar->northern_hemisphere = settings->value ("northern_hemisphere", "1").toBool();

  calendar->moon_phase_algo = settings->value ("moon_phase_algo", MOON_PHASE_TRIG2).toInt();


  calendar->setGridVisible (true);
  calendar->setVerticalHeaderFormat (QCalendarWidget::NoVerticalHeader);
  
  if (settings->value ("start_on_sunday", "0").toBool())
     calendar->setFirstDayOfWeek (Qt::Sunday);
  else  
      calendar->setFirstDayOfWeek (Qt::Monday);
  
  connect (calendar, SIGNAL(clicked (const QDate &)), this, SLOT(calendar_clicked (const QDate &)));
  connect (calendar, SIGNAL(activated (const QDate &)), this, SLOT(calendar_activated (const QDate &)));
  connect (calendar, SIGNAL(currentPageChanged (int, int)), this, SLOT(calendar_currentPageChanged (int, int)));

  idx_tab_calendar = main_tab_widget->addTab (calendar, tr ("dates"));
}


void rvln::calendar_currentPageChanged (int year, int month)
{
  calendar_update();
} 


void rvln::calendar_update()
{
  if (settings->value ("start_week_on_sunday", "0").toBool())
     calendar->setFirstDayOfWeek (Qt::Sunday);
  else  
      calendar->setFirstDayOfWeek (Qt::Monday);

  int year = calendar->yearShown(); 
  int month = calendar->monthShown();  

  QDate dbase (year, month, 1); 
  
  QTextCharFormat format_past;
  QTextCharFormat format_future;
  QTextCharFormat format_normal;
  
  format_past.setFontStrikeOut (true);
  format_future.setFontUnderline (true);
    
  int days_count = dbase.daysInMonth();

  for (int day = 1; day <= days_count; day++)
      {  
       QDate date (year, month, day);
       QString sdate;
       sdate = sdate.sprintf ("%02d-%02d-%02d", year, month, day);
       QString fname  = dir_days + "/" + sdate;
        
       if (file_exists (fname))
          {
           if (date < QDate::currentDate())
              calendar->setDateTextFormat (date, format_past);
           else
           if (date >= QDate::currentDate())
              calendar->setDateTextFormat (date, format_future);
          } 
        else
            calendar->setDateTextFormat (date, format_normal);
      }
}

  
void rvln::idx_tab_edit_activate()
{
  fileMenu->menuAction()->setVisible (true);
  editMenu->menuAction()->setVisible (true);
  menu_programs->menuAction()->setVisible (true);
  menu_cal->menuAction()->setVisible (false);
  menu_markup->menuAction()->setVisible (true);
  menu_functions->menuAction()->setVisible (true);
  menu_search->menuAction()->setVisible (true);
  menu_nav->menuAction()->setVisible (true);
  menu_fm->menuAction()->setVisible (false);
  menu_view->menuAction()->setVisible (true);
  helpMenu->menuAction()->setVisible (true);
}


void rvln::idx_tab_calendar_activate()
{
  fileMenu->menuAction()->setVisible (true);
  editMenu->menuAction()->setVisible (false);
  menu_cal->menuAction()->setVisible (true);
  menu_programs->menuAction()->setVisible (true);
  menu_markup->menuAction()->setVisible (false);
  menu_functions->menuAction()->setVisible (false);
  menu_search->menuAction()->setVisible (false);
  menu_nav->menuAction()->setVisible (false);
  menu_fm->menuAction()->setVisible (false);
  menu_view->menuAction()->setVisible (true);
  helpMenu->menuAction()->setVisible (true);
}


void rvln::idx_tab_tune_activate()
{
  opt_update_keyb();

  fileMenu->menuAction()->setVisible (true);
  editMenu->menuAction()->setVisible (false);
  menu_programs->menuAction()->setVisible (true);
  menu_markup->menuAction()->setVisible (false);
  menu_functions->menuAction()->setVisible (true);
  menu_search->menuAction()->setVisible (false);
  menu_nav->menuAction()->setVisible (false);
  menu_fm->menuAction()->setVisible (false);
  menu_view->menuAction()->setVisible (true);
  helpMenu->menuAction()->setVisible (true);
  menu_cal->menuAction()->setVisible (false);
}


void rvln::idx_tab_fman_activate()
{
  fileMenu->menuAction()->setVisible (true);
  editMenu->menuAction()->setVisible (false);
  menu_programs->menuAction()->setVisible (true);
  menu_markup->menuAction()->setVisible (false);
  menu_functions->menuAction()->setVisible (true);
  menu_search->menuAction()->setVisible (true);
  menu_nav->menuAction()->setVisible (false);
  menu_fm->menuAction()->setVisible (true);
  menu_view->menuAction()->setVisible (true);
  helpMenu->menuAction()->setVisible (true);
  menu_cal->menuAction()->setVisible (false);
}


void rvln::idx_tab_learn_activate()
{
  fileMenu->menuAction()->setVisible (true);
  editMenu->menuAction()->setVisible (false);
  menu_programs->menuAction()->setVisible (true);
  menu_markup->menuAction()->setVisible (false);
  menu_functions->menuAction()->setVisible (false);
  menu_search->menuAction()->setVisible (true);
  menu_nav->menuAction()->setVisible (false);
  menu_fm->menuAction()->setVisible (false);
  menu_view->menuAction()->setVisible (true);
  helpMenu->menuAction()->setVisible (true);
  menu_cal->menuAction()->setVisible (false);
}


void rvln::cal_add_days()
{
  QDate selected = calendar->selectedDate();
  selected = selected.addDays (fif_get_text().toInt());
  calendar->setSelectedDate (selected);
}


void rvln::cal_add_months()
{
  QDate selected = calendar->selectedDate();
  selected = selected.addMonths (fif_get_text().toInt());
  calendar->setSelectedDate (selected);
}


void rvln::cal_add_years()
{
  QDate selected = calendar->selectedDate();
  selected = selected.addYears (fif_get_text().toInt());
  calendar->setSelectedDate (selected);
}


void rvln::cal_set_date_a()
{
  date1 = calendar->selectedDate();
}


void rvln::cal_set_date_b()
{
  date2 = calendar->selectedDate();
}


void rvln::cal_diff_days()
{
  int days = date2.daysTo (date1);
  if (days < 0)
      days = ~ days;

  log->log (QString::number (days));
}


void rvln::cal_remove()
{
  QString fname = dir_days + "/" + calendar->selectedDate().toString ("yyyy-MM-dd");
  QFile::remove (fname);
  calendar_update();
}


void rvln::rename_selected()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  if (!d->textEdit->textCursor().hasSelection())
     {
      log->log (tr ("Select the file name first!"));
      return;
     }

  QString fname = d->get_filename_at_cursor();

  if (fname.isEmpty())
     return;

  QString newname = fif_get_text();
  if (newname.isEmpty())
     return;

  QFileInfo fi (fname);
  if (! fi.exists() && ! fi.isWritable())
     return;

  QString newfpath (fi.path());
  newfpath.append ("/").append (newname);
  QFile::rename (fname, newfpath);
  update_dyn_menus();
  fman->refresh();

  QDir dir (d->file_name);
  QString new_name = dir.relativeFilePath (newfpath);

  if (new_name.startsWith (".."))
     new_name = new_name.remove (0, 1);

  if (d->textEdit->textCursor().selectedText().startsWith ("./") && ! new_name.startsWith ("./"))
     new_name = "./" + new_name;

  if (! d->textEdit->textCursor().selectedText().startsWith ("./") && new_name.startsWith ("./"))
     new_name = new_name.remove (0, 2);

  if (d->textEdit->textCursor().hasSelection())
     d->set_selected_text (new_name.trimmed());
}


void rvln::update_labels_menu()
{
  menu_labels->clear();

  CDocument *d = documents->get_current();
  if (! d)
     return;

  create_menu_from_list (this, menu_labels, d->labels, SLOT(select_label()));
}


void rvln::update_labels_list()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  d->update_labels();
  update_labels_menu();
}


void rvln::select_label()
{
  QAction *Act = qobject_cast<QAction *>(sender());
  //qDebug() << Act->text();

  CDocument *d = documents->get_current();
  if (! d)
     return;

  QTextCursor cr;

  QString text_to_find = settings->value ("label_start", "[?").toString()
                         + Act->text()
                         + settings->value ("label_end", "?]").toString();

  qDebug() << text_to_find;

  cr = d->textEdit->document()->find (text_to_find);

  if (! cr.isNull())
     d->textEdit->setTextCursor (cr);
}


void rvln::add_user_font()
{
  QString fontfname = QFileDialog::getOpenFileName (this,
                        tr ("Select font"), "",
                        tr("Fonts (*.ttf *.otf)"));


  QStringList sl;

  if (file_exists (fname_userfonts))
     sl = qstring_load (fname_userfonts).split ("\n");

  sl.append (fontfname);
  qstring_save (fname_userfonts, sl.join("\n").trimmed());
  load_userfonts();
}


void rvln::load_userfonts()
{
  font_database->removeAllApplicationFonts();

  userfont_ids.clear();

  if (! file_exists (fname_userfonts))
     return;

  QStringList sl = qstring_load (fname_userfonts).split ("\n");

  for (int i = 0; i < sl.count(); i++)
      {
       userfont_ids.append (font_database->addApplicationFont (sl[i]));
      }
}


void rvln::fn_insert_cpp()
{
  CDocument *d = documents->get_current();
  if (d)
     d->textEdit->textCursor().insertText (qstring_load (":/text-data/tpl_cpp.cpp"));
}


void rvln::fn_insert_c()
{
  CDocument *d = documents->get_current();
  if (d)
     d->textEdit->textCursor().insertText (qstring_load (":/text-data/tpl_c.c"));
}


inline bool CFSizeFNameLessThan (CFSizeFName *v1, CFSizeFName *v2)
{
  return v1->size > v2->size;
}


void rvln::mrkup_document_weight()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  QString result;
  QStringList l = html_get_by_patt (d->textEdit->toPlainText(), "src=\"");

  QFileInfo f (d->file_name);
  QUrl baseUrl (d->file_name);

//  result += tr ("%1 %2 kbytes<br>").arg (d->file_name).arg (QString::number (f.size() / 1024));

  QList <CFSizeFName*> lst;
  lst.append (new CFSizeFName (f.size(), d->file_name));

  int size_total = 0;
  int files_total = 1;

  foreach (QString url, l)
          {
           QUrl relativeUrl (url);
           QString resolved = baseUrl.resolved (relativeUrl).toString();
           QFileInfo info (resolved);
           if (! info.exists())
              lst.append (new CFSizeFName (info.size(), tr ("%1 is not found<br>").arg (resolved)));
           else
               {
                lst.append (new CFSizeFName (info.size(), resolved));
                size_total += info.size();
                ++files_total;
               }
           }

  qSort (lst.begin(), lst.end(), CFSizeFNameLessThan);


  for (int i = 0; i < lst.size(); i++)
     {
      result += tr ("%1 kbytes %2 <br>").arg (QString::number (lst[i]->size / 1024)).arg (lst[i]->fname);
      delete lst[i];
     }


  result.prepend (tr ("Total size = %1 kbytes in %2 files<br>").arg (QString::number (size_total / 1024))
                                                               .arg (QString::number (files_total)));

  log->log (result);
}


void rvln::fman_unpack_zip()
{
  QString fn = fman->get_sel_fname();
  if (fn.isNull() || fn.isEmpty())
     return;

  CZipper z;

  z.unzip (fman->get_sel_fname(), fman->dir.path());
}


void rvln::fman_zip_info()
{
  QString fn = fman->get_sel_fname();
  if (fn.isNull() || fn.isEmpty())
     return;

  CZipper z;

  QString s = z.unzip_info (fman->get_sel_fname());
  log->log (s);
}


void rvln::cmb_icon_sizes_currentIndexChanged (const QString &text)
{
  settings->setValue ("icon_size", text);

  setIconSize (QSize (text.toInt(), text.toInt()));
  tb_fman_dir->setIconSize (QSize (text.toInt(), text.toInt()));
}


void rvln::cmb_ui_tabs_currentIndexChanged (int i)
{
  main_tab_widget->setTabPosition (int_to_tabpos (i));

  settings->setValue ("ui_tabs_align", i);

  qDebug() << "cmb_ui_tabs_currentIndexChanged ";
}


void rvln::cmb_docs_tabs_currentIndexChanged (int i)
{
  tab_widget->setTabPosition (int_to_tabpos (i));
  
  settings->setValue ("docs_tabs_align", i);

  qDebug() << "cmb_docs_tabs_currentIndexChanged " << i;
}


void rvln::cal_set_to_current()
{
  calendar->showToday();
}


void rvln::clipboard_dataChanged()
{
  if (! capture_to_storage_file)
     return;

  CDocument *ddest = documents->get_document_by_fname (fname_storage_file);
  if (ddest)
     {
      QString t = QApplication::clipboard()->text();
      ddest->textEdit->textCursor().insertText (t);
      ddest->textEdit->textCursor().insertText ("\n");
     }
}


void rvln::fn_remove_by_regexp()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  QString t = d->textEdit->textCursor().selectedText();
  t.remove (QRegExp (fif_get_text()));
  d->textEdit->textCursor().insertText (t);
}


void rvln::capture_clipboard_to_storage_file()
{
  capture_to_storage_file = ! capture_to_storage_file;
}


void rvln::set_as_storage_file()
{
  CDocument *d = documents->get_current();
  if (! d)
     return;

  fname_storage_file = d->file_name;
}


void rvln::copy_to_storage_file()
{
  CDocument *dsource = documents->get_current();
  if (! dsource)
     return;

  CDocument *ddest = documents->get_document_by_fname (fname_storage_file);
  if (ddest)
     {
      QString t = dsource->textEdit->textCursor().selectedText();
      ddest->textEdit->textCursor().insertText (t);
      ddest->textEdit->textCursor().insertText ("\n");
     }
}


void rvln::cal_moon_mode()
{
  calendar->moon_mode = ! calendar->moon_mode;
  calendar->do_update();

  settings->setValue ("moon_mode", calendar->moon_mode);
}


void rvln::create_moon_phase_algos()
{
  moon_phase_algos.insert (MOON_PHASE_TRIG2, tr ("Trigonometric 2"));
  moon_phase_algos.insert (MOON_PHASE_TRIG1, tr ("Trigonometric 1"));
  moon_phase_algos.insert (MOON_PHASE_CONWAY, tr ("Conway"));
//  moon_phase_algos.insert (MOON_PHASE_SIMPLE, tr ("Simple1"));
  moon_phase_algos.insert (MOON_PHASE_LEUESHKANOV, tr ("Leueshkanov"));
}


void rvln::cal_gen_mooncal()
{
  int jdate1 = date1.toJulianDay();
  int jdate2 = date2.toJulianDay();

  QString s;

  QString date_format = settings->value("date_format", "dd/MM/yyyy").toString();

  for (int d = jdate1; d <= jdate2; d++)
     {
      QDate date = QDate::fromJulianDay (d);
      int moon_day = moon_phase_trig2 (date.year(), date.month(), date.day());

      s += date.toString (date_format);
      s += " = ";
      s += QString::number (moon_day);
      s += "\n";
     }

  CDocument *nd = documents->create_new();
  nd->textEdit->textCursor().insertText (s);
  main_tab_widget->setCurrentIndex (idx_tab_edit);
}


void rvln::leaving_tune()
{
  qDebug() << "Leaving Tune";


  settings->setValue ("date_format", ed_date_format->text());
  settings->setValue ("time_format", ed_time_format->text());

  settings->setValue ("override_locale_val", ed_locale_override->text());

  settings->setValue ("word_wrap", cb_wordwrap->checkState());

  settings->setValue ("additional_hl", cb_hl_current_line->checkState());

  settings->setValue ("session_restore", cb_session_restore->checkState());



  settings->setValue ("show_linenums", cb_show_linenums->checkState());
  settings->setValue ("use_hl_wrap", cb_use_hl_wrap->checkState());
  settings->setValue ("hl_brackets", cb_hl_brackets->checkState());
  settings->setValue ("auto_indent", cb_auto_indent->checkState());


  settings->setValue ("spaces_instead_of_tabs", cb_spaces_instead_of_tabs->checkState());
  settings->setValue ("cursor_xy_visible", cb_cursor_xy_visible->checkState());
  settings->setValue ("tab_sp_width", spb_tab_sp_width->value());

  settings->setValue ("center_on_scroll", cb_center_on_cursor->checkState());
  settings->setValue ("show_margin", cb_show_margin->checkState());
  settings->setValue ("margin_pos", spb_margin_pos->value());
  settings->setValue ("b_preview", cb_auto_img_preview->checkState());

  settings->setValue ("override_locale", cb_override_locale->checkState());
  settings->setValue ("use_trad_dialogs", cb_use_trad_dialogs->checkState());
  settings->setValue ("start_week_on_sunday", cb_start_on_sunday->checkState());

  settings->setValue ("northern_hemisphere", cb_northern_hemisphere->checkState());

  calendar->northern_hemisphere = bool (cb_northern_hemisphere->checkState());

  int i = moon_phase_algos.key (cmb_moon_phase_algos->currentText());
  settings->setValue ("moon_phase_algo", i);
  calendar->moon_phase_algo = i;

  settings->setValue ("zip_charset_in", cmb_zip_charset_in->currentText());
  settings->setValue ("zip_charset_out", cmb_zip_charset_out->currentText());

  settings->setValue ("label_end", ed_label_end->text());
  settings->setValue ("label_start", ed_label_start->text());

  settings->setValue ("output_image_fmt", cmb_output_image_fmt->currentText());

  settings->setValue ("img_filter", cb_output_image_flt->checkState());


  settings->setValue("fuzzy_q", spb_fuzzy_q->value());


  settings->setValue("img_quality", spb_img_quality->value());
  settings->setValue ("img_post_proc", cb_zip_after_scale->checkState());

  settings->setValue ("ed_side_size", ed_side_size->text());

  settings->setValue ("ed_link_options", ed_link_options->text());

  settings->setValue ("ed_cols_per_row", ed_cols_per_row->text());

  calendar->do_update();
  documents->apply_settings();
}



QAction* rvln::add_to_menu (QMenu *menu,
                            const QString &caption,
                            const char *method,
                            const QString &shortkt,
                            const QString &iconpath
                           )
{
  QAction *act = new QAction (caption, this);

  if (! shortkt.isEmpty())
     act->setShortcut (shortkt);

  if (! iconpath.isEmpty())
     act->setIcon (QIcon (iconpath));

  connect (act, SIGNAL(triggered()), this, method);
  menu->addAction (act);
  return act;
}




void rvln::cmb_ui_langs_currentIndexChanged (const QString &text)
{
 qDebug() << "void rvln::cmb_ui_langs_currentIndexChanged (const QString &text)";

//  settings->setValue ("ui_lang", text);
  if (text == "en")
     {
      qApp->removeTranslator (&myappTranslator);
      qApp->removeTranslator (&qtTranslator);
     }
  else
      {

      qApp->removeTranslator (&myappTranslator);
      qApp->removeTranslator (&qtTranslator);

      QString ts = text;
      if (ts.length() != 2)
         ts = "en";

      qtTranslator.load (QString ("qt_%1").arg (ts),
                         QLibraryInfo::location (QLibraryInfo::TranslationsPath));
      qApp->installTranslator (&qtTranslator);

      myappTranslator.load (":/translations/tea_" + ts);
      qApp->installTranslator (&myappTranslator);
      }
}


void rvln::tab_options_pageChanged (int index)
{
  if (index == idx_tab_keyboard)
     {
    //  opt_update_keyb();
     }
}


void rvln::search_fuzzy_mode()
{
  menu_find_whole_words->setChecked (false);
  menu_find_regexp->setChecked (false);;
}


void rvln::search_regexp_mode()
{
  menu_find_fuzzy->setChecked (false);
}


void rvln::search_whole_words_mode()
{
  menu_find_fuzzy->setChecked (false);
}



void rvln::test()
{
/*  CDocument *d = documents->get_current();
  if (! d)
     return;

    QTime time_start;
    time_start.start();

  pb_status->show();
  pb_status->setRange (0, d->textEdit->toPlainText().size() - 1);
  pb_status->setFormat (tr ("%p% completed"));
  pb_status->setTextVisible (true);

   int i = 0;

   QString fiftxt = fif_get_text();

   d->text_to_search = fiftxt;

   QTextCursor cr = d->textEdit->document()->find (d->text_to_search, 0, get_search_options());
   cr.movePosition (QTextCursor::Right, QTextCursor::KeepAnchor, d->text_to_search.size());
8779797

    do
      {

          {
           f = cr.blockCharFormat();
           f.setUnderlineStyle (QTextCharFormat::SpellCheckUnderline);
           f.setUnderlineColor (QColor (hash_get_val (documents->palette, "error", "red")));
           cr.mergeCharFormat (f);
          }

        pb_status->setValue (i++);
       }
    while (cr.movePosition (QTextCursor::NextWord));

    cr.setPosition (pos);
    d->textEdit->setTextCursor (cr);
    d->textEdit->document()->setModified (false);

    pb_status->hide();

    log->log (tr("elapsed milliseconds: %1").arg (time_start.elapsed()));
//  CDocument *d = documents->get_current();
//  if (! d)
  //   return;


//  QLabel *l_font_demo = new QLabel (d->get_all_text());
//  l_font_demo->setFont (QFont (sl[0]));
//  l_font_demo->show();


*/

/*  CDocument *d = documents->get_current();
  if (! d)
     return;

  QStringList l = text_get_bookmarks (d->textEdit->toPlainText());
  qstring_list_print (l);
*/


/*
  QRegExp e ("([0-1][0-9]|2[0-3]):([0-5][0-9])");
  QString s ("aaa 22:13 bbb");
  qDebug() << s.indexOf (e);
  */
/*
  QString source = "aaa 22:13 bbb 44:56";
  QRegExp e ("([0-1][0-9]|2[0-3]):([0-5][0-9])");
  
  QString sl_parsed;
 
  int i = 0;
  int x = 0;
  
  while (x != -1)
       {
        qDebug() << "i=" << i;
        x = source.indexOf (e, i);
        qDebug() << "x=" << x; 
        if (x != -1)   
           i += (x + 4);
        //else   
          //  i += 4;       
       }
*/  

//    WId wid = QxtWindowSystem::findWindow("Mail - Kontact");
  //  QPixmap screenshot = QPixmap::grabWindow(wid);
  
//full desktop:
//originalPixmap = QPixmap::grabWindow(QApplication::desktop()->winId());
  
}

