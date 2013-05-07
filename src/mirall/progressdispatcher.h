#ifndef PROGRESSDISPATCHER_H
#define PROGRESSDISPATCHER_H

#include <QObject>

namespace Mirall {


/**
 * @brief The FolderScheduler class schedules folders for sync
 */
class ProgressDispatcher : public QObject
{
    Q_OBJECT
public:
    static ProgressDispatcher* instance();
    ~ProgressDispatcher();

public:
    void setFolderUploadProgress( const QString&, const QString&, long, long );

signals:
    void folderUploadProgress( const QString&, const QString&, long, long );

public slots:
    
private:
    ProgressDispatcher(QObject* parent = 0);

    static ProgressDispatcher* _instance;
};

}
#endif // PROGRESSDISPATCHER_H
