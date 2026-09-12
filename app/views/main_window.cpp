#include "views/main_window.hpp"

#include <QAction>
#include <QCloseEvent>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSaveFile>
#include <QSettings>
#include <QTabWidget>
#include <QToolButton>
#include <QVBoxLayout>

#include "app_paths.hpp"
#include "carnet/date.hpp"
#include "carnet/export.hpp"
#include "database_call.hpp"
#include "dialogs/about_dialog.hpp"
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
    auto* exportMenu = new QMenu(this);
    exportMenu->addAction(tr("Month summary..."), this,
                          [this] { askExport(QStringLiteral("summary"), &MainWindow::exportSummary); });
    exportMenu->addAction(tr("Month transactions..."), this, [this] {
        askExport(QStringLiteral("transactions"), &MainWindow::exportTransactions);
    });
    auto* exportButton = new QToolButton(central);
    exportButton->setText(tr("Export"));
    exportButton->setMenu(exportMenu);
    exportButton->setPopupMode(QToolButton::InstantPopup);
    top->addWidget(exportButton);
    auto* rulesButton = new QPushButton(tr("Rules..."), central);
    top->addWidget(rulesButton);
    auto* aboutButton = new QToolButton(central);
    aboutButton->setText(tr("?"));
    aboutButton->setToolTip(tr("About Carnet (F1)"));
    top->addWidget(aboutButton);
    layout->addLayout(top);

    tabs_ = new QTabWidget(central);
    overviewTab_ = new OverviewTab(db_, tabs_);
    tabs_->addTab(overviewTab_, tr("Overview"));
    transactionsTab_ = new TransactionsTab(db_, tabs_);
    tabs_->addTab(transactionsTab_, tr("Transactions"));
    tabs_->setCurrentWidget(transactionsTab_);
    layout->addWidget(tabs_);

    setCentralWidget(central);
    resize(1000, 640);

    connect(monthPicker_, &MonthPicker::monthChanged, transactionsTab_, &TransactionsTab::setMonth);
    connect(monthPicker_, &MonthPicker::monthChanged, overviewTab_, &OverviewTab::setMonth);
    // category edits happen in the Transactions tab, so coming back to the Overview picks them up
    connect(tabs_, &QTabWidget::currentChanged, this, [this](int index) {
        if (tabs_->widget(index) == overviewTab_) {
            overviewTab_->reload();
        }
    });
    connect(importButton, &QPushButton::clicked, this, &MainWindow::openImport);
    connect(rulesButton, &QPushButton::clicked, this, &MainWindow::openRules);
    connect(aboutButton, &QToolButton::clicked, this, &MainWindow::openAbout);

    addShortcut("previousMonth", QKeySequence(Qt::CTRL | Qt::Key_PageUp),
                [this] { monthPicker_->setMonth(month() - std::chrono::months{1}); });
    addShortcut("nextMonth", QKeySequence(Qt::CTRL | Qt::Key_PageDown),
                [this] { monthPicker_->setMonth(month() + std::chrono::months{1}); });
    addShortcut("currentMonth", QKeySequence(Qt::CTRL | Qt::Key_Home), [this] {
        const std::chrono::year_month_day today{
            std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now())};
        monthPicker_->setMonth(today.year() / today.month());
    });
    addShortcut("import", QKeySequence(Qt::CTRL | Qt::Key_I), [this] { openImport(); });
    addShortcut("rules", QKeySequence(Qt::CTRL | Qt::Key_R), [this] { openRules(); });
    addShortcut("export", QKeySequence(Qt::CTRL | Qt::Key_E), [exportButton] { exportButton->showMenu(); });
    addShortcut("overviewTab", QKeySequence(Qt::CTRL | Qt::Key_1),
                [this] { tabs_->setCurrentWidget(overviewTab_); });
    addShortcut("transactionsTab", QKeySequence(Qt::CTRL | Qt::Key_2),
                [this] { tabs_->setCurrentWidget(transactionsTab_); });
    addShortcut("about", QKeySequence(Qt::Key_F1), [this] { openAbout(); });

    restoreSettings();
}

void MainWindow::addShortcut(const char* name, const QKeySequence& keys, const std::function<void()>& run) {
    auto* action = new QAction(this);
    action->setObjectName(name);
    action->setShortcut(keys);
    addAction(action);
    connect(action, &QAction::triggered, this, run);
}

void MainWindow::openAbout() {
    AboutDialog dialog(this);
    dialog.exec();
}

void MainWindow::restoreSettings() {
    const QSettings settings(settingsPath(), QSettings::IniFormat);
    restoreGeometry(settings.value("window/geometry").toByteArray());
    const int tab = settings.value("window/tab", tabs_->currentIndex()).toInt();
    if (tab >= 0 && tab < tabs_->count()) {
        tabs_->setCurrentIndex(tab);
    }
    // anything unreadable keeps the picker on the current month
    const auto day = parseDate(settings.value("window/month").toString().toStdString() + "-01", "yyyy-MM-dd");
    monthPicker_->setMonth(day ? day->year() / day->month() : monthPicker_->month());
}

void MainWindow::closeEvent(QCloseEvent* event) {
    QSettings settings(settingsPath(), QSettings::IniFormat);
    settings.setValue("window/geometry", saveGeometry());
    settings.setValue("window/tab", tabs_->currentIndex());
    settings.setValue("window/month", QString::fromStdString(toIsoString(month() / 1).substr(0, 7)));
    QMainWindow::closeEvent(event);
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

std::chrono::year_month MainWindow::month() const {
    return monthPicker_->month();
}

bool MainWindow::exportSummary(const QString& path) {
    MonthOverview overview;
    if (!tryDatabase(this, [&] { overview = db_.monthOverview(month()); })) {
        return false;
    }
    return writeFile(path, summaryCsv(month(), overview, tr("Uncategorized").toStdString()));
}

bool MainWindow::exportTransactions(const QString& path) {
    std::vector<Transaction> rows;
    std::vector<Category> categories;
    if (!tryDatabase(this, [&] {
            rows = db_.transactionsInMonth(month());
            categories = db_.categories();
        })) {
        return false;
    }
    return writeFile(path, transactionsCsv(rows, categories, tr("Uncategorized").toStdString()));
}

void MainWindow::askExport(const QString& kind, bool (MainWindow::*write)(const QString&)) {
    const QString isoMonth = QString::fromStdString(toIsoString(month() / 1).substr(0, 7));
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Export"), QDir::home().filePath(QString("carnet-%1-%2.csv").arg(isoMonth, kind)),
        tr("CSV files (*.csv)"));
    if (!path.isEmpty()) {
        (this->*write)(path);
    }
}

bool MainWindow::writeFile(const QString& path, const std::string& content) {
    // QSaveFile writes beside the target and renames on commit, so a failure never leaves half a file
    QSaveFile file(path);
    const auto size = static_cast<qint64>(content.size());
    const bool ok =
        file.open(QIODevice::WriteOnly) && file.write(content.data(), size) == size && file.commit();
    // a hidden window (a test, the screenshot tool) has nobody to show the box to; the result says it
    if (!ok && isVisible()) {
        QMessageBox::critical(this, tr("Carnet"), tr("Could not write %1: %2").arg(path, file.errorString()));
    }
    return ok;
}

}
