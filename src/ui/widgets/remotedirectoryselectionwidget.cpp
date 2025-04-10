// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "remotedirectoryselectionwidget.h"

#include <QCoreApplication>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QIcon>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>

#include "target_os.h"
#include "rpc/rpc.h"
#include "rpc/servers.h"
#include "rpc/serversettings.h"
#include "ui/stylehelpers.h"

using namespace Qt::StringLiterals;

namespace tremotesf {
    RemoteDirectorySelectionWidgetViewModel::RemoteDirectorySelectionWidgetViewModel(
        QString path, const Rpc* rpc, QObject* parent
    )
        : QObject(parent),
          mRpc(rpc),
          mPath(std::move(path)),
          mMode(
              rpc->isLocal()
                  ? Mode::Local
                  : (Servers::instance()->currentServerHasMountedDirectories() ? Mode::RemoteMounted : Mode::Remote)
          ) {}

    QString RemoteDirectorySelectionWidgetViewModel::fileDialogDirectory() {
        if (mMode == Mode::RemoteMounted) {
            return Servers::instance()->fromRemoteToLocalDirectory(mPath, mRpc->serverSettings());
        }
        return mPath;
    }

    void RemoteDirectorySelectionWidgetViewModel::updatePathProgrammatically(QString path) {
        auto displayPath = toNativeSeparators(path);
        updatePathImpl(std::move(path), std::move(displayPath));
    }

    void RemoteDirectorySelectionWidgetViewModel::onPathEditedByUser(const QString& text) {
        auto displayPath = text.trimmed();
        auto path = normalizePath(displayPath);
        updatePathImpl(std::move(path), std::move(displayPath));
    }

    void RemoteDirectorySelectionWidgetViewModel::onFileDialogAccepted(QString path) {
        if (mMode != Mode::RemoteMounted) {
            updatePathProgrammatically(std::move(path));
            return;
        }
        auto remoteDirectory = Servers::instance()->fromLocalToRemoteDirectory(path, mRpc->serverSettings());
        if (remoteDirectory.isEmpty()) {
            emit showMountedDirectoryError();
        } else {
            updatePathProgrammatically(std::move(remoteDirectory));
        }
    }

    QString RemoteDirectorySelectionWidgetViewModel::normalizePath(const QString& path) const {
        return tremotesf::normalizePath(path, mRpc->serverSettings()->data().pathOs);
    }

    QString RemoteDirectorySelectionWidgetViewModel::toNativeSeparators(const QString& path) const {
        return tremotesf::toNativeSeparators(path, mRpc->serverSettings()->data().pathOs);
    }

    void RemoteDirectorySelectionWidgetViewModel::updatePathImpl(QString path, QString displayPath) {
        if (path != mPath) {
            mPath = std::move(path);
            mDisplayPath = std::move(displayPath);
            emit pathChanged();
        }
    }

    RemoteDirectorySelectionWidget::RemoteDirectorySelectionWidget(QWidget* parent) : QWidget(parent) {}

    void RemoteDirectorySelectionWidget::setup(QString path, const Rpc* rpc) {
        auto layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);

        mTextField = createTextField();
        layout->addWidget(mTextField, 1);

        mSelectDirectoryButton = new QPushButton(QIcon::fromTheme("document-open"_L1), QString(), this);
        layout->addWidget(mSelectDirectoryButton);
        mSelectDirectoryButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        if constexpr (targetOs == TargetOs::UnixMacOS) {
            if (determineStyle() == KnownStyle::macOS) {
                // Button becomes ugly if we don't set these specific values
                mSelectDirectoryButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                mSelectDirectoryButton->setMaximumSize(32, 32);
            }
        }

        mViewModel = createViewModel(std::move(path), rpc);

        mSelectDirectoryButton->setEnabled(mViewModel->enableFileDialog());
        if (mViewModel->enableFileDialog()) {
            QObject::connect(
                mSelectDirectoryButton,
                &QPushButton::clicked,
                this,
                &RemoteDirectorySelectionWidget::showFileDialog
            );
        }

        const auto lineEdit = lineEditFromTextField();
        const auto updateLineEdit = [=, this]() { lineEdit->setText(mViewModel->displayPath()); };
        updateLineEdit();
        QObject::connect(mViewModel, &RemoteDirectorySelectionWidgetViewModel::pathChanged, this, updateLineEdit);
        QObject::connect(
            mViewModel,
            &RemoteDirectorySelectionWidgetViewModel::pathChanged,
            this,
            &RemoteDirectorySelectionWidget::pathChanged
        );

        QObject::connect(lineEdit, &QLineEdit::textEdited, this, [=, this](const auto& text) {
            mViewModel->onPathEditedByUser(text);
        });

        QObject::connect(
            static_cast<RemoteDirectorySelectionWidgetViewModel*>(mViewModel),
            &RemoteDirectorySelectionWidgetViewModel::showMountedDirectoryError,
            this,
            [=, this] {
                QMessageBox::warning(
                    this,
                    //: Dialog title
                    qApp->translate("tremotesf", "Error"),
                    qApp->translate("tremotesf", "Selected directory should be inside mounted directory")
                );
            }
        );
    }

    RemoteDirectorySelectionWidgetViewModel*
    RemoteDirectorySelectionWidget::createViewModel(QString path, const Rpc* rpc) {
        return new RemoteDirectorySelectionWidgetViewModel(std::move(path), rpc, this);
    }

    QWidget* RemoteDirectorySelectionWidget::createTextField() { return new QLineEdit(this); }

    QLineEdit* RemoteDirectorySelectionWidget::lineEditFromTextField() { return qobject_cast<QLineEdit*>(mTextField); }

    void RemoteDirectorySelectionWidget::showFileDialog() {
        auto dialog = new QFileDialog(
            this,
            //: Directory chooser dialog title
            qApp->translate("tremotesf", "Select Directory"),
            mViewModel->fileDialogDirectory()
        );
        dialog->setFileMode(QFileDialog::Directory);
        dialog->setOptions(QFileDialog::ShowDirsOnly);
        dialog->setAttribute(Qt::WA_DeleteOnClose);

        QObject::connect(dialog, &QFileDialog::accepted, this, [=, this] {
            mViewModel->onFileDialogAccepted(dialog->selectedFiles().constFirst());
        });

        if constexpr (targetOs == TargetOs::Windows) {
            dialog->open();
        } else {
            dialog->show();
        }
    }
}
