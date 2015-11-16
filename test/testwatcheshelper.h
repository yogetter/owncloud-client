/*
 *    This software is in the public domain, furnished "as is", without technical
 *       support, and with no warranty, express or implied, as to its usefulness for
 *          any purpose.
 *          */

#ifndef MIRALL_INOTIFYWATCHER_H
#define MIRALL_INOTIFYWATCHER_H

#include <QtTest>

#include "watcheshelper_linux.h"

using namespace OCC;

class TestWatchesHelper : public QObject
{
    Q_OBJECT
private:
    WatchesHelper _helper;

private slots:
    void initTestCase() {
        _helper = new WatchesHelper;
    }

    // Test the recursive path listing function findFoldersBelow
    void testSudoBinary() {
        QVERIFY(!_helper->sudoBinary().isEmpty());
    }

    void testReadUsersWatchLimit() {
        QVERIFY(_helper->readUserWatchesLimit() > 0);
    }

    void cleanupTestCase() {
        delete _helper;
        _helper  = 0;
    }
};

#endif
