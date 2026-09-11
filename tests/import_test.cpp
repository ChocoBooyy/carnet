#include <gtest/gtest.h>

#include <chrono>
#include <string>
#include <vector>

#include "carnet/import.hpp"
#include "carnet/text.hpp"

namespace carnet {
namespace {

using namespace std::chrono;

ImportProfile bankProfile() {
    ImportProfile profile;
    profile.name = "Test bank";
    profile.colParty = 3;
    return profile;
}

std::vector<TxnDraft> readOk(std::string_view text, const ImportProfile& profile) {
    const auto result = readStatement(text, profile);
    if (!result) {
        for (const ParseError& error : result.error()) {
            ADD_FAILURE() << error.message;
        }
        return {};
    }
    return result->drafts;
}

std::vector<std::string> readErrors(std::string_view text, const ImportProfile& profile) {
    const auto result = readStatement(text, profile);
    if (result) {
        ADD_FAILURE() << "read " << result->drafts.size() << " drafts";
        return {};
    }
    std::vector<std::string> messages;
    for (const ParseError& error : result.error()) {
        messages.push_back(error.message);
    }
    return messages;
}

TEST(ReadStatement, MapsColumnsThroughProfile) {
    const auto drafts = readOk("Date;Amount;Description;Counterparty\n"
                               "14/03/2026;-12,50;Bakery   Dupont ;DUPONT  SPRL\n"
                               "15/03/2026;2150,00;Salary March;ACME\n",
                               bankProfile());
    ASSERT_EQ(drafts.size(), 2u);
    EXPECT_EQ(drafts[0].bookedOn, 2026y / March / 14);
    EXPECT_EQ(drafts[0].amount, -1250);
    EXPECT_EQ(drafts[0].description, "Bakery Dupont");
    EXPECT_EQ(drafts[0].counterparty, "DUPONT SPRL");
    EXPECT_EQ(drafts[0].occurrence, 1);
    EXPECT_EQ(drafts[1].amount, 215000);
    EXPECT_EQ(drafts[1].description, "Salary March");
}

TEST(ReadStatement, ColumnOrderComesFromProfile) {
    ImportProfile profile;
    profile.separator = ',';
    profile.decimalMark = '.';
    profile.dateFormat = "yyyy-MM-dd";
    profile.colAmount = 0;
    profile.colDesc = 1;
    profile.colDate = 2;
    const auto drafts = readOk("amount,text,date\n-4.20,Parking,2026-03-02\n", profile);
    ASSERT_EQ(drafts.size(), 1u);
    EXPECT_EQ(drafts[0].bookedOn, 2026y / March / 2);
    EXPECT_EQ(drafts[0].amount, -420);
    EXPECT_EQ(drafts[0].description, "Parking");
}

TEST(ReadStatement, SkipLinesCountsRows) {
    ImportProfile profile = bankProfile();
    profile.skipLines = 0;
    EXPECT_EQ(readOk("14/03/2026;-1,00;A;B\n", profile).size(), 1u);

    profile.skipLines = 2;
    EXPECT_EQ(readOk("Account BE00 0000 0000 0000\nDate;Amount;Description;Counterparty\n"
                     "14/03/2026;-1,00;A;B\n",
                     profile)
                  .size(),
              1u);

    profile.skipLines = 5;
    EXPECT_TRUE(readOk("Date;Amount;Description;Counterparty\n14/03/2026;-1,00;A;B\n", profile).empty());
}

TEST(ReadStatement, UnmappedCounterpartyIsEmpty) {
    ImportProfile profile = bankProfile();
    profile.colParty.reset();
    const auto drafts = readOk("Date;Amount;Description\n14/03/2026;-1,00;A\n", profile);
    ASSERT_EQ(drafts.size(), 1u);
    EXPECT_EQ(drafts[0].counterparty, "");
}

TEST(ReadStatement, NumbersIdenticalRows) {
    const auto drafts = readOk("Date;Amount;Description;Counterparty\n"
                               "05/03/2026;-3,20;Coffee;BAKERY\n"
                               "05/03/2026;-3,20;Coffee  ;BAKERY\n"
                               "05/03/2026;-4,10;Coffee;BAKERY\n"
                               "06/03/2026;-3,20;Coffee;BAKERY\n",
                               bankProfile());
    ASSERT_EQ(drafts.size(), 4u);
    EXPECT_EQ(drafts[0].occurrence, 1);
    EXPECT_EQ(drafts[1].occurrence, 2);
    EXPECT_EQ(drafts[2].occurrence, 1);
    EXPECT_EQ(drafts[3].occurrence, 1);
}

TEST(ReadStatement, CollectsEveryBadRow) {
    const auto errors = readErrors("Date;Amount;Description;Counterparty\n"
                                   "14/03/2026;-12,50;Fine;X\n"
                                   "14/03/2026;-12,5,0;Lunch;X\n"
                                   "32/03/2026;-8,00;Parking;X\n"
                                   "14/03/2026;-4,00\n"
                                   "14/03/2026;-4,00;   ;X\n",
                                   bankProfile());
    ASSERT_EQ(errors.size(), 4u);
    EXPECT_TRUE(errors[0].starts_with("line 3: bad amount '-12,5,0'")) << errors[0];
    EXPECT_EQ(errors[1], "line 4: bad date '32/03/2026'");
    EXPECT_EQ(errors[2], "line 5: expected at least 4 columns, got 2");
    EXPECT_EQ(errors[3], "line 6: empty description");
}

TEST(ReadStatement, WrongSeparatorReportsColumns) {
    ImportProfile profile = bankProfile();
    profile.separator = ',';
    const auto errors = readErrors("Date;Amount;Description;Counterparty\n"
                                   "14/03/2026;-12;A;B\n"
                                   "15/03/2026;-13;C;D\n",
                                   profile);
    ASSERT_EQ(errors.size(), 2u);
    EXPECT_EQ(errors[0], "line 2: expected at least 4 columns, got 1");
    EXPECT_EQ(errors[1], "line 3: expected at least 4 columns, got 1");
}

TEST(ReadStatement, HeaderOnlyGivesNoDrafts) {
    EXPECT_TRUE(readOk("Date;Amount;Description;Counterparty\n", bankProfile()).empty());
}

TEST(ReadStatement, CsvErrorSurfacesAsOneError) {
    const auto errors =
        readErrors("Date;Amount;Description;Counterparty\n14/03/2026;-1,00;\"open;B\n", bankProfile());
    ASSERT_EQ(errors.size(), 1u);
    EXPECT_EQ(errors[0], "line 2: unclosed quote");
}

TEST(NormalizeText, TrimsAndCollapsesWhitespace) {
    EXPECT_EQ(normalizeText("  Bakery \t  Dupont  "), "Bakery Dupont");
    EXPECT_EQ(normalizeText("multi\r\nline"), "multi line");
    EXPECT_EQ(normalizeText("   "), "");
    EXPECT_EQ(normalizeText(""), "");
}

ImportProfile statusProfile() {
    ImportProfile profile = bankProfile();
    profile.colStatus = 4;
    profile.skipStatus = "Refus\xC3\xA9";
    return profile;
}

TEST(ReadStatement, SkipsRowsWithTheSkipStatus) {
    const auto result = readStatement("Date;Amount;Description;Counterparty;Status\n"
                                      "14/03/2026;-1,00;A;X;Accept\xC3\xA9\n"
                                      "15/03/2026;-2,00;B;X;Refus\xC3\xA9\n"
                                      "16/03/2026;-3,00;C;X;  REFUSE \n",
                                      statusProfile());
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->drafts.size(), 1u);
    EXPECT_EQ(result->drafts[0].description, "A");
    EXPECT_EQ(result->skipped, 2);
}

TEST(ReadStatement, SkippedRowsAreNotParsed) {
    const auto result = readStatement("Date;Amount;Description;Counterparty;Status\n"
                                      "14/03/2026;-1,00;A;X;Accept\xC3\xA9\n"
                                      "31/04/2026;-2,5,0;;X;Refus\xC3\xA9\n",
                                      statusProfile());
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->drafts.size(), 1u);
    EXPECT_EQ(result->skipped, 1);
}

TEST(ReadStatement, StatusColumnCountsForWidth) {
    const auto errors = readErrors("Date;Amount;Description;Counterparty;Status\n"
                                   "14/03/2026;-1,00;A;X\n",
                                   statusProfile());
    ASSERT_EQ(errors.size(), 1u);
    EXPECT_EQ(errors[0], "line 2: expected at least 5 columns, got 4");
}

TEST(ReadStatement, WithoutStatusNothingIsSkipped) {
    const auto result =
        readStatement("Date;Amount;Description;Counterparty\n14/03/2026;-1,00;A;X\n", bankProfile());
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->skipped, 0);

    ImportProfile emptySkip = statusProfile();
    emptySkip.skipStatus.clear();
    const auto kept = readStatement("Date;Amount;Description;Counterparty;Status\n"
                                    "15/03/2026;-2,00;B;X;Refus\xC3\xA9\n",
                                    emptySkip);
    ASSERT_TRUE(kept.has_value());
    EXPECT_EQ(kept->drafts.size(), 1u);
}

}
}
