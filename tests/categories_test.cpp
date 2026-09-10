#include <gtest/gtest.h>

#include <sqlite3.h>

#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include "carnet/database.hpp"
#include "db_lookup.hpp"
#include "migrations.hpp"
#include "temp_db.hpp"

namespace carnet {
namespace {

std::vector<std::string> categoryNames(const Database& db) {
    std::vector<std::string> names;
    for (const Category& category : db.categories()) {
        names.push_back(category.name);
    }
    return names;
}

TEST(Categories, DefaultsExist) {
    TempDbPath file;
    const auto db = Database::open(file.path());
    EXPECT_EQ(categoryNames(db), (std::vector<std::string>{"Eating out", "Groceries", "Leisure", "Rent",
                                                           "Salary", "Transfers", "Transport", "Utilities"}));
}

TEST(Categories, AddRenameDelete) {
    TempDbPath file;
    auto db = Database::open(file.path());
    const auto id = db.addCategory("  Health  ");
    EXPECT_EQ(categoryId(db, "Health"), id);
    db.renameCategory(id, "Health care");
    EXPECT_EQ(categoryId(db, "Health care"), id);
    db.deleteCategory(id);
    EXPECT_EQ(db.categories().size(), 8u);
}

TEST(Categories, DuplicateNameRejectedIgnoringCase) {
    TempDbPath file;
    auto db = Database::open(file.path());
    EXPECT_THROW(db.addCategory("groceries"), DatabaseError);
    EXPECT_THROW(db.renameCategory(categoryId(db, "Rent"), "SALARY"), DatabaseError);
    EXPECT_THROW(db.addCategory("   "), DatabaseError);
    EXPECT_EQ(db.categories().size(), 8u);
}

TEST(Categories, UnknownIdsThrow) {
    TempDbPath file;
    auto db = Database::open(file.path());
    EXPECT_THROW(db.renameCategory(999, "Other"), DatabaseError);
    EXPECT_THROW(db.deleteCategory(999), DatabaseError);
}

TEST(Categories, UpgradesVersionOneDatabase) {
    TempDbPath file;
    sqlite3* raw = nullptr;
    ASSERT_EQ(sqlite3_open(file.path().string().c_str(), &raw), SQLITE_OK);
    ASSERT_EQ(sqlite3_exec(raw, migrations()[0], nullptr, nullptr, nullptr), SQLITE_OK);
    ASSERT_EQ(sqlite3_exec(raw,
                           "PRAGMA user_version = 1;"
                           "INSERT INTO account (name) VALUES ('Current');"
                           "INSERT INTO txn (account_id, booked_on, amount_cents, description, occurrence) "
                           "VALUES (1, '2026-03-05', -320, 'Coffee', 1);",
                           nullptr, nullptr, nullptr),
              SQLITE_OK);
    sqlite3_close(raw);

    const auto db = Database::open(file.path());
    EXPECT_EQ(db.schemaVersion(), 5);
    EXPECT_EQ(db.transactionCount(), 1);
    EXPECT_EQ(db.categories().size(), 8u);

    const auto txns = db.transactionsInMonth(std::chrono::year{2026} / std::chrono::March);
    ASSERT_EQ(txns.size(), 1u);
    EXPECT_EQ(txns[0].categoryId, std::nullopt);
    EXPECT_FALSE(txns[0].manual);
}

bool inTotals(const Database& db, std::string_view name) {
    for (const Category& category : db.categories()) {
        if (category.name == name) {
            return category.inTotals;
        }
    }
    ADD_FAILURE() << "no category " << name;
    return false;
}

TEST(Categories, TransfersStayOutOfTotals) {
    TempDbPath file;
    const auto db = Database::open(file.path());
    EXPECT_FALSE(inTotals(db, "Transfers"));
    EXPECT_TRUE(inTotals(db, "Groceries"));
    EXPECT_TRUE(inTotals(db, "Salary"));
}

TEST(Categories, InTotalsPersists) {
    TempDbPath file;
    {
        auto db = Database::open(file.path());
        db.setCategoryInTotals(categoryId(db, "Leisure"), false);
        db.setCategoryInTotals(categoryId(db, "Transfers"), true);
    }
    const auto db = Database::open(file.path());
    EXPECT_FALSE(inTotals(db, "Leisure"));
    EXPECT_TRUE(inTotals(db, "Transfers"));
}

TEST(Categories, NewCategoriesCountInTotals) {
    TempDbPath file;
    auto db = Database::open(file.path());
    db.addCategory("Health");
    EXPECT_TRUE(inTotals(db, "Health"));
    EXPECT_THROW(db.setCategoryInTotals(999, false), DatabaseError);
}

TEST(Categories, UpgradeKeepsOwnTransfersCategory) {
    TempDbPath file;
    sqlite3* raw = nullptr;
    ASSERT_EQ(sqlite3_open(file.path().string().c_str(), &raw), SQLITE_OK);
    for (int i = 0; i < 3; ++i) {
        ASSERT_EQ(sqlite3_exec(raw, migrations()[static_cast<std::size_t>(i)], nullptr, nullptr, nullptr),
                  SQLITE_OK);
    }
    ASSERT_EQ(sqlite3_exec(raw, "PRAGMA user_version = 3; INSERT INTO category (name) VALUES ('transfers');",
                           nullptr, nullptr, nullptr),
              SQLITE_OK);
    sqlite3_close(raw);

    const auto db = Database::open(file.path());
    EXPECT_EQ(db.schemaVersion(), 5);
    EXPECT_EQ(db.categories().size(), 8u);
    EXPECT_FALSE(inTotals(db, "transfers"));
}

std::optional<Cents> budgetOf(const Database& db, std::string_view name) {
    for (const Category& category : db.categories()) {
        if (category.name == name) {
            return category.budget;
        }
    }
    ADD_FAILURE() << "no category " << name;
    return std::nullopt;
}

TEST(Budgets, RoundTripAndClear) {
    TempDbPath file;
    {
        auto db = Database::open(file.path());
        EXPECT_EQ(budgetOf(db, "Groceries"), std::nullopt);
        db.setCategoryBudget(categoryId(db, "Groceries"), 35000);
    }
    auto db = Database::open(file.path());
    EXPECT_EQ(budgetOf(db, "Groceries"), 35000);
    db.setCategoryBudget(categoryId(db, "Groceries"), std::nullopt);
    EXPECT_EQ(budgetOf(db, "Groceries"), std::nullopt);
}

TEST(Budgets, RejectsBadValues) {
    TempDbPath file;
    auto db = Database::open(file.path());
    const auto groceries = categoryId(db, "Groceries");
    EXPECT_THROW(db.setCategoryBudget(groceries, 0), DatabaseError);
    EXPECT_THROW(db.setCategoryBudget(groceries, -500), DatabaseError);
    EXPECT_THROW(db.setCategoryBudget(999, 1000), DatabaseError);
    EXPECT_THROW(db.setCategoryBudget(categoryId(db, "Transfers"), 1000), DatabaseError);
    EXPECT_EQ(budgetOf(db, "Groceries"), std::nullopt);
}

TEST(Budgets, LeavingTotalsClearsTheBudget) {
    TempDbPath file;
    auto db = Database::open(file.path());
    const auto leisure = categoryId(db, "Leisure");
    db.setCategoryBudget(leisure, 8000);
    db.setCategoryInTotals(leisure, true);
    EXPECT_EQ(budgetOf(db, "Leisure"), 8000);
    db.setCategoryInTotals(leisure, false);
    EXPECT_EQ(budgetOf(db, "Leisure"), std::nullopt);
}

TEST(Budgets, UpgradeFromVersionFourHasNoBudgets) {
    TempDbPath file;
    sqlite3* raw = nullptr;
    ASSERT_EQ(sqlite3_open(file.path().string().c_str(), &raw), SQLITE_OK);
    for (int i = 0; i < 4; ++i) {
        ASSERT_EQ(sqlite3_exec(raw, migrations()[static_cast<std::size_t>(i)], nullptr, nullptr, nullptr),
                  SQLITE_OK);
    }
    ASSERT_EQ(sqlite3_exec(raw, "PRAGMA user_version = 4;", nullptr, nullptr, nullptr), SQLITE_OK);
    sqlite3_close(raw);

    const auto db = Database::open(file.path());
    EXPECT_EQ(db.schemaVersion(), 5);
    for (const Category& category : db.categories()) {
        EXPECT_EQ(category.budget, std::nullopt) << category.name;
    }
}

std::vector<std::string> rulePatterns(const Database& db) {
    std::vector<std::string> patterns;
    for (const Rule& rule : db.rules()) {
        patterns.push_back(rule.pattern);
    }
    return patterns;
}

TEST(Rules, KeepPriorityOrder) {
    TempDbPath file;
    auto db = Database::open(file.path());
    db.addRule("delhaize", categoryId(db, "Groceries"));
    db.addRule("shell", categoryId(db, "Transport"));
    db.addRule("  acme  ", categoryId(db, "Salary"));

    EXPECT_EQ(rulePatterns(db), (std::vector<std::string>{"delhaize", "shell", "acme"}));
    const auto rules = db.rules();
    EXPECT_EQ(rules[0].priority, 1);
    EXPECT_EQ(rules[2].priority, 3);
    EXPECT_EQ(rules[1].categoryId, categoryId(db, "Transport"));
}

TEST(Rules, MoveSwapsNeighbours) {
    TempDbPath file;
    auto db = Database::open(file.path());
    const auto groceries = categoryId(db, "Groceries");
    const auto a = db.addRule("a", groceries);
    const auto b = db.addRule("b", groceries);
    const auto c = db.addRule("c", groceries);

    EXPECT_TRUE(db.moveRule(c, MoveDirection::Up));
    EXPECT_EQ(rulePatterns(db), (std::vector<std::string>{"a", "c", "b"}));
    EXPECT_TRUE(db.moveRule(a, MoveDirection::Down));
    EXPECT_EQ(rulePatterns(db), (std::vector<std::string>{"c", "a", "b"}));

    EXPECT_FALSE(db.moveRule(c, MoveDirection::Up));
    EXPECT_FALSE(db.moveRule(b, MoveDirection::Down));
    EXPECT_EQ(rulePatterns(db), (std::vector<std::string>{"c", "a", "b"}));
}

TEST(Rules, UpdateAndDelete) {
    TempDbPath file;
    auto db = Database::open(file.path());
    const auto a = db.addRule("shel", categoryId(db, "Groceries"));
    const auto b = db.addRule("colruyt", categoryId(db, "Groceries"));

    db.updateRule(a, "shell", categoryId(db, "Transport"));
    db.deleteRule(b);

    const auto rules = db.rules();
    ASSERT_EQ(rules.size(), 1u);
    EXPECT_EQ(rules[0].id, a);
    EXPECT_EQ(rules[0].pattern, "shell");
    EXPECT_EQ(rules[0].categoryId, categoryId(db, "Transport"));
}

TEST(Rules, UnknownIdsThrow) {
    TempDbPath file;
    auto db = Database::open(file.path());
    EXPECT_THROW(db.updateRule(999, "x", categoryId(db, "Rent")), DatabaseError);
    EXPECT_THROW(db.deleteRule(999), DatabaseError);
    EXPECT_THROW(db.moveRule(999, MoveDirection::Up), DatabaseError);
}

TEST(Rules, RejectsUnknownCategoryAndBlankPattern) {
    TempDbPath file;
    auto db = Database::open(file.path());
    EXPECT_THROW(db.addRule("shell", 999), DatabaseError);
    EXPECT_THROW(db.addRule("   ", categoryId(db, "Transport")), DatabaseError);
    EXPECT_TRUE(db.rules().empty());
}

TEST(Rules, DeletingCategoryDeletesItsRules) {
    TempDbPath file;
    auto db = Database::open(file.path());
    const auto health = db.addCategory("Health");
    db.addRule("pharmacy", health);
    db.addRule("shell", categoryId(db, "Transport"));
    db.deleteCategory(health);
    EXPECT_EQ(rulePatterns(db), (std::vector<std::string>{"shell"}));
}

}
}
