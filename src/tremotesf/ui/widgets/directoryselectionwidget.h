// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_FILESELECTIONWIDGET_H
#define TREMOTESF_FILESELECTIONWIDGET_H

#include <vector>
#include <QWidget>

#include "libtremotesf/pathutils.h"

class QComboBox;
class QLineEdit;
class QPushButton;

namespace tremotesf {
    class DirectorySelectionWidgetViewModel : public QObject {
        Q_OBJECT

    public:
        explicit DirectorySelectionWidgetViewModel(QString path, QString displayPath, QObject* parent = nullptr)
            : QObject(parent), mPath(std::move(path)), mDisplayPath(std::move(displayPath)) {}

        [[nodiscard]] QString path() const { return mPath; };
        [[nodiscard]] QString displayPath() const { return mDisplayPath; };

        [[nodiscard]] virtual bool enableFileDialog() const { return true; }
        [[nodiscard]] virtual QString fileDialogDirectory() const { return mPath; };

        [[nodiscard]] virtual bool useComboBox() const { return false; };
        struct ComboBoxItem {
            QString path{};
            QString displayPath{};
            [[nodiscard]] bool operator==(const ComboBoxItem&) const = default;
        };
        [[nodiscard]] virtual std::vector<ComboBoxItem> comboBoxItems() const { return {}; };

        void updatePath(const QString& path);

        void onPathEditedByUser(const QString& text);
        virtual void onFileDialogAccepted(const QString& path);
        void onComboBoxItemSelected(const QString& path, const QString& displayPath);

    protected:
        [[nodiscard]] virtual QString normalizePath(const QString& path) const;
        [[nodiscard]] virtual QString toNativeSeparators(const QString& path) const;

        void updatePath(const QString& path, const QString& displayPath);

        QString mPath{};
        QString mDisplayPath{};

    signals:
        void pathChanged();
        void comboBoxItemsChanged();
    };

    class DirectorySelectionWidget : public QWidget {
        Q_OBJECT

    public:
        explicit DirectorySelectionWidget(const QString& path, QWidget* parent = nullptr);

        [[nodiscard]] QString path() const { return mViewModel->path(); }

    protected:
        explicit DirectorySelectionWidget(DirectorySelectionWidgetViewModel* viewModel, QWidget* parent = nullptr);

        DirectorySelectionWidgetViewModel* mViewModel{};

    private:
        void setupPathTextField();
        void setupComboBoxItems();
        void showFileDialog();

        QLineEdit* mTextLineEdit{};
        QComboBox* mTextComboBox{};

    signals:
        void pathChanged();
    };
}

#endif // TREMOTESF_FILESELECTIONWIDGET_H
