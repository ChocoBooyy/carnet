#include <QListWidget>
#include <QTemporaryDir>
#include <QTest>

#include <filesystem>
#include <optional>

#include "carnet/database.hpp"
#include "dialogs/rules_dialog.hpp"

using carnet::RulesDialog;

class RulesDialogTest : public QObject {
    Q_OBJECT

private slots:
    void init();
    void checkboxShowsInTotals();
    void uncheckingSavesTheFlag();

private:
    QListWidgetItem* categoryItem(RulesDialog& dialog, const QString& name) const;

    QTemporaryDir dir_;
    std::optional<carnet::Database> db_;
};

void RulesDialogTest::init() {
    db_.reset();
    const std::filesystem::path path(dir_.filePath("carnet.db").toStdWString());
    std::filesystem::remove(path);
    db_ = carnet::Database::open(path);
}

QListWidgetItem* RulesDialogTest::categoryItem(RulesDialog& dialog, const QString& name) const {
    for (auto* list : dialog.findChildren<QListWidget*>()) {
        const auto items = list->findItems(name, Qt::MatchExactly);
        if (!items.isEmpty() && (items[0]->flags() & Qt::ItemIsUserCheckable)) {
            return items[0];
        }
    }
    QTest::qFail("no checkable category item", __FILE__, __LINE__);
    return nullptr;
}

void RulesDialogTest::checkboxShowsInTotals() {
    RulesDialog dialog(*db_);
    QCOMPARE(categoryItem(dialog, "Transfers")->checkState(), Qt::Unchecked);
    QCOMPARE(categoryItem(dialog, "Groceries")->checkState(), Qt::Checked);
}

void RulesDialogTest::uncheckingSavesTheFlag() {
    {
        RulesDialog dialog(*db_);
        categoryItem(dialog, "Leisure")->setCheckState(Qt::Unchecked);
        QVERIFY(dialog.changed());
    }
    for (const carnet::Category& category : db_->categories()) {
        QCOMPARE(category.inTotals, category.name != "Leisure" && category.name != "Transfers");
    }
}

QTEST_MAIN(RulesDialogTest)
#include "rules_dialog_test.moc"
