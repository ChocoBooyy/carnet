#pragma once

#include <QStyledItemDelegate>

#include "carnet/database.hpp"

namespace carnet {

class CategoryDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit CategoryDelegate(Database& db, QObject* parent = nullptr);

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                          const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;

private:
    Database& db_;
};

}
