// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QSettings>
#include <QTest>

#include "fileutils.h"
#include "servers.h"

using namespace Qt::StringLiterals;

namespace tremotesf {
    class ServersMigrationTest final : public QObject {
        Q_OBJECT
    private slots:
        void ClientCertificateMigration() {
            const auto expectedCertificateValue = "lol"_ba;

            auto settings = QSettings(QString{}, QSettings::IniFormat);
            settings.beginGroup("server"_L1);
            settings.setValue("address", "nope");
            settings.setValue("localCertificate"_L1, expectedCertificateValue);
            settings.endGroup();

            auto servers = Servers(&settings, nullptr);
            const auto server = servers.servers().at(0);
            QCOMPARE(server.connectionConfiguration.clientCertificateEnabled, true);
            QCOMPARE(server.connectionConfiguration.clientCertificate, expectedCertificateValue);
        }

        void ClientCertificateMigrationMissingKey() {
            auto settings = QSettings(QString{}, QSettings::IniFormat);
            settings.beginGroup("server"_L1);
            settings.setValue("address", "nope");
            settings.endGroup();

            auto servers = Servers(&settings, nullptr);
            const auto server = servers.servers().at(0);
            QCOMPARE(server.connectionConfiguration.clientCertificateEnabled, false);
            QCOMPARE(server.connectionConfiguration.clientCertificate.isEmpty(), true);
        }

        void ClientCertificateMigrationEmptyValue() {
            auto settings = QSettings(QString{}, QSettings::IniFormat);
            settings.beginGroup("server"_L1);
            settings.setValue("address", "nope");
            settings.setValue("localCertificate"_L1, QByteArray{});
            settings.endGroup();

            auto servers = Servers(&settings, nullptr);
            const auto server = servers.servers().at(0);
            QCOMPARE(server.connectionConfiguration.clientCertificateEnabled, false);
            QCOMPARE(server.connectionConfiguration.clientCertificate.isEmpty(), true);
        }

        void ServerCertificateMigrationSelfSigned() {
            const auto expectedCertificateValue = readFile(TEST_DATA_PATH "/server-certs/self-signed.pem");

            auto settings = QSettings(QString{}, QSettings::IniFormat);
            settings.beginGroup("server"_L1);
            settings.setValue("address", "nope");
            settings.setValue("selfSignedCertificateEnabled"_L1, true);
            settings.setValue("selfSignedCertificate"_L1, expectedCertificateValue);
            settings.endGroup();

            auto servers = Servers(&settings, nullptr);
            const auto server = servers.servers().at(0);
            QCOMPARE(
                server.connectionConfiguration.serverCertificateMode,
                ConnectionConfiguration::ServerCertificateMode::SelfSigned
            );
            QCOMPARE(server.connectionConfiguration.serverRootCertificate, expectedCertificateValue);
        }

        void ServerCertificateMigrationSelfSignedDisabled() {
            const auto expectedCertificateValue = readFile(TEST_DATA_PATH "/server-certs/self-signed.pem");

            auto settings = QSettings(QString{}, QSettings::IniFormat);
            settings.beginGroup("server"_L1);
            settings.setValue("address", "nope");
            settings.setValue("selfSignedCertificateEnabled"_L1, false);
            settings.setValue("selfSignedCertificate"_L1, expectedCertificateValue);
            settings.endGroup();

            auto servers = Servers(&settings, nullptr);
            const auto server = servers.servers().at(0);
            QCOMPARE(
                server.connectionConfiguration.serverCertificateMode,
                ConnectionConfiguration::ServerCertificateMode::None
            );
            QCOMPARE(server.connectionConfiguration.serverRootCertificate, expectedCertificateValue);
        }

