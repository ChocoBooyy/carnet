#pragma once

#include <chrono>
#include <expected>
#include <optional>
#include <string>
#include <string_view>

#include "carnet/parse_error.hpp"

namespace carnet {

// format tokens are dd/d, MM/M and yyyy; other letters are unsupported, anything else must appear in text
// as-is
std::expected<std::chrono::year_month_day, ParseError> parseDate(std::string_view text,
                                                                 std::string_view format);

std::string toIsoString(std::chrono::year_month_day date);

// "2026-09", the form a month is saved in
std::string toIsoMonth(std::chrono::year_month month);
std::optional<std::chrono::year_month> parseIsoMonth(std::string_view text);

std::chrono::year_month currentMonth();

}
