#pragma once

#include <QString>

#include <chrono>

#include "carnet/money.hpp"

namespace carnet {

// "1 234,56", without the currency sign, for fields the user edits
QString formatAmount(Cents cents);
QString formatCents(Cents cents);
QString formatDate(std::chrono::year_month_day date);
QString formatMonth(std::chrono::year_month month);

}
