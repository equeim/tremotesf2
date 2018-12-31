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

#ifndef TREMOTESF_FILESELECTIONWIDGET_H
#define TREMOTESF_FILESELECTIONWIDGET_H

#include <QWidget>

class QComboBox;
class QLineEdit;
class QPushButton;

namespace tremotesf
{
    class FileSelectionWidget : public QWidget
    {
        Q_OBJECT
    public:
        explicit FileSelectionWidget(bool directory,
                                     const QString& filter,
                                     bool connectTextWithFileDialog,
                                     bool comboBox,
                                     QWidget* parent = nullptr);

        QComboBox* textComboBox() const;
        QStringList textComboBoxItems() const;
        QString text() const;
        void setText(const QString& text);
        QPushButton* selectionButton() const;
        void setFileDialogDirectory(const QString& directory);

    private:
        QLineEdit* mTextLineEdit = nullptr;
        QComboBox* mTextComboBox = nullptr;
        QPushButton* mSelectionButton;
        bool mConnectTextWithFileDialog;
        QString mFileDialogDirectory;

    signals:
        void textChanged(const QString& text);
        void fileDialogAccepted(const QString& filePath);
    };
}

#endif // TREMOTESF_FILESELECTIONWIDGET_H
