// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_FILESELECTIONWIDGET_H
#define TREMOTESF_FILESELECTIONWIDGET_H

#include <QWidget>

class QComboBox;
class QLineEdit;
class QPushButton;

namespace tremotesf {
    class FileSelectionWidget : public QWidget {
        Q_OBJECT
    public:
        explicit FileSelectionWidget(
            bool directory,
            const QString& filter,
            bool connectTextWithFileDialog,
            bool comboBox,
            QWidget* parent = nullptr
        );

        QComboBox* textComboBox() const;
        QStringList textComboBoxItems() const;
        QString text() const;
        void setText(const QString& text);
        QPushButton* selectionButton() const;
        void setFileDialogDirectory(const QString& directory);

    private:
        QLineEdit* mTextLineEdit{};
        QComboBox* mTextComboBox{};
        QPushButton* mSelectionButton{};
        bool mConnectTextWithFileDialog{};
        QString mFileDialogDirectory{};

    signals:
        void textChanged(const QString& text);
        void fileDialogAccepted(const QString& filePath);
    };
}

#endif // TREMOTESF_FILESELECTIONWIDGET_H
