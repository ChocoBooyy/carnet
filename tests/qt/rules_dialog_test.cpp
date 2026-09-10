#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
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
    void budgetInputForms();
    void badBudgetIsRejected();
    void excludedCategoryHasNoBudget();

private:
    QListWidgetItem* categoryItem(RulesDialog& dialog, const QString& name) const;
    std::optional<carnet::Cents> budgetOf(const QString& name) const;
    void select(RulesDialog& dialog, const QString& category) const;
    void setBudget(RulesDialog& dialog, const QString& category, const QString& text) const;

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
    // a category with a budget reads "Groceries (350,00 EUR a month)"
    for (auto* list : dialog.findChildren<QListWidget*>()) {
        for (auto* item : list->findItems(name, Qt::MatchStartsWith)) {
            if ((item->flags() & Qt::ItemIsUserCheckable) &&
                (item->text() == name || item->text().startsWith(name + " ("))) {
                return item;
            }
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

std::optional<carnet::Cents> RulesDialogTest::budgetOf(const QString& name) const {
    for (const carnet::Category& category : db_->categories()) {
        if (QString::fromStdString(category.name) == name) {
            return category.budget;
        }
    }
    QTest::qFail("no such category", __FILE__, __LINE__);
    return std::nullopt;
}

void RulesDialogTest::select(RulesDialog& dialog, const QString& category) const {
    QListWidgetItem* item = categoryItem(dialog, category);
    item->listWidget()->setCurrentItem(item);
}

void RulesDialogTest::setBudget(RulesDialog& dialog, const QString& category, const QString& text) const {
    select(dialog, category);
    dialog.findChild<QLineEdit*>("budgetEdit")->setText(text);
    dialog.findChild<QPushButton*>("setBudget")->click();
}

void RulesDialogTest::budgetInputForms() {
    RulesDialog dialog(*db_);
    setBudget(dialog, "Groceries", "1 250,00");
    QCOMPARE(budgetOf("Groceries"), std::optional<carnet::Cents>(125000));
    QVERIFY(categoryItem(dialog, "Groceries")->text().contains("1 250,00"));
    setBudget(dialog, "Groceries", "350");
    QCOMPARE(budgetOf("Groceries"), std::optional<carnet::Cents>(35000));
    setBudget(dialog, "Groceries", "12.50");
    QCOMPARE(budgetOf("Groceries"), std::optional<carnet::Cents>(1250));
    QVERIFY(dialog.changed());
    setBudget(dialog, "Groceries", "");
    QCOMPARE(budgetOf("Groceries"), std::optional<carnet::Cents>());
}

void RulesDialogTest::badBudgetIsRejected() {
    for (const carnet::Category& category : db_->categories()) {
        if (category.name == "Rent") {
            db_->setCategoryBudget(category.id, 1000);
        }
    }
    RulesDialog dialog(*db_);
    for (const QString& text : {QString("abc"), QString("0"), QString("-5")}) {
        setBudget(dialog, "Rent", text);
        QCOMPARE(budgetOf("Rent"), std::optional<carnet::Cents>(1000));
        QVERIFY(!dialog.findChild<QLabel*>("budgetHint")->text().isEmpty());
    }
}

void RulesDialogTest::excludedCategoryHasNoBudget() {
    RulesDialog dialog(*db_);
    select(dialog, "Transfers");
    QVERIFY(!dialog.findChild<QLineEdit*>("budgetEdit")->isEnabled());
    select(dialog, "Groceries");
    QVERIFY(dialog.findChild<QLineEdit*>("budgetEdit")->isEnabled());
}

QTEST_MAIN(RulesDialogTest)
#include "rules_dialog_test.moc"
