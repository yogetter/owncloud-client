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

#include <QThread>
#include <QDebug>

#include "mirall/folderwatcher.h"
#include "mirall/folderwatcher_win.h"

#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

namespace Mirall {

void WatcherThread::run()
{
#if 0
    _handle = FindFirstChangeNotification((wchar_t*)_path.utf16(),
                                         true, // recursive watch
                                         FILE_NOTIFY_CHANGE_FILE_NAME |
                                         FILE_NOTIFY_CHANGE_DIR_NAME |
                                         FILE_NOTIFY_CHANGE_LAST_WRITE);
#else
    // USES_CONVERSION;
    _handle = CreateFile(
      (wchar_t*)_path.utf16(), /* pointer to the file name */
      FILE_LIST_DIRECTORY,                /* access (read-write) mode */
      FILE_SHARE_READ|FILE_SHARE_DELETE,  /* share mode */
      NULL, /* security descriptor */
      OPEN_EXISTING, /* how to create */
      FILE_FLAG_BACKUP_SEMANTICS, /* file attributes */
      NULL /* file with attributes to copy */
    );
#endif
    if (_handle == INVALID_HANDLE_VALUE)
    {
        qDebug() << Q_FUNC_INFO << "FindFirstChangeNotification function failed, stopping watcher!";
        FindCloseChangeNotification(_handle);
        _handle = 0;
        return;
    }

    if (_handle == NULL)
    {
        qDebug() << Q_FUNC_INFO << "FindFirstChangeNotification returned null, stopping watcher!";
        FindCloseChangeNotification(_handle);
        _handle = 0;
        return;
    }

    while(true) {
#if 0
        switch(WaitForSingleObject(_handle, /*wait*/ INFINITE)) {
        case WAIT_OBJECT_0:
            if (FindNextChangeNotification(_handle) == false) {
                qDebug() << Q_FUNC_INFO << "FindFirstChangeNotification returned FALSE, stopping watcher!";
                FindCloseChangeNotification(_handle);
                _handle = 0;
                return;
            }
            // qDebug() << Q_FUNC_INFO << "Change detected in" << _path << "from" << QThread::currentThread    ();
            emit changed(_path);
            break;
        default:
            qDebug()  << Q_FUNC_INFO << "Error while watching";
        }
#else
        FILE_NOTIFY_INFORMATION Buffer[32*1024];
        DWORD BytesReturned;
        while( ReadDirectoryChangesW(
                   _handle, /* handle to directory */
                   &Buffer, /* read results buffer */
                   sizeof(Buffer), /* length of buffer */
                   TRUE, /* monitoring option */
                   FILE_NOTIFY_CHANGE_SECURITY|
                   FILE_NOTIFY_CHANGE_CREATION|
                   FILE_NOTIFY_CHANGE_LAST_ACCESS|
                   FILE_NOTIFY_CHANGE_LAST_WRITE|
                   FILE_NOTIFY_CHANGE_SIZE|
                   FILE_NOTIFY_CHANGE_ATTRIBUTES|
                   FILE_NOTIFY_CHANGE_DIR_NAME|
                   FILE_NOTIFY_CHANGE_FILE_NAME, /* filter conditions */
                   &BytesReturned, /* bytes returned */
                   NULL, /* overlapped buffer */
                   NULL)) {
            qDebug () << "PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP";
        }
#endif
    }
}

WatcherThread::~WatcherThread()
{
    if (_handle)
        FindCloseChangeNotification(_handle);
}

FolderWatcherPrivate::FolderWatcherPrivate(FolderWatcher *p, const QString& path)
    : _parent(p)
{
    _thread = new WatcherThread(path);
    connect(_thread, SIGNAL(changed(const QString&)),
            _parent,SLOT(changeDetected(const QString&)));
    _thread->start();
}

FolderWatcherPrivate::~FolderWatcherPrivate()
{
    _thread->terminate();
    _thread->wait();
    delete _thread;
}

} // namespace Mirall
