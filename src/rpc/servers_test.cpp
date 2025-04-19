// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <ranges>

#include <QTest>
#include <QSettings>

#include "servers.h"
#include "pathutils.h"
#include "log/log.h"

using namespace Qt::StringLiterals;

namespace fmt {
    template<>
    struct formatter<tremotesf::PathOs> : tremotesf::SimpleFormatter {
        format_context::iterator format(tremotesf::PathOs pathOs, format_context& ctx) const {
            switch (pathOs) {
            case tremotesf::PathOs::Unix:
                return fmt::format_to(ctx.out(), "Unix");
            case tremotesf::PathOs::Windows:
                return fmt::format_to(ctx.out(), "Windows");
                break;
            }
            throw std::logic_error("Unknown PathOs value");
        }
    };
}

namespace tremotesf {
    std::vector<MountedDirectory>
    createMountedDirectories(std::span<const QString> localDirs, std::span<const QString> remoteDirs) {
        std::vector<MountedDirectory> dirs{};
        dirs.reserve(localDirs.size());
        for (const auto& [local, remote] : std::views::zip(localDirs, remoteDirs)) {
            dirs.push_back({.localPath = local, .remotePath = remote});
        }
        return dirs;
    }

    class ServersTest final : public QObject {
        Q_OBJECT

    private slots:
        void Case1() {
            check(
                {.mountedLocalDirectories = []() -> std::vector<QString> {
                     if constexpr (targetOs == TargetOs::Windows) {
                         return {"G:/downloads"_L1, "G:/downloads2"_L1};
                     } else {
                         return {"/mnt/downloads"_L1, "/mnt/downloads2"_L1};
                     }
                 }(),
                 .mountedRemoteDirectoriesWhenServerIsUnix = {"/home/test/Downloads"_L1, "/home/test/Downloads2"_L1},
                 .mountedRemoteDirectoriesWhenServerIsWindows =
                     {"C:/Users/test/Downloads"_L1, "C:/Users/test/Downloads2"_L1},

                 .localPathsToCheck = []() -> std::vector<QString> {
                     if constexpr (targetOs == TargetOs::Windows) {
                         return {
                             {},
                             "G:/"_L1,
                             "G:/nope"_L1,
                             "G:/downloads"_L1,
                             "G:/downloads/"_L1,
                             "G:/downloads/hmm"_L1,
                             "G:/downloads2"_L1,
                             "G:/downloads2/"_L1,
                             "G:/downloads2/hmm"_L1,
                         };
                     } else {
                         return {
                             {},
                             "/"_L1,
                             "/nope"_L1,
                             "/mnt/downloads"_L1,
                             "/mnt/downloads/"_L1,
                             "/mnt/downloads/hmm"_L1,
                             "/mnt/downloads2"_L1,
                             "/mnt/downloads2/"_L1,
                             "/mnt/downloads2/hmm"_L1,
                         };
                     }
                 }(),
                 .expectedRemotePathsWhenServerIsUnix =
                     {
                         {},
                         {},
                         {},
                         "/home/test/Downloads"_L1,
                         "/home/test/Downloads"_L1,
                         "/home/test/Downloads/hmm"_L1,
                         "/home/test/Downloads2"_L1,
                         "/home/test/Downloads2"_L1,
                         "/home/test/Downloads2/hmm"_L1,
                     },
                 .expectedRemotePathsWhenServerIsWindows =
                     {
                         {},
                         {},
                         {},
                         "C:/Users/test/Downloads"_L1,
                         "C:/Users/test/Downloads"_L1,
                         "C:/Users/test/Downloads/hmm"_L1,
                         "C:/Users/test/Downloads2"_L1,
                         "C:/Users/test/Downloads2"_L1,
                         "C:/Users/test/Downloads2/hmm"_L1,
                     },

                 .remotePathsToCheckWhenServerIsUnix =
                     {
                         {},
                         "/"_L1,
                         "/nope"_L1,
                         "/home/test/Downloads"_L1,
                         "/home/test/Downloads/"_L1,
                         "/home/test/Downloads/hmm"_L1,
                         "/home/test/Downloads2"_L1,
                         "/home/test/Downloads2/"_L1,
                         "/home/test/Downloads2/hmm"_L1,
                     },
                 .remotePathsToCheckWhenServerIsWindows =
                     {
                         {},
                         "C:/"_L1,
                         "C:/nope"_L1,
                         "C:/Users/test/Downloads"_L1,
                         "C:/Users/test/Downloads/"_L1,
                         "C:/Users/test/Downloads/hmm"_L1,
                         "C:/Users/test/Downloads2"_L1,
                         "C:/Users/test/Downloads2/"_L1,
                         "C:/Users/test/Downloads2/hmm"_L1,
                     },
                 .expectedLocalPaths = []() -> std::vector<QString> {
                     if constexpr (targetOs == TargetOs::Windows) {
                         return {
                             {},
                             {},
                             {},
                             "G:/downloads"_L1,
                             "G:/downloads"_L1,
                             "G:/downloads/hmm"_L1,
                             "G:/downloads2"_L1,
                             "G:/downloads2"_L1,
                             "G:/downloads2/hmm"_L1,
                         };
                     } else {
                         return {
                             {},
                             {},
                             {},
                             "/mnt/downloads"_L1,
                             "/mnt/downloads"_L1,
                             "/mnt/downloads/hmm"_L1,
                             "/mnt/downloads2"_L1,
                             "/mnt/downloads2"_L1,
                             "/mnt/downloads2/hmm"_L1,
                         };
                     }
                 }()}
            );
        }

