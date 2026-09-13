#pragma once

#include <QTemporaryDir>
#include <QTest>

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string_view>

#include "carnet/database.hpp"

// a database file in a folder that disappears with the test object; fresh() starts each test clean
class TestDatabase {
public:
    carnet::Database& fresh() {
        db_.reset();
        std::filesystem::remove(path());
        db_ = carnet::Database::open(path());
        return *db_;
    }

    // closes and opens the same file, to prove something was really saved
    carnet::Database& reopen() {
        db_.reset();
        db_ = carnet::Database::open(path());
        return *db_;
    }

    carnet::Database& operator*() {
        return *db_;
    }

    carnet::Database* operator->() {
        return &*db_;
    }

    const carnet::Database* operator->() const {
        return &*db_;
    }

    QString filePath(const QString& name) const {
        return dir_.filePath(name);
    }

    std::int64_t categoryId(std::string_view name) const {
        for (const carnet::Category& category : db_->categories()) {
            if (category.name == name) {
                return category.id;
            }
        }
        QTest::qFail("no such category", __FILE__, __LINE__);
        return 0;
    }

private:
    std::filesystem::path path() const {
        return std::filesystem::path(dir_.filePath("carnet.db").toStdWString());
    }

    QTemporaryDir dir_;
    std::optional<carnet::Database> db_;
};
