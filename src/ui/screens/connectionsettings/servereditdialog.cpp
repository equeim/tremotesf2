// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "servereditdialog.h"

#include <array>
#include <vector>

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QIcon>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpressionValidator>
#include <QScrollArea>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>

#include "fileutils.h"
#include "log/log.h"
#include "rpc/pathutils.h"
#include "stdutils.h"
#include "target_os.h"
#include "ui/widgets/commondelegate.h"
#include "rpc/servers.h"
#include "serversmodel.h"

namespace tremotesf {
    namespace {
        constexpr auto removeIconName = "list-remove"_l1;

        constexpr std::array proxyTypeComboBoxValues{
            ConnectionConfiguration::ProxyType::Default,
            ConnectionConfiguration::ProxyType::Http,
            ConnectionConfiguration::ProxyType::Socks5,
            ConnectionConfiguration::ProxyType::None,
        };

        ConnectionConfiguration::ProxyType proxyTypeFromComboBoxIndex(int index) {
            if (index == -1) {
                return ConnectionConfiguration::ProxyType::Default;
            }
            return proxyTypeComboBoxValues.at(static_cast<size_t>(index));
        }
    }

    class MountedDirectoriesWidget final : public QTableWidget {
        Q_OBJECT

    public:
        MountedDirectoriesWidget(int rows, int columns, QWidget* parent = nullptr)
            : QTableWidget(rows, columns, parent) {
            setMinimumHeight(192);
            setSelectionMode(QAbstractItemView::SingleSelection);
            setItemDelegate(new CommonDelegate(this));
            setHorizontalHeaderLabels({//: Column title in the list of mounted directories
                                       qApp->translate("tremotesf", "Local directory"),
                                       //: Column title in the list of mounted directories
                                       qApp->translate("tremotesf", "Remote directory")
            });
            horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
            horizontalHeader()->setSectionsClickable(false);
            verticalHeader()->setVisible(false);

            setContextMenuPolicy(Qt::CustomContextMenu);

            //: Context menu item
            auto removeAction =
                new QAction(QIcon::fromTheme(removeIconName), qApp->translate("tremotesf", "&Remove"), this);
            removeAction->setShortcut(QKeySequence::Delete);
            addAction(removeAction);

            QObject::connect(removeAction, &QAction::triggered, this, [=, this] {
                const auto items(selectionModel()->selectedIndexes());
                if (!items.isEmpty()) {
                    removeRow(items.first().row());
                }
            });

            QObject::connect(this, &QWidget::customContextMenuRequested, this, [=, this](auto pos) {
                const QModelIndex index(indexAt(pos));
                if (!index.isValid()) {
                    return;
                }

                QMenu contextMenu;

                QAction* selectAction = nullptr;
                if (index.column() == 0) {
                    //: Context menu item to open directory chooser
                    selectAction = contextMenu.addAction(qApp->translate("tremotesf", "&Select..."));
                }

                contextMenu.addAction(removeAction);

                auto executed = contextMenu.exec(viewport()->mapToGlobal(pos));
                if (executed && executed == selectAction) {
                    const QString directory(QFileDialog::getExistingDirectory(this));
                    if (!directory.isEmpty()) {
                        const auto item = this->item(index.row(), index.column());
                        if (item) {
                            item->setText(directory);
                            item->setToolTip(directory);
                        }
                    }
                }
            });
        }

        void addRow(const QString& localDirectory, const QString& remoteDirectory) {
            const int row = rowCount();
            insertRow(row);
            const auto localItem = new QTableWidgetItem(localDirectory);
            localItem->setToolTip(localDirectory);
            setItem(row, 0, localItem);
            const auto remoteItem = new QTableWidgetItem(remoteDirectory);
            remoteItem->setToolTip(remoteDirectory);
            setItem(row, 1, remoteItem);
        }

    protected:
        void keyPressEvent(QKeyEvent* event) override {
            QTableWidget::keyPressEvent(event);
            switch (event->key()) {
            case Qt::Key_Return:
            case Qt::Key_Enter:
                event->accept();
                if (state() != EditingState) {
                    edit(currentIndex());
                }
                break;
            default:
                break;
            }
        }
    };

