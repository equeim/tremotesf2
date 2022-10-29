// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "directoryselectionwidget.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QCoreApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPushButton>

#include "libtremotesf/literals.h"
#include "libtremotesf/target_os.h"

namespace tremotesf {
    void DirectorySelectionWidgetViewModel::updatePath(const QString& path) {
        updatePath(path, toNativeSeparators(path));
    }

    void DirectorySelectionWidgetViewModel::onPathEditedByUser(const QString& text) {
        const auto displayPath = text.trimmed();
        const auto path = normalizePath(displayPath);
        updatePath(path, displayPath);
    }

    void DirectorySelectionWidgetViewModel::onFileDialogAccepted(const QString& path) {
        updatePath(path, toNativeSeparators(path));
    }

    void DirectorySelectionWidgetViewModel::onComboBoxItemSelected(const QString& path, const QString& displayPath) {
        updatePath(path, displayPath);
    }

    QString DirectorySelectionWidgetViewModel::normalizePath(const QString& path) const {
        return QDir::toNativeSeparators(path);
    }

    QString DirectorySelectionWidgetViewModel::toNativeSeparators(const QString& path) const {
        return QDir::fromNativeSeparators(path);
    }

    void DirectorySelectionWidgetViewModel::updatePath(const QString& path, const QString& displayPath) {
        if (path != mPath) {
            mPath = path;
            mDisplayPath = displayPath;
            emit pathChanged();
        }
    }

    namespace {
        class ComboBoxEventFilter : public QObject {
            Q_OBJECT

        public:
            explicit ComboBoxEventFilter(QComboBox* comboBox) : QObject(comboBox), mComboBox(comboBox) {}

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

    DirectorySelectionWidget::DirectorySelectionWidget(const QString& path, QWidget* parent)
        : DirectorySelectionWidget(new DirectorySelectionWidgetViewModel(path, path), parent) {}

    DirectorySelectionWidget::DirectorySelectionWidget(DirectorySelectionWidgetViewModel* viewModel, QWidget* parent)
        : QWidget(parent), mViewModel(viewModel) {
        mViewModel->setParent(this);

        auto layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);

        if (mViewModel->useComboBox()) {
            mTextComboBox = new QComboBox(this);
            layout->addWidget(mTextComboBox, 1);
            mTextComboBox->setEditable(true);
            mTextComboBox->view()->installEventFilter(new ComboBoxEventFilter(mTextComboBox));
            mTextLineEdit = mTextComboBox->lineEdit();
        } else {
            mTextLineEdit = new QLineEdit(this);
            layout->addWidget(mTextLineEdit, 1);
        }

        auto selectionButton = new QPushButton(QIcon::fromTheme("document-open"_l1), QString(), this);
        layout->addWidget(selectionButton);
        selectionButton->setEnabled(mViewModel->enableFileDialog());
        if (mViewModel->enableFileDialog()) {
            QObject::connect(selectionButton, &QPushButton::clicked, this, &DirectorySelectionWidget::showFileDialog);
        }

        setupComboBoxItems();
        setupPathTextField();

        QObject::connect(
            mViewModel,
            &DirectorySelectionWidgetViewModel::pathChanged,
            this,
            &DirectorySelectionWidget::pathChanged
        );
    }

    void DirectorySelectionWidget::setupPathTextField() {
        const auto onPathChanged = [=]() { mTextLineEdit->setText(mViewModel->displayPath()); };
        onPathChanged();
        QObject::connect(mViewModel, &DirectorySelectionWidgetViewModel::pathChanged, this, onPathChanged);

        QObject::connect(mTextLineEdit, &QLineEdit::textEdited, this, [=](const auto& text) {
            mViewModel->onPathEditedByUser(text);
        });

        if (mTextComboBox) {
            QObject::connect(mTextComboBox, qOverload<int>(&QComboBox::activated), this, [=](int index) {
                if (index != -1) {
                    mViewModel->onComboBoxItemSelected(
                        mTextComboBox->itemData(index).toString(),
                        mTextComboBox->itemText(index)
                    );
                }
            });
        }
    }

    void DirectorySelectionWidget::setupComboBoxItems() {
        if (!mTextComboBox) return;
        const auto setup = [=] {
            const auto items = mViewModel->comboBoxItems();
            mTextComboBox->clear();
            for (const auto& [path, displayPath] : items) {
                mTextComboBox->addItem(displayPath, path);
            }
            mTextLineEdit->setText(mViewModel->displayPath());
        };
        setup();
        QObject::connect(mViewModel, &DirectorySelectionWidgetViewModel::comboBoxItemsChanged, this, setup);
    }

    void DirectorySelectionWidget::showFileDialog() {
        auto dialog =
            new QFileDialog(this, qApp->translate("tremotesf", "Select Directory"), mViewModel->fileDialogDirectory());
        dialog->setFileMode(QFileDialog::Directory);
        dialog->setOptions(QFileDialog::ShowDirsOnly);
        dialog->setAttribute(Qt::WA_DeleteOnClose);

        QObject::connect(dialog, &QFileDialog::accepted, this, [=] {
            mViewModel->onFileDialogAccepted(dialog->selectedFiles().constFirst());
        });

        if constexpr (isTargetOsWindows) {
            dialog->open();
        } else {
            dialog->show();
        }
    }
}

#include "directoryselectionwidget.moc"
