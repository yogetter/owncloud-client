/*
 * Copyright (C) by Duncan Mac-Vicar P. <duncan@kde.org>
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
#include "mirall/owncloudwizard.h"
#include "mirall/mirallconfigfile.h"
#include "mirall/theme.h"
#include <QProgressIndicator.h>

#include <QtCore>
#include <QtGui>

#include <stdlib.h>

namespace Mirall
{

void setupCustomMedia( QVariant variant, QLabel *label )
{
    if( !label ) return;

    QPixmap pix = variant.value<QPixmap>();
    if( !pix.isNull() ) {
        label->setPixmap(pix);
        label->setAlignment( Qt::AlignTop | Qt::AlignRight );
        label->setVisible(true);
    } else {
        QString str = variant.toString();
        if( !str.isEmpty() ) {
            label->setText( str );
            label->setTextFormat( Qt::RichText );
            label->setVisible(true);
            label->setOpenExternalLinks(true);
        }
    }
}

// ======================================================================

OwncloudSetupPage::OwncloudSetupPage()
{
    _ui.setupUi(this);

    // Backgroundcolor for owncloud logo #1d2d42
    setTitle(tr("Create Connection to %1").arg(Theme::instance()->appNameGUI()));

    connect(_ui.leUrl, SIGNAL(textChanged(QString)), SLOT(handleNewOcUrl(QString)));

    registerField( QLatin1String("OCUrl"),    _ui.leUrl );
    registerField( QLatin1String("OCUser"),   _ui.leUsername );
    registerField( QLatin1String("OCPasswd"), _ui.lePassword);
    connect( _ui.lePassword, SIGNAL(textChanged(QString)), this, SIGNAL(completeChanged()));
    connect( _ui.leUsername, SIGNAL(textChanged(QString)), this, SIGNAL(completeChanged()));
    connect( _ui.cbAdvanced, SIGNAL(stateChanged (int)), SLOT(slotToggleAdvanced(int)));
    connect( _ui.pbSelectLocalFolder, SIGNAL(clicked()), SLOT(slotSelectFolder()));
    _ui.errorLabel->setVisible(true);
    _ui.advancedBox->setVisible(false);

    _progressIndi = new QProgressIndicator;
    _ui.resultLayout->addWidget( _progressIndi );
    _progressIndi->setVisible(false);

    // Error label
    QString style = QLatin1String("border: 1px solid #eed3d7; border-radius: 5px; padding: 3px;"
                                  "background-color: #f2dede; color: #b94a48;");


    _ui.errorLabel->setStyleSheet( style );
    _ui.errorLabel->setWordWrap(true);
    _ui.errorLabel->setVisible(false);
    setTitle( tr("<font color=\"#ffffff\" size=\"5\">Connect to your %1 Server</font>").arg( Theme::instance()->appNameGUI()));
    setSubTitle( tr("<font color=\"#1d2d42\">Enter user credentials to access your %1</font>").arg(Theme::instance()->appNameGUI()));

    // ButtonGroup for
    _selectiveSyncButtons = new QButtonGroup;
    _selectiveSyncButtons->addButton( _ui.pbBoxMode );
    _selectiveSyncButtons->addButton( _ui.pbSelectiveMode );
    connect( _selectiveSyncButtons, SIGNAL(buttonClicked (QAbstractButton*)),
             SLOT(slotChangedSelective(QAbstractButton*)));

    _ui.selectiveSyncLabel->setVisible(false);
    _ui.pbBoxMode->setVisible(false);
    _ui.pbSelectiveMode->setVisible(false);

    _checking = false;

    setupCustomization();
}

OwncloudSetupPage::~OwncloudSetupPage()
{
    delete _progressIndi;
}

void OwncloudSetupPage::slotToggleAdvanced(int state)
{
    _ui.advancedBox->setVisible( state == Qt::Checked );
}

void OwncloudSetupPage::slotChangedSelective(QAbstractButton* button)
{
    if( button = _ui.pbBoxMode ) {
        // box mode - sync the entire oC
    } else {
        // content mode, select folder list.
    }
}

void OwncloudSetupPage::setOCUser( const QString & user )
{
    if( _ui.leUsername->text().isEmpty() ) {
        _ui.leUsername->setText(user);
    }
}

void OwncloudSetupPage::setOCUrl( const QString& newUrl )
{
    QString url( newUrl );
    if( url.isEmpty() ) {
        _ui.leUrl->clear();
        return;
    }

    _ui.leUrl->setText( url );
}

void OwncloudSetupPage::setupCustomization()
{
    // set defaults for the customize labels.

    // _ui.topLabel->hide();
    _ui.bottomLabel->hide();

    Theme *theme = Theme::instance();
    QVariant variant = theme->customMedia( Theme::oCSetupTop );
    if( variant.isNull() ) {
        _ui.topLabel->setOpenExternalLinks(true);
        _ui.topLabel->setText("If you don't have an ownCloud server yet, see <a href=\"https://owncloud.com\">owncloud.com</a> for more info.");
    } else {
        setupCustomMedia( variant, _ui.topLabel );
    }

    variant = theme->customMedia( Theme::oCSetupBottom );
    setupCustomMedia( variant, _ui.bottomLabel );

    QString fixUrl = theme->overrideServerUrl();
    if( !fixUrl.isEmpty() ) {
        setOCUrl( fixUrl );
        _ui.leUrl->setEnabled( false );
        _ui.leUrl->hide();
    }
}

// slot hit from textChanged of the url entry field.
void OwncloudSetupPage::handleNewOcUrl(const QString& ocUrl)
{
    QString url = ocUrl;
    int len = 0;
    bool visible = false;
#if 0
    if (url.startsWith(QLatin1String("https://"))) {
        _ui.urlLabel->setPixmap( QPixmap(":/mirall/resources/security-high.png"));
        _ui.urlLabel->setToolTip(tr("This url is secure. You can use it."));
        visible = true;
    }
    if (url.startsWith(QLatin1String("http://"))) {
        _ui.urlLabel->setPixmap( QPixmap(":/mirall/resources/security-low.png"));
        _ui.urlLabel->setToolTip(tr("This url is NOT secure. You should not use it."));
        visible = true;
    }
#endif

}

bool OwncloudSetupPage::isComplete() const
{
    if( _ui.leUrl->text().isEmpty() ) return false;
    if( _checking ) return false;

    return !( _ui.lePassword->text().isEmpty() || _ui.leUsername->text().isEmpty() );
}

void OwncloudSetupPage::initializePage()
{
    _connected = false;
}

int OwncloudSetupPage::nextId() const
{
  return OwncloudWizard::Page_Result;
}

QString OwncloudSetupPage::url() const
{
    QString url = _ui.leUrl->text().simplified();
    return url;
}

void OwncloudSetupPage::setConnected( bool comp )
{
    _connected = comp;
}

bool OwncloudSetupPage::validatePage()
{
    bool re = false;

    if( ! _connected) {
        setErrorString(QString::null);
        _checking = true;
        _progressIndi->setVisible(true);
        _progressIndi->startAnimation();
        emit completeChanged();

        emit connectToOCUrl( url() );
        return false;
    } else {
        // connecting is running
        stopSpinner();
        _checking = false;
        emit completeChanged();
        return true;
    }
}

void OwncloudSetupPage::setErrorString( const QString& err )
{
    if( err.isEmpty()) {
        _ui.errorLabel->setVisible(false);
    } else {
        _ui.errorLabel->setVisible(true);
        _ui.errorLabel->setText(err);
    }
    _checking = false;
    emit completeChanged();
    stopSpinner();
}

void OwncloudSetupPage::stopSpinner()
{
    // _ui.addressLayout->removeWidget( _progressIndi );

    _progressIndi->setVisible(false);
    _progressIndi->stopAnimation();
}

OwncloudSetupPage::SyncMode OwncloudSetupPage::syncMode()
{
    if( _selectiveSyncButtons->checkedButton() &&
            _selectiveSyncButtons->checkedButton() == _ui.pbSelectiveMode ) {
        return SelectiveMode;
    }
    return BoxMode;
}

void OwncloudSetupPage::setLocalFolder( const QString& folder )
{
    _ui.pbSelectLocalFolder->setText(folder);

    QString t = tr("Your entire account will be synced to the local folder %1").arg(folder);
    _ui.syncModeLabel->setText(t);
}

QString OwncloudSetupPage::selectedLocalFolder() const
{
    return _ui.pbSelectLocalFolder->text();
}

void OwncloudSetupPage::slotSelectFolder()
{

    QString dir = QFileDialog::getExistingDirectory(0, tr("Local Sync Folder"), QDir::homePath());
    if( !dir.isEmpty() ) {
        setLocalFolder(dir);
    }
}

// ======================================================================

OwncloudWizardResultPage::OwncloudWizardResultPage()
{
    _ui.setupUi(this);
    // no fields to register.

    _ui.pbOpenLocal->setText("Open local folder");

    _ui.pbOpenServer->setText(tr("Open %1").arg(Theme::instance()->appNameGUI()));

    setTitle( tr("<font color=\"#ffffff\" size=\"5\">Everything set up!</font>").arg( Theme::instance()->appNameGUI()));
    setSubTitle( tr("<font color=\"#1d2d42\">Enter user credentials to access your %1</font>").arg(Theme::instance()->appNameGUI()));

    _ui.pbOpenLocal->setIcon(QIcon(":/mirall/resources/folder-sync.png"));
    _ui.pbOpenLocal->setText(tr("Open Local Folder"));
    _ui.pbOpenLocal->setIconSize(QSize(48, 48));

    _ui.pbOpenLocal->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    _ui.pbOpenServer->setIcon(QIcon(":/mirall/resources/mirall-48.png"));
    _ui.pbOpenServer->setText(tr("Open Server"));
    _ui.pbOpenServer->setIconSize(QSize(48, 48));
    _ui.pbOpenServer->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    setupCustomization();
}

OwncloudWizardResultPage::~OwncloudWizardResultPage()
{
}

void OwncloudWizardResultPage::setOwncloudUrl( const QString& url )
{
    _url = url;
}

void OwncloudWizardResultPage::setLocalFolder( const QString& folder )
{
    _localFolder = folder;
    _ui.localFolderLabel->setText(tr("Your entire account is synced to the local folder %1").arg(folder));
}

void OwncloudWizardResultPage::setupCustomization()
{
    // set defaults for the customize labels.
    _ui.topLabel->setText( QString::null );
    _ui.topLabel->hide();

    QVariant variant = Theme::instance()->customMedia( Theme::oCSetupResultTop );
    setupCustomMedia( variant, _ui.topLabel );
}

// ======================================================================

/**
 * Folder wizard itself
 */

