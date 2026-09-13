#include "dialogs/rules_dialog.hpp"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTabWidget>
#include <QVBoxLayout>

#include <algorithm>

#include "carnet/money.hpp"
#include "database_call.hpp"
#include "models/format.hpp"

namespace carnet {

RulesDialog::RulesDialog(Database& db, QWidget* parent) : QDialog(parent), db_(db) {
    setWindowTitle(tr("Rules and categories"));

    auto* tabs = new QTabWidget(this);
    tabs->addTab(buildRulesTab(), tr("Rules"));
    tabs->addTab(buildCategoriesTab(), tr("Categories"));
    auto* close = new QDialogButtonBox(QDialogButtonBox::Close, this);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(tabs);
    layout->addWidget(close);
    resize(620, 480);

    connect(close, &QDialogButtonBox::rejected, this, &QDialog::accept);
    reload();
}

bool RulesDialog::changed() const {
    return changed_;
}

QWidget* RulesDialog::buildRulesTab() {
    auto* page = new QWidget(this);
    ruleList_ = new QListWidget(page);
    patternEdit_ = new QLineEdit(page);
    patternEdit_->setPlaceholderText(tr("Text the description or counterparty contains"));
    ruleCategoryBox_ = new QComboBox(page);
    status_ = new QLabel(page);

    auto* add = new QPushButton(tr("Add"), page);
    auto* update = new QPushButton(tr("Update"), page);
    auto* remove = new QPushButton(tr("Delete"), page);
    auto* up = new QPushButton(tr("Move up"), page);
    auto* down = new QPushButton(tr("Move down"), page);
    auto* apply = new QPushButton(tr("Apply rules now"), page);

    auto* side = new QVBoxLayout;
    side->addWidget(up);
    side->addWidget(down);
    side->addWidget(remove);
    side->addStretch();
    side->addWidget(apply);
    auto* top = new QHBoxLayout;
    top->addWidget(ruleList_, 1);
    top->addLayout(side);

    auto* form = new QGridLayout;
    form->addWidget(new QLabel(tr("Contains"), page), 0, 0);
    form->addWidget(patternEdit_, 0, 1);
    form->addWidget(new QLabel(tr("Category"), page), 1, 0);
    form->addWidget(ruleCategoryBox_, 1, 1);
    auto* formButtons = new QHBoxLayout;
    formButtons->addStretch();
    formButtons->addWidget(add);
    formButtons->addWidget(update);

    auto* layout = new QVBoxLayout(page);
    layout->addLayout(top, 1);
    layout->addLayout(form);
    layout->addLayout(formButtons);
    layout->addWidget(status_);

    connect(ruleList_, &QListWidget::currentRowChanged, this, &RulesDialog::ruleSelected);
    connect(add, &QPushButton::clicked, this, &RulesDialog::addRule);
    connect(update, &QPushButton::clicked, this, &RulesDialog::updateRule);
    connect(remove, &QPushButton::clicked, this, &RulesDialog::deleteRule);
    connect(up, &QPushButton::clicked, this, [this] { moveRule(MoveDirection::Up); });
    connect(down, &QPushButton::clicked, this, [this] { moveRule(MoveDirection::Down); });
    connect(apply, &QPushButton::clicked, this, &RulesDialog::applyNow);
    return page;
}

QWidget* RulesDialog::buildCategoriesTab() {
    auto* page = new QWidget(this);
    categoryList_ = new QListWidget(page);
    auto* add = new QPushButton(tr("Add..."), page);
    auto* rename = new QPushButton(tr("Rename..."), page);
    auto* remove = new QPushButton(tr("Delete"), page);

    auto* side = new QVBoxLayout;
    side->addWidget(add);
    side->addWidget(rename);
    side->addWidget(remove);
    side->addStretch();
    auto* hint = new QLabel(tr("Checked categories count in the monthly totals. Uncheck money that only "
                               "moves between your own accounts."),
                            page);
    hint->setWordWrap(true);
    budgetEdit_ = new QLineEdit(page);
    budgetEdit_->setObjectName("budgetEdit");
    budgetEdit_->setPlaceholderText(tr("No budget"));
    budgetButton_ = new QPushButton(tr("Set budget"), page);
    budgetButton_->setObjectName("setBudget");
    budgetHint_ = new QLabel(page);
    budgetHint_->setObjectName("budgetHint");
    auto* budgetRow = new QHBoxLayout;
    budgetRow->addWidget(new QLabel(tr("Monthly budget"), page));
    budgetRow->addWidget(budgetEdit_, 1);
    budgetRow->addWidget(budgetButton_);
    auto* listColumn = new QVBoxLayout;
    listColumn->addWidget(categoryList_, 1);
    listColumn->addWidget(hint);
    listColumn->addLayout(budgetRow);
    listColumn->addWidget(budgetHint_);
    auto* layout = new QHBoxLayout(page);
    layout->addLayout(listColumn, 1);
    layout->addLayout(side);

    connect(add, &QPushButton::clicked, this, &RulesDialog::addCategory);
    connect(rename, &QPushButton::clicked, this, &RulesDialog::renameCategory);
    connect(remove, &QPushButton::clicked, this, &RulesDialog::deleteCategory);
    connect(categoryList_, &QListWidget::itemChanged, this, &RulesDialog::inTotalsToggled);
    connect(categoryList_, &QListWidget::currentRowChanged, this, &RulesDialog::categorySelected);
    connect(budgetButton_, &QPushButton::clicked, this, &RulesDialog::setBudget);
    connect(budgetEdit_, &QLineEdit::returnPressed, this, &RulesDialog::setBudget);
    return page;
}

void RulesDialog::reload() {
    const int ruleRow = ruleList_->currentRow();
    const int categoryRow = categoryList_->currentRow();
    if (!tryDatabase(this, [&] {
            categories_ = db_.categories();
            rules_ = db_.rules();
        })) {
        return;
    }

    ruleCategoryBox_->clear();
    const QSignalBlocker blocker(categoryList_);
    categoryList_->clear();
    for (const Category& category : categories_) {
        const QString name = QString::fromStdString(category.name);
        ruleCategoryBox_->addItem(name, qint64(category.id));
        const QString label =
            category.budget ? tr("%1 (%2 a month)").arg(name, formatCents(*category.budget)) : name;
        auto* item = new QListWidgetItem(label, categoryList_);
        item->setData(Qt::UserRole, qint64(category.id));
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(category.inTotals ? Qt::Checked : Qt::Unchecked);
    }
    ruleList_->clear();
    for (const Rule& rule : rules_) {
        auto* item =
            new QListWidgetItem(tr("contains \"%1\": %2")
                                    .arg(QString::fromStdString(rule.pattern), categoryName(rule.categoryId)),
                                ruleList_);
        item->setData(Qt::UserRole, qint64(rule.id));
    }
    ruleList_->setCurrentRow(std::min(ruleRow, ruleList_->count() - 1));
    categoryList_->setCurrentRow(std::min(categoryRow, categoryList_->count() - 1));
    categorySelected();
}

void RulesDialog::ruleSelected() {
    const int row = ruleList_->currentRow();
    if (row < 0 || row >= static_cast<int>(rules_.size())) {
        return;
    }
    const Rule& rule = rules_[static_cast<std::size_t>(row)];
    patternEdit_->setText(QString::fromStdString(rule.pattern));
    ruleCategoryBox_->setCurrentIndex(ruleCategoryBox_->findData(qint64(rule.categoryId)));
}

void RulesDialog::edit(auto&& call) {
    if (tryDatabase(this, call)) {
        changed_ = true;
        status_->clear();
    }
    reload();
}

void RulesDialog::inTotalsToggled(QListWidgetItem* item) {
    const std::int64_t id = item->data(Qt::UserRole).toLongLong();
    const bool inTotals = item->checkState() == Qt::Checked;
    if (tryDatabase(this, [&] { db_.setCategoryInTotals(id, inTotals); })) {
        changed_ = true;
        status_->clear();
    }
    // queued, since reloading deletes the item this signal is about; it also shows a cleared budget
    // or puts the box back as the database has it
    QMetaObject::invokeMethod(this, &RulesDialog::reload, Qt::QueuedConnection);
}

void RulesDialog::categorySelected() {
    budgetHint_->clear();
    const auto id = selectedId(categoryList_);
    const auto found = id ? std::ranges::find(categories_, *id, &Category::id) : categories_.end();
    const bool editable = found != categories_.end() && found->inTotals;
    budgetEdit_->setEnabled(editable);
    budgetButton_->setEnabled(editable);
    budgetEdit_->setText(editable && found->budget ? formatAmount(*found->budget) : QString());
}

void RulesDialog::setBudget() {
    const auto id = selectedId(categoryList_);
    if (!id) {
        return;
    }
    const std::string text = budgetEdit_->text().trimmed().toStdString();
    std::optional<Cents> budget;
    if (!text.empty()) {
        // a comma is the usual mark here; a dot is accepted for people who type 12.50
        auto amount = parseAmount(text, ',');
        if (!amount) {
            amount = parseAmount(text, '.');
        }
        if (!amount) {
            budgetHint_->setText(tr("Not an amount: %1").arg(budgetEdit_->text()));
            return;
        }
        if (*amount <= 0) {
            budgetHint_->setText(tr("A budget must be more than zero"));
            return;
        }
        budget = *amount;
    }
    edit([&] { db_.setCategoryBudget(*id, budget); });
}

void RulesDialog::addRule() {
    const QString pattern = patternEdit_->text().trimmed();
    if (pattern.isEmpty() || ruleCategoryBox_->currentIndex() < 0) {
        return;
    }
    edit([&] { db_.addRule(pattern.toStdString(), ruleCategoryBox_->currentData().toLongLong()); });
    ruleList_->setCurrentRow(ruleList_->count() - 1);
}

void RulesDialog::updateRule() {
    const auto id = selectedId(ruleList_);
    const QString pattern = patternEdit_->text().trimmed();
    if (!id || pattern.isEmpty()) {
        return;
    }
    edit([&] { db_.updateRule(*id, pattern.toStdString(), ruleCategoryBox_->currentData().toLongLong()); });
}

void RulesDialog::deleteRule() {
    if (const auto id = selectedId(ruleList_)) {
        edit([&] { db_.deleteRule(*id); });
    }
}

void RulesDialog::moveRule(MoveDirection direction) {
    const auto id = selectedId(ruleList_);
    if (!id) {
        return;
    }
    const int row = ruleList_->currentRow();
    bool moved = false;
    edit([&] { moved = db_.moveRule(*id, direction); });
    if (moved) {
        ruleList_->setCurrentRow(direction == MoveDirection::Up ? row - 1 : row + 1);
    }
}

void RulesDialog::applyNow() {
    int changedRows = 0;
    if (tryDatabase(this, [&] { changedRows = db_.applyRules(); })) {
        changed_ = true;
        status_->setText(tr("%1 transactions recategorized").arg(changedRows));
    }
}

void RulesDialog::addCategory() {
    const QString name = QInputDialog::getText(this, tr("Add category"), tr("Name:")).trimmed();
    if (!name.isEmpty()) {
        edit([&] { db_.addCategory(name.toStdString()); });
    }
}

void RulesDialog::renameCategory() {
    const auto id = selectedId(categoryList_);
    if (!id) {
        return;
    }
    const QString name =
        QInputDialog::getText(this, tr("Rename category"), tr("Name:"), QLineEdit::Normal, categoryName(*id))
            .trimmed();
    if (!name.isEmpty()) {
        edit([&] { db_.renameCategory(*id, name.toStdString()); });
    }
}

void RulesDialog::deleteCategory() {
    const auto id = selectedId(categoryList_);
    if (!id) {
        return;
    }
    const auto answer = QMessageBox::question(
        this, tr("Delete category"),
        tr("Transactions in \"%1\" become uncategorized and its rules are deleted. Delete it?")
            .arg(categoryName(*id)));
    if (answer == QMessageBox::Yes) {
        edit([&] { db_.deleteCategory(*id); });
    }
}

std::optional<std::int64_t> RulesDialog::selectedId(const QListWidget* list) const {
    const QListWidgetItem* item = list->currentItem();
    if (item == nullptr) {
        return std::nullopt;
    }
    return item->data(Qt::UserRole).toLongLong();
}

QString RulesDialog::categoryName(std::int64_t id) const {
    const auto found = std::ranges::find(categories_, id, &Category::id);
    return found == categories_.end() ? QString() : QString::fromStdString(found->name);
}

}
