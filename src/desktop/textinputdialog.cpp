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
#include <QPushButton>
#include <QVBoxLayout>

namespace tremotesf
{
    TextInputDialog::TextInputDialog(const QString& title,
                                     const QString& labelText,
                                     const QString& text,
                                     const QString& okButtonText,
                                     QWidget* parent)
        : QDialog(parent),
          mLineEdit(new QLineEdit(text, this))
    {
        setWindowTitle(title);

        auto layout = new QVBoxLayout(this);
        layout->setSizeConstraint(QLayout::SetMinAndMaxSize);

        auto label = new QLabel(labelText, this);
        label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        layout->addWidget(label);
        layout->addWidget(mLineEdit);

        auto dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        if (!okButtonText.isEmpty()) {
            dialogButtonBox->button(QDialogButtonBox::Ok)->setText(okButtonText);
        }
        QObject::connect(dialogButtonBox, &QDialogButtonBox::accepted, this, &TextInputDialog::accept);
        QObject::connect(dialogButtonBox, &QDialogButtonBox::rejected, this, &TextInputDialog::reject);

        if (text.isEmpty()) {
            dialogButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        }
        QObject::connect(mLineEdit, &QLineEdit::textChanged, this, [=](const QString& text) {
            if (text.isEmpty()) {
                dialogButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
            } else {
                dialogButtonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
            }
        });

        layout->addWidget(dialogButtonBox);
    }

    QSize TextInputDialog::sizeHint() const
    {
        return minimumSizeHint().expandedTo(QSize(256, 0));
    }

    QString TextInputDialog::text() const
    {
        return mLineEdit->text();
    }
}
