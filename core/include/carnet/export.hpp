#pragma once

#include <chrono>
#include <span>
#include <string>
#include <string_view>

#include "carnet/database.hpp"
#include "carnet/overview.hpp"

namespace carnet {

// both return a whole file: UTF-8 with a byte order mark, ';' separated, CRLF lines, comma decimals,
// so it opens straight in a spreadsheet set up for a Belgian or French locale
std::string summaryCsv(std::chrono::year_month month, const MonthOverview& overview,
                       std::string_view uncategorizedLabel);
std::string transactionsCsv(std::span<const Transaction> transactions, std::span<const Category> categories,
                            std::string_view uncategorizedLabel);

}
