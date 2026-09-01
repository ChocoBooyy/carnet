#pragma once

#include <cstdint>
#include <expected>
#include <string_view>

#include "carnet/parse_error.hpp"

namespace carnet {

using Cents = std::int64_t;

// decimalMark comes from the import profile, ',' or '.'; the other one is read as a thousands separator
std::expected<Cents, ParseError> parseAmount(std::string_view text, char decimalMark);

}
