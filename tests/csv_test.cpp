#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "carnet/csv.hpp"

namespace carnet {
namespace {

using Fields = std::vector<std::string>;

std::vector<CsvRow> parseOk(std::string_view text, char separator = ';') {
    const auto result = parseCsv(text, separator);
    if (!result) {
        ADD_FAILURE() << result.error().message;
        return {};
    }
    return *result;
}

std::string parseError(std::string_view text, char separator = ';') {
    const auto result = parseCsv(text, separator);
    if (result) {
        ADD_FAILURE() << "parsed " << result->size() << " rows";
        return {};
    }
    return result.error().message;
}

TEST(ParseCsv, SplitsFieldsOnSeparator) {
    const auto rows = parseOk("a;b;c\n1;2;3\n");
    ASSERT_EQ(rows.size(), 2u);
    EXPECT_EQ(rows[0].line, 1);
    EXPECT_EQ(rows[0].fields, (Fields{"a", "b", "c"}));
    EXPECT_EQ(rows[1].line, 2);
    EXPECT_EQ(rows[1].fields, (Fields{"1", "2", "3"}));
}

TEST(ParseCsv, CommaSeparator) {
    const auto rows = parseOk("a,b;c\n", ',');
    ASSERT_EQ(rows.size(), 1u);
    EXPECT_EQ(rows[0].fields, (Fields{"a", "b;c"}));
}

TEST(ParseCsv, QuotedFieldKeepsSeparator) {
    const auto rows = parseOk("\"a;b\";c\n");
    ASSERT_EQ(rows.size(), 1u);
    EXPECT_EQ(rows[0].fields, (Fields{"a;b", "c"}));
}

TEST(ParseCsv, DoubledQuoteIsOneQuote) {
    const auto rows = parseOk("\"say \"\"hi\"\"\";x\n");
    ASSERT_EQ(rows.size(), 1u);
    EXPECT_EQ(rows[0].fields, (Fields{"say \"hi\"", "x"}));
}

TEST(ParseCsv, LineBreakInsideQuotes) {
    const auto rows = parseOk("\"multi\nline\";x\nnext;y\n");
    ASSERT_EQ(rows.size(), 2u);
    EXPECT_EQ(rows[0].line, 1);
    EXPECT_EQ(rows[0].fields, (Fields{"multi\nline", "x"}));
    EXPECT_EQ(rows[1].line, 3);
    EXPECT_EQ(rows[1].fields, (Fields{"next", "y"}));
}

TEST(ParseCsv, SkipsByteOrderMark) {
    const auto rows = parseOk("\xEF\xBB\xBF"
                              "a;b\n");
    ASSERT_EQ(rows.size(), 1u);
    EXPECT_EQ(rows[0].fields, (Fields{"a", "b"}));
}

TEST(ParseCsv, HandlesCrlf) {
    const auto rows = parseOk("a;b\r\n\"c\";d\r\n");
    ASSERT_EQ(rows.size(), 2u);
    EXPECT_EQ(rows[0].fields, (Fields{"a", "b"}));
    EXPECT_EQ(rows[1].line, 2);
    EXPECT_EQ(rows[1].fields, (Fields{"c", "d"}));
}

TEST(ParseCsv, SkipsBlankLines) {
    const auto rows = parseOk("a\n\nb\n");
    ASSERT_EQ(rows.size(), 2u);
    EXPECT_EQ(rows[0].line, 1);
    EXPECT_EQ(rows[1].line, 3);
}

TEST(ParseCsv, QuotedEmptyFieldIsNotABlankLine) {
    const auto rows = parseOk("a\n\"\"\nb\n");
    ASSERT_EQ(rows.size(), 3u);
    EXPECT_EQ(rows[1].line, 2);
    EXPECT_EQ(rows[1].fields, (Fields{""}));
}

TEST(ParseCsv, TrailingSeparatorGivesEmptyField) {
    const auto rows = parseOk("a;b;\n");
    ASSERT_EQ(rows.size(), 1u);
    EXPECT_EQ(rows[0].fields, (Fields{"a", "b", ""}));

    const auto atEnd = parseOk("a;");
    ASSERT_EQ(atEnd.size(), 1u);
    EXPECT_EQ(atEnd[0].fields, (Fields{"a", ""}));
}

TEST(ParseCsv, LastLineWithoutLineBreak) {
    const auto rows = parseOk("a;b\nc;d");
    ASSERT_EQ(rows.size(), 2u);
    EXPECT_EQ(rows[1].fields, (Fields{"c", "d"}));
}

TEST(ParseCsv, RejectsTextAfterClosingQuote) {
    EXPECT_EQ(parseError("x;y\n\"a\"b;c\n"), "line 2: unexpected character after closing quote");
}

TEST(ParseCsv, UnclosedQuoteReportsStartLine) {
    EXPECT_EQ(parseError("x;y\n\"open\nmore\n"), "line 2: unclosed quote");
}

TEST(ParseCsv, EmptyInputGivesNoRows) {
    EXPECT_TRUE(parseOk("").empty());
    EXPECT_TRUE(parseOk("\n\n").empty());
}

}
}
