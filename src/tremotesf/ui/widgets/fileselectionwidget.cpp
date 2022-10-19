// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "fileselectionwidget.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QCoreApplication>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPushButton>

#include "libtremotesf/literals.h"
#include "libtremotesf/target_os.h"

namespace tremotesf {
    namespace {
        class ComboBoxViewEventFilter : public QObject {
            Q_OBJECT

        public:
            explicit ComboBoxViewEventFilter(QComboBox* comboBox) : QObject(comboBox), mComboBox(comboBox) {}

        protected:
            bool eventFilter(QObject* watched, QEvent* event) override {
                if (event->type() == QEvent::KeyPress &&
                    static_cast<QKeyEvent*>(event)->matches(QKeySequence::Delete)) {
                    const QModelIndex index(static_cast<QAbstractItemView*>(watched)->currentIndex());
                    if (index.isValid()) {
                        mComboBox->removeItem(index.row());
                        return true;
                    }
                }
                return QObject::eventFilter(watched, event);
            }

        private:
            QComboBox* mComboBox{};
        };
    }

    FileSelectionWidget::FileSelectionWidget(
        bool directory, const QString& filter, bool connectTextWithFileDialog, bool comboBox, QWidget* parent
    )
        : QWidget(parent),
          mSelectionButton(new QPushButton(QIcon::fromTheme("document-open"_l1), QString(), this)),
          mConnectTextWithFileDialog(connectTextWithFileDialog) {
        auto layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);

        if (comboBox) {
            mTextComboBox = new QComboBox(this);
            mTextComboBox->setEditable(true);
            mTextComboBox->view()->installEventFilter(new ComboBoxViewEventFilter(mTextComboBox));
            layout->addWidget(mTextComboBox, 1);
        } else {
            mTextLineEdit = new QLineEdit(this);
            layout->addWidget(mTextLineEdit, 1);
        }

        layout->addWidget(mSelectionButton);

        if (comboBox) {
            QObject::connect(mTextComboBox, &QComboBox::currentTextChanged, this, [=](const auto& text) {
                if (connectTextWithFileDialog) {
                    mFileDialogDirectory = text;
                }
                emit textChanged(text);
            });
        } else {
            QObject::connect(mTextLineEdit, &QLineEdit::textEdited, this, [=](const auto& text) {
                if (connectTextWithFileDialog) {
                    mFileDialogDirectory = text;
                }
                emit textChanged(text);
            });
        }

        QObject::connect(mSelectionButton, &QPushButton::clicked, this, [=] {
            QFileDialog* dialog{};
            if (directory) {
                dialog = new QFileDialog(this, qApp->translate("tremotesf", "Select Directory"), mFileDialogDirectory);
                dialog->setFileMode(QFileDialog::Directory);
                dialog->setOptions(QFileDialog::ShowDirsOnly);
            } else {
                dialog = new QFileDialog(this, qApp->translate("tremotesf", "Select File"), QString(), filter);
                dialog->setFileMode(QFileDialog::ExistingFile);
            }
            dialog->setAttribute(Qt::WA_DeleteOnClose);

            QObject::connect(dialog, &QFileDialog::accepted, this, [=] {
                const QString filePath(dialog->selectedFiles().constFirst());

                if (directory && connectTextWithFileDialog) {
                    mFileDialogDirectory = filePath;
                }

                if (connectTextWithFileDialog) {
                    if (comboBox) {
                        mTextComboBox->setCurrentText(filePath);
                    } else {
                        mTextLineEdit->setText(filePath);
                    }
                }

                emit fileDialogAccepted(filePath);
            });

            if constexpr (isTargetOsWindows) {
                dialog->open();
            } else {
                dialog->show();
            }
        });
    }

    QComboBox* FileSelectionWidget::textComboBox() const { return mTextComboBox; }

    QStringList FileSelectionWidget::textComboBoxItems() const {
        QStringList items;
        if (mTextComboBox) {
            items.reserve(mTextComboBox->count() + 1);
            for (int i = 0, max = mTextComboBox->count(); i < max; ++i) {
                items.push_back(mTextComboBox->itemText(i));
            }
            if (!items.contains(mTextComboBox->currentText())) {
                items.push_back(mTextComboBox->currentText());
            }
        }
        return items;
    }

    QString FileSelectionWidget::text() const {
        if (mTextLineEdit) {
            return mTextLineEdit->text();
        }
        return mTextComboBox->currentText();
    }

    void FileSelectionWidget::setText(const QString& text) {
        if (mTextLineEdit) {
            mTextLineEdit->setText(text);
        } else {
            mTextComboBox->setCurrentText(text);
        }
        if (mConnectTextWithFileDialog) {
            mFileDialogDirectory = text;
        }
    }

    QPushButton* FileSelectionWidget::selectionButton() const { return mSelectionButton; }

    void FileSelectionWidget::setFileDialogDirectory(const QString& directory) { mFileDialogDirectory = directory; }
}

#include "fileselectionwidget.moc"
