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

#include <QDebug>
#include <QUrl>
#include <QLocalSocket>
#include <QMetaObject>
#include <QStringList>

using namespace Mirall;

SocketApi::SocketApi(QObject* parent, const QUrl& localFile)
    : QObject(parent)
    , _localServer(0)
{
    qDebug() << Q_FUNC_INFO << localFile.toLocalFile();

    _localServer = new QLocalServer(this);
    if(!_localServer->listen( "ownCloud" ))
        qDebug() << "cant start server";
    else
        qDebug() << "server started";

    connect(_localServer, SIGNAL(newConnection()), this, SLOT(onNewConnection()));
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

void SocketApi::sendMessage(QLocalSocket* socket, const QString& message)
{
    qDebug() << Q_FUNC_INFO << message;
    QString localMessage = message;
    socket->write(localMessage.append("\n").toUtf8());
}

void SocketApi::command_ONLINELINK(const QString& argument, QLocalSocket* socket)
{
    qDebug() << "copy online link to clipboard: " << argument;
}
