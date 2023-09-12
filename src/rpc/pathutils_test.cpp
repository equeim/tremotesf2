// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <QTest>

#include "pathutils.h"

using namespace tremotesf;

class PathUtilsTest final : public QObject {
    Q_OBJECT

private:
    struct NormalizeTestCase {
        QString inputPath{};
        QString expectedNormalizedPath{};
        PathOs pathOs;
    };

    struct NativeSeparatorsTestCase {
        QString inputPath{};
        QString expectedNativeSeparatorsPath{};
        PathOs pathOs;
    };
private slots:
    void checkNormalize() {
        const auto testCases = std::array{
            NormalizeTestCase{"", "", PathOs::Unix},
            NormalizeTestCase{"", "", PathOs::Windows},

            NormalizeTestCase{"/", "/", PathOs::Unix},
            NormalizeTestCase{"/", "/", PathOs::Windows}, // Whatever that is we leave it as it is

            NormalizeTestCase{"//", "/", PathOs::Unix},
            NormalizeTestCase{"//", "//", PathOs::Windows}, // UNC path

            NormalizeTestCase{"///", "/", PathOs::Unix},
            NormalizeTestCase{"///", "//", PathOs::Windows}, // UNC path? whatever

            NormalizeTestCase{" / ", "/", PathOs::Unix},
            NormalizeTestCase{" / ", "/", PathOs::Windows}, // Whatever that is we leave it as it is

            NormalizeTestCase{"///home//foo", "/home/foo", PathOs::Unix},

            NormalizeTestCase{"C:/home//foo", "C:/home/foo", PathOs::Windows},
            NormalizeTestCase{"C:/home//foo/", "C:/home/foo", PathOs::Windows},
            NormalizeTestCase{R"(C:\home\foo)", "C:/home/foo", PathOs::Windows},
            NormalizeTestCase{R"(C:\home\foo\\)", "C:/home/foo", PathOs::Windows},
            NormalizeTestCase{R"(z:\home\foo)", "Z:/home/foo", PathOs::Windows},
            NormalizeTestCase{R"(D:\)", "D:/", PathOs::Windows},
            NormalizeTestCase{R"( D:\ )", "D:/", PathOs::Windows},
            NormalizeTestCase{R"(D:\\)", "D:/", PathOs::Windows},
            NormalizeTestCase{"D:/", "D:/", PathOs::Windows},
            NormalizeTestCase{"D://", "D:/", PathOs::Windows},
            NormalizeTestCase{R"(\\LOCALHOST\c$\home\foo)", R"(//LOCALHOST/c$/home/foo)", PathOs::Windows},

            // Backslashes in Unix paths are untouched
            NormalizeTestCase{R"(///home//fo\o)", R"(/home/fo\o)", PathOs::Unix},

            // Internal whitespace is untouched
            NormalizeTestCase{"///home//fo  o", "/home/fo  o", PathOs::Unix},
            NormalizeTestCase{R"(C:\home\fo o)", "C:/home/fo o", PathOs::Windows},

            // Weird cases from the top of my head
            NormalizeTestCase{"d:", "D:", PathOs::Windows},
            NormalizeTestCase{"d:foo", "D:foo", PathOs::Windows},
            NormalizeTestCase{R"(c::\wtf)", R"(C::/wtf)", PathOs::Windows}};

        for (const auto& [inputPath, expectedNormalizedPath, pathOs] : testCases) {
            QCOMPARE(normalizePath(inputPath, pathOs), expectedNormalizedPath);
        }
    }

    void checkToNativeSeparators() {
        const auto testCases = std::array{
            NativeSeparatorsTestCase{"/", "/", PathOs::Unix},
            NativeSeparatorsTestCase{"/home/foo", "/home/foo", PathOs::Unix},

            NativeSeparatorsTestCase{"C:/", R"(C:\)", PathOs::Windows},
            NativeSeparatorsTestCase{"C:/home/foo", R"(C:\home\foo)", PathOs::Windows},
            NativeSeparatorsTestCase{R"(//LOCALHOST/c$/home/foo)", R"(\\LOCALHOST\c$\home\foo)", PathOs::Windows},
            NativeSeparatorsTestCase{R"(C::/wtf)", R"(C::\wtf)", PathOs::Windows}};
        for (const auto& [inputPath, expectedNativeSeparatorsPath, pathOs] : testCases) {
            QCOMPARE(toNativeSeparators(inputPath, pathOs), expectedNativeSeparatorsPath);
        }
    }
};

QTEST_GUILESS_MAIN(PathUtilsTest)

#include "pathutils_test.moc"
