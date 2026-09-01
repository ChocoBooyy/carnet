#pragma once

#include <string_view>

namespace carnet {

// std::isdigit is undefined for negative char values, which every utf-8 continuation byte is
inline bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

inline std::string_view trim(std::string_view text) {
    const auto first = text.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = text.find_last_not_of(" \t");
    return text.substr(first, last - first + 1);
}

}
