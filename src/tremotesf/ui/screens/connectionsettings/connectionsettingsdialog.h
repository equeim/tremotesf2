// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_SERVERSDIALOG_H
#define TREMOTESF_SERVERSDIALOG_H

#include <QDialog>

class QListView;
class KMessageWidget;

namespace tremotesf
{
    class ServersModel;
    class BaseProxyModel;

    class ConnectionSettingsDialog : public QDialog
    {
        Q_OBJECT

    public:
        explicit ConnectionSettingsDialog(QWidget* parent = nullptr);
        QSize sizeHint() const override;
        void accept() override;

    private:
        void showEditDialogs();
        void removeServers();

    private:
        KMessageWidget* mNoServersWidget;
        ServersModel* mModel;
        BaseProxyModel* mProxyModel;
        QListView* mServersView;
    };
}

#endif // TREMOTESF_SERVERSDIALOG_H
