#pragma once

#include <QSortFilterProxyModel>

#include <string>

namespace carnet {

class TransactionFilter : public QSortFilterProxyModel {
    Q_OBJECT

public:
    static constexpr qint64 allCategories = -1;
    static constexpr qint64 uncategorized = 0;

    explicit TransactionFilter(QObject* parent = nullptr);

    void setSearchText(const QString& text);
    void setCategoryFilter(qint64 filter);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:
    std::string foldedSearch_;
    qint64 categoryFilter_ = allCategories;
};

}
