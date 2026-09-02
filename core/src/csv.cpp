#include "carnet/csv.hpp"

#include <format>

namespace carnet {

namespace {

constexpr std::string_view byteOrderMark = "\xEF\xBB\xBF";

std::size_t lineBreakAt(std::string_view text, std::size_t i) {
    if (text.compare(i, 2, "\r\n") == 0) {
        return 2;
    }
    return text[i] == '\n' ? 1 : 0;
}

}

std::expected<std::vector<CsvRow>, ParseError> parseCsv(std::string_view text, char separator) {
    if (text.starts_with(byteOrderMark)) {
        text.remove_prefix(byteOrderMark.size());
    }

    std::vector<CsvRow> rows;
    int line = 1;
    CsvRow row{line, {}};
    bool quoted = false;
    std::size_t i = 0;
    while (i < text.size()) {
        std::string field;
        if (text[i] == '"') {
            quoted = true;
            const int quoteLine = line;
            ++i;
            while (true) {
                if (i == text.size()) {
                    return std::unexpected(ParseError{std::format("line {}: unclosed quote", quoteLine)});
                }
                if (text[i] == '"') {
                    if (i + 1 < text.size() && text[i + 1] == '"') {
                        field += '"';
                        i += 2;
                        continue;
                    }
                    ++i;
                    break;
                }
                if (text[i] == '\n') {
                    ++line;
                }
                field += text[i];
                ++i;
            }
            if (i < text.size() && text[i] != separator && lineBreakAt(text, i) == 0) {
                return std::unexpected(
                    ParseError{std::format("line {}: unexpected character after closing quote", line)});
            }
        } else {
            while (i < text.size() && text[i] != separator && lineBreakAt(text, i) == 0) {
                field += text[i];
                ++i;
            }
        }
        row.fields.push_back(std::move(field));

        if (i < text.size() && text[i] == separator) {
            ++i;
            if (i == text.size()) {
                row.fields.emplace_back();
            }
            continue;
        }

        // a record holding one empty, unquoted field is a blank line
        if (row.fields.size() > 1 || !row.fields.front().empty() || quoted) {
            rows.push_back(std::move(row));
        }
        if (i < text.size()) {
            i += lineBreakAt(text, i);
            ++line;
        }
        row = CsvRow{line, {}};
        quoted = false;
    }
    if (!row.fields.empty()) {
        rows.push_back(std::move(row));
    }
    return rows;
}

}
