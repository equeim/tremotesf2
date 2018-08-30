/*
 * Tremotesf
 * Copyright (C) 2015-2018 Alexey Rochev <equeim@gmail.com>
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

#include "ipcserver.h"

#include <QFileInfo>
#include <QUrl>

namespace tremotesf
{
    void IpcServer::parseArgument(const QString& argument, ArgumentsParseResult& result)
    {
        const QFileInfo info(argument);
        if (info.isFile()) {
            result.files.push_back(info.absoluteFilePath());
        } else {
            const QUrl url(argument);
            if (url.isLocalFile()) {
                if (QFileInfo(url.path()).isFile()) {
                    result.files.push_back(url.path());
                }
            } else {
                result.urls.push_back(argument);
            }
        }
    }

    ArgumentsParseResult IpcServer::parseArguments(const QStringList& arguments)
    {
        ArgumentsParseResult result;
        for (const QString& argument : arguments) {
            parseArgument(argument, result);
        }
        return result;
    }
}
