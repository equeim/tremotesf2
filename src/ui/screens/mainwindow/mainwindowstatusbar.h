// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_MAINWINDOWSTATUSBAR_H
#define TREMOTESF_MAINWINDOWSTATUSBAR_H

#include <QStatusBar>

class QLabel;
class QMenu;
class QToolButton;

namespace tremotesf {
    class Rpc;

    class StatusBarSeparator;

    class MainWindowStatusBar final : public QStatusBar {
        Q_OBJECT

    public:
        explicit MainWindowStatusBar(const Rpc* rpc, QWidget* parent = nullptr);

    private:
        void updateLayout();
        void updateServerText();
        void updateServerMenu();
        void updateStatusLabels();

        const Rpc* mRpc{};
        QToolButton* mServerButton{};
        QMenu* mServerMenu{};
        QWidget* mServerSpacer{};
        StatusBarSeparator* mFirstSeparator{};
        QLabel* mStatusLabel{};
        StatusBarSeparator* mSecondSeparator{};
        QLabel* mDownloadSpeedImage{};
        QLabel* mDownloadSpeedLabel{};
        StatusBarSeparator* mThirdSeparator{};
        QLabel* mUploadSpeedImage{};
        QLabel* mUploadSpeedLabel{};
        StatusBarSeparator* mFourthSeparator{};
        QLabel* mFreeSpaceLabel{};

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
