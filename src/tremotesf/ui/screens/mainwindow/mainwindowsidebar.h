#ifndef TREMOTESF_MAINWINDOWSIDEBAR_H
#define TREMOTESF_MAINWINDOWSIDEBAR_H

#include <QScrollArea>

namespace tremotesf
{
    class Rpc;
    class TorrentsProxyModel;

    class MainWindowSideBar : public QScrollArea
    {
        Q_OBJECT
    public:
        explicit MainWindowSideBar(Rpc* rpc, TorrentsProxyModel* proxyModel, QWidget* parent = nullptr);
    };
}

#endif // TREMOTESF_MAINWINDOWSIDEBAR_H
