#include "widgets/month_picker.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>

#include "models/format.hpp"

namespace carnet {

MonthPicker::MonthPicker(QWidget* parent) : QWidget(parent) {
    const std::chrono::year_month_day today{
        std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now())};
    month_ = today.year() / today.month();

    auto* previous = new QToolButton(this);
    previous->setArrowType(Qt::LeftArrow);
    previous->setToolTip(tr("Previous month (Ctrl+PgUp)"));
    auto* next = new QToolButton(this);
    next->setArrowType(Qt::RightArrow);
    next->setToolTip(tr("Next month (Ctrl+PgDown)"));
    label_ = new QLabel(formatMonth(month_), this);
    label_->setMinimumWidth(120);
    label_->setAlignment(Qt::AlignCenter);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(previous);
    layout->addWidget(label_);
    layout->addWidget(next);

    connect(previous, &QToolButton::clicked, this, [this] { step(-1); });
    connect(next, &QToolButton::clicked, this, [this] { step(1); });
}

std::chrono::year_month MonthPicker::month() const {
    return month_;
}

void MonthPicker::setMonth(std::chrono::year_month month) {
    month_ = month;
    label_->setText(formatMonth(month_));
    emit monthChanged(month_);
}

void MonthPicker::step(int months) {
    setMonth(month_ + std::chrono::months{months});
}

}
