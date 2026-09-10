#pragma once

#include <QMainWindow>

#include "carnet/database.hpp"

namespace carnet {

class MonthPicker;
class OverviewTab;
class TransactionsTab;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(Database& db, QWidget* parent = nullptr);

private:
    void openImport();
    void openRules();

    Database& db_;
    MonthPicker* monthPicker_;
    OverviewTab* overviewTab_;
    TransactionsTab* transactionsTab_;
};

}
