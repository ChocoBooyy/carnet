#pragma once

#include <QWidget>

#include <chrono>

#include "carnet/database.hpp"

class QLabel;

namespace carnet {

class BudgetBars;
class TrendChart;

class OverviewTab : public QWidget {
    Q_OBJECT

public:
    explicit OverviewTab(Database& db, QWidget* parent = nullptr);

    void setMonth(std::chrono::year_month month);
    void reload();

private:
    Database& db_;
    std::chrono::year_month month_{};
    QLabel* income_;
    QLabel* spent_;
    QLabel* left_;
    BudgetBars* bars_;
    TrendChart* trend_;
};

}
