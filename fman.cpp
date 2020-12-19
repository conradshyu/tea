 /**************************************************************************
 *   2007-2013 by Peter Semiletov                            *
 *   peter.semiletov@gmail.com                                             *
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

#include "fman.h"
#include "utils.h"
#include "logmemo.h"

#include <QStandardItem>
#include <QFileInfoList>
#include <QUrl>
#include <QMimeData>
#include <QDebug>
#include <QApplication>


void CFMan::dir_up()
{
  if (dir.path() == "/")
     return;
  
  QString oldcurdir = dir.dirName();

  dir.cdUp();
  nav (dir.path());

  QModelIndex index = index_from_name (oldcurdir);

  selectionModel()->setCurrentIndex(index, QItemSelectionModel::Rows | QItemSelectionModel::NoUpdate);

  scrollTo (index);
}


void CFMan::nav (const QString &path)
{
  dir.setPath (path);
  if (! dir.exists())
      return;

  setModel (0);

  mymodel->removeRows (0, mymodel->rowCount());
  QFileInfoList lst = dir.entryInfoList (QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot |
                                         QDir::Files | QDir::Drives, QDir::Name |
                                         QDir::DirsFirst | QDir::IgnoreCase | QDir::LocaleAware);

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)

  if (path != "/")
     append_dot_entry ("..");

#else

  if (path.size() != 2)
     append_dot_entry ("..");

#endif

  foreach (QFileInfo fi, lst)
          add_entry (fi);
          
  setModel (mymodel);
  connect (selectionModel(), SIGNAL(currentChanged (const QModelIndex &, const QModelIndex &)), this, SLOT(cb_fman_currentChanged (const QModelIndex &, const QModelIndex &)));
  emit dir_changed (path);
}


const QModelIndex CFMan::index_from_name (const QString &name)
{
  QList <QStandardItem *> lst = mymodel->findItems (name);

  if (lst.size() > 0)
     return mymodel->indexFromItem (lst[0]);
  else
      return QModelIndex();
}


void CFMan::tv_activated (const QModelIndex &index)
{
  QString item_string = index.data().toString();
  
  QString dpath = dir.path();
  
  if (dpath.size() > 1)
     if (dpath.endsWith("/") || dpath.endsWith("\\"))
      	  dpath.truncate(dpath.size() - 1);
    
  QString full_path;
  
  if (dpath == "/")
     full_path = "/" + item_string;
  else
     full_path = dpath + "/" + item_string;
  
  if (item_string == ".." && dir.path() != "/")
     {
      dir_up();
      return;
     }

  if (is_dir (full_path))
     {
      nav (full_path);
      QModelIndex index = mymodel->index (0, 0);
      selectionModel()->setCurrentIndex (index, QItemSelectionModel::Select |
                                         QItemSelectionModel::Rows);

      return;
     }
  else
      emit file_activated (full_path);
}


void CFMan::add_entry (const QFileInfo &fi)
{
  QList<QStandardItem *> items;

  QStandardItem *item = new QStandardItem (fi.fileName());

  if (fi.isDir())
     {
      QFont f = item->font();
      f.setBold (true);
      //f.setWeight (QFont::DemiBold);
      item->setFont(f);
     }

  item->setFlags (Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
  items.append (item);

  item = new QStandardItem (QString::number (fi.size()));
  item->setFlags (Qt::ItemIsSelectable | Qt::ItemIsEnabled);
  items.append (item);

  item = new QStandardItem (fi.lastModified().toString ("yyyy-MM-dd"));
  item->setFlags (Qt::ItemIsSelectable | Qt::ItemIsEnabled);
  items.append (item);

  mymodel->appendRow (items);
}


void CFMan::append_dot_entry (const QString &fname)
{
  QList<QStandardItem *> items;

  QStandardItem *item = new QStandardItem (fname);
  item->setFlags (Qt::ItemIsSelectable | Qt::ItemIsEnabled);
  items.append (item);

  item = new QStandardItem ("-");
  item->setFlags (Qt::ItemIsSelectable | Qt::ItemIsEnabled);
  items.append (item);

  item = new QStandardItem ("-");
  item->setFlags (Qt::ItemIsSelectable | Qt::ItemIsEnabled);
  items.append (item);

  mymodel->appendRow (items);
}


CFMan::CFMan (QWidget *parent): QTreeView (parent)
{
  mymodel = new QStandardItemModel (0, 3, parent);

  mymodel->setHeaderData (0, Qt::Horizontal, QObject::tr ("Name"));
  mymodel->setHeaderData (1, Qt::Horizontal, QObject::tr ("Size"));
  mymodel->setHeaderData (2, Qt::Horizontal, QObject::tr ("Modified at"));

  setRootIsDecorated (false);

  setAlternatingRowColors (true);
  setAllColumnsShowFocus (true);

  setModel (mymodel);

  setDragEnabled (true);
  

#if QT_VERSION >= 0x050000

  header()->setSectionResizeMode (QHeaderView::ResizeToContents);

#else

  header()->setResizeMode (QHeaderView::ResizeToContents);

#endif


  header()->setStretchLastSection (false);

  setSelectionMode (QAbstractItemView::ExtendedSelection);
  setSelectionBehavior (QAbstractItemView::SelectRows);

  connect (this, SIGNAL(activated(const QModelIndex &)), this, SLOT(tv_activated(const QModelIndex &)));

}


void CFMan::cb_fman_currentChanged (const QModelIndex &current, const QModelIndex &previous )
{
  int row = current.row();
  if (row < 0)
     {
      emit current_file_changed ("", "");
      return;
     }

  QModelIndex i = model()->index (row, 0);

  QString item_string = i.data().toString();

  QString full_path = dir.path() + "/" + item_string;

  if (! is_dir (full_path))
     emit current_file_changed (full_path, item_string);
}

/*
CFMan::~CFMan()
{
  qDebug() << "~CFMan()";
}
*/

