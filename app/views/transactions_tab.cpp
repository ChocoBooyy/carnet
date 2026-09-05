#include "views/transactions_tab.hpp"

#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QTableView>
#include <QVBoxLayout>

#include "database_call.hpp"
#include "models/transaction_filter.hpp"
#include "models/transaction_table_model.hpp"
#include "widgets/category_delegate.hpp"

namespace carnet {

TransactionsTab::TransactionsTab(Database& db, QWidget* parent) : QWidget(parent), db_(db) {
    model_ = new TransactionTableModel(db_, this);
    filter_ = new TransactionFilter(this);
    filter_->setSourceModel(model_);

    search_ = new QLineEdit(this);
    search_->setPlaceholderText(tr("Search description or counterparty"));
    search_->setClearButtonEnabled(true);
    categoryBox_ = new QComboBox(this);

    view_ = new QTableView(this);
    view_->setModel(filter_);
    view_->setSortingEnabled(true);
    view_->sortByColumn(TransactionTableModel::DateColumn, Qt::AscendingOrder);
    view_->setItemDelegateForColumn(TransactionTableModel::CategoryColumn, new CategoryDelegate(db_, view_));
    view_->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked |
                           QAbstractItemView::EditKeyPressed);
    view_->setSelectionBehavior(QAbstractItemView::SelectRows);
    view_->verticalHeader()->hide();
    view_->horizontalHeader()->setSectionResizeMode(TransactionTableModel::DescriptionColumn,
                                                    QHeaderView::Stretch);

    auto* controls = new QHBoxLayout;
    controls->addWidget(search_, 1);
    controls->addWidget(categoryBox_);
    auto* layout = new QVBoxLayout(this);
    layout->addLayout(controls);
    layout->addWidget(view_);

    connect(search_, &QLineEdit::textChanged, filter_, &TransactionFilter::setSearchText);
    connect(categoryBox_, &QComboBox::currentIndexChanged, this,
            [this] { filter_->setCategoryFilter(categoryBox_->currentData().toLongLong()); });
    connect(model_, &TransactionTableModel::errorOccurred, this,
            [this](const QString& message) { QMessageBox::critical(this, tr("Carnet"), message); });
}

void TransactionsTab::setMonth(std::chrono::year_month month) {
    tryDatabase(this, [&] { model_->setMonth(month); });
    refreshCategories();
    view_->resizeColumnsToContents();
}

void TransactionsTab::reload() {
    tryDatabase(this, [&] { model_->reload(); });
    refreshCategories();
}

void TransactionsTab::refreshCategories() {
    const qint64 selected = categoryBox_->currentData().isValid() ? categoryBox_->currentData().toLongLong()
                                                                  : TransactionFilter::allCategories;
    std::vector<Category> categories;
    if (!tryDatabase(this, [&] { categories = db_.categories(); })) {
        return;
    }
    const QSignalBlocker blocker(categoryBox_);
    categoryBox_->clear();
    categoryBox_->addItem(tr("All categories"), TransactionFilter::allCategories);
    categoryBox_->addItem(tr("Uncategorized"), TransactionFilter::uncategorized);
    for (const Category& category : categories) {
        categoryBox_->addItem(QString::fromStdString(category.name), qint64(category.id));
    }
    const int position = categoryBox_->findData(selected);
    categoryBox_->setCurrentIndex(position < 0 ? 0 : position);
    filter_->setCategoryFilter(categoryBox_->currentData().toLongLong());
}

}
