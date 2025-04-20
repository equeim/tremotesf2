// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "commandlineparser.h"

#include <optional>
#include <span>
#include <stdexcept>
#include <string_view>

#include <QFileInfo>
#include <QUrl>

#include <fmt/format.h>

#define CXXOPTS_VECTOR_DELIMITER '\0'
#include <cxxopts.hpp>

#include "log/log.h"
#include "target_os.h"

namespace tremotesf {
    namespace {
        std::optional<std::string_view> substrAfterChar(std::string_view str, char ch) {
            const auto index = str.rfind(ch);
            if (index == std::string_view::npos) {
                return std::nullopt;
            }
            return str.substr(index + 1);
        }

        std::string_view executableFileName(std::string_view arg0) {
            if constexpr (targetOs == TargetOs::Windows) {
                if (const auto name = substrAfterChar(arg0, '\\'); name) {
                    return *name;
                }
            }
            if (const auto name = substrAfterChar(arg0, '/'); name) {
                return *name;
            }
            return arg0;
        }

        void parsePositionals(std::span<const std::string> torrents, CommandLineArgs& args) {
            for (const std::string& arg : torrents) {
                if (!arg.empty()) {
                    const auto argument(QString::fromStdString(arg));
                    const QFileInfo info(argument);
                    if (info.isFile()) {
                        args.files.push_back(info.absoluteFilePath());
                    } else {
                        const QUrl url(argument);
                        if (url.isLocalFile()) {
                            const auto path = url.toLocalFile();
                            if (QFileInfo(path).isFile()) {
                                args.files.push_back(path);
                            }
                        } else {
                            args.urls.push_back(argument);
                        }
                    }
                }
            }
        }
    }

    CommandLineArgs parseCommandLine(int& argc, char**& argv) {
        CommandLineArgs args{};

        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        const std::string_view appName = executableFileName(argv[0]);
        const auto versionString = fmt::format("{} {}", appName, TREMOTESF_VERSION);
        cxxopts::Options opts(std::string(appName), versionString);
        std::vector<std::string> torrents;
        opts.add_options(
        )("v,version", "display version information", cxxopts::value<bool>()->default_value("false")
        )("h,help", "display this help", cxxopts::value<bool>()->default_value("false")
        )("m,minimized", "start minimized in notification area", cxxopts::value(args.minimized)->default_value("false")
        )("d,debug-logs", "enable debug logs", cxxopts::value(args.enableDebugLogs)->implicit_value("true")
        )("torrents", "", cxxopts::value(torrents));
        opts.parse_positional("torrents");
        opts.positional_help("torrents");

        const auto result(opts.parse(argc, argv));
        if (result["help"].as<bool>()) {
            printlnStdout(opts.help());
            args.exit = true;
            return args;
        }
        if (result["version"].as<bool>()) {
            printlnStdout(versionString);
            args.exit = true;
            return args;
        }
        parsePositionals(torrents, args);

        return args;
    }
}
