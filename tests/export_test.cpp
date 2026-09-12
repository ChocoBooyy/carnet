#include <gtest/gtest.h>

#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include "carnet/export.hpp"

namespace carnet {
namespace {

using namespace std::chrono;

constexpr std::string_view bom = "\xEF\xBB\xBF";

TEST(ExportSummary, MatchesTheOverview) {
    const MonthOverview overview{220500,
                                 57450,
                                 {{1, "Groceries", 30000, 35000},
                                  {2, "Eating out", 18800, 15000},
                                  {3, "Transport", 7400, std::nullopt},
                                  {std::nullopt, "", 1250, std::nullopt}}};
    EXPECT_EQ(summaryCsv(2026y / September, overview, "Uncategorized"), std::string(bom) +
                                                                            "Month;09/2026\r\n"
                                                                            "Income;2205,00\r\n"
                                                                            "Spent;574,50\r\n"
                                                                            "Left;1630,50\r\n"
                                                                            "\r\n"
                                                                            "Category;Spent;Budget\r\n"
                                                                            "Groceries;300,00;350,00\r\n"
                                                                            "Eating out;188,00;150,00\r\n"
                                                                            "Transport;74,00;\r\n"
                                                                            "Uncategorized;12,50;\r\n");
}

TEST(ExportSummary, NegativeLeft) {
    const MonthOverview overview{1000, 2550, {}};
    EXPECT_NE(summaryCsv(2026y / January, overview, "-").find("Left;-15,50\r\n"), std::string::npos);
}

Transaction txn(year_month_day date, Cents amount, std::string description, std::string counterparty,
                std::optional<std::int64_t> category) {
    Transaction t;
    t.bookedOn = date;
    t.account = "Current";
    t.amount = amount;
    t.description = std::move(description);
    t.counterparty = std::move(counterparty);
    t.categoryId = category;
    return t;
}

const std::vector<Category> categories{{1, "Groceries", true, 35000}, {2, "Transfers", false, std::nullopt}};

TEST(ExportTransactions, WritesEveryColumn) {
    const std::vector<Transaction> rows{txn(2026y / September / 2, -12000, "Delhaize", "", 1),
                                        txn(2026y / September / 3, -50000, "To savings", "Me", 2),
                                        txn(2026y / September / 4, 4000, "Friend", "Ana", std::nullopt)};
    EXPECT_EQ(transactionsCsv(rows, categories, "Uncategorized"),
              std::string(bom) + "Date;Account;Description;Counterparty;Amount;Category;In totals\r\n"
                                 "02/09/2026;Current;Delhaize;;-120,00;Groceries;yes\r\n"
                                 "03/09/2026;Current;To savings;Me;-500,00;Transfers;no\r\n"
                                 "04/09/2026;Current;Friend;Ana;40,00;Uncategorized;yes\r\n");
}

TEST(ExportTransactions, QuotesSpecialCharacters) {
    const std::vector<Transaction> rows{txn(2026y / September / 2, -100, "a;b \"c\"\nd", "x", std::nullopt)};
    const std::string csv = transactionsCsv(rows, categories, "U");
    EXPECT_NE(csv.find("02/09/2026;Current;\"a;b \"\"c\"\"\nd\";x;-1,00;U;yes\r\n"), std::string::npos)
        << csv;
}

TEST(ExportTransactions, NeutralizesFormulas) {
    const std::vector<Transaction> rows{
        txn(2026y / September / 2, -100, "=HYPERLINK(\"x\")", "+32 470", std::nullopt),
        txn(2026y / September / 3, -200, "@sum", "-dash", std::nullopt)};
    const std::string csv = transactionsCsv(rows, categories, "U");
    EXPECT_NE(csv.find(";\"'=HYPERLINK(\"\"x\"\")\";'+32 470;-1,00;"), std::string::npos) << csv;
    EXPECT_NE(csv.find(";'@sum;'-dash;-2,00;"), std::string::npos) << csv;
}

}
}
