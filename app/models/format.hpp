#pragma once

#include <QString>

#include <chrono>

#include "carnet/money.hpp"

namespace carnet {

QString formatCents(Cents cents);
QString formatDate(std::chrono::year_month_day date);
QString formatMonth(std::chrono::year_month month);

}
