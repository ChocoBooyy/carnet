#include <gtest/gtest.h>

#include <fstream>
#include <sstream>
#include <string>

#include "carnet/import.hpp"

namespace carnet {
namespace {

std::string readFile(const char* path) {
    std::ifstream file(path, std::ios::binary);
    std::ostringstream text;
    text << file.rdbuf();
    return text.str();
}

// the README tells people to try Carnet with this file, so it has to import without a single error
TEST(DemoData, ImportsCleanly) {
    ImportProfile profile;
    profile.colDate = 1;
    profile.colAmount = 3;
    profile.colParty = 8;
    profile.colDesc = 10;
    profile.colStatus = 11;
    profile.skipStatus = "Refus\xC3\xA9";

    const std::string text = readFile(CARNET_DEMO_FILE);
    ASSERT_FALSE(text.empty());
    const auto statement = readStatement(text, profile);
    ASSERT_TRUE(statement.has_value());
    EXPECT_EQ(statement->skipped, 1);
    EXPECT_GE(statement->drafts.size(), 60u);
}

}
}
