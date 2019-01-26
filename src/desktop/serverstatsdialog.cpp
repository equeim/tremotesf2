/*
 * Tremotesf
 * Copyright (C) 2015-2018 Alexey Rochev <equeim@gmail.com>
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

#include "serverstatsdialog.h"

#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include <KColumnResizer>
#include <KMessageWidget>

#include "libtremotesf/serverstats.h"
#include "../trpc.h"
#include "../utils.h"

namespace tremotesf
{
    ServerStatsDialog::ServerStatsDialog(Rpc* rpc, QWidget* parent)
        : QDialog(parent)
    {
        setWindowTitle(qApp->translate("tremotesf", "Server Stats"));

        auto layout = new QVBoxLayout(this);

        auto disconnectedWidget = new KMessageWidget(qApp->translate("tremotesf", "Disconnected"));
        disconnectedWidget->setCloseButtonVisible(false);
        disconnectedWidget->setMessageType(KMessageWidget::Warning);
        disconnectedWidget->hide();
        layout->addWidget(disconnectedWidget);

        auto currentSessionGroupBox = new QGroupBox(qApp->translate("tremotesf", "Current session"), this);
        auto currentSessionGroupBoxLayout = new QFormLayout(currentSessionGroupBox);

        auto sessionDownloadedLabel = new QLabel(this);
        currentSessionGroupBoxLayout->addRow(qApp->translate("tremotesf", "Downloaded:"), sessionDownloadedLabel);
        auto sessionUploadedLabel = new QLabel(this);
        currentSessionGroupBoxLayout->addRow(qApp->translate("tremotesf", "Uploaded:"), sessionUploadedLabel);
        auto sessionRatioLabel = new QLabel(this);
        currentSessionGroupBoxLayout->addRow(qApp->translate("tremotesf", "Ratio:"), sessionRatioLabel);
        auto sessionDurationLabel = new QLabel(this);
        currentSessionGroupBoxLayout->addRow(qApp->translate("tremotesf", "Duration:"), sessionDurationLabel);

        layout->addWidget(currentSessionGroupBox);

        auto totalGroupBox = new QGroupBox(qApp->translate("tremotesf", "Total"), this);
        auto totalGroupBoxLayout = new QFormLayout(totalGroupBox);

        auto totalDownloadedLabel = new QLabel(this);
        totalGroupBoxLayout->addRow(qApp->translate("tremotesf", "Downloaded:"), totalDownloadedLabel);
        auto totalUploadedLabel = new QLabel(this);
        totalGroupBoxLayout->addRow(qApp->translate("tremotesf", "Uploaded:"), totalUploadedLabel);
        auto totalRatioLabel = new QLabel(this);
        totalGroupBoxLayout->addRow(qApp->translate("tremotesf", "Ratio:"), totalRatioLabel);
        auto totalDurationLabel = new QLabel(this);
        totalGroupBoxLayout->addRow(qApp->translate("tremotesf", "Duration:"), totalDurationLabel);
        auto sessionCountLabel = new QLabel(this);
        totalGroupBoxLayout->addRow(qApp->translate("tremotesf", "Started:"), sessionCountLabel);

        layout->addWidget(totalGroupBox);

        layout->addStretch();

        auto resizer = new KColumnResizer(this);
        resizer->addWidgetsFromLayout(currentSessionGroupBoxLayout);
        resizer->addWidgetsFromLayout(totalGroupBoxLayout);

        auto dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
        dialogButtonBox->button(QDialogButtonBox::Close)->setDefault(true);
        QObject::connect(dialogButtonBox, &QDialogButtonBox::rejected, this, &ServerStatsDialog::reject);
        layout->addWidget(dialogButtonBox);

        setMinimumSize(minimumSizeHint());

        QObject::connect(rpc, &Rpc::connectedChanged, this, [=]() {
            if (rpc->isConnected()) {
                disconnectedWidget->animatedHide();
            } else {
                disconnectedWidget->animatedShow();
            }
            currentSessionGroupBox->setEnabled(rpc->isConnected());
            totalGroupBox->setEnabled(rpc->isConnected());
            sessionCountLabel->setEnabled(rpc->isConnected());
        });

        const libtremotesf::SessionStats* currentSessionStats = rpc->serverStats()->currentSession();
        const libtremotesf::SessionStats* totalStats = rpc->serverStats()->total();
        auto update = [=]() {
            sessionDownloadedLabel->setText(Utils::formatByteSize(currentSessionStats->downloaded()));
            sessionUploadedLabel->setText(Utils::formatByteSize(currentSessionStats->uploaded()));
            sessionRatioLabel->setText(Utils::formatRatio(currentSessionStats->downloaded(), currentSessionStats->uploaded()));
            sessionDurationLabel->setText(Utils::formatEta(currentSessionStats->duration()));

            totalDownloadedLabel->setText(Utils::formatByteSize(totalStats->downloaded()));
            totalUploadedLabel->setText(Utils::formatByteSize(totalStats->uploaded()));
            totalRatioLabel->setText(Utils::formatRatio(totalStats->downloaded(), totalStats->uploaded()));
            totalDurationLabel->setText(Utils::formatEta(totalStats->duration()));

            sessionCountLabel->setText(qApp->translate("tremotesf", "%n times", nullptr, totalStats->sessionCount()));
        };
        QObject::connect(rpc->serverStats(), &libtremotesf::ServerStats::updated, this, update);
        update();
    }

    QSize ServerStatsDialog::sizeHint() const
    {
        return minimumSizeHint().expandedTo(QSize(300, 320));
    }
}
