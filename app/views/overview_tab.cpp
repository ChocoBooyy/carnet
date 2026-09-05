#include "views/overview_tab.hpp"

#include <QLabel>
#include <QVBoxLayout>

namespace carnet {

OverviewTab::OverviewTab(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    auto* label = new QLabel(tr("The monthly overview is not built yet."), this);
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
}

}
