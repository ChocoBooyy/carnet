#pragma once

#include <QAbstractTableModel>

#include <chrono>
#include <optional>
#include <vector>

#include "carnet/database.hpp"

namespace carnet {

class TransactionTableModel : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column {
        DateColumn,
        AccountColumn,
        DescriptionColumn,
        CounterpartyColumn,
        AmountColumn,
        CategoryColumn,
        ColumnCount
    };

    static constexpr qint64 automaticChoice = -1;
    static constexpr qint64 noCategoryChoice = 0;

    explicit TransactionTableModel(Database& db, QObject* parent = nullptr);

    void setMonth(std::chrono::year_month month);
    void reload();
    const Transaction& transaction(int row) const;

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

signals:
    void errorOccurred(const QString& message);

private:
    void reloadRow(int row);
    QString displayText(const Transaction& txn, int column) const;
    const Category* findCategory(std::optional<std::int64_t> id) const;
    QString categoryName(std::optional<std::int64_t> id) const;
    bool leftOutOfTotals(const Transaction& txn) const;

    Database& db_;
    std::chrono::year_month month_{};
    std::vector<Transaction> rows_;
    std::vector<Category> categories_;
};

}
