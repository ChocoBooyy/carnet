#include <QTest>

#include <vector>

#include "dialogs/column_roles.hpp"

using carnet::ColumnRole;

class ColumnRolesTest : public QObject {
    Q_OBJECT

private slots:
    void appliesCompleteMapping();
    void reportsMissingRoles();
    void reportsDuplicates();
    void roundTripsThroughProfile();
    void growsToProfileColumns();
};

void ColumnRolesTest::appliesCompleteMapping() {
    const std::vector<ColumnRole> roles{ColumnRole::Amount, ColumnRole::Ignore, ColumnRole::Date,
                                        ColumnRole::Description, ColumnRole::Counterparty};
    QCOMPARE(carnet::mappingProblem(roles), std::string());
    carnet::ImportProfile profile;
    profile.colParty = 9;
    carnet::applyRoles(roles, profile);
    QCOMPARE(profile.colAmount, 0);
    QCOMPARE(profile.colDate, 2);
    QCOMPARE(profile.colDesc, 3);
    QCOMPARE(profile.colParty, std::optional<int>(4));

    const std::vector<ColumnRole> noParty{ColumnRole::Date, ColumnRole::Amount, ColumnRole::Description};
    carnet::applyRoles(noParty, profile);
    QCOMPARE(profile.colParty, std::optional<int>());
}

void ColumnRolesTest::reportsMissingRoles() {
    const std::vector<ColumnRole> roles{ColumnRole::Date, ColumnRole::Ignore, ColumnRole::Description};
    QCOMPARE(carnet::mappingProblem(roles), std::string("Choose the Date, Amount and Description columns"));
    QCOMPARE(carnet::mappingProblem(std::vector<ColumnRole>{}),
             std::string("Choose the Date, Amount and Description columns"));
}

void ColumnRolesTest::reportsDuplicates() {
    const std::vector<ColumnRole> roles{ColumnRole::Date, ColumnRole::Amount, ColumnRole::Description,
                                        ColumnRole::Amount};
    QCOMPARE(carnet::mappingProblem(roles), std::string("Amount is chosen twice"));
    const std::vector<ColumnRole> parties{ColumnRole::Date, ColumnRole::Amount, ColumnRole::Description,
                                          ColumnRole::Counterparty, ColumnRole::Counterparty};
    QCOMPARE(carnet::mappingProblem(parties), std::string("Counterparty is chosen twice"));
}

void ColumnRolesTest::roundTripsThroughProfile() {
    carnet::ImportProfile profile;
    profile.colDate = 1;
    profile.colAmount = 3;
    profile.colDesc = 0;
    profile.colParty = 2;
    const std::vector<ColumnRole> expected{ColumnRole::Description, ColumnRole::Date,
                                           ColumnRole::Counterparty, ColumnRole::Amount, ColumnRole::Ignore};
    QCOMPARE(carnet::rolesFromProfile(profile, 5), expected);
}

void ColumnRolesTest::growsToProfileColumns() {
    carnet::ImportProfile profile;
    profile.colDate = 0;
    profile.colAmount = 1;
    profile.colDesc = 6;
    QCOMPARE(carnet::rolesFromProfile(profile, 3).size(), std::size_t(7));
}

QTEST_MAIN(ColumnRolesTest)
#include "column_roles_test.moc"
