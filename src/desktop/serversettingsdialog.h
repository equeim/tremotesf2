/*
 * Tremotesf
 * Copyright (C) 2015-2016 Alexey Rochev <equeim@gmail.com>
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

        KMessageWidget* mDisconnectedMessageWidget;

        QWidget* mDownloadingPageWidget;
        FileSelectionWidget* mDownloadDirectoryWidget;
        QCheckBox* mStartAddedTorrentsCheckBox;
        //QCheckBox* mTrashTorrentFilesCheckBox;
        QCheckBox* mIncompleteFilesCheckBox;
        QCheckBox* mIncompleteDirectoryCheckBox;
        FileSelectionWidget* mIncompleteDirectoryWidget;

        QWidget* mSeedingPageWidget;
        QCheckBox* mRatioLimitCheckBox;
        QDoubleSpinBox* mRatioLimitSpinBox;
        QCheckBox* mIdleSeedingLimitCheckBox;
        QSpinBox* mIdleSeedingLimitSpinBox;

        QWidget* mQueuePageWidget;
        QCheckBox* mMaximumActiveDownloadsCheckBox;
        QSpinBox* mMaximumActiveDownloadsSpinBox;
        QCheckBox* mMaximumActiveUploadsCheckBox;
        QSpinBox* mMaximumActiveUploadsSpinBox;
        QCheckBox* mIdleQueueLimitCheckBox;
        QSpinBox* mIdleQueueLimitSpinBox;

        QWidget* mSpeedPageWidget;
        QCheckBox* mDownloadSpeedLimitCheckBox;
        QSpinBox* mDownloadSpeedLimitSpinBox;
        QCheckBox* mUploadSpeedLimitCheckBox;
        QSpinBox* mUploadSpeedLimitSpinBox;
        QGroupBox* mEnableAlternativeSpeedLimitsGroupBox;
        QSpinBox* mAlternativeDownloadSpeedLimitSpinBox;
        QSpinBox* mAlternativeUploadSpeedLimitSpinBox;
        QGroupBox* mLimitScheduleGroupBox;
        QTimeEdit* mLimitScheduleBeginTimeEdit;
        QTimeEdit* mLimitScheduleEndTimeEdit;
        QComboBox* mLimitScheduleDaysComboBox;

        QWidget* mNetworkPageWidget;
        QSpinBox* mPeerPortSpinBox;
        QCheckBox* mRandomPortCheckBox;
        QCheckBox* mPortForwardingCheckBox;
        QComboBox* mEncryptionComboBox;
        QCheckBox* mUtpCheckBox;
        QCheckBox* mPexCheckBox;
        QCheckBox* mDhtCheckBox;
        QCheckBox* mLpdCheckBox;
        QSpinBox* mTorrentPeerLimitSpinBox;
        QSpinBox* mGlobalPeerLimitSpinBox;
    };
}

#endif // TREMOTESF_SERVERSETTINGSDIALOG_H
