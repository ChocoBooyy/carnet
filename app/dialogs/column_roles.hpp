#pragma once

#include <span>
#include <string>
#include <vector>

#include "carnet/import.hpp"

namespace carnet {

enum class ColumnRole { Ignore, Date, Amount, Description, Counterparty };

std::string mappingProblem(std::span<const ColumnRole> roles);
void applyRoles(std::span<const ColumnRole> roles, ImportProfile& profile);
std::vector<ColumnRole> rolesFromProfile(const ImportProfile& profile, int minColumns);

}
