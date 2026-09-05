#pragma once

#include <QWidget>

#include <chrono>

class QLabel;

namespace carnet {

class MonthPicker : public QWidget {
    Q_OBJECT

public:
    explicit MonthPicker(QWidget* parent = nullptr);

    std::chrono::year_month month() const;
    void setMonth(std::chrono::year_month month);

signals:
    void monthChanged(std::chrono::year_month month);

private:
    void step(int months);

    QLabel* label_;
    std::chrono::year_month month_;
};

}
