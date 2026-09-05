#include <QApplication>
#include <QDir>
#include <QMessageBox>

#include <filesystem>
#include <optional>

#include "app_paths.hpp"
#include "carnet/database.hpp"
#include "views/main_window.hpp"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("Carnet");

    std::optional<carnet::Database> db;
    try {
        const QString file = QDir(carnet::dataDirectory()).filePath("carnet.db");
        db = carnet::Database::open(std::filesystem::path(file.toStdWString()));
    } catch (const carnet::DatabaseError& error) {
        QMessageBox::critical(nullptr, QObject::tr("Carnet"), QString::fromUtf8(error.what()));
        return 1;
    }

    carnet::MainWindow window(*db);
    window.show();
    return app.exec();
}
