// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>

#include <QTest>
#include <QSettings>

#include "servers.h"
#include "pathutils.h"
#include "log/log.h"

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
        for (size_t i = 0; i < localDirs.size(); ++i) {
            dirs.push_back({.localPath = localDirs[i], .remotePath = remoteDirs[i]});
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
                         return {"G:/downloads"_l1, "G:/downloads2"_l1};
                     } else {
                         return {"/mnt/downloads"_l1, "/mnt/downloads2"_l1};
                     }
                 }(),
                 .mountedRemoteDirectoriesWhenServerIsUnix = {"/home/test/Downloads"_l1, "/home/test/Downloads2"_l1},
                 .mountedRemoteDirectoriesWhenServerIsWindows =
                     {"C:/Users/test/Downloads"_l1, "C:/Users/test/Downloads2"_l1},

                 .localPathsToCheck = []() -> std::vector<QString> {
                     if constexpr (targetOs == TargetOs::Windows) {
                         return {
                             {},
                             "G:/"_l1,
                             "G:/nope"_l1,
                             "G:/downloads"_l1,
                             "G:/downloads/"_l1,
                             "G:/downloads/hmm"_l1,
                             "G:/downloads2"_l1,
                             "G:/downloads2/"_l1,
                             "G:/downloads2/hmm"_l1,
                         };
                     } else {
                         return {
                             {},
                             "/"_l1,
                             "/nope"_l1,
                             "/mnt/downloads"_l1,
                             "/mnt/downloads/"_l1,
                             "/mnt/downloads/hmm"_l1,
                             "/mnt/downloads2"_l1,
                             "/mnt/downloads2/"_l1,
                             "/mnt/downloads2/hmm"_l1,
                         };
                     }
                 }(),
                 .expectedRemotePathsWhenServerIsUnix =
                     {
                         {},
                         {},
                         {},
                         "/home/test/Downloads"_l1,
                         "/home/test/Downloads"_l1,
                         "/home/test/Downloads/hmm"_l1,
                         "/home/test/Downloads2"_l1,
                         "/home/test/Downloads2"_l1,
                         "/home/test/Downloads2/hmm"_l1,
                     },
                 .expectedRemotePathsWhenServerIsWindows =
                     {
                         {},
                         {},
                         {},
                         "C:/Users/test/Downloads"_l1,
                         "C:/Users/test/Downloads"_l1,
                         "C:/Users/test/Downloads/hmm"_l1,
                         "C:/Users/test/Downloads2"_l1,
                         "C:/Users/test/Downloads2"_l1,
                         "C:/Users/test/Downloads2/hmm"_l1,
                     },

                 .remotePathsToCheckWhenServerIsUnix =
                     {
                         {},
                         "/"_l1,
                         "/nope"_l1,
                         "/home/test/Downloads"_l1,
                         "/home/test/Downloads/"_l1,
                         "/home/test/Downloads/hmm"_l1,
                         "/home/test/Downloads2"_l1,
                         "/home/test/Downloads2/"_l1,
                         "/home/test/Downloads2/hmm"_l1,
                     },
                 .remotePathsToCheckWhenServerIsWindows =
                     {
                         {},
                         "C:/"_l1,
                         "C:/nope"_l1,
                         "C:/Users/test/Downloads"_l1,
                         "C:/Users/test/Downloads/"_l1,
                         "C:/Users/test/Downloads/hmm"_l1,
                         "C:/Users/test/Downloads2"_l1,
                         "C:/Users/test/Downloads2/"_l1,
                         "C:/Users/test/Downloads2/hmm"_l1,
                     },
                 .expectedLocalPaths = []() -> std::vector<QString> {
                     if constexpr (targetOs == TargetOs::Windows) {
                         return {
                             {},
                             {},
                             {},
                             "G:/downloads"_l1,
                             "G:/downloads"_l1,
                             "G:/downloads/hmm"_l1,
                             "G:/downloads2"_l1,
                             "G:/downloads2"_l1,
                             "G:/downloads2/hmm"_l1,
                         };
                     } else {
                         return {
                             {},
                             {},
                             {},
                             "/mnt/downloads"_l1,
                             "/mnt/downloads"_l1,
                             "/mnt/downloads/hmm"_l1,
                             "/mnt/downloads2"_l1,
                             "/mnt/downloads2"_l1,
                             "/mnt/downloads2/hmm"_l1,
                         };
                     }
                 }()}
            );
        }

        void Case2() {
            check(
                {.mountedLocalDirectories = []() -> std::vector<QString> {
                     if constexpr (targetOs == TargetOs::Windows) {
                         return {"G:/root"_l1};
                     } else {
                         return {"/mnt/root"_l1};
                     }
                 }(),
                 .mountedRemoteDirectoriesWhenServerIsUnix = {"/"_l1},
                 .mountedRemoteDirectoriesWhenServerIsWindows = {"D:/"_l1},

                 .localPathsToCheck = []() -> std::vector<QString> {
                     if constexpr (targetOs == TargetOs::Windows) {
                         return {"G:/root"_l1, "G:/root/"_l1, "G:/root/hmm"_l1, "G:/root/hmm/"_l1};
                     } else {
                         return {"/mnt/root"_l1, "/mnt/root/"_l1, "/mnt/root/hmm"_l1, "/mnt/root/hmm/"_l1};
                     }
                 }(),
                 .expectedRemotePathsWhenServerIsUnix = {"/"_l1, "/"_l1, "/hmm"_l1, "/hmm"_l1},
                 .expectedRemotePathsWhenServerIsWindows = {"D:/"_l1, "D:/"_l1, "D:/hmm"_l1, "D:/hmm"},

                 .remotePathsToCheckWhenServerIsUnix = {"/"_l1, "/hmm"_l1, "/hmm/"_l1},
                 .remotePathsToCheckWhenServerIsWindows = {"D:/"_l1, "D:/hmm"_l1, "D:/hmm/"_l1},
                 .expectedLocalPaths = []() -> std::vector<QString> {
                     if constexpr (targetOs == TargetOs::Windows) {
                         return {"G:/root"_l1, "G:/root/hmm"_l1, "G:/root/hmm"_l1};
                     } else {
                         return {"/mnt/root"_l1, "/mnt/root/hmm"_l1, "/mnt/root/hmm"_l1};
                     }
                 }()}
            );
        }

        void Case3() {
            check(
                {.mountedLocalDirectories = []() -> std::vector<QString> {
                     if constexpr (targetOs == TargetOs::Windows) {
                         return {"//42.42.42.42/D"_l1};
                     } else {
                         return {"/remote"_l1};
                     }
                 }(),
                 .mountedRemoteDirectoriesWhenServerIsUnix = {"/"_l1},
                 .mountedRemoteDirectoriesWhenServerIsWindows = {"D:"_l1},

                 .localPathsToCheck = []() -> std::vector<QString> {
                     if constexpr (targetOs == TargetOs::Windows) {
                         return {
                             "//42.42.42.42/D"_l1,
                             "//42.42.42.42/D/"_l1,
                             "//42.42.42.42/D/hmm"_l1,
                             "//42.42.42.42/D/hmm/"_l1,
                         };
                     } else {
                         return {
                             "/remote"_l1,
                             "/remote/"_l1,
                             "/remote/hmm"_l1,
                             "/remote/hmm/"_l1,
                         };
                     }
                 }(),
                 .expectedRemotePathsWhenServerIsUnix = {"/"_l1, "/"_l1, "/hmm"_l1, "/hmm"_l1},
                 .expectedRemotePathsWhenServerIsWindows = {"D:/"_l1, "D:/"_l1, "D:/hmm"_l1, "D:/hmm"_l1},

                 .remotePathsToCheckWhenServerIsUnix = {"/"_l1, "/hmm"_l1, "/hmm/"_l1},
                 .remotePathsToCheckWhenServerIsWindows = {"D:/"_l1, "D:/hmm"_l1, "D:/hmm/"_l1},
                 .expectedLocalPaths = []() -> std::vector<QString> {
                     if constexpr (targetOs == TargetOs::Windows) {
                         return {"//42.42.42.42/D"_l1, "//42.42.42.42/D/hmm"_l1, "//42.42.42.42/D/hmm"_l1};
                     } else {
                         return {"/remote"_l1, "/remote/hmm"_l1, "/remote/hmm"_l1};
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
            for (size_t i = 0; i < localPaths.size(); ++i) {
                info().log("Converting {} to remote path when server is {}", localPaths[i], remotePathOs);
                QCOMPARE(mServers.fromLocalToRemoteDirectory(localPaths[i], remotePathOs), expectedRemotePaths[i]);
            }
        }

        void checkFromRemoteToLocalDirectory(
            std::span<const QString> remotePaths, PathOs remotePathOs, std::span<const QString> expectedLocalPaths
        ) {
            for (size_t i = 0; i < remotePaths.size(); ++i) {
                info().log("Converting {} to local path when server is {}", remotePaths[i], remotePathOs);
                QCOMPARE(mServers.fromRemoteToLocalDirectory(remotePaths[i], remotePathOs), expectedLocalPaths[i]);
            }
        }

        Servers mServers{new QSettings(QString{}, QSettings::IniFormat), nullptr};
    };
}

QTEST_GUILESS_MAIN(tremotesf::ServersTest)

#include "servers_test.moc"
