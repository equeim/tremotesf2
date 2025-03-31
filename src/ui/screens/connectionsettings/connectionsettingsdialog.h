// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_SERVERSDIALOG_H
#define TREMOTESF_SERVERSDIALOG_H

#include <QDialog>

class QListView;

namespace tremotesf {
    class ServersModel;
    class BaseProxyModel;

    class ConnectionSettingsDialog final : public QDialog {
        Q_OBJECT

    public:
        explicit ConnectionSettingsDialog(QWidget* parent = nullptr);
        void accept() override;

    private:
        void showEditDialogs();
        void removeServers();

        ServersModel* mModel;
        BaseProxyModel* mProxyModel;
        QListView* mServersView;
    };
}

#endif // TREMOTESF_SERVERSDIALOG_H
