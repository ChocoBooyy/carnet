#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace carnet {

struct Rule {
    std::int64_t id = 0;
    int priority = 0;
    std::string pattern;
    std::int64_t categoryId = 0;

    bool operator==(const Rule&) const = default;
};

// rules are tried in the order given; callers pass them in priority order
std::optional<std::int64_t> matchCategory(std::span<const Rule> rules, std::string_view description,
                                          std::string_view counterparty);

}
