/*
 * Copyright (C) by Daniel Molkentin <danimo@owncloud.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

#ifndef ACCOUNTSETTINGS_H
#define ACCOUNTSETTINGS_H

#include <QWidget>
#include <QUrl>

#include "mirall/folder.h"

class QStandardItemModel;
class QModelIndex;
class QStandardItem;
class QNetworkReply;
class QListWidgetItem;

namespace Mirall {

namespace Ui {
class AccountSettings;
}

class AccountSettings : public QWidget
{
    Q_OBJECT

public:
    explicit AccountSettings(QWidget *parent = 0);
    ~AccountSettings();

    void setFolderList( Folder::Map );
    void buttonsSetEnabled();
    void setListWidgetItem(QListWidgetItem* item);

signals:
    void removeFolderAlias( const QString& );
    void fetchFolderAlias( const QString& );
    void pushFolderAlias( const QString& );
    void enableFolderAlias( const QString&, const bool );
    void infoFolderAlias( const QString& );
    void openFolderAlias( const QString& );

public slots:
    void slotAddFolder();
    void slotAddFolder( Folder* );
    void slotFolderWizardAccepted();
    void slotFolderWizardRejected();
    void slotRemoveFolder();
    void slotRemoveSelectedFolder();
    void slotFolderActivated( const QModelIndex& );
    void slotOpenOC();
    void slotEnableFolder();
    void slotInfoFolder();
    void slotUpdateFolderState( const QString& );
    void slotCheckConnection();
    void slotOCInfo( const QString&, const QString&, const QString&, const QString& );
    void slotOCInfoFail( QNetworkReply* );
    void slotDoubleClicked( const QModelIndex& );
    void slotFolderOpenAction( const QString& );
    void slotSetFolderProgress( const QString&, const QString&, long, long );

private:
    void folderToModelItem( QStandardItem *, Folder * );
    void disableProgressBar();
    void enableProgressBar( const QString&, int );
    QStandardItem* folderItem( Folder* );

    Ui::AccountSettings *ui;
    QStandardItemModel *_model;
    QListWidgetItem *_item;
    QUrl   _OCUrl;
};

} // namespace Mirall

#endif // ACCOUNTSETTINGS_H