OwncloudWizard::OwncloudWizard(QWidget *parent)
    : QWizard(parent)
{
    setPage(Page_oCSetup,    new OwncloudSetupPage() );
    setPage(Page_Result,    new OwncloudWizardResultPage() );
    // note: start Id is set by the calling class depending on if the
    // welcome text is to be shown or not.
    setWizardStyle( QWizard::ModernStyle );

    connect( this, SIGNAL(currentIdChanged(int)), SLOT(slotCurrentPageChanged(int)));

    OwncloudSetupPage *p = static_cast<OwncloudSetupPage*>(page(Page_oCSetup));
    connect( p, SIGNAL(connectToOCUrl(QString)), SIGNAL(connectToOCUrl(QString)));

    QPixmap pix(QSize(540, 78));
    pix.fill(QColor("#1d2d42"));
    setPixmap( QWizard::BannerPixmap, pix );

    QPixmap logo( ":/mirall/resources/owncloud_logo.png");
    setPixmap( QWizard::LogoPixmap, logo );
    setWizardStyle(QWizard::ModernStyle);
    setOption( QWizard::NoBackButtonOnStartPage );
    setOption( QWizard::NoCancelButton );
    setTitleFormat(Qt::RichText);
    setSubTitleFormat(Qt::RichText);

}

