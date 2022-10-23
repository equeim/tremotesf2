// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "remotedirectoryselectionwidget.h"

#include "libtremotesf/pathutils.h"
#include "libtremotesf/serversettings.h"
#include "libtremotesf/torrent.h"
#include "tremotesf/rpc/trpc.h"
#include "tremotesf/rpc/servers.h"
#include "tremotesf/settings.h"

#include <QCollator>
#include <QComboBox>
#include <QCoreApplication>
#include <QMessageBox>
#include <QPushButton>

namespace tremotesf {
    namespace {
        inline QString chopTrailingSeparator(QString directory) {
            if (directory.endsWith('/')) {
                directory.chop(1);
            }
            return directory;
        }
    }

    RemoteDirectorySelectionWidgetViewModel::RemoteDirectorySelectionWidgetViewModel(
        const QString& path, const Rpc* rpc, QObject* parent
    )
        : DirectorySelectionWidgetViewModel(path, toNativeRemoteSeparators(path, rpc), parent),
          mRpc(rpc),
          mMode(
              rpc->isLocal()
                  ? Mode::Local
                  : (Servers::instance()->currentServerHasMountedDirectories() ? Mode::RemoteMounted : Mode::Remote)
          ) {}

    QString RemoteDirectorySelectionWidgetViewModel::fileDialogDirectory() const {
        if (mMode == Mode::RemoteMounted) {
            return Servers::instance()->fromRemoteToLocalDirectory(mPath);
        }
        return mPath;
    }

    void RemoteDirectorySelectionWidgetViewModel::onFileDialogAccepted(const QString& path) {
        if (mMode != Mode::RemoteMounted) {
            DirectorySelectionWidgetViewModel::onFileDialogAccepted(path);
            return;
        }
        const QString remoteDirectory = Servers::instance()->fromLocalToRemoteDirectory(path);
        if (remoteDirectory.isEmpty()) {
            emit showMountedDirectoryError();
        } else {
            DirectorySelectionWidgetViewModel::onFileDialogAccepted(remoteDirectory);
        }
    }

    QString RemoteDirectorySelectionWidgetViewModel::normalizeToInternalPath(const QString& path) const {
        return normalizeRemotePath(path, mRpc);
    }

    QString RemoteDirectorySelectionWidgetViewModel::convertToDisplayPath(const QString& path) const {
        return toNativeRemoteSeparators(path, mRpc);
    }

    RemoteDirectorySelectionWidget::RemoteDirectorySelectionWidget(const QString& path, const Rpc* rpc, QWidget* parent)
        : RemoteDirectorySelectionWidget(new RemoteDirectorySelectionWidgetViewModel(path, rpc), parent) {}

    RemoteDirectorySelectionWidget::RemoteDirectorySelectionWidget(
        RemoteDirectorySelectionWidgetViewModel* viewModel, QWidget* parent
    )
        : DirectorySelectionWidget(viewModel, parent) {
        QObject::connect(
            static_cast<RemoteDirectorySelectionWidgetViewModel*>(mViewModel),
            &RemoteDirectorySelectionWidgetViewModel::showMountedDirectoryError,
            this,
            [=] {
                QMessageBox::warning(
                    this,
                    qApp->translate("tremotesf", "Error"),
                    qApp->translate("tremotesf", "Selected directory should be inside mounted directory")
                );
            }
        );
    }

    std::vector<DirectorySelectionWidgetViewModel::ComboBoxItem>
    TorrentDownloadDirectoryDirectorySelectionWidgetViewModel::createComboBoxItems() const {
        QStringList directories{};
        {
            const auto saved = Servers::instance()->currentServerAddTorrentDialogDirectories();
            directories.reserve(saved.size() + static_cast<QStringList::size_type>(mRpc->torrents().size()) + 1);
            for (const auto& directory : saved) {
                directories.push_back(chopTrailingSeparator(directory));
            }
        }
        for (const auto& torrent : mRpc->torrents()) {
            directories.push_back(chopTrailingSeparator(torrent->downloadDirectory()));
        }
        directories.push_back(chopTrailingSeparator(mPath));
        directories.push_back(chopTrailingSeparator(mRpc->serverSettings()->downloadDirectory()));

        directories.removeDuplicates();

        QCollator collator{};
        collator.setCaseSensitivity(Qt::CaseInsensitive);
        collator.setNumericMode(true);
        std::sort(directories.begin(), directories.end(), [&collator](const auto& first, const auto& second) {
            return collator.compare(first, second) < 0;
        });

        std::vector<DirectorySelectionWidgetViewModel::ComboBoxItem> items{};
        items.reserve(static_cast<size_t>(directories.size()));
        std::transform(directories.begin(), directories.end(), std::back_inserter(items), [=](const auto& dir) {
            return DirectorySelectionWidgetViewModel::ComboBoxItem{dir, convertToDisplayPath(dir)};
        });
        return items;
    }

    TorrentDownloadDirectoryDirectorySelectionWidgetViewModel::
        TorrentDownloadDirectoryDirectorySelectionWidgetViewModel(const QString& path, const Rpc* rpc, QObject* parent)
        : RemoteDirectorySelectionWidgetViewModel(path, rpc, parent) {}

    void TorrentDownloadDirectoryDirectorySelectionWidgetViewModel::saveDirectories() {
        QStringList paths{};
        paths.reserve(static_cast<QStringList::size_type>(mComboBoxItems.size() + 1));
        std::transform(mComboBoxItems.begin(), mComboBoxItems.end(), std::back_inserter(paths), [](const auto& item) {
            return item.path;
        });
        if (!paths.contains(mPath)) {
            paths.push_back(mPath);
        }
        Servers::instance()->setCurrentServerAddTorrentDialogDirectories(paths);
        Settings::instance()->setLastDownloadDirectory(mPath);
    }

    void TorrentDownloadDirectoryDirectorySelectionWidgetViewModel::updateComboBoxItems() {
        auto items = createComboBoxItems();
        if (items != mComboBoxItems) {
            mComboBoxItems = std::move(items);
            emit comboBoxItemsChanged();
        }
    }

    QString
    TorrentDownloadDirectoryDirectorySelectionWidgetViewModel::initialPath(const QString& torrentDownloadDirectory) {
        if (Settings::instance()->rememberDownloadDir()) {
            auto lastDir = Settings::instance()->lastDownloadDirectory();
            if (!lastDir.isEmpty()) return lastDir;
        }
        return torrentDownloadDirectory;
    }

    TorrentDownloadDirectoryDirectorySelectionWidget::TorrentDownloadDirectoryDirectorySelectionWidget(
        const QString& path, const Rpc* rpc, QWidget* parent
    )
        : RemoteDirectorySelectionWidget(
              new TorrentDownloadDirectoryDirectorySelectionWidgetViewModel(path, rpc), parent
          ) {}

    void TorrentDownloadDirectoryDirectorySelectionWidget::update(const QString& path) {
        auto model = static_cast<TorrentDownloadDirectoryDirectorySelectionWidgetViewModel*>(mViewModel);
        model->updateComboBoxItems();
        model->updatePath(path);
    }
}
