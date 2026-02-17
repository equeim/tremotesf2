// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <ranges>

#include <QAbstractItemView>
#include <QCollator>
#include <QComboBox>
#include <QKeyEvent>
#include <QLineEdit>

#include "torrentremotedirectoryselectionwidget.h"

#include "rpc/rpc.h"
#include "rpc/servers.h"
#include "rpc/serversettings.h"

namespace tremotesf {
    void TorrentDownloadDirectoryDirectorySelectionWidgetViewModel::saveDirectories(
        std::vector<ComboBoxItem> comboBoxItems
    ) {
        if (mPath.isEmpty()) {
            return;
        }
        auto paths = comboBoxItems
                     | std::views::transform(&ComboBoxItem::path)
                     | std::views::as_rvalue
                     | std::ranges::to<QStringList>();
        if (!paths.contains(mPath)) {
            paths.push_back(mPath);
        }
        auto servers = Servers::instance();
        servers->setCurrentServerLastDownloadDirectories(paths);
        servers->setCurrentServerLastDownloadDirectory(mPath);
    }

    std::vector<TorrentDownloadDirectoryDirectorySelectionWidgetViewModel::ComboBoxItem>
    TorrentDownloadDirectoryDirectorySelectionWidgetViewModel::createInitialComboBoxItems() const {
        QStringList directories = Servers::instance()->currentServerLastDownloadDirectories(mRpc->serverSettings());
        directories.reserve(directories.size() + static_cast<QStringList::size_type>(mRpc->torrents().size()) + 2);
        for (const auto& torrent : mRpc->torrents()) {
            directories.push_back(torrent->data().downloadDirectory);
        }
        if (!mPath.isEmpty()) {
            directories.push_back(mPath);
        }
        directories.push_back(mRpc->serverSettings()->data().downloadDirectory);

        directories.removeDuplicates();

        QCollator collator{};
        collator.setCaseSensitivity(Qt::CaseInsensitive);
        collator.setNumericMode(true);
        // QStringList is not compatibly with std::ranges::sort in Qt 5
        std::ranges::sort(directories, [&collator](const QString& first, const QString& second) {
            return collator.compare(first, second) < 0;
        });

        auto ret = directories
                   | std::views::transform([=, this](QString& dir) {
                         QString display = toNativeSeparators(dir);
                         return TorrentDownloadDirectoryDirectorySelectionWidgetViewModel::ComboBoxItem{
                             .path = std::move(dir),
                             .displayPath = std::move(display)
                         };
                     })
                   | std::ranges::to<std::vector>();
        return ret;
    }

    void TorrentDownloadDirectoryDirectorySelectionWidgetViewModel::updateInitialComboBoxItems() {
        auto items = createInitialComboBoxItems();
        if (items != mInitialComboBoxItems) {
            mInitialComboBoxItems = std::move(items);
            emit initialComboBoxItemsChanged();
        }
    }

    QLineEdit* TorrentDownloadDirectoryDirectorySelectionWidget::lineEditFromTextField() {
        return qobject_cast<QComboBox*>(mTextField)->lineEdit();
    }

    QWidget* TorrentDownloadDirectoryDirectorySelectionWidget::createTextField() {
        const auto comboBox = new QComboBox();
        comboBox->setEditable(true);
        comboBox->setInsertPolicy(QComboBox::NoInsert);
        new ComboBoxDeleteKeyEventFilter(comboBox);
        return comboBox;
    }

    void TorrentDownloadDirectoryDirectorySelectionWidget::setup(QString path, const Rpc* rpc) {
        RemoteDirectorySelectionWidget::setup(std::move(path), rpc);

        const auto viewModel = qobject_cast<TorrentDownloadDirectoryDirectorySelectionWidgetViewModel*>(mViewModel);
        const auto comboBox = qobject_cast<QComboBox*>(mTextField);

        const auto updateItems = [=] {
            const auto items = viewModel->initialComboBoxItems();
            comboBox->clear();
            for (const auto& [itemPath, itemDisplayPath] : items) {
                comboBox->addItem(itemDisplayPath, itemPath);
            }
            comboBox->lineEdit()->setText(viewModel->displayPath());
        };
        updateItems();
        QObject::connect(
            viewModel,
            &TorrentDownloadDirectoryDirectorySelectionWidgetViewModel::initialComboBoxItemsChanged,
            this,
            updateItems
        );

        QObject::connect(comboBox, &QComboBox::activated, this, [=](int index) {
            if (index != -1) {
                viewModel->updatePathProgrammatically(comboBox->itemData(index).toString(), comboBox->itemText(index));
            }
        });
    }

    void TorrentDownloadDirectoryDirectorySelectionWidget::saveDirectories() {
        auto comboBox = qobject_cast<QComboBox*>(mTextField);
        auto comboBoxItems = std::views::iota(0, comboBox->count())
                             | std::views::transform([comboBox](int index) {
                                   return TorrentDownloadDirectoryDirectorySelectionWidgetViewModel::ComboBoxItem{
                                       .path = comboBox->itemData(index).toString(),
                                       .displayPath = comboBox->itemText(index)
                                   };
                               })
                             | std::ranges::to<std::vector>();
        qobject_cast<TorrentDownloadDirectoryDirectorySelectionWidgetViewModel*>(mViewModel)
            ->saveDirectories(std::move(comboBoxItems));
    }

    ComboBoxDeleteKeyEventFilter::ComboBoxDeleteKeyEventFilter(QComboBox* comboBox) : QObject(comboBox) {
        comboBox->view()->installEventFilter(this);
    }

    bool ComboBoxDeleteKeyEventFilter::eventFilter(QObject* watched, QEvent* event) {
        if (event->type() == QEvent::KeyPress && static_cast<QKeyEvent*>(event)->matches(QKeySequence::Delete)) {
            const auto index = qobject_cast<QAbstractItemView*>(watched)->currentIndex();
            if (index.isValid()) {
                qobject_cast<QComboBox*>(parent())->removeItem(index.row());
                return true;
            }
        }
        return QObject::eventFilter(watched, event);
    }

}
