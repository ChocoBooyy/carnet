#include "carnet/database.hpp"

#include <sqlite3.h>

#include <format>
#include <utility>

#include "carnet/date.hpp"
#include "migrations.hpp"
#include "statement.hpp"

namespace carnet {

namespace {

class SqlTransaction {
public:
    explicit SqlTransaction(sqlite3* db) : db_(db) {
        exec(db_, "BEGIN");
    }

    ~SqlTransaction() {
        if (!committed_) {
            // best effort: the error that got us here is already on its way up
            sqlite3_exec(db_, "ROLLBACK", nullptr, nullptr, nullptr);
        }
    }

    SqlTransaction(const SqlTransaction&) = delete;
    SqlTransaction& operator=(const SqlTransaction&) = delete;

    void commit() {
        exec(db_, "COMMIT");
        committed_ = true;
    }

private:
    sqlite3* db_;
    bool committed_ = false;
};

void expectOneChange(sqlite3* db, std::string_view what, std::int64_t id) {
    if (sqlite3_changes(db) != 1) {
        throw DatabaseError(std::format("{} {} not found", what, id));
    }
}

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
    SqlTransaction transaction(db_.get());

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

    recategorize();
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

std::vector<Category> Database::categories() const {
    Statement stmt(db_.get(), "SELECT id, name FROM category ORDER BY name");
    std::vector<Category> result;
    while (stmt.step()) {
        result.push_back({stmt.integer(0), stmt.text(1)});
    }
    return result;
}

std::int64_t Database::addCategory(std::string_view name) {
    Statement(db_.get(), "INSERT INTO category (name) VALUES (?1)").bind(1, normalizeText(name)).step();
    return sqlite3_last_insert_rowid(db_.get());
}

void Database::renameCategory(std::int64_t id, std::string_view name) {
    Statement(db_.get(), "UPDATE category SET name = ?2 WHERE id = ?1")
        .bind(1, id)
        .bind(2, normalizeText(name))
        .step();
    expectOneChange(db_.get(), "category", id);
}

void Database::deleteCategory(std::int64_t id) {
    SqlTransaction transaction(db_.get());
    Statement(db_.get(), "UPDATE txn SET category_manual = 0, category_id = NULL WHERE category_id = ?1")
        .bind(1, id)
        .step();
    Statement(db_.get(), "DELETE FROM category WHERE id = ?1").bind(1, id).step();
    expectOneChange(db_.get(), "category", id);
    recategorize();
    transaction.commit();
}

std::vector<Rule> Database::rules() const {
    Statement stmt(db_.get(), "SELECT id, priority, pattern, category_id FROM rule ORDER BY priority");
    std::vector<Rule> result;
    while (stmt.step()) {
        result.push_back({stmt.integer(0), static_cast<int>(stmt.integer(1)), stmt.text(2), stmt.integer(3)});
    }
    return result;
}

std::int64_t Database::addRule(std::string_view pattern, std::int64_t categoryId) {
    Statement(db_.get(), "INSERT INTO rule (priority, pattern, category_id) "
                         "VALUES ((SELECT coalesce(max(priority), 0) + 1 FROM rule), ?1, ?2)")
        .bind(1, normalizeText(pattern))
        .bind(2, categoryId)
        .step();
    return sqlite3_last_insert_rowid(db_.get());
}

void Database::updateRule(std::int64_t id, std::string_view pattern, std::int64_t categoryId) {
    Statement(db_.get(), "UPDATE rule SET pattern = ?2, category_id = ?3 WHERE id = ?1")
        .bind(1, id)
        .bind(2, normalizeText(pattern))
        .bind(3, categoryId)
        .step();
    expectOneChange(db_.get(), "rule", id);
}

void Database::deleteRule(std::int64_t id) {
    Statement(db_.get(), "DELETE FROM rule WHERE id = ?1").bind(1, id).step();
    expectOneChange(db_.get(), "rule", id);
}

bool Database::moveRule(std::int64_t id, MoveDirection direction) {
    SqlTransaction transaction(db_.get());

    Statement current(db_.get(), "SELECT priority FROM rule WHERE id = ?1");
    if (!current.bind(1, id).step()) {
        throw DatabaseError(std::format("rule {} not found", id));
    }
    const std::int64_t priority = current.integer(0);

    Statement neighbour(db_.get(), direction == MoveDirection::Up
                                       ? "SELECT id, priority FROM rule WHERE priority < ?1 "
                                         "ORDER BY priority DESC LIMIT 1"
                                       : "SELECT id, priority FROM rule WHERE priority > ?1 "
                                         "ORDER BY priority LIMIT 1");
    if (!neighbour.bind(1, priority).step()) {
        return false;
    }
    const std::int64_t otherId = neighbour.integer(0);
    const std::int64_t otherPriority = neighbour.integer(1);

    const auto setPriority = [this](std::int64_t ruleId, std::int64_t value) {
        Statement(db_.get(), "UPDATE rule SET priority = ?2 WHERE id = ?1")
            .bind(1, ruleId)
            .bind(2, value)
            .step();
    };
    // priority is UNIQUE, so the rule waits on a free value while its neighbour takes its place
    setPriority(id, -1);
    setPriority(otherId, priority);
    setPriority(id, otherPriority);

    transaction.commit();
    return true;
}

std::vector<Transaction> Database::transactionsInMonth(std::chrono::year_month month) const {
    Statement stmt(db_.get(), "SELECT id, booked_on, amount_cents, description, counterparty, category_id, "
                              "category_manual FROM txn WHERE booked_on >= ?1 AND booked_on < ?2 "
                              "ORDER BY booked_on, id");
    stmt.bind(1, toIsoString(month / 1)).bind(2, toIsoString((month + std::chrono::months{1}) / 1));

    std::vector<Transaction> result;
    while (stmt.step()) {
        const auto bookedOn = parseDate(stmt.text(1), "yyyy-MM-dd");
        if (!bookedOn) {
            throw DatabaseError(bookedOn.error().message);
        }
        Transaction txn;
        txn.id = stmt.integer(0);
        txn.bookedOn = *bookedOn;
        txn.amount = stmt.integer(2);
        txn.description = stmt.text(3);
        txn.counterparty = stmt.text(4);
        if (!stmt.isNull(5)) {
            txn.categoryId = stmt.integer(5);
        }
        txn.manual = stmt.integer(6) == 1;
        result.push_back(std::move(txn));
    }
    return result;
}

void Database::setCategory(std::int64_t txnId, std::optional<std::int64_t> categoryId) {
    Statement stmt(db_.get(), "UPDATE txn SET category_id = ?2, category_manual = 1 WHERE id = ?1");
    stmt.bind(1, txnId);
    if (categoryId) {
        stmt.bind(2, *categoryId);
    } else {
        stmt.bindNull(2);
    }
    stmt.step();
    expectOneChange(db_.get(), "transaction", txnId);
}

void Database::clearManualCategory(std::int64_t txnId) {
    SqlTransaction transaction(db_.get());
    Statement(db_.get(), "UPDATE txn SET category_manual = 0 WHERE id = ?1").bind(1, txnId).step();
    expectOneChange(db_.get(), "transaction", txnId);
    recategorize();
    transaction.commit();
}

int Database::applyRules() {
    SqlTransaction transaction(db_.get());
    const int changed = recategorize();
    transaction.commit();
    return changed;
}

void Database::migrate() {
    const auto scripts = migrations();
    const int current = schemaVersion();
    if (current > static_cast<int>(scripts.size())) {
        throw DatabaseError(std::format("database version {} is newer than this app", current));
    }
    for (int version = current; version < static_cast<int>(scripts.size()); ++version) {
        SqlTransaction transaction(db_.get());
        exec(db_.get(), scripts[static_cast<std::size_t>(version)]);
        exec(db_.get(), std::format("PRAGMA user_version = {}", version + 1).c_str());
        transaction.commit();
    }
}

int Database::recategorize() {
    const std::vector<Rule> currentRules = rules();

    // collect first: updating the rows a live SELECT is reading has no guaranteed order in SQLite
    std::vector<std::pair<std::int64_t, std::optional<std::int64_t>>> changes;
    Statement select(db_.get(), "SELECT id, description, counterparty, category_id FROM txn "
                                "WHERE category_manual = 0");
    while (select.step()) {
        const auto match = matchCategory(currentRules, select.text(1), select.text(2));
        const auto current = select.isNull(3) ? std::nullopt : std::optional<std::int64_t>(select.integer(3));
        if (match != current) {
            changes.emplace_back(select.integer(0), match);
        }
    }

    Statement update(db_.get(), "UPDATE txn SET category_id = ?2 WHERE id = ?1");
    for (const auto& [txnId, categoryId] : changes) {
        update.bind(1, txnId);
        if (categoryId) {
            update.bind(2, *categoryId);
        } else {
            update.bindNull(2);
        }
        update.step();
        update.reset();
    }
    return static_cast<int>(changes.size());
}

}
