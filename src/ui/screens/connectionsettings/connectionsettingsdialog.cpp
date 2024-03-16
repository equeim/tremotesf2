// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "connectionsettingsdialog.h"

#include <QAction>
#include <QCoreApplication>
#include <QCursor>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QIcon>
#include <QItemSelectionModel>
#include <QListView>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>

#include <KMessageWidget>

#include "ui/itemmodels/baseproxymodel.h"
#include "serversmodel.h"
#include "servereditdialog.h"

namespace tremotesf {
    namespace {
        constexpr auto editIconName = "document-properties"_l1;
        constexpr auto removeIconName = "list-remove"_l1;
    }

    ConnectionSettingsDialog::ConnectionSettingsDialog(QWidget* parent)
        : QDialog(parent),
          //: Servers list placeholder
          mNoServersWidget(new KMessageWidget(qApp->translate("tremotesf", "No servers"), this)),
          mModel(new ServersModel(this)),
          mProxyModel(new BaseProxyModel(mModel, Qt::DisplayRole, std::nullopt, this)),
          mServersView(new QListView(this)) {
        //: Dialog title
        setWindowTitle(qApp->translate("tremotesf", "Connection Settings"));

        mProxyModel->sort();

        auto layout = new QGridLayout(this);

        mNoServersWidget->setCloseButtonVisible(false);
        mNoServersWidget->setMessageType(KMessageWidget::Warning);
        if (!mModel->servers().empty()) {
            mNoServersWidget->hide();
        }
        layout->addWidget(mNoServersWidget, 0, 0);

        mServersView->setContextMenuPolicy(Qt::CustomContextMenu);
        mServersView->setSelectionMode(QListView::ExtendedSelection);
        mServersView->setModel(mProxyModel);
        QObject::connect(mServersView, &QListView::activated, this, &ConnectionSettingsDialog::showEditDialogs);

        auto removeAction = new QAction(
            QIcon::fromTheme(removeIconName),
            //: Server's context menu item
            qApp->translate("tremotesf", "&Remove"),
            this
        );
        removeAction->setShortcut(QKeySequence::Delete);
        mServersView->addAction(removeAction);
        QObject::connect(removeAction, &QAction::triggered, this, &ConnectionSettingsDialog::removeServers);

        QObject::connect(mServersView, &QListView::customContextMenuRequested, this, [=, this](auto pos) {
            if (mServersView->indexAt(pos).isValid()) {
                QMenu contextMenu;
                QAction* editAction = contextMenu.addAction(
                    QIcon::fromTheme(editIconName),
                    //: Server's context menu item
                    qApp->translate("tremotesf", "&Edit...")
                );
                QObject::connect(editAction, &QAction::triggered, this, &ConnectionSettingsDialog::showEditDialogs);
                contextMenu.addAction(removeAction);
                contextMenu.exec(QCursor::pos());
            }
        });

        layout->addWidget(mServersView, 1, 0);

        auto buttonsLayout = new QVBoxLayout();
        layout->addLayout(buttonsLayout, 0, 1, 2, 1);
        auto addServerButton = new QPushButton(
            QIcon::fromTheme("list-add"_l1),
            //: Button
            qApp->translate("tremotesf", "Add Server..."),
            this
        );
        QObject::connect(addServerButton, &QPushButton::clicked, this, [=, this] {
            auto dialog = new ServerEditDialog(mModel, -1, this);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            QObject::connect(dialog, &ServerEditDialog::accepted, this, [=, this] {
                if (mModel->servers().size() == 1) {
                    mNoServersWidget->animatedHide();
                }
            });
            dialog->show();
        });
        buttonsLayout->addWidget(addServerButton);
        auto editButton = new QPushButton(
            QIcon::fromTheme(editIconName),
            //: Button
            qApp->translate("tremotesf", "Edit..."),
            this
        );
        editButton->setEnabled(false);
        QObject::connect(editButton, &QPushButton::clicked, this, &ConnectionSettingsDialog::showEditDialogs);
        buttonsLayout->addWidget(editButton);
        auto removeButton = new QPushButton(
            QIcon::fromTheme(removeIconName),
            //: Button
            qApp->translate("tremotesf", "Remove"),
            this
        );
        removeButton->setEnabled(false);
        QObject::connect(removeButton, &QPushButton::clicked, this, &ConnectionSettingsDialog::removeServers);
        buttonsLayout->addWidget(removeButton);
        buttonsLayout->addStretch();

        QObject::connect(mServersView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [=, this] {
            const bool hasSelection = mServersView->selectionModel()->hasSelection();
            editButton->setEnabled(hasSelection);
            removeButton->setEnabled(hasSelection);
        });

        auto dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        QObject::connect(dialogButtonBox, &QDialogButtonBox::accepted, this, &ConnectionSettingsDialog::accept);
        QObject::connect(dialogButtonBox, &QDialogButtonBox::rejected, this, &ConnectionSettingsDialog::reject);
        layout->addWidget(dialogButtonBox, 2, 0, 1, 2);
    }

    QSize ConnectionSettingsDialog::sizeHint() const { return minimumSizeHint().expandedTo(QSize(384, 320)); }

    void ConnectionSettingsDialog::accept() {
        Servers::instance()->saveServers(mModel->servers(), mModel->currentServerName());
        QDialog::accept();
    }

    void ConnectionSettingsDialog::showEditDialogs() {
        const QModelIndexList indexes(mServersView->selectionModel()->selectedIndexes());
        for (const QModelIndex& index : indexes) {
            auto dialog = new ServerEditDialog(mModel, mProxyModel->sourceIndex(index).row(), this);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->show();
        }
    }

    void ConnectionSettingsDialog::removeServers() {
        while (mServersView->selectionModel()->hasSelection()) {
            mModel->removeServerAtIndex(
                mProxyModel->sourceIndex(mServersView->selectionModel()->selectedIndexes().first())
            );
        }
        if (mModel->servers().empty()) {
            mNoServersWidget->animatedShow();
        }
    }
}
