/*
 * Copyright (C) by Dominik Schmidt <domme@tomahawk-player.org>
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


#include "socketapi.h"
#include "mirallconfigfile.h"
#include "folderman.h"
#include "folder.h"
#include "owncloudfolder.h"
#include "socketapi/publicsharedialog.h"

#include <attica/providermanager.h>

#include <QDebug>
#include <QUrl>
#include <QLocalSocket>
#include <QMetaObject>
#include <QStringList>
#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <QClipboard>
#include <QApplication>

using namespace Mirall;

SocketApi::SocketApi(QObject* parent, const QUrl& localFile, FolderMan* folderMan)
    : QObject(parent)
    , _localServer(0)
    , _folderMan(folderMan)
{
    qDebug() << Q_FUNC_INFO << localFile.toLocalFile();

    // setup socket
    _localServer = new QLocalServer(this);
    if(!_localServer->listen( "ownCloud" ))
        qDebug() << "cant start server";
    else
        qDebug() << "server started";
    connect(_localServer, SIGNAL(newConnection()), this, SLOT(onNewConnection()));

    // setup attica
    QString tmp(QLatin1String("%1ocs/providers.php"));
    QUrl providerFile(tmp.arg(MirallConfigFile().ownCloudUrl()));

    _atticaManager = new Attica::ProviderManager();
    qDebug() << Q_FUNC_INFO << "Add provider file: " << providerFile;
    _atticaManager->addProviderFile(providerFile);
    connect(_atticaManager, SIGNAL(providerAdded(Attica::Provider)), SLOT(onProviderAdded(Attica::Provider)));


    // folder watcher
    connect(_folderMan, SIGNAL(folderSyncStateChange(QString)), SLOT(onSyncStateChanged(QString)));
}

SocketApi::~SocketApi()
{
    qDebug() << Q_FUNC_INFO;
    _localServer->close();
}

void SocketApi::onNewConnection()
{
    qDebug() << Q_FUNC_INFO;
    QLocalSocket* socket = _localServer->nextPendingConnection();
    connect(socket, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    Q_ASSERT(socket->readAll().isEmpty());
    _listener.append(socket);
}

void SocketApi::onReadyRead()
{
    qDebug() << Q_FUNC_INFO;
    QLocalSocket* socket = qobject_cast<QLocalSocket*>(sender());
    Q_ASSERT(socket);

    while(socket->canReadLine())
    {
        QString line = QString(socket->readLine()).trimmed();
        QString command = line.split(":").first();
        QString function = QString(QLatin1String("command_")).append(command);

        QString functionWithArguments = function + QLatin1String("(QString,QLocalSocket*)");
//         qDebug() << "function with arguments" << functionWithArguments;
        int indexOfMethod = this->metaObject()->indexOfMethod(functionWithArguments.toAscii());
//         qDebug() << "index of method: " << indexOfMethod;

        if(indexOfMethod != -1)
        {
            QString argument = line.remove(0, command.length()+1).trimmed();
            QMetaObject::invokeMethod(this, function.toAscii(), Q_ARG(QString, argument), Q_ARG(QLocalSocket*, socket));
        }
        else
        {
            qDebug() << "WARNING! got invalid command over socket: " << command;
        }
    }
}

void SocketApi::onSyncStateChanged(const QString&)
{
    qDebug() << Q_FUNC_INFO;
//     QMessageBox::information(0, "Foo", "Sync state changed!");
    foreach(QLocalSocket* current, _listener)
    {
        sendMessage(current, "UPDATE_VIEW");
    }
}

void SocketApi::onProviderAdded(const Attica::Provider& provider)
{
    qDebug() << Q_FUNC_INFO << provider.name() << provider.baseUrl() << provider.isValid() << provider.isEnabled();
    _atticaProvider = provider;

    MirallConfigFile config;
    _atticaProvider.saveCredentials(config.ownCloudUser(), config.ownCloudPasswd());
}

void SocketApi::onGotPublicShareLink(Attica::BaseJob* job)
{
    Attica::ItemPostJob<Attica::Link>* itemJob = static_cast<Attica::ItemPostJob<Attica::Link>*>(job);
    QString infoText(QLatin1String("Public Share Link copied to clipboard: %1"));
    QString publicLink = itemJob->result().url().toString();

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(publicLink);

//     QMessageBox::information(0, "Copied", infoText.arg(itemJob->result().url().toString()));
}

void SocketApi::sendMessage(QLocalSocket* socket, const QString& message)
{
    qDebug() << Q_FUNC_INFO << message;
    QString localMessage = message;
    socket->write(localMessage.append("\n").toUtf8());
}


void SocketApi::command_RETRIEVE_STATUS(const QString& argument, QLocalSocket* socket)
{
    //TODO: do security checks?!
    ownCloudFolder* folder = qobject_cast< ownCloudFolder* >(_folderMan->folderForPath( argument ));
    // this can happen in offline mode e.g., nothing to worry about
    if(!folder)
        return;

    QDir dir(argument);
    foreach(QString entry, dir.entryList())
    {
        QString absoluteFilePath = dir.absoluteFilePath(entry);
        QString statusString;
        SyncFileStatus fileStatus = folder->fileStatus(absoluteFilePath);
        switch(fileStatus)
        {
            case STATUS_NONE:
                statusString = QLatin1String("STATUS_NONE");
                break;
            case STATUS_EVAL:
                statusString = QLatin1String("STATUS_EVAL");
                break;
            case STATUS_REMOVE:
                statusString = QLatin1String("STATUS_REMOVE");
                break;
            case STATUS_RENAME:
                statusString = QLatin1String("STATUS_RENAME");
                break;
            case STATUS_NEW:
                statusString = QLatin1String("STATUS_NEW");
                break;
            case STATUS_CONFLICT:
                statusString = QLatin1String("STATUS_CONFLICT");
                break;
            case STATUS_IGNORE:
                statusString = QLatin1String("STATUS_IGNORE");
                break;
            case STATUS_SYNC:
                statusString = QLatin1String("STATUS_SYNC");
                break;
            case STATUS_STAT_ERROR:
                statusString = QLatin1String("STATUS_STAT_ERROR");
                break;
            case STATUS_ERROR:
                statusString = QLatin1String("STATUS_ERROR");
                break;
            case STATUS_UPDATED:
                statusString = QLatin1String("STATUS_UPDATED");
                break;
            default:
                qWarning() << "not all SyncFileStatus items checked!";
                Q_ASSERT(false);
                statusString = QLatin1String("STATUS_NONE");

        }
        qDebug() << "******************* FILESTATUS: " << statusString << (int)fileStatus;
        QString message("%1:%2:%3");
        message = message.arg("STATUS").arg(statusString).arg(absoluteFilePath);
        sendMessage(socket, message);
    }

    sendMessage(socket, "finished");
}

void SocketApi::command_PUBLIC_SHARE_LINK(const QString& argument, QLocalSocket* socket)
{
    qDebug() << "copy online link to clipboard: " << argument;

    if(!_atticaProvider.isEnabled()) {
        qWarning() << "Attica Provider is not enabled!";
        return;
    }


    ownCloudFolder* folder = qobject_cast< ownCloudFolder* >(_folderMan->folderForPath( argument ));
//     Q_ASSERT( folder );
//     if(!folder)
//         return;

    int lastChars = argument.length()-folder->path().length();
    QString remotePath;
    remotePath = argument.right(lastChars);
    remotePath.prepend("/");
    remotePath.prepend(folder->secondPath());

    qDebug() << "remote path: " << remotePath;
    _remotePath = remotePath;


    _publicShareDialog = new PublicShareDialog();
    connect(_publicShareDialog, SIGNAL(accepted()), SLOT(onPublicShareDialogAccepted()));
    connect(_publicShareDialog, SIGNAL(rejected()), SLOT(onPublicShareDialogRejected()));
    _publicShareDialog->show();
    _publicShareDialog->activateWindow();
    _publicShareDialog->raise();
}

void SocketApi::onPublicShareDialogAccepted()
{
    qDebug() << Q_FUNC_INFO << _publicShareDialog->password() << _publicShareDialog->validityLength();

    Attica::ItemPostJob<Attica::Link>* job = _atticaProvider.requestPublicShareLink(_remotePath);
    connect(job, SIGNAL(finished(Attica::BaseJob*)), SLOT(onGotPublicShareLink(Attica::BaseJob*)));
    job->start();

    delete _publicShareDialog;
}

void SocketApi::onPublicShareDialogRejected()
{
    qDebug() << Q_FUNC_INFO;

    delete _publicShareDialog;
}


