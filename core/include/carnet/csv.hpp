#pragma once

#include <expected>
#include <string>
#include <string_view>
#include <vector>

#include "carnet/parse_error.hpp"

namespace carnet {

struct CsvRow {
    // the line the row starts on; a quoted field can carry the row over several lines
    int line;
    std::vector<std::string> fields;
};

std::expected<std::vector<CsvRow>, ParseError> parseCsv(std::string_view text, char separator);

}
