#include "dialogs/text_decoding.hpp"

#include <QByteArrayView>
#include <QFile>
#include <QStringDecoder>

#include <array>

namespace carnet {

namespace {

// Windows-1252 differs from Latin-1 only in 0x80-0x9F; 0 marks the five undefined bytes
constexpr std::array<char32_t, 32> windows1252High{{
    0x20AC, 0,      0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030, 0x0160,
    0x2039, 0x0152, 0,      0x017D, 0,      0,      0x2018, 0x2019, 0x201C, 0x201D, 0x2022,
    0x2013, 0x2014, 0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0,      0x017E, 0x0178,
}};

void appendUtf8(std::string& out, char32_t codePoint) {
    if (codePoint < 0x80) {
        out += static_cast<char>(codePoint);
    } else if (codePoint < 0x800) {
        out += static_cast<char>(0xC0 | (codePoint >> 6));
        out += static_cast<char>(0x80 | (codePoint & 0x3F));
    } else {
        out += static_cast<char>(0xE0 | (codePoint >> 12));
        out += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (codePoint & 0x3F));
    }
}

bool isValidUtf8(std::string_view bytes) {
    QStringDecoder decoder(QStringDecoder::Utf8);
    const QString ignored =
        decoder.decode(QByteArrayView(bytes.data(), static_cast<qsizetype>(bytes.size())));
    return !decoder.hasError();
}

}

std::string toUtf8Text(std::string_view bytes) {
    if (isValidUtf8(bytes)) {
        return std::string(bytes);
    }
    std::string result;
    result.reserve(bytes.size() + bytes.size() / 4);
    for (const char c : bytes) {
        const auto byte = static_cast<unsigned char>(c);
        if (byte >= 0x80 && byte <= 0x9F) {
            const char32_t mapped = windows1252High[byte - 0x80];
            appendUtf8(result, mapped == 0 ? char32_t{0xFFFD} : mapped);
        } else {
            appendUtf8(result, byte);
        }
    }
    return result;
}

std::optional<std::string> readTextFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return std::nullopt;
    }
    const QByteArray bytes = file.readAll();
    return toUtf8Text(std::string_view(bytes.constData(), static_cast<std::size_t>(bytes.size())));
}

}
