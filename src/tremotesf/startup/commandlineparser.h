// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_COMMANDLINEPARSER_H
#define TREMOTESF_COMMANDLINEPARSER_H

#include <optional>
#include <QStringList>

namespace tremotesf {
    struct CommandLineArgs {
        QStringList files{};
        QStringList urls{};
        bool minimized{};
        std::optional<bool> enableDebugLogs{};

        bool exit{};
    };

    CommandLineArgs parseCommandLine(int& argc, char**& argv);
}

#endif // TREMOTESF_COMMANDLINEPARSER_H
