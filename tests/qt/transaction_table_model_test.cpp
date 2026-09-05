#include <QBrush>
#include <QFont>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <chrono>
#include <filesystem>
#include <optional>

#include "carnet/database.hpp"
#include "models/transaction_table_model.hpp"

using namespace std::chrono;
using carnet::TransactionTableModel;

class TransactionTableModelTest : public QObject {
    Q_OBJECT

private slots:
    void init();
    void loadsMonth();
    void showsFormattedColumns();
    void colorsIncomeGreen();
    void sortsByRawValues();
    void setCategoryPersists();
    void automaticHandsBack();
    void noCategorySticks();
    void setDataReportsDatabaseErrors();

private:
    std::filesystem::path dbPath() const;
    std::int64_t category(const std::string& name) const;
    void reopen();

    QTemporaryDir dir_;
    std::optional<carnet::Database> db_;
};

std::filesystem::path TransactionTableModelTest::dbPath() const {
    return std::filesystem::path(dir_.filePath("carnet.db").toStdWString());
}

std::int64_t TransactionTableModelTest::category(const std::string& name) const {
    for (const carnet::Category& c : db_->categories()) {
        if (c.name == name) {
            return c.id;
        }
    }
    QTest::qFail("no such category", __FILE__, __LINE__);
    return 0;
}

void TransactionTableModelTest::reopen() {
    db_.reset();
    db_ = carnet::Database::open(dbPath());
}

void TransactionTableModelTest::init() {
    db_.reset();
    std::filesystem::remove(dbPath());
    db_ = carnet::Database::open(dbPath());
    db_->addRule("coffee", category("Eating out"));
    db_->importTransactions("Current",
                            {carnet::TxnDraft{2026y / March / 5, -320, "Coffee", "BAKERY DUPONT", 1},
                             carnet::TxnDraft{2026y / March / 2, 215000, "Salary March", "ACME SPRL", 1},
                             carnet::TxnDraft{2026y / April / 1, -85000, "Rent April", "HOME", 1}});
}

void TransactionTableModelTest::loadsMonth() {
    TransactionTableModel model(*db_);
    model.setMonth(2026y / March);
    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.columnCount(), int(TransactionTableModel::ColumnCount));
}

void TransactionTableModelTest::showsFormattedColumns() {
    TransactionTableModel model(*db_);
    model.setMonth(2026y / March);
    QCOMPARE(model.index(0, TransactionTableModel::DateColumn).data().toString(), QString("02/03/2026"));
    QCOMPARE(model.index(0, TransactionTableModel::AccountColumn).data().toString(), QString("Current"));
    QCOMPARE(model.index(0, TransactionTableModel::DescriptionColumn).data().toString(),
             QString("Salary March"));
    QCOMPARE(model.index(0, TransactionTableModel::CounterpartyColumn).data().toString(),
             QString("ACME SPRL"));
    QCOMPARE(model.index(0, TransactionTableModel::AmountColumn).data().toString(),
             QString("2 150,00 ") + QChar(0x20AC));
    QCOMPARE(model.index(1, TransactionTableModel::CategoryColumn).data().toString(), QString("Eating out"));
    QCOMPARE(model.index(0, TransactionTableModel::CategoryColumn).data().toString(), QString());
}

void TransactionTableModelTest::colorsIncomeGreen() {
    TransactionTableModel model(*db_);
    model.setMonth(2026y / March);
    const QVariant income = model.index(0, TransactionTableModel::AmountColumn).data(Qt::ForegroundRole);
    QCOMPARE(income.value<QBrush>().color(), QColor(0x1b, 0x7f, 0x3a));
    QVERIFY(!model.index(1, TransactionTableModel::AmountColumn).data(Qt::ForegroundRole).isValid());
}

void TransactionTableModelTest::sortsByRawValues() {
    TransactionTableModel model(*db_);
    model.setMonth(2026y / March);
    QCOMPARE(model.index(1, TransactionTableModel::AmountColumn).data(Qt::UserRole).toLongLong(),
             qlonglong(-320));
    QCOMPARE(model.index(0, TransactionTableModel::DateColumn).data(Qt::UserRole).toString(),
             QString("2026-03-02"));
}

void TransactionTableModelTest::setCategoryPersists() {
    const auto leisure = category("Leisure");
    {
        TransactionTableModel model(*db_);
        model.setMonth(2026y / March);
        QVERIFY(model.setData(model.index(1, TransactionTableModel::CategoryColumn), qint64(leisure)));
    }
    reopen();
    TransactionTableModel model(*db_);
    model.setMonth(2026y / March);
    const QModelIndex cell = model.index(1, TransactionTableModel::CategoryColumn);
    QCOMPARE(cell.data().toString(), QString("Leisure"));
    QCOMPARE(cell.data(Qt::EditRole).toLongLong(), leisure);
    QVERIFY(cell.data(Qt::FontRole).value<QFont>().italic());
}

void TransactionTableModelTest::automaticHandsBack() {
    TransactionTableModel model(*db_);
    model.setMonth(2026y / March);
    const QModelIndex cell = model.index(1, TransactionTableModel::CategoryColumn);
    QVERIFY(model.setData(cell, qint64(category("Leisure"))));
    QVERIFY(model.setData(cell, TransactionTableModel::automaticChoice));
    QCOMPARE(cell.data().toString(), QString("Eating out"));
    QCOMPARE(cell.data(Qt::EditRole).toLongLong(), TransactionTableModel::automaticChoice);
    QVERIFY(!cell.data(Qt::FontRole).isValid());
}

void TransactionTableModelTest::noCategorySticks() {
    TransactionTableModel model(*db_);
    model.setMonth(2026y / March);
    const QModelIndex cell = model.index(1, TransactionTableModel::CategoryColumn);
    QVERIFY(model.setData(cell, TransactionTableModel::noCategoryChoice));
    db_->applyRules();
    model.reload();
    QCOMPARE(model.index(1, TransactionTableModel::CategoryColumn).data().toString(), QString());
    QCOMPARE(model.index(1, TransactionTableModel::CategoryColumn).data(Qt::EditRole).toLongLong(),
             TransactionTableModel::noCategoryChoice);
}

void TransactionTableModelTest::setDataReportsDatabaseErrors() {
    TransactionTableModel model(*db_);
    model.setMonth(2026y / March);
    QSignalSpy errors(&model, &TransactionTableModel::errorOccurred);
    QVERIFY(!model.setData(model.index(1, TransactionTableModel::CategoryColumn), qint64(999)));
    QCOMPARE(errors.count(), 1);
}

QTEST_MAIN(TransactionTableModelTest)
#include "transaction_table_model_test.moc"
