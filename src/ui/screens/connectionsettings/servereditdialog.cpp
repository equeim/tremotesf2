// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
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
#include <QLabel>
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
#include "ui/widgets/tooltipwhenelideddelegate.h"
#include "rpc/servers.h"
#include "serversmodel.h"

using namespace Qt::StringLiterals;

namespace tremotesf {
    namespace {
        constexpr auto removeIconName = "list-remove"_L1;

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

        constexpr std::array serverCertificateModeComboBoxValues{
            ConnectionConfiguration::ServerCertificateMode::None,
            ConnectionConfiguration::ServerCertificateMode::SelfSigned,
            ConnectionConfiguration::ServerCertificateMode::CustomRoot
        };

        ConnectionConfiguration::ServerCertificateMode serverCertificateModeFromComboBoxIndex(int index) {
            if (index == -1) {
                return ConnectionConfiguration::ServerCertificateMode::None;
            }
            return serverCertificateModeComboBoxValues.at(static_cast<size_t>(index));
        }
    }

    class CertificateTextField final : public QWidget {
        Q_OBJECT
    public:
        explicit CertificateTextField(const QString& labelText = {}, QWidget* parent = nullptr) : QWidget(parent) {
            auto layout = new QVBoxLayout(this);
            layout->setContentsMargins({});

            layout->addWidget(&label);
            layout->addWidget(&textEdit);
            layout->addWidget(&mLoadFromFileButton);

            label.setText(labelText);
            textEdit.setMinimumHeight(192);

            QObject::connect(
                &mLoadFromFileButton,
                &QPushButton::clicked,
                this,
                &CertificateTextField::loadCertificateFromFile
            );
        }

        QLabel label;
        QPlainTextEdit textEdit;

