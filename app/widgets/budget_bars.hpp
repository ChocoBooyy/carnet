#pragma once

#include <QWidget>

#include <vector>

#include "carnet/overview.hpp"

namespace carnet {

class BudgetBars : public QWidget {
    Q_OBJECT

public:
    static constexpr int nameWidth = 150;
    static constexpr int amountWidth = 190;
    static constexpr int gap = 8;

    explicit BudgetBars(QWidget* parent = nullptr);

    void setRows(std::vector<CategorySpend> rows);
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    int rowHeight() const;

    std::vector<CategorySpend> rows_;
};

}
