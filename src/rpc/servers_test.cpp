// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>

#include <QTest>
#include <QSettings>

#include "servers.h"
#include "pathutils.h"
#include "log/log.h"

namespace tremotesf {
    constexpr auto mountedLocalPathsUnix = std::array{
        "/mnt/downloads"_l1,
        "/mnt/downloads2"_l1,
    };

    constexpr auto mountedLocalPathsWindows = std::array{
        "G:/downloads"_l1,
        "G:/downloads2"_l1,
    };

    constexpr auto mountedLocalPaths = [] {
        if constexpr (targetOs == TargetOs::Windows) {
            return mountedLocalPathsWindows;
        } else {
            return mountedLocalPathsUnix;
        }
    }();

    constexpr auto mountedRemotePathsUnix = std::array{
        "/home/test/Downloads"_l1,
        "/home/test/Downloads2"_l1,
    };

    constexpr auto mountedRemotePathsWindows = std::array{
        "C:/Users/test/Downloads"_l1,
        "C:/Users/test/Downloads2"_l1,
    };

    std::vector<MountedDirectory> createMountedDirectories(PathOs remoteOs) {
        std::vector<MountedDirectory> dirs{};
        dirs.reserve(mountedLocalPaths.size());
        const auto& remotePaths = [remoteOs] {
            return remoteOs == PathOs::Unix ? mountedRemotePathsUnix : mountedRemotePathsWindows;
        }();
        size_t i = 0;
        for (auto localPath : mountedLocalPaths) {
            dirs.push_back({.localPath = localPath, .remotePath = remotePaths.at(i)});
            ++i;
        }
        return dirs;
    }

    class ServersTest final : public QObject {
        Q_OBJECT

    private slots:
        void checkFromLocalToRemoteDirectory() {
            constexpr auto localPaths = [] {
                if constexpr (targetOs == TargetOs::Windows) {
                    return std::array{
                        QLatin1String{},
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
                    return std::array{
                        QLatin1String{},
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
            }();

            mServers.saveServers(
                {{.name = "test", .mountedDirectories = createMountedDirectories(PathOs::Unix)}},
                "test"
            );

            constexpr auto resultsWhenRemoteIsUnix = std::array{
                QLatin1String{},
                QLatin1String{},
                QLatin1String{},
                "/home/test/Downloads"_l1,
                "/home/test/Downloads/"_l1,
                "/home/test/Downloads/hmm"_l1,
                "/home/test/Downloads2"_l1,
                "/home/test/Downloads2/"_l1,
                "/home/test/Downloads2/hmm"_l1,
            };

            for (size_t i = 0; i < localPaths.size(); ++i) {
                info().log("Converting {} to remote path when server is Unix", localPaths[i]);
                QCOMPARE(
                    mServers.fromLocalToRemoteDirectory(localPaths[i], PathOs::Unix),
                    resultsWhenRemoteIsUnix.at(i)
                );
            }

            mServers.saveServers(
                {{.name = "test", .mountedDirectories = createMountedDirectories(PathOs::Windows)}},
                "test"
            );

            constexpr auto resultsWhenRemoteIsWindows = std::array{
                QLatin1String{},
                QLatin1String{},
                QLatin1String{},
                "C:/Users/test/Downloads"_l1,
                "C:/Users/test/Downloads/"_l1,
                "C:/Users/test/Downloads/hmm"_l1,
                "C:/Users/test/Downloads2"_l1,
                "C:/Users/test/Downloads2/"_l1,
                "C:/Users/test/Downloads2/hmm"_l1,
            };

            for (size_t i = 0; i < localPaths.size(); ++i) {
                info().log("Converting {} to remote path when server is Windows", localPaths[i]);
                QCOMPARE(
                    mServers.fromLocalToRemoteDirectory(localPaths[i], PathOs::Windows),
                    resultsWhenRemoteIsWindows.at(i)
                );
            }
        }

        void checkFromRemoteToLocalDirectory() {
            constexpr auto results = [] {
                if constexpr (targetOs == TargetOs::Windows) {
                    return std::array{
                        QLatin1String{},
                        QLatin1String{},
                        QLatin1String{},
                        "G:/downloads"_l1,
                        "G:/downloads/"_l1,
                        "G:/downloads/hmm"_l1,
                        "G:/downloads2"_l1,
                        "G:/downloads2/"_l1,
                        "G:/downloads2/hmm"_l1,
                    };
                } else {
                    return std::array{
                        QLatin1String{},
                        QLatin1String{},
                        QLatin1String{},
                        "/mnt/downloads"_l1,
                        "/mnt/downloads/"_l1,
                        "/mnt/downloads/hmm"_l1,
                        "/mnt/downloads2"_l1,
                        "/mnt/downloads2/"_l1,
                        "/mnt/downloads2/hmm"_l1,
                    };
                }
            }();

            mServers.saveServers(
                {{.name = "test", .mountedDirectories = createMountedDirectories(PathOs::Unix)}},
                "test"
            );

            constexpr auto remotePathsOnUnix = std::array{
                QLatin1String{},
                "/"_l1,
                "/nope"_l1,
                "/home/test/Downloads"_l1,
                "/home/test/Downloads/"_l1,
                "/home/test/Downloads/hmm"_l1,
                "/home/test/Downloads2"_l1,
                "/home/test/Downloads2/"_l1,
                "/home/test/Downloads2/hmm"_l1,
            };

            for (size_t i = 0; i < remotePathsOnUnix.size(); ++i) {
                info().log("Converting {} to local path when server is Unix", remotePathsOnUnix[i]);
                QCOMPARE(mServers.fromRemoteToLocalDirectory(remotePathsOnUnix[i], PathOs::Unix), results.at(i));
            }

            mServers.saveServers(
                {{.name = "test", .mountedDirectories = createMountedDirectories(PathOs::Windows)}},
                "test"
            );

            constexpr auto remotePathsOnWindows = std::array{
                QLatin1String{},
                "C:/"_l1,
                "C:/nope"_l1,
                "C:/Users/test/Downloads"_l1,
                "C:/Users/test/Downloads/"_l1,
                "C:/Users/test/Downloads/hmm"_l1,
                "C:/Users/test/Downloads2"_l1,
                "C:/Users/test/Downloads2/"_l1,
                "C:/Users/test/Downloads2/hmm"_l1,
            };

            for (size_t i = 0; i < remotePathsOnWindows.size(); ++i) {
                info().log("Converting {} to local path when server is Windows", remotePathsOnWindows[i]);
                QCOMPARE(mServers.fromRemoteToLocalDirectory(remotePathsOnWindows[i], PathOs::Windows), results.at(i));
            }
        }

    private:
        Servers mServers{new QSettings(QString{}, QSettings::IniFormat), nullptr};
    };
}

QTEST_GUILESS_MAIN(tremotesf::ServersTest)

#include "servers_test.moc"
