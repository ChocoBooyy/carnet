#pragma once

#include <QColor>

namespace carnet {

// the one accent color the brief allows, for money past its budget
inline QColor overBudgetColor() {
    return QColor(0xc6, 0x28, 0x28);
}

inline QColor incomeColor() {
    return QColor(0x1b, 0x7f, 0x3a);
}

}
