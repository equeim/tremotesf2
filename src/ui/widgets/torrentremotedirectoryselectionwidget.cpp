// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QAbstractItemView>
#include <QCollator>
#include <QComboBox>
#include <QKeyEvent>
#include <QLineEdit>

#include "torrentremotedirectoryselectionwidget.h"

#include "stdutils.h"
#include "rpc/rpc.h"
#include "rpc/servers.h"
#include "rpc/serversettings.h"

namespace tremotesf {
    void TorrentDownloadDirectoryDirectorySelectionWidgetViewModel::saveDirectories() {
        if (mPath.isEmpty()) {
            return;
        }
        auto paths = toContainer<QStringList>(mComboBoxItems | std::views::transform(&ComboBoxItem::path));
        if (!paths.contains(mPath)) {
            paths.push_back(mPath);
        }
        auto servers = Servers::instance();
        servers->setCurrentServerLastDownloadDirectories(paths);
        servers->setCurrentServerLastDownloadDirectory(mPath);
    }

    std::vector<TorrentDownloadDirectoryDirectorySelectionWidgetViewModel::ComboBoxItem>
    TorrentDownloadDirectoryDirectorySelectionWidgetViewModel::createComboBoxItems() const {
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
        std::sort(directories.begin(), directories.end(), [&collator](const auto& first, const auto& second) {
            return collator.compare(first, second) < 0;
        });

        auto ret =
            toContainer<std::vector>(directories | std::views::transform([=, this](QString& dir) {
                                         QString display = toNativeSeparators(dir);
                                         return TorrentDownloadDirectoryDirectorySelectionWidgetViewModel::ComboBoxItem{
                                             .path = std::move(dir),
                                             .displayPath = std::move(display)
                                         };
                                     }));
        return ret;
    }

    void TorrentDownloadDirectoryDirectorySelectionWidgetViewModel::updateComboBoxItems() {
        auto items = createComboBoxItems();
        if (items != mComboBoxItems) {
            mComboBoxItems = std::move(items);
            emit comboBoxItemsChanged();
        }
    }

    namespace {
        class ComboBoxEventFilter final : public QObject {
            Q_OBJECT

        public:
            explicit ComboBoxEventFilter(QComboBox* comboBox) : QObject(comboBox), mComboBox(comboBox) {}

        protected:
            bool eventFilter(QObject* watched, QEvent* event) override {
                if (event->type() == QEvent::KeyPress &&
                    static_cast<QKeyEvent*>(event)->matches(QKeySequence::Delete)) {
                    const QModelIndex index(static_cast<QAbstractItemView*>(watched)->currentIndex());
                    if (index.isValid()) {
                        mComboBox->removeItem(index.row());
                        return true;
                    }
                }
                return QObject::eventFilter(watched, event);
            }

        private:
            QComboBox* mComboBox;
        };
    }

    QLineEdit* TorrentDownloadDirectoryDirectorySelectionWidget::lineEditFromTextField() {
        return qobject_cast<QComboBox*>(mTextField)->lineEdit();
    }

    QWidget* TorrentDownloadDirectoryDirectorySelectionWidget::createTextField() {
        const auto comboBox = new QComboBox();
        comboBox->setEditable(true);
        comboBox->view()->installEventFilter(new ComboBoxEventFilter(comboBox));
        return comboBox;
    }

    void TorrentDownloadDirectoryDirectorySelectionWidget::setup(QString path, const Rpc* rpc) {
        RemoteDirectorySelectionWidget::setup(std::move(path), rpc);

        const auto viewModel = qobject_cast<TorrentDownloadDirectoryDirectorySelectionWidgetViewModel*>(mViewModel);
        const auto comboBox = qobject_cast<QComboBox*>(mTextField);

        const auto updateItems = [=] {
            const auto items = viewModel->comboBoxItems();
            comboBox->clear();
            for (const auto& [itemPath, itemDisplayPath] : items) {
                comboBox->addItem(itemDisplayPath, itemPath);
            }
            comboBox->lineEdit()->setText(viewModel->displayPath());
        };
        updateItems();
        QObject::connect(
            viewModel,
            &TorrentDownloadDirectoryDirectorySelectionWidgetViewModel::comboBoxItemsChanged,
            this,
            updateItems
        );

        QObject::connect(comboBox, qOverload<int>(&QComboBox::activated), this, [=](int index) {
            if (index != -1) {
                viewModel->onComboBoxItemSelected(comboBox->itemData(index).toString(), comboBox->itemText(index));
            }
        });
    }
}

#include "torrentremotedirectoryselectionwidget.moc"
