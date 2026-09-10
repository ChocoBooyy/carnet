#pragma once

#include <QWidget>

#include <chrono>
#include <vector>

#include "carnet/overview.hpp"

namespace carnet {

class TrendChart : public QWidget {
    Q_OBJECT

public:
    explicit TrendChart(QWidget* parent = nullptr);

    void setMonths(std::vector<MonthTotals> months, std::chrono::year_month selected);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    std::vector<MonthTotals> months_;
    std::chrono::year_month selected_{};
};

}
