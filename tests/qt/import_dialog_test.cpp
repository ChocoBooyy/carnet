#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTest>

#include "carnet/database.hpp"
#include "dialogs/import_dialog.hpp"
#include "test_database.hpp"

using carnet::ImportDialog;

class ImportDialogTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void demoFileIsReadyToImport();
    void missingFileIsReported();
    void showsOnlyTheFileName();
    void separatorChangeRereadsTheFile();

private:
    TestDatabase db_;
};

void ImportDialogTest::initTestCase() {
    QStandardPaths::setTestModeEnabled(true);
}

void ImportDialogTest::init() {
    db_.fresh();
}

void ImportDialogTest::demoFileIsReadyToImport() {
    ImportDialog dialog(*db_);
    QVERIFY(dialog.openFile(CARNET_DEMO_FILE));
    auto* preview = dialog.findChild<QTableWidget*>();
    const auto role = [preview](int column) {
        return qobject_cast<QComboBox*>(preview->cellWidget(0, column))->currentText();
    };
    QCOMPARE(role(1), QString("Date"));
    QCOMPARE(role(3), QString("Amount"));
    QCOMPARE(role(8), QString("Counterparty"));
    QCOMPARE(role(10), QString("Description"));
    QCOMPARE(role(11), QString("Status"));
    QVERIFY(dialog.findChild<QPushButton*>("importButton")->isEnabled());
}

void ImportDialogTest::missingFileIsReported() {
    ImportDialog dialog(*db_);
    QVERIFY(!dialog.openFile(db_.filePath("nope.csv")));
}

void ImportDialogTest::showsOnlyTheFileName() {
    ImportDialog dialog(*db_);
    QVERIFY(dialog.openFile(CARNET_DEMO_FILE));
    auto* field = dialog.findChild<QLineEdit*>("fileName");
    QCOMPARE(field->text(), QString("demo-bank.csv"));
    QVERIFY(field->toolTip().endsWith("demo-bank.csv"));
}

void ImportDialogTest::separatorChangeRereadsTheFile() {
    ImportDialog dialog(*db_);
    QVERIFY(dialog.openFile(CARNET_DEMO_FILE));
    auto* preview = dialog.findChild<QTableWidget*>();
    QCOMPARE(preview->columnCount(), 13);
    QComboBox* separator = nullptr;
    for (auto* box : dialog.findChildren<QComboBox*>()) {
        if (box->findData(";") >= 0 && box->findData(",") >= 0 && box->parent() == &dialog &&
            box->currentData().toString() == ";") {
            separator = box;
        }
    }
    QVERIFY(separator != nullptr);
    separator->setCurrentIndex(separator->findData(","));
    QVERIFY(preview->columnCount() != 13);
    separator->setCurrentIndex(separator->findData(";"));
    QCOMPARE(preview->columnCount(), 13);
    QCOMPARE(qobject_cast<QComboBox*>(preview->cellWidget(0, 1))->currentText(), QString("Date"));
}

QTEST_MAIN(ImportDialogTest)
#include "import_dialog_test.moc"
