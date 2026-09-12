#include <QLabel>
#include <QTest>

#include "carnet/database.hpp"
#include "dialogs/about_dialog.hpp"

class AboutDialogTest : public QObject {
    Q_OBJECT

private slots:
    void showsVersions();
};

void AboutDialogTest::showsVersions() {
    carnet::AboutDialog dialog;
    QString text;
    for (const QLabel* label : dialog.findChildren<QLabel*>()) {
        text += label->text() + '\n';
    }
    QVERIFY2(text.contains(CARNET_VERSION), qPrintable(text));
    QVERIFY2(text.contains(qVersion()), qPrintable(text));
    QVERIFY2(text.contains(QString::fromStdString(carnet::sqliteVersion())), qPrintable(text));
}

QTEST_MAIN(AboutDialogTest)
#include "about_dialog_test.moc"
