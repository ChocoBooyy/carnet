#include <gtest/gtest.h>

#include <chrono>
#include <optional>
#include <string>

#include "carnet/date.hpp"

namespace carnet {
namespace {

using namespace std::chrono;

year_month_day parseOk(std::string_view text, std::string_view format) {
    const auto result = parseDate(text, format);
    if (!result) {
        ADD_FAILURE() << result.error().message;
        return {};
    }
    return *result;
}

void expectRejected(std::string_view text, std::string_view format, std::string_view reason) {
    const auto result = parseDate(text, format);
    ASSERT_FALSE(result.has_value()) << "'" << text << "' parsed as " << toIsoString(*result);
    EXPECT_NE(result.error().message.find(reason), std::string::npos) << result.error().message;
}

TEST(ParseDate, CommonBankFormats) {
    EXPECT_EQ(parseOk("14/03/2026", "dd/MM/yyyy"), 2026y / March / 14);
    EXPECT_EQ(parseOk("2026-03-14", "yyyy-MM-dd"), 2026y / March / 14);
    EXPECT_EQ(parseOk("14.03.2026", "dd.MM.yyyy"), 2026y / March / 14);
}

TEST(ParseDate, SingleLetterTokensTakeOneOrTwoDigits) {
    EXPECT_EQ(parseOk("5.3.2026", "d.M.yyyy"), 2026y / March / 5);
    EXPECT_EQ(parseOk("15.12.2026", "d.M.yyyy"), 2026y / December / 15);
}

TEST(ParseDate, IgnoresSurroundingSpaces) {
    EXPECT_EQ(parseOk(" 14/03/2026 ", "dd/MM/yyyy"), 2026y / March / 14);
}

TEST(ParseDate, LeapDay) {
    EXPECT_EQ(parseOk("29/02/2024", "dd/MM/yyyy"), 2024y / February / 29);
}

TEST(ParseDate, RejectsImpossibleDates) {
    expectRejected("31/04/2026", "dd/MM/yyyy", "bad date '31/04/2026'");
    expectRejected("29/02/2025", "dd/MM/yyyy", "bad date");
    expectRejected("14/13/2026", "dd/MM/yyyy", "bad date");
    expectRejected("00/03/2026", "dd/MM/yyyy", "bad date");
}

TEST(ParseDate, RejectsTextThatDoesNotMatchTheFormat) {
    expectRejected("14-03-2026", "dd/MM/yyyy", "date '14-03-2026' does not match 'dd/MM/yyyy'");
    expectRejected("14/03/2026 10:32", "dd/MM/yyyy", "does not match");
    expectRejected("5/03/2026", "dd/MM/yyyy", "does not match");
    expectRejected("14/03/26", "dd/MM/yyyy", "does not match");
    expectRejected("", "dd/MM/yyyy", "does not match");
}

TEST(ParseDate, RejectsUnsupportedFormats) {
    expectRejected("14/03/26", "dd/MM/yy", "unsupported date format 'dd/MM/yy'");
    expectRejected("14/03/2026", "dd/MM/yyyy HH", "unsupported date format");
    expectRejected("03/2026", "MM/yyyy", "unsupported date format");
    expectRejected("14/14/2026", "dd/dd/yyyy", "unsupported date format");
}

TEST(ToIsoString, PadsMonthAndDay) {
    EXPECT_EQ(toIsoString(2026y / March / 4), "2026-03-04");
    EXPECT_EQ(toIsoString(2026y / December / 31), "2026-12-31");
}

TEST(IsoMonth, RoundTrips) {
    EXPECT_EQ(toIsoMonth(2026y / March), "2026-03");
    EXPECT_EQ(parseIsoMonth("2025-12"), std::optional(2025y / December));
    EXPECT_EQ(parseIsoMonth(toIsoMonth(2026y / January)), std::optional(2026y / January));
}

TEST(IsoMonth, RejectsAnythingElse) {
    for (const char* text : {"", "garbage", "2026-13", "2026-3", "2026-03-01", "26-03"}) {
        EXPECT_EQ(parseIsoMonth(text), std::nullopt) << text;
    }
}

TEST(CurrentMonth, MatchesTheClock) {
    const year_month_day today{floor<days>(system_clock::now())};
    EXPECT_EQ(currentMonth(), today.year() / today.month());
}

}
}
