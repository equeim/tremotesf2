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

#ifndef TREMOTESF_ACCOUNTS_H
#define TREMOTESF_ACCOUNTS_H

#include <QObject>

class QSettings;

namespace tremotesf
{
    struct Account
    {
        QString name;
        QString address;
        int port;
        QString apiPath;
        bool https;
        QByteArray localCertificate;
        bool authentication;
        QString username;
        QString password;
        int updateInterval;
        int timeout;
    };

    class Accounts : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(bool hasAccounts READ hasAccounts NOTIFY hasAccountsChanged)
        Q_PROPERTY(QString currentAccountName READ currentAccountName NOTIFY currentAccountChanged)
        Q_PROPERTY(QString currentAccountAddress READ currentAccountAddress NOTIFY currentAccountChanged)
    public:
        static Accounts* instance();

#ifdef TREMOTESF_SAILFISHOS
        static void migrate();
#endif

        bool hasAccounts() const;
        QList<Account> accounts();

        Account currentAccount();
        QString currentAccountName() const;
        QString currentAccountAddress();
        Q_INVOKABLE void setCurrentAccount(const QString& name);

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

        Q_INVOKABLE void removeAccount(const QString& name);

        void saveAccounts(const QList<Account>& accounts, const QString& current);
    private:
        explicit Accounts(QObject* parent = nullptr);
        Account getAccount(const QString& name);

        QSettings* mSettings;
    signals:
        void currentAccountChanged();
        void hasAccountsChanged();
    };
}

#endif // TREMOTESF_ACCOUNTS_H
