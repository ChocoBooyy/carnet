#include <gtest/gtest.h>

#include <chrono>
#include <string>
#include <vector>

#include "carnet/database.hpp"
#include "db_lookup.hpp"
#include "temp_db.hpp"

namespace carnet {
namespace {

using namespace std::chrono;

TxnDraft draft(year_month_day date, std::string description, std::string counterparty, Cents amount) {
    return TxnDraft{date, amount, std::move(description), std::move(counterparty), 1};
}

std::vector<Transaction> march(const Database& db) {
    return db.transactionsInMonth(2026y / March);
}

TEST(Categorize, ImportAppliesRules) {
    TempDbPath file;
    auto db = Database::open(file.path());
    const auto groceries = categoryId(db, "Groceries");
    db.addRule("delhaize", groceries);
    db.importTransactions("Current", {draft(2026y / March / 5, "DELHAIZE WAVRE", "", -4530),
                                      draft(2026y / March / 6, "Cinema", "KINEPOLIS", -1500)});

    const auto txns = march(db);
    ASSERT_EQ(txns.size(), 2u);
    EXPECT_EQ(txns[0].categoryId, groceries);
    EXPECT_FALSE(txns[0].manual);
    EXPECT_EQ(txns[1].categoryId, std::nullopt);
}

TEST(Categorize, FixingARuleMovesItsHistory) {
    TempDbPath file;
    auto db = Database::open(file.path());
    const auto wrong = db.addRule("shell", categoryId(db, "Groceries"));
    db.importTransactions("Current", {draft(2026y / March / 12, "SHELL EXPRESS", "", -6000)});
    ASSERT_EQ(march(db)[0].categoryId, categoryId(db, "Groceries"));

    db.updateRule(wrong, "shell", categoryId(db, "Transport"));
    EXPECT_EQ(db.applyRules(), 1);
    EXPECT_EQ(march(db)[0].categoryId, categoryId(db, "Transport"));
}

TEST(Categorize, ManualChoiceWins) {
    TempDbPath file;
    auto db = Database::open(file.path());
    const auto leisure = categoryId(db, "Leisure");
    db.importTransactions("Current", {draft(2026y / March / 12, "SHELL EXPRESS", "", -6000)});
    const auto id = march(db)[0].id;
    db.setCategory(id, leisure);

    db.addRule("shell", categoryId(db, "Transport"));
    EXPECT_EQ(db.applyRules(), 0);
    db.importTransactions("Current", {draft(2026y / March / 13, "SHELL EXPRESS", "", -5000)});

    const auto txns = march(db);
    ASSERT_EQ(txns.size(), 2u);
    EXPECT_EQ(txns[0].categoryId, leisure);
    EXPECT_TRUE(txns[0].manual);
    EXPECT_EQ(txns[1].categoryId, categoryId(db, "Transport"));
}

TEST(Categorize, ExplicitNoCategoryStays) {
    TempDbPath file;
    auto db = Database::open(file.path());
    db.addRule("shell", categoryId(db, "Transport"));
    db.importTransactions("Current", {draft(2026y / March / 12, "SHELL EXPRESS", "", -6000)});
    const auto id = march(db)[0].id;

    db.setCategory(id, std::nullopt);
    EXPECT_EQ(db.applyRules(), 0);
    EXPECT_EQ(march(db)[0].categoryId, std::nullopt);
    EXPECT_TRUE(march(db)[0].manual);
}

TEST(Categorize, ClearingManualHandsBackToRules) {
    TempDbPath file;
    auto db = Database::open(file.path());
    db.addRule("shell", categoryId(db, "Transport"));
    db.importTransactions("Current", {draft(2026y / March / 12, "SHELL EXPRESS", "", -6000)});
    const auto id = march(db)[0].id;
    db.setCategory(id, categoryId(db, "Leisure"));

    db.clearManualCategory(id);
    EXPECT_EQ(march(db)[0].categoryId, categoryId(db, "Transport"));
    EXPECT_FALSE(march(db)[0].manual);
}

TEST(Categorize, DeletedCategoryReleasesTransactions) {
    TempDbPath file;
    auto db = Database::open(file.path());
    const auto health = db.addCategory("Health");
    const auto groceries = categoryId(db, "Groceries");
    db.addRule("pharmacy", health);
    db.addRule("pharma", groceries);
    db.importTransactions("Current", {draft(2026y / March / 20, "Pharmacy Centrale", "", -2250),
                                      draft(2026y / March / 21, "Pharmacy Gare", "", -1100)});
    db.setCategory(march(db)[1].id, health);

    db.deleteCategory(health);
    const auto txns = march(db);
    ASSERT_EQ(txns.size(), 2u);
    EXPECT_EQ(txns[0].categoryId, groceries);
    EXPECT_EQ(txns[1].categoryId, groceries);
    EXPECT_FALSE(txns[1].manual);
}

TEST(Categorize, TransactionsInMonthFiltersAndOrders) {
    TempDbPath file;
    auto db = Database::open(file.path());
    db.importTransactions("Current", {draft(2026y / March / 31, "Last", "SHOP", -300),
                                      draft(2026y / February / 28, "Before", "", -100),
                                      draft(2026y / March / 1, "First", "", -200),
                                      draft(2026y / April / 1, "After", "", -400)});

    const auto txns = march(db);
    ASSERT_EQ(txns.size(), 2u);
    EXPECT_EQ(txns[0].description, "First");
    EXPECT_EQ(txns[0].bookedOn, 2026y / March / 1);
    EXPECT_EQ(txns[0].amount, -200);
    EXPECT_EQ(txns[0].counterparty, "");
    EXPECT_EQ(txns[1].description, "Last");
    EXPECT_EQ(txns[1].counterparty, "SHOP");
}

TEST(Categorize, UnknownTransactionThrows) {
    TempDbPath file;
    auto db = Database::open(file.path());
    EXPECT_THROW(db.setCategory(999, std::nullopt), DatabaseError);
    EXPECT_THROW(db.clearManualCategory(999), DatabaseError);
}

}
}
