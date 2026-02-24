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
            },

            // URL normalization tests


            NormalizeTestCase{
                .inputPath = "SMB://HOSTNAME/PATH",
                .expectedNormalizedPath = "smb://HOSTNAME/PATH",
                .pathOs = PathOs::Unix
            },
            NormalizeTestCase{
                .inputPath = "smb:////hostname/path/to/share",
                .expectedNormalizedPath = "smb://hostname/path/to/share",
                .pathOs = PathOs::Unix
            },
            NormalizeTestCase{
                .inputPath = "smb:///hostname/path/to/share",
                .expectedNormalizedPath = "smb://hostname/path/to/share",
                .pathOs = PathOs::Unix
            },
            NormalizeTestCase{
                .inputPath = "smb://hostname//path/to/share",
                .expectedNormalizedPath = "smb://hostname/path/to/share",
                .pathOs = PathOs::Unix
            },
            NormalizeTestCase{
                .inputPath = "ftp://hostname/path//to/share",
                .expectedNormalizedPath = "ftp://hostname/path/to/share",
                .pathOs = PathOs::Unix
            },
            NormalizeTestCase{
                .inputPath = "z://hostname/path/to/share",
                .expectedNormalizedPath = "Z:/hostname/path/to/share",
                .pathOs = PathOs::Windows
            }, // single char before :// is not a scheme url, but windows drive

            // ips - untouched
            NormalizeTestCase{
                .inputPath = "smb://192.168.1.100/share",
                .expectedNormalizedPath = "smb://192.168.1.100/share",
                .pathOs = PathOs::Unix
            },
            NormalizeTestCase{
                .inputPath = "ftp://[::1]:21/share",
                .expectedNormalizedPath = "ftp://[::1]:21/share",
                .pathOs = PathOs::Unix
            },
            // local network hostnames and domain names - untouched
            NormalizeTestCase{
                .inputPath = "nfs://localhost/share",
                .expectedNormalizedPath = "nfs://localhost/share",
                .pathOs = PathOs::Unix
            },
            NormalizeTestCase{
                .inputPath = "smb://example.com/path",
                .expectedNormalizedPath = "smb://example.com/path",
                .pathOs = PathOs::Unix
            },
            // file protocol - untouched
            NormalizeTestCase{
                .inputPath = "file://local/path",
                .expectedNormalizedPath = "file://local/path",
                .pathOs = PathOs::Unix
            },
            // full RFC example with username, password and port - untouched
            NormalizeTestCase{
                .inputPath = "ftp://user:password@example:21/share",
                .expectedNormalizedPath = "ftp://user:password@example:21/share",
                .pathOs = PathOs::Unix
            },
            // same ipv6 - untouched
            NormalizeTestCase{
                .inputPath = "ftp://user:password@[::1]:21/path",
                .expectedNormalizedPath = "ftp://user:password@[::1]:21/path",
                .pathOs = PathOs::Unix
            },
            // weird paths - untouched
            NormalizeTestCase{
                .inputPath = "ftp://:@hostname/path/to/share",
                .expectedNormalizedPath = "ftp://:@hostname/path/to/share",
                .pathOs = PathOs::Unix
            },
            NormalizeTestCase{
                .inputPath = "ftp://hostname:/path/to/share",
                .expectedNormalizedPath = "ftp://hostname:/path/to/share",
                .pathOs = PathOs::Unix
            },
            // weird path - collapse slashes inside
            NormalizeTestCase{
                .inputPath = "/path/with/http://inside",
                .expectedNormalizedPath = "/path/with/http:/inside",
                .pathOs = PathOs::Unix
            }
        };

        for (const auto& [inputPath, expectedNormalizedPath, pathOs] : testCases) {
            QCOMPARE(normalizePath(inputPath, pathOs), expectedNormalizedPath);
        }
    }

    void checkSchemeDetection() {
        // Test cases that should be detected as scheme URLs
        const std::vector<QString> passCases = {
            "ftp://hostname/",
            "ftp://hostname.com/",
            "ftp://@hostname:21/",
            "ftp://user:@hostname:21/",
            "ftp://user:@hostname.com:21/",
            "ftp://user:pass@hostname:21/",
            "ftp://user:pass@hostname:21/asdasd/asdasdasd/",
            "ftp://user:pass@hostname:21//asdasd/asdasdasd/",
            "ftp://user:@hostname://",
            "ftp://user@192.168.100.1/",
            "ftp://user:@192.168.100.1:21/",
            "ftp://user@[::1]/",
            "ftp://user:@[::1]:21/"
        };

        for (const auto& url : passCases) {
            QCOMPARE(isSchemeUrl(url), true);
        }

        // Test cases that should NOT be detected as scheme URLs
        const std::vector<QString> failCases = {
            "C:/",
            "C://",
            "C:\\",
            "C:\\\\",
            "C:/file",
            "C://file/path",
            "C:\\file",
            "C:\\\\file",
            "//scheme://url/insdie/some/path"
            "user@hostname"
            "//user@hostname"
        };

        for (const auto& input : failCases) {
            QCOMPARE(isSchemeUrl(input), false);
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