        void Case2() {
            check(
                {.mountedLocalDirectories = []() -> std::vector<QString> {
                     if constexpr (targetOs == TargetOs::Windows) {
                         return {"G:/root"_L1};
                     } else {
                         return {"/mnt/root"_L1};
                     }
                 }(),
                 .mountedRemoteDirectoriesWhenServerIsUnix = {"/"_L1},
                 .mountedRemoteDirectoriesWhenServerIsWindows = {"D:/"_L1},

                 .localPathsToCheck = []() -> std::vector<QString> {
                     if constexpr (targetOs == TargetOs::Windows) {
                         return {"G:/root"_L1, "G:/root/"_L1, "G:/root/hmm"_L1, "G:/root/hmm/"_L1};
                     } else {
                         return {"/mnt/root"_L1, "/mnt/root/"_L1, "/mnt/root/hmm"_L1, "/mnt/root/hmm/"_L1};
                     }
                 }(),
                 .expectedRemotePathsWhenServerIsUnix = {"/"_L1, "/"_L1, "/hmm"_L1, "/hmm"_L1},
                 .expectedRemotePathsWhenServerIsWindows = {"D:/"_L1, "D:/"_L1, "D:/hmm"_L1, "D:/hmm"},

                 .remotePathsToCheckWhenServerIsUnix = {"/"_L1, "/hmm"_L1, "/hmm/"_L1},
                 .remotePathsToCheckWhenServerIsWindows = {"D:/"_L1, "D:/hmm"_L1, "D:/hmm/"_L1},
                 .expectedLocalPaths = []() -> std::vector<QString> {
                     if constexpr (targetOs == TargetOs::Windows) {
                         return {"G:/root"_L1, "G:/root/hmm"_L1, "G:/root/hmm"_L1};
                     } else {
                         return {"/mnt/root"_L1, "/mnt/root/hmm"_L1, "/mnt/root/hmm"_L1};
                     }
                 }()}
            );
        }

        void Case3() {
            check(
                {.mountedLocalDirectories = []() -> std::vector<QString> {
                     if constexpr (targetOs == TargetOs::Windows) {
                         return {"//42.42.42.42/D"_L1};
                     } else {
                         return {"/remote"_L1};
                     }
                 }(),
                 .mountedRemoteDirectoriesWhenServerIsUnix = {"/"_L1},
                 .mountedRemoteDirectoriesWhenServerIsWindows = {"D:"_L1},

                 .localPathsToCheck = []() -> std::vector<QString> {
                     if constexpr (targetOs == TargetOs::Windows) {
                         return {
                             "//42.42.42.42/D"_L1,
                             "//42.42.42.42/D/"_L1,
                             "//42.42.42.42/D/hmm"_L1,
                             "//42.42.42.42/D/hmm/"_L1,
                         };
                     } else {
                         return {
                             "/remote"_L1,
                             "/remote/"_L1,
                             "/remote/hmm"_L1,
                             "/remote/hmm/"_L1,
                         };
                     }
                 }(),
                 .expectedRemotePathsWhenServerIsUnix = {"/"_L1, "/"_L1, "/hmm"_L1, "/hmm"_L1},
                 .expectedRemotePathsWhenServerIsWindows = {"D:/"_L1, "D:/"_L1, "D:/hmm"_L1, "D:/hmm"_L1},

                 .remotePathsToCheckWhenServerIsUnix = {"/"_L1, "/hmm"_L1, "/hmm/"_L1},
                 .remotePathsToCheckWhenServerIsWindows = {"D:/"_L1, "D:/hmm"_L1, "D:/hmm/"_L1},
                 .expectedLocalPaths = []() -> std::vector<QString> {
                     if constexpr (targetOs == TargetOs::Windows) {
                         return {"//42.42.42.42/D"_L1, "//42.42.42.42/D/hmm"_L1, "//42.42.42.42/D/hmm"_L1};
                     } else {
                         return {"/remote"_L1, "/remote/hmm"_L1, "/remote/hmm"_L1};
                     }
                 }()}
            );
        }

