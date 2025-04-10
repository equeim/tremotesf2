// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "connectionsettingsdialog.h"

#include <QAction>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QIcon>
#include <QItemSelectionModel>
#include <QListView>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>

#include "serversmodel.h"
#include "servereditdialog.h"
#include "ui/itemmodels/baseproxymodel.h"
#include "ui/widgets/listplaceholder.h"

using namespace Qt::StringLiterals;

namespace tremotesf {
    namespace {
        constexpr auto editIconName = "document-properties"_L1;
        constexpr auto removeIconName = "list-remove"_L1;
    }

    ConnectionSettingsDialog::ConnectionSettingsDialog(QWidget* parent)
        : QDialog(parent),
          //: Servers list placeholder
          mModel(new ServersModel(this)),
          mProxyModel(new BaseProxyModel(mModel, Qt::DisplayRole, std::nullopt, this)),
          mServersView(new QListView(this)) {
        //: Dialog title
        setWindowTitle(qApp->translate("tremotesf", "Connection Settings"));

        mProxyModel->sort();

        auto layout = new QGridLayout(this);

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

        QObject::connect(mServersView, &QListView::customContextMenuRequested, this, [=, this](QPoint pos) {
            if (mServersView->indexAt(pos).isValid()) {
                QMenu contextMenu;
                QAction* editAction = contextMenu.addAction(
                    QIcon::fromTheme(editIconName),
                    //: Server's context menu item
                    qApp->translate("tremotesf", "&Edit...")
                );
                QObject::connect(editAction, &QAction::triggered, this, &ConnectionSettingsDialog::showEditDialogs);
                contextMenu.addAction(removeAction);
                contextMenu.exec(mServersView->viewport()->mapToGlobal(pos));
            }
        });

        auto placeholder = createListPlaceholderLabel(qApp->translate("tremotesf", "No servers"));
        addListPlaceholderLabelToViewportAndManageVisibility(mServersView, placeholder);

        layout->addWidget(mServersView, 0, 0);

        auto buttonsLayout = new QVBoxLayout();
        layout->addLayout(buttonsLayout, 0, 1);
        auto addServerButton = new QPushButton(
            QIcon::fromTheme("list-add"_L1),
            //: Button
            qApp->translate("tremotesf", "Add Server..."),
            this
        );
        QObject::connect(addServerButton, &QPushButton::clicked, this, [=, this] {
            auto dialog = new ServerEditDialog(mModel, -1, this);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
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
        layout->addWidget(dialogButtonBox, 1, 0, 1, 2);

        resize(sizeHint().expandedTo(QSize(384, 320)));
    }

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
    }
}