    private:
        void loadCertificateFromFile() {
            auto* fileDialog = new QFileDialog(
                nativeParentWidget(),
                //: File chooser dialog title
                qApp->translate("tremotesf", "Select Files")
            );
            fileDialog->setAttribute(Qt::WA_DeleteOnClose);
            fileDialog->setFileMode(QFileDialog::ExistingFile);
            fileDialog->setMimeTypeFilters({"application/x-pem-file"_L1});

            QObject::connect(fileDialog, &QFileDialog::accepted, this, [=, this] {
                try {
                    textEdit.setPlainText(readFile(fileDialog->selectedFiles().first()));
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

        QPushButton mLoadFromFileButton{//: Button
                                        qApp->translate("tremotesf", "Load from file...")
        };
    };

    class MountedDirectoriesWidget final : public QTableWidget {
        Q_OBJECT

    public:
        MountedDirectoriesWidget(int rows, int columns, QWidget* parent = nullptr)
            : QTableWidget(rows, columns, parent) {
            setMinimumHeight(192);
            setSelectionMode(QAbstractItemView::SingleSelection);
            setItemDelegate(new TooltipWhenElidedDelegate(this));
            setHorizontalHeaderLabels(
                {//: Column title in the list of mounted directories
                 qApp->translate("tremotesf", "Local directory"),
                 //: Column title in the list of mounted directories
                 qApp->translate("tremotesf", "Remote directory")
                }
            );
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

            QObject::connect(this, &QWidget::customContextMenuRequested, this, [=, this](QPoint pos) {
                const QModelIndex index(indexAt(pos));
                if (!index.isValid()) {
                    return;
                }

                QMenu contextMenu;

                const QAction* selectAction = nullptr;
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
        const auto setInitialDependentState = setupUi();

        resize(sizeHint().expandedTo(QSize(384, 512)));

        if (row == -1) {
            //: Dialog title
            setWindowTitle(qApp->translate("tremotesf", "Add Server"));

            mPortSpinBox->setValue(9091);
            mApiPathLineEdit->setText("/transmission/rpc"_L1);
            mProxyTypeComboBox->setCurrentIndex(
                // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
                indexOfCasted<int>(proxyTypeComboBoxValues, ConnectionConfiguration::ProxyType::Default).value()
            );
            mHttpsGroupBox->setChecked(false);
            mServerCertificateModeComboBox->setCurrentIndex(
                // NOLINTBEGIN(bugprone-unchecked-optional-access)
                indexOfCasted<int>(
                    serverCertificateModeComboBoxValues,
                    ConnectionConfiguration::ServerCertificateMode::None
                )
                    .value()
                // NOLINTEND(bugprone-unchecked-optional-access)
            );
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
            mServerCertificateModeComboBox->setCurrentIndex(
                // NOLINTBEGIN(bugprone-unchecked-optional-access)
                indexOfCasted<int>(
                    serverCertificateModeComboBoxValues,
                    server.connectionConfiguration.serverCertificateMode
                )
                    .value()
                // NOLINTEND(bugprone-unchecked-optional-access)
            );
            mServerRootCertificateField->textEdit.setPlainText(server.connectionConfiguration.serverRootCertificate);
            mServerLeafCertificateField->textEdit.setPlainText(server.connectionConfiguration.serverLeafCertificate);
            mClientCertificateCheckBox->setChecked(server.connectionConfiguration.clientCertificateEnabled);
            mClientCertificateField->textEdit.setPlainText(server.connectionConfiguration.clientCertificate);

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

        setInitialDependentState();
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

    std::function<void()> ServerEditDialog::setupUi() {
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
        formLayout->addRow(qApp->translate("tremotesf", "Name:"), mNameLineEdit);

        mAddressLineEdit = new QLineEdit(this);
        auto addressValidator = new QRegularExpressionValidator(QRegularExpression(R"(^\S+)"), this);
        mAddressLineEdit->setValidator(addressValidator);
        formLayout->addRow(qApp->translate("tremotesf", "Address:"), mAddressLineEdit);

        mPortSpinBox = new QSpinBox(this);
        const int maxPort = 65535;
        mPortSpinBox->setMaximum(maxPort);
        formLayout->addRow(qApp->translate("tremotesf", "Port:"), mPortSpinBox);

        mApiPathLineEdit = new QLineEdit(this);
        formLayout->addRow(qApp->translate("tremotesf", "API path:"), mApiPathLineEdit);

        auto proxyGroupBox = new QGroupBox(qApp->translate("tremotesf", "Proxy"), this);
        auto proxyLayout = new QFormLayout(proxyGroupBox);
        proxyLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

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
        proxyLayout->addRow(qApp->translate("tremotesf", "Proxy type:"), mProxyTypeComboBox);

        mProxyHostnameLineEdit = new QLineEdit(this);
        mProxyHostnameLineEdit->setValidator(addressValidator);
        proxyLayout->addRow(qApp->translate("tremotesf", "Address:"), mProxyHostnameLineEdit);

        mProxyPortSpinBox = new QSpinBox(this);
        mProxyPortSpinBox->setMaximum(maxPort);
        proxyLayout->addRow(qApp->translate("tremotesf", "Port:"), mProxyPortSpinBox);

        mProxyUserLineEdit = new QLineEdit(this);
        proxyLayout->addRow(qApp->translate("tremotesf", "Username:"), mProxyUserLineEdit);
        mProxyPasswordLineEdit = new QLineEdit(this);
        mProxyPasswordLineEdit->setEchoMode(QLineEdit::Password);
        proxyLayout->addRow(qApp->translate("tremotesf", "Password:"), mProxyPasswordLineEdit);

        formLayout->addRow(proxyGroupBox);

        const auto updateProxyFieldsVisibility = [=, this] {
            bool visible{};
            switch (proxyTypeFromComboBoxIndex(mProxyTypeComboBox->currentIndex())) {
            case ConnectionConfiguration::ProxyType::Default:
            case ConnectionConfiguration::ProxyType::None:
                visible = false;
                break;
            default:
                visible = true;
            }
            for (int i = 1, max = proxyLayout->rowCount(); i < max; ++i) {
                proxyLayout->itemAt(i, QFormLayout::LabelRole)->widget()->setVisible(visible);
                proxyLayout->itemAt(i, QFormLayout::FieldRole)->widget()->setVisible(visible);
            }
        };
        QObject::connect(mProxyTypeComboBox, &QComboBox::currentTextChanged, this, updateProxyFieldsVisibility);

        mHttpsGroupBox = new QGroupBox(qApp->translate("tremotesf", "HTTPS"), this);
        mHttpsGroupBox->setCheckable(true);

        auto httpsGroupBoxLayout = new QFormLayout(mHttpsGroupBox);
        httpsGroupBoxLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

        mServerCertificateModeComboBox = new QComboBox(this);
        mServerCertificateModeComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        for (const auto type : serverCertificateModeComboBoxValues) {
            switch (type) {
            case ConnectionConfiguration::ServerCertificateMode::None:
                //: Server does not use custom certificates
                mServerCertificateModeComboBox->addItem(qApp->translate("tremotesf", "No"));
                break;
            case ConnectionConfiguration::ServerCertificateMode::SelfSigned:
                mServerCertificateModeComboBox->addItem(qApp->translate("tremotesf", "Self-signed certificate"));
                break;
            case ConnectionConfiguration::ServerCertificateMode::CustomRoot:
                mServerCertificateModeComboBox->addItem(qApp->translate("tremotesf", "Custom CA root certificate"));
                break;
            }
        }

        httpsGroupBoxLayout->addRow(
            qApp->translate("tremotesf", "Server uses custom certificates:"),
            mServerCertificateModeComboBox
        );

        mServerRootCertificateField = new CertificateTextField({}, this);
        httpsGroupBoxLayout->addRow(mServerRootCertificateField);

        auto serverLeafCertificateNotice = new QLabel(
            qApp->translate(
                "tremotesf",
                "If server's leaf certificate does not have correct host name, you need to provide it too for "
                "certificate validation to pass"
            ),
            this
        );
        serverLeafCertificateNotice->setWordWrap(true);
        httpsGroupBoxLayout->addRow(serverLeafCertificateNotice);

        mServerLeafCertificateField =
            new CertificateTextField(qApp->translate("tremotesf", "Server's leaf certificate in PEM format:"), this);
        httpsGroupBoxLayout->addRow(mServerLeafCertificateField);

        const auto updateServerCertificateFieldsState = [=, this] {
            switch (serverCertificateModeFromComboBoxIndex(mServerCertificateModeComboBox->currentIndex())) {
            case ConnectionConfiguration::ServerCertificateMode::SelfSigned:
                mServerRootCertificateField->label.setText(
                    qApp->translate("tremotesf", "Server's leaf certificate in PEM format:")
                );
                mServerRootCertificateField->show();
                serverLeafCertificateNotice->hide();
                mServerLeafCertificateField->hide();
                break;
            case ConnectionConfiguration::ServerCertificateMode::CustomRoot:
                mServerRootCertificateField->label.setText(
                    qApp->translate("tremotesf", "Server's CA root certificate in PEM format:")
                );
                mServerRootCertificateField->show();
                serverLeafCertificateNotice->show();
                mServerLeafCertificateField->show();
                break;
            case ConnectionConfiguration::ServerCertificateMode::None:
            default:
                mServerRootCertificateField->hide();
                serverLeafCertificateNotice->hide();
                mServerLeafCertificateField->hide();
                break;
            }
        };

        QObject::connect(
            mServerCertificateModeComboBox,
            &QComboBox::currentTextChanged,
            this,
            updateServerCertificateFieldsState
        );

        mClientCertificateCheckBox = new QCheckBox(
            //: Check box label
            qApp->translate("tremotesf", "Use client certificate authentication"),
            this
        );
        httpsGroupBoxLayout->addRow(mClientCertificateCheckBox);
        mClientCertificateField = new CertificateTextField(
            qApp->translate("tremotesf", "Client certificate with private key in PEM format:"),
            this
        );
        httpsGroupBoxLayout->addRow(mClientCertificateField);

        const auto updateClientCertificateFieldVisibility = [this] {
            mClientCertificateField->setVisible(mClientCertificateCheckBox->isChecked());
        };
        QObject::connect(mClientCertificateCheckBox, &QCheckBox::toggled, this, updateClientCertificateFieldVisibility);

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
            QIcon::fromTheme("list-add"_L1),
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

        auto dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        QObject::connect(dialogButtonBox, &QDialogButtonBox::accepted, this, &ServerEditDialog::accept);
        QObject::connect(dialogButtonBox, &QDialogButtonBox::rejected, this, &ServerEditDialog::reject);
        topLayout->addWidget(dialogButtonBox);

        const auto updateAcceptButtonState = [=, this] {
            dialogButtonBox->button(QDialogButtonBox::Ok)
                ->setEnabled(mNameLineEdit->hasAcceptableInput() && mAddressLineEdit->hasAcceptableInput());
        };
        QObject::connect(mNameLineEdit, &QLineEdit::textChanged, this, updateAcceptButtonState);
        QObject::connect(mAddressLineEdit, &QLineEdit::textChanged, this, updateAcceptButtonState);

        return [=] {
            updateProxyFieldsVisibility();
            updateServerCertificateFieldsState();
            updateClientCertificateFieldVisibility();
            updateAcceptButtonState();
        };
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
        ConnectionConfiguration connectionConfiguration{
            .address = mAddressLineEdit->text(),
            .port = mPortSpinBox->value(),
            .apiPath = mApiPathLineEdit->text(),

            .proxyType = proxyTypeFromComboBoxIndex(mProxyTypeComboBox->currentIndex()),
            .proxyHostname = mProxyHostnameLineEdit->text(),
            .proxyPort = mProxyPortSpinBox->value(),
            .proxyUser = mProxyUserLineEdit->text(),
            .proxyPassword = mProxyPasswordLineEdit->text(),

            .https = mHttpsGroupBox->isChecked(),

            .serverCertificateMode =
                serverCertificateModeFromComboBoxIndex(mServerCertificateModeComboBox->currentIndex()),
            .serverRootCertificate = mServerRootCertificateField->textEdit.toPlainText().toUtf8(),
            .serverLeafCertificate = mServerLeafCertificateField->textEdit.toPlainText().toUtf8(),

            .clientCertificateEnabled = mClientCertificateCheckBox->isChecked(),
            .clientCertificate = mClientCertificateField->textEdit.toPlainText().toUtf8(),

            .authentication = mAuthenticationGroupBox->isChecked(),
            .username = mUsernameLineEdit->text(),
            .password = mPasswordLineEdit->text(),

            .updateInterval = mUpdateIntervalSpinBox->value(),
            .timeout = mTimeoutSpinBox->value(),

            .autoReconnectEnabled = mAutoReconnectGroupBox->isChecked(),
            .autoReconnectInterval = mAutoReconnectSpinBox->value(),
        };

        if (mServersModel) {
            mServersModel->setServer(
                mServerName,
                mNameLineEdit->text(),
                std::move(connectionConfiguration),
                std::move(mountedDirectories)
            );
        } else {
            Servers::instance()
                ->setServer(mServerName, mNameLineEdit->text(), connectionConfiguration, std::move(mountedDirectories));
        }
    }
}

#include "servereditdialog.moc"
