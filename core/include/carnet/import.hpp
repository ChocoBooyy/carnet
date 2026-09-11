#pragma once

#include <chrono>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "carnet/money.hpp"
#include "carnet/parse_error.hpp"

namespace carnet {

struct ImportProfile {
    std::string name;
    char separator = ';';
    char decimalMark = ',';
    std::string dateFormat = "dd/MM/yyyy";
    int skipLines = 1;
    int colDate = 0;
    int colAmount = 1;
    int colDesc = 2;
    std::optional<int> colParty;
    std::optional<int> colStatus;
    // rows whose status matches this (ignoring case and accents) are skipped, e.g. a refused card payment
    std::string skipStatus;

    bool operator==(const ImportProfile&) const = default;
};

struct TxnDraft {
    std::chrono::year_month_day bookedOn;
    Cents amount = 0;
    std::string description;
    std::string counterparty;
    // position among identical rows of the same file, so two real 3,20 coffees are not taken for one
    int occurrence = 1;
};

struct StatementRows {
    std::vector<TxnDraft> drafts;
    int skipped = 0;
};

std::expected<StatementRows, std::vector<ParseError>> readStatement(std::string_view text,
                                                                    const ImportProfile& profile);

}
