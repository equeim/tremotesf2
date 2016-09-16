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

#ifndef TREMOTESF_ACCOUNTSMODEL_H
#define TREMOTESF_ACCOUNTSMODEL_H

#include <QAbstractListModel>
#include <QList>

#include "accounts.h"

namespace tremotesf
{
    class AccountsModel : public QAbstractListModel
    {
        Q_OBJECT
    public:
#ifdef TREMOTESF_SAILFISHOS
        enum Role
        {
            NameRole = Qt::UserRole,
            IsCurrentRole,
            AddressRole,
            PortRole,
            ApiPathRole,
            HttpsRole,
            LocalCertificateRole,
            AuthenticationRole,
            UsernameRole,
            PasswordRole,
            UpdateIntervalRole,
            TimeoutRole
        };
        Q_ENUMS(Role)
#endif
        explicit AccountsModel(QObject* parent = nullptr);

        QVariant data(const QModelIndex& index, int role) const override;
        Qt::ItemFlags flags(const QModelIndex& index) const override;
        int rowCount(const QModelIndex&) const override;
        bool setData(const QModelIndex& modelIndex, const QVariant& value, int role) override;

        const QList<Account>& accounts() const;
        const QString& currentAccountName() const;

        Q_INVOKABLE bool hasAccount(const QString& name) const;

        Q_INVOKABLE void setAccount(const QString& oldName,
                                    const QString& name,
                                    const QString& address,
                                    int port,
                                    const QString& apiPath,
                                    bool https,
                                    const QByteArray& localCertificate,
                                    bool authentication,
                                    const QString& username,
                                    const QString& password,
                                    int updateInterval,
                                    int timeout);
        Q_INVOKABLE void removeAccountAtIndex(const QModelIndex& index);
        Q_INVOKABLE void removeAccountAtRow(int row);
#ifdef TREMOTESF_SAILFISHOS
    protected:
        QHash<int, QByteArray> roleNames() const override;
#endif
    private:
        int accountRow(const QString& name) const;

        QList<Account> mAccounts;
        QString mCurrentAccount;
    };
}

#endif // TREMOTESF_ACCOUNTSMODEL_H
