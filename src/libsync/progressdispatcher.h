/*
 * Copyright (C) by Klaas Freitag <freitag@owncloud.com>
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

#ifndef PROGRESSDISPATCHER_H
#define PROGRESSDISPATCHER_H

#include "owncloudlib.h"
#include <QObject>
#include <QHash>
#include <QTime>
#include <QQueue>
#include <QElapsedTimer>
#include <QTimer>
#include <QDebug>

#include "syncfileitem.h"

namespace OCC {

class ProgressInfo : public QObject
{
    Q_OBJECT
public:
    ProgressInfo()
        : _totalSizeOfCompletedJobs(0)
        , _maxFilesPerSecond(2.0)
        , _maxBytesPerSecond(100000.0)
    {}

    /**
     * Called when propagation starts.
     *
     * hasStarted() will return true afterwards.
     */
    void start()
    {
        connect(&_updateEstimatesTimer, SIGNAL(timeout()), SLOT(updateEstimates()));
        _updateEstimatesTimer.start(1000);
    }

    /**
     * Returns true when propagation has started (start() was called).
     *
     * This is used when the SyncEngine wants to indicate a new sync
     * is about to start via the transmissionProgress() signal. The
     * first ProgressInfo will have hasStarted() == false.
     */
    bool hasStarted() const
    {
        return _updateEstimatesTimer.isActive();
    }

    /**
     * Increase the file and size totals by the amount indicated in item.
     */
    void adjustTotalsForFile(const SyncFileItem & item)
    {
        if (!item._isDirectory) {
            _fileProgress._total++;
            if (isSizeDependent(item)) {
                _sizeProgress._total += item._size;
            }
        } else if (item._instruction != CSYNC_INSTRUCTION_NONE) {
            // Added or removed directories certainly count.
            _fileProgress._total++;
        }
    }

    /**
     * Adjust the total size by some value.
     *
     * Deprecated. Used only in the legacy propagator.
     */
    void adjustTotalSize(qint64 change)
    {
        _sizeProgress._total += change;
    }

    quint64 totalFiles() const
    {
        return _fileProgress._total;
    }

    quint64 completedFiles() const
    {
        return _fileProgress._completed;
    }

    quint64 currentFile() const
    {
        return completedFiles() + _currentItems.size();
    }

    quint64 totalSize() const
    {
        return _sizeProgress._total;
    }

    quint64 completedSize() const
    {
        return _sizeProgress._completed;
    }

    /** Return true is the size need to be taken in account in the total amount of time */
    static inline bool isSizeDependent(const SyncFileItem & item)
    {
        return ! item._isDirectory && (
               item._instruction == CSYNC_INSTRUCTION_CONFLICT
            || item._instruction == CSYNC_INSTRUCTION_SYNC
            || item._instruction == CSYNC_INSTRUCTION_NEW);
    }

    /**
     * Holds estimates about progress, returned to the user.
     */
    struct Estimates
    {
        /// Estimated completion amount per second. (of bytes or files)
        quint64 estimatedBandwidth;

        /// Estimated eta in milliseconds.
        quint64 estimatedEta;
    };

    /**
     * Holds the current state of something making progress and maintains an
     * estimate of the current progress per second.
     */
    struct Progress
    {
        Progress()
            : _progressPerSec(0)
            , _completed(0)
            , _prevCompleted(0)
            , _total(0)
        {
        }

        Estimates estimates() const
        {
            Estimates est;
            est.estimatedBandwidth = _progressPerSec;
            if (_progressPerSec != 0) {
                est.estimatedEta = (_total - _completed) / _progressPerSec * 1000.0;
            } else {
                est.estimatedEta = 0; // looks better than quint64 max
            }
            return est;
        }

        quint64 completed() const
        {
            return _completed;
        }

        quint64 remaining() const
        {
            return _total - _completed;
        }

    private:
        /**
         * Update the exponential moving average estimate of _progressPerSec.
         */
        void update()
        {
            // a good way to think about the smoothing factor:
            // If we make progress P per sec and then stop making progress at all,
            // after N calls to this function the _progressPerSec will have
            // reduced to P*smoothing^N.
            const double smoothing = 0.9; // after 30s, only 4% of the original value is left
            _progressPerSec = smoothing * _progressPerSec + (1.0 - smoothing) * (_completed - _prevCompleted);
            _prevCompleted = _completed;
        }

        double _progressPerSec;
        quint64 _completed;
        quint64 _prevCompleted;
        quint64 _total;

        friend class ProgressInfo;
    };

    struct ProgressItem
    {
        SyncFileItem _item;
        Progress _progress;
    };
    QHash<QString, ProgressItem> _currentItems;

    SyncFileItem _lastCompletedItem;

    // Used during local and remote update phase
    QString _currentDiscoveredFolder;

    void setProgressComplete(const SyncFileItem &item)
    {
        _currentItems.remove(item._file);
        _fileProgress._completed += item._affectedItems;
        if (ProgressInfo::isSizeDependent(item)) {
            _totalSizeOfCompletedJobs += item._size;
        }
        recomputeCompletedSize();
        _lastCompletedItem = item;
    }

    void setProgressItem(const SyncFileItem &item, quint64 size)
    {
        _currentItems[item._file]._item = item;
        _currentItems[item._file]._progress._completed = size;
        _currentItems[item._file]._progress._total = item._size;
        recomputeCompletedSize();

        // This seems dubious!
        _lastCompletedItem = SyncFileItem();
    }

    /**
     * Get the total completion estimate
     */
    Estimates totalProgress() const
    {
        Estimates file = _fileProgress.estimates();
        if (_sizeProgress._total == 0) {
            return file;
        }

        Estimates size = _sizeProgress.estimates();

        // If we have size information, we prefer an estimate based
        // on the upload speed. That's particularly relevant for large file
        // up/downloads.
        // However, when many *small* files are transfered, the estimate
        // can become very pessimistic as the transfered amount per second
        // drops.
        // So, if we detect a high rate of files per second, gradually prefer
        // a file-per-second estimate, where the remaining transfer is done
        // with the highest speed we've seen.
        quint64 combinedEta = file.estimatedEta + _sizeProgress.remaining() / _maxBytesPerSecond * 1000;
        double filesPerSec = _fileProgress._progressPerSec;
        if (filesPerSec > 5 && combinedEta < size.estimatedEta) {
            // value between 0 (fps==5) and 1 (fps==20)
            double scale = qMin((filesPerSec - 5.0) / 15.0, 1.0);
            size.estimatedEta = (1.0 - scale) * size.estimatedEta + scale * combinedEta;
        }
        return size;
    }

    /**
     * Get the current file completion estimate structure
     */
    Estimates fileProgress(const SyncFileItem &item) const
    {
        return _currentItems[item._file]._progress.estimates();
    }

