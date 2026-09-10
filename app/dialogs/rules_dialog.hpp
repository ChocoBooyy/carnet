#pragma once

#include <QDialog>

#include <optional>
#include <vector>

#include "carnet/database.hpp"

class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;

namespace carnet {

class RulesDialog : public QDialog {
    Q_OBJECT

public:
    explicit RulesDialog(Database& db, QWidget* parent = nullptr);

    bool changed() const;

private:
    QWidget* buildRulesTab();
    QWidget* buildCategoriesTab();
    void reload();
    void ruleSelected();
    void addRule();
    void updateRule();
    void deleteRule();
    void moveRule(MoveDirection direction);
    void applyNow();
    void addCategory();
    void renameCategory();
    void deleteCategory();
    void inTotalsToggled(QListWidgetItem* item);
    void categorySelected();
    void setBudget();
    std::optional<std::int64_t> selectedId(const QListWidget* list) const;
    QString categoryName(std::int64_t id) const;
    void edit(auto&& call);

    Database& db_;
    bool changed_ = false;
    std::vector<Category> categories_;
    std::vector<Rule> rules_;

    QListWidget* ruleList_;
    QLineEdit* patternEdit_;
    QComboBox* ruleCategoryBox_;
    QLabel* status_;
    QListWidget* categoryList_;
    QLineEdit* budgetEdit_;
    QPushButton* budgetButton_;
    QLabel* budgetHint_;
};

}
