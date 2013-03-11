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

#ifndef OWNCLOUDCONNECTION_H
#define OWNCLOUDCONNECTION_H

namespace Mirall {

class Folder;

#include <QObject>
#include <QSet>
#include <QUrl>

class OwncloudConnection;

class OwncloudConnectionManager : public QObject
{
    Q_OBJECT
public:
    OwncloudConnectionManager *instance();

    OwncloudConnection* addConnection(const QUrl &url, const QString &user, const QString &password);
    void removeConnection(OwncloudConnection *connection);
    QSet<OwncloudConnection *> connections() { return _connections; }
    void saveConnections();
    void loadConnections();

    // persist connections here?

private:
    OwncloudConnectionManager();
    QSet<OwncloudConnection *> _connections;
};

class OwncloudConnection : public QObject
{
    Q_OBJECT
public:
    QSet<Folder*> folders() const;
    void addFolder(Folder *folder);
    void removeFolder(Folder *folder);

    bool isOnline() const;
    QUrl webdavUrl() const;

protected:
    explicit OwncloudConnection(QObject *parent = 0);
    void setUrl(const QUrl &url) const;
    void setCredentials(const QString &user, const QString &password);

signals:
    void onlineStateChanged(bool online);

private:
    friend class OwncloudConnectionManager;
    QUrl _url;
    QSet<Folder*> _folders;
    bool _isOnline;
    QString _user;
    QString _password;
};

} // namespace Mirall

#endif // OWNCLOUDCONNECTION_H
