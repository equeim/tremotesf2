// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_MAINWINDOWSTATUSBAR_H
#define TREMOTESF_MAINWINDOWSTATUSBAR_H

#include <QStatusBar>

class QLabel;

namespace tremotesf {
    class Rpc;

    class StatusBarSeparator;

    class MainWindowStatusBar final : public QStatusBar {
        Q_OBJECT

    public:
        explicit MainWindowStatusBar(const Rpc* rpc, QWidget* parent = nullptr);

    private:
        void updateLayout();
        void updateServerLabel();
        void updateStatusLabels();
        void showContextMenu(QPoint pos);

        const Rpc* mRpc{};
        QLabel* mNoServersErrorImage{};
        QLabel* mServerLabel{};
        StatusBarSeparator* mFirstSeparator{};
        QLabel* mStatusLabel{};
        StatusBarSeparator* mSecondSeparator{};
        QLabel* mDownloadSpeedImage{};
        QLabel* mDownloadSpeedLabel{};
        StatusBarSeparator* mThirdSeparator{};
        QLabel* mUploadSpeedImage{};
        QLabel* mUploadSpeedLabel{};

    signals:
        void showConnectionSettingsDialog();
    };

    class StatusBarSeparator final : public QWidget {
        Q_OBJECT
    public:
        explicit StatusBarSeparator(QWidget* parent = nullptr);
        QSize sizeHint() const override;

    protected:
        void paintEvent(QPaintEvent* event) override;
    };
}

#endif // TREMOTESF_MAINWINDOWSTATUSBAR_H
