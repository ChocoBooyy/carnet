#pragma once

#include <gtest/gtest.h>

#include <filesystem>
#include <format>
#include <system_error>

namespace carnet {

class TempDbPath {
public:
    TempDbPath() {
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        path_ = std::filesystem::temp_directory_path() /
                std::format("carnet_{}_{}.db", info->test_suite_name(), info->name());
        std::filesystem::remove(path_);
    }

    ~TempDbPath() {
        std::error_code ignored;
        std::filesystem::remove(path_, ignored);
    }

    TempDbPath(const TempDbPath&) = delete;
    TempDbPath& operator=(const TempDbPath&) = delete;

    const std::filesystem::path& path() const {
        return path_;
    }

private:
    std::filesystem::path path_;
};

}
