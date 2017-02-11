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

#ifndef TREMOTESF_ACCOUNTSDIALOG_H
#define TREMOTESF_ACCOUNTSDIALOG_H

#include <QDialog>

class QListView;
class KMessageWidget;

namespace tremotesf
{
    class AccountsModel;
    class BaseProxyModel;

    class AccountsDialog : public QDialog
    {
    public:
        explicit AccountsDialog(QWidget* parent = nullptr);
        QSize sizeHint() const override;
        void accept() override;

    private:
        void showEditDialogs();
        void removeAccounts();

    private:
        KMessageWidget* mNoAccountsWidget;
        AccountsModel* mModel;
        BaseProxyModel* mProxyModel;
        QListView* mAccountsView;
    };
}

#endif // TREMOTESF_ACCOUNTSDIALOG_H
