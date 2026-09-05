#include "models/transaction_filter.hpp"

#include "carnet/rules.hpp"
#include "models/transaction_table_model.hpp"

namespace carnet {

TransactionFilter::TransactionFilter(QObject* parent) : QSortFilterProxyModel(parent) {
    setSortRole(Qt::UserRole);
}

void TransactionFilter::setSearchText(const QString& text) {
    beginFilterChange();
    foldedSearch_ = foldText(text.trimmed().toStdString());
    endFilterChange(Direction::Rows);
}

void TransactionFilter::setCategoryFilter(qint64 filter) {
    beginFilterChange();
    categoryFilter_ = filter;
    endFilterChange(Direction::Rows);
}

bool TransactionFilter::filterAcceptsRow(int sourceRow, const QModelIndex&) const {
    const auto* model = qobject_cast<const TransactionTableModel*>(sourceModel());
    if (model == nullptr) {
        return true;
    }
    const Transaction& txn = model->transaction(sourceRow);

    if (categoryFilter_ == uncategorized && txn.categoryId) {
        return false;
    }
    if (categoryFilter_ > 0 && txn.categoryId != categoryFilter_) {
        return false;
    }
    if (foldedSearch_.empty()) {
        return true;
    }
    return foldText(txn.description).contains(foldedSearch_) ||
           foldText(txn.counterparty).contains(foldedSearch_);
}

}
