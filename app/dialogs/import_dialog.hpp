#pragma once

#include <QDialog>

#include <chrono>
#include <string>
#include <vector>

#include "carnet/csv.hpp"
#include "carnet/database.hpp"
#include "dialogs/column_roles.hpp"

class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QTableWidget;

namespace carnet {

class ImportDialog : public QDialog {
    Q_OBJECT

public:
    explicit ImportDialog(Database& db, QWidget* parent = nullptr);

    // loads a bank export into the preview, as picking it with Browse does
    bool openFile(const QString& path);

signals:
    void imported(std::chrono::year_month newestMonth);

private:
    void browse();
    void loadProfiles(const QString& select);
    void profileChosen();
    void parseText();
    void separatorChanged();
    void refreshPreview();
    void updateState();
    void suggestSkipStatusFrom(int column);
    void suggestFromHeader();
    char separator() const;
    ImportProfile currentProfile() const;
    const ImportProfile* savedProfile() const;
    void runImport();

    Database& db_;
    std::string text_;
    std::vector<CsvRow> rows_;
    std::vector<ImportProfile> profiles_;
    std::vector<ColumnRole> roles_;
    QString csvProblem_;

    QLineEdit* fileEdit_;
    QComboBox* accountBox_;
    QComboBox* profileBox_;
    QComboBox* separatorBox_;
    QComboBox* decimalBox_;
    QLineEdit* dateFormatEdit_;
    QSpinBox* skipBox_;
    QLineEdit* skipStatusEdit_;
    QTableWidget* preview_;
    QLabel* hint_;
    QPlainTextEdit* result_;
    QPushButton* importButton_;
};

}
