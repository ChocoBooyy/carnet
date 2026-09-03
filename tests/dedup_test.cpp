#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "carnet/database.hpp"
#include "carnet/import.hpp"
#include "temp_db.hpp"

namespace carnet {
namespace {

std::string readData(std::string_view name) {
    std::ifstream in(std::filesystem::path(CARNET_TEST_DATA_DIR) / name, std::ios::binary);
    EXPECT_TRUE(in.is_open()) << name;
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

ImportProfile statementProfile() {
    ImportProfile profile;
    profile.name = "Test bank";
    profile.colParty = 3;
    return profile;
}

ImportSummary importFile(Database& db, std::string_view name) {
    const auto drafts = readStatement(readData(name), statementProfile());
    if (!drafts) {
        for (const ParseError& error : drafts.error()) {
            ADD_FAILURE() << name << ": " << error.message;
        }
        return {};
    }
    return db.importTransactions("Current account", *drafts);
}

TEST(Dedup, IdenticalRowsInOneFileAreKept) {
    TempDbPath file;
    auto db = Database::open(file.path());
    const auto summary = importFile(db, "march.csv");
    EXPECT_EQ(summary.added, 11);
    EXPECT_EQ(summary.alreadyPresent, 0);
    EXPECT_EQ(db.transactionCount(), 11);
}

TEST(Dedup, OverlappingExportsOnlyAddNewRows) {
    TempDbPath file;
    auto db = Database::open(file.path());
    importFile(db, "march.csv");
    const auto summary = importFile(db, "march_april.csv");
    EXPECT_EQ(summary.added, 5);
    EXPECT_EQ(summary.alreadyPresent, 4);
    EXPECT_EQ(db.transactionCount(), 16);

    const auto again = importFile(db, "march_april.csv");
    EXPECT_EQ(again.added, 0);
    EXPECT_EQ(again.alreadyPresent, 9);
}

TEST(Dedup, BrokenFileImportsNothing) {
    TempDbPath file;
    auto db = Database::open(file.path());
    importFile(db, "march.csv");

    const auto result = readStatement(readData("broken.csv"), statementProfile());
    ASSERT_FALSE(result.has_value());
    ASSERT_EQ(result.error().size(), 3u);
    EXPECT_TRUE(result.error()[0].message.starts_with("line 3: bad amount")) << result.error()[0].message;
    EXPECT_EQ(result.error()[1].message, "line 4: bad date '31/04/2026'");
    EXPECT_EQ(result.error()[2].message, "line 5: expected at least 4 columns, got 2");
    EXPECT_EQ(db.transactionCount(), 11);
}

}
}
