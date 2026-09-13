// Renders the README screenshots from docs/demo/demo-bank.csv. The window is laid out and drawn by the
// real platform but never shown, so running it does not disturb the desktop.
#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTimer>

#include <filesystem>
#include <functional>
#include <iostream>
#include <vector>

#ifdef _WIN32
#include <crtdbg.h>
#include <stdlib.h>
#include <windows.h>
#endif

#include "carnet/database.hpp"
#include "carnet/import.hpp"
#include "dialogs/import_dialog.hpp"
#include "dialogs/rules_dialog.hpp"
#include "dialogs/text_decoding.hpp"
#include "views/main_window.hpp"
#include "widgets/month_picker.hpp"

namespace {

using namespace std::chrono;

void silenceCrashDialogs() {
#ifdef _WIN32
    // a crash would otherwise put a dialog on the desktop this tool promises not to touch
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#ifdef _DEBUG
    for (int type : {_CRT_WARN, _CRT_ERROR, _CRT_ASSERT}) {
        _CrtSetReportMode(type, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(type, _CRTDBG_FILE_STDERR);
    }
#endif
#endif
}

bool fillDemo(carnet::Database& db, const QString& demoPath) {
    carnet::ImportProfile profile;
    profile.name = "Demo bank";
    profile.colDate = 1;
    profile.colAmount = 3;
    profile.colParty = 8;
    profile.colDesc = 10;
    profile.colStatus = 11;
    profile.skipStatus = "Refus\xC3\xA9";
    const auto text = carnet::readTextFile(demoPath);
    if (!text) {
        return false;
    }
    const auto statement = carnet::readStatement(*text, profile);
    if (!statement) {
        return false;
    }
    db.importTransactions("Current account", statement->drafts);

    const auto id = [&db](std::string_view name) {
        for (const carnet::Category& category : db.categories()) {
            if (category.name == name) {
                return category.id;
            }
        }
        return db.addCategory(name);
    };
    const std::vector<std::pair<const char*, const char*>> rules{
        {"delhaize", "Groceries"}, {"colruyt", "Groceries"},   {"aldi", "Groceries"},
        {"sncb", "Transport"},     {"pizza", "Eating out"},    {"restaurant", "Eating out"},
        {"spotify", "Leisure"},    {"acme", "Salary"},         {"immo lambert", "Rent"},
        {"proximus", "Utilities"}, {"alex demo", "Transfers"},
    };
    for (const auto& [pattern, category] : rules) {
        db.addRule(pattern, id(category));
    }
    db.applyRules();
    db.setCategoryBudget(id("Groceries"), 35000);
    db.setCategoryBudget(id("Eating out"), 12000);
    db.setCategoryBudget(id("Transport"), 6000);
    return true;
}

}

int main(int argc, char* argv[]) {
    silenceCrashDialogs();
    QApplication app(argc, argv);
    QApplication::setApplicationName("CarnetScreenshots");
    // keep the tool's window settings away from a real carnet.ini
    QStandardPaths::setTestModeEnabled(true);

    const QDir source(CARNET_SOURCE_DIR);
    const QString demoPath = source.filePath("docs/demo/demo-bank.csv");
    const QDir out(argc > 1 ? QString::fromLocal8Bit(argv[1]) : source.filePath("docs/screenshots"));
    QTemporaryDir dir;
    auto db = carnet::Database::open(std::filesystem::path(dir.filePath("carnet.db").toStdWString()));
    if (!fillDemo(db, demoPath)) {
        std::cerr << "could not read " << demoPath.toStdString() << '\n';
        return 1;
    }

    carnet::MainWindow window(db);
    window.setAttribute(Qt::WA_DontShowOnScreen);
    window.resize(1280, 800);
    window.show();
    window.findChild<carnet::MonthPicker*>()->setMonth(2026y / September);
    auto* tabs = window.findChild<QTabWidget*>();

    carnet::ImportDialog importDialog(db, &window);
    carnet::RulesDialog rulesDialog(db, &window);

    int failures = 0;
    const auto save = [&](QWidget& widget, const char* name) {
        const QString path = out.filePath(name);
        if (widget.grab().save(path)) {
            std::cout << "wrote " << path.toStdString() << '\n';
        } else {
            std::cerr << "could not write " << path.toStdString() << '\n';
            ++failures;
        }
    };

    // each step waits for the previous one's layout and painting to settle
    const std::vector<std::function<void()>> steps{
        [&] { tabs->setCurrentIndex(0); },
        [&] { save(window, "overview.png"); },
        [&] { tabs->setCurrentIndex(1); },
        [&] { save(window, "transactions.png"); },
        [&] {
            importDialog.setAttribute(Qt::WA_DontShowOnScreen);
            importDialog.openFile(demoPath);
            importDialog.show();
        },
        [&] {
            save(importDialog, "import.png");
            importDialog.hide();
            rulesDialog.setAttribute(Qt::WA_DontShowOnScreen);
            rulesDialog.show();
        },
        [&] {
            save(rulesDialog, "rules.png");
            rulesDialog.hide();
            app.exit(failures == 0 ? 0 : 1);
        },
    };
    for (std::size_t i = 0; i < steps.size(); ++i) {
        QTimer::singleShot(500 * static_cast<int>(i + 1), &app, steps[i]);
    }
    return app.exec();
}
