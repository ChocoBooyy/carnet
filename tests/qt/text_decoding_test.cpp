#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include <optional>
#include <string>

#include "dialogs/text_decoding.hpp"

class TextDecodingTest : public QObject {
    Q_OBJECT

private slots:
    void keepsUtf8();
    void convertsWindows1252();
    void undefinedBytes();
    void readsAFileAsUtf8();
    void missingFileIsEmpty();
};

void TextDecodingTest::keepsUtf8() {
    const std::string utf8 = "\xEF\xBB\xBF"
                             "Date;CAF\xC3\x89\n";
    QCOMPARE(carnet::toUtf8Text(utf8), utf8);
}

void TextDecodingTest::convertsWindows1252() {
    QCOMPARE(carnet::toUtf8Text("CAF\xC9;10\x80"), std::string("CAF\xC3\x89;10\xE2\x82\xAC"));
    QCOMPARE(carnet::toUtf8Text("\x9C"
                                "uvre"),
             std::string("\xC5\x93uvre"));
}

void TextDecodingTest::undefinedBytes() {
    QCOMPARE(carnet::toUtf8Text("a\x81z\xE9"), std::string("a\xEF\xBF\xBDz\xC3\xA9"));
}

void TextDecodingTest::readsAFileAsUtf8() {
    QTemporaryDir dir;
    const QString path = dir.filePath("export.csv");
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("CAF\xC9\n");
    file.close();
    QCOMPARE(carnet::readTextFile(path), std::optional<std::string>("CAF\xC3\x89\n"));
}

void TextDecodingTest::missingFileIsEmpty() {
    QTemporaryDir dir;
    QCOMPARE(carnet::readTextFile(dir.filePath("nope.csv")), std::optional<std::string>());
}

QTEST_MAIN(TextDecodingTest)
#include "text_decoding_test.moc"
