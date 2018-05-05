/*
 * Tremotesf
 * Copyright (C) 2015-2017 Alexey Rochev <equeim@gmail.com>
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

namespace tremotesf
{
    class FileSelectionWidget;
    class Rpc;

    class ServerSettingsDialog : public QDialog
    {
    public:
        explicit ServerSettingsDialog(const Rpc* rpc, QWidget* parent = nullptr);
        QSize sizeHint() const override;
        void accept() override;

    private:
        void setupUi();
        void loadSettings();

    private:
        const Rpc* mRpc;

        KMessageWidget* mDisconnectedMessageWidget = nullptr;

        QWidget* mDownloadingPageWidget = nullptr;
        FileSelectionWidget* mDownloadDirectoryWidget = nullptr;
        QCheckBox* mStartAddedTorrentsCheckBox = nullptr;
        //QCheckBox* mTrashTorrentFilesCheckBox = nullptr;
        QCheckBox* mIncompleteFilesCheckBox = nullptr;
        QCheckBox* mIncompleteDirectoryCheckBox = nullptr;
        FileSelectionWidget* mIncompleteDirectoryWidget = nullptr;

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
