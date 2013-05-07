#include "progressdispatcher.h"

namespace Mirall {

ProgressDispatcher* ProgressDispatcher::_instance = 0;

ProgressDispatcher* ProgressDispatcher::instance() {
    if (!_instance) {
        _instance = new ProgressDispatcher();
    }
    return _instance;
}

ProgressDispatcher::ProgressDispatcher(QObject *parent) :
    QObject(parent)
{

}

ProgressDispatcher::~ProgressDispatcher()
{

}

void ProgressDispatcher::setFolderUploadProgress( const QString& alias, const QString& folder, long p1, long p2)
{
    emit folderUploadProgress( alias, folder, p1, p2 );
}

}
