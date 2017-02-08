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

#include "accounts.h"

#include <QCoreApplication>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>

namespace tremotesf
{
    namespace
    {
        const QString versionKey(QStringLiteral("version"));
        const QString currentAccountKey(QStringLiteral("current"));
        const QString addressKey(QStringLiteral("address"));
        const QString portKey(QStringLiteral("port"));
        const QString apiPathKey(QStringLiteral("apiPath"));
        const QString httpsKey(QStringLiteral("https"));
        const QString localCertificateKey(QStringLiteral("localCertificate"));
        const QString authenticationKey(QStringLiteral("authentication"));
        const QString usernameKey(QStringLiteral("username"));
        const QString passwordKey(QStringLiteral("password"));
        const QString updateIntervalKey(QStringLiteral("updateInterval"));
        const QString timeoutKey(QStringLiteral("timeout"));

        Accounts* instancePointer = nullptr;
    }

    Accounts* Accounts::instance()
    {
        if (!instancePointer) {
            instancePointer = new Accounts(qApp);
        }
        return instancePointer;
    }

#ifdef TREMOTESF_SAILFISHOS
    void Accounts::migrate()
    {
        QSettings settings;
        if (settings.value(versionKey).toInt() != 1) {
            QSettings accountsSettings(qApp->organizationName(), QStringLiteral("accounts"));
            if (accountsSettings.childGroups().isEmpty()) {
                for (const QString& group : settings.childGroups()) {
                    settings.beginGroup(group);
                    accountsSettings.beginGroup(group);

                    accountsSettings.setValue(addressKey, settings.value(addressKey));
                    accountsSettings.setValue(portKey, settings.value(portKey));
                    accountsSettings.setValue(apiPathKey, settings.value(apiPathKey));
                    accountsSettings.setValue(httpsKey, settings.value(httpsKey));
                    if (settings.value(localCertificateKey).toBool()) {
                        const QString localCertificatePath(QStandardPaths::locate(QStandardPaths::DataLocation, QStringLiteral("%1.pem").arg(group)));
                        if (!localCertificatePath.isEmpty()) {
                            QFile file(localCertificatePath);
                            if (file.open(QFile::ReadOnly)) {
                                accountsSettings.setValue(localCertificateKey, file.readAll());
                            }
                        }
                    }
                    accountsSettings.setValue(authenticationKey, settings.value(authenticationKey));
                    accountsSettings.setValue(usernameKey, settings.value(usernameKey));
                    accountsSettings.setValue(passwordKey, settings.value(passwordKey));
                    accountsSettings.setValue(updateIntervalKey, settings.value(updateIntervalKey));
                    accountsSettings.setValue(timeoutKey, settings.value(timeoutKey));

                    accountsSettings.endGroup();
                    settings.endGroup();
                }
                accountsSettings.setValue(currentAccountKey, settings.value(QStringLiteral("currentAccount")));
            }
            settings.clear();
            settings.setValue(versionKey, 1);
        }
    }
#endif

    bool Accounts::hasAccounts() const
    {
        return !mSettings->childGroups().isEmpty();
    }

    QList<Account> Accounts::accounts()
    {
        QList<Account> list;
        for (const QString& group : mSettings->childGroups()) {
            list.append(getAccount(group));
        }
        return list;
    }

    Account Accounts::currentAccount()
    {
        return getAccount(currentAccountName());
    }

    QString Accounts::currentAccountName() const
    {
        return mSettings->value(currentAccountKey).toString();
    }

    QString Accounts::currentAccountAddress()
    {
        mSettings->beginGroup(currentAccountName());
        const QString address(mSettings->value(addressKey).toString());
        mSettings->endGroup();
        return address;
    }

    void Accounts::setCurrentAccount(const QString& name)
    {
        mSettings->setValue(currentAccountKey, name);
        emit currentAccountChanged();
    }

