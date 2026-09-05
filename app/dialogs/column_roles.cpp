#include "dialogs/column_roles.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <utility>

namespace carnet {

std::string mappingProblem(std::span<const ColumnRole> roles) {
    const auto count = [roles](ColumnRole role) {
        return std::ranges::count(roles, role);
    };
    if (count(ColumnRole::Date) == 0 || count(ColumnRole::Amount) == 0 ||
        count(ColumnRole::Description) == 0) {
        return "Choose the Date, Amount and Description columns";
    }
    constexpr std::array<std::pair<ColumnRole, const char*>, 4> names{{
        {ColumnRole::Date, "Date"},
        {ColumnRole::Amount, "Amount"},
        {ColumnRole::Description, "Description"},
        {ColumnRole::Counterparty, "Counterparty"},
    }};
    for (const auto& [role, name] : names) {
        if (count(role) > 1) {
            return std::format("{} is chosen twice", name);
        }
    }
    return {};
}

void applyRoles(std::span<const ColumnRole> roles, ImportProfile& profile) {
    profile.colParty.reset();
    for (std::size_t i = 0; i < roles.size(); ++i) {
        const int column = static_cast<int>(i);
        switch (roles[i]) {
        case ColumnRole::Date:
            profile.colDate = column;
            break;
        case ColumnRole::Amount:
            profile.colAmount = column;
            break;
        case ColumnRole::Description:
            profile.colDesc = column;
            break;
        case ColumnRole::Counterparty:
            profile.colParty = column;
            break;
        case ColumnRole::Ignore:
            break;
        }
    }
}

std::vector<ColumnRole> rolesFromProfile(const ImportProfile& profile, int minColumns) {
    const int highest =
        std::max({profile.colDate, profile.colAmount, profile.colDesc, profile.colParty.value_or(0)});
    std::vector<ColumnRole> roles(static_cast<std::size_t>(std::max(minColumns, highest + 1)),
                                  ColumnRole::Ignore);
    roles[static_cast<std::size_t>(profile.colDate)] = ColumnRole::Date;
    roles[static_cast<std::size_t>(profile.colAmount)] = ColumnRole::Amount;
    roles[static_cast<std::size_t>(profile.colDesc)] = ColumnRole::Description;
    if (profile.colParty) {
        roles[static_cast<std::size_t>(*profile.colParty)] = ColumnRole::Counterparty;
    }
    return roles;
}

}
