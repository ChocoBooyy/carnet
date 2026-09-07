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
    void appliesStatusColumn();
    void suggestsTheStatusToSkip();
    void suggestsRolesFromFrenchHeader();
    void suggestsRolesFromOtherHeaders();
    void unknownHeaderSuggestsNothing();
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

void ColumnRolesTest::appliesStatusColumn() {
    const std::vector<ColumnRole> roles{ColumnRole::Date, ColumnRole::Amount, ColumnRole::Description,
                                        ColumnRole::Ignore, ColumnRole::Status};
    carnet::ImportProfile profile;
    carnet::applyRoles(roles, profile);
    QCOMPARE(profile.colStatus, std::optional<int>(4));
    QCOMPARE(carnet::rolesFromProfile(profile, 0), roles);

    const std::vector<ColumnRole> twice{ColumnRole::Date, ColumnRole::Amount, ColumnRole::Description,
                                        ColumnRole::Status, ColumnRole::Status};
    QCOMPARE(carnet::mappingProblem(twice), std::string("Status is chosen twice"));

    const std::vector<ColumnRole> without{ColumnRole::Date, ColumnRole::Amount, ColumnRole::Description};
    carnet::applyRoles(without, profile);
    QCOMPARE(profile.colStatus, std::optional<int>());
}

void ColumnRolesTest::suggestsTheStatusToSkip() {
    QCOMPARE(carnet::suggestSkipStatus(
                 std::vector<std::string>{"Accept\xC3\xA9", "Refus\xC3\xA9", "Accept\xC3\xA9"}),
             std::string("Refus\xC3\xA9"));
    QCOMPARE(carnet::suggestSkipStatus(std::vector<std::string>{"Accepted", " Rejected "}),
             std::string("Rejected"));
    QCOMPARE(carnet::suggestSkipStatus(std::vector<std::string>{"Uitgevoerd", "Geweigerd"}),
             std::string("Geweigerd"));
    QCOMPARE(carnet::suggestSkipStatus(std::vector<std::string>{"Accept\xC3\xA9"}), std::string());
}

void ColumnRolesTest::suggestsRolesFromFrenchHeader() {
    const std::vector<std::string> header{"N\xC2\xB0 de s\xC3\xA9quence",
                                          "Date d'ex\xC3\xA9"
                                          "cution",
                                          "Date valeur",
                                          "Montant",
                                          "Devise du compte",
                                          "Num\xC3\xA9ro de compte",
                                          "Type de transaction",
                                          "Contrepartie",
                                          "Nom de la contrepartie",
                                          "Communication",
                                          "D\xC3\xA9tails",
                                          "Statut",
                                          "Motif du refus"};
    const std::vector<ColumnRole> expected{
        ColumnRole::Ignore,       ColumnRole::Date,   ColumnRole::Ignore,      ColumnRole::Amount,
        ColumnRole::Ignore,       ColumnRole::Ignore, ColumnRole::Ignore,      ColumnRole::Ignore,
        ColumnRole::Counterparty, ColumnRole::Ignore, ColumnRole::Description, ColumnRole::Status,
        ColumnRole::Ignore};
    QCOMPARE(carnet::suggestRoles(header), expected);
}

void ColumnRolesTest::suggestsRolesFromOtherHeaders() {
    const std::vector<std::string> dutch{"Boekingsdatum", "Valutadatum", "Bedrag", "Naam tegenpartij",
                                         "Omschrijving"};
    const std::vector<ColumnRole> dutchRoles{ColumnRole::Date, ColumnRole::Ignore, ColumnRole::Amount,
                                             ColumnRole::Counterparty, ColumnRole::Description};
    QCOMPARE(carnet::suggestRoles(dutch), dutchRoles);

    const std::vector<std::string> english{" Amount ", "Description", "Date"};
    const std::vector<ColumnRole> englishRoles{ColumnRole::Amount, ColumnRole::Description, ColumnRole::Date};
    QCOMPARE(carnet::suggestRoles(english), englishRoles);
}

void ColumnRolesTest::unknownHeaderSuggestsNothing() {
    const std::vector<std::string> header{"14/03/2026", "-12,50", "Bakery"};
    QCOMPARE(carnet::suggestRoles(header), std::vector<ColumnRole>(3, ColumnRole::Ignore));
}

QTEST_MAIN(ColumnRolesTest)
#include "column_roles_test.moc"
