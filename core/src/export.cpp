#include "carnet/export.hpp"

#include <algorithm>
#include <cstdint>
#include <format>
#include <initializer_list>

namespace carnet {

namespace {

constexpr std::string_view bom = "\xEF\xBB\xBF";
constexpr std::string_view endOfLine = "\r\n";

// "1234,56": no thousands separator, so a spreadsheet reads it as a number
std::string amount(Cents cents) {
    const bool negative = cents < 0;
    const std::uint64_t magnitude =
        negative ? 0 - static_cast<std::uint64_t>(cents) : static_cast<std::uint64_t>(cents);
    return std::format("{}{},{:02}", negative ? "-" : "", magnitude / 100, magnitude % 100);
}

std::string text(std::string_view value) {
    std::string field(value);
    // a spreadsheet runs a leading = + - @ as a formula; the apostrophe keeps it plain text
    if (!field.empty() && std::string_view("=+-@").contains(field.front())) {
        field.insert(0, 1, '\'');
    }
    if (field.find_first_of(";\"\r\n") == std::string::npos) {
        return field;
    }
    std::string quoted = "\"";
    for (const char c : field) {
        quoted += c;
        if (c == '"') {
            quoted += '"';
        }
    }
    return quoted + '"';
}

std::string date(std::chrono::year_month_day day) {
    return std::format("{:02}/{:02}/{}", static_cast<unsigned>(day.day()), static_cast<unsigned>(day.month()),
                       static_cast<int>(day.year()));
}

void line(std::string& out, std::initializer_list<std::string_view> fields) {
    bool first = true;
    for (const std::string_view field : fields) {
        if (!first) {
            out += ';';
        }
        out += field;
        first = false;
    }
    out += endOfLine;
}

}

std::string summaryCsv(std::chrono::year_month month, const MonthOverview& overview,
                       std::string_view uncategorizedLabel) {
    std::string out(bom);
    line(out, {"Month", std::format("{:02}/{}", static_cast<unsigned>(month.month()),
                                    static_cast<int>(month.year()))});
    line(out, {"Income", amount(overview.income)});
    line(out, {"Spent", amount(overview.spent)});
    line(out, {"Left", amount(overview.income - overview.spent)});
    out += endOfLine;
    line(out, {"Category", "Spent", "Budget"});
    for (const CategorySpend& row : overview.categories) {
        line(out, {text(row.categoryId ? std::string_view(row.name) : uncategorizedLabel), amount(row.spent),
                   row.budget ? amount(*row.budget) : std::string()});
    }
    return out;
}

std::string transactionsCsv(std::span<const Transaction> transactions, std::span<const Category> categories,
                            std::string_view uncategorizedLabel) {
    std::string out(bom);
    line(out, {"Date", "Account", "Description", "Counterparty", "Amount", "Category", "In totals"});
    for (const Transaction& txn : transactions) {
        const auto category =
            txn.categoryId ? std::ranges::find(categories, *txn.categoryId, &Category::id) : categories.end();
        const bool known = category != categories.end();
        line(out, {date(txn.bookedOn), text(txn.account), text(txn.description), text(txn.counterparty),
                   amount(txn.amount), text(known ? std::string_view(category->name) : uncategorizedLabel),
                   known && !category->inTotals ? "no" : "yes"});
    }
    return out;
}

}
