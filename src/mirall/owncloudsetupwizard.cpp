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

#include "mirall/owncloudsetupwizard.h"
#include "mirall/mirallconfigfile.h"
#include "mirall/owncloudinfo.h"
#include "mirall/folderman.h"
#include "mirall/credentialstore.h"
#include "mirall/connectionvalidator.h"

#include <QtCore>
#include <QProcess>
#include <QMessageBox>
#include <QDesktopServices>

namespace Mirall {

class Theme;

OwncloudSetupWizard::OwncloudSetupWizard( FolderMan *folderMan, Theme *theme, QObject *parent ) :
    QObject( parent ),
    _mkdirRequestReply(0),
    _checkInstallationRequest(0),
    _folderMan(folderMan)
{
    _ocWizard = new OwncloudWizard();

    connect( _ocWizard, SIGNAL(connectToOCUrl( const QString& ) ),
             this, SLOT(slotConnectToOCUrl( const QString& )));

    connect( _ocWizard, SIGNAL(finished(int)),this,SLOT(slotAssistantFinished(int)));

    connect( _ocWizard, SIGNAL(clearPendingRequests()),
             this, SLOT(slotClearPendingRequests()));

    _ocWizard->setWindowTitle( tr("%1 Connection Wizard").arg( theme ? theme->appNameGUI() : QLatin1String("Mirall") ) );

}

OwncloudSetupWizard::~OwncloudSetupWizard()
{

}

void OwncloudSetupWizard::startWizard(bool intro)
{
    // create the ocInfo object
    connect(ownCloudInfo::instance(),SIGNAL(ownCloudInfoFound(QString,QString,QString,QString)),
            SLOT(slotOwnCloudFound(QString,QString,QString,QString)));
    connect(ownCloudInfo::instance(),SIGNAL(noOwncloudFound(QNetworkReply*)),
            SLOT(slotNoOwnCloudFound(QNetworkReply*)));
    connect(ownCloudInfo::instance(),SIGNAL(webdavColCreated(QNetworkReply::NetworkError)),
            SLOT(slotCreateRemoteFolderFinished(QNetworkReply::NetworkError)));

    MirallConfigFile cfgFile;
    // Fill the entry fields with existing values.
    QString url = cfgFile.ownCloudUrl();
    if( !url.isEmpty() ) {
        _ocWizard->setOCUrl( url );
    }
    QString user = cfgFile.ownCloudUser();
    if( !user.isEmpty() ) {
        _ocWizard->setOCUser( user );
    }

    if (intro)
        _ocWizard->setStartId(OwncloudWizard::Page_oCWelcome);
    else
        _ocWizard->setStartId(OwncloudWizard::Page_oCSetup);
    _ocWizard->restart();
    _ocWizard->show();
}


// Method executed when the user ends the wizard, either with 'accept' or 'reject'.
// accept the custom config to be the main one if Accepted.
void OwncloudSetupWizard::slotAssistantFinished( int result )
{
    MirallConfigFile cfg( _configHandle );

    if( result == QDialog::Rejected ) {
        // the old config remains valid. Remove the temporary one.
        cfg.cleanupCustomConfig();
        qDebug() << "Rejected the new config, use the old!";
    } else if( result == QDialog::Accepted ) {
        qDebug() << "Config Changes were accepted!";

        // go through all folders and remove the journals if the server changed.
        MirallConfigFile prevCfg;
        if( prevCfg.ownCloudUrl() != cfg.ownCloudUrl() ) {
            qDebug() << "ownCloud URL has changed, journals needs to be wiped.";
            _folderMan->wipeAllJournals();
        }

        // save the user credentials and afterwards clear the cred store.
        cfg.acceptCustomConfig();

        // Now write the resulting folder definition if folder names are set.
        if( !( _localFolder.isEmpty() || _remoteFolder.isEmpty() ) ) { // both variables are set.
            if( _folderMan ) {
                _folderMan->addFolderDefinition( QLatin1String("owncloud"), Theme::instance()->appName(),
				_localFolder, _remoteFolder, false );
                _ocWizard->appendToResultWidget(tr("<font color=\"green\"><b>Local sync folder %1 successfully created!</b></font>").arg(_localFolder));
            } else {
                qDebug() << "WRN: Folderman is zero in Setup Wizzard.";
            }
        }
    } else {
        qDebug() << "WRN: Got unknown dialog result code " << result;
    }

    // clear the custom config handle
    _configHandle.clear();
    ownCloudInfo::instance()->setCustomConfigHandle( QString::null );

    // disconnect the ocInfo object
    disconnect(ownCloudInfo::instance(), SIGNAL(ownCloudInfoFound(QString,QString,QString,QString)),
               this, SLOT(slotOwnCloudFound(QString,QString,QString,QString)));
    disconnect(ownCloudInfo::instance(), SIGNAL(noOwncloudFound(QNetworkReply*)),
               this, SLOT(slotNoOwnCloudFound(QNetworkReply*)));
    disconnect(ownCloudInfo::instance(), SIGNAL(webdavColCreated(QNetworkReply::NetworkError)),
               this, SLOT(slotCreateRemoteFolderFinished(QNetworkReply::NetworkError)));

    // notify others.
    emit ownCloudWizardDone( result );
}

void OwncloudSetupWizard::slotConnectToOCUrl( const QString& url )
{
  qDebug() << "Connect to url: " << url;
  _ocWizard->setField(QLatin1String("OCUrl"), url );
  _ocWizard->appendToResultWidget(tr("Trying to connect to %1 at %2...")
                                  .arg( Theme::instance()->appNameGUI() ).arg(url) );
  testOwnCloudConnect();
}

// Cleanup pending requests if user clicks the back button
void OwncloudSetupWizard::slotClearPendingRequests()
{
    qDebug() << "Pending request: " << _mkdirRequestReply;
    if( _mkdirRequestReply && _mkdirRequestReply->isRunning() ) {
        qDebug() << "ABORTing pending mkdir request.";
        _mkdirRequestReply->abort();
    }
    if( _checkInstallationRequest && _checkInstallationRequest->isRunning() ) {
        qDebug() << "ABORTing pending check installation request.";
        _checkInstallationRequest->abort();
    }
}

void OwncloudSetupWizard::testOwnCloudConnect()
{
    // write a temporary config.
    QDateTime now = QDateTime::currentDateTime();
    _configHandle = now.toString(QLatin1String("MMddyyhhmmss"));

    MirallConfigFile cfgFile( _configHandle );

    cfgFile.writeOwncloudConfig( Theme::instance()->appName(),
                                 _ocWizard->field(QLatin1String("OCUrl")).toString(),
                                 _ocWizard->field(QLatin1String("OCUser")).toString(),
                                 _ocWizard->field(QLatin1String("OCPasswd")).toString(),
                                 _ocWizard->field(QLatin1String("secureConnect")).toBool(),
                                 _ocWizard->field(QLatin1String("PwdNoLocalStore")).toBool() );

    // If there is already a config, take its proxy config.
    if( ownCloudInfo::instance()->isConfigured() ) {
        MirallConfigFile prevCfg;
        if( prevCfg.proxyType() != QNetworkProxy::DefaultProxy ) {
            cfgFile.setProxyType( prevCfg.proxyType(), prevCfg.proxyHostName(), prevCfg.proxyPort(),
                                  prevCfg.proxyUser(), prevCfg.proxyPassword() );
        }
    }

    _conValidator = new ConnectionValidator( _configHandle );
    connect( _conValidator, SIGNAL(connectionResult(ConnectionValidator::Status)),
             SLOT(slotConnectionResult(ConnectionValidator::Status)));
    _conValidator->checkConnection();

}

void OwncloudSetupWizard::slotConnectionResult( ConnectionValidator::Status status )
{
    qDebug() << "Connection Validator Result: " << _conValidator->statusString(status);
    int cnt = 0;
    QString infoMsg, msgHeader;
    bool connection = false;

    switch( status ) {
    case ConnectionValidator::Undefined:
        qDebug() << "Connection Validator Undefined.";
        break;
    case ConnectionValidator::Connected:
        connection = true;
        break;
    case ConnectionValidator::NotConfigured:
        qDebug() << "Connection Validator Not Configured.";
        break;
    case ConnectionValidator::ServerVersionMismatch:
        qDebug() << "Connection Validator ServerVersionMismatch.";
        msgHeader = tr("%1 Server Mismatch").arg(Theme::instance()->appNameGUI());
        infoMsg = tr("<b>Server and client are incompatible.</b> Please update the server and restart the client.");
        return;
        break;
    case ConnectionValidator::CredentialsTooManyAttempts:
        qDebug() << "Connection Validator Too many attempts.";
        infoMsg = tr("Too many authentication attempts to %1.")
                .arg(Theme::instance()->appNameGUI());
        msgHeader = tr("Credentials");
        break;
    case ConnectionValidator::CredentialError:
        qDebug() << "Connection Validator Credential Error.";
        infoMsg = tr("Error to fetch user credentials to. Please check configuration.");
        msgHeader = tr("Credentials");
        break;
    case ConnectionValidator::CredentialsUserCanceled:
        qDebug() << "Connection Validator Credential User Canceled.";
        infoMsg = tr("User canceled authentication request to %1")
                .arg(Theme::instance()->appNameGUI());
        msgHeader = tr("Credentials");
        break;
    case ConnectionValidator::CredentialsWrong:
        qDebug() << "Connection Validator Credentials wrong.";
        infoMsg = tr("<b>Username or password are wrong</b>. Please check again.");
        msgHeader = tr("Credentials");
        break;
    case ConnectionValidator::StatusNotFound:
        qDebug() << "Connection Validator Status No Found.";
        // Check again in a while.
        if( _configHandle.isEmpty() ) {
            qDebug() << "Could not reach server, trying again.";
            QTimer::singleShot(30000, _conValidator, SLOT(checkConnection()));
        } else {
            // First setup mode.
            infoMsg = tr("<b>Server is not reachable.</b> Please check server address.");
            msgHeader = tr("Host unreachable");
        }

        break;
    default:
        qDebug() << "Connection Validator Undefined.";
        break;
    }

    showMsg( msgHeader, infoMsg );
    _ocWizard->successfullyConnected( connection );
    _conValidator->deleteLater();
}

#define QS(X) QLatin1String(X)

void OwncloudSetupWizard::showMsg( const QString& header, const QString& msg)
{
    // QString style;
    if( header.isEmpty() || msg.isEmpty() ) return;
    QString m; //  = QS("<h2>") + header + QS("</h2>");
    m += QS("<p>") + msg + QS("</p>");
    _ocWizard->showConnectInfo( m );
}



OwncloudWizard *OwncloudSetupWizard::wizard()
{
    return _ocWizard;
}

void OwncloudSetupWizard::setupSyncFolder()
{
    _localFolder = QDir::homePath() + QDir::separator() + Theme::instance()->defaultClientFolder();

    if( ! _folderMan ) return;

    qDebug() << "Setup local sync folder for new oC connection " << _localFolder;
    QDir fi( _localFolder );

    bool localFolderOk = true;
    if( fi.exists() ) {
        // there is an existing local folder. If its non empty, it can only be synced if the
        // ownCloud is newly created.
        _ocWizard->appendToResultWidget( tr("Local sync folder %1 already exists, setting it up for sync.<br/><br/>").arg(_localFolder));
    } else {
        QString res = tr("Creating local sync folder %1... ").arg(_localFolder);
        if( fi.mkpath( _localFolder ) ) {
            // FIXME: Create a local sync folder.
            res += tr("ok");
        } else {
            res += tr("failed.");
            qDebug() << "Failed to create " << fi.path();
            localFolderOk = false;
        }
        _ocWizard->appendToResultWidget( res );
    }

    if( localFolderOk ) {
        _remoteFolder = Theme::instance()->defaultServerFolder();
        // slotCreateRemoteFolder(true);
    }
}
#if 0
void OwncloudSetupWizard::slotCreateRemoteFolder(bool credentialsOk )
{
    if( ! credentialsOk ) {
        // User pressed cancel while being asked for password.
        _ocWizard->appendToResultWidget("User canceled password dialog. Can not connect.");
        return;
    }

    if( createRemoteFolder( _remoteFolder ) ) {
        qDebug() << "Started remote folder creation ok";
    } else {
        _ocWizard->appendToResultWidget(tr("Creation of remote folder %1 could not be started.").arg(_remoteFolder));
    }
}

bool OwncloudSetupWizard::createRemoteFolder( const QString& folder )
{
    if( folder.isEmpty() ) return false;

    qDebug() << "creating folder on ownCloud: " << folder;

    _mkdirRequestReply = ownCloudInfo::instance()->mkdirRequest( folder );

    return true;
}

void OwncloudSetupWizard::slotCreateRemoteFolderFinished( QNetworkReply::NetworkError error )
{
    qDebug() << "** webdav mkdir request finished " << error;
    bool success = true;

    if( error == QNetworkReply::NoError ) {
        _ocWizard->appendToResultWidget( tr("Remote folder %1 created successfully.").arg(_remoteFolder));
    } else if( error == 202 ) {
        _ocWizard->appendToResultWidget( tr("The remote folder %1 already exists. Connecting it for syncing.").arg(_remoteFolder));
    } else if( error > 202 && error < 300 ) {
        _ocWizard->appendToResultWidget( tr("The folder creation resulted in HTTP error code %1").arg((int)error) );
    } else if( error == QNetworkReply::OperationCanceledError ) {
        _ocWizard->appendToResultWidget( tr("<p><font color=\"red\">Remote folder creation failed probably because the provided credentials are wrong.</font>"
                                            "<br/>Please go back and check your credentials.</p>"));
        _localFolder.clear();
        _remoteFolder.clear();
        success = false;
    } else {
        _ocWizard->appendToResultWidget( tr("Remote folder %1 creation failed with error <tt>%2</tt>.").arg(_remoteFolder).arg(error));
        _localFolder.clear();
        _remoteFolder.clear();
        success = false;
    }

    finalizeSetup( success );
}
#endif
void OwncloudSetupWizard::finalizeSetup( bool success )
{

    if( success ) {
        if( !(_localFolder.isEmpty() || _remoteFolder.isEmpty() )) {
            _ocWizard->appendToResultWidget( tr("A sync connection from %1 to remote directory %2 was set up.")
                                             .arg(_localFolder).arg(_remoteFolder));
        }
        _ocWizard->appendToResultWidget( QLatin1String(" "));
        _ocWizard->appendToResultWidget( QLatin1String("<p><font color=\"green\"><b>")
                                         + tr("Successfully connected to %1!")
                                         .arg(Theme::instance()->appNameGUI())
                                         + QLatin1String("</b></font></p>"));
        _ocWizard->appendToResultWidget( tr("Press Finish to permanently accept this connection."));

        // Go to result page.

    } else {
        _ocWizard->appendToResultWidget(QLatin1String("<p><font color=\"red\">")
                                        + tr("Connection to %1 could not be established. Please check again.")
                                        .arg(Theme::instance()->appNameGUI())
                                        + QLatin1String("</font></p>"));
    }
    _ocWizard->successfullyConnected(success);
}

}
