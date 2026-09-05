#include <QTemporaryDir>
#include <QTest>

#include <chrono>
#include <filesystem>
#include <optional>

#include "carnet/database.hpp"
#include "models/transaction_filter.hpp"
#include "models/transaction_table_model.hpp"

using namespace std::chrono;
using carnet::TransactionFilter;

class TransactionFilterTest : public QObject {
    Q_OBJECT

private slots:
    void init();
    void emptySearchShowsAll();
    void searchIgnoresCaseAndAccents();
    void searchMatchesCounterparty();
    void filtersByCategory();
    void filtersCombine();

private:
    QTemporaryDir dir_;
    std::optional<carnet::Database> db_;
    std::optional<carnet::TransactionTableModel> model_;
    std::optional<TransactionFilter> filter_;
    std::int64_t groceries_ = 0;
};

void TransactionFilterTest::init() {
    filter_.reset();
    model_.reset();
    db_.reset();
    const std::filesystem::path path(dir_.filePath("carnet.db").toStdWString());
    std::filesystem::remove(path);
    db_ = carnet::Database::open(path);
    for (const carnet::Category& c : db_->categories()) {
        if (c.name == "Groceries") {
            groceries_ = c.id;
        }
    }
    db_->addRule("delhaize", groceries_);
    db_->importTransactions("Current",
                            {carnet::TxnDraft{2026y / March / 5, -320, "CAF\xC3\x89 DU COIN", "", 1},
                             carnet::TxnDraft{2026y / March / 6, -4530, "Shopping", "DELHAIZE", 1},
                             carnet::TxnDraft{2026y / March / 7, 215000, "Salary", "ACME", 1}});
    model_.emplace(*db_);
    model_->setMonth(2026y / March);
    filter_.emplace();
    filter_->setSourceModel(&*model_);
}

void TransactionFilterTest::emptySearchShowsAll() {
    filter_->setSearchText("");
    QCOMPARE(filter_->rowCount(), 3);
}

void TransactionFilterTest::searchIgnoresCaseAndAccents() {
    filter_->setSearchText("cafe");
    QCOMPARE(filter_->rowCount(), 1);
    filter_->setSearchText("Caf\xC3\xA9");
    QCOMPARE(filter_->rowCount(), 1);
}

void TransactionFilterTest::searchMatchesCounterparty() {
    filter_->setSearchText("delhaize");
    QCOMPARE(filter_->rowCount(), 1);
}

void TransactionFilterTest::filtersByCategory() {
    filter_->setCategoryFilter(groceries_);
    QCOMPARE(filter_->rowCount(), 1);
    filter_->setCategoryFilter(TransactionFilter::uncategorized);
    QCOMPARE(filter_->rowCount(), 2);
    filter_->setCategoryFilter(TransactionFilter::allCategories);
    QCOMPARE(filter_->rowCount(), 3);
}

void TransactionFilterTest::filtersCombine() {
    filter_->setSearchText("cafe");
    filter_->setCategoryFilter(groceries_);
    QCOMPARE(filter_->rowCount(), 0);
}

QTEST_MAIN(TransactionFilterTest)
#include "transaction_filter_test.moc"
