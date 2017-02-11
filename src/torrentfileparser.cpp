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

#include <QCoreApplication>
#include <QFile>
#include <QDebug>
#include <QMimeDatabase>
#include <QThread>

namespace tremotesf
{
    namespace
    {
        class Worker : public QObject
        {
            Q_OBJECT
        public:
            void parse(const QString& filePath)
            {
                QVariantMap result;
                TorrentFileParser::Error error = TorrentFileParser::NoError;
                QFile file(filePath);
                if (file.open(QFile::ReadOnly)) {
                    const QMimeType mimeType(QMimeDatabase().mimeTypeForFile(filePath,
                                                                             QMimeDatabase::MatchContent));
                    if (mimeType.name() == "application/x-bittorrent") {
                        mData = file.readAll();
                        result = parseMap();
                        if (mError) {
                            mData.clear();
                            result.clear();
                            qDebug() << "error parsing file";
                            error = TorrentFileParser::ParsingError;
                        }
                    } else {
                        qDebug() << "wrong mime type" << mimeType.name();
                        error = TorrentFileParser::WrongMimeType;
                    }
                } else {
                    qDebug() << "error reading file";
                    error = TorrentFileParser::ReadingError;
                }
                emit done(error, mData, result);
            }

        private:
            QVariant parseVariant()
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

            QVariantMap parseMap()
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

            QVariantList parseList()
            {
                QVariantList list;
                ++mIndex;
                while (mData.at(mIndex) != 'e' && !mError) {
                    list.append(parseVariant());
                }
                ++mIndex;
                return list;
            }

            QString parseString()
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

            qint64 parseInt()
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

            QByteArray mData;
            int mIndex = 0;
            bool mError = false;
        signals:
            void done(tremotesf::TorrentFileParser::Error error,
                      const QByteArray& fileData,
                      const QVariantMap& parseResult);
        };
    }

    TorrentFileParser::TorrentFileParser(const QString& filePath, QObject* parent)
        : QObject(parent),
          mWorkerThread(new QThread(this)),
          mError(NoError),
          mLoaded(false)
    {
        setFilePath(filePath);
    }

    TorrentFileParser::~TorrentFileParser()
    {
        mWorkerThread->quit();
        mWorkerThread->wait();
    }

    const QString& TorrentFileParser::filePath() const
    {
        return mFilePath;
    }

    void TorrentFileParser::setFilePath(const QString& path)
    {
        if (path.isEmpty() || !mFilePath.isEmpty()) {
            return;
        }

        mFilePath = path;

        auto worker = new Worker();
        worker->moveToThread(mWorkerThread);
        QObject::connect(mWorkerThread, &QThread::finished, worker, &QObject::deleteLater);
        QObject::connect(this, &TorrentFileParser::requestParse, worker, &Worker::parse);
        QObject::connect(worker,
                         &Worker::done,
                         this,
                         [=](Error error, const QByteArray& fileData, const QVariantMap& parseResult) {
                             mLoaded = true;
                             mError = error;
                             mFileData = fileData;
                             mParseResult = parseResult;
                             emit done();

                             mWorkerThread->quit();
                         });
        mWorkerThread->start();
        emit requestParse(mFilePath);
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
}

#include "torrentfileparser.moc"
