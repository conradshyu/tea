/***************************************************************************
 *   2007-2013 by Peter Semiletov                                          *
 *   tea@list.ru                                                           *
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

/* some code by
Copyright (C) 2006-2008 Trolltech ASA. All rights reserved.
*/

/*
code from qPEditor:
*   Copyright (C) 2007 by Michael Protasov
*   mik-protasov@anyqsoft.com */

/*
Diego Iastrubni <elcuco@kde.org> //some GPL'ed code from new-editor-diego-3,
found on qtcentre forum
*/

/*
code from qwriter:
*   Copyright (C) 2009 by Gancov Kostya                                   *
*   kossne@mail.ru                                                        *
*/



#include "document.h"
#include "utils.h"
#include "gui_utils.h"

#include <QSettings>
#include <QLabel>
#include <QPainter>
#include <QDebug>
#include <QUrl>
#include <QDir>
#include <QXmlStreamReader>
#include <QMimeData>


QSettings *settings;


void CDocument::update_status()
{
  if (! cursor_xy_visible)
     {
      holder->l_status_bar->setText (charset);
      return;
     }

  QTextBlock block = textEdit->document()->begin();
  int cursor_offset = textEdit->textCursor().position();
  
  int off = 0;
  int y = 1;

  while (cursor_offset >= (off + block.length()))
        {
         off += block.length();
         block = block.next();
         y++;
        }

  int x = cursor_offset - off + 1;

  holder->l_status_bar->setText (QString ("%1:%2 %3").arg (
                                 QString::number (y)).arg (
                                 QString::number (x)).arg (charset));
}


void CDocument::update_title (bool fullname)
{
  QMainWindow *w = qobject_cast <QMainWindow *> (holder->parent_wnd);

  if (fullname)
     w->setWindowTitle (file_name);
  else
     w->setWindowTitle (QFileInfo (file_name).fileName());
}


void CDocument::reload (const QString &enc)
{
  if (file_exists (file_name))
      open_file (file_name, enc);
}


bool CDocument::open_file (const QString &fileName, const QString &codec)
{
  CTio *tio = holder->tio_handler.get_for_fname (fileName);

  if (fileName.contains (holder->dir_config))
      tio->charset = "UTF-8";
  else
      tio->charset = codec;

  if (! tio->load (fileName))
     {
      holder->log->log (tr ("cannot open %1 because of: %2")
                           .arg (fileName)
                           .arg (tio->error_string));
      return false;
     }

  charset = tio->charset;
  eol = tio->eol;

  textEdit->setPlainText (tio->data);

  file_name = fileName;

  set_tab_caption (QFileInfo (file_name).fileName());
  set_hl();
  set_markup_mode();
  textEdit->document()->setModified (false);

  holder->log->log (tr ("%1 is open").arg (file_name));

  QMutableListIterator <QString> i (holder->recent_files);

  while (i.hasNext())
        {
         QStringList lt = i.next().split(",");
         if (lt.size() > 0 && lt[0] == file_name) 
            i.remove();
        }

  return true;
}


bool CDocument::save_with_name (const QString &fileName, const QString &codec)
{
  CTio *tio = holder->tio_handler.get_for_fname (fileName);

  if (fileName.contains (holder->dir_config))
      tio->charset = "UTF-8";
  else
      tio->charset = codec;

  tio->data = textEdit->toPlainText();
  
   if (eol != "\n")
      tio->data.replace ("\n", eol);

  if (! tio->save (fileName))
     {
      holder->log->log (tr ("cannot save %1 because of: %2")
                        .arg (fileName)
                        .arg (tio->error_string));
      return false;
     }

  charset = tio->charset;
  file_name = fileName;

  set_tab_caption (QFileInfo (file_name).fileName());

  holder->log->log (tr ("%1 is saved").arg (file_name));

  update_title();
  update_status();

  textEdit->document()->setModified (false);

  return true;
}


CDocument::CDocument (QObject *parent): QObject (parent)
{
  QString fname = tr ("new[%1]").arg (QTime::currentTime().toString ("hh-mm-ss"));
  
  fnameswoexts.insert ("configure", "sh");
  fnameswoexts.insert ("install-sh", "sh");
  
  markup_mode = "HTML";
  file_name = fname;
  cursor_xy_visible = true;
  charset = "UTF-8";
  text_to_search = "";
  position = 0;
  
#if defined(Q_OS_WIN) || defined(Q_OS_OS2)

  eol = "\r\n";

  qDebug() << "defined(Q_OS_WIN) || defined(Q_OS_OS2)";

#elif defined(Q_OS_LINUX)

  eol = "\n";

  qDebug() << "Q_OS_LINUX";


#elif defined(Q_OS_MAC)

  eol = "\r";

  qDebug() << "Q_OS_MAC";


#endif

  qDebug() << "CDocument ()";

  if (eol == "\n")
    qDebug() << "LF";
  if (eol == "\r\n")
    qDebug() << "CRLF";
  if (eol == "\r")
    qDebug() << "CR";
}


