#pragma once

#include <QMainWindow>

#include "carnet/database.hpp"

namespace carnet {

class MonthPicker;
class TransactionsTab;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(Database& db, QWidget* parent = nullptr);

private:
    void openImport();

    Database& db_;
    MonthPicker* monthPicker_;
    TransactionsTab* transactionsTab_;
};

}
