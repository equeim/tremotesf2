// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_SETTINGSDIALOG_H
#define TREMOTESF_SETTINGSDIALOG_H

#include <QDialog>

class QCheckBox;

namespace tremotesf
{
    class SettingsDialog : public QDialog
    {
        Q_OBJECT
    public:
        explicit SettingsDialog(QWidget* parent = nullptr);
    };
}

#endif // TREMOTESF_SETTINGSDIALOG_H