    void Accounts::setAccount(const QString& oldName,
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
        bool currentChanged = false;
        const QString current(currentAccountName());
        if (oldName == current) {
            if (name != oldName) {
                mSettings->setValue(currentAccountKey, name);
            }
            currentChanged = true;
        } else if (name == current) {
            currentChanged = true;
        }

        if (!oldName.isEmpty() && name != oldName) {
            mSettings->remove(oldName);
        }

        mSettings->beginGroup(name);
        mSettings->setValue(addressKey, address);
        mSettings->setValue(portKey, port);
        mSettings->setValue(apiPathKey, apiPath);
        mSettings->setValue(httpsKey, https);
        mSettings->setValue(localCertificateKey, localCertificate);
        mSettings->setValue(authenticationKey, authentication);
        mSettings->setValue(usernameKey, username);
        mSettings->setValue(passwordKey, password);
        mSettings->setValue(updateIntervalKey, updateInterval);
        mSettings->setValue(timeoutKey, timeout);
        mSettings->endGroup();

        if (currentChanged) {
            emit currentAccountChanged();
        }

        if (oldName.isEmpty() && mSettings->childGroups().size() == 1) {
            emit hasAccountsChanged();
        }
    }

    void Accounts::removeAccount(const QString &name)
    {
        mSettings->remove(name);
        const QStringList accounts(mSettings->childGroups());
        if (accounts.isEmpty()) {
            setCurrentAccount(QString());
            emit hasAccountsChanged();
        } else if (name == currentAccountName()) {
            setCurrentAccount(accounts.first());
        }
    }

    void Accounts::saveAccounts(const QList<Account>& accounts, const QString &current)
    {
        const bool hadAccounts = hasAccounts();
        mSettings->clear();
        mSettings->setValue(currentAccountKey, current);
        for (const Account& account : accounts) {
            mSettings->beginGroup(account.name);
            mSettings->setValue(addressKey, account.address);
            mSettings->setValue(portKey, account.port);
            mSettings->setValue(apiPathKey, account.apiPath);
            mSettings->setValue(httpsKey, account.https);
            mSettings->setValue(localCertificateKey, account.localCertificate);
            mSettings->setValue(authenticationKey, account.authentication);
            mSettings->setValue(usernameKey, account.username);
            mSettings->setValue(passwordKey, account.password);
            mSettings->setValue(updateIntervalKey, account.updateInterval);
            mSettings->setValue(timeoutKey, account.timeout);
            mSettings->endGroup();
        }
        emit currentAccountChanged();
        if (hasAccounts() != hadAccounts) {
            emit hasAccountsChanged();
        }
    }

    Accounts::Accounts(QObject* parent)
        : QObject(parent),


#ifdef Q_OS_WIN
          mSettings(new QSettings(QSettings::IniFormat,
#else
          mSettings(new QSettings(QSettings::NativeFormat,
#endif
                                  QSettings::UserScope,
                                  qApp->organizationName(),
                                  "accounts",
                                  this))
    {
        if (hasAccounts()) {
            bool setFirst = true;
            const QString current(currentAccountName());
            if (!current.isEmpty()) {
                for (const QString& group : mSettings->childGroups()) {
                    if (group == current) {
                        setFirst = false;
                        break;
                    }
                }
            }
            if (setFirst) {
                setCurrentAccount(mSettings->childGroups().first());
            }
        } else {
            mSettings->remove(currentAccountKey);
        }
    }

    Account Accounts::getAccount(const QString& name)
    {
        mSettings->beginGroup(name);
        const Account account {
            mSettings->group(),
                    mSettings->value(addressKey).toString(),
                    mSettings->value(portKey).toInt(),
                    mSettings->value(apiPathKey).toString(),
                    mSettings->value(httpsKey).toBool(),
                    mSettings->value(localCertificateKey).toByteArray(),
                    mSettings->value(authenticationKey).toBool(),
                    mSettings->value(usernameKey).toString(),
                    mSettings->value(passwordKey).toString(),
                    mSettings->value(updateIntervalKey).toInt(),
                    mSettings->value(timeoutKey).toInt()
        };
        mSettings->endGroup();
        return account;
    }
}
