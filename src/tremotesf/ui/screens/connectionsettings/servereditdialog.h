// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_SERVEREDITDIALOG_H
#define TREMOTESF_SERVEREDITDIALOG_H

#include <QDialog>

class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QFormLayout;
class QGroupBox;
class QLineEdit;
class QPlainTextEdit;
class QSpinBox;

namespace tremotesf {
    class MountedDirectoriesWidget;
    class ServersModel;

    class ServerEditDialog : public QDialog {
        Q_OBJECT

    public:
        explicit ServerEditDialog(ServersModel* serversModel, int row, QWidget* parent = nullptr);
        QSize sizeHint() const override;
        void accept() override;

    private:
        void setupUi();
        void setProxyFieldsVisible();
        void canAcceptUpdate();
        void setServer();
        void loadCertificateFromFile(QPlainTextEdit* target);

    private:
        ServersModel* mServersModel;
        QString mServerName;

        QLineEdit* mNameLineEdit = nullptr;
        QLineEdit* mAddressLineEdit = nullptr;
        QSpinBox* mPortSpinBox = nullptr;
        QLineEdit* mApiPathLineEdit = nullptr;

        QFormLayout* mProxyLayout = nullptr;
        QComboBox* mProxyTypeComboBox = nullptr;
        QLineEdit* mProxyHostnameLineEdit = nullptr;
        QSpinBox* mProxyPortSpinBox = nullptr;
        QLineEdit* mProxyUserLineEdit = nullptr;
        QLineEdit* mProxyPasswordLineEdit = nullptr;

        QGroupBox* mHttpsGroupBox = nullptr;
        QCheckBox* mSelfSignedCertificateCheckBox = nullptr;
        QPlainTextEdit* mSelfSignedCertificateEdit = nullptr;
        QCheckBox* mClientCertificateCheckBox = nullptr;
        QPlainTextEdit* mClientCertificateEdit = nullptr;

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
