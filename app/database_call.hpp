#pragma once

#include <QMessageBox>
#include <QString>
#include <QWidget>

#include <utility>

#include "carnet/database.hpp"

namespace carnet {

// the one place where database failures turn into a dialog; nothing may throw through Qt's event loop
template <typename F> bool tryDatabase(QWidget* parent, F&& call) {
    try {
        std::forward<F>(call)();
        return true;
    } catch (const DatabaseError& error) {
        QMessageBox::critical(parent, QObject::tr("Carnet"), QString::fromUtf8(error.what()));
        return false;
    }
}

}
