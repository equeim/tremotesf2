// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "remotedirectoryselectionwidget.h"

#include "libtremotesf/pathutils.h"
#include "libtremotesf/serversettings.h"
#include "libtremotesf/stdutils.h"
#include "libtremotesf/torrent.h"
#include "tremotesf/rpc/trpc.h"
#include "tremotesf/rpc/servers.h"
#include "tremotesf/ui/widgets/remotedirectoryselectionwidget.h"

#include <QCollator>
#include <QComboBox>
#include <QCoreApplication>
#include <QMessageBox>
#include <QPushButton>

namespace tremotesf {
    RemoteDirectorySelectionWidgetViewModel::RemoteDirectorySelectionWidgetViewModel(
        const QString& path, const Rpc* rpc, QObject* parent
    )
        : DirectorySelectionWidgetViewModel({}, {}, parent),
          mRpc(rpc),
          mMode(
              rpc->isLocal()
                  ? Mode::Local
                  : (Servers::instance()->currentServerHasMountedDirectories() ? Mode::RemoteMounted : Mode::Remote)
          ) {
        updatePath(path);
    }

    bool RemoteDirectorySelectionWidgetViewModel::enableFileDialog() const { return mMode != Mode::Remote; }

    QString RemoteDirectorySelectionWidgetViewModel::fileDialogDirectory() const {
        if (mMode == Mode::RemoteMounted) {
            return Servers::instance()->fromRemoteToLocalDirectory(mPath, mRpc->serverSettings());
        }
        return mPath;
    }

    void RemoteDirectorySelectionWidgetViewModel::onFileDialogAccepted(const QString& path) {
        if (mMode != Mode::RemoteMounted) {
            DirectorySelectionWidgetViewModel::onFileDialogAccepted(path);
            return;
        }
        const QString remoteDirectory = Servers::instance()->fromLocalToRemoteDirectory(path, mRpc->serverSettings());
        if (remoteDirectory.isEmpty()) {
            emit showMountedDirectoryError();
        } else {
            DirectorySelectionWidgetViewModel::onFileDialogAccepted(remoteDirectory);
        }
    }

    QString RemoteDirectorySelectionWidgetViewModel::normalizePath(const QString& path) const {
        return tremotesf::normalizePath(path, mRpc->serverSettings()->data().pathOs);
    }

    QString RemoteDirectorySelectionWidgetViewModel::toNativeSeparators(const QString& path) const {
        return tremotesf::toNativeSeparators(path, mRpc->serverSettings()->data().pathOs);
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
            [=, this] {
                QMessageBox::warning(
                    this,
                    //: Dialog title
                    qApp->translate("tremotesf", "Error"),
                    qApp->translate("tremotesf", "Selected directory should be inside mounted directory")
                );
            }
        );
    }

    std::vector<DirectorySelectionWidgetViewModel::ComboBoxItem>
    TorrentDownloadDirectoryDirectorySelectionWidgetViewModel::createComboBoxItems() const {
        QStringList directories = Servers::instance()->currentServerLastDownloadDirectories(mRpc->serverSettings());
        directories.reserve(directories.size() + static_cast<QStringList::size_type>(mRpc->torrents().size()) + 2);
        for (const auto& torrent : mRpc->torrents()) {
            directories.push_back(torrent->data().downloadDirectory);
        }
        if (!mPath.isEmpty()) {
            directories.push_back(mPath);
        }
        directories.push_back(mRpc->serverSettings()->data().downloadDirectory);

        directories.removeDuplicates();

        QCollator collator{};
        collator.setCaseSensitivity(Qt::CaseInsensitive);
        collator.setNumericMode(true);
        std::sort(directories.begin(), directories.end(), [&collator](const auto& first, const auto& second) {
            return collator.compare(first, second) < 0;
        });

        return createTransforming<std::vector<DirectorySelectionWidgetViewModel::ComboBoxItem>>(
            std::move(directories),
            [=, this](QString&& dir) {
                QString display = toNativeSeparators(dir);
                return DirectorySelectionWidgetViewModel::ComboBoxItem{std::move(dir), std::move(display)};
            }
        );
    }

    TorrentDownloadDirectoryDirectorySelectionWidgetViewModel::
        TorrentDownloadDirectoryDirectorySelectionWidgetViewModel(const QString& path, const Rpc* rpc, QObject* parent)
        : RemoteDirectorySelectionWidgetViewModel(path, rpc, parent) {}

    void TorrentDownloadDirectoryDirectorySelectionWidgetViewModel::saveDirectories() {
        auto paths = createTransforming<QStringList>(mComboBoxItems, [](const auto& item) { return item.path; });
        if (!paths.contains(mPath)) {
            paths.push_back(mPath);
        }
        auto servers = Servers::instance();
        servers->setCurrentServerLastDownloadDirectories(paths);
        servers->setCurrentServerLastDownloadDirectory(mPath);
    }

    void TorrentDownloadDirectoryDirectorySelectionWidgetViewModel::updateComboBoxItems() {
        auto items = createComboBoxItems();
        if (items != mComboBoxItems) {
            mComboBoxItems = std::move(items);
            emit comboBoxItemsChanged();
        }
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
