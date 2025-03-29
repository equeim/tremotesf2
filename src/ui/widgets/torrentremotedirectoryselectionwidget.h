// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_TORRENTREMOTEDIRECTORYSELECTIONWIDGET_H
#define TREMOTESF_TORRENTREMOTEDIRECTORYSELECTIONWIDGET_H

#include "remotedirectoryselectionwidget.h"

class QComboBox;

namespace tremotesf {
    class Rpc;

    class TorrentDownloadDirectoryDirectorySelectionWidgetViewModel final
        : public RemoteDirectorySelectionWidgetViewModel {
        Q_OBJECT

    public:
        using RemoteDirectorySelectionWidgetViewModel::RemoteDirectorySelectionWidgetViewModel;

        struct ComboBoxItem {
            QString path{};
            QString displayPath{};
            [[nodiscard]] bool operator==(const ComboBoxItem&) const = default;
        };
        [[nodiscard]] std::vector<ComboBoxItem> initialComboBoxItems() const { return mInitialComboBoxItems; };

        void onComboBoxItemSelected(QString path, QString displayPath) {
            updatePathImpl(std::move(path), std::move(displayPath));
        }
        void saveDirectories(std::vector<ComboBoxItem> comboBoxItems);

    protected:
        void updatePathImpl(QString path, QString displayPath) override;

    private:
        [[nodiscard]] std::vector<ComboBoxItem> createInitialComboBoxItems() const;
        void updateInitialComboBoxItems();

        std::vector<ComboBoxItem> mInitialComboBoxItems{createInitialComboBoxItems()};

    signals:
        void initialComboBoxItemsChanged();
    };

    class TorrentDownloadDirectoryDirectorySelectionWidget final : public RemoteDirectorySelectionWidget {
        Q_OBJECT

    public:
        using RemoteDirectorySelectionWidget::RemoteDirectorySelectionWidget;

        void setup(QString path, const Rpc* rpc) override;

        void saveDirectories();

    protected:
        QWidget* createTextField() override;
        QLineEdit* lineEditFromTextField() override;
        RemoteDirectorySelectionWidgetViewModel* createViewModel(QString path, const Rpc* rpc) override {
            return new TorrentDownloadDirectoryDirectorySelectionWidgetViewModel(std::move(path), rpc, this);
        }
    };

    class ComboBoxDeleteKeyEventFilter final : public QObject {
        Q_OBJECT

    public:
        explicit ComboBoxDeleteKeyEventFilter(QComboBox* comboBox);

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;
    };
}

#endif // TREMOTESF_TORRENTREMOTEDIRECTORYSELECTIONWIDGET_H