QString OwncloudWizard::ocUrl() const
{
    QString url = field("OCUrl").toString().simplified();
    return url;
}

void OwncloudWizard::setLocalFolder( const QString& folder )
{
    OwncloudSetupPage *p = static_cast<OwncloudSetupPage*>(page(Page_oCSetup));
    p->setLocalFolder(folder);
}

QString OwncloudWizard::selectedLocalFolder() const
{
    OwncloudSetupPage *p = static_cast<OwncloudSetupPage*>(page(Page_oCSetup));
    return p->selectedLocalFolder();
}

void OwncloudWizard::showConnectInfo( const QString& msg )
{
    OwncloudSetupPage *p = static_cast<OwncloudSetupPage*>(page(Page_oCSetup));
    if( p ) {
        p->setErrorString( msg );
    }
}

void OwncloudWizard::successfullyConnected(bool enable)
{
    OwncloudSetupPage *p = static_cast<OwncloudSetupPage*>(page(Page_oCSetup));
    if( p ) {
        p->setConnected( enable );
    }
    if( enable ) {
        next();
    }
}

void OwncloudWizard::slotCurrentPageChanged( int id )
{
  qDebug() << "Current Wizard page changed to " << id;

  if( id == Page_oCSetup ) {
      setButtonText( QWizard::NextButton, tr("Connect...") );
      emit clearPendingRequests();
  }

  if( id == Page_Result ) {
    appendToResultWidget( QString::null );
    showOCUrlLabel( false );
    OwncloudWizardResultPage *p = static_cast<OwncloudWizardResultPage*> (page( Page_Result ));
    if( p ) p->setLocalFolder( selectedLocalFolder() );
  }
}

void OwncloudWizard::showOCUrlLabel( bool show )
{
  OwncloudWizardResultPage *p = static_cast<OwncloudWizardResultPage*> (page( Page_Result ));
}

void OwncloudWizard::displayError( const QString& msg )
{
    OwncloudSetupPage *p = static_cast<OwncloudSetupPage*>(page(Page_oCSetup));
    if( p ) p->setErrorString( msg );
}

void OwncloudWizard::appendToResultWidget( const QString& msg, LogType type )
{
  OwncloudWizardResultPage *r = static_cast<OwncloudWizardResultPage*> (page( Page_Result ));
  qDebug() << "XXXXXXXXXXXXX " << msg;
}

void OwncloudWizard::setOCUrl( const QString& url )
{
  _oCUrl = url;
  OwncloudSetupPage *p = static_cast<OwncloudSetupPage*>(page(Page_oCSetup));
  if( p )
      p->setOCUrl( url );

}

void OwncloudWizard::setOCUser( const QString& user )
{
  _oCUser = user;
  OwncloudSetupPage *p = static_cast<OwncloudSetupPage*>(page(Page_oCSetup));
  if( p )
      p->setOCUser( user );
}

OwncloudSetupPage::SyncMode OwncloudWizard::syncMode()
{
    OwncloudSetupPage *p = static_cast<OwncloudSetupPage*>(page(Page_oCSetup));
    if( p )
        return p->syncMode();
    return OwncloudSetupPage::BoxMode;
}

} // end namespace
