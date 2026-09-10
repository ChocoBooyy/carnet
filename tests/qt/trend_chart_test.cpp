#include <QImage>
#include <QTest>

#include <chrono>

#include "widgets/chart_colors.hpp"
#include "widgets/trend_chart.hpp"

using namespace std::chrono;
using carnet::TrendChart;

namespace {

bool slotHas(const QImage& image, int slotCount, int slot, const QColor& color) {
    for (int x = image.width() * slot / slotCount; x < image.width() * (slot + 1) / slotCount; ++x) {
        for (int y = 0; y < image.height(); ++y) {
            if (image.pixelColor(x, y) == color) {
                return true;
            }
        }
    }
    return false;
}

QImage render(TrendChart& chart) {
    chart.resize(600, 160);
    QImage image(chart.size(), QImage::Format_RGB32);
    chart.render(&image);
    return image;
}

}

class TrendChartTest : public QObject {
    Q_OBJECT

private slots:
    void drawsSpentAndIncome();
    void allZerosDrawNothing();
};

void TrendChartTest::drawsSpentAndIncome() {
    TrendChart chart;
    chart.setMonths({{2026y / August, 0, 9900}, {2026y / September, 220500, 57450}}, 2026y / September);
    const QImage image = render(chart);
    const QColor spent = chart.palette().color(QPalette::Highlight);
    QVERIFY(slotHas(image, 2, 0, spent));
    QVERIFY(!slotHas(image, 2, 0, carnet::incomeColor()));
    QVERIFY(slotHas(image, 2, 1, spent));
    QVERIFY(slotHas(image, 2, 1, carnet::incomeColor()));
}

void TrendChartTest::allZerosDrawNothing() {
    TrendChart chart;
    chart.setMonths({{2026y / August, 0, 0}, {2026y / September, 0, 0}}, 2026y / September);
    const QImage image = render(chart);
    QVERIFY(!slotHas(image, 1, 0, chart.palette().color(QPalette::Highlight)));
    QVERIFY(!slotHas(image, 1, 0, carnet::incomeColor()));
    chart.setMonths({}, 2026y / September);
    render(chart);
}

QTEST_MAIN(TrendChartTest)
#include "trend_chart_test.moc"
