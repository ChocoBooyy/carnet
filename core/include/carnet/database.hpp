#pragma once

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "carnet/import.hpp"
#include "carnet/rules.hpp"

struct sqlite3;

namespace carnet {

class DatabaseError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

struct ImportSummary {
    int added = 0;
    int alreadyPresent = 0;
};

struct Category {
    std::int64_t id = 0;
    std::string name;
    // false for money that only moves between his own accounts, so totals do not count it twice
    bool inTotals = true;

    bool operator==(const Category&) const = default;
};

struct Transaction {
    std::int64_t id = 0;
    std::chrono::year_month_day bookedOn;
    std::string account;
    Cents amount = 0;
    std::string description;
    std::string counterparty;
    std::optional<std::int64_t> categoryId;
    bool manual = false;
};

enum class MoveDirection { Up, Down };

class Database {
public:
    static Database open(const std::filesystem::path& path);

    int schemaVersion() const;
    bool foreignKeysEnabled() const;

    std::vector<ImportProfile> profiles() const;
    void saveProfile(const ImportProfile& profile);
    ImportSummary importTransactions(std::string_view accountName, const std::vector<TxnDraft>& drafts);

    int transactionCount() const;
    int accountCount() const;
    std::vector<std::string> accounts() const;

    std::vector<Category> categories() const;
    std::int64_t addCategory(std::string_view name);
    void renameCategory(std::int64_t id, std::string_view name);
    void deleteCategory(std::int64_t id);
    void setCategoryInTotals(std::int64_t id, bool inTotals);

    std::vector<Rule> rules() const;
    std::int64_t addRule(std::string_view pattern, std::int64_t categoryId);
    void updateRule(std::int64_t id, std::string_view pattern, std::int64_t categoryId);
    void deleteRule(std::int64_t id);
    bool moveRule(std::int64_t id, MoveDirection direction);

    std::vector<Transaction> transactionsInMonth(std::chrono::year_month month) const;
    void setCategory(std::int64_t txnId, std::optional<std::int64_t> categoryId);
    void clearManualCategory(std::int64_t txnId);
    int applyRules();

private:
    struct Closer {
        void operator()(sqlite3* db) const;
    };

    explicit Database(sqlite3* db);
    void migrate();
    int recategorize();

    std::unique_ptr<sqlite3, Closer> db_;
};

}
