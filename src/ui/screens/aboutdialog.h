// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_ABOUTDIALOG_H
#define TREMOTESF_ABOUTDIALOG_H

#include <QDialog>

namespace tremotesf {
    class AboutDialog final : public QDialog {
        Q_OBJECT

    public:
        explicit AboutDialog(QWidget* parent = nullptr);
        QSize sizeHint() const override;
    };
}

#endif // TREMOTESF_ABOUTDIALOG_H