CDocument::~CDocument()
{
  if (file_name.startsWith (holder->dir_config) && textEdit->document()->isModified()) 
     save_with_name_plain (file_name);
  else
  if (textEdit->document()->isModified() && file_exists (file_name))
     {
      if (QMessageBox::warning (0, "TEA",
                                tr ("%1 has been modified.\n"
                                "Do you want to save your changes?")
                                .arg (file_name),
                                QMessageBox::Ok | QMessageBox::Default,
                                QMessageBox::Cancel | QMessageBox::Escape) == QMessageBox::Ok)
         save_with_name (file_name, charset);
     }

  if (! file_name.startsWith (holder->dir_config))
     {
      holder->add_to_recent (this);
      holder->update_recent_menu();
     }

  if (file_name.startsWith (holder->todo.dir_days))
      holder->todo.load_dayfile();

  int i = holder->tab_widget->indexOf (tab_page);
  if (i != -1)
     holder->tab_widget->removeTab (i);
}


void CDocument::create_new()
{
  textEdit = new CTEAEdit;

  textEdit->doc = this;

  textEdit->currentLineColor = hash_get_val (holder->palette, 
                                             "cur_line_color", 
                                             "#EEF6FF");

  highlighter = NULL;

  int tab_index = holder->tab_widget->addTab (textEdit, file_name);
  tab_page = holder->tab_widget->widget (tab_index);
   
  textEdit->setFocus (Qt::OtherFocusReason);
}


int CDocument::get_tab_idx()
{
  return holder->tab_widget->indexOf (tab_page);
}


void CDocument::set_tab_caption (const QString &fileName)
{
  holder->tab_widget->setTabText (get_tab_idx(), fileName);
}


void document_holder::reload_recent_list (void)
{
  if (! file_exists (recent_list_fname))
     return;

  recent_files = qstring_load (recent_list_fname).split ("\n");
}


document_holder::~document_holder()
{
  while (! list.isEmpty())
        delete list.takeFirst();

  qstring_save (recent_list_fname, recent_files.join ("\n"));
}


CDocument* document_holder::create_new()
{
  CDocument *doc = new CDocument;

  doc->holder = this;
  doc->markup_mode = markup_mode;
  list.append (doc);

  doc->create_new();

  tab_widget->setCurrentIndex (tab_widget->indexOf (doc->tab_page));
  apply_settings_single (doc);

  doc->update_title();
  doc->update_status();
  
  return doc;
}


CDocument* document_holder::get_document_by_fname (const QString &fileName)
{
  if (fileName.isNull() || fileName.isEmpty())
     return NULL;

  foreach (CDocument *d, list)
          if (d->file_name == fileName)
              return d;

  return NULL;
}


CDocument* document_holder::open_file (const QString &fileName, const QString &codec)
{
  if (! file_exists (fileName))
     return NULL;
  
  if (is_image (fileName))
     {
      CDocument *td = get_current();
      if (td)
         {
          td->insert_image (fileName);
          td->textEdit->setFocus (Qt::OtherFocusReason);
         }
      return td;
     }   
   
  CDocument *d = get_document_by_fname (fileName);
  if (d)
     {
      tab_widget->setCurrentIndex (tab_widget->indexOf (d->tab_page));
      return d;
     }

  CDocument *doc = create_new();
  doc->open_file (fileName, codec);
  doc->update_status();
  doc->update_title();

  tab_widget->setCurrentIndex (tab_widget->indexOf (doc->tab_page));
  
  return doc;
}


void document_holder::close_by_idx (int i)
{
  if (i < 0)
     return;

  CDocument *d = list[i];
  list.removeAt (i);
  delete d;
}


void document_holder::close_current()
{
  close_by_idx (tab_widget->currentIndex());
}


CDocument* document_holder::get_current()
{
  int i = tab_widget->currentIndex();
  if (i < 0)
     return NULL;

  return list[i];
}


bool CDocument::save_with_name_plain (const QString &fileName)
{
  QFile file (fileName);
  if (! file.open (QFile::WriteOnly | QFile::Text))
      return false;

  QTextStream out (&file);
  out.setCodec (charset.toUtf8().data());
  out << textEdit->toPlainText();

  return true;
}


