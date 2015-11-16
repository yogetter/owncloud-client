/*
 * Copyright (C) by Daniel Molkentin <danimO@owncloud.com>
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

#ifndef MIRALL_WATCHESHELPER_LINUX_H
#define MIRALL_WATCHESHELPER_LINUX_H

#include <QProcess>
#include <QTemporaryFile>
#include <QMessageBox>

namespace OCC {

/**
 * @brief Helps with incrasing inotify watches
 *
 * This class is an implementation detail of
 * the Linux folder watcher.
 */
class WatchesHelper : public QObject {
    Q_OBJECT
public:
    WatchesHelper();
    ~WatchesHelper();

public slots:
    void slotTryIncreaseUserWatchesLimit();

signals:
    void watchesIncreasingSuccess();

private:
    int readUserWatchesLimit();
    void increaseUserWatchesLimit();
    QByteArray sudoBinary();
    bool _ran;
};

} // namespace OCC

#endif //MIRALL_WATCHESHELPER_LINUX_H
