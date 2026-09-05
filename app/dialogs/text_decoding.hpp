#pragma once

#include <string>
#include <string_view>

namespace carnet {

// bank exports arrive as UTF-8 or Windows-1252; core only reads UTF-8
std::string toUtf8Text(std::string_view bytes);

}
