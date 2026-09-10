#pragma once

#include <QColor>
#include <QPalette>

namespace carnet {

// the one accent color the brief allows, for money past its budget
inline QColor overBudgetColor() {
    return QColor(0xc6, 0x28, 0x28);
}

// the dark green that reads well on a light background disappears on a dark one
inline QColor incomeColor(const QPalette& palette) {
    return palette.color(QPalette::Base).lightness() < 128 ? QColor(0x5c, 0xc9, 0x7b)
                                                           : QColor(0x1b, 0x7f, 0x3a);
}

}
