#ifndef IMG_VIEWER_H
#define IMG_VIEWER_H

//#include <QtGui>

#include <QObject>
#include <QWidget>
#include <QFileInfoList>
#include <QLabel>


class CViewerWindow: public QWidget
{  
  Q_OBJECT

public:

   qreal angle;
   int scale;
   
   QFileInfoList fi;
   QString file_name;
   int current_index;
   
   QLabel *img_full;

   //QPoint orgn;
   //QRubberBand *rubber_band;
   
   CViewerWindow (QWidget *parent = 0);
   void show_image (const QString &fname);
//   void show_image_from_clipboard();

   void show_again();
   
protected:

  void keyPressEvent (QKeyEvent *event);    
 // void mousePressEvent(QMouseEvent *event);
 // void mouseMoveEvent(QMouseEvent *event);
 // void mouseReleaseEvent(QMouseEvent *event);
};


class CImgViewer: public QObject
{
  Q_OBJECT

public:

  QWidget window_mini; 
  CViewerWindow window_full;

  QLabel *img_mini;

  CImgViewer (QObject *parent = 0);
  void set_image_mini (const QString &fname);
  void set_image_full (const QString &fname);
  //void set_image_full_clipboard();

  QString get_the_thumb_name (const QString &img_fname);
};

#endif // IMG_VIEWER_H
