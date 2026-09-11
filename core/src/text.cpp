#include "carnet/text.hpp"

#include <algorithm>
#include <array>

namespace carnet {

namespace {

bool isSpace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

struct Fold {
    char32_t first;
    char32_t last;
    std::string_view to;
};

constexpr std::array<Fold, 14> folds{{
    {0xDF, 0xDF, "ss"},
    {0xE0, 0xE5, "a"},
    {0xE6, 0xE6, "ae"},
    {0xE7, 0xE7, "c"},
    {0xE8, 0xEB, "e"},
    {0xEC, 0xEF, "i"},
    {0xF1, 0xF1, "n"},
    {0xF2, 0xF6, "o"},
    {0xF8, 0xF8, "o"},
    {0xF9, 0xFC, "u"},
    {0xFD, 0xFD, "y"},
    {0xFF, 0xFF, "y"},
    {0x152, 0x153, "oe"},
    {0x178, 0x178, "y"},
}};

std::string_view foldLetter(char32_t codePoint) {
    // latin-1 capitals sit exactly 0x20 below their small letters, except the multiplication sign
    if (codePoint >= 0xC0 && codePoint <= 0xDE && codePoint != 0xD7) {
        codePoint += 0x20;
    }
    const auto fold = std::ranges::find_if(
        folds, [codePoint](const Fold& f) { return codePoint >= f.first && codePoint <= f.last; });
    return fold == folds.end() ? std::string_view{} : fold->to;
}

}

std::string foldText(std::string_view text) {
    std::string result;
    result.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        const auto byte = static_cast<unsigned char>(text[i]);
        if (byte >= 'A' && byte <= 'Z') {
            result += static_cast<char>(byte - 'A' + 'a');
            continue;
        }
        const bool twoByteLead = (byte & 0xE0) == 0xC0;
        if (twoByteLead && i + 1 < text.size()) {
            const auto next = static_cast<unsigned char>(text[i + 1]);
            if ((next & 0xC0) == 0x80) {
                const auto folded = foldLetter(static_cast<char32_t>(((byte & 0x1Fu) << 6) | (next & 0x3Fu)));
                if (!folded.empty()) {
                    result += folded;
                    ++i;
                    continue;
                }
            }
        }
        result += text[i];
    }
    return result;
}

std::string normalizeText(std::string_view text) {
    std::string result;
    bool pendingSpace = false;
    for (const char c : text) {
        if (isSpace(c)) {
            pendingSpace = !result.empty();
            continue;
        }
        if (pendingSpace) {
            result += ' ';
            pendingSpace = false;
        }
        result += c;
    }
    return result;
}

}
