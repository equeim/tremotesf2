// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_MAINWINDOW_H
#define TREMOTESF_MAINWINDOW_H

#include <memory>
#include <QMainWindow>

namespace tremotesf {
    class IpcServer;

    class MainWindow final : public QMainWindow {
        Q_OBJECT

    public:
        MainWindow(QStringList&& commandLineFiles, QStringList&& commandLineUrls, QWidget* parent = nullptr);
        ~MainWindow() override;
        Q_DISABLE_COPY_MOVE(MainWindow)

        QSize sizeHint() const override;
        void initialShow(bool minimized);

    protected:
        bool event(QEvent* event) override;
        void showEvent(QShowEvent* event) override;
        void hideEvent(QHideEvent* event) override;
        void closeEvent(QCloseEvent* event) override;
        void dragEnterEvent(QDragEnterEvent* event) override;
        void dropEvent(QDropEvent* event) override;

    private:
        class Impl;
        std::unique_ptr<Impl> mImpl;
    };
}

#endif // TREMOTESF_MAINWINDOW_H
