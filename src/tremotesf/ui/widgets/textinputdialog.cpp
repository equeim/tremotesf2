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

#include "textinputdialog.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace tremotesf
{
    TextInputDialog::TextInputDialog(const QString& title,
                                     const QString& labelText,
                                     const QString& text,
                                     const QString& okButtonText,
                                     bool multiline,
                                     QWidget* parent)
        : QDialog(parent)
    {
        setWindowTitle(title);

        auto layout = new QVBoxLayout(this);
        layout->setSizeConstraint(QLayout::SetMinAndMaxSize);

        auto label = new QLabel(labelText, this);
        label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        layout->addWidget(label);

        if (multiline) {
            mPlainTextEdit = new QPlainTextEdit(text, this);
            layout->addWidget(mPlainTextEdit);
        } else {
            mLineEdit = new QLineEdit(text, this);
            layout->addWidget(mLineEdit);
        }

        auto dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        if (!okButtonText.isEmpty()) {
            dialogButtonBox->button(QDialogButtonBox::Ok)->setText(okButtonText);
        }
        QObject::connect(dialogButtonBox, &QDialogButtonBox::accepted, this, &TextInputDialog::accept);
        QObject::connect(dialogButtonBox, &QDialogButtonBox::rejected, this, &TextInputDialog::reject);

        if (text.isEmpty()) {
            dialogButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        }

        const auto onTextChanged = [=](const QString& text) {
            if (text.isEmpty()) {
                dialogButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
            } else {
                dialogButtonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
            }
        };

        if (multiline) {
            QObject::connect(mPlainTextEdit, &QPlainTextEdit::textChanged, this, [=] {
                onTextChanged(mPlainTextEdit->toPlainText());
            });
        } else {
            QObject::connect(mLineEdit, &QLineEdit::textChanged, this, onTextChanged);
        }

        layout->addWidget(dialogButtonBox);

        setMinimumSize(minimumSizeHint());
    }

    QSize TextInputDialog::sizeHint() const
    {
        return minimumSizeHint().expandedTo(QSize(256, 0));
    }

    QString TextInputDialog::text() const
    {
        return mLineEdit ? mLineEdit->text() : mPlainTextEdit->toPlainText();
    }
}