    ServerEditDialog::ServerEditDialog(ServersModel* serversModel, int row, QWidget* parent)
        : QDialog(parent), mServersModel(serversModel) {
        setupUi();
        resize(sizeHint().expandedTo(QSize(384, 512)));

        if (row == -1) {
            //: Dialog title
            setWindowTitle(qApp->translate("tremotesf", "Add Server"));

            mPortSpinBox->setValue(9091);
            mApiPathLineEdit->setText("/transmission/rpc"_l1);
            mProxyTypeComboBox->setCurrentIndex(
                // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
                indexOfCasted<int>(proxyTypeComboBoxValues, ConnectionConfiguration::ProxyType::Default).value()
            );
            mHttpsGroupBox->setChecked(false);
            mAuthenticationGroupBox->setChecked(false);
            mUpdateIntervalSpinBox->setValue(5);
            mTimeoutSpinBox->setValue(30);
            mAutoReconnectGroupBox->setChecked(false);
            mAutoReconnectSpinBox->setValue(30);
        } else {
            const Server& server = mServersModel->servers().at(static_cast<size_t>(row));

            mServerName = server.name;
            setWindowTitle(mServerName);

            mNameLineEdit->setText(mServerName);
            mAddressLineEdit->setText(server.connectionConfiguration.address);
            mPortSpinBox->setValue(server.connectionConfiguration.port);
            mApiPathLineEdit->setText(server.connectionConfiguration.apiPath);

            mProxyTypeComboBox->setCurrentIndex(
                // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
                indexOfCasted<int>(proxyTypeComboBoxValues, server.connectionConfiguration.proxyType).value()
            );
            mProxyHostnameLineEdit->setText(server.connectionConfiguration.proxyHostname);
            mProxyPortSpinBox->setValue(server.connectionConfiguration.proxyPort);
            mProxyUserLineEdit->setText(server.connectionConfiguration.proxyUser);
            mProxyPasswordLineEdit->setText(server.connectionConfiguration.proxyPassword);

            mHttpsGroupBox->setChecked(server.connectionConfiguration.https);
            mSelfSignedCertificateCheckBox->setChecked(server.connectionConfiguration.selfSignedCertificateEnabled);
            mSelfSignedCertificateEdit->setPlainText(server.connectionConfiguration.selfSignedCertificate);
            mClientCertificateCheckBox->setChecked(server.connectionConfiguration.clientCertificateEnabled);
            mClientCertificateEdit->setPlainText(server.connectionConfiguration.clientCertificate);

            mAuthenticationGroupBox->setChecked(server.connectionConfiguration.authentication);
            mUsernameLineEdit->setText(server.connectionConfiguration.username);
            mPasswordLineEdit->setText(server.connectionConfiguration.password);

            mUpdateIntervalSpinBox->setValue(server.connectionConfiguration.updateInterval);
            mTimeoutSpinBox->setValue(server.connectionConfiguration.timeout);

            mAutoReconnectGroupBox->setChecked(server.connectionConfiguration.autoReconnectEnabled);
            mAutoReconnectSpinBox->setValue(server.connectionConfiguration.autoReconnectInterval);

            for (const auto& [localDirectory, remoteDirectory] : server.mountedDirectories) {
                mMountedDirectoriesWidget->addRow(localDirectory, remoteDirectory);
            }
        }

        setProxyFieldsVisible();
    }

    void ServerEditDialog::accept() {
        if (mServersModel) {
            const QString newName(mNameLineEdit->text());
            if (newName != mServerName && mServersModel->hasServer(newName)) {
                QMessageBox messageBox(
                    QMessageBox::Warning,
                    //: Dialog title
                    qApp->translate("tremotesf", "Overwrite Server"),
                    qApp->translate("tremotesf", "Server already exists"),
                    QMessageBox::Ok | QMessageBox::Cancel,
                    this
                );
                messageBox.setDefaultButton(QMessageBox::Cancel);
                //: Dialog's confirmation button
                messageBox.button(QMessageBox::Ok)->setText(qApp->translate("tremotesf", "Overwrite"));
                if (messageBox.exec() != QMessageBox::Ok) {
                    return;
                }
            }
        }
        setServer();
        QDialog::accept();
    }

