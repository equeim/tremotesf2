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

#include "fileselectionwidget.h"

#include <QCoreApplication>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QIcon>
#include <QLineEdit>
#include <QPushButton>

namespace tremotesf
{
    FileSelectionWidget::FileSelectionWidget(bool directory,
                                             const QString& filter,
                                             bool connectLineEditWithDialog,
                                             QWidget* parent)
        : QWidget(parent),
          mLineEdit(new QLineEdit(this)),
          mSelectionButton(new QPushButton(QIcon::fromTheme(QLatin1String("document-open")), QString(), this)),
          mConnectLineEditWithDialog(connectLineEditWithDialog)
    {
        auto layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(mLineEdit);
        layout->addWidget(mSelectionButton);

        if (connectLineEditWithDialog) {
            QObject::connect(mLineEdit, &QLineEdit::textEdited, this, [=](const QString& text) {
                mFileDialogDirectory = text;
            });
        }

        QObject::connect(mSelectionButton, &QPushButton::clicked, this, [=]() {
            QFileDialog* dialog;
            if (directory) {
                dialog = new QFileDialog(this, qApp->translate("tremotesf", "Select Directory"), mFileDialogDirectory);
                dialog->setFileMode(QFileDialog::Directory);
                dialog->setOptions(QFileDialog::ShowDirsOnly);
            } else {
                dialog = new QFileDialog(this, qApp->translate("tremotesf", "Select File"), QString(), filter);
                dialog->setFileMode(QFileDialog::ExistingFile);
            }
            dialog->setAttribute(Qt::WA_DeleteOnClose);

            QObject::connect(dialog, &QFileDialog::accepted, this, [=]() {
                const QString filePath(dialog->selectedFiles().constFirst());

                if (directory && connectLineEditWithDialog) {
                    mFileDialogDirectory = filePath;
                }

                if (connectLineEditWithDialog) {
                    mLineEdit->setText(filePath);
                }

                emit fileDialogAccepted(filePath);
            });

#ifdef Q_OS_WIN
            dialog->open();
#else
            dialog->show();
#endif
        });
    }

    QLineEdit* FileSelectionWidget::lineEdit() const
    {
        return mLineEdit;
    }

    void FileSelectionWidget::setLineEditText(const QString& text)
    {
        mLineEdit->setText(text);
        if (mConnectLineEditWithDialog) {
            mFileDialogDirectory = text;
        }
    }

    QPushButton* FileSelectionWidget::selectionButton() const
    {
        return mSelectionButton;
    }

    void FileSelectionWidget::setFileDialogDirectory(const QString& directory)
    {
        mFileDialogDirectory = directory;
    }
}
