#include "views/main_window.hpp"

#include <QHBoxLayout>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>

#include "dialogs/import_dialog.hpp"
#include "views/overview_tab.hpp"
#include "views/transactions_tab.hpp"
#include "widgets/month_picker.hpp"

namespace carnet {

MainWindow::MainWindow(Database& db, QWidget* parent) : QMainWindow(parent), db_(db) {
    setWindowTitle(tr("Carnet"));

    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);

    auto* top = new QHBoxLayout;
    monthPicker_ = new MonthPicker(central);
    top->addWidget(monthPicker_);
    top->addStretch();
    auto* importButton = new QPushButton(tr("Import CSV..."), central);
    top->addWidget(importButton);
    layout->addLayout(top);

    auto* tabs = new QTabWidget(central);
    tabs->addTab(new OverviewTab(tabs), tr("Overview"));
    transactionsTab_ = new TransactionsTab(db_, tabs);
    tabs->addTab(transactionsTab_, tr("Transactions"));
    tabs->setCurrentWidget(transactionsTab_);
    layout->addWidget(tabs);

    setCentralWidget(central);
    resize(1000, 640);

    connect(monthPicker_, &MonthPicker::monthChanged, transactionsTab_, &TransactionsTab::setMonth);
    connect(importButton, &QPushButton::clicked, this, &MainWindow::openImport);
    transactionsTab_->setMonth(monthPicker_->month());
}

void MainWindow::openImport() {
    ImportDialog dialog(db_, this);
    connect(&dialog, &ImportDialog::imported, this, [this](std::chrono::year_month month) {
        monthPicker_->setMonth(month);
        transactionsTab_->reload();
    });
    dialog.exec();
}

}
