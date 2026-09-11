#pragma once

#include <string>
#include <string_view>

namespace carnet {

// trims and collapses runs of whitespace to one space, so bank padding does not make rows differ
std::string normalizeText(std::string_view text);

// lowercase with Latin accents removed, for matching that ignores case and accents
std::string foldText(std::string_view text);

}
