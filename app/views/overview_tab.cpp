#include "views/overview_tab.hpp"

#include <QFormLayout>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>

#include "database_call.hpp"
#include "models/format.hpp"
#include "widgets/budget_bars.hpp"
#include "widgets/chart_colors.hpp"
#include "widgets/trend_chart.hpp"

namespace carnet {

namespace {

QLabel* amountLabel(const char* name, QWidget* parent) {
    auto* label = new QLabel(parent);
    label->setObjectName(name);
    QFont font = label->font();
    font.setPointSizeF(font.pointSizeF() * 1.3);
    label->setFont(font);
    return label;
}

}

OverviewTab::OverviewTab(Database& db, QWidget* parent) : QWidget(parent), db_(db) {
    income_ = amountLabel("income", this);
    spent_ = amountLabel("spent", this);
    left_ = amountLabel("left", this);
    auto* totals = new QFormLayout;
    totals->addRow(tr("Income"), income_);
    totals->addRow(tr("Spent"), spent_);
    totals->addRow(tr("Left"), left_);

    bars_ = new BudgetBars(this);
    auto* scroll = new QScrollArea(this);
    scroll->setFrameShape(QFrame::NoFrame);
    // the bars shrink and elide names, so only a long category list ever needs scrolling
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setWidgetResizable(true);
    scroll->setWidget(bars_);

    trend_ = new TrendChart(this);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(totals);
    layout->addWidget(new QLabel(tr("Spending per category"), this));
    layout->addWidget(scroll, 1);
    layout->addWidget(new QLabel(tr("Last 6 months, spent and income"), this));
    layout->addWidget(trend_);
}

void OverviewTab::setMonth(std::chrono::year_month month) {
    month_ = month;
    reload();
}

void OverviewTab::reload() {
    MonthOverview overview;
    std::vector<MonthTotals> months;
    if (!tryDatabase(this, [&] {
            overview = db_.monthOverview(month_);
            months = db_.monthlyTotals(month_, 6);
        })) {
        return;
    }
    income_->setText(formatCents(overview.income));
    spent_->setText(formatCents(overview.spent));
    const Cents left = overview.income - overview.spent;
    left_->setText(formatCents(left));
    QPalette leftPalette = left_->palette();
    leftPalette.setColor(QPalette::WindowText,
                         left < 0 ? overBudgetColor() : palette().color(QPalette::WindowText));
    left_->setPalette(leftPalette);
    bars_->setRows(std::move(overview.categories));
    trend_->setMonths(std::move(months), month_);
}

}
