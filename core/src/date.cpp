#include "carnet/date.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <optional>
#include <vector>

#include "string_util.hpp"

namespace carnet {

namespace {

enum class Field { Literal, Day, Month, Year };

struct Token {
    Field field;
    char literal;
    std::size_t minDigits;
    std::size_t maxDigits;
};

struct TokenSpec {
    std::string_view letters;
    Token token;
};

constexpr std::array<TokenSpec, 5> tokenSpecs{{
    {"dd", {Field::Day, 0, 2, 2}},
    {"d", {Field::Day, 0, 1, 2}},
    {"MM", {Field::Month, 0, 2, 2}},
    {"M", {Field::Month, 0, 1, 2}},
    {"yyyy", {Field::Year, 0, 4, 4}},
}};

bool isAsciiLetter(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

std::optional<std::vector<Token>> tokenize(std::string_view format) {
    std::vector<Token> tokens;
    for (std::size_t i = 0; i < format.size();) {
        const char c = format[i];
        if (!isAsciiLetter(c)) {
            tokens.push_back({Field::Literal, c, 0, 0});
            ++i;
            continue;
        }
        std::size_t run = 1;
        while (i + run < format.size() && format[i + run] == c) {
            ++run;
        }
        const auto letters = format.substr(i, run);
        const auto spec = std::ranges::find(tokenSpecs, letters, &TokenSpec::letters);
        if (spec == tokenSpecs.end()) {
            return std::nullopt;
        }
        tokens.push_back(spec->token);
        i += run;
    }

    for (const Field field : {Field::Day, Field::Month, Field::Year}) {
        if (std::ranges::count(tokens, field, &Token::field) != 1) {
            return std::nullopt;
        }
    }
    return tokens;
}

}

std::expected<std::chrono::year_month_day, ParseError> parseDate(std::string_view text,
                                                                 std::string_view format) {
    const auto tokens = tokenize(format);
    if (!tokens) {
        return std::unexpected(ParseError{std::format("unsupported date format '{}'", format)});
    }
    const auto mismatch = [&] {
        return std::unexpected(ParseError{std::format("date '{}' does not match '{}'", text, format)});
    };

    int dayNumber = 0;
    int monthNumber = 0;
    int yearNumber = 0;
    std::string_view rest = trim(text);
    for (const Token& token : *tokens) {
        if (token.field == Field::Literal) {
            if (rest.empty() || rest.front() != token.literal) {
                return mismatch();
            }
            rest.remove_prefix(1);
            continue;
        }

        int value = 0;
        std::size_t digits = 0;
        while (digits < token.maxDigits && digits < rest.size() && isDigit(rest[digits])) {
            value = value * 10 + (rest[digits] - '0');
            ++digits;
        }
        if (digits < token.minDigits) {
            return mismatch();
        }
        rest.remove_prefix(digits);

        switch (token.field) {
        case Field::Day:
            dayNumber = value;
            break;
        case Field::Month:
            monthNumber = value;
            break;
        case Field::Year:
            yearNumber = value;
            break;
        case Field::Literal:
            break;
        }
    }
    if (!rest.empty()) {
        return mismatch();
    }

    const std::chrono::year_month_day date{std::chrono::year{yearNumber},
                                           std::chrono::month{static_cast<unsigned>(monthNumber)},
                                           std::chrono::day{static_cast<unsigned>(dayNumber)}};
    if (!date.ok()) {
        return std::unexpected(ParseError{std::format("bad date '{}'", text)});
    }
    return date;
}

std::string toIsoString(std::chrono::year_month_day date) {
    return std::format("{:%F}", date);
}

std::string toIsoMonth(std::chrono::year_month month) {
    return toIsoString(month / 1).substr(0, 7);
}

std::optional<std::chrono::year_month> parseIsoMonth(std::string_view text) {
    const auto day = parseDate(std::string(text) + "-01", "yyyy-MM-dd");
    if (!day) {
        return std::nullopt;
    }
    return day->year() / day->month();
}

std::chrono::year_month currentMonth() {
    const std::chrono::year_month_day today{
        std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now())};
    return today.year() / today.month();
}

}
