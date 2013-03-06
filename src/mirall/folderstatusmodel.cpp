/*
 * Copyright (C) by Klaas Freitag <freitag@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

#include "mirall/folderstatusmodel.h"
#include "mirall/folder.h"
#include "mirall/theme.h"
#include "mirall/owncloudinfo.h"
#include "mirall/mirallconfigfile.h"
#include "mirall/credentialstore.h"
#include "mirall/fileitemdialog.h"

#include <QtCore>
#include <QtGui>

namespace Mirall {

FolderStatusModel::FolderStatusModel()
    :QStandardItemModel()
{

}

Qt::ItemFlags FolderStatusModel::flags ( const QModelIndex&  )
{
    return Qt::ItemIsSelectable;
}

QVariant FolderStatusModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == Qt::EditRole)
        return QVariant();
    else
        return QStandardItemModel::data(index,role);
}

// ====================================================================================

FolderViewDelegate::FolderViewDelegate()
    :QStyledItemDelegate()
{

}

FolderViewDelegate::~FolderViewDelegate()
{
  // TODO Auto-generated destructor stub
}

//alocate each item size in listview.
QSize FolderViewDelegate::sizeHint(const QStyleOptionViewItem & option ,
                                   const QModelIndex & index) const
{
  int w = 0;

  QString p = qvariant_cast<QString>(index.data(FolderPathRole));
  QFont aliasFont = QApplication::font();
  QFont font = QApplication::font();
  aliasFont.setPointSize( font.pointSize() +2 );

  QFontMetrics fm(font);
  QFontMetrics aliasFm(aliasFont);

  int margin = aliasFm.height()/2;

  w = 8 + fm.boundingRect( p ).width();

  // calc height

  int h = margin;  // margin to top
  h += aliasFm.height();       // alias
  h += fm.height()/2;          // between alias and local path
  h += fm.height();            // local path
  h += fm.height()/2;          // between local and remote path
  h += fm.height();            // remote path
  h += margin;     // bottom margin

  int minHeight = 48 + margin + margin; // icon + margins

  if( h < minHeight ) h = minHeight;

  // add some space to show an error condition.
  if( ! qvariant_cast<QString>(index.data(FolderErrorMsg)).isEmpty() ) {
      h += margin+fm.height();
  }

  return QSize( w, h );
}

void FolderViewDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                               const QModelIndex &index) const
{
  QStyledItemDelegate::paint(painter,option,index);

  painter->save();

  QFont aliasFont    = QApplication::font();
  QFont subFont = QApplication::font();
  QFont errorFont = subFont;

  //font.setPixelSize(font.weight()+);
  aliasFont.setBold(true);
  aliasFont.setPointSize( subFont.pointSize()+2 );

  QFontMetrics subFm( subFont );
  QFontMetrics aliasFm( aliasFont );
  int margin = aliasFm.height()/2;

  QIcon folderIcon = qvariant_cast<QIcon>(index.data(FolderIconRole));
  QIcon statusIcon = qvariant_cast<QIcon>(index.data(FolderStatusIcon));
  QString aliasText = qvariant_cast<QString>(index.data(FolderAliasRole));
  QString pathText = qvariant_cast<QString>(index.data(FolderPathRole));
  QString remotePath = qvariant_cast<QString>(index.data(FolderSecondPathRole));
  QString errorText  = qvariant_cast<QString>(index.data(FolderErrorMsg));

  // QString statusText = qvariant_cast<QString>(index.data(FolderStatus));
  bool syncEnabled = index.data(FolderSyncEnabled).toBool();
  // QString syncStatus = syncEnabled? tr( "Enabled" ) : tr( "Disabled" );

  QSize iconsize(48, 48); //  = icon.actualSize(option.decorationSize);

  QRect aliasRect = option.rect;
  QRect iconRect = option.rect;

  iconRect.setLeft( margin );
  iconRect.setWidth( 48 );
  iconRect.setTop( iconRect.top() + margin ); // (iconRect.height()-iconsize.height())/2);

  QRect statusRect = iconRect;
  statusRect.setLeft( option.rect.right() - margin - 48 );
  statusRect.setRight( option.rect.right() - margin );

  aliasRect.setLeft(iconRect.right()+margin);

  aliasRect.setTop(aliasRect.top() + aliasFm.height()/2 );
  aliasRect.setBottom(aliasRect.top()+subFm.height());

  // local directory box
  QRect localPathRect = aliasRect;
  localPathRect.setTop(aliasRect.bottom() + margin / 3);
  localPathRect.setBottom(localPathRect.top()+subFm.height());

  // remote directory box
  QRect remotePathRect = localPathRect;
  remotePathRect.setTop( localPathRect.bottom() + subFm.height()/2 );
  remotePathRect.setBottom( remotePathRect.top() + subFm.height());

  iconRect.setBottom(remotePathRect.bottom());

  //painter->drawPixmap(QPoint(iconRect.right()/2,iconRect.top()/2),icon.pixmap(iconsize.width(),iconsize.height()));
  if( syncEnabled ) {
      painter->drawPixmap(QPoint(iconRect.left(),iconRect.top()), folderIcon.pixmap(iconsize.width(),iconsize.height()));
  } else {
      painter->drawPixmap(QPoint(iconRect.left(),iconRect.top()), folderIcon.pixmap(iconsize.width(),iconsize.height(), QIcon::Disabled ));
  }

  painter->drawPixmap(QPoint(statusRect.left(), statusRect.top()), statusIcon.pixmap(48,48));

  painter->setFont(aliasFont);
  painter->drawText(aliasRect, aliasText);

  painter->setFont(subFont);
  painter->drawText(localPathRect.left(),localPathRect.top()+17, pathText);
  painter->drawText(remotePathRect, tr("Remote path: %1").arg(remotePath));

  // paint an error overlay if there is an error string
  if( !errorText.isEmpty() ) {
      QRect errorRect = localPathRect;
      errorRect.setLeft( iconRect.left());
      errorRect.setTop( iconRect.bottom()+subFm.height()/2 );
      errorRect.setHeight(subFm.height()+margin);
      errorRect.setRight( statusRect.right() );

      painter->setBrush( QColor(0xbb, 0x4d, 0x4d) );
      painter->setPen( QColor(0xaa, 0xaa, 0xaa));
      painter->drawRoundedRect( errorRect, 4, 4 );

      QIcon warnIcon(":/mirall/resources/warning-16");
      painter->drawPixmap( QPoint(errorRect.left()+2, errorRect.top()+2), warnIcon.pixmap(QSize(16,16)));

      painter->setPen( Qt::white );
      painter->setFont(errorFont);
      QRect errorTextRect = errorRect;
      errorTextRect.setLeft( errorTextRect.left()+margin/2 +16);
      errorTextRect.setTop( errorTextRect.top()+margin/2 );

      int linebreak = errorText.indexOf(QLatin1String("<br"));
      QString eText = errorText;
      if(linebreak) {
          eText = errorText.left(linebreak);
      }
      painter->drawText(errorTextRect, eText);
  }

  // painter->drawText(lastSyncRect, tr("Last Sync: %1").arg( statusText ));
  // painter->drawText(statusRect, tr("Sync Status: %1").arg( syncStatus ));
  painter->restore();

}


bool FolderViewDelegate::editorEvent ( QEvent * event, QAbstractItemModel * model, const QStyleOptionViewItem & option, const QModelIndex & index )
{
    return false;
}

/*
  * in the show event, start a connection check to the ownCloud.
  */
//void FolderStatusWidget::showEvent ( QShowEvent *event  )
//{
//    QTimer::singleShot(0, this, SLOT(slotCheckConnection()));
//    QDialog::showEvent( event );
//}

}
