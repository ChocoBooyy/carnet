#include <gtest/gtest.h>

#include <sqlite3.h>

#include <chrono>
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
                                                           "Salary", "Transport", "Utilities"}));
}

TEST(Categories, AddRenameDelete) {
    TempDbPath file;
    auto db = Database::open(file.path());
    const auto id = db.addCategory("  Health  ");
    EXPECT_EQ(categoryId(db, "Health"), id);
    db.renameCategory(id, "Health care");
    EXPECT_EQ(categoryId(db, "Health care"), id);
    db.deleteCategory(id);
    EXPECT_EQ(db.categories().size(), 7u);
}

TEST(Categories, DuplicateNameRejectedIgnoringCase) {
    TempDbPath file;
    auto db = Database::open(file.path());
    EXPECT_THROW(db.addCategory("groceries"), DatabaseError);
    EXPECT_THROW(db.renameCategory(categoryId(db, "Rent"), "SALARY"), DatabaseError);
    EXPECT_THROW(db.addCategory("   "), DatabaseError);
    EXPECT_EQ(db.categories().size(), 7u);
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
    EXPECT_EQ(db.schemaVersion(), 2);
    EXPECT_EQ(db.transactionCount(), 1);
    EXPECT_EQ(db.categories().size(), 7u);

    const auto txns = db.transactionsInMonth(std::chrono::year{2026} / std::chrono::March);
    ASSERT_EQ(txns.size(), 1u);
    EXPECT_EQ(txns[0].categoryId, std::nullopt);
    EXPECT_FALSE(txns[0].manual);
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
