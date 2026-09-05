#include "widgets/category_delegate.hpp"

#include <QComboBox>

#include "models/transaction_table_model.hpp"

namespace carnet {

CategoryDelegate::CategoryDelegate(Database& db, QObject* parent) : QStyledItemDelegate(parent), db_(db) {
}

QWidget* CategoryDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem&,
                                        const QModelIndex&) const {
    std::vector<Category> categories;
    try {
        categories = db_.categories();
    } catch (const DatabaseError&) {
        return nullptr;
    }

    auto* combo = new QComboBox(parent);
    combo->addItem(tr("(automatic)"), TransactionTableModel::automaticChoice);
    combo->addItem(tr("(none)"), TransactionTableModel::noCategoryChoice);
    for (const Category& category : categories) {
        combo->addItem(QString::fromStdString(category.name), qint64(category.id));
    }

    // commit on the first pick instead of waiting for the editor to lose focus
    auto* self = const_cast<CategoryDelegate*>(this);
    connect(combo, &QComboBox::activated, self, [self, combo] {
        emit self->commitData(combo);
        emit self->closeEditor(combo);
    });
    return combo;
}

void CategoryDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const {
    auto* combo = static_cast<QComboBox*>(editor);
    const int position = combo->findData(index.data(Qt::EditRole).toLongLong());
    combo->setCurrentIndex(position < 0 ? 0 : position);
}

void CategoryDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
                                    const QModelIndex& index) const {
    auto* combo = static_cast<QComboBox*>(editor);
    model->setData(index, combo->currentData(), Qt::EditRole);
}

}
