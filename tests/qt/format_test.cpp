#include <QTest>

#include <chrono>
#include <limits>

#include "models/format.hpp"

using namespace std::chrono;

class FormatTest : public QObject {
    Q_OBJECT

private slots:
    void cents_data();
    void cents();
    void dates();
    void months();
    void amounts();
};

void FormatTest::cents_data() {
    QTest::addColumn<qint64>("cents");
    QTest::addColumn<QString>("expected");

    const QString euro(QChar(0x20AC));
    QTest::newRow("zero") << qint64(0) << QString("0,00 ") + euro;
    QTest::newRow("expense") << qint64(-1250) << QString("-12,50 ") + euro;
    QTest::newRow("thousands") << qint64(215000) << QString("2 150,00 ") + euro;
    QTest::newRow("millions") << qint64(-123456789) << QString("-1 234 567,89 ") + euro;
    QTest::newRow("cents only") << qint64(-5) << QString("-0,05 ") + euro;
    QTest::newRow("min") << std::numeric_limits<qint64>::min()
                         << QString("-92 233 720 368 547 758,08 ") + euro;
}

void FormatTest::cents() {
    QFETCH(qint64, cents);
    QFETCH(QString, expected);
    QCOMPARE(carnet::formatCents(cents), expected);
}

void FormatTest::dates() {
    QCOMPARE(carnet::formatDate(2026y / March / 4), QString("04/03/2026"));
    QCOMPARE(carnet::formatDate(2026y / December / 31), QString("31/12/2026"));
}

void FormatTest::months() {
    QCOMPARE(carnet::formatMonth(2026y / March), QString("March 2026"));
}

void FormatTest::amounts() {
    QCOMPARE(carnet::formatAmount(35000), QString("350,00"));
    QCOMPARE(carnet::formatAmount(-123456789), QString("-1 234 567,89"));
}

QTEST_MAIN(FormatTest)
#include "format_test.moc"
