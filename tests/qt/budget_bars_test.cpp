#include <QImage>
#include <QScrollArea>
#include <QScrollBar>
#include <QTest>

#include <cstdlib>

#include "widgets/budget_bars.hpp"
#include "widgets/chart_colors.hpp"

using carnet::BudgetBars;

namespace {

QImage render(BudgetBars& bars, int width = 600) {
    bars.resize(width, bars.sizeHint().height());
    QImage image(bars.size(), QImage::Format_RGB32);
    bars.render(&image);
    return image;
}

// looks only across the bar span, so the amount text next to it cannot count as a bar
bool barHas(const QImage& image, int rows, int row, const QColor& color) {
    const int y = image.height() * (2 * row + 1) / (2 * rows);
    for (int x = BudgetBars::nameWidth + BudgetBars::gap;
         x < image.width() - BudgetBars::gap - BudgetBars::amountWidth; ++x) {
        if (image.pixelColor(x, y) == color) {
            return true;
        }
    }
    return false;
}

}

class BudgetBarsTest : public QObject {
    Q_OBJECT

private slots:
    void heightFollowsRows();
    void overBudgetIsRed();
    void emptyShowsMessage();
    void hugeValuesStayInside();
    void fillsScrollAreaAndScrollsOnlyWhenNeeded();
    void uncategorizedVisibleInDarkMode();
};

void BudgetBarsTest::heightFollowsRows() {
    BudgetBars bars;
    bars.setRows({{1, "A", 100, std::nullopt}});
    const int one = bars.sizeHint().height();
    bars.setRows({{1, "A", 100, std::nullopt}, {2, "B", 50, std::nullopt}, {3, "C", 10, std::nullopt}});
    QCOMPARE(bars.sizeHint().height(), 3 * one);
}

void BudgetBarsTest::overBudgetIsRed() {
    BudgetBars bars;
    bars.setRows({{1, "Eating out", 18800, 15000}, {2, "Groceries", 30000, 35000}});
    const QImage image = render(bars);
    QVERIFY(barHas(image, 2, 0, carnet::overBudgetColor()));
    QVERIFY(!barHas(image, 2, 1, carnet::overBudgetColor()));
    QVERIFY(barHas(image, 2, 1, bars.palette().color(QPalette::Highlight)));
}

void BudgetBarsTest::emptyShowsMessage() {
    BudgetBars bars;
    bars.setRows({});
    QVERIFY(bars.sizeHint().height() > 0);
    const QImage image = render(bars);
    QVERIFY(!barHas(image, 1, 0, bars.palette().color(QPalette::Highlight)));
}

void BudgetBarsTest::hugeValuesStayInside() {
    BudgetBars bars;
    bars.setRows({{1, std::string(200, 'x'), 10'000'000'000, 100}, {2, "Small", 1, std::nullopt}});
    const QImage image = render(bars, 400);
    QVERIFY(barHas(image, 2, 0, carnet::overBudgetColor()));
    // the bar stops before the amount column even at 100 000 000 times its budget
    const int pastBar = image.width() - BudgetBars::amountWidth - BudgetBars::gap / 2;
    QVERIFY(image.pixelColor(pastBar, image.height() / 4) != carnet::overBudgetColor());
}

void BudgetBarsTest::fillsScrollAreaAndScrollsOnlyWhenNeeded() {
    QScrollArea area;
    area.setWidgetResizable(true);
    auto* bars = new BudgetBars;
    area.setWidget(bars);
    bars->setRows({{1, "A", 100, std::nullopt}});
    area.resize(600, 400);
    area.show();
    QCoreApplication::processEvents();
    QVERIFY(!area.verticalScrollBar()->isVisible());
    QCOMPARE(bars->height(), area.viewport()->height());

    std::vector<carnet::CategorySpend> many;
    for (int i = 0; i < 100; ++i) {
        many.push_back({i, "Row", 100, std::nullopt});
    }
    bars->setRows(many);
    QCoreApplication::processEvents();
    QVERIFY(area.verticalScrollBar()->isVisible());
}

void BudgetBarsTest::uncategorizedVisibleInDarkMode() {
    // colors close to the Windows 11 dark theme, where Mid and Midlight are nearly the same grey
    QPalette dark;
    dark.setColor(QPalette::Base, QColor(0x2d, 0x2d, 0x2d));
    dark.setColor(QPalette::Window, QColor(0x2d, 0x2d, 0x2d));
    dark.setColor(QPalette::Text, QColor(0xff, 0xff, 0xff));
    dark.setColor(QPalette::Midlight, QColor(0x33, 0x33, 0x33));
    dark.setColor(QPalette::Mid, QColor(0x2f, 0x2f, 0x2f));
    dark.setColor(QPalette::PlaceholderText, QColor(0x9d, 0x9d, 0x9d));
    BudgetBars bars;
    bars.setPalette(dark);
    bars.setRows({{std::nullopt, "", 100, std::nullopt}});
    const QImage image = render(bars);
    const int y = image.height() / 2;
    const QColor filled = image.pixelColor(BudgetBars::nameWidth + BudgetBars::gap + 2, y);
    const QColor track = dark.color(QPalette::Midlight);
    QVERIFY2(std::abs(filled.lightness() - track.lightness()) > 60,
             qPrintable(QString("bar %1 on track %2").arg(filled.name(), track.name())));
}

QTEST_MAIN(BudgetBarsTest)
#include "budget_bars_test.moc"
