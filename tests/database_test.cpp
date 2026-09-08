#include <gtest/gtest.h>

#include <sqlite3.h>

#include <chrono>
#include <string>
#include <vector>

#include "carnet/database.hpp"
#include "carnet/import.hpp"
#include "temp_db.hpp"

namespace carnet {
namespace {

TEST(Database, CreatesSchemaOnFirstOpen) {
    TempDbPath file;
    const auto db = Database::open(file.path());
    EXPECT_EQ(db.schemaVersion(), 4);
}

TEST(Database, ReopenKeepsVersion) {
    TempDbPath file;
    {
        const auto db = Database::open(file.path());
    }
    const auto db = Database::open(file.path());
    EXPECT_EQ(db.schemaVersion(), 4);
}

TEST(Database, RefusesNewerDatabase) {
    TempDbPath file;
    sqlite3* raw = nullptr;
    ASSERT_EQ(sqlite3_open(file.path().string().c_str(), &raw), SQLITE_OK);
    ASSERT_EQ(sqlite3_exec(raw, "PRAGMA user_version = 99", nullptr, nullptr, nullptr), SQLITE_OK);
    sqlite3_close(raw);

    try {
        Database::open(file.path());
        FAIL() << "opened a database from a newer version";
    } catch (const DatabaseError& error) {
        EXPECT_EQ(std::string(error.what()), "database version 99 is newer than this app");
    }
}

TEST(Database, EnablesForeignKeys) {
    TempDbPath file;
    const auto db = Database::open(file.path());
    EXPECT_TRUE(db.foreignKeysEnabled());
}

ImportProfile profileNamed(std::string name) {
    ImportProfile profile;
    profile.name = std::move(name);
    return profile;
}

TEST(Database, ProfileRoundTrip) {
    TempDbPath file;
    auto db = Database::open(file.path());

    ImportProfile kbc = profileNamed("KBC");
    kbc.colParty = 5;
    ImportProfile belfius = profileNamed("Belfius");
    belfius.separator = ',';
    belfius.decimalMark = '.';
    belfius.dateFormat = "yyyy-MM-dd";
    belfius.skipLines = 3;
    belfius.colDate = 1;
    belfius.colAmount = 4;
    belfius.colDesc = 2;
    belfius.colStatus = 6;
    belfius.skipStatus = "Refus\xC3\xA9";
    db.saveProfile(kbc);
    db.saveProfile(belfius);

    const auto profiles = db.profiles();
    ASSERT_EQ(profiles.size(), 2u);
    EXPECT_EQ(profiles[0], belfius);
    EXPECT_EQ(profiles[1], kbc);
}

TEST(Database, SavingSameNameUpdates) {
    TempDbPath file;
    auto db = Database::open(file.path());
    ImportProfile profile = profileNamed("KBC");
    db.saveProfile(profile);
    profile.dateFormat = "d/M/yyyy";
    profile.colParty = 3;
    db.saveProfile(profile);

    const auto profiles = db.profiles();
    ASSERT_EQ(profiles.size(), 1u);
    EXPECT_EQ(profiles[0], profile);
}

TEST(Database, RejectsInvalidProfile) {
    TempDbPath file;
    auto db = Database::open(file.path());
    ImportProfile profile = profileNamed("Broken");
    profile.separator = '|';
    EXPECT_THROW(db.saveProfile(profile), DatabaseError);
    EXPECT_TRUE(db.profiles().empty());
}

TxnDraft draft(std::string description, Cents amount, int occurrence = 1) {
    using namespace std::chrono;
    return TxnDraft{2026y / March / 14, amount, std::move(description), "SHOP", occurrence};
}

std::vector<TxnDraft> threeDrafts() {
    return {draft("Coffee", -320), draft("Groceries", -4530), draft("Salary", 215000)};
}

TEST(Database, ImportAddsRows) {
    TempDbPath file;
    auto db = Database::open(file.path());
    const auto summary = db.importTransactions("Current", threeDrafts());
    EXPECT_EQ(summary.added, 3);
    EXPECT_EQ(summary.alreadyPresent, 0);
    EXPECT_EQ(db.transactionCount(), 3);
}

TEST(Database, ReimportCountsAlreadyPresent) {
    TempDbPath file;
    auto db = Database::open(file.path());
    db.importTransactions("Current", threeDrafts());
    const auto summary = db.importTransactions("Current", threeDrafts());
    EXPECT_EQ(summary.added, 0);
    EXPECT_EQ(summary.alreadyPresent, 3);
    EXPECT_EQ(db.transactionCount(), 3);
}

TEST(Database, AccountCreatedOnce) {
    TempDbPath file;
    auto db = Database::open(file.path());
    db.importTransactions("Current", {draft("Coffee", -320)});
    db.importTransactions("Current", {draft("Lunch", -1250)});
    EXPECT_EQ(db.accountCount(), 1);
}

TEST(Database, AccountsAreSeparate) {
    TempDbPath file;
    auto db = Database::open(file.path());
    EXPECT_EQ(db.importTransactions("Current", threeDrafts()).added, 3);
    EXPECT_EQ(db.importTransactions("Savings", threeDrafts()).added, 3);
    EXPECT_EQ(db.accountCount(), 2);
    EXPECT_EQ(db.transactionCount(), 6);
}

TEST(Database, OccurrenceKeepsIdenticalRows) {
    TempDbPath file;
    auto db = Database::open(file.path());
    const auto summary =
        db.importTransactions("Current", {draft("Coffee", -320, 1), draft("Coffee", -320, 2)});
    EXPECT_EQ(summary.added, 2);
    EXPECT_EQ(db.transactionCount(), 2);
}

TEST(Database, FailedImportRollsBack) {
    TempDbPath file;
    auto db = Database::open(file.path());
    EXPECT_THROW(db.importTransactions("Current", {draft("Coffee", -320), draft("Broken", -100, 0)}),
                 DatabaseError);
    EXPECT_EQ(db.transactionCount(), 0);
    EXPECT_EQ(db.accountCount(), 0);
}

TEST(Database, DataSurvivesReopen) {
    TempDbPath file;
    {
        auto db = Database::open(file.path());
        db.importTransactions("Current", threeDrafts());
    }
    const auto db = Database::open(file.path());
    EXPECT_EQ(db.transactionCount(), 3);
}

TEST(Database, ListsAccounts) {
    TempDbPath file;
    auto db = Database::open(file.path());
    db.importTransactions("Savings", {draft("Transfer", -1000)});
    db.importTransactions("Current", {draft("Coffee", -320)});
    db.importTransactions("Current", {draft("Lunch", -1250)});
    EXPECT_EQ(db.accounts(), (std::vector<std::string>{"Current", "Savings"}));
}

TEST(Database, TransactionsCarryTheirAccount) {
    using namespace std::chrono;
    TempDbPath file;
    auto db = Database::open(file.path());
    db.importTransactions("Savings", {draft("Transfer", -1000)});
    const auto txns = db.transactionsInMonth(2026y / March);
    ASSERT_EQ(txns.size(), 1u);
    EXPECT_EQ(txns[0].account, "Savings");
}

}
}
