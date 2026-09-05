#pragma once

#include <gtest/gtest.h>

#include <cstdint>
#include <string_view>

#include "carnet/database.hpp"

namespace carnet {

inline std::int64_t categoryId(const Database& db, std::string_view name) {
    for (const Category& category : db.categories()) {
        if (category.name == name) {
            return category.id;
        }
    }
    ADD_FAILURE() << "no category named " << name;
    return 0;
}

}
