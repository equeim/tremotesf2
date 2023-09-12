// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_TORRENTREMOTEDIRECTORYSELECTIONWIDGET_H
#define TREMOTESF_TORRENTREMOTEDIRECTORYSELECTIONWIDGET_H

#include "remotedirectoryselectionwidget.h"

namespace tremotesf {
    class Rpc;

    class TorrentDownloadDirectoryDirectorySelectionWidgetViewModel final : public RemoteDirectorySelectionWidgetViewModel {
        Q_OBJECT

    public:
        using RemoteDirectorySelectionWidgetViewModel::RemoteDirectorySelectionWidgetViewModel;

        struct ComboBoxItem {
            QString path{};
            QString displayPath{};
            [[nodiscard]] bool operator==(const ComboBoxItem&) const = default;
        };
        [[nodiscard]] std::vector<ComboBoxItem> comboBoxItems() const { return mComboBoxItems; };

        void onComboBoxItemSelected(QString path, QString displayPath) {
            updatePathImpl(std::move(path), std::move(displayPath));
        }
        void saveDirectories();

    protected:
        void updatePathImpl(QString path, QString displayPath) override {
            RemoteDirectorySelectionWidgetViewModel::updatePathImpl(std::move(path), std::move(displayPath));
            updateComboBoxItems();
        }

    private:
        [[nodiscard]] std::vector<ComboBoxItem> createComboBoxItems() const;
        void updateComboBoxItems();

        std::vector<ComboBoxItem> mComboBoxItems{createComboBoxItems()};

    signals:
        void comboBoxItemsChanged();
    };

    class TorrentDownloadDirectoryDirectorySelectionWidget final : public RemoteDirectorySelectionWidget {
        Q_OBJECT

    public:
        using RemoteDirectorySelectionWidget::RemoteDirectorySelectionWidget;

        void setup(QString path, const Rpc* rpc) override;

        void saveDirectories() {
            static_cast<TorrentDownloadDirectoryDirectorySelectionWidgetViewModel*>(mViewModel)->saveDirectories();
        }

    protected:
        QWidget* createTextField() override;
        QLineEdit* lineEditFromTextField() override;
        RemoteDirectorySelectionWidgetViewModel* createViewModel(QString path, const Rpc* rpc) override {
            return new TorrentDownloadDirectoryDirectorySelectionWidgetViewModel(std::move(path), rpc, this);
        }
    };
}

#endif // TREMOTESF_TORRENTREMOTEDIRECTORYSELECTIONWIDGET_H
