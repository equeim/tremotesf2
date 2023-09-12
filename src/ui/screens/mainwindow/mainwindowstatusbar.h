// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_MAINWINDOWSTATUSBAR_H
#define TREMOTESF_MAINWINDOWSTATUSBAR_H

#include <QStatusBar>

class QLabel;
class KSeparator;

namespace tremotesf {
    class Rpc;

    class MainWindowStatusBar final : public QStatusBar {
        Q_OBJECT

    public:
        explicit MainWindowStatusBar(const Rpc* rpc, QWidget* parent = nullptr);

    private:
        void updateLayout();
        void updateServerLabel();
        void updateStatusLabels();

        const Rpc* mRpc{};
        QLabel* mNoServersErrorImage{};
        QLabel* mServerLabel{};
        KSeparator* mFirstSeparator{};
        QLabel* mStatusLabel{};
        KSeparator* mSecondSeparator{};
        QLabel* mDownloadSpeedImage{};
        QLabel* mDownloadSpeedLabel{};
        KSeparator* mThirdSeparator{};
        QLabel* mUploadSpeedImage{};
        QLabel* mUploadSpeedLabel{};
    };
}

#endif // TREMOTESF_MAINWINDOWSTATUSBAR_H
