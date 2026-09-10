#include "widgets/budget_bars.hpp"

#include <QPainter>

#include <algorithm>

#include "models/format.hpp"
#include "widgets/chart_colors.hpp"

namespace carnet {

BudgetBars::BudgetBars(QWidget* parent) : QWidget(parent) {
    // grows to fill a scroll area, and never shrinks below its rows so the area scrolls instead
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
}

void BudgetBars::setRows(std::vector<CategorySpend> rows) {
    rows_ = std::move(rows);
    updateGeometry();
    update();
}

int BudgetBars::rowHeight() const {
    return fontMetrics().height() + 12;
}

QSize BudgetBars::sizeHint() const {
    return {nameWidth + amountWidth + 200, rowHeight() * std::max(1, static_cast<int>(rows_.size()))};
}

QSize BudgetBars::minimumSizeHint() const {
    return {nameWidth + amountWidth + 40, sizeHint().height()};
}

void BudgetBars::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.fillRect(rect(), palette().color(QPalette::Base));
    if (rows_.empty()) {
        painter.setPen(palette().color(QPalette::PlaceholderText));
        painter.drawText(rect(), Qt::AlignCenter, tr("No spending this month"));
        return;
    }

    const Cents largest = std::ranges::max(rows_, {}, &CategorySpend::spent).spent;
    const int barLeft = nameWidth + gap;
    const int barSpace = std::max(0, width() - barLeft - gap - amountWidth);
    const QFontMetrics metrics = fontMetrics();
    for (std::size_t i = 0; i < rows_.size(); ++i) {
        const CategorySpend& row = rows_[i];
        const int top = static_cast<int>(i) * rowHeight();
        const QRect nameRect(0, top, nameWidth, rowHeight());
        const QRect amountRect(width() - amountWidth, top, amountWidth, rowHeight());
        const QRect track(barLeft, top + 5, barSpace, rowHeight() - 10);

        // ratios only decide pixel lengths; the money itself stays in cents
        const double reference = static_cast<double>(row.budget ? *row.budget : largest);
        const double ratio = reference > 0 ? static_cast<double>(row.spent) / reference : 0.0;
        const bool over = row.budget && row.spent > *row.budget;
        QColor fill = palette().color(QPalette::Highlight);
        if (over) {
            fill = overBudgetColor();
        } else if (!row.categoryId) {
            // Mid sits right next to Midlight in dark themes; placeholder grey stands out in both
            fill = palette().color(QPalette::PlaceholderText);
        }

        painter.fillRect(track, palette().color(QPalette::Midlight));
        painter.fillRect(QRect(track.left(), track.top(),
                               static_cast<int>(track.width() * std::min(ratio, 1.0)), track.height()),
                         fill);

        painter.setPen(palette().color(QPalette::Text));
        painter.drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter,
                         metrics.elidedText(QString::fromStdString(row.name), Qt::ElideRight, nameWidth));
        const QString amount = row.budget
                                   ? tr("%1 / %2").arg(formatCents(row.spent), formatCents(*row.budget))
                                   : formatCents(row.spent);
        painter.setPen(over ? overBudgetColor() : palette().color(QPalette::Text));
        painter.drawText(amountRect, Qt::AlignRight | Qt::AlignVCenter, amount);
    }
}

}