private slots:
    /**
     * Called every second once started, this function updates the
     * estimates.
     */
    void updateEstimates()
    {
        _sizeProgress.update();
        _fileProgress.update();
        QMutableHashIterator<QString, ProgressItem> it(_currentItems);
        while (it.hasNext()) {
            it.next();
            it.value()._progress.update();
        }

        _maxFilesPerSecond = qMax(_fileProgress._progressPerSec,
                                  _maxFilesPerSecond);
        _maxBytesPerSecond = qMax(_sizeProgress._progressPerSec,
                                  _maxBytesPerSecond);
    }

private:
    // Sets the completed size by summing finished jobs with the progress
    // of active ones.
    void recomputeCompletedSize()
    {
        quint64 r = _totalSizeOfCompletedJobs;
        foreach(const ProgressItem &i, _currentItems) {
            if (isSizeDependent(i._item))
                r += i._progress._completed;
        }
        _sizeProgress._completed = r;
    }

    // Triggers the update() slot every second once propagation started.
    QTimer _updateEstimatesTimer;

    Progress _sizeProgress;
    Progress _fileProgress;

    // All size from completed jobs only.
    quint64 _totalSizeOfCompletedJobs;

    // The fastest observed rate of files per second in this sync.
    double _maxFilesPerSecond;
    double _maxBytesPerSecond;
};

namespace Progress {

    OWNCLOUDSYNC_EXPORT QString asActionString( const SyncFileItem& item );
    OWNCLOUDSYNC_EXPORT QString asResultString(  const SyncFileItem& item );

    OWNCLOUDSYNC_EXPORT bool isWarningKind( SyncFileItem::Status );
    OWNCLOUDSYNC_EXPORT bool isIgnoredKind( SyncFileItem::Status );

}

/**
 * @file progressdispatcher.h
 * @brief A singleton class to provide sync progress information to other gui classes.
 *
 * How to use the ProgressDispatcher:
 * Just connect to the two signals either to progress for every individual file
 * or the overall sync progress.
 *
 */
class OWNCLOUDSYNC_EXPORT ProgressDispatcher : public QObject
{
    Q_OBJECT

    friend class Folder; // only allow Folder class to access the setting slots.
public:
    static ProgressDispatcher* instance();
    ~ProgressDispatcher();

signals:
    /**
      @brief Signals the progress of data transmission.

      @param[out]  folder The folder which is being processed
      @param[out]  progress   A struct with all progress info.

     */
    void progressInfo( const QString& folder, const ProgressInfo& progress );
    /**
     * @brief: the item's job is completed
     */
    void jobCompleted(const QString &folder, const SyncFileItem & item);

    void syncItemDiscovered(const QString &folder, const SyncFileItem & item);

protected:
    void setProgressInfo(const QString& folder, const ProgressInfo& progress);

private:
    ProgressDispatcher(QObject* parent = 0);

    QElapsedTimer _timer;
    static ProgressDispatcher* _instance;
};

}
#endif // PROGRESSDISPATCHER_H
