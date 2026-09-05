#include "app_paths.hpp"

#include <QDir>
#include <QStandardPaths>

namespace carnet {

QString dataDirectory() {
    const QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(path);
    return path;
}

QString settingsPath() {
    return QDir(dataDirectory()).filePath("carnet.ini");
}

}
