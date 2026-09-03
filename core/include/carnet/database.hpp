#pragma once

#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "carnet/import.hpp"

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

private:
    struct Closer {
        void operator()(sqlite3* db) const;
    };

    explicit Database(sqlite3* db);
    void migrate();

    std::unique_ptr<sqlite3, Closer> db_;
};

}
