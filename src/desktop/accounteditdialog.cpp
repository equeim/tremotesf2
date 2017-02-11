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

#include "accounteditdialog.h"

#include <QAbstractButton>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpressionValidator>
#include <QSpinBox>
#include <QVBoxLayout>

#include "../accounts.h"
#include "../accountsmodel.h"
#include "../utils.h"

namespace tremotesf
{
    AccountEditDialog::AccountEditDialog(AccountsModel* accountsModel, int row, QWidget* parent)
        : QDialog(parent),
          mAccountsModel(accountsModel)
    {
        setupUi();

        if (row == -1) {
            setWindowTitle(qApp->translate("tremotesf", "Add Account"));

            mPortSpinBox->setValue(9091);
            mApiPathLineEdit->setText(QStringLiteral("/transmission/rpc"));
            mHttpsGroupBox->setChecked(false);
            mAuthenticationGroupBox->setChecked(false);
            mUpdateIntervalSpinBox->setValue(5);
            mTimeoutSpinBox->setValue(30);
        } else {
            const Account& account = mAccountsModel->accounts().at(row);

            mAccountName = account.name;
            setWindowTitle(mAccountName);

            mNameLineEdit->setText(mAccountName);
            mAddressLineEdit->setText(account.address);
            mPortSpinBox->setValue(account.port);
            mApiPathLineEdit->setText(account.apiPath);
            mHttpsGroupBox->setChecked(account.https);
            mLocalCertificateTextEdit->setPlainText(account.localCertificate);
            mAuthenticationGroupBox->setChecked(account.authentication);
            mUsernameLineEdit->setText(account.username);
            mPasswordLineEdit->setText(account.password);
            mUpdateIntervalSpinBox->setValue(account.updateInterval);
            mTimeoutSpinBox->setValue(account.timeout);
        }
    }

    QSize AccountEditDialog::sizeHint() const
    {
        return layout()->totalMinimumSize().expandedTo(QSize(384, 0));
    }

    void AccountEditDialog::accept()
    {
        if (mAccountsModel) {
            const QString newName(mNameLineEdit->text());
            if (newName != mAccountName && mAccountsModel->hasAccount(newName)) {
                QMessageBox messageBox(QMessageBox::Warning,
                                       qApp->translate("tremotesf", "Overwrite Account"),
                                       qApp->translate("tremotesf", "Account already exists"),
                                       QMessageBox::Ok | QMessageBox::Cancel,
                                       this);
                messageBox.setDefaultButton(QMessageBox::Cancel);
                messageBox.button(QMessageBox::Ok)->setText(qApp->translate("tremotesf", "Overwrite"));
                if (messageBox.exec() != QMessageBox::Ok) {
                    return;
                }
            }
        }
        setAccount();
        QDialog::accept();
    }

    void AccountEditDialog::setupUi()
    {
        auto layout = new QVBoxLayout(this);

        auto formLayout = new QFormLayout();
        layout->addLayout(formLayout);
        formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

        mNameLineEdit = new QLineEdit(this);
        mNameLineEdit->setValidator(new QRegularExpressionValidator(QRegularExpression(QStringLiteral("^\\S.*")), this));
        QObject::connect(mNameLineEdit, &QLineEdit::textChanged, this, &AccountEditDialog::canAcceptUpdate);
        formLayout->addRow(qApp->translate("tremotesf", "Name:"), mNameLineEdit);

        mAddressLineEdit = new QLineEdit(this);
        mAddressLineEdit->setValidator(new QRegularExpressionValidator(QRegularExpression(QStringLiteral("^\\S+")), this));
        QObject::connect(mAddressLineEdit, &QLineEdit::textChanged, this, &AccountEditDialog::canAcceptUpdate);
        formLayout->addRow(qApp->translate("tremotesf", "Address:"), mAddressLineEdit);

        mPortSpinBox = new QSpinBox(this);
        mPortSpinBox->setMaximum(65535);
        formLayout->addRow(qApp->translate("tremotesf", "Port:"), mPortSpinBox);

        mApiPathLineEdit = new QLineEdit(this);
        formLayout->addRow(qApp->translate("tremotesf", "API path:"), mApiPathLineEdit);

        mHttpsGroupBox = new QGroupBox(qApp->translate("tremotesf", "HTTPS"), this);
        mHttpsGroupBox->setCheckable(true);
        auto httpsGroupBoxLayout = new QVBoxLayout(mHttpsGroupBox);
        httpsGroupBoxLayout->addWidget(new QLabel(qApp->translate("tremotesf", "Local certificate:")));
        mLocalCertificateTextEdit = new QPlainTextEdit(this);
        httpsGroupBoxLayout->addWidget(mLocalCertificateTextEdit);
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
        mUpdateIntervalSpinBox->setSuffix(qApp->translate("tremotesf", " s"));
        formLayout->addRow(qApp->translate("tremotesf", "Update interval:"), mUpdateIntervalSpinBox);

        mTimeoutSpinBox = new QSpinBox(this);
        mTimeoutSpinBox->setMinimum(5);
        mTimeoutSpinBox->setMaximum(60);
        mTimeoutSpinBox->setSuffix(qApp->translate("tremotesf", " s"));
        formLayout->addRow(qApp->translate("tremotesf", "Timeout:"), mTimeoutSpinBox);

        mDialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        QObject::connect(mDialogButtonBox, &QDialogButtonBox::accepted, this, &AccountEditDialog::accept);
        QObject::connect(mDialogButtonBox, &QDialogButtonBox::rejected, this, &AccountEditDialog::reject);
        layout->addWidget(mDialogButtonBox);
    }

    void AccountEditDialog::canAcceptUpdate()
    {
        mDialogButtonBox->button(QDialogButtonBox::Ok)
            ->setEnabled(mNameLineEdit->hasAcceptableInput() &&
                         mAddressLineEdit->hasAcceptableInput());
    }

    void AccountEditDialog::setAccount()
    {
        if (mAccountsModel) {
            mAccountsModel->setAccount(mAccountName,
                                       mNameLineEdit->text(),
                                       mAddressLineEdit->text(),
                                       mPortSpinBox->value(),
                                       mApiPathLineEdit->text(),
                                       mHttpsGroupBox->isChecked(),
                                       mLocalCertificateTextEdit->toPlainText().toLatin1(),
                                       mAuthenticationGroupBox->isChecked(),
                                       mUsernameLineEdit->text(),
                                       mPasswordLineEdit->text(),
                                       mUpdateIntervalSpinBox->value(),
                                       mTimeoutSpinBox->value());
        } else {
            Accounts::instance()->setAccount(mAccountName,
                                             mNameLineEdit->text(),
                                             mAddressLineEdit->text(),
                                             mPortSpinBox->value(),
                                             mApiPathLineEdit->text(),
                                             mHttpsGroupBox->isChecked(),
                                             mLocalCertificateTextEdit->toPlainText().toLatin1(),
                                             mAuthenticationGroupBox->isChecked(),
                                             mUsernameLineEdit->text(),
                                             mPasswordLineEdit->text(),
                                             mUpdateIntervalSpinBox->value(),
                                             mTimeoutSpinBox->value());
        }
    }
}
