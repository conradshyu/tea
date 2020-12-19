#include "img_viewer.h"
#include "utils.h"

#include <QVBoxLayout>
#include <QCryptographicHash>
#include <QDir>
#include <QImageReader>
#include <QKeyEvent>


CImgViewer::CImgViewer (QObject *parent): QObject (parent)
{
  window_mini.setWindowFlags (Qt::Tool);
  window_mini.resize (200, 200);
  img_mini = new QLabel (tr ("preview"));
  img_mini->setAlignment (Qt::AlignCenter);
  QVBoxLayout *lt = new QVBoxLayout;
  lt->addWidget (img_mini);
  window_mini.setLayout (lt);
}


void CImgViewer::set_image_mini (const QString &fname)
{
  window_mini.resize (200, 200);
  window_mini.setWindowTitle (QFileInfo (fname).fileName());

  QString fn = get_the_thumb_name (fname);
  if (! fn.isEmpty())
     {
      QPixmap pm (fn);
      img_mini->setPixmap (pm);
     }
  else 
      {
       QPixmap pm (fname);
       if ((pm.width() > (window_mini.width() - 10)) ||
           (pm.height() > (window_mini.height() - 10)))
          img_mini->setPixmap (pm.scaled (190, 190, Qt::KeepAspectRatio));
       else
           img_mini->setPixmap (pm);
      }
}


void CImgViewer::set_image_full (const QString &fname)
{
  window_full.show_image (fname);
}


QString CImgViewer::get_the_thumb_name (const QString &img_fname)
{
 
//FIXME: add OS2 tweak!
#if !defined(Q_WS_WIN) || !defined(Q_OS_OS2)

  QCryptographicHash h (QCryptographicHash::Md5); 
  
  QString uri (img_fname);
  uri.prepend ("file://");
  h.addData (uri.toUtf8());
  QString digest = h.result().toHex();

  QString fname (QDir::homePath());
  fname.append ("/.thumbnails/large");
  fname.append ("/").append (digest).append (".png");
 
  if (file_exists (fname))
     return fname;
  
  fname.clear();
  fname.append (QDir::homePath());
  fname.append ("/.thumbnails/normal");
  fname.append ("/").append (digest).append (".png");
 
  if (file_exists (fname))
     return fname;
  
#endif 
  
  return QString();
}


CViewerWindow::CViewerWindow (QWidget *parent): QWidget (parent)
{
  current_index = 0; 
  angle = 0.0;
  scale = 100;
  
  img_full = new QLabel (tr ("preview"));
  img_full->setAlignment (Qt::AlignCenter);
  QVBoxLayout *lt = new QVBoxLayout;
  lt->addWidget (img_full);
  setLayout (lt);
}


void CViewerWindow::show_image (const QString &fname)
{
  if (fname.isEmpty())
     return;
  
  if (! file_exists (fname))
      return; 
  
  QPixmap pm (fname);

  if (file_name != fname)
     scale = 100;

  if (scale != 100)
     pm = pm.scaledToWidth (get_value (pm.width(), scale));
  
  resize (pm.width() + 5, pm.height() + 5);
  img_full->setPixmap (pm);

  if (get_file_path (file_name) != get_file_path (fname))
  //re-read dir contents if the new file is at the another dir
  //than the previous one  
     {
      QStringList filters;
  
      QList <QByteArray> a = QImageReader::supportedImageFormats();

      foreach (QByteArray x, a)
              {
               QString t (x.data());
               t.prepend ("*.");
               filters.append (t);
              }; 
  
     QDir dir (get_file_path (fname));
     fi = dir.entryInfoList (filters, 
                             QDir::Files | QDir::Readable, 
                             QDir::Name); 
    }

  file_name = fname;
  
  for (int i = 0; i < fi.size(); i++)
       if (fi.at(i).filePath() == file_name)
          {  
           current_index = i;
           break;
          };
  
  QString s_wnd_title (QFileInfo (fname).fileName());
  s_wnd_title.append (QString (" - %1x%2").arg (pm.width()).arg(pm.height()));
  setWindowTitle (s_wnd_title);
}


void CViewerWindow::show_again()
{
  if (file_name.isEmpty())
     return;
  
  QImage pm (file_name);
  if (scale != 100)
     pm = pm.scaledToWidth (get_value (pm.width(), scale));
                           
  QTransform transform;
  transform.rotate (angle);
    
  QPixmap pixmap = QPixmap::fromImage (pm.transformed (transform));
  
  resize (pixmap.width() + 5, pixmap.height() + 5);
  img_full->setPixmap (pixmap);
  
  QString s_wnd_title (QFileInfo (file_name).fileName());
  s_wnd_title.append (QString (" - %1x%2").arg (pm.width()).arg(pm.height()));
  setWindowTitle (s_wnd_title);
}


void CViewerWindow::keyPressEvent (QKeyEvent *event)    
{
  if (event->key() == Qt::Key_Escape)
     {
      event->accept();
      close();
     }

  if ((event->key() == Qt::Key_Space) ||
      event->key() == Qt::Key_PageDown)
     {
      if (current_index < (fi.size() - 1))
          show_image (fi.at(++current_index).filePath());        
    
      event->accept();
      return;
     }

  if (event->key() == Qt::Key_PageUp)
     {
      if (current_index != 0)
         show_image (fi.at(--current_index).filePath());        
    
      event->accept();
      return;
     }

  if (event->key() == Qt::Key_Home)
     {
      current_index = 0;
      show_image (fi.at(current_index).filePath());        
      event->accept();
      return;
     }

  if (event->key() == Qt::Key_End)
     {
      current_index = fi.size() - 1;
      show_image (fi.at(current_index).filePath());        
      event->accept();
      return;
     }

  if (event->key() == Qt::Key_BracketRight)
     {
      angle += 90.0;
      if (angle >= 360.0)
         angle = 0.0;
      
      show_again();
      event->accept();
      return;
     }
  
  if (event->key() == Qt::Key_BracketLeft)
     {
      angle -= 90.0;
      if (angle <= 0)
         angle = 360;
      
      show_again();
      event->accept();
      return;
     }

  if (event->key() == Qt::Key_Plus)
     {
      scale += 10;
          
      show_again();
      event->accept();
      return;
     }

  if (event->key() == Qt::Key_Minus)
     {
      if (scale > 10)
         scale -= 10;
          
      show_again();
      event->accept();
      return;
     }
  
  QWidget::keyPressEvent (event);
}

/*
void CViewerWindow::show_image_from_clipboard()
{
  
  QClipboard *clipboard = QApplication::clipboard();
  const QMimeData *mimeData = clipboard->mimeData();

  if (! mimeData->hasImage()) 
     return;
          
  QPixmap pm (qvariant_cast<QPixmap>(mimeData->imageData()));
  if (scale != 100)
     pm = pm.scaledToWidth (get_value (pm.width(), scale));
  
  resize (pm.width() + 5, pm.height() + 5);
  img_full->setPixmap (pm);
  
  file_name = "";
  
  QString s_wnd_title (tr ("Clipboard"));
  s_wnd_title.append (QString (" - %1x%2").arg (pm.width()).arg(pm.height()));
  setWindowTitle (s_wnd_title);
}
*/
