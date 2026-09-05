#pragma once

#include <QWidget>

#include <chrono>

#include "carnet/database.hpp"

class QComboBox;
class QLineEdit;
class QTableView;

namespace carnet {

class TransactionFilter;
class TransactionTableModel;

class TransactionsTab : public QWidget {
    Q_OBJECT

public:
    explicit TransactionsTab(Database& db, QWidget* parent = nullptr);

    void setMonth(std::chrono::year_month month);
    void reload();

private:
    void refreshCategories();

    Database& db_;
    TransactionTableModel* model_;
    TransactionFilter* filter_;
    QLineEdit* search_;
    QComboBox* categoryBox_;
    QTableView* view_;
};

}
