#include <QLabel>
#include <QScrollArea>
#include <QScrollBar>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>

#include <chrono>
#include <filesystem>
#include <optional>

#include "carnet/database.hpp"
#include "views/overview_tab.hpp"
#include "widgets/chart_colors.hpp"

using namespace std::chrono;
using carnet::OverviewTab;

class OverviewTabTest : public QObject {
    Q_OBJECT

private slots:
    void init();
    void showsTotals();
    void negativeLeftIsRed();
    void reloadFollowsDatabase();
    void noScrollbarsForFewCategories();

private:
    QString text(OverviewTab& tab, const char* name) const {
        return tab.findChild<QLabel*>(name)->text();
    }

    QTemporaryDir dir_;
    std::optional<carnet::Database> db_;
};

void OverviewTabTest::init() {
    db_.reset();
    const std::filesystem::path path(dir_.filePath("carnet.db").toStdWString());
    std::filesystem::remove(path);
    db_ = carnet::Database::open(path);
    db_->importTransactions("Current", {carnet::TxnDraft{2026y / March / 2, 215000, "Salary", "", 1},
                                        carnet::TxnDraft{2026y / March / 5, -32050, "Rent part", "", 1}});
}

void OverviewTabTest::showsTotals() {
    OverviewTab tab(*db_);
    tab.setMonth(2026y / March);
    const QString euro(QChar(0x20AC));
    QCOMPARE(text(tab, "income"), QString("2 150,00 ") + euro);
    QCOMPARE(text(tab, "spent"), QString("320,50 ") + euro);
    QCOMPARE(text(tab, "left"), QString("1 829,50 ") + euro);
}

void OverviewTabTest::negativeLeftIsRed() {
    OverviewTab tab(*db_);
    tab.setMonth(2026y / March);
    QVERIFY(tab.findChild<QLabel*>("left")->palette().color(QPalette::WindowText) !=
            carnet::overBudgetColor());
    db_->importTransactions("Current", {carnet::TxnDraft{2026y / March / 9, -300000, "Car", "", 1}});
    tab.reload();
    QVERIFY(text(tab, "left").startsWith("-"));
    QCOMPARE(tab.findChild<QLabel*>("left")->palette().color(QPalette::WindowText),
             carnet::overBudgetColor());
}

void OverviewTabTest::reloadFollowsDatabase() {
    OverviewTab tab(*db_);
    tab.setMonth(2026y / March);
    for (const carnet::Category& c : db_->categories()) {
        if (c.name == "Transfers") {
            db_->setCategory(db_->transactionsInMonth(2026y / March)[1].id, c.id);
        }
    }
    tab.reload();
    QCOMPARE(text(tab, "spent"), QString("0,00 ") + QChar(0x20AC));
}

void OverviewTabTest::noScrollbarsForFewCategories() {
    // the real window loads the Overview while another tab is showing, then switches to it
    QTabWidget tabs;
    auto* overview = new OverviewTab(*db_);
    overview->setMonth(2026y / February);
    tabs.addTab(overview, "Overview");
    tabs.addTab(new QWidget, "Other");
    tabs.setCurrentIndex(1);
    tabs.resize(1400, 900);
    tabs.show();
    QCoreApplication::processEvents();
    overview->setMonth(2026y / March);
    QCoreApplication::processEvents();
    tabs.grab();
    tabs.setCurrentIndex(0);
    QCoreApplication::processEvents();
    auto* area = overview->findChild<QScrollArea*>();
    QVERIFY(!area->verticalScrollBar()->isVisible());
    QVERIFY(!area->horizontalScrollBar()->isVisible());
}

QTEST_MAIN(OverviewTabTest)
#include "overview_tab_test.moc"
