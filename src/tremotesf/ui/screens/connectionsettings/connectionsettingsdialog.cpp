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

#include "tremotesf/ui/itemmodels/baseproxymodel.h"
#include "serversmodel.h"
#include "servereditdialog.h"

namespace tremotesf
{
    namespace
    {
        const QLatin1String editIconName("document-properties");
        const QLatin1String removeIconName("list-remove");
    }

    ConnectionSettingsDialog::ConnectionSettingsDialog(QWidget* parent)
        : QDialog(parent),
          mNoServersWidget(new KMessageWidget(qApp->translate("tremotesf", "No servers"), this)),
          mModel(new ServersModel(this)),
          mProxyModel(new BaseProxyModel(mModel, Qt::DisplayRole, this)),
          mServersView(new QListView(this))
    {
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

        auto removeAction = new QAction(QIcon::fromTheme(removeIconName), qApp->translate("tremotesf", "&Remove"), this);
        removeAction->setShortcut(QKeySequence::Delete);
        mServersView->addAction(removeAction);
        QObject::connect(removeAction, &QAction::triggered, this, &ConnectionSettingsDialog::removeServers);

        QObject::connect(mServersView, &QListView::customContextMenuRequested, this, [=](auto pos) {
            if (mServersView->indexAt(pos).isValid()) {
                QMenu contextMenu;
                QAction* editAction = contextMenu.addAction(QIcon::fromTheme(editIconName), qApp->translate("tremotesf", "&Edit..."));
                QObject::connect(editAction, &QAction::triggered, this, &ConnectionSettingsDialog::showEditDialogs);
                contextMenu.addAction(removeAction);
                contextMenu.exec(QCursor::pos());
            }
        });

        layout->addWidget(mServersView, 1, 0);

        auto buttonsLayout = new QVBoxLayout();
        layout->addLayout(buttonsLayout, 0, 1, 2, 1);
        auto addServerButton = new QPushButton(QIcon::fromTheme(QLatin1String("list-add")), qApp->translate("tremotesf", "Add Server..."), this);
        QObject::connect(addServerButton, &QPushButton::clicked, this, [=] {
            auto dialog = new ServerEditDialog(mModel, -1, this);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            QObject::connect(dialog, &ServerEditDialog::accepted, this, [=] {
                if (mModel->servers().size() == 1) {
                    mNoServersWidget->animatedHide();
                }
            });
            dialog->show();
        });
        buttonsLayout->addWidget(addServerButton);
        auto editButton = new QPushButton(QIcon::fromTheme(editIconName), qApp->translate("tremotesf", "Edit..."), this);
        editButton->setEnabled(false);
        QObject::connect(editButton, &QPushButton::clicked, this, &ConnectionSettingsDialog::showEditDialogs);
        buttonsLayout->addWidget(editButton);
        auto removeButton = new QPushButton(QIcon::fromTheme(removeIconName), qApp->translate("tremotesf", "Remove"), this);
        removeButton->setEnabled(false);
        QObject::connect(removeButton, &QPushButton::clicked, this, &ConnectionSettingsDialog::removeServers);
        buttonsLayout->addWidget(removeButton);
        buttonsLayout->addStretch();

        QObject::connect(mServersView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [=] {
            const bool hasSelection = mServersView->selectionModel()->hasSelection();
            editButton->setEnabled(hasSelection);
            removeButton->setEnabled(hasSelection);
        });

        auto dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        QObject::connect(dialogButtonBox, &QDialogButtonBox::accepted, this, &ConnectionSettingsDialog::accept);
        QObject::connect(dialogButtonBox, &QDialogButtonBox::rejected, this, &ConnectionSettingsDialog::reject);
        layout->addWidget(dialogButtonBox, 2, 0, 1, 2);

        setMinimumSize(minimumSizeHint());
    }

    QSize ConnectionSettingsDialog::sizeHint() const
    {
        return minimumSizeHint().expandedTo(QSize(384, 320));
    }

    void ConnectionSettingsDialog::accept()
    {
        Servers::instance()->saveServers(mModel->servers(), mModel->currentServerName());
        QDialog::accept();
    }

    void ConnectionSettingsDialog::showEditDialogs()
    {
        const QModelIndexList indexes(mServersView->selectionModel()->selectedIndexes());
        for (const QModelIndex& index : indexes) {
            auto dialog = new ServerEditDialog(mModel,
                                               mProxyModel->sourceIndex(index).row(),
                                               this);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->show();
        }
    }

    void ConnectionSettingsDialog::removeServers()
    {
        while (mServersView->selectionModel()->hasSelection()) {
            mModel->removeServerAtIndex(mProxyModel->sourceIndex(mServersView->selectionModel()->selectedIndexes().first()));
        }
        if (mModel->servers().empty()) {
            mNoServersWidget->animatedShow();
        }
    }
}
