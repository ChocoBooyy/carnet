#pragma once

#include <chrono>
#include <expected>
#include <string>
#include <string_view>

#include "carnet/parse_error.hpp"

namespace carnet {

// format tokens are dd/d, MM/M and yyyy; other letters are unsupported, anything else must appear in text
// as-is
std::expected<std::chrono::year_month_day, ParseError> parseDate(std::string_view text,
                                                                 std::string_view format);

std::string toIsoString(std::chrono::year_month_day date);

}
