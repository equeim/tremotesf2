/*
 * Tremotesf
 * Copyright (C) 2015-2017 Alexey Rochev <equeim@gmail.com>
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

#include "servers.h"

#include <QCoreApplication>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>

namespace tremotesf
{
    namespace
    {
#ifdef Q_OS_WIN
        const QSettings::Format settingsFormat = QSettings::IniFormat;
#else
        const QSettings::Format settingsFormat = QSettings::NativeFormat;
#endif

        const QString versionKey(QStringLiteral("version"));
        const QString currentServerKey(QStringLiteral("current"));
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

        Servers* instancePointer = nullptr;
    }

    Servers* Servers::instance()
    {
        if (!instancePointer) {
            instancePointer = new Servers(qApp);
        }
        return instancePointer;
    }

#ifdef TREMOTESF_SAILFISHOS
    void Servers::migrateFrom0()
    {
        QSettings settings;
        if (settings.value(versionKey).toInt() != 1) {
            QSettings ServersSettings(qApp->organizationName(), QStringLiteral("Servers"));
            if (ServersSettings.childGroups().isEmpty()) {
                for (const QString& group : settings.childGroups()) {
                    settings.beginGroup(group);
                    ServersSettings.beginGroup(group);

                    ServersSettings.setValue(addressKey, settings.value(addressKey));
                    ServersSettings.setValue(portKey, settings.value(portKey));
                    ServersSettings.setValue(apiPathKey, settings.value(apiPathKey));
                    ServersSettings.setValue(httpsKey, settings.value(httpsKey));
                    if (settings.value(localCertificateKey).toBool()) {
                        const QString localCertificatePath(QStandardPaths::locate(QStandardPaths::DataLocation,
                                                                                  QStringLiteral("%1.pem").arg(group)));
                        if (!localCertificatePath.isEmpty()) {
                            QFile file(localCertificatePath);
                            if (file.open(QFile::ReadOnly)) {
                                ServersSettings.setValue(localCertificateKey, file.readAll());
                            }
                        }
                    }
                    ServersSettings.setValue(authenticationKey, settings.value(authenticationKey));
                    ServersSettings.setValue(usernameKey, settings.value(usernameKey));
                    ServersSettings.setValue(passwordKey, settings.value(passwordKey));
                    ServersSettings.setValue(updateIntervalKey, settings.value(updateIntervalKey));
                    ServersSettings.setValue(timeoutKey, settings.value(timeoutKey));

                    ServersSettings.endGroup();
                    settings.endGroup();
                }
                ServersSettings.setValue(currentServerKey, settings.value(QStringLiteral("currentAccount")));
            }
            settings.clear();
            settings.setValue(versionKey, 1);
        }
    }
#endif
    void Servers::migrateFromAccounts()
    {
        QSettings accounts(settingsFormat,
                           QSettings::UserScope,
                           qApp->organizationName(),
                           QStringLiteral("accounts"));
        if (!accounts.childGroups().isEmpty()) {
            if (QFile::copy(accounts.fileName(),
                            QSettings(settingsFormat,
                                      QSettings::UserScope,
                                      qApp->organizationName(),
                                      QStringLiteral("servers"))
                                .fileName())) {
                QFile::remove(accounts.fileName());
            }
        }
    }

    bool Servers::hasServers() const
    {
        return !mSettings->childGroups().isEmpty();
    }

    QList<Server> Servers::servers()
    {
        QList<Server> list;
        for (const QString& group : mSettings->childGroups()) {
            list.append(getServer(group));
        }
        return list;
    }

    Server Servers::currentServer()
    {
        return getServer(currentServerName());
    }

    QString Servers::currentServerName() const
    {
        return mSettings->value(currentServerKey).toString();
    }

    QString Servers::currentServerAddress()
    {
        mSettings->beginGroup(currentServerName());
        const QString address(mSettings->value(addressKey).toString());
        mSettings->endGroup();
        return address;
    }

    void Servers::setCurrentServer(const QString& name)
    {
        mSettings->setValue(currentServerKey, name);
        emit currentServerChanged();
    }

    void Servers::setServer(const QString& oldName,
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
        const QString current(currentServerName());
        if (oldName == current) {
            if (name != oldName) {
                mSettings->setValue(currentServerKey, name);
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
            emit currentServerChanged();
        }

        if (oldName.isEmpty() && mSettings->childGroups().size() == 1) {
            emit hasServersChanged();
        }
    }

    void Servers::removeServer(const QString& name)
    {
        mSettings->remove(name);
        const QStringList Servers(mSettings->childGroups());
        if (Servers.isEmpty()) {
            setCurrentServer(QString());
            emit hasServersChanged();
        } else if (name == currentServerName()) {
            setCurrentServer(Servers.first());
        }
    }

    void Servers::saveServers(const QList<Server>& servers, const QString& current)
    {
        const bool hadServers = hasServers();
        mSettings->clear();
        mSettings->setValue(currentServerKey, current);
        for (const Server& server : servers) {
            mSettings->beginGroup(server.name);
            mSettings->setValue(addressKey, server.address);
            mSettings->setValue(portKey, server.port);
            mSettings->setValue(apiPathKey, server.apiPath);
            mSettings->setValue(httpsKey, server.https);
            mSettings->setValue(localCertificateKey, server.localCertificate);
            mSettings->setValue(authenticationKey, server.authentication);
            mSettings->setValue(usernameKey, server.username);
            mSettings->setValue(passwordKey, server.password);
            mSettings->setValue(updateIntervalKey, server.updateInterval);
            mSettings->setValue(timeoutKey, server.timeout);
            mSettings->endGroup();
        }
        emit currentServerChanged();
        if (hasServers() != hadServers) {
            emit hasServersChanged();
        }
    }

    Servers::Servers(QObject* parent)
        : QObject(parent),
          mSettings(new QSettings(settingsFormat,
                                  QSettings::UserScope,
                                  qApp->organizationName(),
                                  QStringLiteral("servers"),
                                  this))
    {
        if (hasServers()) {
            bool setFirst = true;
            const QString current(currentServerName());
            if (!current.isEmpty()) {
                for (const QString& group : mSettings->childGroups()) {
                    if (group == current) {
                        setFirst = false;
                        break;
                    }
                }
            }
            if (setFirst) {
                setCurrentServer(mSettings->childGroups().first());
            }
        } else {
            mSettings->remove(currentServerKey);
        }
    }

    Server Servers::getServer(const QString& name)
    {
        mSettings->beginGroup(name);
        const Server server{
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
            mSettings->value(timeoutKey).toInt()};
        mSettings->endGroup();
        return server;
    }
}
