#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>

#include <filesystem>
#include <optional>

#include "carnet/database.hpp"
#include "dialogs/import_dialog.hpp"

using carnet::ImportDialog;

class ImportDialogTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void demoFileIsReadyToImport();
    void missingFileIsReported();
    void showsOnlyTheFileName();

private:
    QTemporaryDir dir_;
    std::optional<carnet::Database> db_;
};

void ImportDialogTest::initTestCase() {
    QStandardPaths::setTestModeEnabled(true);
}

void ImportDialogTest::init() {
    db_.reset();
    const std::filesystem::path path(dir_.filePath("carnet.db").toStdWString());
    std::filesystem::remove(path);
    db_ = carnet::Database::open(path);
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
    QVERIFY(!dialog.openFile(dir_.filePath("nope.csv")));
}

void ImportDialogTest::showsOnlyTheFileName() {
    ImportDialog dialog(*db_);
    QVERIFY(dialog.openFile(CARNET_DEMO_FILE));
    auto* field = dialog.findChild<QLineEdit*>("fileName");
    QCOMPARE(field->text(), QString("demo-bank.csv"));
    QVERIFY(field->toolTip().endsWith("demo-bank.csv"));
}

QTEST_MAIN(ImportDialogTest)
#include "import_dialog_test.moc"