    void ServerEditDialog::setupUi() {
        auto topLayout = new QVBoxLayout(this);
        auto scrollArea = new QScrollArea(this);
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        scrollArea->setWidgetResizable(true);
        topLayout->addWidget(scrollArea);

        auto widget = new QWidget(this);
        auto formLayout = new QFormLayout(widget);
        formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

        mNameLineEdit = new QLineEdit(this);
        mNameLineEdit->setValidator(new QRegularExpressionValidator(QRegularExpression(R"(^\S.*)"), this));
        QObject::connect(mNameLineEdit, &QLineEdit::textChanged, this, &ServerEditDialog::canAcceptUpdate);
        formLayout->addRow(qApp->translate("tremotesf", "Name:"), mNameLineEdit);

        mAddressLineEdit = new QLineEdit(this);
        auto addressValidator = new QRegularExpressionValidator(QRegularExpression(R"(^\S+)"), this);
        mAddressLineEdit->setValidator(addressValidator);
        QObject::connect(mAddressLineEdit, &QLineEdit::textChanged, this, &ServerEditDialog::canAcceptUpdate);
        formLayout->addRow(qApp->translate("tremotesf", "Address:"), mAddressLineEdit);

        mPortSpinBox = new QSpinBox(this);
        const int maxPort = 65535;
        mPortSpinBox->setMaximum(maxPort);
        formLayout->addRow(qApp->translate("tremotesf", "Port:"), mPortSpinBox);

        mApiPathLineEdit = new QLineEdit(this);
        formLayout->addRow(qApp->translate("tremotesf", "API path:"), mApiPathLineEdit);

        auto proxyGroupBox = new QGroupBox(qApp->translate("tremotesf", "Proxy"), this);
        mProxyLayout = new QFormLayout(proxyGroupBox);
        mProxyLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

        mProxyTypeComboBox = new QComboBox(this);
        mProxyTypeComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        for (const auto type : proxyTypeComboBoxValues) {
            switch (type) {
            case ConnectionConfiguration::ProxyType::Default:
                //: Default proxy option
                mProxyTypeComboBox->addItem(qApp->translate("tremotesf", "Default"));
                break;
            case ConnectionConfiguration::ProxyType::Http:
                //: HTTP proxy option
                mProxyTypeComboBox->addItem(qApp->translate("tremotesf", "HTTP"));
                break;
            case ConnectionConfiguration::ProxyType::Socks5:
                //: SOCKS5 proxy option
                mProxyTypeComboBox->addItem(qApp->translate("tremotesf", "SOCKS5"));
                break;
            case ConnectionConfiguration::ProxyType::None:
                //: None proxy option
                mProxyTypeComboBox->addItem(qApp->translate("tremotesf", "None"));
                break;
            }
        }
        QObject::connect(
            mProxyTypeComboBox,
            &QComboBox::currentTextChanged,
            this,
            &ServerEditDialog::setProxyFieldsVisible
        );
        mProxyLayout->addRow(qApp->translate("tremotesf", "Proxy type:"), mProxyTypeComboBox);

        mProxyHostnameLineEdit = new QLineEdit(this);
        mProxyHostnameLineEdit->setValidator(addressValidator);
        mProxyLayout->addRow(qApp->translate("tremotesf", "Address:"), mProxyHostnameLineEdit);

        mProxyPortSpinBox = new QSpinBox(this);
        mProxyPortSpinBox->setMaximum(maxPort);
        mProxyLayout->addRow(qApp->translate("tremotesf", "Port:"), mProxyPortSpinBox);

        mProxyUserLineEdit = new QLineEdit(this);
        mProxyLayout->addRow(qApp->translate("tremotesf", "Username:"), mProxyUserLineEdit);
        mProxyPasswordLineEdit = new QLineEdit(this);
        mProxyPasswordLineEdit->setEchoMode(QLineEdit::Password);
        mProxyLayout->addRow(qApp->translate("tremotesf", "Password:"), mProxyPasswordLineEdit);

        formLayout->addRow(proxyGroupBox);

        mHttpsGroupBox = new QGroupBox(qApp->translate("tremotesf", "HTTPS"), this);
        mHttpsGroupBox->setCheckable(true);

        auto httpsGroupBoxLayout = new QVBoxLayout(mHttpsGroupBox);

        mSelfSignedCertificateCheckBox = new QCheckBox(
            //: Check box label
            qApp->translate("tremotesf", "Server uses self-signed certificate"),
            this
        );
        httpsGroupBoxLayout->addWidget(mSelfSignedCertificateCheckBox);
        mSelfSignedCertificateEdit = new QPlainTextEdit(this);
        mSelfSignedCertificateEdit->setMinimumHeight(192);
        mSelfSignedCertificateEdit->setPlaceholderText(
            //: Text field placeholder
            qApp->translate("tremotesf", "Server's certificate in PEM format")
        );
        mSelfSignedCertificateEdit->setVisible(false);
        httpsGroupBoxLayout->addWidget(mSelfSignedCertificateEdit);
        auto* selfSignedCertificateLoadFromFile = new QPushButton(
            //: Button
            qApp->translate("tremotesf", "Load from file..."),
            this
        );
        selfSignedCertificateLoadFromFile->setVisible(false);
        QObject::connect(selfSignedCertificateLoadFromFile, &QPushButton::clicked, this, [this] {
            loadCertificateFromFile(mSelfSignedCertificateEdit);
        });
        httpsGroupBoxLayout->addWidget(selfSignedCertificateLoadFromFile);
        QObject::connect(mSelfSignedCertificateCheckBox, &QCheckBox::toggled, this, [=, this](bool checked) {
            mSelfSignedCertificateEdit->setVisible(checked);
            selfSignedCertificateLoadFromFile->setVisible(checked);
        });

        mClientCertificateCheckBox = new QCheckBox(
            //: Check box label
            qApp->translate("tremotesf", "Use client certificate authentication"),
            this
        );
        httpsGroupBoxLayout->addWidget(mClientCertificateCheckBox);
        mClientCertificateEdit = new QPlainTextEdit(this);
        mClientCertificateEdit->setMinimumHeight(192);
        mClientCertificateEdit->setPlaceholderText(
            //: Text field placeholder
            qApp->translate("tremotesf", "Certificate in PEM format with private key")
        );
        mClientCertificateEdit->setVisible(false);
        httpsGroupBoxLayout->addWidget(mClientCertificateEdit);
        //: Button
        auto* clientCertificateLoadFromFile = new QPushButton(qApp->translate("tremotesf", "Load from file..."), this);
        clientCertificateLoadFromFile->setVisible(false);
        QObject::connect(clientCertificateLoadFromFile, &QPushButton::clicked, this, [this] {
            loadCertificateFromFile(mClientCertificateEdit);
        });
        httpsGroupBoxLayout->addWidget(clientCertificateLoadFromFile);
        QObject::connect(mClientCertificateCheckBox, &QCheckBox::toggled, this, [=, this](bool checked) {
            mClientCertificateEdit->setVisible(checked);
            clientCertificateLoadFromFile->setVisible(checked);
        });

        formLayout->addRow(mHttpsGroupBox);

        //: Check box label
        mAuthenticationGroupBox = new QGroupBox(qApp->translate("tremotesf", "Authentication"), this);
        mAuthenticationGroupBox->setCheckable(true);
        auto authenticationGroupBoxLayout = new QFormLayout(mAuthenticationGroupBox);
        authenticationGroupBoxLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        mUsernameLineEdit = new QLineEdit(this);
        authenticationGroupBoxLayout->addRow(qApp->translate("tremotesf", "Username:"), mUsernameLineEdit);
        mPasswordLineEdit = new QLineEdit(this);
        mPasswordLineEdit->setEchoMode(QLineEdit::Password);
        authenticationGroupBoxLayout->addRow(qApp->translate("tremotesf", "Password:"), mPasswordLineEdit);
        formLayout->addRow(mAuthenticationGroupBox);

        mUpdateIntervalSpinBox = new QSpinBox(this);
        mUpdateIntervalSpinBox->setMinimum(1);
        mUpdateIntervalSpinBox->setMaximum(3600);
        //: Suffix that is added to input field with number of seconds, e.g. "30 s"
        mUpdateIntervalSpinBox->setSuffix(qApp->translate("tremotesf", " s"));
        formLayout->addRow(qApp->translate("tremotesf", "Update interval:"), mUpdateIntervalSpinBox);

        mTimeoutSpinBox = new QSpinBox(this);
        mTimeoutSpinBox->setMinimum(5);
        mTimeoutSpinBox->setMaximum(60);
        //: Suffix that is added to input field with number of seconds, e.g. "30 s"
        mTimeoutSpinBox->setSuffix(qApp->translate("tremotesf", " s"));
        formLayout->addRow(qApp->translate("tremotesf", "Timeout:"), mTimeoutSpinBox);

        //: Check box label
        mAutoReconnectGroupBox = new QGroupBox(qApp->translate("tremotesf", "Auto reconnect on error"), this);
        mAutoReconnectGroupBox->setCheckable(true);
        formLayout->addRow(mAutoReconnectGroupBox);
        auto* autoReconnectFormLayout = new QFormLayout(mAutoReconnectGroupBox);
        autoReconnectFormLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        mAutoReconnectSpinBox = new QSpinBox(this);
        mAutoReconnectSpinBox->setMinimum(1);
        mAutoReconnectSpinBox->setMaximum(3600);
        //: Suffix that is added to input field with number of seconds, e.g. "30 s"
        mAutoReconnectSpinBox->setSuffix(qApp->translate("tremotesf", " s"));
        autoReconnectFormLayout->addRow(
            qApp->translate("tremotesf", "Auto reconnect interval:"),
            mAutoReconnectSpinBox
        );

        auto mountedDirectoriesGroupBox = new QGroupBox(qApp->translate("tremotesf", "Mounted directories"), this);
        auto mountedDirectoriesLayout = new QGridLayout(mountedDirectoriesGroupBox);
        mMountedDirectoriesWidget = new MountedDirectoriesWidget(0, 2);
        mountedDirectoriesLayout->addWidget(mMountedDirectoriesWidget, 0, 0, 1, 2);
        auto addDirectoriesButton = new QPushButton(
            QIcon::fromTheme("list-add"_l1),
            //: Button
            qApp->translate("tremotesf", "Add"),
            this
        );
        QObject::connect(addDirectoriesButton, &QPushButton::clicked, this, [=, this] {
            const QString directory(QFileDialog::getExistingDirectory(this));
            if (!directory.isEmpty()) {
                mMountedDirectoriesWidget->addRow(directory, QString());
            }
        });
        mountedDirectoriesLayout->addWidget(addDirectoriesButton, 1, 0);
        auto removeDirectoriesButton = new QPushButton(
            QIcon::fromTheme(removeIconName),
            //: Button
            qApp->translate("tremotesf", "Remove"),
            this
        );
        QObject::connect(removeDirectoriesButton, &QPushButton::clicked, this, [=, this] {
            const auto items(mMountedDirectoriesWidget->selectionModel()->selectedIndexes());
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

    void ServerEditDialog::setProxyFieldsVisible() {
        bool visible{};
        switch (proxyTypeFromComboBoxIndex(mProxyTypeComboBox->currentIndex())) {
        case ConnectionConfiguration::ProxyType::Default:
        case ConnectionConfiguration::ProxyType::None:
            visible = false;
            break;
        default:
            visible = true;
        }
        for (int i = 1, max = mProxyLayout->rowCount(); i < max; ++i) {
            mProxyLayout->itemAt(i, QFormLayout::LabelRole)->widget()->setVisible(visible);
            mProxyLayout->itemAt(i, QFormLayout::FieldRole)->widget()->setVisible(visible);
        }
    }

    void ServerEditDialog::canAcceptUpdate() {
        mDialogButtonBox->button(QDialogButtonBox::Ok)
            ->setEnabled(mNameLineEdit->hasAcceptableInput() && mAddressLineEdit->hasAcceptableInput());
    }

    void ServerEditDialog::setServer() {
        std::vector<MountedDirectory> mountedDirectories{};
        mountedDirectories.reserve(static_cast<size_t>(mMountedDirectoriesWidget->rowCount()));
        for (int i = 0, max = mMountedDirectoriesWidget->rowCount(); i < max; ++i) {
            const auto localItem = mMountedDirectoriesWidget->item(i, 0);
            const QString localDirectory =
                localItem ? normalizePath(localItem->text().trimmed(), localPathOs) : QString{};
            const auto remoteItem = mMountedDirectoriesWidget->item(i, 1);
            const QString remoteDirectory = remoteItem ? remoteItem->text().trimmed() : QString{};
            if (!localDirectory.isEmpty() && !remoteDirectory.isEmpty()) {
                mountedDirectories.push_back({.localPath = localDirectory, .remotePath = remoteDirectory});
            }
        }

        if (mServersModel) {
            mServersModel->setServer(
                mServerName,
                mNameLineEdit->text(),
                mAddressLineEdit->text(),
                mPortSpinBox->value(),
                mApiPathLineEdit->text(),

                proxyTypeFromComboBoxIndex(mProxyTypeComboBox->currentIndex()),
                mProxyHostnameLineEdit->text(),
                mProxyPortSpinBox->value(),
                mProxyUserLineEdit->text(),
                mProxyPasswordLineEdit->text(),

                mHttpsGroupBox->isChecked(),
                mSelfSignedCertificateCheckBox->isChecked(),
                mSelfSignedCertificateEdit->toPlainText().toLatin1(),
                mClientCertificateCheckBox->isChecked(),
                mClientCertificateEdit->toPlainText().toLatin1(),

                mAuthenticationGroupBox->isChecked(),
                mUsernameLineEdit->text(),
                mPasswordLineEdit->text(),

                mUpdateIntervalSpinBox->value(),
                mTimeoutSpinBox->value(),

                mAutoReconnectGroupBox->isChecked(),
                mAutoReconnectSpinBox->value(),

                mountedDirectories
            );
        } else {
            Servers::instance()->setServer(
                mServerName,
                mNameLineEdit->text(),
                mAddressLineEdit->text(),
                mPortSpinBox->value(),
                mApiPathLineEdit->text(),

                proxyTypeFromComboBoxIndex(mProxyTypeComboBox->currentIndex()),
                mProxyHostnameLineEdit->text(),
                mProxyPortSpinBox->value(),
                mProxyUserLineEdit->text(),
                mProxyPasswordLineEdit->text(),

                mHttpsGroupBox->isChecked(),
                mSelfSignedCertificateCheckBox->isChecked(),
                mSelfSignedCertificateEdit->toPlainText().toLatin1(),
                mClientCertificateCheckBox->isChecked(),
                mClientCertificateEdit->toPlainText().toLatin1(),

                mAuthenticationGroupBox->isChecked(),
                mUsernameLineEdit->text(),
                mPasswordLineEdit->text(),

                mUpdateIntervalSpinBox->value(),
                mTimeoutSpinBox->value(),

                mAutoReconnectGroupBox->isChecked(),
                mAutoReconnectSpinBox->value(),

                mountedDirectories
            );
        }
    }

    void ServerEditDialog::loadCertificateFromFile(QPlainTextEdit* target) {
        auto* fileDialog = new QFileDialog(
            this,
            //: File chooser dialog title
            qApp->translate("tremotesf", "Select Files"),
            {},
            /*qApp->translate("tremotesf", "Torrent Files (*.torrent)")*/ {}
        );
        fileDialog->setAttribute(Qt::WA_DeleteOnClose);
        fileDialog->setFileMode(QFileDialog::ExistingFile);
        fileDialog->setMimeTypeFilters({"application/x-pem-file"_l1});

        QObject::connect(fileDialog, &QFileDialog::accepted, this, [=] {
            try {
                target->setPlainText(readFile(fileDialog->selectedFiles().first()));
            } catch (const QFileError& e) {
                warning().logWithException(e, "Failed to read certificate from file");
            }
        });

        if constexpr (targetOs == TargetOs::Windows) {
            fileDialog->open();
        } else {
            fileDialog->show();
        }
    }
}

#include "servereditdialog.moc"
