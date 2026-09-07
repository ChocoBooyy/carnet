#include "dialogs/import_dialog.hpp"

#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>

#include <algorithm>
#include <ranges>

#include "app_paths.hpp"
#include "carnet/csv.hpp"
#include "database_call.hpp"
#include "dialogs/text_decoding.hpp"

namespace carnet {

namespace {

constexpr int previewRows = 10;
constexpr int newProfile = -1;

}

ImportDialog::ImportDialog(Database& db, QWidget* parent) : QDialog(parent), db_(db) {
    setWindowTitle(tr("Import CSV"));

    pathEdit_ = new QLineEdit(this);
    pathEdit_->setReadOnly(true);
    auto* browseButton = new QPushButton(tr("Browse..."), this);
    auto* fileRow = new QHBoxLayout;
    fileRow->addWidget(pathEdit_, 1);
    fileRow->addWidget(browseButton);

    accountBox_ = new QComboBox(this);
    accountBox_->setEditable(true);
    profileBox_ = new QComboBox(this);

    separatorBox_ = new QComboBox(this);
    separatorBox_->addItem(";", ";");
    separatorBox_->addItem(",", ",");
    decimalBox_ = new QComboBox(this);
    decimalBox_->addItem(",", ",");
    decimalBox_->addItem(".", ".");
    dateFormatEdit_ = new QLineEdit("dd/MM/yyyy", this);
    skipBox_ = new QSpinBox(this);
    skipBox_->setRange(0, 20);
    skipBox_->setValue(1);
    auto* settingsRow = new QHBoxLayout;
    settingsRow->addWidget(new QLabel(tr("Separator"), this));
    settingsRow->addWidget(separatorBox_);
    settingsRow->addWidget(new QLabel(tr("Decimal"), this));
    settingsRow->addWidget(decimalBox_);
    settingsRow->addWidget(new QLabel(tr("Date format"), this));
    settingsRow->addWidget(dateFormatEdit_);
    settingsRow->addWidget(new QLabel(tr("Skip lines"), this));
    settingsRow->addWidget(skipBox_);
    settingsRow->addStretch();

    skipStatusEdit_ = new QLineEdit(this);
    skipStatusEdit_->setPlaceholderText(tr("for example Refused, needs a Status column"));

    auto* form = new QFormLayout;
    form->addRow(tr("File"), fileRow);
    form->addRow(tr("Account"), accountBox_);
    form->addRow(tr("Profile"), profileBox_);
    form->addRow(settingsRow);
    form->addRow(tr("Skip rows with status"), skipStatusEdit_);

    preview_ = new QTableWidget(this);
    preview_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    hint_ = new QLabel(this);
    result_ = new QPlainTextEdit(this);
    result_->setReadOnly(true);
    result_->setMaximumHeight(90);
    importButton_ = new QPushButton(tr("Import"), this);
    auto* closeButton = new QPushButton(tr("Close"), this);
    auto* buttons = new QHBoxLayout;
    buttons->addWidget(hint_, 1);
    buttons->addWidget(importButton_);
    buttons->addWidget(closeButton);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(preview_, 1);
    layout->addWidget(result_);
    layout->addLayout(buttons);
    resize(900, 560);

    const QSettings settings(settingsPath(), QSettings::IniFormat);
    std::vector<std::string> accounts;
    tryDatabase(this, [&] { accounts = db_.accounts(); });
    for (const std::string& account : accounts) {
        accountBox_->addItem(QString::fromStdString(account));
    }
    accountBox_->setCurrentText(settings.value("import/account", tr("Current account")).toString());
    loadProfiles(settings.value("import/profile").toString());

    connect(browseButton, &QPushButton::clicked, this, &ImportDialog::browse);
    connect(profileBox_, &QComboBox::currentIndexChanged, this, &ImportDialog::profileChosen);
    connect(separatorBox_, &QComboBox::currentIndexChanged, this, &ImportDialog::refreshPreview);
    connect(skipBox_, &QSpinBox::valueChanged, this, &ImportDialog::refreshPreview);
    connect(skipStatusEdit_, &QLineEdit::textChanged, this, &ImportDialog::updateState);
    connect(accountBox_, &QComboBox::currentTextChanged, this, &ImportDialog::updateState);
    connect(importButton_, &QPushButton::clicked, this, &ImportDialog::runImport);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    updateState();
}

void ImportDialog::browse() {
    const QString path =
        QFileDialog::getOpenFileName(this, tr("Choose a bank export"), {}, tr("CSV files (*.csv)"));
    if (path.isEmpty()) {
        return;
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        result_->setPlainText(tr("Could not open %1").arg(path));
        return;
    }
    const QByteArray bytes = file.readAll();
    text_ = toUtf8Text(std::string_view(bytes.constData(), static_cast<std::size_t>(bytes.size())));
    pathEdit_->setText(path);
    result_->clear();
    if (savedProfile() == nullptr) {
        suggestFromHeader();
    }
    refreshPreview();
}

void ImportDialog::suggestFromHeader() {
    const auto rows = parseCsv(text_, separator());
    const int headerRow = skipBox_->value() - 1;
    if (!rows || headerRow < 0 || headerRow >= static_cast<int>(rows->size())) {
        return;
    }
    roles_ = suggestRoles((*rows)[static_cast<std::size_t>(headerRow)].fields);
    skipStatusEdit_->clear();
    const auto status = std::ranges::find(roles_, ColumnRole::Status);
    if (status != roles_.end()) {
        suggestSkipStatusFrom(static_cast<int>(status - roles_.begin()));
    }
}

void ImportDialog::loadProfiles(const QString& select) {
    tryDatabase(this, [&] { profiles_ = db_.profiles(); });
    const QSignalBlocker blocker(profileBox_);
    profileBox_->clear();
    profileBox_->addItem(tr("New profile"), newProfile);
    for (std::size_t i = 0; i < profiles_.size(); ++i) {
        profileBox_->addItem(QString::fromStdString(profiles_[i].name), static_cast<int>(i));
    }
    const int position = profileBox_->findText(select);
    profileBox_->setCurrentIndex(position < 0 ? 0 : position);
    profileChosen();
}

void ImportDialog::profileChosen() {
    const ImportProfile* profile = savedProfile();
    if (profile == nullptr) {
        refreshPreview();
        return;
    }
    {
        const QSignalBlocker blockSeparator(separatorBox_);
        const QSignalBlocker blockSkip(skipBox_);
        separatorBox_->setCurrentIndex(separatorBox_->findData(QString(QChar(profile->separator))));
        decimalBox_->setCurrentIndex(decimalBox_->findData(QString(QChar(profile->decimalMark))));
        dateFormatEdit_->setText(QString::fromStdString(profile->dateFormat));
        skipBox_->setValue(profile->skipLines);
    }
    skipStatusEdit_->setText(QString::fromStdString(profile->skipStatus));
    roles_ = rolesFromProfile(*profile, static_cast<int>(roles_.size()));
    refreshPreview();
}

void ImportDialog::refreshPreview() {
    preview_->clear();
    preview_->setRowCount(0);
    preview_->setColumnCount(0);
    csvProblem_.clear();
    if (text_.empty()) {
        updateState();
        return;
    }

    const auto rows = parseCsv(text_, separator());
    if (!rows) {
        csvProblem_ = QString::fromStdString(rows.error().message);
        updateState();
        return;
    }
    const int shown = std::min(static_cast<int>(rows->size()), previewRows);
    int columns = 0;
    for (int r = 0; r < shown; ++r) {
        columns = std::max(columns, static_cast<int>((*rows)[static_cast<std::size_t>(r)].fields.size()));
    }
    roles_.resize(static_cast<std::size_t>(std::max(columns, static_cast<int>(roles_.size()))),
                  ColumnRole::Ignore);

    preview_->setColumnCount(static_cast<int>(roles_.size()));
    preview_->setRowCount(shown + 1);
    for (int c = 0; c < static_cast<int>(roles_.size()); ++c) {
        auto* roleBox = new QComboBox(preview_);
        roleBox->addItems(
            {tr("(ignore)"), tr("Date"), tr("Amount"), tr("Description"), tr("Counterparty"), tr("Status")});
        roleBox->setCurrentIndex(static_cast<int>(roles_[static_cast<std::size_t>(c)]));
        connect(roleBox, &QComboBox::currentIndexChanged, this, [this, c](int index) {
            roles_[static_cast<std::size_t>(c)] = static_cast<ColumnRole>(index);
            if (roles_[static_cast<std::size_t>(c)] == ColumnRole::Status) {
                suggestSkipStatusFrom(c);
            }
            updateState();
        });
        preview_->setCellWidget(0, c, roleBox);
    }
    const QColor skipped = palette().color(QPalette::Disabled, QPalette::Text);
    for (int r = 0; r < shown; ++r) {
        const auto& fields = (*rows)[static_cast<std::size_t>(r)].fields;
        for (int c = 0; c < static_cast<int>(fields.size()); ++c) {
            auto* item = new QTableWidgetItem(QString::fromStdString(fields[static_cast<std::size_t>(c)]));
            if (r < skipBox_->value()) {
                item->setForeground(skipped);
            }
            preview_->setItem(r + 1, c, item);
        }
    }
    preview_->resizeColumnsToContents();
    updateState();
}

void ImportDialog::updateState() {
    QString problem;
    if (text_.empty()) {
        problem = tr("Choose a file");
    } else if (!csvProblem_.isEmpty()) {
        problem = csvProblem_;
    } else if (accountBox_->currentText().trimmed().isEmpty()) {
        problem = tr("Name the account");
    } else {
        problem = QString::fromStdString(mappingProblem(roles_));
        if (problem.isEmpty() && !skipStatusEdit_->text().trimmed().isEmpty() &&
            std::ranges::find(roles_, ColumnRole::Status) == roles_.end()) {
            problem = tr("Choose the Status column, or clear the status to skip");
        }
    }
    hint_->setText(problem);
    importButton_->setEnabled(problem.isEmpty());
}

void ImportDialog::suggestSkipStatusFrom(int column) {
    // a value typed by hand wins over a guess
    if (!skipStatusEdit_->text().trimmed().isEmpty()) {
        return;
    }
    const auto rows = parseCsv(text_, separator());
    if (!rows) {
        return;
    }
    std::vector<std::string> values;
    for (const CsvRow& row : *rows | std::views::drop(skipBox_->value())) {
        if (static_cast<std::size_t>(column) < row.fields.size()) {
            values.push_back(row.fields[static_cast<std::size_t>(column)]);
        }
    }
    skipStatusEdit_->setText(QString::fromStdString(suggestSkipStatus(values)));
}

char ImportDialog::separator() const {
    return separatorBox_->currentData().toString().at(0).toLatin1();
}

const ImportProfile* ImportDialog::savedProfile() const {
    const int index = profileBox_->currentData().toInt();
    return index == newProfile ? nullptr : &profiles_[static_cast<std::size_t>(index)];
}

ImportProfile ImportDialog::currentProfile() const {
    ImportProfile profile;
    if (const ImportProfile* saved = savedProfile()) {
        profile.name = saved->name;
    }
    profile.separator = separator();
    profile.decimalMark = decimalBox_->currentData().toString().at(0).toLatin1();
    profile.dateFormat = dateFormatEdit_->text().trimmed().toStdString();
    profile.skipLines = skipBox_->value();
    applyRoles(roles_, profile);
    profile.skipStatus = skipStatusEdit_->text().trimmed().toStdString();
    return profile;
}

void ImportDialog::runImport() {
    ImportProfile profile = currentProfile();
    const ImportProfile* saved = savedProfile();
    if (saved == nullptr) {
        bool ok = false;
        const QString name =
            QInputDialog::getText(this, tr("Save profile"), tr("Profile name, for example your bank:"),
                                  QLineEdit::Normal, {}, &ok)
                .trimmed();
        if (!ok || name.isEmpty()) {
            return;
        }
        profile.name = name.toStdString();
    }

    const auto statement = readStatement(text_, profile);
    if (!statement) {
        QStringList lines{tr("Nothing was imported. Fix the profile or the file:")};
        for (const ParseError& error : statement.error()) {
            lines << QString::fromStdString(error.message);
        }
        result_->setPlainText(lines.join('\n'));
        return;
    }

    const QString account = accountBox_->currentText().trimmed();
    ImportSummary summary;
    const bool ok = tryDatabase(this, [&] {
        if (saved == nullptr || *saved != profile) {
            db_.saveProfile(profile);
        }
        summary = db_.importTransactions(account.toStdString(), statement->drafts);
    });
    if (!ok) {
        return;
    }

    QString text = tr("%1 added, %2 already present").arg(summary.added).arg(summary.alreadyPresent);
    if (statement->skipped > 0) {
        text += tr(", %1 skipped (status %2)")
                    .arg(statement->skipped)
                    .arg(QString::fromStdString(profile.skipStatus));
    }
    result_->setPlainText(text);
    QSettings settings(settingsPath(), QSettings::IniFormat);
    settings.setValue("import/account", account);
    settings.setValue("import/profile", QString::fromStdString(profile.name));
    loadProfiles(QString::fromStdString(profile.name));

    if (!statement->drafts.empty()) {
        const auto newest = std::ranges::max(statement->drafts, {}, &TxnDraft::bookedOn).bookedOn;
        emit imported(newest.year() / newest.month());
    }
}

}
