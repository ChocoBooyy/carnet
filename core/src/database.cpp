#include "carnet/database.hpp"

#include <sqlite3.h>

#include <format>

#include "carnet/date.hpp"
#include "migrations.hpp"
#include "statement.hpp"

namespace carnet {

namespace {

class Transaction {
public:
    explicit Transaction(sqlite3* db) : db_(db) {
        exec(db_, "BEGIN");
    }

    ~Transaction() {
        if (!committed_) {
            // best effort: the error that got us here is already on its way up
            sqlite3_exec(db_, "ROLLBACK", nullptr, nullptr, nullptr);
        }
    }

    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;

    void commit() {
        exec(db_, "COMMIT");
        committed_ = true;
    }

private:
    sqlite3* db_;
    bool committed_ = false;
};

}

void Database::Closer::operator()(sqlite3* db) const {
    sqlite3_close(db);
}

Database::Database(sqlite3* db) : db_(db) {
}

Database Database::open(const std::filesystem::path& path) {
    const std::u8string utf8 = path.u8string();
    sqlite3* raw = nullptr;
    const int rc = sqlite3_open_v2(reinterpret_cast<const char*>(utf8.c_str()), &raw,
                                   SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    Database db(raw);
    if (rc != SQLITE_OK) {
        throw DatabaseError(raw != nullptr ? sqlite3_errmsg(raw) : "out of memory");
    }
    exec(raw, "PRAGMA foreign_keys = ON");
    db.migrate();
    return db;
}

int Database::schemaVersion() const {
    Statement stmt(db_.get(), "PRAGMA user_version");
    stmt.step();
    return static_cast<int>(stmt.integer(0));
}

bool Database::foreignKeysEnabled() const {
    Statement stmt(db_.get(), "PRAGMA foreign_keys");
    stmt.step();
    return stmt.integer(0) == 1;
}

std::vector<ImportProfile> Database::profiles() const {
    Statement stmt(db_.get(), "SELECT name, separator, decimal_mark, date_format, skip_lines, col_date, "
                              "col_amount, col_desc, col_party FROM import_profile ORDER BY name");
    std::vector<ImportProfile> result;
    while (stmt.step()) {
        ImportProfile profile;
        profile.name = stmt.text(0);
        profile.separator = stmt.text(1).front();
        profile.decimalMark = stmt.text(2).front();
        profile.dateFormat = stmt.text(3);
        profile.skipLines = static_cast<int>(stmt.integer(4));
        profile.colDate = static_cast<int>(stmt.integer(5));
        profile.colAmount = static_cast<int>(stmt.integer(6));
        profile.colDesc = static_cast<int>(stmt.integer(7));
        if (!stmt.isNull(8)) {
            profile.colParty = static_cast<int>(stmt.integer(8));
        }
        result.push_back(std::move(profile));
    }
    return result;
}

void Database::saveProfile(const ImportProfile& profile) {
    Statement stmt(db_.get(),
                   "INSERT INTO import_profile (name, separator, decimal_mark, date_format, skip_lines, "
                   "col_date, col_amount, col_desc, col_party) VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9) "
                   "ON CONFLICT (name) DO UPDATE SET separator = excluded.separator, "
                   "decimal_mark = excluded.decimal_mark, date_format = excluded.date_format, "
                   "skip_lines = excluded.skip_lines, col_date = excluded.col_date, "
                   "col_amount = excluded.col_amount, col_desc = excluded.col_desc, "
                   "col_party = excluded.col_party");
    stmt.bind(1, profile.name)
        .bind(2, std::string_view(&profile.separator, 1))
        .bind(3, std::string_view(&profile.decimalMark, 1))
        .bind(4, profile.dateFormat)
        .bind(5, std::int64_t{profile.skipLines})
        .bind(6, std::int64_t{profile.colDate})
        .bind(7, std::int64_t{profile.colAmount})
        .bind(8, std::int64_t{profile.colDesc});
    if (profile.colParty) {
        stmt.bind(9, std::int64_t{*profile.colParty});
    } else {
        stmt.bindNull(9);
    }
    stmt.step();
}

ImportSummary Database::importTransactions(std::string_view accountName,
                                           const std::vector<TxnDraft>& drafts) {
    Transaction transaction(db_.get());

    Statement(db_.get(), "INSERT INTO account (name) VALUES (?1) ON CONFLICT (name) DO NOTHING")
        .bind(1, accountName)
        .step();
    Statement findAccount(db_.get(), "SELECT id FROM account WHERE name = ?1");
    findAccount.bind(1, accountName).step();
    const std::int64_t accountId = findAccount.integer(0);

    Statement insert(db_.get(), "INSERT INTO txn (account_id, booked_on, amount_cents, description, "
                                "counterparty, occurrence) VALUES (?1, ?2, ?3, ?4, ?5, ?6) "
                                "ON CONFLICT DO NOTHING");
    ImportSummary summary;
    for (const TxnDraft& draft : drafts) {
        insert.bind(1, accountId)
            .bind(2, toIsoString(draft.bookedOn))
            .bind(3, draft.amount)
            .bind(4, draft.description)
            .bind(6, std::int64_t{draft.occurrence});
        if (draft.counterparty.empty()) {
            insert.bindNull(5);
        } else {
            insert.bind(5, draft.counterparty);
        }
        insert.step();
        if (sqlite3_changes(db_.get()) == 1) {
            ++summary.added;
        } else {
            ++summary.alreadyPresent;
        }
        insert.reset();
    }

    transaction.commit();
    return summary;
}

int Database::transactionCount() const {
    Statement stmt(db_.get(), "SELECT count(*) FROM txn");
    stmt.step();
    return static_cast<int>(stmt.integer(0));
}

int Database::accountCount() const {
    Statement stmt(db_.get(), "SELECT count(*) FROM account");
    stmt.step();
    return static_cast<int>(stmt.integer(0));
}

void Database::migrate() {
    const auto scripts = migrations();
    const int current = schemaVersion();
    if (current > static_cast<int>(scripts.size())) {
        throw DatabaseError(std::format("database version {} is newer than this app", current));
    }
    for (int version = current; version < static_cast<int>(scripts.size()); ++version) {
        Transaction transaction(db_.get());
        exec(db_.get(), scripts[static_cast<std::size_t>(version)]);
        exec(db_.get(), std::format("PRAGMA user_version = {}", version + 1).c_str());
        transaction.commit();
    }
}

}
