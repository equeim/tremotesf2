/*
 * Tremotesf
 * Copyright (C) 2015-2020 Alexey Rochev <equeim@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "commandlineparser.h"

#include <stdexcept>

#include <QFileInfo>
#include <QUrl>

#define CXXOPTS_VECTOR_DELIMITER '\0'
#include "3rdparty/cxxopts.hpp"

#include "libtremotesf/log.h"

namespace tremotesf
{
    namespace
    {
        inline std::string parseAppName(const char* arg)
        {
            const char* sep = strrchr(arg, '/');
            return std::string(sep ? sep + 1 : arg);
        }

        void parsePositionals(const std::vector<std::string>& torrents, CommandLineArgs& args)
        {
            for (const std::string& arg : torrents) {
                if (!arg.empty()) {
                    const auto argument(QString::fromStdString(arg));
                    const QFileInfo info(argument);
                    if (info.isFile()) {
                        args.files.push_back(info.absoluteFilePath());
                    } else {
                        const QUrl url(argument);
                        if (url.isLocalFile()) {
                            if (QFileInfo(url.path()).isFile()) {
                                args.files.push_back(url.path());
                            }
                        } else {
                            args.urls.push_back(argument);
                        }
                    }
                }
            }
        }
    }

    CommandLineArgs parseCommandLine(int& argc, char**& argv)
    {
        CommandLineArgs args{};

        const std::string appName(parseAppName(argv[0]));
        const auto versionString = fmt::format("{} {}", appName, TREMOTESF_VERSION);
        cxxopts::Options opts(appName, versionString);
        std::vector<std::string> torrents;
        opts.add_options()
            ("v,version", "display version information", cxxopts::value<bool>())
            ("h,help", "display this help", cxxopts::value<bool>())
            ("m,minimized", "start minimized in notification area", cxxopts::value<bool>(args.minimized))
            ("torrents", "", cxxopts::value<decltype(torrents)>(torrents));
            opts.parse_positional("torrents");
            opts.positional_help("torrents");
        try {
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
        } catch (const cxxopts::OptionException& e) {
            throw std::runtime_error(e.what());
        }

        return args;
    }
}
