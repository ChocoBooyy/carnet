#include "models/format.hpp"

#include <QLocale>

#include <cstdint>

namespace carnet {

QString formatCents(Cents cents) {
    // the magnitude is taken as unsigned so the most negative value does not overflow
    const bool negative = cents < 0;
    const std::uint64_t magnitude =
        negative ? 0 - static_cast<std::uint64_t>(cents) : static_cast<std::uint64_t>(cents);

    QString euros = QString::number(magnitude / 100);
    for (qsizetype i = euros.size() - 3; i > 0; i -= 3) {
        euros.insert(i, u' ');
    }
    const QString decimals = QString("%1").arg(magnitude % 100, 2, 10, QChar(u'0'));
    return QString("%1%2,%3 ").arg(negative ? QString("-") : QString(), euros, decimals) + QChar(0x20AC);
}

QString formatDate(std::chrono::year_month_day date) {
    return QString("%1/%2/%3")
        .arg(static_cast<unsigned>(date.day()), 2, 10, QChar(u'0'))
        .arg(static_cast<unsigned>(date.month()), 2, 10, QChar(u'0'))
        .arg(static_cast<int>(date.year()));
}

QString formatMonth(std::chrono::year_month month) {
    const QLocale english(QLocale::English);
    return english.standaloneMonthName(static_cast<int>(static_cast<unsigned>(month.month()))) + u' ' +
           QString::number(static_cast<int>(month.year()));
}

}
