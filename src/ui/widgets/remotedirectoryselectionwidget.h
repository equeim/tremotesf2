// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_FILESELECTIONWIDGET_H
#define TREMOTESF_FILESELECTIONWIDGET_H

#include <QWidget>

class QLineEdit;
class QPushButton;

namespace tremotesf {
    class Rpc;

    class RemoteDirectorySelectionWidgetViewModel : public QObject {
        Q_OBJECT

    public:
        explicit RemoteDirectorySelectionWidgetViewModel(QString path, const Rpc* rpc, QObject* parent = nullptr);

        [[nodiscard]] QString path() const { return mPath; };
        [[nodiscard]] QString displayPath() const { return mDisplayPath; };

        [[nodiscard]] virtual bool enableFileDialog() const { return mMode != Mode::Remote; }
        [[nodiscard]] virtual QString fileDialogDirectory();

        void updatePathProgrammatically(QString path);
        void updatePathProgrammatically(QString path, QString displayPath);
        void onPathEditedByUser(const QString& text);
        void onFileDialogAccepted(QString path);

    protected:
        [[nodiscard]] QString normalizePath(const QString& path) const;
        [[nodiscard]] QString toNativeSeparators(const QString& path) const;

        const Rpc* mRpc{};

        QString mPath{};
        QString mDisplayPath{toNativeSeparators(mPath)};

        enum class Mode { Local, RemoteMounted, Remote };
        Mode mMode{};

    private:
        [[nodiscard]] bool updatePathImpl(QString path, QString displayPath);

    signals:
        void pathChanged(bool textFieldWasEdited);
        void showMountedDirectoryError();
    };

    class RemoteDirectorySelectionWidget : public QWidget {
        Q_OBJECT

    public:
        explicit RemoteDirectorySelectionWidget(QWidget* parent = nullptr);

        virtual void setup(QString path, const Rpc* rpc);
        [[nodiscard]] QString path() const { return mViewModel->path(); }
        virtual void resetPath(QString path) { mViewModel->updatePathProgrammatically(std::move(path)); }

    protected:
        virtual QWidget* createTextField();
        virtual QLineEdit* lineEditFromTextField();
        virtual RemoteDirectorySelectionWidgetViewModel* createViewModel(QString path, const Rpc* rpc);

        RemoteDirectorySelectionWidgetViewModel* mViewModel{};
        QWidget* mTextField{};
        QPushButton* mSelectDirectoryButton{};

    private:
        void showFileDialog();

    signals:
        void pathChanged();
    };
}

#endif // TREMOTESF_FILESELECTIONWIDGET_H
