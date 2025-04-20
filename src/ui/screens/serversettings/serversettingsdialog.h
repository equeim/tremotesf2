// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_SERVERSETTINGSDIALOG_H
#define TREMOTESF_SERVERSETTINGSDIALOG_H

#include <QDialog>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QGroupBox;
class QSpinBox;
class QTimeEdit;

class KMessageWidget;

namespace tremotesf {
    class RemoteDirectorySelectionWidget;
    class Rpc;

    class ServerSettingsDialog final : public QDialog {
        Q_OBJECT

    public:
        explicit ServerSettingsDialog(const Rpc* rpc, QWidget* parent = nullptr);
        void accept() override;

    private:
        void setupUi();
        void loadSettings();

        const Rpc* mRpc;

        KMessageWidget* mDisconnectedMessageWidget = nullptr;

        QWidget* mDownloadingPageWidget = nullptr;
        RemoteDirectorySelectionWidget* mDownloadDirectoryWidget = nullptr;
        QCheckBox* mStartAddedTorrentsCheckBox = nullptr;
        QCheckBox* mIncompleteFilesCheckBox = nullptr;
        QCheckBox* mIncompleteDirectoryCheckBox = nullptr;
        RemoteDirectorySelectionWidget* mIncompleteDirectoryWidget = nullptr;

        QWidget* mSeedingPageWidget = nullptr;
        QCheckBox* mRatioLimitCheckBox = nullptr;
        QDoubleSpinBox* mRatioLimitSpinBox = nullptr;
        QCheckBox* mIdleSeedingLimitCheckBox = nullptr;
        QSpinBox* mIdleSeedingLimitSpinBox = nullptr;

        QWidget* mQueuePageWidget = nullptr;
        QCheckBox* mMaximumActiveDownloadsCheckBox = nullptr;
        QSpinBox* mMaximumActiveDownloadsSpinBox = nullptr;
        QCheckBox* mMaximumActiveUploadsCheckBox = nullptr;
        QSpinBox* mMaximumActiveUploadsSpinBox = nullptr;
        QCheckBox* mIdleQueueLimitCheckBox = nullptr;
        QSpinBox* mIdleQueueLimitSpinBox = nullptr;

        QWidget* mSpeedPageWidget = nullptr;
        QCheckBox* mDownloadSpeedLimitCheckBox = nullptr;
        QSpinBox* mDownloadSpeedLimitSpinBox = nullptr;
        QCheckBox* mUploadSpeedLimitCheckBox = nullptr;
        QSpinBox* mUploadSpeedLimitSpinBox = nullptr;
        QGroupBox* mEnableAlternativeSpeedLimitsGroupBox = nullptr;
        QSpinBox* mAlternativeDownloadSpeedLimitSpinBox = nullptr;
        QSpinBox* mAlternativeUploadSpeedLimitSpinBox = nullptr;
        QGroupBox* mLimitScheduleGroupBox = nullptr;
        QTimeEdit* mLimitScheduleBeginTimeEdit = nullptr;
        QTimeEdit* mLimitScheduleEndTimeEdit = nullptr;
        QComboBox* mLimitScheduleDaysComboBox = nullptr;

        QWidget* mNetworkPageWidget = nullptr;
        QSpinBox* mPeerPortSpinBox = nullptr;
        QCheckBox* mRandomPortCheckBox = nullptr;
        QCheckBox* mPortForwardingCheckBox = nullptr;
        QComboBox* mEncryptionComboBox = nullptr;
        QCheckBox* mUtpCheckBox = nullptr;
        QCheckBox* mPexCheckBox = nullptr;
        QCheckBox* mDhtCheckBox = nullptr;
        QCheckBox* mLpdCheckBox = nullptr;
        QSpinBox* mTorrentPeerLimitSpinBox = nullptr;
        QSpinBox* mGlobalPeerLimitSpinBox = nullptr;
    };
}

#endif // TREMOTESF_SERVERSETTINGSDIALOG_H