QString CDocument::get_filename_at_cursor()
{
  if (textEdit->textCursor().hasSelection())
    {
     QFileInfo nf (file_name);
     QDir cd (nf.absolutePath());
     return cd.cleanPath (cd.absoluteFilePath(textEdit->textCursor().selectedText()));
    }

  QString s = textEdit->toPlainText();

  QString x;

  int pos = textEdit->textCursor().position();

  int end = s.indexOf ("\"", pos);
  if (end == -1)
     return x;

  int start = s.lastIndexOf ("\"", pos);
  if (start == -1)
     return x;

  x = s.mid (start + 1, end - (start + 1));

  if (x.startsWith("#"))
     return x;

  QFileInfo inf (file_name);
  QDir cur_dir (inf.absolutePath());

  return cur_dir.cleanPath (cur_dir.absoluteFilePath(x));
}


CSyntaxHighlighter::CSyntaxHighlighter (QTextDocument *parent, CDocument *doc):
                                        QSyntaxHighlighter (parent)
{
  document = doc;
  casecare = true;
  wrap = true;
}


void CDocument::set_hl (bool mode_auto, const QString &theext)
{
  QString ext;
  
  if (mode_auto)
     ext = file_get_ext (file_name);
  else
      ext = theext;

  if (ext.isNull() || ext.isEmpty())
    {
     QFileInfo fi (file_name);
     ext = fnameswoexts[fi.fileName()];
    }

  if (ext.isNull() || ext.isEmpty())
     return;

  QString fname = holder->hls.value (ext);

  if (fname.isNull() || fname.isEmpty())
     return;

  if (highlighter)
     delete highlighter;
  
  highlighter = new CSyntaxHighlighter (textEdit->document(), this);
  highlighter->load_from_xml (fname);
      
  if (textEdit->use_hl_wrap)
     {
      if (highlighter->wrap)
         textEdit->setLineWrapMode (QPlainTextEdit::WidgetWidth);
       else
          textEdit->setLineWrapMode (QPlainTextEdit::NoWrap);
     }
  
//  qDebug() << "hl file is: " << fname;
}


void CDocument::goto_pos (int pos)
{
  QTextCursor cr = textEdit->textCursor();
  cr.setPosition (pos);
  textEdit->setTextCursor (cr);
}


void document_holder::apply_settings_single (CDocument *d)
{
  QFont f;
  
  f.fromString (settings->value ("editor_font_name", "Monospace").toString());
  f.setPointSize (settings->value ("editor_font_size", "12").toInt());
  d->textEdit->setFont (f);
  d->textEdit->lineNumberArea->setFont (f);

  d->textEdit->setCenterOnScroll (settings->value ("center_on_scroll", true).toBool());
  
  d->textEdit->use_hl_wrap = settings->value ("use_hl_wrap", true).toBool();
  
  if (settings->value ("word_wrap", "2").toInt() == 2)
     d->textEdit->setLineWrapMode (QPlainTextEdit::WidgetWidth);
  else
      d->textEdit->setLineWrapMode (QPlainTextEdit::NoWrap);

  d->textEdit->tab_sp_width = settings->value ("tab_sp_width", 8).toInt();
  d->textEdit->spaces_instead_of_tabs = settings->value ("spaces_instead_of_tabs", true).toBool();

  d->textEdit->setTabStopWidth (d->textEdit->tab_sp_width * d->textEdit->brace_width);
  
  d->textEdit->setup_brace_width();

  d->textEdit->set_show_linenums (settings->value ("show_linenums", false).toBool());
  d->textEdit->set_show_margin (settings->value ("show_margin", false).toBool());
  d->textEdit->set_margin_pos (settings->value ("margin_pos", 72).toInt());

  d->textEdit->highlightCurrentLine = settings->value ("additional_hl", false).toBool();
  d->textEdit->hl_brackets = settings->value ("hl_brackets", false).toBool();

  d->textEdit->currentLineColor = QColor (hash_get_val (palette, "cur_line_color", "#EEF6FF"));
  d->textEdit->brackets_color = QColor (hash_get_val (palette, "brackets", "yellow"));
  
  d->cursor_xy_visible = settings->value ("cursor_xy_visible", "2").toBool();
  d->textEdit->auto_indent = settings->value ("auto_indent", false).toBool();

  QString text_color = hash_get_val (palette, "text", "black");
  d->textEdit->text_color = QColor (text_color);

  QString back_color = hash_get_val (palette, "background", "white");
  QString sel_back_color = hash_get_val (palette, "sel-background", "black");
  QString sel_text_color = hash_get_val (palette, "sel-text", "white");

  d->textEdit->margin_color = QColor (hash_get_val (palette, "margin_color", text_color));
  d->textEdit->linenums_bg = QColor (hash_get_val (palette, "linenums_bg", back_color));

  QString sheet = QString ("QPlainTextEdit { color: %1 ; background-color: %2 ; selection-color: %3; selection-background-color: %4;}").arg (
                           text_color).arg (
                           back_color).arg (
                           sel_text_color).arg (
                           sel_back_color);

  d->textEdit->setStyleSheet (sheet);
  d->textEdit->repaint();
  d->set_hl();
}


