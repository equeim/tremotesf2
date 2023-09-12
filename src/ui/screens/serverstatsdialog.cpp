// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

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

#include "rpc/serverstats.h"
#include "rpc/trpc.h"
#include "utils.h"

namespace tremotesf {
    ServerStatsDialog::ServerStatsDialog(Rpc* rpc, QWidget* parent) : QDialog(parent) {
        //: Dialog title
        setWindowTitle(qApp->translate("tremotesf", "Server Stats"));

        auto layout = new QVBoxLayout(this);

        //: Message that appears when disconnected from server
        auto disconnectedWidget = new KMessageWidget(qApp->translate("tremotesf", "Disconnected"));
        disconnectedWidget->setCloseButtonVisible(false);
        disconnectedWidget->setMessageType(KMessageWidget::Warning);
        disconnectedWidget->hide();
        layout->addWidget(disconnectedWidget);

        //: Server stats section for current Transmission launch
        auto currentSessionGroupBox = new QGroupBox(qApp->translate("tremotesf", "Current session"), this);
        auto currentSessionGroupBoxLayout = new QFormLayout(currentSessionGroupBox);

        auto sessionDownloadedLabel = new QLabel(this);
        //: Downloaded bytes
        currentSessionGroupBoxLayout->addRow(qApp->translate("tremotesf", "Downloaded:"), sessionDownloadedLabel);
        auto sessionUploadedLabel = new QLabel(this);
        //: Uploaded bytes
        currentSessionGroupBoxLayout->addRow(qApp->translate("tremotesf", "Uploaded:"), sessionUploadedLabel);
        auto sessionRatioLabel = new QLabel(this);
        currentSessionGroupBoxLayout->addRow(qApp->translate("tremotesf", "Ratio:"), sessionRatioLabel);
        auto sessionDurationLabel = new QLabel(this);
        //: How much time Transmission is running
        currentSessionGroupBoxLayout->addRow(qApp->translate("tremotesf", "Duration:"), sessionDurationLabel);

        layout->addWidget(currentSessionGroupBox);

        //: Server stats section for all Transmission launches (accumulated)
        auto totalGroupBox = new QGroupBox(qApp->translate("tremotesf", "Total"), this);
        auto totalGroupBoxLayout = new QFormLayout(totalGroupBox);

        auto totalDownloadedLabel = new QLabel(this);
        //: Downloaded bytes
        totalGroupBoxLayout->addRow(qApp->translate("tremotesf", "Downloaded:"), totalDownloadedLabel);
        auto totalUploadedLabel = new QLabel(this);
        //: Uploaded bytes
        totalGroupBoxLayout->addRow(qApp->translate("tremotesf", "Uploaded:"), totalUploadedLabel);
        auto totalRatioLabel = new QLabel(this);
        totalGroupBoxLayout->addRow(qApp->translate("tremotesf", "Ratio:"), totalRatioLabel);
        auto totalDurationLabel = new QLabel(this);
        //: How much time Transmission is running
        totalGroupBoxLayout->addRow(qApp->translate("tremotesf", "Duration:"), totalDurationLabel);
        auto sessionCountLabel = new QLabel(this);
        //: How many times Transmission was launched
        totalGroupBoxLayout->addRow(qApp->translate("tremotesf", "Started:"), sessionCountLabel);
        auto freeSpaceField = new QLabel(this);
        totalGroupBoxLayout->addRow(qApp->translate("tremotesf", "Free space in download directory:"), freeSpaceField);
        auto* freeSpaceLabel = qobject_cast<QLabel*>(totalGroupBoxLayout->labelForField(freeSpaceField));
        freeSpaceLabel->setWordWrap(true);
        freeSpaceLabel->setAlignment(Qt::AlignTrailing | Qt::AlignVCenter);

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

        QObject::connect(rpc, &Rpc::connectedChanged, this, [=] {
            if (rpc->isConnected()) {
                disconnectedWidget->animatedHide();
            } else {
                disconnectedWidget->animatedShow();
            }
            currentSessionGroupBox->setEnabled(rpc->isConnected());
            totalGroupBox->setEnabled(rpc->isConnected());
            sessionCountLabel->setEnabled(rpc->isConnected());
        });

        auto update = [=] {
            const SessionStats currentSessionStats(rpc->serverStats()->currentSession());
            sessionDownloadedLabel->setText(Utils::formatByteSize(currentSessionStats.downloaded()));
            sessionUploadedLabel->setText(Utils::formatByteSize(currentSessionStats.uploaded()));
            sessionRatioLabel->setText(
                Utils::formatRatio(currentSessionStats.downloaded(), currentSessionStats.uploaded())
            );
            sessionDurationLabel->setText(Utils::formatEta(currentSessionStats.duration()));

            const SessionStats totalStats(rpc->serverStats()->total());
            totalDownloadedLabel->setText(Utils::formatByteSize(totalStats.downloaded()));
            totalUploadedLabel->setText(Utils::formatByteSize(totalStats.uploaded()));
            totalRatioLabel->setText(Utils::formatRatio(totalStats.downloaded(), totalStats.uploaded()));
            totalDurationLabel->setText(Utils::formatEta(totalStats.duration()));

            //: How many times Transmission was launched
            sessionCountLabel->setText(qApp->translate("tremotesf", "%Ln times", nullptr, totalStats.sessionCount()));

            rpc->getDownloadDirFreeSpace();
        };
        QObject::connect(rpc->serverStats(), &ServerStats::updated, this, update);
        QObject::connect(rpc, &Rpc::gotDownloadDirFreeSpace, this, [freeSpaceField](long long bytes) {
            freeSpaceField->setText(Utils::formatByteSize(bytes));
        });
        update();
    }

    QSize ServerStatsDialog::sizeHint() const { return minimumSizeHint().expandedTo(QSize(300, 320)); }
}
