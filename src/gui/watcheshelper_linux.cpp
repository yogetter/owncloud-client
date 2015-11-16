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

#include <QProcess>
#include <QTemporaryFile>
#include <QMessageBox>

#include "watcheshelper_linux.h"
#include "theme.h"
#include "logger.h"

#define xstr(s) str(s)
#define str(s) #s
#define MAX_WATCHES_VALUE 65536

namespace OCC {

WatchesHelper::WatchesHelper() :
    QObject(),
    _ran(false)
{

}

WatchesHelper::~WatchesHelper()
{
}

void WatchesHelper::slotTryIncreaseUserWatchesLimit()
{
    // If it doesn't succeed the first time around, don't re-attempt. This includes other folders asking.
    if (_ran) {
        return;
    }

    QMessageBox::information(0, tr("Insufficient watches"),
                             tr("%1 cannot watch all folders for changes and will attempt to fix the problem.\n"
                                "This requires root privileges. Please follow the instructions.")
                             .arg(Theme::instance()->appNameGUI()), QMessageBox::Ok);
    increaseUserWatchesLimit();
    if (readUserWatchesLimit() < MAX_WATCHES_VALUE ) {
        Logger::instance()->postGuiLog(Theme::instance()->appNameGUI(), tr("Failed to resolve problem. Not all folders will be watched for changes."));
    } else {
        Logger::instance()->postGuiLog(Theme::instance()->appNameGUI(), tr("Successfully resolved the problem."));
        emit watchesIncreasingSuccess();
    }
    _ran = true;
}

int WatchesHelper::readUserWatchesLimit()
{
    QFile maxUserWatches("/proc/sys/fs/inotify/max_user_watches");
    if (!maxUserWatches.open(QIODevice::ReadOnly)) {
        return -1;
    }
    return maxUserWatches.readAll().toInt();
}

void WatchesHelper::increaseUserWatchesLimit()
{
    QTemporaryFile bashScript;
    bashScript.open();
    QByteArray content(
                "#!/bin/bash\n\n"
                "target=/etc/sysconf.d/30-owncloud.conf"
                "new_max_watches=" str(MAX_WATCHES_VALUE)
                "echo $new_max_watches > /proc/sys/fs/inotify/max_user_watches\n\n"
                "echo \\# Increase inotify availability >> $target\n"
                "echo fs.inotify.max_user_watches = $new_max_watches >> $target\n\n"

                );
    bashScript.setPermissions(bashScript.permissions() |
                              QFileDevice::ExeUser|QFileDevice::ExeGroup|QFileDevice::ExeOther);
    bashScript.write(content);
    QProcess runScript;
    QStringList args;
    args << bashScript.fileName();
    runScript.execute(sudoBinary(), args);
}

QByteArray WatchesHelper::sudoBinary()
{
    QProcess proc;

    static QByteArray programs[] = { "pkexec", "gtksu", "kdesu" };

    for(const QByteArray program : programs) {
        if (proc.execute("which", QStringList(QString::fromAscii(program))) == 0) {
            QByteArray path = proc.readAll();
            path.chop(1);
            return path;
        }
    }
    return "";
}

} // namespace OCC
