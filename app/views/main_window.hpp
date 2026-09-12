#pragma once

#include <QMainWindow>

#include <chrono>
#include <functional>
#include <string>

#include "carnet/database.hpp"

class QTabWidget;

namespace carnet {

class MonthPicker;
class OverviewTab;
class TransactionsTab;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(Database& db, QWidget* parent = nullptr);

    std::chrono::year_month month() const;
    bool exportSummary(const QString& path);
    bool exportTransactions(const QString& path);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void restoreSettings();
    void openImport();
    void openRules();
    void openAbout();
    void addShortcut(const char* name, const QKeySequence& keys, const std::function<void()>& run);
    void askExport(const QString& kind, bool (MainWindow::*write)(const QString&));
    bool writeFile(const QString& path, const std::string& content);

    Database& db_;
    MonthPicker* monthPicker_;
    QTabWidget* tabs_;
    OverviewTab* overviewTab_;
    TransactionsTab* transactionsTab_;
};

}
