#include "carnet/rules.hpp"

#include "carnet/text.hpp"

namespace carnet {

std::optional<std::int64_t> matchCategory(std::span<const Rule> rules, std::string_view description,
                                          std::string_view counterparty) {
    const std::string foldedDescription = foldText(description);
    const std::string foldedCounterparty = foldText(counterparty);
    for (const Rule& rule : rules) {
        const std::string pattern = foldText(rule.pattern);
        if (pattern.empty()) {
            continue;
        }
        if (foldedDescription.contains(pattern) || foldedCounterparty.contains(pattern)) {
            return rule.categoryId;
        }
    }
    return std::nullopt;
}

}
