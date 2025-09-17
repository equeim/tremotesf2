// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
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
            NormalizeTestCase{.inputPath = "", .expectedNormalizedPath = "", .pathOs = PathOs::Unix},
            NormalizeTestCase{.inputPath = "", .expectedNormalizedPath = "", .pathOs = PathOs::Windows},

            NormalizeTestCase{.inputPath = "/", .expectedNormalizedPath = "/", .pathOs = PathOs::Unix},
            NormalizeTestCase{
                .inputPath = "/",
                .expectedNormalizedPath = "/",
                .pathOs = PathOs::Windows
            }, // Whatever that is we leave it as it is

            NormalizeTestCase{.inputPath = "//", .expectedNormalizedPath = "/", .pathOs = PathOs::Unix},
            NormalizeTestCase{.inputPath = "//", .expectedNormalizedPath = "//", .pathOs = PathOs::Windows}, // UNC path

            NormalizeTestCase{.inputPath = "///", .expectedNormalizedPath = "/", .pathOs = PathOs::Unix},
            NormalizeTestCase{
                .inputPath = "///",
                .expectedNormalizedPath = "//",
                .pathOs = PathOs::Windows
            }, // UNC path? whatever

            NormalizeTestCase{.inputPath = " / ", .expectedNormalizedPath = "/", .pathOs = PathOs::Unix},
            NormalizeTestCase{
                .inputPath = " / ",
                .expectedNormalizedPath = "/",
                .pathOs = PathOs::Windows
            }, // Whatever that is we leave it as it is

            NormalizeTestCase{
                .inputPath = "///home//foo",
                .expectedNormalizedPath = "/home/foo",
                .pathOs = PathOs::Unix
            },

            NormalizeTestCase{
                .inputPath = "C:/home//foo",
                .expectedNormalizedPath = "C:/home/foo",
                .pathOs = PathOs::Windows
            },
            NormalizeTestCase{
                .inputPath = "C:/home//foo/",
                .expectedNormalizedPath = "C:/home/foo",
                .pathOs = PathOs::Windows
            },
            NormalizeTestCase{
                .inputPath = R"(C:\home\foo)",
                .expectedNormalizedPath = "C:/home/foo",
                .pathOs = PathOs::Windows
            },
            NormalizeTestCase{
                .inputPath = R"(C:\home\foo\\)",
                .expectedNormalizedPath = "C:/home/foo",
                .pathOs = PathOs::Windows
            },
            NormalizeTestCase{
                .inputPath = R"(z:\home\foo)",
                .expectedNormalizedPath = "Z:/home/foo",
                .pathOs = PathOs::Windows
            },
            NormalizeTestCase{.inputPath = R"(D:\)", .expectedNormalizedPath = "D:/", .pathOs = PathOs::Windows},
            NormalizeTestCase{.inputPath = R"( D:\ )", .expectedNormalizedPath = "D:/", .pathOs = PathOs::Windows},
            NormalizeTestCase{.inputPath = R"(D:\\)", .expectedNormalizedPath = "D:/", .pathOs = PathOs::Windows},
            NormalizeTestCase{.inputPath = "D:/", .expectedNormalizedPath = "D:/", .pathOs = PathOs::Windows},
            NormalizeTestCase{.inputPath = "D://", .expectedNormalizedPath = "D:/", .pathOs = PathOs::Windows},
            NormalizeTestCase{
                .inputPath = R"(\\LOCALHOST\c$\home\foo)",
                .expectedNormalizedPath = R"(//LOCALHOST/c$/home/foo)",
                .pathOs = PathOs::Windows
            },

            // Backslashes in Unix paths are untouched
            NormalizeTestCase{
                .inputPath = R"(///home//fo\o)",
                .expectedNormalizedPath = R"(/home/fo\o)",
                .pathOs = PathOs::Unix
            },

            // Internal whitespace is untouched
            NormalizeTestCase{
                .inputPath = "///home//fo  o",
                .expectedNormalizedPath = "/home/fo  o",
                .pathOs = PathOs::Unix
            },
            NormalizeTestCase{
                .inputPath = R"(C:\home\fo o)",
                .expectedNormalizedPath = "C:/home/fo o",
                .pathOs = PathOs::Windows
            },

            // Weird cases from the top of my head
            NormalizeTestCase{.inputPath = "d:", .expectedNormalizedPath = "D:/", .pathOs = PathOs::Windows},
            NormalizeTestCase{.inputPath = "d:foo", .expectedNormalizedPath = "D:foo", .pathOs = PathOs::Windows},
            NormalizeTestCase{
                .inputPath = R"(c::\wtf)",
                .expectedNormalizedPath = R"(C::/wtf)",
                .pathOs = PathOs::Windows
            }
        };

        for (const auto& [inputPath, expectedNormalizedPath, pathOs] : testCases) {
            QCOMPARE(normalizePath(inputPath, pathOs), expectedNormalizedPath);
        }
    }

    void checkToNativeSeparators() {
        const auto testCases = std::array{
            NativeSeparatorsTestCase{.inputPath = "/", .expectedNativeSeparatorsPath = "/", .pathOs = PathOs::Unix},
            NativeSeparatorsTestCase{
                .inputPath = "/home/foo",
                .expectedNativeSeparatorsPath = "/home/foo",
                .pathOs = PathOs::Unix
            },

            NativeSeparatorsTestCase{
                .inputPath = "C:/",
                .expectedNativeSeparatorsPath = R"(C:\)",
                .pathOs = PathOs::Windows
            },
            NativeSeparatorsTestCase{
                .inputPath = "C:/home/foo",
                .expectedNativeSeparatorsPath = R"(C:\home\foo)",
                .pathOs = PathOs::Windows
            },
            NativeSeparatorsTestCase{
                .inputPath = R"(//LOCALHOST/c$/home/foo)",
                .expectedNativeSeparatorsPath = R"(\\LOCALHOST\c$\home\foo)",
                .pathOs = PathOs::Windows
            },
            NativeSeparatorsTestCase{
                .inputPath = R"(C::/wtf)",
                .expectedNativeSeparatorsPath = R"(C::\wtf)",
                .pathOs = PathOs::Windows
            }
        };
        for (const auto& [inputPath, expectedNativeSeparatorsPath, pathOs] : testCases) {
            QCOMPARE(toNativeSeparators(inputPath, pathOs), expectedNativeSeparatorsPath);
        }
    }
};

QTEST_GUILESS_MAIN(PathUtilsTest)

#include "pathutils_test.moc"
