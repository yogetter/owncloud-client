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


#include "accountsettings.h"
#include "ui_accountsettings.h"

#include "mirall/theme.h"
#include "mirall/folderstatusmodel.h"
#include "mirall/owncloudinfo.h"
#include "mirall/credentialstore.h"
#include "mirall/folderscheduler.h"
#include "mirall/folderwizard.h"

#include <QDebug>
#include <QDesktopServices>
#include <QListWidgetItem>

namespace Mirall {

AccountSettings::AccountSettings(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AccountSettings),
    _item(0)
{
    ui->setupUi(this);
    disableProgressBar();

    _model = new FolderStatusModel;
    FolderViewDelegate *delegate = new FolderViewDelegate;

    ui->_folderList->setItemDelegate( delegate );
    ui->_folderList->setModel( _model );
    ui->_folderList->setMinimumWidth( 300 );
    ui->_folderList->setEditTriggers( QAbstractItemView::NoEditTriggers );

    ui->_ButtonAdd->setEnabled(true);
    ui->_ButtonRemove->setEnabled(false);
    ui->_ButtonEnable->setEnabled(false);
    ui->_ButtonInfo->setEnabled(false);

    connect(ui->_ButtonRemove, SIGNAL(clicked()), this, SLOT(slotRemoveFolder()));

    connect(ui->_ButtonEnable, SIGNAL(clicked()), this, SLOT(slotEnableFolder()));
    connect(ui->_ButtonInfo,   SIGNAL(clicked()), this, SLOT(slotInfoFolder()));
    connect(ui->_ButtonAdd,    SIGNAL(clicked()), this, SLOT(slotAddFolder()));

    connect(ui->_folderList, SIGNAL(clicked(QModelIndex)), SLOT(slotFolderActivated(QModelIndex)));
    connect(ui->_folderList, SIGNAL(doubleClicked(QModelIndex)),SLOT(slotDoubleClicked(QModelIndex)));

    connect( FolderScheduler::instance(), SIGNAL(folderSyncStateChange(QString)),
             this,SLOT(slotSyncStateChange(QString)));

    ui->connectLabel->setWordWrap( true );

    setFolderList(FolderScheduler::instance()->map());

    slotCheckConnection();
}

void AccountSettings::slotFolderActivated( const QModelIndex& indx )
{
  bool state = indx.isValid();

  ui->_ButtonRemove->setEnabled( state );
  ui->_ButtonEnable->setEnabled( state );
  ui->_ButtonInfo->setEnabled( state );

  if ( state ) {
    bool folderEnabled = _model->data( indx, FolderViewDelegate::FolderSyncEnabled).toBool();
    qDebug() << "folder is sync enabled: " << folderEnabled;
    if ( folderEnabled ) {
      ui->_ButtonEnable->setText( tr( "Pause" ) );
    } else {
      ui->_ButtonEnable->setText( tr( "Resume" ) );
    }
  }
}

void AccountSettings::slotAddFolder( Folder *folder )
{
    if( ! folder || folder->alias().isEmpty() ) return;

    QStandardItem *item = new QStandardItem();
    folderToModelItem( item, folder );
    _model->appendRow( item );
    slotCheckConnection();
}


void AccountSettings::buttonsSetEnabled()
{
    bool haveFolders = ui->_folderList->model()->rowCount() > 0;

    ui->_ButtonRemove->setEnabled(false);
    if( Theme::instance()->singleSyncFolder() ) {
        // only one folder synced folder allowed.
        ui->_ButtonAdd->setVisible(!haveFolders);
    } else {
        ui->_ButtonAdd->setVisible(true);
        ui->_ButtonAdd->setEnabled(true);
    }

    QModelIndex selected = ui->_folderList->currentIndex();
    bool isSelected = selected.isValid();

    ui->_ButtonEnable->setEnabled(isSelected);
    ui->_ButtonRemove->setEnabled(isSelected);
    ui->_ButtonInfo->setEnabled(isSelected);
}

void AccountSettings::setListWidgetItem( QListWidgetItem *item )
{
    _item = item;
}

QStandardItem* AccountSettings::folderItem( Folder *folder )
{
    QStandardItem *item = 0;
    int row = 0;

    if( ! folder ) return 0;

    item = _model->item( row );

    while( item ) {
        if( item->data( FolderViewDelegate::FolderAliasRole ) == folder->alias() ) {
            // its the item to update!
            break;
        }
        item = _model->item( ++row );
    }
    return item;
}

void AccountSettings::slotUpdateFolderState( const QString& folderName )
{
    Folder *folder = 0;
    QStandardItem *item = 0;

    if( folderName.isEmpty() ) {
        return;
    }

    folder = FolderScheduler::instance()->folder(folderName);
    item = folderItem( folder );

    if( item ) {
        folderToModelItem( item, folder );
    } else {
        // the dialog is not visible.
    }
    slotCheckConnection();
}

void AccountSettings::slotSetFolderProgress( const QString& folderName, const QString& file,
                                      long p1, long p2 )
{
    Folder *folder = 0;

    QStandardItem *item = 0;

    if( folderName.isEmpty() ) {
        return;
    }

    folder = FolderScheduler::instance()->folder(folderName);
    item = folderItem( folder );

    if( item ) {
        QVariant v;
        item->setData( file,  FolderViewDelegate::SyncFileName  );
        v.setValue(p1);
        item->setData( v,    FolderViewDelegate::SyncProgress1 );
        v.setValue(p2);
        item->setData( v,    FolderViewDelegate::SyncProgress2 );
    }
}

void AccountSettings::folderToModelItem( QStandardItem *item, Folder *f )
{
    if( ! item || !f ) return;

    QIcon icon = Theme::instance()->folderIcon( f->backend() );
    item->setData( icon,                   FolderViewDelegate::FolderIconRole );
    item->setData( f->nativePath(),        FolderViewDelegate::FolderPathRole );
    item->setData( f->nativeSecondPath(),  FolderViewDelegate::FolderSecondPathRole );
    item->setData( f->alias(),             FolderViewDelegate::FolderAliasRole );
    item->setData( f->syncEnabled(),       FolderViewDelegate::FolderSyncEnabled );

    SyncResult res = f->syncResult();
    SyncResult::Status status = res.status();

    QString errors = res.errorStrings().join(QLatin1String("<br/>"));

    Theme *theme = Theme::instance();

    item->setData( theme->statusHeaderText( status ),  Qt::ToolTipRole );
    if( f->syncEnabled() ) {
        item->setData( theme->syncStateIcon( status ), FolderViewDelegate::FolderStatusIcon );
    } else {
        item->setData( theme->folderDisabledIcon( ),   FolderViewDelegate::FolderStatusIcon ); // size 48 before
    }
    item->setData( theme->statusHeaderText( status ),  FolderViewDelegate::FolderStatus );
    item->setData( errors,                             FolderViewDelegate::FolderErrorMsg );
}

void AccountSettings::slotRemoveFolder()
{
    QModelIndex selected = ui->_folderList->selectionModel()->currentIndex();
    if( selected.isValid() ) {
        QString alias = _model->data( selected, FolderViewDelegate::FolderAliasRole ).toString();
        qDebug() << "Remove Folder alias " << alias;
        if( !alias.isEmpty() ) {
            // remove from file system through folder man
            emit(removeFolderAlias( alias ));
            // _model->removeRow( selected.row() );
        }
    }
    slotCheckConnection();
}

void AccountSettings::slotRemoveSelectedFolder()
{
    QModelIndex selected = ui->_folderList->selectionModel()->currentIndex();
    if( selected.isValid() ) {
        _model->removeRow( selected.row() );
    }
    buttonsSetEnabled();
    slotCheckConnection();
}

void AccountSettings::slotDoubleClicked( const QModelIndex& indx )
{
    if( ! indx.isValid() ) return;
    QString alias = _model->data( indx, FolderViewDelegate::FolderAliasRole ).toString();

    emit openFolderAlias( alias );
}

void AccountSettings::slotCheckConnection()
{
    if( ownCloudInfo::instance()->isConfigured() ) {
        connect(ownCloudInfo::instance(), SIGNAL(ownCloudInfoFound(const QString&, const QString&, const QString&, const QString&)),
                this, SLOT(slotOCInfo( const QString&, const QString&, const QString&, const QString& )));
        connect(ownCloudInfo::instance(), SIGNAL(noOwncloudFound(QNetworkReply*)),
                this, SLOT(slotOCInfoFail(QNetworkReply*)));

        ui->connectLabel->setText( tr("Checking %1 connection...").arg(Theme::instance()->appNameGUI()));
        qDebug() << "Check status.php from statusdialog.";
        ownCloudInfo::instance()->checkInstallation();
    } else {
        // ownCloud is not yet configured.
        ui->connectLabel->setText( tr("No %1 connection configured.").arg(Theme::instance()->appNameGUI()));
        ui->_ButtonAdd->setEnabled( false);
    }
}

void AccountSettings::setFolderList( Folder::Map folders )
{
    _model->clear();
    foreach( Folder *f, folders ) {
        qDebug() << "Folder: " << f;
        slotAddFolder( f );
    }

   QModelIndex idx = _model->index(0, 0);
   if (idx.isValid())
        ui->_folderList->setCurrentIndex(idx);
    buttonsSetEnabled();

}

// move from Application
void AccountSettings::slotFolderOpenAction( const QString& alias )
{
    Folder *f = FolderScheduler::instance()->folder(alias);
    qDebug() << "opening local url " << f->path();
    if( f ) {
        QUrl url(f->path(), QUrl::TolerantMode);
        url.setScheme( QLatin1String("file") );

#ifdef Q_OS_WIN32
        // work around a bug in QDesktopServices on Win32, see i-net
        QString filePath = f->path();

        if (filePath.startsWith(QLatin1String("\\\\")) || filePath.startsWith(QLatin1String("//")))
            url.setUrl(QDir::toNativeSeparators(filePath));
        else
            url = QUrl::fromLocalFile(filePath);
#endif
        QDesktopServices::openUrl(url);
    }
}


void AccountSettings::slotAddFolder()
{
    FolderScheduler *folderScheduler = FolderScheduler::instance();

    folderScheduler->setSyncEnabled(false); // do not start more syncs.

    FolderWizard *folderWizard = new FolderWizard(this);
    Folder::Map folderMap = folderScheduler->map();
    folderWizard->setFolderMap( &folderMap );

    connect(folderWizard, SIGNAL(accepted()), SLOT(slotFolderWizardAccepted()));
    connect(folderWizard, SIGNAL(rejected()), SLOT(slotFolderWizardRejected()));
    folderWizard->open();
}

void AccountSettings::slotFolderWizardAccepted()
{
    FolderWizard *folderWizard = qobject_cast<FolderWizard*>(sender());

    qDebug() << "* Folder wizard completed";

    bool goodData = true;

    QString alias        = folderWizard->field(QLatin1String("alias")).toString();
    QString sourceFolder = folderWizard->field(QLatin1String("sourceFolder")).toString();
    QString backend      = QLatin1String("csync");
    QString targetPath;

    if (folderWizard->field(QLatin1String("local?")).toBool()) {
        // setup a local csync folder
        targetPath = folderWizard->field(QLatin1String("targetLocalFolder")).toString();
    } else if (folderWizard->field(QLatin1String("remote?")).toBool()) {
        // setup a remote csync folder
        targetPath  = folderWizard->field(QLatin1String("targetURLFolder")).toString();
    } else if( folderWizard->field(QLatin1String("OC?")).toBool() ||
               Theme::instance()->singleSyncFolder()) {
        // setup a ownCloud folder
        backend    = QLatin1String("owncloud");
        targetPath = folderWizard->field(QLatin1String("targetOCFolder")).toString(); //empty in single folder mode
    } else {
        qWarning() << "* Folder not local and note remote?";
        goodData = false;
    }

    FolderScheduler* folderScheduler = FolderScheduler::instance();
    folderScheduler->setSyncEnabled(true); // do start sync again.

    if( goodData ) {
        folderScheduler->addFolderDefinition( backend, alias, sourceFolder, targetPath, false );
        Folder *f = folderScheduler->setupFolderFromConfigFile( alias );
        if( f ) {
            slotAddFolder( f );
        }
    }
}

void AccountSettings::slotFolderWizardRejected()
{
    qDebug() << "* Folder wizard cancelled";
    FolderWizard *folderWizard = qobject_cast<FolderWizard*>(sender());
    FolderScheduler* folderScheduler = FolderScheduler::instance();
    folderScheduler->setSyncEnabled(true);
    folderScheduler->slotScheduleAllFolders();
}

void AccountSettings::slotEnableFolder()
{
  QModelIndex selected = ui->_folderList->selectionModel()->currentIndex();
  if( selected.isValid() ) {
    QString alias = _model->data( selected, FolderViewDelegate::FolderAliasRole ).toString();
    bool folderEnabled = _model->data( selected, FolderViewDelegate::FolderSyncEnabled).toBool();
    qDebug() << "Toggle enabled/disabled Folder alias " << alias << " - current state: " << folderEnabled;
    if( !alias.isEmpty() ) {
      emit(enableFolderAlias( alias, !folderEnabled ));

      // set the button text accordingly.
      slotFolderActivated( selected );
    }
  }
}

void AccountSettings::slotInfoFolder()
{
  QModelIndex selected = ui->_folderList->selectionModel()->currentIndex();
  if( selected.isValid() ) {
    QString alias = _model->data( selected, FolderViewDelegate::FolderAliasRole ).toString();
    qDebug() << "Info Folder alias " << alias;
    if( !alias.isEmpty() ) {
      emit(infoFolderAlias( alias ));
    }
  }
}

void AccountSettings::slotOCInfo( const QString& url, const QString& versionStr, const QString& version, const QString& )
{
#ifdef Q_OS_WIN32
        // work around a bug in QDesktopServices on Win32, see i-net
        QString filePath = url;

        if (filePath.startsWith("\\\\") || filePath.startsWith("//"))
            _OCUrl.setUrl(QDir::toNativeSeparators(filePath));
        else
            _OCUrl = QUrl::fromLocalFile(filePath);
#else
    _OCUrl = QUrl::fromLocalFile(url);
#endif

    qDebug() << "#-------# oC found on " << url;
    /* enable the open button */
    ui->connectLabel->setOpenExternalLinks(true);
    ui->connectLabel->setText( tr("Connected to <a href=\"%1\">%1</a> as <i>%2</i>.")
                          .arg(url).arg( CredentialStore::instance()->user()) );
    ui->connectLabel->setToolTip( tr("Version: %1 (%2)").arg(versionStr).arg(version));
    ui->_ButtonAdd->setEnabled(true);

    disconnect(ownCloudInfo::instance(), SIGNAL(ownCloudInfoFound(const QString&, const QString&, const QString&, const QString&)),
            this, SLOT(slotOCInfo( const QString&, const QString&, const QString&, const QString& )));
    disconnect(ownCloudInfo::instance(), SIGNAL(noOwncloudFound(QNetworkReply*)),
            this, SLOT(slotOCInfoFail(QNetworkReply*)));
}

void AccountSettings::slotOCInfoFail( QNetworkReply *reply)
{
    QString errStr = tr("unknown problem.");
    if( reply ) errStr = reply->errorString();

    ui->connectLabel->setText( tr("<p>Failed to connect to %1: <tt>%2</tt></p>").arg(Theme::instance()->appNameGUI()).arg(errStr) );
    ui->_ButtonAdd->setEnabled( false);

    disconnect(ownCloudInfo::instance(), SIGNAL(ownCloudInfoFound(const QString&, const QString&, const QString&, const QString&)),
            this, SLOT(slotOCInfo( const QString&, const QString&, const QString&, const QString& )));
    disconnect(ownCloudInfo::instance(), SIGNAL(noOwncloudFound(QNetworkReply*)),
            this, SLOT(slotOCInfoFail(QNetworkReply*)));

}

void AccountSettings::slotOpenOC()
{
  if( _OCUrl.isValid() )
    QDesktopServices::openUrl( _OCUrl );
}

void AccountSettings::enableProgressBar( const QString& file, int maximum )
{
    ui->progressBar->setMaximum( maximum );

    ui->progressBar->setEnabled(true);
    ui->fileProgressLabel->setText(tr("Uploading %1").arg(file));

}

void AccountSettings::disableProgressBar()
{
    // ui->progressBar->setMaximum(0);
    ui->progressBar->setValue(0);
    ui->progressBar->setEnabled(false);
    ui->fileProgressLabel->setText(tr("No activity."));
    // ui->progressBar->hide();
}


AccountSettings::~AccountSettings()
{
    delete ui;
}

} // namespace Mirall