    private:
        struct TestCase {
            std::vector<QString> mountedLocalDirectories;
            std::vector<QString> mountedRemoteDirectoriesWhenServerIsUnix;
            std::vector<QString> mountedRemoteDirectoriesWhenServerIsWindows;

            std::vector<QString> localPathsToCheck;
            std::vector<QString> expectedRemotePathsWhenServerIsUnix;
            std::vector<QString> expectedRemotePathsWhenServerIsWindows;

            std::vector<QString> remotePathsToCheckWhenServerIsUnix;
            std::vector<QString> remotePathsToCheckWhenServerIsWindows;
            std::vector<QString> expectedLocalPaths;
        };

        void check(TestCase testCase) {
            mServers.saveServers(
                {{.name = "test",
                  .mountedDirectories = createMountedDirectories(
                      testCase.mountedLocalDirectories,
                      testCase.mountedRemoteDirectoriesWhenServerIsUnix
                  )}},
                "test"
            );

            checkFromLocalToRemoteDirectory(
                testCase.localPathsToCheck,
                PathOs::Unix,
                testCase.expectedRemotePathsWhenServerIsUnix
            );
            if (QTest::currentTestFailed()) return;

            checkFromRemoteToLocalDirectory(
                testCase.remotePathsToCheckWhenServerIsUnix,
                PathOs::Unix,
                testCase.expectedLocalPaths
            );
            if (QTest::currentTestFailed()) return;

            mServers.saveServers(
                {{.name = "test",
                  .mountedDirectories = createMountedDirectories(
                      testCase.mountedLocalDirectories,
                      testCase.mountedRemoteDirectoriesWhenServerIsWindows
                  )}},
                "test"
            );

            checkFromLocalToRemoteDirectory(
                testCase.localPathsToCheck,
                PathOs::Windows,
                testCase.expectedRemotePathsWhenServerIsWindows
            );
            if (QTest::currentTestFailed()) return;

            checkFromRemoteToLocalDirectory(
                testCase.remotePathsToCheckWhenServerIsWindows,
                PathOs::Windows,
                testCase.expectedLocalPaths
            );
        }

        void checkFromLocalToRemoteDirectory(
            std::span<const QString> localPaths, PathOs remotePathOs, std::span<const QString> expectedRemotePaths
        ) {
            for (const auto& [localPath, expectedRemotePath] : std::views::zip(localPaths, expectedRemotePaths)) {
                info().log("Converting {} to remote path when server is {}", localPath, remotePathOs);
                QCOMPARE(mServers.fromLocalToRemoteDirectory(localPath, remotePathOs), expectedRemotePath);
            }
        }

        void checkFromRemoteToLocalDirectory(
            std::span<const QString> remotePaths, PathOs remotePathOs, std::span<const QString> expectedLocalPaths
        ) {
            for (const auto& [remotePath, expectedLocalPath] : std::views::zip(remotePaths, expectedLocalPaths)) {
                info().log("Converting {} to local path when server is {}", remotePath, remotePathOs);
                QCOMPARE(mServers.fromRemoteToLocalDirectory(remotePath, remotePathOs), expectedLocalPath);
            }
        }

        Servers mServers{new QSettings(QString{}, QSettings::IniFormat), nullptr};
    };
}

QTEST_GUILESS_MAIN(tremotesf::ServersTest)

#include "servers_test.moc"
