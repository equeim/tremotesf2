// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_TEXTINPUTDIALOG_H
#define TREMOTESF_TEXTINPUTDIALOG_H

#include <QDialog>

class QLineEdit;
class QPlainTextEdit;

namespace tremotesf {
    class TextInputDialog final : public QDialog {
        Q_OBJECT

    public:
        explicit TextInputDialog(
            const QString& title,
            const QString& labelText,
            const QString& text,
            const QString& okButtonText,
            bool multiline,
            QWidget* parent = nullptr
        );
        QString text() const;

    private:
        QLineEdit* mLineEdit = nullptr;
        QPlainTextEdit* mPlainTextEdit = nullptr;
    };
}

#endif
