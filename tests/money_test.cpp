#include <gtest/gtest.h>

#include <format>
#include <limits>
#include <string>

#include "carnet/money.hpp"

namespace carnet {
namespace {

const std::string unicodeMinus = "\xE2\x88\x92";
const std::string noBreakSpace = "\xC2\xA0";
const std::string narrowNoBreakSpace = "\xE2\x80\xAF";

Cents parseOk(std::string_view text, char decimalMark = ',') {
    const auto result = parseAmount(text, decimalMark);
    if (!result) {
        ADD_FAILURE() << result.error().message;
        return 0;
    }
    return *result;
}

void expectRejected(std::string_view text, std::string_view reason, char decimalMark = ',') {
    const auto result = parseAmount(text, decimalMark);
    ASSERT_FALSE(result.has_value()) << "'" << text << "' parsed as " << *result;
    const std::string& message = result.error().message;
    EXPECT_NE(message.find(std::format("'{}'", text)), std::string::npos) << message;
    EXPECT_NE(message.find(reason), std::string::npos) << message;
}

TEST(ParseAmount, WholeEuros) {
    EXPECT_EQ(parseOk("12"), 1200);
    EXPECT_EQ(parseOk("0"), 0);
    EXPECT_EQ(parseOk("007,50"), 750);
}

TEST(ParseAmount, DecimalMarkComesFromProfile) {
    EXPECT_EQ(parseOk("12,50", ','), 1250);
    EXPECT_EQ(parseOk("12.50", '.'), 1250);
}

TEST(ParseAmount, SubEuroAmounts) {
    EXPECT_EQ(parseOk("0,05"), 5);
    EXPECT_EQ(parseOk("-0,05"), -5);
    EXPECT_EQ(parseOk("0,5"), 50);
}

TEST(ParseAmount, SingleDecimalMeansTensOfCents) {
    EXPECT_EQ(parseOk("12,5"), 1250);
}

TEST(ParseAmount, Signs) {
    EXPECT_EQ(parseOk("-12,50"), -1250);
    EXPECT_EQ(parseOk("+12,50"), 1250);
    EXPECT_EQ(parseOk(unicodeMinus + "12,50"), -1250);
    EXPECT_EQ(parseOk("-0,00"), 0);
}

TEST(ParseAmount, ThousandsSeparators) {
    EXPECT_EQ(parseOk("1.234,56", ','), 123456);
    EXPECT_EQ(parseOk("1,234.56", '.'), 123456);
    EXPECT_EQ(parseOk("1 234 567,00"), 123456700);
    EXPECT_EQ(parseOk("1" + noBreakSpace + "234,56"), 123456);
    EXPECT_EQ(parseOk("1" + narrowNoBreakSpace + "234,56"), 123456);
    EXPECT_EQ(parseOk("1.234", ','), 123400);
}

TEST(ParseAmount, IgnoresSurroundingSpaces) {
    EXPECT_EQ(parseOk("  -12,50\t"), -1250);
}

TEST(ParseAmount, LargestValueFits) {
    EXPECT_EQ(parseOk("92233720368547758,07"), std::numeric_limits<Cents>::max());
    EXPECT_EQ(parseOk("-92233720368547758,07"), -std::numeric_limits<Cents>::max());
}

TEST(ParseAmount, RejectsMalformedInput) {
    expectRejected("", "empty");
    expectRejected("   ", "empty");
    expectRejected("abc", "unexpected character");
    expectRejected("12,50 EUR", "unexpected character");
    expectRejected("--5", "unexpected character");
    expectRejected("5-", "unexpected character");
    expectRejected("-", "missing digits");
    expectRejected(",50", "missing digits");
}

TEST(ParseAmount, RejectsBadDecimals) {
    expectRejected("12,505", "more than two decimals");
    expectRejected("12,", "missing decimals");
    expectRejected("12,5x", "unexpected character");
}

TEST(ParseAmount, RejectsBadGrouping) {
    expectRejected("1.23,00", "bad digit grouping");
    expectRejected("1234.567,00", "bad digit grouping");
    // an export read with the wrong decimal mark must not turn 12,50 into 1250 euros
    expectRejected("12.50", "bad digit grouping");
    expectRejected("12,50", "bad digit grouping", '.');
    expectRejected("1,23.00", "bad digit grouping", '.');
    expectRejected("1.234 567,00", "mixed separators");
    expectRejected("1..234", "misplaced separator");
    expectRejected(".234", "misplaced separator");
}

TEST(ParseAmount, RejectsValuesThatDoNotFit) {
    expectRejected("92233720368547758,08", "too large");
    expectRejected("99999999999999999999", "too large");
}

}
}
