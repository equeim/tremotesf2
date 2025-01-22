// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "torrentpropertiesdialog.h"

#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

#include <KMessageWidget>

#include "torrentpropertieswidget.h"
#include "settings.h"
#include "log/log.h"
#include "rpc/rpc.h"
#include "rpc/torrent.h"

SPECIALIZE_FORMATTER_FOR_QDEBUG(QRect)

namespace tremotesf {
    TorrentPropertiesDialog::TorrentPropertiesDialog(Torrent* torrent, Rpc* rpc, QWidget* parent)
        : QDialog(parent), mTorrentPropertiesWidget(new TorrentPropertiesWidget(rpc, this)) {
        auto layout = new QVBoxLayout(this);
        auto messageWidget = new KMessageWidget(this);
        layout->addWidget(messageWidget);
        messageWidget->setCloseButtonVisible(false);
        messageWidget->setMessageType(KMessageWidget::Warning);
        messageWidget->hide();

        mTorrentPropertiesWidget->setTorrent(torrent);
        layout->addWidget(mTorrentPropertiesWidget);

        QObject::connect(
            mTorrentPropertiesWidget,
            &TorrentPropertiesWidget::hasTorrentChanged,
            this,
            [=](bool hasTorrent) {
                if (hasTorrent) {
                    if (messageWidget->isVisible()) {
                        messageWidget->animatedHide();
                    }
                } else {
                    if (rpc->connectionState() == Rpc::ConnectionState::Disconnected) {
                        //: Message that appears when disconnected from server
                        messageWidget->setText(qApp->translate("tremotesf", "Disconnected"));
                    } else {
                        //: Message that appears when torrent is removed
                        messageWidget->setText(qApp->translate("tremotesf", "Torrent Removed"));
                    }
                    messageWidget->animatedShow();
                }
            }
        );

        auto dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
        QObject::connect(dialogButtonBox, &QDialogButtonBox::rejected, this, &TorrentPropertiesDialog::reject);
        layout->addWidget(dialogButtonBox);

        dialogButtonBox->button(QDialogButtonBox::Close)->setDefault(true);

        setWindowTitle(torrent->data().name);
        const QString torrentHash = torrent->data().hashString;
        QObject::connect(rpc, &Rpc::torrentsUpdated, this, [=, this] {
            auto torrent = rpc->torrentByHash(torrentHash);
            mTorrentPropertiesWidget->setTorrent(torrent);
            if (torrent) {
                setWindowTitle(torrent->data().name);
            }
        });

        restoreGeometry(Settings::instance()->get_torrentPropertiesDialogGeometry());
    }

    void TorrentPropertiesDialog::saveState() {
        debug().log("Saving TorrentPropertiesDialog state, window geometry is {}", geometry());
        Settings::instance()->set_torrentPropertiesDialogGeometry(saveGeometry());
        mTorrentPropertiesWidget->saveState();
    }
}
