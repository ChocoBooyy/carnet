#include "carnet/money.hpp"

#include <algorithm>
#include <cassert>
#include <format>
#include <limits>

#include "string_util.hpp"

namespace carnet {

namespace {

constexpr std::string_view unicodeMinus = "\xE2\x88\x92";
constexpr std::string_view noBreakSpace = "\xC2\xA0";
constexpr std::string_view narrowNoBreakSpace = "\xE2\x80\xAF";

bool appendDigit(Cents& value, char digit) {
    const Cents d = digit - '0';
    if (value > (std::numeric_limits<Cents>::max() - d) / 10) {
        return false;
    }
    value = value * 10 + d;
    return true;
}

std::string_view leadingSeparator(std::string_view text, char groupMark) {
    if (text.front() == groupMark || text.front() == ' ') {
        return text.substr(0, 1);
    }
    for (const auto space : {noBreakSpace, narrowNoBreakSpace}) {
        if (text.starts_with(space)) {
            return text.substr(0, space.size());
        }
    }
    return {};
}

}

std::expected<Cents, ParseError> parseAmount(std::string_view text, char decimalMark) {
    assert(decimalMark == ',' || decimalMark == '.');
    const auto fail = [text](std::string_view reason) {
        return std::unexpected(ParseError{std::format("bad amount '{}': {}", text, reason)});
    };

    std::string_view rest = trim(text);
    if (rest.empty()) {
        return fail("empty");
    }

    bool negative = false;
    if (rest.starts_with('-') || rest.starts_with(unicodeMinus)) {
        negative = true;
        rest.remove_prefix(rest.starts_with('-') ? 1 : unicodeMinus.size());
    } else if (rest.starts_with('+')) {
        rest.remove_prefix(1);
    }

    // digits go straight into an integer count of cents, so the value never passes through a double
    Cents cents = 0;
    const char groupMark = decimalMark == ',' ? '.' : ',';
    std::string_view separator;
    int groupCount = 1;
    int groupDigits = 0;
    while (!rest.empty() && rest.front() != decimalMark) {
        if (isDigit(rest.front())) {
            if (!appendDigit(cents, rest.front())) {
                return fail("too large");
            }
            ++groupDigits;
            rest.remove_prefix(1);
            continue;
        }
        const auto sep = leadingSeparator(rest, groupMark);
        if (sep.empty()) {
            return fail("unexpected character");
        }
        if (groupDigits == 0) {
            return fail("misplaced separator");
        }
        if (!separator.empty() && sep != separator) {
            return fail("mixed separators");
        }
        if (groupCount == 1 ? groupDigits > 3 : groupDigits != 3) {
            return fail("bad digit grouping");
        }
        separator = sep;
        ++groupCount;
        groupDigits = 0;
        rest.remove_prefix(sep.size());
    }
    if (groupDigits == 0) {
        return fail("missing digits");
    }
    if (groupCount > 1 && groupDigits != 3) {
        return fail("bad digit grouping");
    }

    std::size_t decimals = 0;
    if (!rest.empty()) {
        rest.remove_prefix(1);
        if (rest.empty()) {
            return fail("missing decimals");
        }
        if (!std::ranges::all_of(rest, isDigit)) {
            return fail("unexpected character");
        }
        if (rest.size() > 2) {
            return fail("more than two decimals");
        }
        for (const char c : rest) {
            if (!appendDigit(cents, c)) {
                return fail("too large");
            }
        }
        decimals = rest.size();
    }
    for (; decimals < 2; ++decimals) {
        if (!appendDigit(cents, '0')) {
            return fail("too large");
        }
    }
    return negative ? -cents : cents;
}

}
