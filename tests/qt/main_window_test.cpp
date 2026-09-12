#include <QAction>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>

#include <chrono>
#include <filesystem>
#include <optional>

#include "app_paths.hpp"
#include "carnet/database.hpp"
#include "carnet/export.hpp"
#include "views/main_window.hpp"
#include "widgets/month_picker.hpp"

using namespace std::chrono;
using carnet::MainWindow;

class MainWindowTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void exportWritesSummary();
    void exportWritesTransactions();
    void exportFailureIsReported();
    void monthIsRemembered();
    void tabIsRemembered();
    void badSettingsFallBack();
    void shortcutsAreRegistered();
    void monthActionsMove();

private:
    static std::string read(const QString& path);
    static void showSeptember(MainWindow& window);

    QTemporaryDir dir_;
    std::optional<carnet::Database> db_;
};

void MainWindowTest::initTestCase() {
    // settings go to a test location, never to the real carnet.ini
    QStandardPaths::setTestModeEnabled(true);
}

void MainWindowTest::init() {
    QFile::remove(carnet::settingsPath());
    db_.reset();
    const std::filesystem::path path(dir_.filePath("carnet.db").toStdWString());
    std::filesystem::remove(path);
    db_ = carnet::Database::open(path);
    db_->importTransactions("Current",
                            {carnet::TxnDraft{2026y / September / 2, 215000, "Salary", "", 1},
                             carnet::TxnDraft{2026y / September / 5, -3250, "Bakery; corner", "", 1}});
}

std::string MainWindowTest::read(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return file.readAll().toStdString();
}

void MainWindowTest::showSeptember(MainWindow& window) {
    window.findChild<carnet::MonthPicker*>()->setMonth(2026y / September);
}

void MainWindowTest::exportWritesSummary() {
    MainWindow window(*db_);
    showSeptember(window);
    const QString path = dir_.filePath("summary.csv");
    QVERIFY(window.exportSummary(path));
    QCOMPARE(read(path),
             carnet::summaryCsv(2026y / September, db_->monthOverview(2026y / September), "Uncategorized"));
}

void MainWindowTest::exportWritesTransactions() {
    MainWindow window(*db_);
    showSeptember(window);
    const QString path = dir_.filePath("transactions.csv");
    QVERIFY(window.exportTransactions(path));
    QCOMPARE(read(path), carnet::transactionsCsv(db_->transactionsInMonth(2026y / September),
                                                 db_->categories(), "Uncategorized"));
}

void MainWindowTest::exportFailureIsReported() {
    MainWindow window(*db_);
    showSeptember(window);
    const QString path = dir_.filePath("missing/folder/summary.csv");
    QVERIFY(!window.exportSummary(path));
    QVERIFY(!QFile::exists(path));
}

void MainWindowTest::monthIsRemembered() {
    {
        MainWindow window(*db_);
        window.findChild<carnet::MonthPicker*>()->setMonth(2025y / March);
        window.close();
    }
    MainWindow window(*db_);
    QCOMPARE(window.month(), 2025y / March);
}

void MainWindowTest::tabIsRemembered() {
    {
        MainWindow window(*db_);
        window.findChild<QTabWidget*>()->setCurrentIndex(0);
        window.close();
    }
    MainWindow window(*db_);
    QCOMPARE(window.findChild<QTabWidget*>()->currentIndex(), 0);
}

void MainWindowTest::badSettingsFallBack() {
    {
        QSettings settings(carnet::settingsPath(), QSettings::IniFormat);
        settings.setValue("window/month", "garbage");
        settings.setValue("window/tab", 7);
    }
    const year_month_day today{floor<days>(system_clock::now())};
    MainWindow window(*db_);
    QCOMPARE(window.month(), today.year() / today.month());
    QCOMPARE(window.findChild<QTabWidget*>()->currentIndex(), 1);
}

void MainWindowTest::shortcutsAreRegistered() {
    MainWindow window(*db_);
    const std::vector<std::pair<const char*, QKeySequence>> expected{
        {"previousMonth", QKeySequence(Qt::CTRL | Qt::Key_PageUp)},
        {"nextMonth", QKeySequence(Qt::CTRL | Qt::Key_PageDown)},
        {"currentMonth", QKeySequence(Qt::CTRL | Qt::Key_Home)},
        {"import", QKeySequence(Qt::CTRL | Qt::Key_I)},
        {"rules", QKeySequence(Qt::CTRL | Qt::Key_R)},
        {"export", QKeySequence(Qt::CTRL | Qt::Key_E)},
        {"overviewTab", QKeySequence(Qt::CTRL | Qt::Key_1)},
        {"transactionsTab", QKeySequence(Qt::CTRL | Qt::Key_2)},
        {"about", QKeySequence(Qt::Key_F1)},
    };
    for (const auto& [name, keys] : expected) {
        auto* action = window.findChild<QAction*>(name);
        QVERIFY2(action != nullptr, name);
        QCOMPARE(action->shortcut(), keys);
    }
}

void MainWindowTest::monthActionsMove() {
    MainWindow window(*db_);
    showSeptember(window);
    window.findChild<QAction*>("nextMonth")->trigger();
    QCOMPARE(window.month(), 2026y / October);
    window.findChild<QAction*>("previousMonth")->trigger();
    window.findChild<QAction*>("previousMonth")->trigger();
    QCOMPARE(window.month(), 2026y / August);
    window.findChild<QAction*>("currentMonth")->trigger();
    const year_month_day today{floor<days>(system_clock::now())};
    QCOMPARE(window.month(), today.year() / today.month());
    window.findChild<QAction*>("overviewTab")->trigger();
    QCOMPARE(window.findChild<QTabWidget*>()->currentIndex(), 0);
}

QTEST_MAIN(MainWindowTest)
#include "main_window_test.moc"
