#include "migrations.hpp"

#include <array>

namespace carnet {

namespace {

// entry N brings a database to user_version N + 1; never edit an entry that has shipped, add a new one
constexpr std::array<const char*, 1> scripts{
    R"sql(
CREATE TABLE account (
  id   INTEGER PRIMARY KEY,
  name TEXT NOT NULL UNIQUE CHECK (length(trim(name)) > 0)
);

CREATE TABLE import_profile (
  id           INTEGER PRIMARY KEY,
  name         TEXT NOT NULL UNIQUE CHECK (length(trim(name)) > 0),
  separator    TEXT NOT NULL CHECK (separator IN (';', ',')),
  decimal_mark TEXT NOT NULL CHECK (decimal_mark IN (',', '.')),
  date_format  TEXT NOT NULL,
  skip_lines   INTEGER NOT NULL DEFAULT 1 CHECK (skip_lines >= 0),
  col_date     INTEGER NOT NULL CHECK (col_date >= 0),
  col_amount   INTEGER NOT NULL CHECK (col_amount >= 0),
  col_desc     INTEGER NOT NULL CHECK (col_desc >= 0),
  col_party    INTEGER CHECK (col_party IS NULL OR col_party >= 0)
);

CREATE TABLE txn (
  id           INTEGER PRIMARY KEY,
  account_id   INTEGER NOT NULL REFERENCES account(id),
  booked_on    TEXT NOT NULL CHECK (booked_on GLOB '[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]'),
  amount_cents INTEGER NOT NULL,
  description  TEXT NOT NULL,
  counterparty TEXT,
  occurrence   INTEGER NOT NULL CHECK (occurrence >= 1),
  UNIQUE (account_id, booked_on, amount_cents, description, occurrence)
);

CREATE INDEX txn_by_month ON txn (booked_on);
)sql",
};

}

std::span<const char* const> migrations() {
    return scripts;
}

}
