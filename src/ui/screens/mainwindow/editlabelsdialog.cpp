// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <ranges>

#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QVBoxLayout>

#include "editlabelsdialog.h"
#include "rpc/rpc.h"
#include "rpc/torrent.h"
#include "ui/widgets/editlabelswidget.h"

namespace tremotesf {
    EditLabelsDialog::EditLabelsDialog(const std::vector<Torrent*>& selectedTorrents, Rpc* rpc, QWidget* parent)
        : QDialog(parent) {
        setWindowTitle(qApp->translate("tremotesf", "Edit Labels"));

        auto layout = new QVBoxLayout(this);

        auto enabledLabels = selectedTorrents.at(0)->data().labels;
        if (!enabledLabels.empty() && selectedTorrents.size() > 1) {
            for (Torrent* torrent : std::views::drop(selectedTorrents, 1)) {
                if (torrent->data().labels != enabledLabels) {
                    enabledLabels.clear();
                    break;
                }
            }
        }
        auto editLabelsWidget = new EditLabelsWidget(enabledLabels, rpc, this);
        layout->addWidget(editLabelsWidget);

        auto torrentIds = selectedTorrents
                          | std::views::transform([](Torrent* t) { return t->data().id; })
                          | std::ranges::to<std::vector>();

        const auto saveLabels = [=, torrentIds = std::move(torrentIds)] {
            rpc->setTorrentsLabels(torrentIds, editLabelsWidget->enabledLabels());
        };

        auto dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        layout->addWidget(dialogButtonBox);
        QObject::connect(dialogButtonBox, &QDialogButtonBox::accepted, this, [=, this] {
            if (!editLabelsWidget->comboBoxHasFocus()) {
                saveLabels();
                accept();
            }
        });

        QObject::connect(dialogButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

        editLabelsWidget->setFocusOnComboBox();
    }
}
