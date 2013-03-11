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

#include "owncloudconnection.h"

#include <QSettings>

#include "mirall/mirallconfigfile.h"
#include "mirall/folderman.h"


namespace Mirall {

OwncloudConnectionManager *OwncloudConnectionManager::_instance = 0;


OwncloudConnection::OwncloudConnection(QObject *parent) :
    QObject(parent)
{
}

void OwncloudConnection::setUrl(const QUrl &url) const
{
    _url = url;
}

void OwncloudConnection::setCredentials(const QString &user, const QString &password)
{
    _user = user;
    _password = password;
}

OwncloudConnectionManager *OwncloudConnectionManager::instance()
{
    if (!_instance)
        _instance = new OwncloudConnectionManager();

    return _instance;
}

OwncloudConnection *OwncloudConnectionManager::addConnection(const QUrl &url, const QString &user, const QString &password)
{
    OwncloudConnection *conn = new OwncloudConnection;
    conn->setUrl(url);
    conn->setCredentials(user, password);
    _connections.insert(conn);
    return conn;
}

void OwncloudConnectionManager::removeConnection(OwncloudConnection *connection)
{
    if (!_connections.contains(connection))
        return;

    _connections.remove(connection);
    connection->deleteLater();
}

void OwncloudConnectionManager::saveConnections()
{

}

void OwncloudConnectionManager::loadConnections()
{
}

void OwncloudConnection::addFolder(Folder *folder)
{
    _folders.add(folder);
}

void OwncloudConnection::removeFolder(Folder *folder)
{
    _folders.remove(folder);
}

bool OwncloudConnection::isOnline() const
{
    return _isOnline;
}

QUrl OwncloudConnection::webdavUrl() const
{
    QUrl webdav = _url;
    QString path = webdav.path() + QLatin1String('/remote.php/webdav');
    webdav.setPath(path);
    return webdav;
}

} // namespace Mirall
