#pragma once

#include <span>
#include <string>
#include <vector>

#include "carnet/import.hpp"

namespace carnet {

// the order is the order of the choices in the import dialog
enum class ColumnRole { Ignore, Date, Amount, Description, Counterparty, Status };

std::string mappingProblem(std::span<const ColumnRole> roles);
void applyRoles(std::span<const ColumnRole> roles, ImportProfile& profile);
std::vector<ColumnRole> rolesFromProfile(const ImportProfile& profile, int minColumns);

// a guess from the header row's column names, Ignore where no name is known
std::vector<ColumnRole> suggestRoles(std::span<const std::string> header);

// the first value that reads like a refusal in the bank's language, or empty when none does
std::string suggestSkipStatus(std::span<const std::string> values);

}
