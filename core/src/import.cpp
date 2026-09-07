#include "carnet/import.hpp"

#include <algorithm>
#include <cassert>
#include <format>
#include <map>
#include <ranges>
#include <tuple>

#include "carnet/csv.hpp"
#include "carnet/date.hpp"
#include "carnet/rules.hpp"

namespace carnet {

namespace {

bool isSpace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

ParseError atLine(int line, std::string_view message) {
    return ParseError{std::format("line {}: {}", line, message)};
}

const std::string& field(const CsvRow& row, int column) {
    return row.fields[static_cast<std::size_t>(column)];
}

std::size_t requiredColumns(const ImportProfile& profile) {
    const int highest = std::max({profile.colDate, profile.colAmount, profile.colDesc,
                                  profile.colParty.value_or(0), profile.colStatus.value_or(0)});
    return static_cast<std::size_t>(highest) + 1;
}

std::optional<ParseError> widthProblem(const CsvRow& row, const ImportProfile& profile) {
    const std::size_t needed = requiredColumns(profile);
    if (row.fields.size() < needed) {
        return atLine(row.line,
                      std::format("expected at least {} columns, got {}", needed, row.fields.size()));
    }
    return std::nullopt;
}

bool hasSkipStatus(const CsvRow& row, const ImportProfile& profile) {
    if (!profile.colStatus || profile.skipStatus.empty()) {
        return false;
    }
    return foldText(normalizeText(field(row, *profile.colStatus))) ==
           foldText(normalizeText(profile.skipStatus));
}

std::expected<TxnDraft, ParseError> readRow(const CsvRow& row, const ImportProfile& profile) {
    const auto bookedOn = parseDate(field(row, profile.colDate), profile.dateFormat);
    if (!bookedOn) {
        return std::unexpected(atLine(row.line, bookedOn.error().message));
    }
    const auto amount = parseAmount(field(row, profile.colAmount), profile.decimalMark);
    if (!amount) {
        return std::unexpected(atLine(row.line, amount.error().message));
    }
    std::string description = normalizeText(field(row, profile.colDesc));
    if (description.empty()) {
        return std::unexpected(atLine(row.line, "empty description"));
    }

    TxnDraft draft{*bookedOn, *amount, std::move(description), {}, 1};
    if (profile.colParty) {
        draft.counterparty = normalizeText(field(row, *profile.colParty));
    }
    return draft;
}

void numberOccurrences(std::vector<TxnDraft>& drafts) {
    std::map<std::tuple<std::chrono::year_month_day, Cents, std::string>, int> seen;
    for (TxnDraft& draft : drafts) {
        draft.occurrence = ++seen[{draft.bookedOn, draft.amount, draft.description}];
    }
}

}

std::expected<StatementRows, std::vector<ParseError>> readStatement(std::string_view text,
                                                                    const ImportProfile& profile) {
    assert(profile.colDate >= 0 && profile.colAmount >= 0 && profile.colDesc >= 0 &&
           profile.colParty.value_or(0) >= 0 && profile.colStatus.value_or(0) >= 0);
    const auto rows = parseCsv(text, profile.separator);
    if (!rows) {
        return std::unexpected(std::vector<ParseError>{rows.error()});
    }

    StatementRows result;
    std::vector<ParseError> errors;
    for (const CsvRow& row : *rows | std::views::drop(std::max(profile.skipLines, 0))) {
        if (auto problem = widthProblem(row, profile)) {
            errors.push_back(std::move(*problem));
            continue;
        }
        // a skipped row is never parsed: refused payments can carry amounts or dates that are not real
        if (hasSkipStatus(row, profile)) {
            ++result.skipped;
            continue;
        }
        auto draft = readRow(row, profile);
        if (draft) {
            result.drafts.push_back(std::move(*draft));
        } else {
            errors.push_back(std::move(draft.error()));
        }
    }
    if (!errors.empty()) {
        return std::unexpected(std::move(errors));
    }

    numberOccurrences(result.drafts);
    return result;
}

std::string normalizeText(std::string_view text) {
    std::string result;
    bool pendingSpace = false;
    for (const char c : text) {
        if (isSpace(c)) {
            pendingSpace = !result.empty();
            continue;
        }
        if (pendingSpace) {
            result += ' ';
            pendingSpace = false;
        }
        result += c;
    }
    return result;
}

}
