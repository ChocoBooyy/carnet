#include "models/transaction_table_model.hpp"

#include <QBrush>
#include <QFont>
#include <QPalette>

#include <algorithm>

#include "carnet/date.hpp"
#include "models/format.hpp"

namespace carnet {

TransactionTableModel::TransactionTableModel(Database& db, QObject* parent)
    : QAbstractTableModel(parent), db_(db) {
}

void TransactionTableModel::setMonth(std::chrono::year_month month) {
    month_ = month;
    reload();
}

void TransactionTableModel::reload() {
    // query before the reset so a database error leaves the old rows showing instead of a half-reset model
    auto rows = db_.transactionsInMonth(month_);
    auto categories = db_.categories();
    beginResetModel();
    rows_ = std::move(rows);
    categories_ = std::move(categories);
    endResetModel();
}

const Transaction& TransactionTableModel::transaction(int row) const {
    return rows_[static_cast<std::size_t>(row)];
}

int TransactionTableModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(rows_.size());
}

int TransactionTableModel::columnCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant TransactionTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) {
        return {};
    }
    const Transaction& txn = transaction(index.row());
    const int column = index.column();

    switch (role) {
    case Qt::DisplayRole:
        return displayText(txn, column);
    case Qt::EditRole:
        if (column == CategoryColumn) {
            return txn.manual ? QVariant(qint64(txn.categoryId.value_or(noCategoryChoice)))
                              : QVariant(automaticChoice);
        }
        return displayText(txn, column);
    case Qt::UserRole:
        if (column == DateColumn) {
            return QString::fromStdString(toIsoString(txn.bookedOn));
        }
        if (column == AmountColumn) {
            return qint64(txn.amount);
        }
        return displayText(txn, column);
    case Qt::ForegroundRole:
        if (leftOutOfTotals(txn)) {
            return QBrush(QPalette().color(QPalette::Disabled, QPalette::Text));
        }
        if (column == AmountColumn && txn.amount > 0) {
            return QBrush(QColor(0x1b, 0x7f, 0x3a));
        }
        return {};
    case Qt::TextAlignmentRole:
        if (column == AmountColumn) {
            return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
        }
        return {};
    case Qt::FontRole:
        if (column == CategoryColumn && txn.manual) {
            QFont font;
            font.setItalic(true);
            return font;
        }
        return {};
    case Qt::ToolTipRole:
        if (column == CategoryColumn && txn.manual) {
            return tr("Set by hand");
        }
        // bank descriptions are long and the useful part is often in the middle
        if (column == DescriptionColumn) {
            return QString::fromStdString(txn.description);
        }
        return {};
    default:
        return {};
    }
}

QVariant TransactionTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return {};
    }
    switch (section) {
    case DateColumn:
        return tr("Date");
    case AccountColumn:
        return tr("Account");
    case DescriptionColumn:
        return tr("Description");
    case CounterpartyColumn:
        return tr("Counterparty");
    case AmountColumn:
        return tr("Amount");
    case CategoryColumn:
        return tr("Category");
    default:
        return {};
    }
}

Qt::ItemFlags TransactionTableModel::flags(const QModelIndex& index) const {
    const Qt::ItemFlags base = QAbstractTableModel::flags(index);
    return index.column() == CategoryColumn ? base | Qt::ItemIsEditable : base;
}

bool TransactionTableModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (!index.isValid() || index.column() != CategoryColumn || role != Qt::EditRole) {
        return false;
    }
    const std::int64_t txnId = transaction(index.row()).id;
    const qint64 choice = value.toLongLong();
    try {
        if (choice == automaticChoice) {
            db_.clearManualCategory(txnId);
        } else if (choice == noCategoryChoice) {
            db_.setCategory(txnId, std::nullopt);
        } else {
            db_.setCategory(txnId, choice);
        }
        reloadRow(index.row());
    } catch (const DatabaseError& error) {
        emit errorOccurred(QString::fromUtf8(error.what()));
        return false;
    }
    return true;
}

void TransactionTableModel::reloadRow(int row) {
    // the database may have changed more than this row (rules), but only this row was edited here
    const std::int64_t txnId = transaction(row).id;
    for (Transaction& fresh : db_.transactionsInMonth(month_)) {
        if (fresh.id == txnId) {
            rows_[static_cast<std::size_t>(row)] = std::move(fresh);
            break;
        }
    }
    emit dataChanged(index(row, 0), index(row, ColumnCount - 1));
}

QString TransactionTableModel::displayText(const Transaction& txn, int column) const {
    switch (column) {
    case DateColumn:
        return formatDate(txn.bookedOn);
    case AccountColumn:
        return QString::fromStdString(txn.account);
    case DescriptionColumn:
        return QString::fromStdString(txn.description);
    case CounterpartyColumn:
        return QString::fromStdString(txn.counterparty);
    case AmountColumn:
        return formatCents(txn.amount);
    case CategoryColumn:
        return categoryName(txn.categoryId);
    default:
        return {};
    }
}

const Category* TransactionTableModel::findCategory(std::optional<std::int64_t> id) const {
    if (!id) {
        return nullptr;
    }
    const auto found = std::ranges::find(categories_, *id, &Category::id);
    return found == categories_.end() ? nullptr : &*found;
}

QString TransactionTableModel::categoryName(std::optional<std::int64_t> id) const {
    const Category* category = findCategory(id);
    return category == nullptr ? QString() : QString::fromStdString(category->name);
}

bool TransactionTableModel::leftOutOfTotals(const Transaction& txn) const {
    const Category* category = findCategory(txn.categoryId);
    return category != nullptr && !category->inTotals;
}

}
