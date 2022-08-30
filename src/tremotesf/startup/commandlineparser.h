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

#ifndef TREMOTESF_COMMANDLINEPARSER_H
#define TREMOTESF_COMMANDLINEPARSER_H

#include <QStringList>

namespace tremotesf
{
    struct CommandLineArgs
    {
        QStringList files;
        QStringList urls;
        bool minimized;

        bool exit;
    };

    CommandLineArgs parseCommandLine(int& argc, char**& argv);
}

#endif // TREMOTESF_COMMANDLINEPARSER_H
