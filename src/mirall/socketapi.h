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


#ifndef SOCKETAPI_H
#define SOCKETAPI_H

#include <attica/provider.h>

#include <QLocalServer>

namespace Attica {
    class ProviderManager;
}

class QUrl;
class QSocket;
class QStringList;

namespace Mirall {

class FolderMan;

class SocketApi : public QObject
{
Q_OBJECT

public:
    SocketApi(QObject* parent, const QUrl& localFile, FolderMan* folderMan);
    virtual ~SocketApi();

private slots:
    void onNewConnection();
    void onReadyRead();

    void onProviderAdded(const Attica::Provider& provider);

    void onGotPublicShareLink(Attica::BaseJob*);

private:
    void sendMessage(QLocalSocket* socket, const QString& message);

    Q_INVOKABLE void command_RETRIEVE_STATUS(const QString& argument, QLocalSocket* socket);
    Q_INVOKABLE void command_PUBLIC_SHARE_LINK(const QString& argument, QLocalSocket* socket);

private:
    QLocalServer* _localServer;

    Attica::ProviderManager* _atticaManager;
    Attica::Provider _atticaProvider;

    FolderMan* _folderMan;
};

}
#endif // SOCKETAPI_H