void document_holder::apply_settings()
{
  foreach (CDocument *d, list)
          apply_settings_single (d);
}


void document_holder::add_to_recent (CDocument *d)
{
  if (! file_exists (d->file_name))
     return;

  QString s (d->file_name);
  s += ",";
  s += d->charset;
  s += ",";
  s += QString ("%1").arg (d->textEdit->textCursor().position());

  recent_files.prepend (s);
  if (recent_files.size() > 13)
     recent_files.removeLast();
}


void document_holder::update_recent_menu()
{
  recent_menu->clear();
  create_menu_from_list (this, recent_menu, recent_files, SLOT(open_recent()));
}


void document_holder::open_recent()
{
  QAction *act = qobject_cast<QAction *>(sender());

  int i = recent_files.indexOf (act->text());
  if (i == -1)
     return;

  open_file_triplex (recent_files[i]);
  update_recent_menu();
}


CDocument* document_holder::open_file_triplex (const QString &triplex)
{
  QStringList sl = triplex.split (",");
  if (sl.size() < 3)
     return NULL;

  CDocument *d = open_file (sl[0], sl[1]);
  if (d)
     d->goto_pos (sl[2].toInt());

  return d;
}


QString CDocument::get_triplex()
{
  QString s (file_name);
  s += ",";
  s += charset;
  s += ",";
  s += QString::number (textEdit->textCursor().position()); 
  return s;
}


void CDocument::set_markup_mode()
{
  markup_mode = holder->markup_mode;

  QString e = file_get_ext (file_name);

  if (e == "htm" || e == "html")
    markup_mode = "HTML";
  else
  if (e == "xhtml")
    markup_mode = "XHTML";
  else
  if (e == "xml")
    markup_mode = "Docbook";
  else
  if (e == "tex")
    markup_mode = "LaTeX";
  else
  if (e == "lout")
    markup_mode = "Lout";
  else
  if (e == "dokuwiki")
    markup_mode = "DokuWiki";
  else
  if (e == "mediawiki")
    markup_mode = "MediaWiki";
}


void CTEAEdit::cb_cursorPositionChanged()
{
  viewport()->update();
  
  if (doc)
     doc->update_status();
     
  if (hl_brackets)   
     braceHighlight();   
}


void CTEAEdit::setCurrentLineColor (const QColor &newColor)
{
  currentLineColor = newColor;
  emit repaint();
}


void CTEAEdit::set_brackets_color (const QColor &newColor)
{
  brackets_color = newColor;
  emit repaint();
}


void CTEAEdit::set_hl_cur_line (bool enable)
{
  highlightCurrentLine = enable;
  emit repaint();
}


void CTEAEdit::set_hl_brackets (bool enable)
{
  hl_brackets = enable;
  emit repaint();
}


void CTEAEdit::set_show_margin (bool enable)
{
  draw_margin = enable;
  emit repaint();
}


void CTEAEdit::set_show_linenums (bool enable)
{
  draw_linenums = enable;
  updateLineNumberAreaWidth (0);
  emit repaint();
}


void CTEAEdit::set_margin_pos (int mp)
{
  margin_pos = mp;
  margin_x = brace_width * margin_pos;
  emit repaint();
}


void CTEAEdit::paintEvent (QPaintEvent *event)
{
  if (draw_margin || highlightCurrentLine)
     {
      QPainter painter (viewport());

      if (highlightCurrentLine)
         {
          QRect r = cursorRect();
          r.setX (0);
          r.setWidth (viewport()->width());
          painter.fillRect (r, QBrush (currentLineColor));
         }

      if (draw_margin)
         {
          QPen pen = painter.pen();
          pen.setColor (margin_color);
          painter.setPen (pen);
          painter.drawLine (margin_x, 0, margin_x, viewport()->height());
         }
     }

  QPlainTextEdit::paintEvent (event);
}


void CTEAEdit::setup_brace_width()
{
  QFontMetrics *fm = new QFontMetrics (font());
  brace_width = fm->averageCharWidth();
}


