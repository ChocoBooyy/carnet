#include "views/main_window.hpp"

#include <QHBoxLayout>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>

#include "database_call.hpp"
#include "dialogs/import_dialog.hpp"
#include "dialogs/rules_dialog.hpp"
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
    auto* rulesButton = new QPushButton(tr("Rules..."), central);
    top->addWidget(rulesButton);
    layout->addLayout(top);

    auto* tabs = new QTabWidget(central);
    overviewTab_ = new OverviewTab(db_, tabs);
    tabs->addTab(overviewTab_, tr("Overview"));
    transactionsTab_ = new TransactionsTab(db_, tabs);
    tabs->addTab(transactionsTab_, tr("Transactions"));
    tabs->setCurrentWidget(transactionsTab_);
    layout->addWidget(tabs);

    setCentralWidget(central);
    resize(1000, 640);

    connect(monthPicker_, &MonthPicker::monthChanged, transactionsTab_, &TransactionsTab::setMonth);
    connect(monthPicker_, &MonthPicker::monthChanged, overviewTab_, &OverviewTab::setMonth);
    // category edits happen in the Transactions tab, so coming back to the Overview picks them up
    connect(tabs, &QTabWidget::currentChanged, this, [this, tabs](int index) {
        if (tabs->widget(index) == overviewTab_) {
            overviewTab_->reload();
        }
    });
    connect(importButton, &QPushButton::clicked, this, &MainWindow::openImport);
    connect(rulesButton, &QPushButton::clicked, this, &MainWindow::openRules);
    transactionsTab_->setMonth(monthPicker_->month());
    overviewTab_->setMonth(monthPicker_->month());
}

void MainWindow::openImport() {
    ImportDialog dialog(db_, this);
    connect(&dialog, &ImportDialog::imported, this, [this](std::chrono::year_month month) {
        monthPicker_->setMonth(month);
        transactionsTab_->reload();
        overviewTab_->reload();
    });
    dialog.exec();
}

void MainWindow::openRules() {
    RulesDialog dialog(db_, this);
    dialog.exec();
    if (dialog.changed()) {
        tryDatabase(this, [&] { db_.applyRules(); });
        transactionsTab_->reload();
        overviewTab_->reload();
    }
}

}
