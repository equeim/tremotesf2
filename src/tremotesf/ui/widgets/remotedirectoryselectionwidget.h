// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_REMOTEDIRECTORYSELECTIONWIDGET_H
#define TREMOTESF_REMOTEDIRECTORYSELECTIONWIDGET_H

#include "directoryselectionwidget.h"

namespace tremotesf {
    class Rpc;

    class RemoteDirectorySelectionWidgetViewModel : public DirectorySelectionWidgetViewModel {
        Q_OBJECT
    public:
        explicit RemoteDirectorySelectionWidgetViewModel(
            const QString& path, const Rpc* rpc, QObject* parent = nullptr
        );

        [[nodiscard]] QString fileDialogDirectory() const override;
        void onFileDialogAccepted(const QString& path) override;

    signals:
        void showMountedDirectoryError();

    protected:
        [[nodiscard]] QString normalizeToInternalPath(const QString& path) const override;
        [[nodiscard]] QString convertToDisplayPath(const QString& path) const override;

        const Rpc* mRpc{};

        enum class Mode { Local, RemoteMounted, Remote };
        Mode mMode{};
    };

    class RemoteDirectorySelectionWidget : public DirectorySelectionWidget {
        Q_OBJECT

    public:
        explicit RemoteDirectorySelectionWidget(const QString& path, const Rpc* rpc, QWidget* parent = nullptr);

        void updatePath(const QString& path) {
            static_cast<RemoteDirectorySelectionWidgetViewModel*>(mViewModel)->updatePath(path);
        }

    protected:
        explicit RemoteDirectorySelectionWidget(
            RemoteDirectorySelectionWidgetViewModel* viewModel, QWidget* parent = nullptr
        );
    };

    class TorrentDownloadDirectoryDirectorySelectionWidgetViewModel : public RemoteDirectorySelectionWidgetViewModel {
        Q_OBJECT
    public:
        explicit TorrentDownloadDirectoryDirectorySelectionWidgetViewModel(
            const QString& path, const Rpc* rpc, QObject* parent = nullptr
        );

        [[nodiscard]] bool useComboBox() const override { return true; }
        [[nodiscard]] std::vector<ComboBoxItem> comboBoxItems() const override { return mComboBoxItems; };

        void saveDirectories();
        void updateComboBoxItems();

    private:
        [[nodiscard]] static QString initialPath(const QString& torrentDownloadDirectory);
        [[nodiscard]] std::vector<ComboBoxItem> createComboBoxItems() const;

        std::vector<ComboBoxItem> mComboBoxItems{createComboBoxItems()};
    };

    class TorrentDownloadDirectoryDirectorySelectionWidget : public RemoteDirectorySelectionWidget {
        Q_OBJECT

    public:
        TorrentDownloadDirectoryDirectorySelectionWidget(
            const QString& path, const Rpc* rpc, QWidget* parent = nullptr
        );

        void update(const QString& path);

        void saveDirectories() {
            static_cast<TorrentDownloadDirectoryDirectorySelectionWidgetViewModel*>(mViewModel)->saveDirectories();
        }
    };
}

#endif // TREMOTESF_REMOTEDIRECTORYSELECTIONWIDGET_H
