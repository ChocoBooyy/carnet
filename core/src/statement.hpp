#pragma once

#include <cstdint>
#include <string>
#include <string_view>

struct sqlite3;
struct sqlite3_stmt;

namespace carnet {

class Statement {
public:
    Statement(sqlite3* db, std::string_view sql);
    ~Statement();

    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;

    Statement& bind(int index, std::string_view value);
    Statement& bind(int index, std::int64_t value);
    Statement& bindNull(int index);

    bool step();
    void reset();

    std::int64_t integer(int column) const;
    std::string text(int column) const;
    bool isNull(int column) const;

private:
    sqlite3* db_;
    sqlite3_stmt* stmt_ = nullptr;
};

void exec(sqlite3* db, const char* sql);

}
