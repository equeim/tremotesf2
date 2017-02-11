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

#ifndef TREMOTESF_DIRECTORYCONTENTMODEL_H
#define TREMOTESF_DIRECTORYCONTENTMODEL_H

#include <QAbstractListModel>
#include <QQmlParserStatus>

namespace tremotesf
{
    struct File
    {
        QString path;
        QString name;
        bool directory;
    };

    class DirectoryContentModel : public QAbstractListModel, public QQmlParserStatus
    {
        Q_OBJECT
        Q_INTERFACES(QQmlParserStatus)
        Q_PROPERTY(QString directory READ directory WRITE setDirectory NOTIFY directoryChanged)
        Q_PROPERTY(QString parentDirectory READ parentDirectory NOTIFY directoryChanged)
        Q_PROPERTY(bool showFiles READ showFiles WRITE setShowFiles)
        Q_PROPERTY(QStringList nameFilters READ nameFilters WRITE setNameFilters)
    public:
        DirectoryContentModel();
        void classBegin() override;
        void componentComplete() override;

        QVariant data(const QModelIndex& index, int role) const override;
        int rowCount(const QModelIndex&) const;

        const QString& directory() const;
        void setDirectory(const QString& directory);

        const QString& parentDirectory() const;

        bool showFiles() const;
        void setShowFiles(bool show);

        const QStringList& nameFilters() const;
        void setNameFilters(const QStringList& filters);

    protected:
        QHash<int, QByteArray> roleNames() const override;

    private:
        void loadDirectory();

        bool mComponentCompleted;
        QList<File> mFiles;
        QString mDirectory;
        QString mParentDirectory;
        bool mShowFiles;
        QStringList mNameFilters;
    signals:
        void directoryChanged();
    };
}

#endif // TREMOTESF_DIRECTORYCONTENTMODEL_H