CTEAEdit::CTEAEdit (QWidget *parent): QPlainTextEdit (parent)
{
  highlightCurrentLine = false;
  setup_brace_width();
  
  lineNumberArea = new LineNumberArea (this);
  connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(updateLineNumberAreaWidth(int)));
  connect(this, SIGNAL(updateRequest(const QRect &, int)), this, SLOT(updateLineNumberArea(const QRect &, int)));
  updateLineNumberAreaWidth (0);

  margin_pos = 72;
  margin_x = brace_width * margin_pos;
  draw_margin = false;
  hl_brackets = false;
  auto_indent = false;
  use_hl_wrap = true;
  tab_sp_width = 8;
  spaces_instead_of_tabs = true;
  connect (this, SIGNAL(cursorPositionChanged()), this, SLOT(cb_cursorPositionChanged()));
}


QStringList CDocument::get_words()
{
  QStringList result;

  QTextCursor cr = textEdit->textCursor();

  QString text = textEdit->toPlainText();

  cr.setPosition (0);
  cr.movePosition (QTextCursor::Start, QTextCursor::MoveAnchor);

  do
    {
     QChar c = text[cr.position()];
     if (char_is_shit (c))
         while (char_is_shit (c))
               {
                cr.movePosition (QTextCursor::NextCharacter);
                c = text[cr.position()];
               }

     cr.movePosition (QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
     c = text[cr.position()];

     QString stext = cr.selectedText();

     if (! stext.isNull() && stext.endsWith ("\""))
        {
         cr.movePosition (QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
         stext = cr.selectedText();
        }  
           
     if (! stext.isNull() || ! stext.isEmpty())
        result.append (stext);     
    }
  while (cr.movePosition (QTextCursor::NextWord));

  return result;
}


void CSyntaxHighlighter::highlightBlock (const QString &text)
{
  foreach (HighlightingRule rule, highlightingRules)
          {
           QRegExp expression (rule.pattern);

           int index;

           if (! casecare)
              index = text.toLower().indexOf (expression);
           else
               index = text.indexOf (expression);

           while (index >= 0)
                 {
                  int length = expression.matchedLength();
                  setFormat (index, length, rule.format);

                  if (! casecare)
                      index = text.toLower().indexOf (expression, index + length);
                  else
                      index = text.indexOf (expression, index + length);
                 }
           }

  setCurrentBlockState (0);

  int startIndex = 0;

  if (commentStartExpression.isEmpty() || commentEndExpression.isEmpty())
     return;

  if (previousBlockState() != 1)
     {
      if (! casecare)
         startIndex = text.toLower().indexOf (commentStartExpression);
      else
          startIndex = text.indexOf (commentStartExpression);
     }

  while (startIndex >= 0)
        {
         int endIndex;
         if (! casecare)
            endIndex = text.toLower().indexOf (commentEndExpression, startIndex);
         else
             endIndex = text.indexOf (commentEndExpression, startIndex);

         int commentLength;
         if (endIndex == -1)
            {
             setCurrentBlockState (1);
             commentLength = text.length() - startIndex;
            }
         else
             commentLength = endIndex - startIndex + commentEndExpression.matchedLength();

         setFormat (startIndex, commentLength, multiLineCommentFormat);

         if (! casecare)
            startIndex = text.toLower().indexOf (commentStartExpression, startIndex + commentLength);
         else
              startIndex = text.indexOf (commentStartExpression, startIndex + commentLength);
        }
}


QTextCharFormat tformat_from_style (const QString &fontstyle, const QString &color)
{
  QTextCharFormat tformat;
  tformat.setForeground (QBrush (QColor (color)));

  if (fontstyle.isNull() || fontstyle.isEmpty())
     return tformat; 

  if (fontstyle.contains ("bold"))
      tformat.setFontWeight (QFont::Bold);

  if (fontstyle.contains ("italic"))
      tformat.setFontItalic (true);

  if (fontstyle.contains ("underline"))
      tformat.setFontUnderline (true);

  if (fontstyle.contains ("strikeout"))
      tformat.setFontStrikeOut (true);

  return tformat;
}


void CSyntaxHighlighter::load_from_xml (const QString &fname)
{
  exts = "default";
  langs = "default";

  QString temp = qstring_load (fname);
  QXmlStreamReader xml (temp);

   while (! xml.atEnd())
        {
         xml.readNext();

         QString tag_name = xml.name().toString().toLower();


         if (xml.isStartElement())
            if (tag_name == "document")
                {
                 exts = xml.attributes().value ("exts").toString();
                 langs = xml.attributes().value ("langs").toString();
                }


         if (xml.isStartElement())
            {

             if (tag_name == "item")
                {
                 QString attr_type = xml.attributes().value ("type").toString();
                 QString attr_name = xml.attributes().value ("name").toString();

                 if (attr_name == "options")
                    {
                     QString s_wrap = xml.attributes().value ("wrap").toString();
                     if (! s_wrap.isNull() || ! s_wrap.isEmpty())
                        if (s_wrap == "0" || s_wrap == "false")
                           wrap = false;

                     }

                 if (attr_type == "keywords")
                    {
                     QString color = hash_get_val (document->holder->palette, xml.attributes().value ("color").toString(), "darkBlue");
                     QTextCharFormat fmt = tformat_from_style (xml.attributes().value ("fontstyle").toString(), color);

                     QStringList keywordPatterns = xml.readElementText().trimmed().split(";");

                     HighlightingRule rule;

                     for (int i = 0; i < keywordPatterns.size(); i++)
                          if (! keywordPatterns.at(i).isNull())
                             {
                              rule.pattern = QRegExp (keywordPatterns.at(i).trimmed());
                              rule.format = fmt;
                              highlightingRules.append (rule);
                             }
                     } //keywords
                 else
                    if (attr_type == "item")
                       {
                        QString color = hash_get_val (document->holder->palette, xml.attributes().value ("color").toString(), "darkBlue");
                        QTextCharFormat fmt = tformat_from_style (xml.attributes().value ("fontstyle").toString(), color);

                        HighlightingRule rule;
                        rule.pattern = QRegExp (xml.readElementText().trimmed());
                        rule.format = fmt;
                        highlightingRules.append(rule);
                       }
                    else
                        if (attr_type == "mcomment-start")
                           {
                            QString color = hash_get_val (document->holder->palette, xml.attributes().value ("color").toString(), "gray");
                            QTextCharFormat fmt = tformat_from_style (xml.attributes().value ("color").toString(), color);

                            multiLineCommentFormat = fmt;
                            commentStartExpression = QRegExp (xml.readElementText().trimmed());
                           }
                    else
                        if (attr_type == "mcomment-end")
                           {
                            commentEndExpression = QRegExp (xml.readElementText().trimmed());
                           }
                    else
                        if (attr_type == "comment")
                           {
                            if (xml.attributes().value ("name").toString() == "cm_mult")
                               cm_mult = xml.readElementText().trimmed();
                            else
                            if (xml.attributes().value ("name").toString() == "cm_single")
                               cm_single = xml.readElementText().trimmed();

                          }

                     }//item

       }//is start

  if (xml.hasError())
     qDebug() << "xml parse error";

  } //cycle
}


void document_holder::save_to_session (const QString &fileName)
{
  if (list.size() < 0)
     return;

  fname_current_session = fileName;
  QString l;

  foreach (CDocument *d, list)
          {
           l += d->get_triplex();
           l += "\n";
          }
  
  qstring_save (fileName, l.trimmed());
}


void document_holder::load_from_session (const QString &fileName)
{
  if (! file_exists (fileName))
     return;

  QStringList l = qstring_load (fileName).split ("\n");
  int c = l.size();
  if (c < 0)
     return;

  foreach (QString t, l)  
          open_file_triplex (t);

  fname_current_session = fileName;
}


void document_holder::load_palette (const QString &fileName)
{
  if (! file_exists (fileName))
      return;

  palette.clear();
  palette = hash_load_keyval (fileName);
}


QString CDocument::get_selected_text() const
{
  return textEdit->textCursor().selectedText();
}


void CDocument::set_selected_text (const QString &value)
{
  textEdit->textCursor().insertText (value);
}


void CDocument::insert_image (const QString &full_path)
{
  textEdit->textCursor().insertText (get_insert_image (file_name, full_path, markup_mode));
}


void CTEAEdit::calc_auto_indent()
{
  QTextCursor cur = textCursor();
  cur.movePosition (QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
  int aindent = 0;
  
  QString s = cur.selectedText();
  int len = s.size();
  for (int i = 0; i < len; i++)
      if (! s.at(i).isSpace())
         {
          aindent = i; 
          break;
         }

  if (aindent != 0)
     indent_val = indent_val.fill (' ', aindent);
  else 
      indent_val.clear();
}


void CTEAEdit::indent()
{
  if (! textCursor().hasSelection())
     {
      QString fl;
      fl = fl.fill (' ', tab_sp_width);
         
      if (spaces_instead_of_tabs)
         textCursor().insertText (fl);
      else
          textCursor().insertText ("\t");
       
      return;
     }

  QStringList l = textCursor().selectedText().split (QChar::ParagraphSeparator);
       
  QString fl;
  fl = fl.fill (' ', tab_sp_width);
        
  QMutableListIterator <QString> i (l);

  while (i.hasNext())
        {
         QString s = i.next();
         if (spaces_instead_of_tabs)
            i.setValue (s.prepend (fl));
         else
             i.setValue (s.prepend ("\t"));
         }       
       
  textCursor().insertText (l.join ("\n"));
  
  QTextCursor cur = textCursor();
  cur.movePosition (QTextCursor::Up, QTextCursor::KeepAnchor, l.size() - 1);   
  cur.movePosition (QTextCursor::StartOfLine, QTextCursor::KeepAnchor);   
  
  setTextCursor (cur);
}


void CTEAEdit::un_indent()
{
  if (! textCursor().hasSelection())
     {
      QString t = textCursor().selectedText();
      if (! t.isEmpty() && (t.size() > 1) && t[0].isSpace()) 
         t = t.mid (1);

      return;
     }
   
  QStringList l = textCursor().selectedText().split (QChar::ParagraphSeparator);
   
  QMutableListIterator <QString> i (l);

  while (i.hasNext())
        {
         QString t = i.next();
         
         if (! t.isEmpty() && (t.size() > 1)) 
             if (t[0] == '\t' || t[0].isSpace())
                {
                 t = t.mid (1);
                 i.setValue (t);
                }              
         }       
   
  textCursor().insertText (l.join ("\n"));

  QTextCursor cur = textCursor();
  cur.movePosition (QTextCursor::Up, QTextCursor::KeepAnchor, l.size() - 1);   
  cur.movePosition (QTextCursor::StartOfLine, QTextCursor::KeepAnchor);   

  setTextCursor (cur);
}


void CTEAEdit::keyPressEvent (QKeyEvent *event)
{
  if (auto_indent)
     if (event->key() == Qt::Key_Return)
        {
         calc_auto_indent();
         QPlainTextEdit::keyPressEvent (event);
         textCursor().insertText (indent_val);
         return;
        }
   
  if (event->key() == Qt::Key_Tab)
     {
      indent();
      event->accept();
      return;
     }
  else
  if (event->key() == Qt::Key_Backtab)
     {
      un_indent();
      event->accept();
      return;
     }
  
  QPlainTextEdit::keyPressEvent (event);
}


int CTEAEdit::lineNumberAreaWidth()
{
  if (! draw_linenums)
     return 0; 

  int digits = 1;
  int max = qMax (1, blockCount());
  
  while (max >= 10) 
        {
         max /= 10;
         ++digits;
        }

  int space = (brace_width * 2) + (brace_width * digits);

  return space;
}


void CTEAEdit::updateLineNumberAreaWidth (int newBlockCount)
{
  setViewportMargins (lineNumberAreaWidth(), 0, 0, 0);
}


void CTEAEdit::updateLineNumberArea (const QRect &rect, int dy)
{
  if (dy)
     lineNumberArea->scroll (0, dy);
  else
     lineNumberArea->update (0, rect.y(), lineNumberArea->width(), rect.height());

  if (rect.contains (viewport()->rect()))
     updateLineNumberAreaWidth (0);
}


void CTEAEdit::lineNumberAreaPaintEvent (QPaintEvent *event)
{
  if (! draw_linenums)
     return;

  QPainter painter (lineNumberArea);
  painter.fillRect (event->rect(), linenums_bg);
  painter.setPen (text_color);

  QTextBlock block = firstVisibleBlock();
  int blockNumber = block.blockNumber();
  int top = (int) blockBoundingGeometry (block).translated (contentOffset()).top();
  int bottom = top + (int) blockBoundingRect (block).height();

  int w = lineNumberArea->width();
  int h = fontMetrics().height();

  while (block.isValid() && top <= event->rect().bottom()) 
        {
         if (block.isVisible() && bottom >= event->rect().top()) 
            {
             QString number = QString::number (blockNumber + 1) + " ";
//             painter.setPen (text_color);
             painter.drawText (0, top, w, h, Qt::AlignRight, number);
             /*int index = bookMark.indexOf(number.toInt());
 
             if (index != -1) 
                {
                //painter.drawText(0, top, 30, fontMetrics().height(), Qt::AlignCenter, "+");
                 painter.setBrush(QBrush(Qt::yellow, Qt::SolidPattern));
                 painter.drawEllipse(5, top + (fontMetrics().height()/4), fontMetrics().height()/2, fontMetrics().height()/2);
                }*/
              }

         block = block.next();
         top = bottom;
         bottom = top + (int) blockBoundingRect(block).height();
         ++blockNumber;
        }
}


void CTEAEdit::resizeEvent (QResizeEvent *e)
{
  QPlainTextEdit::resizeEvent (e);
  QRect cr = contentsRect();
  lineNumberArea->setGeometry (QRect (cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}


void CTEAEdit::braceHighlight()
{
  extraSelections.clear();
  setExtraSelections(extraSelections);
  selection.format.setBackground(brackets_color);

  QTextDocument *doc = document();
  QTextCursor cursor = textCursor();
  QTextCursor beforeCursor = cursor;

  cursor.movePosition (QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
  QString brace = cursor.selectedText();

  beforeCursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
  QString beforeBrace = beforeCursor.selectedText();

  if ((brace != "{") && 
      (brace != "}") &&
      (brace != "[") &&
      (brace != "]") && 
      (brace != "(") && 
      (brace != ")") && 
      (brace != "<") && 
      (brace != ">")) 
      {
       if ((beforeBrace == "{") 
           || (beforeBrace == "}") 
           || (beforeBrace == "[")
           || (beforeBrace == "]")
           || (beforeBrace == "(")
           || (beforeBrace == ")")
           || (beforeBrace == "<")
           || (beforeBrace == ">")) 
          {
           cursor = beforeCursor;
           brace = cursor.selectedText();
          }
        else 
            return;
       }

  QString openBrace;
  QString closeBrace;

  if ((brace == "{") || (brace == "}")) 
     {
      openBrace = "{";
      closeBrace = "}";
     }
  else
  if ((brace == "[") || (brace == "]")) 
     {
      openBrace = "[";
      closeBrace = "]";
     }
  else
  if ((brace == "(") || (brace == ")")) 
     {
      openBrace = "(";
      closeBrace = ")";
     }
  else
  if ((brace == "<") || (brace == ">")) 
     {
      openBrace = "<";
      closeBrace = ">";
     }


  if (brace == openBrace) 
     {
      QTextCursor cursor1 = doc->find (closeBrace, cursor);
      QTextCursor cursor2 = doc->find (openBrace, cursor);
      if (cursor2.isNull()) 
         {
          selection.cursor = cursor;
          extraSelections.append (selection);
          selection.cursor = cursor1;
          extraSelections.append (selection);
          setExtraSelections (extraSelections);
         } 
      else 
          {
           while (cursor1.position() > cursor2.position()) 
                 {
                  cursor1 = doc->find (closeBrace, cursor1);
                  cursor2 = doc->find (openBrace, cursor2);
                  if (cursor2.isNull()) 
                      break;
                 }
                
           selection.cursor = cursor;
           extraSelections.append (selection);
           selection.cursor = cursor1;
           extraSelections.append (selection);
           setExtraSelections (extraSelections);
          }
       } 
   else
       {
        if (brace == closeBrace) 
           {
            QTextCursor cursor1 = doc->find (openBrace, cursor, QTextDocument::FindBackward);
            QTextCursor cursor2 = doc->find (closeBrace, cursor, QTextDocument::FindBackward);
            if (cursor2.isNull()) 
               {
                selection.cursor = cursor;
                extraSelections.append (selection);
                selection.cursor = cursor1;
                extraSelections.append (selection);
                setExtraSelections (extraSelections);
               }
            else 
                {
                 while (cursor1.position() < cursor2.position()) 
                       {
                        cursor1 = doc->find (openBrace, cursor1, QTextDocument::FindBackward);
                        cursor2 = doc->find (closeBrace, cursor2, QTextDocument::FindBackward);
                        if (cursor2.isNull()) 
                           break;
                       }
                       
                 selection.cursor = cursor;
                 extraSelections.append (selection);
                 selection.cursor = cursor1;
                 extraSelections.append (selection);
                 setExtraSelections (extraSelections);
                }
             }
     }
}


bool CTEAEdit::canInsertFromMimeData (const QMimeData *source)
{
//  if (source->hasFormat ("text/uri-list"))
  //    return true;
  //else
    return true;
}


void CTEAEdit::insertFromMimeData (const QMimeData *source)
{
  QString fName;
  QFileInfo info;

  bool b_ins_plain_text = ! source->hasUrls();
  if (source->hasUrls() && source->urls().at(0).scheme() != "file")
      b_ins_plain_text = true;

  if (b_ins_plain_text)
    {
     QPlainTextEdit::insertFromMimeData (source);
     return;
    }

  foreach (QUrl u, source->urls())
         {
          fName = u.toLocalFile();
          info.setFile(fName);
          if (info.isFile())
             doc->holder->open_file (fName, "UTF-8");
         }
}


QStringList text_get_bookmarks (const QString &s, const QString &sstart, const QString &send)
{
  QStringList result;

  int c = s.size();
  int i = 0;

  while (i < c)
        {
         int start = s.indexOf (sstart, i, Qt::CaseInsensitive);

         if (start == -1)
             break;

         int end = s.indexOf (send, start + sstart.length());
         if (end == -1)
             break;

         result.prepend (s.mid (start + sstart.length(), (end - start) - send.length()));

         i = end + 1;
        }

  return result;
}


void CDocument::update_labels()
{
  labels.clear();
  labels = text_get_bookmarks (textEdit->toPlainText(),
                                        settings->value ("label_start", "[?").toString(),
                                        settings->value ("label_end", "?]").toString());
}