QString CFMan::get_sel_fname()
{
  if (! selectionModel()->hasSelection())
      return QString();

  QModelIndex index = selectionModel()->currentIndex();
  QString item_string = index.data().toString();
  QString full_path (dir.path());
  full_path.append ("/").append (item_string);
  return full_path;
}


QStringList CFMan::get_sel_fnames()
{
  if (! selectionModel()->hasSelection())
      return QStringList();

  QModelIndexList il = selectionModel()->QItemSelectionModel::selectedRows (0);
  QStringList li;

  foreach (QModelIndex index, il)
          {
           QString item_string = index.data().toString();
           if (item_string != "..") 
              {
               QString full_path = dir.path() + "/" + item_string;
               li.append (full_path);
              } 
          }

  return li;
}


void CFMan::refresh()
{
  QString current;

  if (selectionModel()->hasSelection())
     {    
      QModelIndex index = selectionModel()->currentIndex();
      current = index.data().toString();
     } 

  nav (dir.path());

  QModelIndex index = index_from_name (current);
  selectionModel()->setCurrentIndex (index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
  scrollTo (index);
}


const QModelIndex CFMan::index_from_idx (int idx)
{
  QStandardItem *item = mymodel->item (idx);
  if (item)
     return mymodel->indexFromItem (item);
  else
      return QModelIndex();
}


int CFMan::get_sel_index()
{
  if (! selectionModel()->hasSelection())
     return -1;

  QModelIndex index = selectionModel()->currentIndex();
  return index.row();
}


void CFMan::mouseMoveEvent (QMouseEvent *event)
{
  if (! (event->buttons() & Qt::LeftButton)) 
     return;

  QStringList l = get_sel_fnames();
  if (l.size() < 1)
     return;
  
  QDrag *drag = new QDrag (this);
  QMimeData *mimeData = new QMimeData;
 
  QList <QUrl> list;

  foreach (QString fn, l)
           list.append (QUrl::fromLocalFile (fn)); 

  mimeData->setUrls (list); 
  drag->setMimeData (mimeData);
 
  if (drag->exec (Qt::CopyAction | 
                  Qt::MoveAction |
                  Qt::LinkAction) == Qt::MoveAction)
     refresh();
    
  event->accept();
}


void CFMan::keyPressEvent (QKeyEvent *event)
{
  if (event->key() == Qt::Key_Insert)
       {
        bool sel = false;
        QModelIndex index = selectionModel()->currentIndex();
        int row = index.row();

        if (selectionModel()->isSelected (index))
           sel = true;

        sel = ! sel;

        if (sel)
           selectionModel()->select (index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
        else
            selectionModel()->select (index, QItemSelectionModel::Deselect | QItemSelectionModel::Rows);


        if (row < mymodel->rowCount() - 1)
           {
            row++;

            QModelIndex newindex = mymodel->index (row, 0);

            selectionModel()->setCurrentIndex (newindex, /*QItemSelectionModel::Select*/QItemSelectionModel::Current | QItemSelectionModel::Rows);
            scrollTo (newindex);
           }

        event->accept();
        return;
       }

  if (event->key() == Qt::Key_Backspace)
       {
        dir_up();
        event->accept();
        return;
       }


  if( event->key() == Qt::Key_Up )
             {
              if (currentIndex().row() == 0)
                 {
                  event->accept();
                  return;
                  }

                 selectionModel()->setCurrentIndex(indexAbove(currentIndex()), QItemSelectionModel::Rows | QItemSelectionModel::NoUpdate );
                 event->accept();
                 return;

             }

             if( event->key() == Qt::Key_Down )
                {
                 if (currentIndex().row() == mymodel->rowCount() - 1)
                    {
                     event->accept();
                     return;
                    }


                 selectionModel()->setCurrentIndex( indexBelow(currentIndex()), QItemSelectionModel::Rows | QItemSelectionModel::NoUpdate );
                 event->accept();
                 return;
             }


             if( event->key() == Qt::Key_PageUp )
                {
                 QModelIndex idx = moveCursor (QAbstractItemView::MovePageUp, Qt::NoModifier);
                 selectionModel()->setCurrentIndex(idx, QItemSelectionModel::Rows | QItemSelectionModel::NoUpdate);
                 event->accept();
                 return;
                }


             if( event->key() == Qt::Key_PageDown )
                {
                 QModelIndex idx = moveCursor (QAbstractItemView::MovePageDown, Qt::NoModifier);
                 selectionModel()->setCurrentIndex(idx, QItemSelectionModel::Rows | QItemSelectionModel::NoUpdate);
                 event->accept();
                 return;
                }


             if( event->key() == Qt::Key_End )
                {

                 QModelIndex idx = mymodel->index (mymodel->rowCount() - 1, 0);
                 selectionModel()->setCurrentIndex(idx, QItemSelectionModel::Rows | QItemSelectionModel::NoUpdate );
                 event->accept();
                 return;

                }


             if( event->key() == Qt::Key_Home)
                {
                 QModelIndex idx = mymodel->index (0, 0);
                 selectionModel()->setCurrentIndex(idx, QItemSelectionModel::Rows | QItemSelectionModel::NoUpdate);
                 event->accept();
                 return;
                }


  QTreeView::keyPressEvent (event);
}


void CFMan::drawRow (QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{


 if(index.row() == currentIndex().row())
 // if (index.row() == selectionModel()->currentIndex().row())
     {
      QStyleOptionViewItem current_option = option;
      QTreeView::drawRow (painter, current_option, index);


      QStyleOptionFocusRect o;
        o.rect = option.rect.adjusted(1,1,-1,-1);


         o.state |= QStyle::State_KeyboardFocusChange;
         o.state |= QStyle::State_Item;

       QApplication::style()->drawPrimitive (QStyle::PE_FrameFocusRect, &o, painter);

     }
else
    QTreeView::drawRow (painter, option, index);

}

