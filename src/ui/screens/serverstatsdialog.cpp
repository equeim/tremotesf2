// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
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

#include "coroutines/coroutines.h"
#include "rpc/serverstats.h"
#include "rpc/rpc.h"
#include "formatutils.h"

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
        currentSessionGroupBoxLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

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
        totalGroupBoxLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

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

        resize(sizeHint().expandedTo(QSize(300, 320)));

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

        auto update = [=, this] {
            const SessionStats currentSessionStats(rpc->serverStats()->currentSession());
            sessionDownloadedLabel->setText(formatutils::formatByteSize(currentSessionStats.downloaded()));
            sessionUploadedLabel->setText(formatutils::formatByteSize(currentSessionStats.uploaded()));
            sessionRatioLabel->setText(
                formatutils::formatRatio(currentSessionStats.downloaded(), currentSessionStats.uploaded())
            );
            sessionDurationLabel->setText(formatutils::formatEta(currentSessionStats.duration()));

            const SessionStats totalStats(rpc->serverStats()->total());
            totalDownloadedLabel->setText(formatutils::formatByteSize(totalStats.downloaded()));
            totalUploadedLabel->setText(formatutils::formatByteSize(totalStats.uploaded()));
            totalRatioLabel->setText(formatutils::formatRatio(totalStats.downloaded(), totalStats.uploaded()));
            totalDurationLabel->setText(formatutils::formatEta(totalStats.duration()));

            //: How many times Transmission was launched
            sessionCountLabel->setText(qApp->translate("tremotesf", "%Ln times", nullptr, totalStats.sessionCount()));

            mFreeSpaceCoroutineScope.cancelAll();
            mFreeSpaceCoroutineScope.launch(getDownloadDirFreeSpace(rpc, freeSpaceField));
        };
        QObject::connect(rpc->serverStats(), &ServerStats::updated, this, update);
        update();
    }

    Coroutine<> ServerStatsDialog::getDownloadDirFreeSpace(Rpc* rpc, QLabel* freeSpaceField) {
        const auto freeSpace = co_await rpc->getDownloadDirFreeSpace();
        if (freeSpace) {
            freeSpaceField->setText(formatutils::formatByteSize(*freeSpace));
        }
    }
}
