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

#include <QDebug>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFileInfo>
#include <QUrl>
#include <QValidator>
#include <QWizardPage>
#include <QDir>
#include <QScrollBar>
#include <QSslSocket>

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


OwncloudWelcomePage::OwncloudWelcomePage()
{
    setTitle(tr("Welcome to %1").arg(Theme::instance()->appNameGUI()));

    QVBoxLayout *lay = new QVBoxLayout(this);
    QLabel *content = new QLabel;
    lay->addWidget(content, 100, Qt::AlignTop);
    content->setAlignment(Qt::AlignTop);
    content->setTextFormat(Qt::RichText);
    content->setWordWrap(true);
    Theme *theme = Theme::instance();
    if (theme->overrideServerUrl().isEmpty()) {
        content->setText(tr("<p>In order to connect to your %1 server, you need to provide the server address "
                            "as well as your credentials.</p><p>This wizard will guide you through the process.<p>"
                            "<p>If you have not received this information, please contact your %1 provider.</p>")
                         .arg(theme->appNameGUI()));
    } else {
        content->setText(tr("<p>In order to connect to your %1 server, you need to provide "
                            "your credentials.</p><p>This wizard will guide you through "
                            "the setup process.</p>").arg(theme->appNameGUI()));
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
    _ui.errorLabel->setVisible(false);
    _ui.advancedBox->setVisible(false);

    // Error label
    // painter->setBrush( QColor(0xbb, 0x4d, 0x4d) );
    // painter->setPen( QColor(0xaa, 0xaa, 0xaa));

    QString style = QLatin1String("border: 1px solid red; border-radius: 4px; padding: 4px;"
                                  "background-color: #bb4d4d; color: #ffffff;");

    _ui.errorLabel->setStyleSheet( style );
    _ui.errorLabel->setWordWrap(true);
    setupCustomization();
}

OwncloudSetupPage::~OwncloudSetupPage()
{
}

void OwncloudSetupPage::slotToggleAdvanced(int state)
{
    _ui.advancedBox->setVisible( state == Qt::Checked );
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

    _ui.topLabel->hide();
    _ui.bottomLabel->hide();

    Theme *theme = Theme::instance();
    QVariant variant = theme->customMedia( Theme::oCSetupTop );
    setupCustomMedia( variant, _ui.topLabel );
    variant = theme->customMedia( Theme::oCSetupBottom );
    setupCustomMedia( variant, _ui.bottomLabel );

    QString fixUrl = theme->overrideServerUrl();
    if( !fixUrl.isEmpty() ) {
        setOCUrl( fixUrl );
        _ui.leUrl->setEnabled( false );
        _ui.leUrl->hide();
        _ui.serverAddressLabel->hide();
    }
}

// slot hit from textChanged of the url entry field.
void OwncloudSetupPage::handleNewOcUrl(const QString& ocUrl)
{
    QString url = ocUrl;
#if 0
    int len = 0;
    if (url.startsWith(QLatin1String("https://"))) {
        len = 8;
    }
    if (url.startsWith(QLatin1String("http://"))) {
        len = 7;
    }
    if( len ) {
        int pos = _ui.leUrl->cursorPosition();
        url.remove(0, len);
        _ui.leUrl->setText(url);
        _ui.leUrl->setCursorPosition(qMax(0, pos-len));

    }
#endif
}

bool OwncloudSetupPage::isComplete() const
{
    if( _ui.leUrl->text().isEmpty() ) return false;

    return !( _ui.lePassword->text().isEmpty() || _ui.leUsername->text().isEmpty() );
}

void OwncloudSetupPage::initializePage()
{
    _connected = false;
}

int OwncloudSetupPage::nextId() const
{
  return OwncloudWizard::Page_Install;
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
        emit connectToOCUrl( url() );
        return false;
    } else {
        // connecting is running
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
}

// ======================================================================

OwncloudWizardResultPage::OwncloudWizardResultPage()
{
    _ui.setupUi(this);
    // no fields to register.
    _ui.resultTextEdit->setAcceptRichText(true);
    _ui.ocLinkLabel->setVisible( false );

    setupCustomization();
}

OwncloudWizardResultPage::~OwncloudWizardResultPage()
{
}

void OwncloudWizardResultPage::initializePage()
{
    _complete = false;
    // _ui.lineEditOCAlias->setText( "Owncloud" );
}

bool OwncloudWizardResultPage::isComplete() const
{
    return _complete;
}

void OwncloudWizardResultPage::appendResultText( const QString& msg, OwncloudWizard::LogType type )
{
  if( msg.isEmpty() ) {
    _ui.resultTextEdit->clear();
  } else {
    if( type == OwncloudWizard::LogParagraph ) {
      _ui.resultTextEdit->append( msg );
    } else {
      // _ui.resultTextEdit->append( msg );
      _ui.resultTextEdit->insertPlainText(msg );
    }
    _ui.resultTextEdit->verticalScrollBar()->setValue( _ui.resultTextEdit->verticalScrollBar()->maximum() );
  }
}

void OwncloudWizardResultPage::showOCUrlLabel( const QString& url, bool show )
{
  _ui.ocLinkLabel->setText( tr("Congratulations! Your <a href=\"%1\" title=\"%1\">new %2</a> is now up and running!")
          .arg(url).arg( Theme::instance()->appNameGUI()));
  _ui.ocLinkLabel->setOpenExternalLinks( true );

  if( show ) {
    _ui.ocLinkLabel->setVisible( true );
  } else {
    _ui.ocLinkLabel->setVisible( false );
  }
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
    setPage(Page_oCWelcome,  new OwncloudWelcomePage() );
    setPage(Page_oCSetup,    new OwncloudSetupPage() );
    setPage(Page_Install,    new OwncloudWizardResultPage() );
    // note: start Id is set by the calling class depending on if the
    // welcome text is to be shown or not.
#ifdef Q_WS_MAC
    setWizardStyle( QWizard::ModernStyle );
#endif

    connect( this, SIGNAL(currentIdChanged(int)), SLOT(slotCurrentPageChanged(int)));

    OwncloudSetupPage *p = static_cast<OwncloudSetupPage*>(page(Page_oCSetup));
    connect( p, SIGNAL(connectToOCUrl(QString)), SIGNAL(connectToOCUrl(QString)));
}

QString OwncloudWizard::ocUrl() const
{
    QString url = field("OCUrl").toString().simplified();
    return url;
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
  qDebug() << "Page_install is " << Page_Install;

  button(QWizard::BackButton)->setVisible(id != Page_oCSetup);

  if( id == Page_oCSetup ) {
      setButtonText( QWizard::NextButton, tr("Connect...") );
      emit clearPendingRequests();
  }

  if( id == Page_Install ) {
    appendToResultWidget( QString::null );
    showOCUrlLabel( false );
  }
}

void OwncloudWizard::showOCUrlLabel( bool show )
{
  OwncloudWizardResultPage *p = static_cast<OwncloudWizardResultPage*> (page( Page_Install ));
  p->showOCUrlLabel( _oCUrl, show );
}

void OwncloudWizard::displayError( const QString& msg )
{
    OwncloudSetupPage *p = static_cast<OwncloudSetupPage*>(page(Page_oCSetup));
    if( p ) p->setErrorString( msg );
}

void OwncloudWizard::appendToResultWidget( const QString& msg, LogType type )
{
  OwncloudWizardResultPage *r = static_cast<OwncloudWizardResultPage*> (page( Page_Install ));
  r->appendResultText(msg, type);
  qDebug() << "XXXXXXXXXXXXX " << msg;
}

void OwncloudWizard::setOCUrl( const QString& url )
{
  _oCUrl = url;
#ifdef OWNCLOUD_CLIENT
  OwncloudSetupPage *p = static_cast<OwncloudSetupPage*>(page(Page_oCSetup));
#else
  OwncloudWizardSelectTypePage *p = static_cast<OwncloudWizardSelectTypePage*>(page( Page_SelectType ));
#endif
  if( p )
      p->setOCUrl( url );

}

void OwncloudWizard::setOCUser( const QString& user )
{
  _oCUser = user;
#ifdef OWNCLOUD_CLIENT
  OwncloudSetupPage *p = static_cast<OwncloudSetupPage*>(page(Page_oCSetup));
  if( p )
      p->setOCUser( user );
#else
  OwncloudWizardSelectTypePage *p = static_cast<OwncloudWizardSelectTypePage*>(page( Page_SelectType ));
#endif
}

} // end namespace
