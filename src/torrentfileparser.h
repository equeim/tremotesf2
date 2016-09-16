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

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

namespace tremotesf
{
    class TorrentFileParser : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(QString filePath READ filePath WRITE setFilePath)
        Q_PROPERTY(bool error READ error NOTIFY done)
        Q_PROPERTY(QVariantMap result READ result NOTIFY done)
    public:
        explicit TorrentFileParser(const QString& filePath = QString(), QObject* parent = nullptr);

        const QString& filePath() const;
        void setFilePath(const QString& path);

        bool error() const;

        const QVariantMap& result() const;
    private:
        QVariant parseVariant();
        QVariantMap parseMap();
        QVariantList parseList();
        QString parseString();
        qint64 parseInt();

        QString mFilePath;
        QByteArray mData;
        int mIndex;
        bool mError;
        QVariantMap mResult;
    signals:
        void done();
    };
}
