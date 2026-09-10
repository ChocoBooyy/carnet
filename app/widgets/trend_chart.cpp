#include "widgets/trend_chart.hpp"

#include <QLocale>
#include <QPainter>

#include <algorithm>

#include "widgets/chart_colors.hpp"

namespace carnet {

TrendChart::TrendChart(QWidget* parent) : QWidget(parent) {
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void TrendChart::setMonths(std::vector<MonthTotals> months, std::chrono::year_month selected) {
    months_ = std::move(months);
    selected_ = selected;
    update();
}

QSize TrendChart::sizeHint() const {
    return {480, 160};
}

void TrendChart::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.fillRect(rect(), palette().color(QPalette::Base));
    if (months_.empty()) {
        return;
    }
    Cents highest = 0;
    for (const MonthTotals& month : months_) {
        highest = std::max({highest, month.income, month.spent});
    }

    const int labelHeight = fontMetrics().height() + 6;
    const int chartBottom = height() - labelHeight;
    const int chartHeight = std::max(0, chartBottom - 6);
    const int slotWidth = width() / static_cast<int>(months_.size());
    const int barWidth = std::max(2, slotWidth * 3 / 10);
    const QLocale english(QLocale::English);

    const auto barHeight = [&](Cents value) {
        // a ratio for pixels only; an all-zero range draws no bars instead of dividing by zero
        return highest > 0 ? static_cast<int>(chartHeight *
                                              (static_cast<double>(value) / static_cast<double>(highest)))
                           : 0;
    };

    for (std::size_t i = 0; i < months_.size(); ++i) {
        const MonthTotals& month = months_[i];
        const int left = static_cast<int>(i) * slotWidth;
        const int centre = left + slotWidth / 2;
        const int spent = barHeight(month.spent);
        const int income = barHeight(month.income);
        painter.fillRect(QRect(centre - barWidth, chartBottom - spent, barWidth, spent),
                         palette().color(QPalette::Highlight));
        painter.fillRect(QRect(centre, chartBottom - income, barWidth, income), incomeColor(palette()));

        QFont font = painter.font();
        font.setBold(month.month == selected_);
        painter.setFont(font);
        painter.setPen(palette().color(QPalette::Text));
        painter.drawText(QRect(left, chartBottom, slotWidth, labelHeight), Qt::AlignCenter,
                         english.monthName(static_cast<int>(static_cast<unsigned>(month.month.month())),
                                           QLocale::ShortFormat));
    }
}

}
