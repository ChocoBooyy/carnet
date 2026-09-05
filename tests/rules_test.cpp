#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "carnet/rules.hpp"

namespace carnet {
namespace {

const std::string upperEAcute = "\xC3\x89";
const std::string lowerEAcute = "\xC3\xA9";
const std::string upperOE = "\xC5\x92";
const std::string sharpS = "\xC3\x9F";
const std::string upperCCedilla = "\xC3\x87";
const std::string upperAGrave = "\xC3\x80";
const std::string upperODiaeresis = "\xC3\x96";

Rule rule(std::string pattern, std::int64_t categoryId) {
    Rule result;
    result.pattern = std::move(pattern);
    result.categoryId = categoryId;
    return result;
}

TEST(FoldText, LowercasesAscii) {
    EXPECT_EQ(foldText("BAKERY Dupont 42"), "bakery dupont 42");
}

TEST(FoldText, StripsFrenchAndDutchAccents) {
    EXPECT_EQ(foldText("CAF" + upperEAcute), "cafe");
    EXPECT_EQ(foldText("caf" + lowerEAcute), "cafe");
    EXPECT_EQ(foldText(upperOE + "uvre"), "oeuvre");
    EXPECT_EQ(foldText("Stra" + sharpS + "e"), "strasse");
    EXPECT_EQ(foldText(upperCCedilla + "a " + upperAGrave + " " + upperODiaeresis + "l"), "ca a ol");
}

TEST(FoldText, LeavesOtherCharactersAlone) {
    const std::string multiplication = "\xC3\x97";
    const std::string euro = "\xE2\x82\xAC";
    const std::string greekAlpha = "\xCE\x91";
    const std::string text = "3" + multiplication + "2 " + euro + " " + greekAlpha;
    EXPECT_EQ(foldText(text), text);
}

TEST(MatchCategory, FirstMatchingRuleWins) {
    const std::vector<Rule> rules{rule("delhaize", 1), rule("delhaize wavre", 2)};
    EXPECT_EQ(matchCategory(rules, "DELHAIZE WAVRE 1300", ""), 1);

    const std::vector<Rule> reordered{rule("delhaize wavre", 2), rule("delhaize", 1)};
    EXPECT_EQ(matchCategory(reordered, "DELHAIZE WAVRE 1300", ""), 2);
}

TEST(MatchCategory, IgnoresCase) {
    const std::vector<Rule> rules{rule("Shell", 3)};
    EXPECT_EQ(matchCategory(rules, "SHELL EXPRESS", ""), 3);
    EXPECT_EQ(matchCategory(rules, "shell", ""), 3);
}

TEST(MatchCategory, IgnoresAccentsBothWays) {
    EXPECT_EQ(matchCategory(std::vector{rule("cafe", 4)}, "CAF" + upperEAcute + " DU COIN", ""), 4);
    EXPECT_EQ(matchCategory(std::vector{rule("caf" + lowerEAcute, 4)}, "CAFE DU COIN", ""), 4);
}

TEST(MatchCategory, MatchesCounterparty) {
    EXPECT_EQ(matchCategory(std::vector{rule("acme", 5)}, "Salary March", "ACME SPRL"), 5);
}

TEST(MatchCategory, NoMatchGivesNothing) {
    EXPECT_EQ(matchCategory(std::vector{rule("shell", 3)}, "Groceries", "DELHAIZE"), std::nullopt);
    EXPECT_EQ(matchCategory(std::vector<Rule>{}, "Groceries", "DELHAIZE"), std::nullopt);
}

TEST(MatchCategory, PatternIsLiteral) {
    const std::vector<Rule> rules{rule("50%", 6)};
    EXPECT_EQ(matchCategory(rules, "Discount 50% off", ""), 6);
    EXPECT_EQ(matchCategory(rules, "Discount 500 off", ""), std::nullopt);
}

}
}
