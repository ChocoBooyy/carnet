#include <QBrush>
#include <QFont>
#include <QGuiApplication>
#include <QPalette>
#include <QSignalSpy>
#include <QTest>

#include <chrono>

#include "carnet/database.hpp"
#include "models/transaction_table_model.hpp"
#include "test_database.hpp"

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
    void descriptionTooltipShowsFullText();
    void greysRowsLeftOutOfTotals();
    void incomeReadableInDarkMode();

private:
    TestDatabase db_;
};

void TransactionTableModelTest::init() {
    db_.fresh();
    db_->addRule("coffee", db_.categoryId("Eating out"));
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
    const auto leisure = db_.categoryId("Leisure");
    {
        TransactionTableModel model(*db_);
        model.setMonth(2026y / March);
        QVERIFY(model.setData(model.index(1, TransactionTableModel::CategoryColumn), qint64(leisure)));
    }
    db_.reopen();
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
    QVERIFY(model.setData(cell, qint64(db_.categoryId("Leisure"))));
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

void TransactionTableModelTest::descriptionTooltipShowsFullText() {
    TransactionTableModel model(*db_);
    model.setMonth(2026y / March);
    QCOMPARE(model.index(0, TransactionTableModel::DescriptionColumn).data(Qt::ToolTipRole).toString(),
             QString("Salary March"));
    QVERIFY(!model.index(0, TransactionTableModel::AmountColumn).data(Qt::ToolTipRole).isValid());
}

void TransactionTableModelTest::greysRowsLeftOutOfTotals() {
    db_->setCategory(db_->transactionsInMonth(2026y / March)[0].id, db_.categoryId("Transfers"));
    TransactionTableModel model(*db_);
    model.setMonth(2026y / March);
    const QColor grey = QPalette().color(QPalette::Disabled, QPalette::Text);
    for (int column = 0; column < TransactionTableModel::ColumnCount; ++column) {
        QCOMPARE(model.index(0, column).data(Qt::ForegroundRole).value<QBrush>().color(), grey);
    }
    QVERIFY(model.index(0, TransactionTableModel::DescriptionColumn)
                .data(Qt::ToolTipRole)
                .toString()
                .contains("Salary March"));
    QVERIFY(!model.index(1, TransactionTableModel::DescriptionColumn).data(Qt::ForegroundRole).isValid());
}

void TransactionTableModelTest::incomeReadableInDarkMode() {
    const QPalette original = QGuiApplication::palette();
    QPalette dark = original;
    dark.setColor(QPalette::Base, QColor(0x2d, 0x2d, 0x2d));
    QGuiApplication::setPalette(dark);
    TransactionTableModel model(*db_);
    model.setMonth(2026y / March);
    const QColor income =
        model.index(0, TransactionTableModel::AmountColumn).data(Qt::ForegroundRole).value<QBrush>().color();
    QGuiApplication::setPalette(original);
    QVERIFY2(income.lightness() - dark.color(QPalette::Base).lightness() > 60, qPrintable(income.name()));
    QVERIFY(income.green() > income.red());
}

QTEST_MAIN(TransactionTableModelTest)
#include "transaction_table_model_test.moc"
