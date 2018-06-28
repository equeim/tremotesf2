/*
 * Tremotesf
 * Copyright (C) 2015-2018 Alexey Rochev <equeim@gmail.com>
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

#include "servereditdialog.h"

#include <QAbstractButton>
#include <QCheckBox>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QIcon>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpressionValidator>
#include <QScrollArea>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>

#include "../servers.h"
#include "../serversmodel.h"
#include "../utils.h"

namespace tremotesf
{
    ServerEditDialog::ServerEditDialog(ServersModel* serversModel, int row, QWidget* parent)
        : QDialog(parent),
          mServersModel(serversModel)
    {
        setupUi();

        if (row == -1) {
            setWindowTitle(qApp->translate("tremotesf", "Add Server"));

            mPortSpinBox->setValue(9091);
            mApiPathLineEdit->setText(QLatin1String("/transmission/rpc"));
            mHttpsGroupBox->setChecked(false);
            mAuthenticationGroupBox->setChecked(false);
            mUpdateIntervalSpinBox->setValue(5);
            mBackgroundUpdateIntervalSpinBox->setValue(30);
            mTimeoutSpinBox->setValue(30);
        } else {
            const Server& server = mServersModel->servers()[row];

            mServerName = server.name;
            setWindowTitle(mServerName);

            mNameLineEdit->setText(mServerName);
            mAddressLineEdit->setText(server.address);
            mPortSpinBox->setValue(server.port);
            mApiPathLineEdit->setText(server.apiPath);
            mHttpsGroupBox->setChecked(server.https);
            mSelfSignedCertificateCheckBox->setChecked(server.selfSignedCertificateEnabled);
            mSelfSignedCertificateEdit->setPlainText(server.selfSignedCertificate);
            mClientCertificateCheckBox->setChecked(server.clientCertificateEnabled);
            mClientCertificateEdit->setPlainText(server.clientCertificate);
            mAuthenticationGroupBox->setChecked(server.authentication);
            mUsernameLineEdit->setText(server.username);
            mPasswordLineEdit->setText(server.password);
            mUpdateIntervalSpinBox->setValue(server.updateInterval);
            mBackgroundUpdateIntervalSpinBox->setValue(server.backgroundUpdateInterval);
            mTimeoutSpinBox->setValue(server.timeout);

            for (auto i = server.mountedDirectories.cbegin(), end = server.mountedDirectories.cend();
                 i != end;
                 ++i) {
                const int row = mMountedDirectoriesWidget->rowCount();
                mMountedDirectoriesWidget->insertRow(row);
                mMountedDirectoriesWidget->setItem(row, 0, new QTableWidgetItem(i.key()));
                mMountedDirectoriesWidget->setItem(row, 1, new QTableWidgetItem(i.value().toString()));
            }
        }
    }

    QSize ServerEditDialog::sizeHint() const
    {
        return QDialog::sizeHint().expandedTo(QSize(std::max(static_cast<QScrollArea*>(layout()->itemAt(0)->widget())->widget()->sizeHint().width(), 384), 512));
    }

    void ServerEditDialog::accept()
    {
        if (mServersModel) {
            const QString newName(mNameLineEdit->text());
            if (newName != mServerName && mServersModel->hasServer(newName)) {
                QMessageBox messageBox(QMessageBox::Warning,
                                       qApp->translate("tremotesf", "Overwrite Server"),
                                       qApp->translate("tremotesf", "Server already exists"),
                                       QMessageBox::Ok | QMessageBox::Cancel,
                                       this);
                messageBox.setDefaultButton(QMessageBox::Cancel);
                messageBox.button(QMessageBox::Ok)->setText(qApp->translate("tremotesf", "Overwrite"));
                if (messageBox.exec() != QMessageBox::Ok) {
                    return;
                }
            }
        }
        setServer();
        QDialog::accept();
    }

    void ServerEditDialog::setupUi()
    {
        auto topLayout = new QVBoxLayout(this);
        auto scrollArea = new QScrollArea(this);
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea->setWidgetResizable(true);
        topLayout->addWidget(scrollArea);

        auto widget = new QWidget(this);
        auto formLayout = new QFormLayout(widget);
        formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

        mNameLineEdit = new QLineEdit(this);
        mNameLineEdit->setValidator(new QRegularExpressionValidator(QRegularExpression(QLatin1String("^\\S.*")), this));
        QObject::connect(mNameLineEdit, &QLineEdit::textChanged, this, &ServerEditDialog::canAcceptUpdate);
        formLayout->addRow(qApp->translate("tremotesf", "Name:"), mNameLineEdit);

        mAddressLineEdit = new QLineEdit(this);
        mAddressLineEdit->setValidator(new QRegularExpressionValidator(QRegularExpression(QLatin1String("^\\S+")), this));
        QObject::connect(mAddressLineEdit, &QLineEdit::textChanged, this, &ServerEditDialog::canAcceptUpdate);
        formLayout->addRow(qApp->translate("tremotesf", "Address:"), mAddressLineEdit);

        mPortSpinBox = new QSpinBox(this);
        mPortSpinBox->setMaximum(65535);
        formLayout->addRow(qApp->translate("tremotesf", "Port:"), mPortSpinBox);

        mApiPathLineEdit = new QLineEdit(this);
        formLayout->addRow(qApp->translate("tremotesf", "API path:"), mApiPathLineEdit);

        mHttpsGroupBox = new QGroupBox(qApp->translate("tremotesf", "HTTPS"), this);
        mHttpsGroupBox->setCheckable(true);

        auto httpsGroupBoxLayout = new QVBoxLayout(mHttpsGroupBox);

        mSelfSignedCertificateCheckBox = new QCheckBox(qApp->translate("tremotesf", "Server uses self-signed certificate"), this);
        mSelfSignedCertificateEdit = new QPlainTextEdit(this);
        mSelfSignedCertificateEdit->setMinimumHeight(192);
        mSelfSignedCertificateEdit->setPlaceholderText(qApp->translate("tremotesf", "Server's certificate in PEM format"));
        mSelfSignedCertificateEdit->setVisible(false);
        QObject::connect(mSelfSignedCertificateCheckBox, &QCheckBox::toggled, mSelfSignedCertificateEdit, &QWidget::setVisible);
        httpsGroupBoxLayout->addWidget(mSelfSignedCertificateCheckBox);
        httpsGroupBoxLayout->addWidget(mSelfSignedCertificateEdit);

        mClientCertificateCheckBox = new QCheckBox(qApp->translate("tremotesf", "Use client certificate authentication"), this);
        mClientCertificateEdit = new QPlainTextEdit(this);
        mClientCertificateEdit->setMinimumHeight(192);
        mClientCertificateEdit->setPlaceholderText(qApp->translate("tremotesf", "Certificate in PEM format with private key"));
        mClientCertificateEdit->setVisible(false);
        QObject::connect(mClientCertificateCheckBox, &QCheckBox::toggled, mClientCertificateEdit, &QWidget::setVisible);
        httpsGroupBoxLayout->addWidget(mClientCertificateCheckBox);
        httpsGroupBoxLayout->addWidget(mClientCertificateEdit);

        formLayout->addRow(mHttpsGroupBox);

        mAuthenticationGroupBox = new QGroupBox(qApp->translate("tremotesf", "Authentication"), this);
        mAuthenticationGroupBox->setCheckable(true);
        auto authenticationGroupBoxLayout = new QFormLayout(mAuthenticationGroupBox);
        mUsernameLineEdit = new QLineEdit(this);
        authenticationGroupBoxLayout->addRow(qApp->translate("tremotesf", "Username:"), mUsernameLineEdit);
        mPasswordLineEdit = new QLineEdit(this);
        mPasswordLineEdit->setEchoMode(QLineEdit::Password);
        authenticationGroupBoxLayout->addRow(qApp->translate("tremotesf", "Password:"), mPasswordLineEdit);
        formLayout->addRow(mAuthenticationGroupBox);

        mUpdateIntervalSpinBox = new QSpinBox(this);
        mUpdateIntervalSpinBox->setMinimum(1);
        mUpdateIntervalSpinBox->setMaximum(3600);
        //: Seconds
        mUpdateIntervalSpinBox->setSuffix(qApp->translate("tremotesf", " s"));
        formLayout->addRow(qApp->translate("tremotesf", "Update interval:"), mUpdateIntervalSpinBox);

        mBackgroundUpdateIntervalSpinBox = new QSpinBox(this);
        mBackgroundUpdateIntervalSpinBox->setMinimum(1);
        mBackgroundUpdateIntervalSpinBox->setMaximum(3600);
        //: Seconds
        mBackgroundUpdateIntervalSpinBox->setSuffix(qApp->translate("tremotesf", " s"));
        formLayout->addRow(qApp->translate("tremotesf", "Background update interval:"), mBackgroundUpdateIntervalSpinBox);

        mTimeoutSpinBox = new QSpinBox(this);
        mTimeoutSpinBox->setMinimum(5);
        mTimeoutSpinBox->setMaximum(60);
        //: Seconds
        mTimeoutSpinBox->setSuffix(qApp->translate("tremotesf", " s"));
        formLayout->addRow(qApp->translate("tremotesf", "Timeout:"), mTimeoutSpinBox);

        auto mountedDirectoriesGroupBox = new QGroupBox(qApp->translate("tremotesf", "Mounted directories"), this);
        auto mountedDirectoriesLayout = new QGridLayout(mountedDirectoriesGroupBox);
        mMountedDirectoriesWidget = new QTableWidget(0, 2);
        mMountedDirectoriesWidget->setMinimumHeight(192);
        mMountedDirectoriesWidget->setHorizontalHeaderLabels({qApp->translate("tremotesf", "Local directory"),
                                                             qApp->translate("tremotesf", "Remote directory")});
        mMountedDirectoriesWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        mMountedDirectoriesWidget->horizontalHeader()->setSectionsClickable(false);
        mMountedDirectoriesWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        mMountedDirectoriesWidget->setSelectionMode(QAbstractItemView::SingleSelection);
        mountedDirectoriesLayout->addWidget(mMountedDirectoriesWidget, 0, 0, 1, 2);
        auto addDirectoriesButton = new QPushButton(QIcon::fromTheme(QLatin1String("list-add")), qApp->translate("tremotesf", "Add"), this);
        QObject::connect(addDirectoriesButton, &QPushButton::clicked, this, [=]() {
            mMountedDirectoriesWidget->insertRow(mMountedDirectoriesWidget->rowCount());
        });
        mountedDirectoriesLayout->addWidget(addDirectoriesButton, 1, 0);
        auto removeDirectoriesButton = new QPushButton(QIcon::fromTheme(QLatin1String("list-remove")), qApp->translate("tremotesf", "Remove"), this);
        QObject::connect(removeDirectoriesButton, &QPushButton::clicked, this, [=]() {
            const auto items(mMountedDirectoriesWidget->selectionModel()->selectedRows());
            if (!items.isEmpty()) {
                mMountedDirectoriesWidget->removeRow(items.first().row());
            }
        });
        mountedDirectoriesLayout->addWidget(removeDirectoriesButton, 1, 1);
        formLayout->addRow(mountedDirectoriesGroupBox);

        scrollArea->setWidget(widget);

        mDialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        QObject::connect(mDialogButtonBox, &QDialogButtonBox::accepted, this, &ServerEditDialog::accept);
        QObject::connect(mDialogButtonBox, &QDialogButtonBox::rejected, this, &ServerEditDialog::reject);
        topLayout->addWidget(mDialogButtonBox);
    }

    void ServerEditDialog::canAcceptUpdate()
    {
        mDialogButtonBox->button(QDialogButtonBox::Ok)
            ->setEnabled(mNameLineEdit->hasAcceptableInput() &&
                         mAddressLineEdit->hasAcceptableInput());
    }

    void ServerEditDialog::setServer()
    {
        QVariantMap mountedDirectories;
        for (int i = 0, max = mMountedDirectoriesWidget->rowCount(); i < max; ++i) {
            const auto localItem = mMountedDirectoriesWidget->item(i, 0);
            const QString localDirectory(localItem ? localItem->text().trimmed() : QString());
            const auto remoteItem = mMountedDirectoriesWidget->item(i, 1);
            const QString remoteDirectory(remoteItem ? remoteItem->text().trimmed() : QString());
            if (!localDirectory.isEmpty() && !remoteDirectory.isEmpty()) {
                mountedDirectories.insert(localDirectory, remoteDirectory);
            }
        }

        if (mServersModel) {

            mServersModel->setServer(mServerName,
                                     mNameLineEdit->text(),
                                     mAddressLineEdit->text(),
                                     mPortSpinBox->value(),
                                     mApiPathLineEdit->text(),
                                     mHttpsGroupBox->isChecked(),
                                     mSelfSignedCertificateCheckBox->isChecked(),
                                     mSelfSignedCertificateEdit->toPlainText().toLatin1(),
                                     mClientCertificateCheckBox->isChecked(),
                                     mClientCertificateEdit->toPlainText().toLatin1(),
                                     mAuthenticationGroupBox->isChecked(),
                                     mUsernameLineEdit->text(),
                                     mPasswordLineEdit->text(),
                                     mUpdateIntervalSpinBox->value(),
                                     mBackgroundUpdateIntervalSpinBox->value(),
                                     mTimeoutSpinBox->value(),
                                     mountedDirectories);
        } else {
            Servers::instance()->setServer(mServerName,
                                           mNameLineEdit->text(),
                                           mAddressLineEdit->text(),
                                           mPortSpinBox->value(),
                                           mApiPathLineEdit->text(),
                                           mHttpsGroupBox->isChecked(),
                                           mSelfSignedCertificateCheckBox->isChecked(),
                                           mSelfSignedCertificateEdit->toPlainText().toLatin1(),
                                           mClientCertificateCheckBox->isChecked(),
                                           mClientCertificateEdit->toPlainText().toLatin1(),
                                           mAuthenticationGroupBox->isChecked(),
                                           mUsernameLineEdit->text(),
                                           mPasswordLineEdit->text(),
                                           mUpdateIntervalSpinBox->value(),
                                           mBackgroundUpdateIntervalSpinBox->value(),
                                           mTimeoutSpinBox->value(),
                                           mountedDirectories);
        }
    }
}
