// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <ranges>

#include <QComboBox>
#include <QCoreApplication>
#include <QFocusEvent>
#include <QGridLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>

#include "editlabelswidget.h"

#include "rpc/rpc.h"
#include "rpc/torrent.h"

using namespace Qt::StringLiterals;

namespace tremotesf {
    namespace {
        const QIcon& tagIcon() {
            static const auto icon = QIcon::fromTheme("tag"_L1);
            return icon;
        }
    }

    EditLabelsWidget::EditLabelsWidget(const std::vector<QString>& enabledLabels, Rpc* rpc, QWidget* parent)
        : QWidget{parent}, mRpc{rpc} {
        auto layout = new QGridLayout(this);
        layout->setContentsMargins(QMargins{});

        mLabelsList = new QListWidget(this);
        layout->addWidget(mLabelsList, 0, 0, 1, 2);
        mLabelsList->setSelectionMode(QAbstractItemView::ExtendedSelection);
        mLabelsList->setIconSize(QSize(16, 16));

        const auto addLabel = [=, this](const QString& label) {
            mLabelsList->addItem(label);
            const auto item = mLabelsList->item(mLabelsList->count() - 1);
            item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);
            item->setIcon(tagIcon());
        };
        for (const auto& label : enabledLabels) {
            addLabel(label);
        }
        mLabelsList->sortItems();

        const auto updateListVisibility = [this] { mLabelsList->setVisible(mLabelsList->count() > 0); };
        updateListVisibility();
        QObject::connect(mLabelsList->model(), &QAbstractItemModel::rowsInserted, this, updateListVisibility);
        QObject::connect(mLabelsList->model(), &QAbstractItemModel::rowsRemoved, this, updateListVisibility);

        const auto removeAction =
            new QAction(QIcon::fromTheme("list-remove"_L1), qApp->translate("tremotesf", "&Remove"), mLabelsList);
        mLabelsList->addAction(removeAction);
        removeAction->setShortcut(QKeySequence::Delete);
        QObject::connect(removeAction, &QAction::triggered, this, [this] { qDeleteAll(mLabelsList->selectedItems()); });

        mLabelsList->setContextMenuPolicy(Qt::CustomContextMenu);
        QObject::connect(mLabelsList, &QWidget::customContextMenuRequested, this, [=, this](const QPoint& pos) {
            if (mLabelsList->selectionModel()->hasSelection() && mLabelsList->indexAt(pos).isValid()) {
                const auto menu = new QMenu(this);
                menu->addAction(removeAction);
                menu->popup(mLabelsList->viewport()->mapToGlobal(pos));
            }
        });

        mComboBox = new QComboBox(this);
        layout->addWidget(mComboBox, 1, 0);
        mComboBox->setEditable(true);
        mComboBox->setInsertPolicy(QComboBox::NoInsert);
        mComboBox->lineEdit()->setPlaceholderText(qApp->translate("tremotesf", "New label..."));
        updateComboBoxLabels();
        QObject::connect(rpc, &Rpc::connectedChanged, this, &EditLabelsWidget::updateComboBoxLabels);

        const auto addButton =
            new QPushButton(QIcon::fromTheme("list-add"_L1), qApp->translate("tremotesf", "Add"), this);
        layout->addWidget(addButton, 1, 1);
        addButton->setEnabled(false);
        const auto updateButtonEnabledState = [=, this] {
            addButton->setEnabled(!mComboBox->currentText().trimmed().isEmpty());
        };
        QObject::connect(mComboBox, &QComboBox::currentTextChanged, this, updateButtonEnabledState);

        layout->setColumnStretch(0, 1);

        const auto addLabelFromComboBox = [=, this] {
            const auto text = mComboBox->currentText().trimmed();
            if (!text.isEmpty() && mLabelsList->findItems(text, Qt::MatchExactly).isEmpty()) {
                addLabel(text);
                return true;
            }
            return false;
        };

        QObject::connect(mComboBox, &QComboBox::activated, this, [=, this] {
            addLabelFromComboBox();
            mComboBox->setCurrentIndex(-1);
        });
        QObject::connect(mComboBox->lineEdit(), &QLineEdit::returnPressed, this, [=, this] {
            if (addLabelFromComboBox()) {
                mComboBox->clearEditText();
            }
        });
        QObject::connect(addButton, &QPushButton::clicked, this, [=, this] {
            if (addLabelFromComboBox()) {
                mComboBox->clearEditText();
            }
        });
    }

    void EditLabelsWidget::setFocusOnComboBox() { mComboBox->setFocus(); }

    bool EditLabelsWidget::comboBoxHasFocus() const { return mComboBox->hasFocus(); }

    std::vector<QString> EditLabelsWidget::enabledLabels() const {
        return std::views::iota(0, mLabelsList->count())
               | std::views::transform([this](int i) { return mLabelsList->item(i)->text(); })
               | std::ranges::to<std::vector>();
    }

    void EditLabelsWidget::updateComboBoxLabels() {
        mComboBox->clear();
        if (!mRpc->isConnected()) {
            return;
        }
        const auto allLabels = mRpc->torrents()
                               | std::views::transform([](const auto& t) { return t->data().labels; })
                               | std::views::join
                               | std::views::common // For some reason GCC 14 requies views::common here
                               | std::ranges::to<std::set>();
        for (const auto& label : allLabels) {
            mComboBox->addItem(tagIcon(), label);
        }
        mComboBox->setCurrentIndex(-1);
    }

} // namespace tremotesf
