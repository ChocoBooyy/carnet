#pragma once

#include <QString>

#include <optional>
#include <string>
#include <string_view>

namespace carnet {

// bank exports arrive as UTF-8 or Windows-1252; core only reads UTF-8
std::string toUtf8Text(std::string_view bytes);

// the whole file as UTF-8, or nothing when it cannot be read
std::optional<std::string> readTextFile(const QString& path);

}
