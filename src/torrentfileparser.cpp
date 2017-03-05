/*
 * Tremotesf
 * Copyright (C) 2015-2017 Alexey Rochev <equeim@gmail.com>
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

#include <memory>

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QFutureWatcher>
#include <QMimeDatabase>
#include <QtConcurrentRun>

namespace tremotesf
{
    namespace {
        class Worker
        {
        public:
            void parse(const QString& filePath)
            {
                QFile file(filePath);
                if (file.open(QFile::ReadOnly)) {
                    const QMimeType mimeType(QMimeDatabase().mimeTypeForFile(filePath,
                                                                             QMimeDatabase::MatchContent));
                    if (mimeType.name() == "application/x-bittorrent") {
                        fileData = file.readAll();
                        parseResult = parseMap();
                        if (error != TorrentFileParser::NoError) {
                            fileData.clear();
                            parseResult.clear();
                            qWarning() << "error parsing file";
                        }
                    } else {
                        qWarning() << "wrong mime type" << mimeType.name();
                        error = TorrentFileParser::WrongMimeType;
                    }
                } else {
                    qWarning() << "error reading file";
                    error = TorrentFileParser::ReadingError;
                }
            }

            QByteArray fileData;
            QVariantMap parseResult;
            TorrentFileParser::Error error;

        private:
            QVariant parseVariant()
            {
                const char firstChar = fileData.at(mIndex);
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
                error = TorrentFileParser::ParsingError;
                return QVariant();
            }

            QVariantMap parseMap()
            {
                QVariantMap map;
                ++mIndex;
                while ((fileData.at(mIndex) != 'e') && (error == TorrentFileParser::NoError)) {
                    const QString key(parseString());
                    const QVariant value(parseVariant());
                    map.insert(key, value);
                }
                ++mIndex;
                return map;
            }

            QVariantList parseList()
            {
                QVariantList list;
                ++mIndex;
                while ((fileData.at(mIndex) != 'e') && (error == TorrentFileParser::NoError)) {
                    list.append(parseVariant());
                }
                ++mIndex;
                return list;
            }

            QString parseString()
            {
                const int separatorIndex = fileData.indexOf(':', mIndex);

                if (separatorIndex == -1) {
                    error = TorrentFileParser::ParsingError;
                    return QString();
                }

                const int length = fileData.mid(mIndex, separatorIndex - mIndex).toInt();
                mIndex = separatorIndex + 1;
                const QString string(fileData.mid(mIndex, length));
                mIndex += length;
                return string;
            }

            long long parseInt()
            {
                mIndex++;
                const int endIndex = fileData.indexOf('e', mIndex);

                if (endIndex == -1) {
                    error = TorrentFileParser::ParsingError;
                    return 0;
                }

                bool ok;
                const long long number = fileData.mid(mIndex, endIndex - mIndex).toLongLong(&ok);

                if (!ok) {
                    error = TorrentFileParser::ParsingError;
                    return 0;
                }

                mIndex = endIndex + 1;
                return number;
            }

            int mIndex = 0;
        };
    }

    TorrentFileParser::TorrentFileParser(const QString& filePath, QObject* parent)
        : QObject(parent),
          mIndex(0),
          mError(NoError),
          mLoaded(false)
    {
        setFilePath(filePath);
    }

    const QString& TorrentFileParser::filePath() const
    {
        return mFilePath;
    }

    void TorrentFileParser::setFilePath(const QString& path)
    {
        if (!path.isEmpty() && mFilePath.isEmpty()) {
            mFilePath = path;
            parse();
        }
    }

    bool TorrentFileParser::isLoaded() const
    {
        return mLoaded;
    }

    TorrentFileParser::Error TorrentFileParser::error() const
    {
        return mError;
    }

    QString TorrentFileParser::errorString() const
    {
        switch (mError) {
        case ReadingError:
            return qApp->translate("tremotesf", "Error reading torrent file");
        case WrongMimeType:
            return qApp->translate("tremotesf", "Wrong MIME type");
        case ParsingError:
            return qApp->translate("tremotesf", "Error parsing torrent file");
        default:
            return QString();
        }
    }

    const QByteArray& TorrentFileParser::fileData() const
    {
        return mFileData;
    }

    const QVariantMap& TorrentFileParser::parseResult() const
    {
        return mParseResult;
    }

    void TorrentFileParser::parse()
    {
        auto worker = std::make_shared<Worker>();

        const QString& filePath = mFilePath;
        const auto future = QtConcurrent::run([=]() {
            worker->parse(filePath);
        });

        auto watcher = new QFutureWatcher<void>(this);
        QObject::connect(watcher, &QFutureWatcher<void>::finished, this, [=]() {
            mFileData = worker->fileData;
            mParseResult = worker->parseResult;
            mError = worker->error;
            mLoaded = true;
            emit done();
            watcher->deleteLater();
        });
        watcher->setFuture(future);
    }
}
