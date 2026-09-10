#include <gtest/gtest.h>

#include <chrono>
#include <map>
#include <string>
#include <vector>

#include "carnet/database.hpp"
#include "db_lookup.hpp"
#include "temp_db.hpp"

namespace carnet {
namespace {

using namespace std::chrono;

struct Row {
    year_month_day date;
    Cents amount;
    std::string description;
    std::string category; // empty: uncategorized
};

// September 2026, worked out by hand:
//   Salary      +2150,00                          -> income
//   Groceries   -120,00 -200,00 +20,00 = -300,00  -> bar 300,00 / 350,00
//   Eating out  -95,00 -93,00          = -188,00  -> bar 188,00 / 150,00 (over)
//   Transport   -64,00 -10,00          = -74,00   -> bar 74,00, no budget
//   Leisure     -30,00 +45,00          = +15,00   -> income, no bar
//   Utilities   -30,00 +30,00          = 0        -> nothing
//   uncategorized -12,50 (bar), +40,00 (income)
//   Transfers   -500,00                           -> ignored
//   Income = 2150,00 + 15,00 + 40,00 = 2205,00
//   Spent  = 300,00 + 188,00 + 74,00 + 12,50 = 574,50
// Outside the month: 31 August groceries -99,00, 1 October eating out -50,00.
const std::vector<Row> fixture{
    {2026y / August / 31, -9900, "Delhaize august", "Groceries"},
    {2026y / September / 1, 215000, "Salary september", "Salary"},
    {2026y / September / 2, -12000, "Delhaize", "Groceries"},
    {2026y / September / 3, -3000, "Water bill", "Utilities"},
    {2026y / September / 5, -9500, "Restaurant", "Eating out"},
    {2026y / September / 8, -3000, "Cinema", "Leisure"},
    {2026y / September / 10, -20000, "Colruyt", "Groceries"},
    {2026y / September / 12, 2000, "Colruyt refund", "Groceries"},
    {2026y / September / 14, -6400, "SNCB", "Transport"},
    {2026y / September / 15, 4500, "Cinema refund", "Leisure"},
    {2026y / September / 18, -1250, "Unknown shop", ""},
    {2026y / September / 19, 4000, "Friend paid back", ""},
    {2026y / September / 20, -9300, "Pizza", "Eating out"},
    {2026y / September / 21, -50000, "To savings", "Transfers"},
    {2026y / September / 25, 3000, "Water refund", "Utilities"},
    {2026y / September / 30, -1000, "Bus", "Transport"},
    {2026y / October / 1, -5000, "Lunch october", "Eating out"},
};

class OverviewQuery : public ::testing::Test {
protected:
    void SetUp() override {
        std::vector<TxnDraft> drafts;
        std::map<std::string, std::string> categoryOf;
        for (const Row& row : fixture) {
            drafts.push_back(TxnDraft{row.date, row.amount, row.description, "", 1});
            categoryOf[row.description] = row.category;
        }
        db_.importTransactions("Current", drafts);
        for (const auto month : {2026y / August, 2026y / September, 2026y / October}) {
            for (const Transaction& txn : db_.transactionsInMonth(month)) {
                const std::string& name = categoryOf.at(txn.description);
                if (!name.empty()) {
                    db_.setCategory(txn.id, categoryId(db_, name));
                }
            }
        }
        db_.setCategoryBudget(categoryId(db_, "Groceries"), 35000);
        db_.setCategoryBudget(categoryId(db_, "Eating out"), 15000);
    }

    TempDbPath file_;
    Database db_ = Database::open(file_.path());
};

TEST_F(OverviewQuery, HandCalculatedMonth) {
    const MonthOverview overview = db_.monthOverview(2026y / September);
    EXPECT_EQ(overview.income, 220500);
    EXPECT_EQ(overview.spent, 57450);
    const std::vector<CategorySpend> expected{
        {categoryId(db_, "Groceries"), "Groceries", 30000, 35000},
        {categoryId(db_, "Eating out"), "Eating out", 18800, 15000},
        {categoryId(db_, "Transport"), "Transport", 7400, std::nullopt},
        {std::nullopt, "Uncategorized", 1250, std::nullopt},
    };
    EXPECT_EQ(overview.categories, expected);
}

TEST_F(OverviewQuery, SixMonthsEndingAtSeptember) {
    const std::vector<MonthTotals> expected{
        {2026y / April, 0, 0}, {2026y / May, 0, 0},       {2026y / June, 0, 0},
        {2026y / July, 0, 0},  {2026y / August, 0, 9900}, {2026y / September, 220500, 57450},
    };
    EXPECT_EQ(db_.monthlyTotals(2026y / September, 6), expected);
}

TEST_F(OverviewQuery, TotalsCrossTheYear) {
    const auto months = db_.monthlyTotals(2026y / January, 6);
    ASSERT_EQ(months.size(), 6u);
    EXPECT_EQ(months.front().month, 2025y / August);
    EXPECT_EQ(months.back().month, 2026y / January);
}

TEST_F(OverviewQuery, OnlyIncomeMonth) {
    db_.importTransactions("Current", {TxnDraft{2026y / November / 1, 215000, "Salary november", "", 1}});
    db_.setCategory(db_.transactionsInMonth(2026y / November)[0].id, categoryId(db_, "Salary"));
    const MonthOverview overview = db_.monthOverview(2026y / November);
    EXPECT_EQ(overview.income, 215000);
    EXPECT_EQ(overview.spent, 0);
    EXPECT_TRUE(overview.categories.empty());
}

TEST_F(OverviewQuery, EmptyMonth) {
    EXPECT_EQ(db_.monthOverview(2025y / January), MonthOverview{});
}

}
}
