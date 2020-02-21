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

#include <iostream>

#include <QFileInfo>
#include <QUrl>

#include "3rdparty/cxxopts.hpp"

namespace tremotesf
{
    namespace
    {
        inline std::string parseAppName(const char* arg)
        {
            const char* sep = strrchr(arg, '/');
            return std::string(sep ? sep + 1 : arg);
        }

        void parsePositional(const std::string& arg, CommandLineArgs& args)
        {
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

    CommandLineArgs parseCommandLine(int& argc, char**& argv)
    {
        CommandLineArgs args{};

        const std::string appName(parseAppName(argv[0]));
        const std::string versionString(appName + " " TREMOTESF_VERSION);
        cxxopts::Options opts(appName, versionString);
        opts.add_options()
            ("v,version", "display version information", cxxopts::value<bool>())
            ("h,help", "display this help", cxxopts::value<bool>())
#ifdef TREMOTESF_SAILFISHOS
            ("torrent", "", cxxopts::value<std::string>()->default_value(""));
            opts.parse_positional("torrent");
            opts.positional_help("torrent");
#else
            ("m,minimized", "start minimized in notification area", cxxopts::value<bool>(args.minimized))
            ("torrents", "", cxxopts::value<std::vector<std::string>>());
            opts.parse_positional("torrents");
            opts.positional_help("torrents");
#endif
        try {
            const auto result(opts.parse(argc, argv));
            if (result["help"].as<bool>()) {
                std::cout << opts.help() << std::endl;
                args.exit = true;
                return args;
            }
            if (result["version"].as<bool>()) {
                std::cout << versionString << std::endl;
                args.exit = true;
                return args;
            }
#ifdef TREMOTESF_SAILFISHOS
            parsePositional(result["torrent"].as<std::string>(), args);
#else
            for (const auto& i : result.arguments()) {
                if (i.key() == "torrents") {
                    parsePositional(i.value(), args);
                }
            }
#endif
        } catch (const cxxopts::OptionException& e) {
            std::cerr << e.what() << std::endl;
            args.exit = true;
            args.returnCode = 1;
        }

        return args;
    }
}
