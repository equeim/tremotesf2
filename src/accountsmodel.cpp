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

#include "accountsmodel.h"

namespace tremotesf
{
    AccountsModel::AccountsModel(QObject* parent)
        : QAbstractListModel(parent),
          mAccounts(Accounts::instance()->accounts()),
          mCurrentAccount(Accounts::instance()->currentAccountName())
    {

    }

    QVariant AccountsModel::data(const QModelIndex& index, int role) const
    {
        const Account& account = mAccounts.at(index.row());
#ifdef TREMOTESF_SAILFISHOS
        switch (role) {
        case NameRole:
            return account.name;
        case IsCurrentRole:
            return (account.name == mCurrentAccount);
        case AddressRole:
            return account.address;
        case PortRole:
            return account.port;
        case ApiPathRole:
            return account.apiPath;
        case HttpsRole:
            return account.https;
        case LocalCertificateRole:
            return account.localCertificate;
        case AuthenticationRole:
            return account.authentication;
        case UsernameRole:
            return account.username;
        case PasswordRole:
            return account.password;
        case UpdateIntervalRole:
            return account.updateInterval;
        case TimeoutRole:
            return account.timeout;
        }
#else
        switch (role) {
        case Qt::CheckStateRole:
            if (account.name == mCurrentAccount) {
                return Qt::Checked;
            }
            return Qt::Unchecked;
        case Qt::DisplayRole:
            return account.name;
        }
#endif
        return QVariant();
    }

    Qt::ItemFlags AccountsModel::flags(const QModelIndex& index) const
    {
        return QAbstractListModel::flags(index) | Qt::ItemIsUserCheckable;
    }

    int AccountsModel::rowCount(const QModelIndex&) const
    {
        return mAccounts.size();
    }

    bool AccountsModel::setData(const QModelIndex& modelIndex, const QVariant& value, int role)
    {
#ifdef TREMOTESF_SAILFISHOS
        if (role == IsCurrentRole && value.toBool()) {
#else
        if (role == Qt::CheckStateRole && value.toInt() == Qt::Checked) {
#endif
            const QString current(mAccounts.at(modelIndex.row()).name);
            if (current != mCurrentAccount) {
                mCurrentAccount = current;
                emit dataChanged(index(0), index(mAccounts.size() - 1));
                return true;
            }
        }
        return false;
    }

    const QList<Account>& AccountsModel::accounts() const
    {
        return mAccounts;
    }

    const QString&AccountsModel::currentAccountName() const
    {
        return mCurrentAccount;
    }

    bool AccountsModel::hasAccount(const QString& name) const
    {
        if (accountRow(name) == -1) {
            return false;
        }
        return true;
    }

    void AccountsModel::setAccount(const QString& oldName,
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
                                   int timeout)
    {
        if (!oldName.isEmpty() && name != oldName) {
            const int row = accountRow(oldName);
            if (row != -1) {
                beginRemoveRows(QModelIndex(), row, row);
                mAccounts.removeAt(row);
                endRemoveRows();
            }
        }

        int row = accountRow(name);
        if (row == -1) {
            row = mAccounts.size();
            beginInsertRows(QModelIndex(), row, row);
            if (row == 0) {
                mCurrentAccount = name;
            }
            mAccounts.append(Account {
                                 name,
                                 address,
                                 port,
                                 apiPath,
                                 https,
                                 localCertificate,
                                 authentication,
                                 username,
                                 password,
                                 updateInterval,
                                 timeout
                             });
            endInsertRows();
        } else {
            Account& account = mAccounts[row];
            account.address = address;
            account.port = port;
            account.apiPath = apiPath;
            account.https = https;
            account.localCertificate = localCertificate;
            account.authentication = authentication;
            account.username = username;
            account.password = password;
            account.updateInterval = updateInterval;
            account.timeout = timeout;
            if (oldName == mCurrentAccount) {
                mCurrentAccount = name;
            }
            const QModelIndex modelIndex(index(row));
            emit dataChanged(modelIndex, modelIndex);
        }
    }

    void AccountsModel::removeAccountAtIndex(const QModelIndex& index)
    {
        removeAccountAtRow(index.row());
    }

    void AccountsModel::removeAccountAtRow(int row)
    {
        const bool current = (mAccounts.at(row).name == mCurrentAccount);
        beginRemoveRows(QModelIndex(), row, row);
        mAccounts.removeAt(row);
        endRemoveRows();
        if (current && !mAccounts.isEmpty()) {
            mCurrentAccount = mAccounts.first().name;
            const QModelIndex modelIndex(index(0, 0));
            emit dataChanged(modelIndex, modelIndex);
        }
    }

#ifdef TREMOTESF_SAILFISHOS
    QHash<int, QByteArray> AccountsModel::roleNames() const
    {
        return {{NameRole, "name"},
                {IsCurrentRole, "current"},
                {AddressRole, "address"},
                {PortRole, "port"},
                {ApiPathRole, "apiPath"},
                {HttpsRole, "https"},
                {LocalCertificateRole, "localCertificate"},
                {AuthenticationRole, "authentication"},
                {UsernameRole, "username"},
                {PasswordRole, "password"},
                {UpdateIntervalRole, "updateInterval"},
                {TimeoutRole, "timeout"}};
    }
#endif

    int AccountsModel::accountRow(const QString& name) const
    {
        for (int i = 0, max = mAccounts.size(); i < max; ++i) {
            if (mAccounts.at(i).name == name) {
                return i;
            }
        }
        return -1;
    }
}
