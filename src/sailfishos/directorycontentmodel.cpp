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

#include "directorycontentmodel.h"

#include <QDir>
#include <QStandardPaths>

namespace tremotesf
{
    namespace
    {
        enum Role
        {
            PathRole,
            NameRole,
            DirectoryRole
        };
    }

    DirectoryContentModel::DirectoryContentModel()
        : mComponentCompleted(false),
          mDirectory(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)),
          mParentDirectory(QStringLiteral("/home")),
          mShowFiles(true)
    {
    }

    void DirectoryContentModel::classBegin()
    {
    }

    void DirectoryContentModel::componentComplete()
    {
        mComponentCompleted = true;
        loadDirectory();
    }

    QVariant DirectoryContentModel::data(const QModelIndex& index, int role) const
    {
        const File& file = mFiles.at(index.row());
        switch (role) {
        case PathRole:
            return file.path;
        case NameRole:
            return file.name;
        case DirectoryRole:
            return file.directory;
        }
        return QVariant();
    }

    int DirectoryContentModel::rowCount(const QModelIndex&) const
    {
        return mFiles.size();
    }

    const QString& DirectoryContentModel::directory() const
    {
        return mDirectory;
    }

    void DirectoryContentModel::setDirectory(const QString& directory)
    {
        if (directory.isEmpty() || (directory == mDirectory)) {
            return;
        }

        mDirectory = directory;

        QFileInfo info(mDirectory);
        if (info.isFile()) {
            mDirectory = info.path();
        }

        QDir dir(mDirectory);
        dir.cdUp();
        mParentDirectory = dir.path();

        emit directoryChanged();

        if (mComponentCompleted) {
            loadDirectory();
        }
    }

    const QString& DirectoryContentModel::parentDirectory() const
    {
        return mParentDirectory;
    }

    bool DirectoryContentModel::showFiles() const
    {
        return mShowFiles;
    }

    void DirectoryContentModel::setShowFiles(bool show)
    {
        mShowFiles = show;
    }

    const QStringList& DirectoryContentModel::nameFilters() const
    {
        return mNameFilters;
    }

    void DirectoryContentModel::setNameFilters(const QStringList& filters)
    {
        mNameFilters = filters;
    }

    QHash<int, QByteArray> DirectoryContentModel::roleNames() const
    {
        return {{PathRole, QByteArrayLiteral("path")},
                {NameRole, QByteArrayLiteral("name")},
                {DirectoryRole, QByteArrayLiteral("directory")}};
    }

    void DirectoryContentModel::loadDirectory()
    {
        beginRemoveRows(QModelIndex(), 0, mFiles.size() - 1);
        mFiles.clear();
        endRemoveRows();

        QDir::Filters filters = QDir::AllDirs | QDir::NoDotAndDotDot;
        if (mShowFiles) {
            filters |= QDir::Files;
        }

        const QFileInfoList files(QDir(mDirectory).entryInfoList(mNameFilters, filters, QDir::DirsFirst));

        beginInsertRows(QModelIndex(), 0, files.size() - 1);

        for (const QFileInfo& info : files) {
            mFiles.append({info.filePath(),
                           info.fileName(),
                           info.isDir()});
        }

        endInsertRows();
    }
}