        void ServerCertificateMigrationCustomRoot() {
            const auto expectedRootCertificateValue =
                readFile(TEST_DATA_PATH "/server-certs/signed-with-custom-root/root.pem");
            const auto expectedLeafCertificateValue =
                readFile(TEST_DATA_PATH "/server-certs/signed-with-custom-root/without-intermediate/leaf.pem");

            auto settings = QSettings(QString{}, QSettings::IniFormat);
            settings.beginGroup("server"_L1);
            settings.setValue("address", "nope");
            settings.setValue("selfSignedCertificateEnabled"_L1, true);
            settings.setValue(
                "selfSignedCertificate"_L1,
                expectedRootCertificateValue + "\n" + expectedLeafCertificateValue
            );
            settings.endGroup();

            auto servers = Servers(&settings, nullptr);
            const auto server = servers.servers().at(0);
            QCOMPARE(
                server.connectionConfiguration.serverCertificateMode,
                ConnectionConfiguration::ServerCertificateMode::CustomRoot
            );
            QCOMPARE(server.connectionConfiguration.serverRootCertificate, expectedRootCertificateValue);
            QCOMPARE(server.connectionConfiguration.serverLeafCertificate, expectedLeafCertificateValue);
        }

        void ServerCertificateMigrationCustomRootDisabled() {
            const auto expectedRootCertificateValue =
                readFile(TEST_DATA_PATH "/server-certs/signed-with-custom-root/root.pem");
            const auto expectedLeafCertificateValue =
                readFile(TEST_DATA_PATH "/server-certs/signed-with-custom-root/without-intermediate/leaf.pem");

            auto settings = QSettings(QString{}, QSettings::IniFormat);
            settings.beginGroup("server"_L1);
            settings.setValue("address", "nope");
            settings.setValue("selfSignedCertificateEnabled"_L1, false);
            settings.setValue(
                "selfSignedCertificate"_L1,
                expectedRootCertificateValue + "\n" + expectedLeafCertificateValue
            );
            settings.endGroup();

            auto servers = Servers(&settings, nullptr);
            const auto server = servers.servers().at(0);
            QCOMPARE(
                server.connectionConfiguration.serverCertificateMode,
                ConnectionConfiguration::ServerCertificateMode::None
            );
            QCOMPARE(server.connectionConfiguration.serverRootCertificate, expectedRootCertificateValue);
            QCOMPARE(server.connectionConfiguration.serverLeafCertificate, expectedLeafCertificateValue);
        }

        void ServerCertificateMigrationEmptyCertificateValue() {
            auto settings = QSettings(QString{}, QSettings::IniFormat);
            settings.beginGroup("server"_L1);
            settings.setValue("address", "nope");
            settings.setValue("selfSignedCertificateEnabled"_L1, true);
            settings.setValue("selfSignedCertificate"_L1, QByteArray{});
            settings.endGroup();

            auto servers = Servers(&settings, nullptr);
            const auto server = servers.servers().at(0);
            QCOMPARE(
                server.connectionConfiguration.serverCertificateMode,
                ConnectionConfiguration::ServerCertificateMode::None
            );
            QCOMPARE(server.connectionConfiguration.serverRootCertificate.isEmpty(), true);
            QCOMPARE(server.connectionConfiguration.serverLeafCertificate.isEmpty(), true);
        }

        void ServerCertificateMigrationCertificateCantBeParsed() {
            auto settings = QSettings(QString{}, QSettings::IniFormat);
            settings.beginGroup("server"_L1);
            settings.setValue("address", "nope");
            settings.setValue("selfSignedCertificateEnabled"_L1, true);
            settings.setValue("selfSignedCertificate"_L1, "lol"_L1);
            settings.endGroup();

            auto servers = Servers(&settings, nullptr);
            const auto server = servers.servers().at(0);
            QCOMPARE(
                server.connectionConfiguration.serverCertificateMode,
                ConnectionConfiguration::ServerCertificateMode::None
            );
            QCOMPARE(server.connectionConfiguration.serverRootCertificate.isEmpty(), true);
            QCOMPARE(server.connectionConfiguration.serverLeafCertificate.isEmpty(), true);
        }
    };
}

QTEST_GUILESS_MAIN(tremotesf::ServersMigrationTest)

#include "servers_migration_test.moc"
