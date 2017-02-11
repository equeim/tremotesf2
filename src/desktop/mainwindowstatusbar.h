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

#ifndef TREMOTESF_MAINWINDOWSTATUSBAR_H
#define TREMOTESF_MAINWINDOWSTATUSBAR_H

#include <QStatusBar>

class QLabel;
class KSeparator;

namespace tremotesf
{
    class Rpc;

    class MainWindowStatusBar : public QStatusBar
    {
    public:
        explicit MainWindowStatusBar(const Rpc* rpc, QWidget* parent = nullptr);

    private:
        void updateLayout();
        void updateAccountLabel();
        void updateStatusLabels();

    private:
        const Rpc* mRpc;

        QLabel* mNoAccountsErrorImage;
        QLabel* mAccountLabel;
        KSeparator* mFirstSeparator;
        QLabel* mStatusLabel;
        KSeparator* mSecondSeparator;
        QLabel* mDownloadSpeedImage;
        QLabel* mDownloadSpeedLabel;
        KSeparator* mThirdSeparator;
        QLabel* mUploadSpeedImage;
        QLabel* mUploadSpeedLabel;
    };
}

#endif // TREMOTESF_MAINWINDOWSTATUSBAR_H
