#include "dialogs/column_roles.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <utility>

#include "carnet/text.hpp"

namespace carnet {

std::string mappingProblem(std::span<const ColumnRole> roles) {
    const auto count = [roles](ColumnRole role) {
        return std::ranges::count(roles, role);
    };
    if (count(ColumnRole::Date) == 0 || count(ColumnRole::Amount) == 0 ||
        count(ColumnRole::Description) == 0) {
        return "Choose the Date, Amount and Description columns";
    }
    constexpr std::array<std::pair<ColumnRole, const char*>, 5> names{{
        {ColumnRole::Date, "Date"},
        {ColumnRole::Amount, "Amount"},
        {ColumnRole::Description, "Description"},
        {ColumnRole::Counterparty, "Counterparty"},
        {ColumnRole::Status, "Status"},
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
    profile.colStatus.reset();
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
        case ColumnRole::Status:
            profile.colStatus = column;
            break;
        case ColumnRole::Ignore:
            break;
        }
    }
}

std::vector<ColumnRole> rolesFromProfile(const ImportProfile& profile, int minColumns) {
    const int highest = std::max({profile.colDate, profile.colAmount, profile.colDesc,
                                  profile.colParty.value_or(0), profile.colStatus.value_or(0)});
    std::vector<ColumnRole> roles(static_cast<std::size_t>(std::max(minColumns, highest + 1)),
                                  ColumnRole::Ignore);
    roles[static_cast<std::size_t>(profile.colDate)] = ColumnRole::Date;
    roles[static_cast<std::size_t>(profile.colAmount)] = ColumnRole::Amount;
    roles[static_cast<std::size_t>(profile.colDesc)] = ColumnRole::Description;
    if (profile.colParty) {
        roles[static_cast<std::size_t>(*profile.colParty)] = ColumnRole::Counterparty;
    }
    if (profile.colStatus) {
        roles[static_cast<std::size_t>(*profile.colStatus)] = ColumnRole::Status;
    }
    return roles;
}

std::vector<ColumnRole> suggestRoles(std::span<const std::string> header) {
    // folded names, most specific first; banks often have both a booking and a value date, and
    // both an account number and a name for the other party, so near names are left out on purpose
    struct Known {
        ColumnRole role;
        std::vector<std::string_view> names;
    };
    const std::array<Known, 5> known{{
        {ColumnRole::Date,
         {"date d'execution", "date de comptabilisation", "boekingsdatum", "uitvoeringsdatum", "booking date",
          "transaction date", "date", "datum"}},
        {ColumnRole::Amount, {"montant", "bedrag", "amount"}},
        {ColumnRole::Description,
         {"details", "detail", "omschrijving", "description", "libelle", "communication", "mededeling"}},
        {ColumnRole::Counterparty,
         {"nom de la contrepartie", "naam tegenpartij", "counterparty name", "tegenpartij", "counterparty",
          "beneficiaire"}},
        {ColumnRole::Status, {"statut", "status"}},
    }};

    std::vector<std::string> folded;
    for (const std::string& name : header) {
        folded.push_back(foldText(normalizeText(name)));
    }
    std::vector<ColumnRole> roles(header.size(), ColumnRole::Ignore);
    for (const Known& entry : known) {
        for (const std::string_view name : entry.names) {
            const auto match = std::ranges::find(folded, name);
            if (match != folded.end()) {
                roles[static_cast<std::size_t>(match - folded.begin())] = entry.role;
                break;
            }
        }
    }
    return roles;
}

std::string suggestSkipStatus(std::span<const std::string> values) {
    // French, Dutch and English, the languages the header suggestions cover
    constexpr std::array<std::string_view, 5> refusals{"refus", "rejet", "reject", "geweigerd", "declin"};
    for (const std::string& value : values) {
        const std::string folded = foldText(value);
        if (std::ranges::any_of(refusals, [&](std::string_view word) { return folded.contains(word); })) {
            return normalizeText(value);
        }
    }
    return {};
}

}
