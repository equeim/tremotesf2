/*
 * Tremotesf
 * Copyright (C) 2015-2016 Alexey Rochev <equeim@gmail.com>
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

#include "torrentfileparser.h"

#include <QDebug>
#include <QFile>
#include <QMimeDatabase>

namespace tremotesf
{
    TorrentFileParser::TorrentFileParser(const QString& filePath, QObject* parent)
        : QObject(parent),
          mError(false)
    {
        setFilePath(filePath);
    }

    const QString& TorrentFileParser::filePath() const
    {
        return mFilePath;
    }

    void TorrentFileParser::setFilePath(const QString& path)
    {
        if (path == mFilePath) {
            return;
        }

        mFilePath = path;
        mData.clear();
        mIndex = 0;
        mError = false;
        mResult.clear();

        if (!mFilePath.isEmpty()) {
            const QMimeDatabase database;
            const QMimeType mimeType(database.mimeTypeForFile(mFilePath, QMimeDatabase::MatchContent));
            if (mimeType.name() == "application/x-bittorrent") {
                QFile file(mFilePath);
                if (file.open(QIODevice::ReadOnly)) {
                    mData = file.readAll();
                    mResult = parseMap();
                    if (mError) {
                        qDebug() << "Parsing error";
                        mResult.clear();
                    }
                } else {
                    qDebug() << "Error opening file for reading";
                    mError = true;
                }
            } else {
                qDebug() << "Wrong MIME type:" << mimeType.name();
                mError = true;
            }
        }

        emit done();
    }

    bool TorrentFileParser::error() const
    {
        return mError;
    }

    const QVariantMap& TorrentFileParser::result() const
    {
        return mResult;
    }

    QVariant TorrentFileParser::parseVariant()
    {
        const char firstChar = mData.at(mIndex);
        if (firstChar == 'i') {
            return parseInt();
        }
        if (firstChar == 'l') {
            return parseList();
        }
        if (firstChar == 'd') {
            return parseMap();
        }
        if (QChar(firstChar).isDigit()) {
            return parseString();
        }

        mError = true;
        return QVariant();
    }

    QVariantMap TorrentFileParser::parseMap()
    {
        QVariantMap map;
        ++mIndex;
        while (mData.at(mIndex) != 'e' && !mError) {
            const QString key(parseString());
            const QVariant value(parseVariant());
            map.insert(key, value);
        }
        ++mIndex;
        return map;
    }

    QVariantList TorrentFileParser::parseList()
    {
        QVariantList list;
        ++mIndex;
        while (mData.at(mIndex) != 'e' && !mError) {
            list.append(parseVariant());
        }
        ++mIndex;
        return list;
    }

    QString TorrentFileParser::parseString()
    {
        const int separatorIndex = mData.indexOf(':', mIndex);

        if (separatorIndex == -1) {
            mError = true;
            return QString();
        }

        const int length = mData.mid(mIndex, separatorIndex - mIndex).toInt();
        mIndex = separatorIndex + 1;
        const QString string(mData.mid(mIndex, length));
        mIndex += length;
        return string;
    }

    qint64 TorrentFileParser::parseInt()
    {
        mIndex++;
        const int endIndex = mData.indexOf('e', mIndex);

        if (endIndex == -1) {
            mError = true;
            return 0;
        }

        bool ok;
        const qint64 number = mData.mid(mIndex, endIndex - mIndex).toLongLong(&ok);

        if (!ok) {
            mError = true;
            return 0;
        }

        mIndex = endIndex + 1;
        return number;
    }
}
