// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_SERVEREDITDIALOG_H
#define TREMOTESF_SERVEREDITDIALOG_H

#include <functional>
#include <QDialog>

class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QFormLayout;
class QGroupBox;
class QLineEdit;
class QSpinBox;

namespace tremotesf {
    class CertificateTextField;
    class MountedDirectoriesWidget;
    class ServersModel;

    class ServerEditDialog final : public QDialog {
        Q_OBJECT

    public:
        explicit ServerEditDialog(ServersModel* serversModel, int row, QWidget* parent = nullptr);
        void accept() override;

    private:
        std::function<void()> setupUi();
        void setServer();

        ServersModel* mServersModel;
        QString mServerName;

        QLineEdit* mNameLineEdit = nullptr;
        QLineEdit* mAddressLineEdit = nullptr;
        QSpinBox* mPortSpinBox = nullptr;
        QLineEdit* mApiPathLineEdit = nullptr;

        QComboBox* mProxyTypeComboBox = nullptr;
        QLineEdit* mProxyHostnameLineEdit = nullptr;
        QSpinBox* mProxyPortSpinBox = nullptr;
        QLineEdit* mProxyUserLineEdit = nullptr;
        QLineEdit* mProxyPasswordLineEdit = nullptr;

        QGroupBox* mHttpsGroupBox = nullptr;

        QComboBox* mServerCertificateModeComboBox = nullptr;
        CertificateTextField* mServerRootCertificateField = nullptr;
        CertificateTextField* mServerLeafCertificateField = nullptr;
        QCheckBox* mClientCertificateCheckBox = nullptr;
        CertificateTextField* mClientCertificateField = nullptr;

        QGroupBox* mAuthenticationGroupBox = nullptr;
        QLineEdit* mUsernameLineEdit = nullptr;
        QLineEdit* mPasswordLineEdit = nullptr;

        QSpinBox* mUpdateIntervalSpinBox = nullptr;
        QSpinBox* mTimeoutSpinBox = nullptr;

        QGroupBox* mAutoReconnectGroupBox = nullptr;
        QSpinBox* mAutoReconnectSpinBox = nullptr;

        MountedDirectoriesWidget* mMountedDirectoriesWidget = nullptr;

        QDialogButtonBox* mDialogButtonBox = nullptr;
    };
}

#endif // TREMOTESF_SERVEREDITDIALOG_H
