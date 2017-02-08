/*
 * Tremotesf
 * Copyright (C) 2015-2016 Alexey Rochev <equeim@gmail.com>
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

#include "accountsdialog.h"

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

#include "../accountsmodel.h"
#include "../baseproxymodel.h"
#include "../utils.h"
#include "accounteditdialog.h"

namespace tremotesf
{
    namespace
    {
        const QString editIconName(QStringLiteral("document-properties"));
        const QString removeIconName(QStringLiteral("list-remove"));
    }

    AccountsDialog::AccountsDialog(QWidget* parent)
        : QDialog(parent)
    {
        setWindowTitle(qApp->translate("tremotesf", "Accounts"));

        mModel = new AccountsModel(this);
        mProxyModel = new BaseProxyModel(mModel, Qt::DisplayRole, this);
        mProxyModel->sort();

        auto layout = new QGridLayout(this);

        mNoAccountsWidget = new KMessageWidget(qApp->translate("tremotesf", "No accounts"), this);
        mNoAccountsWidget->setCloseButtonVisible(false);
        mNoAccountsWidget->setMessageType(KMessageWidget::Warning);
        if (!mModel->accounts().isEmpty()) {
            mNoAccountsWidget->hide();
        }
        layout->addWidget(mNoAccountsWidget, 0, 0);

        mAccountsView = new QListView(this);
        mAccountsView->setContextMenuPolicy(Qt::CustomContextMenu);
        mAccountsView->setSelectionMode(QListView::ExtendedSelection);
        mAccountsView->setModel(mProxyModel);
        QObject::connect(mAccountsView, &QListView::activated, this, &AccountsDialog::showEditDialogs);
        QObject::connect(mAccountsView, &QListView::customContextMenuRequested, this, [=](const QPoint& pos) {
            if (mAccountsView->indexAt(pos).isValid()) {
                QMenu contextMenu;
                QAction* editAction = contextMenu.addAction(QIcon::fromTheme(editIconName), qApp->translate("tremotesf", "&Edit..."));
                QObject::connect(editAction, &QAction::triggered, this, &AccountsDialog::showEditDialogs);
                QAction* removeAction = contextMenu.addAction(QIcon::fromTheme(removeIconName), qApp->translate("tremotesf", "&Remove"));
                QObject::connect(removeAction, &QAction::triggered, this, &AccountsDialog::removeAccounts);
                contextMenu.exec(QCursor::pos());
            }
        });
        layout->addWidget(mAccountsView, 1, 0);

        auto buttonsLayout = new QVBoxLayout();
        layout->addLayout(buttonsLayout, 0, 1, 2, 1);
        auto addAccountButton = new QPushButton(QIcon::fromTheme(QStringLiteral("list-add")), qApp->translate("tremotesf", "Add..."), this);
        QObject::connect(addAccountButton, &QPushButton::clicked, this, [=]() {
            auto dialog = new AccountEditDialog(mModel, -1, this);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            QObject::connect(dialog, &AccountEditDialog::accepted, this, [=]() {
                if (mModel->accounts().size() == 1) {
                    mNoAccountsWidget->animatedHide();
                }
            });
            dialog->show();
        });
        buttonsLayout->addWidget(addAccountButton);
        auto editButton = new QPushButton(QIcon::fromTheme(editIconName), qApp->translate("tremotesf", "Edit..."), this);
        editButton->setEnabled(false);
        QObject::connect(editButton, &QPushButton::clicked, this, &AccountsDialog::showEditDialogs);
        buttonsLayout->addWidget(editButton);
        auto removeButton = new QPushButton(QIcon::fromTheme(removeIconName), qApp->translate("tremotesf", "Remove"), this);
        removeButton->setEnabled(false);
        QObject::connect(removeButton, &QPushButton::clicked, this, &AccountsDialog::removeAccounts);
        buttonsLayout->addWidget(removeButton);
        buttonsLayout->addStretch();

        QObject::connect(mAccountsView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [=]() {
            const bool hasSelection = mAccountsView->selectionModel()->hasSelection();
            editButton->setEnabled(hasSelection);
            removeButton->setEnabled(hasSelection);
        });

        auto dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        QObject::connect(dialogButtonBox, &QDialogButtonBox::accepted, this, &AccountsDialog::accept);
        QObject::connect(dialogButtonBox, &QDialogButtonBox::rejected, this, &AccountsDialog::reject);
        layout->addWidget(dialogButtonBox, 2, 0, 1, 2);
    }

    QSize AccountsDialog::sizeHint() const
    {
        return layout()->totalMinimumSize().expandedTo(QSize(384, 320));
    }

    void AccountsDialog::accept()
    {
        Accounts::instance()->saveAccounts(mModel->accounts(), mModel->currentAccountName());
        QDialog::accept();
    }

    void AccountsDialog::showEditDialogs()
    {
        for (const QModelIndex& index : mAccountsView->selectionModel()->selectedIndexes()) {
            auto dialog = new AccountEditDialog(mModel,
                                                mProxyModel->sourceIndex(index).row(),
                                                this);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->show();
        }
    }

    void AccountsDialog::removeAccounts()
    {
        while (mAccountsView->selectionModel()->hasSelection()) {
            mModel->removeAccountAtIndex(mProxyModel->sourceIndex(mAccountsView->selectionModel()->selectedIndexes().first()));
        }
        if (mModel->accounts().isEmpty()) {
            mNoAccountsWidget->animatedShow();
        }
    }
}
