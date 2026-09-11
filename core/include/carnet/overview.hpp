#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "carnet/money.hpp"

namespace carnet {

struct CategorySpend {
    std::optional<std::int64_t> categoryId; // empty for uncategorized money
    std::string name;                       // empty for uncategorized money; the app names it
    Cents spent = 0;
    std::optional<Cents> budget;

    bool operator==(const CategorySpend&) const = default;
};

struct MonthOverview {
    Cents income = 0;
    Cents spent = 0;
    std::vector<CategorySpend> categories; // largest spent first

    bool operator==(const MonthOverview&) const = default;
};

struct MonthTotals {
    std::chrono::year_month month;
    Cents income = 0;
    Cents spent = 0;

    bool operator==(const MonthTotals&) const = default;
};

}
