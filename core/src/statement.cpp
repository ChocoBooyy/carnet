#include "statement.hpp"

#include <sqlite3.h>

#include "carnet/database.hpp"

namespace carnet {

namespace {

[[noreturn]] void fail(sqlite3* db) {
    throw DatabaseError(sqlite3_errmsg(db));
}

}

Statement::Statement(sqlite3* db, std::string_view sql) : db_(db) {
    if (sqlite3_prepare_v2(db_, sql.data(), static_cast<int>(sql.size()), &stmt_, nullptr) != SQLITE_OK) {
        fail(db_);
    }
}

Statement::~Statement() {
    sqlite3_finalize(stmt_);
}

Statement& Statement::bind(int index, std::string_view value) {
    if (sqlite3_bind_text(stmt_, index, value.data(), static_cast<int>(value.size()), SQLITE_TRANSIENT) !=
        SQLITE_OK) {
        fail(db_);
    }
    return *this;
}

Statement& Statement::bind(int index, std::int64_t value) {
    if (sqlite3_bind_int64(stmt_, index, value) != SQLITE_OK) {
        fail(db_);
    }
    return *this;
}

Statement& Statement::bindNull(int index) {
    if (sqlite3_bind_null(stmt_, index) != SQLITE_OK) {
        fail(db_);
    }
    return *this;
}

bool Statement::step() {
    const int rc = sqlite3_step(stmt_);
    if (rc == SQLITE_ROW) {
        return true;
    }
    if (rc == SQLITE_DONE) {
        return false;
    }
    fail(db_);
}

void Statement::reset() {
    sqlite3_reset(stmt_);
    sqlite3_clear_bindings(stmt_);
}

std::int64_t Statement::integer(int column) const {
    return sqlite3_column_int64(stmt_, column);
}

std::string Statement::text(int column) const {
    const auto* chars = reinterpret_cast<const char*>(sqlite3_column_text(stmt_, column));
    if (chars == nullptr) {
        return {};
    }
    return std::string(chars, static_cast<std::size_t>(sqlite3_column_bytes(stmt_, column)));
}

bool Statement::isNull(int column) const {
    return sqlite3_column_type(stmt_, column) == SQLITE_NULL;
}

void exec(sqlite3* db, const char* sql) {
    char* message = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &message) != SQLITE_OK) {
        const std::string text = message != nullptr ? message : sqlite3_errmsg(db);
        sqlite3_free(message);
        throw DatabaseError(text);
    }
}

}
